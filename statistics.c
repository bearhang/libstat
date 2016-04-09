#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "dlist.h"
#include "statistics.h"

#define DEFAULT_ITEM_LEN 1024
#define DEFAULT_HASH_TABLE_LEN (DEFAULT_ITEM_LEN  << 1)
#define MAX_KEY_LEN 16

#ifdef DEBUG
#define log(fmt, ...) \
    printf("[libstat] "fmt" [%s():%d]\n", ##__VA_ARGS__, __FUNCTION__, __LINE__)
#else
#define log(fmt, ...)
#endif

struct stat_item {
    struct dlist_head hash_link;
    char key[MAX_KEY_LEN];
    int key_id;
};

struct thread_stat {
    struct dlist_head thread_link;
    size_t items[0];
};

struct cpu_stat {
    pthread_mutex_t thread_lock;
    struct dlist_head thread_list;
};

struct root_stat {
    int cpu_count;
    int htable_len;
    int item_len;
    int item_next;
    pthread_key_t item_key;
    pthread_mutex_t item_lock;
    struct stat_item *items;
    struct cpu_stat *cpu_stats;
    struct dlist_head *item_htable;
};

static struct root_stat *g_root;
static pthread_once_t g_root_once = PTHREAD_ONCE_INIT;
static int g_item_len = DEFAULT_ITEM_LEN;
static int g_htable_len = DEFAULT_HASH_TABLE_LEN;

// copy from fs/ext3
static unsigned int stat_hash_key(const char *name, int len)
{
	unsigned int hash;
    unsigned int hash0 = 0x12a3fe2d;
    unsigned int hash1 = 0x37abe8f9;
	const signed char *scp = (const signed char *) name;

	while (len--) {
		hash = hash1 + (hash0 ^ (((int) *scp++) * 7152373));

		if (hash & 0x80000000)
			hash -= 0x7fffffff;
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}

static void stat_root_init(void)
{
    int i;
    struct cpu_stat *cpu;

    g_root = (struct root_stat *)malloc(sizeof(*g_root));
    assert(g_root != NULL);

    g_root->cpu_count = sysconf(_SC_NPROCESSORS_CONF);
    g_root->htable_len = g_htable_len;
    g_root->item_len = g_item_len;
    g_root->item_next = 0;
    pthread_key_create(&g_root->item_key, NULL);
    pthread_mutex_init(&g_root->item_lock, NULL);

    g_root->items =
        (struct stat_item *)malloc(sizeof(struct stat_item) * g_root->item_len);
    assert(g_root->items != NULL);
    for (i = 0; i < g_root->item_len, i++) {
        g_root->items[i].key_id = i;
    }

    g_root->cpu_stats =
        (struct cpu_stat *)malloc(sizeof(struct cpu_stat) * g_root->cpu_count);
    assert(g_root->cpu_stats != NULL);
    for (i = 0; i < g_root->cpu_count; i++) {
        cpu = g_root->cpu_stats + i;
        pthread_mutex_init(&cpu->thread_lock, NULL);
        dlist_init(&cpu->thread_list);
    }

    g_root->item_htable =
        (struct dlist_head *)malloc(sizeof(struct dlist_head) * g_root->htable_len);
    assert(g_root->item_htable != NULL);
    for (i = 0; i < g_root->htable_len; i++) {
        dlist_init(g_root->item_htable + i);
    }
}

void stat_init(int item_len, int htable_len)
{
    g_item_len = item_len;
    g_htable_len = htable_len;
    pthread_once(&g_root_once, stat_root_init);
}

static inline struct root_stat *stat_get_root(void)
{
    pthread_once(&g_root_once, stat_root_init);
    return g_root;
}

static struct thread_stat *stat_set_thread(struct root_stat *root)
{
    int cpu_id;
    size_t size;
    struct thread_stat *thread;
    struct cpu_stat *cpu;

    size = sizeof(struct thread_stat) + sizeof(size_t) * root->item_len;
    thread = (struct thread_stat *)calloc(size, 1);
    assert(thread != NULL);

    pthread_setspecific(root->item_key, thread);

    cpu_id = sched_getcpu();
    cpu = root->cpu_stats + cpu_id;
    pthread_mutex_lock(&cpu->thread_lock);
    dlist_add_tail(&cpu->thread_list, &thread->thread_link);
    pthread_mutex_unlock(&cpu->thread_lock);

    return thread;
}

static struct thread_stat *stat_get_thread(struct root_stat *root)
{
    struct thread_stat *thread;

    thread = pthread_getspecific(root->item_key);
    if (thread == NULL)
        thread = stat_set_thread(root);

    return thread;
}

static void stat_htable_add(struct root_stat *root, struct stat_item *item)
{
    unsigned int hash;
    struct dlist_head *bucket;

    hash = stat_hash_key(item->key, strlen(item->key)) % root->htable_len;
    bucket = root->item_htable + hash;
    dlist_add_tail(bucket, &item->hash_link);
}

static struct stat_item * stat_htable_find(struct root_stat *root, char *key)
{
    unsigned int hash;
    struct dlist_head *bucket;
    struct stat_item *item;

    hash = stat_hash_key(key, strlen(key)) % root->htable_len;
    bucket = root->item_htable + hash;

    dlist_for_each_entry(item, bucket, hash_link) {
        if (strncmp(item->key, key, MAX_KEY_LEN) == 0) {
            return item;
        }
    }

    return NULL;
}

int stat_key_to_id(struct root_stat *root, char *key)
{
    struct stat_item *item;

    item = stat_htable_find(root, key);
    if (item == NULL) {
        return -1;
    }

    return item->key_id;
}

void stat_key_init(char *key, int *key_id)
{
    struct root_stat *root;
    struct stat_item *item;

    root = stat_get_root();
    assert(root->item_next < root->item_len);

    pthread_mutex_lock(&root->item_lock);
    if (*key_id != -1)
        goto unlock;

    *key_id = root->item_next++;

    item = root->items + *key_id;
    strncpy(item->key, key, MAX_KEY_LEN);
    item->key[MAX_KEY_LEN - 1] = '\0';

    stat_htable_add(root, item);

unlock:
    pthread_mutex_unlock(&root->item_lock);
}

void stat_inc(int key_id)
{
    struct root_stat *root;
    struct thread_stat *thread;

    root = stat_get_root();
    assert(key_id >= 0 && key_id < root->item_next);

    thread = stat_get_thread(root);
    thread->items[key_id]++;
}

void stat_add(int key_id, size_t cnt)
{
    struct root_stat *root;
    struct thread_stat *thread;

    root = stat_get_root();
    assert(key_id >= 0 && key_id < root->item_next);

    thread = stat_get_thread(root);
    thread->items[key_id] += cnt;
}

static inline size_t _stat_sum_thread(struct stat_root *root, int key_id)
{
    struct thread_stat *thread;

    assert(key_id >= 0 && key_id < root->item_next);

    thread = stat_get_thread(root);
    return thread->items[key_id];
}

size_t stat_sum_thread_key(char *key)
{
    int key_id;
    struct root_stat *root;

    root = stat_get_root();
    key_id = stat_key_to_id(root, key);

    return _stat_sum_thread(root, key_id);
}

size_t stat_sum_thread_id(int key_id)
{
    struct root_stat *root;

    root = stat_get_root();

    return _stat_sum_thread(root, key_id);
}

static inline size_t __stat_sum_cpu(struct root_stat *root,
        int key_id, int cpu_id)
{
    size_t sum = 0;
    struct cpu_stat *cpu;
    struct thread_stat *thread;

    cpu = root->cpu_stats + cpu_id;

    // lock?
    dlist_for_each_entry(thread, &cpu->thread_list, thread_link) {
        sum += thread->items[key_id];
    }

    return sum;
}

static inline size_t _stat_sum_cpu(struct stat_root *root, int key_id)
{
    int cpu_id;

    assert(key_id >= 0 && key_id < root->item_next);

    cpu_id = sched_getcpu();

    return __stat_sum_cpu(root, key_id, cpu_id);
}

size_t stat_sum_cpu_key(char *key)
{
    int key_id
    struct root_stat *root;

    root = stat_get_root();
    key_id = stat_key_to_id(root, key);

    return _stat_sum_cpu(root, key_id);
}

size_t stat_sum_cpu_id(int key_id)
{
    struct root_stat *root;

    root = stat_get_root();

    return _stat_sum_cpu(root, key_id);
}

static inline size_t _stat_sum(struct root_stat *root, int key_id)
{
    int i;
    size_t sum = 0;

    assert(key_id >= 0 && key_id < root->item_next);

    for (i = 0; i < root->cpu_count; i++) {
        sum += __stat_sum_cpu(root, key_id, i);
    }

    return sum;
}

size_t stat_sum_key(char *key)
{
    int key_id;
    struct root_stat *root;

    root = stat_get_root();
    key_id = stat_key_to_id(root, key);

    return _stat_sum(root, key_id);
}

size_t stat_sum_id(int key_id)
{
    struct root_stat *root;

    root = stat_get_root();

    return _stat_sum(root, key_id);
}
