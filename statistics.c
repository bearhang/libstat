#include <stdio.h>
#include <pthread.h>
#include <assert.h>

#include "dlist.h"
#include "statistics.h"

#define DEFAULT_ITEM_LEN 1024
#define MAX_KEY_LEN 16

struct stat_item {
    char key[MAX_KEY_LEN];
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
    int item_len;
    int item_next;
    pthread_key_t item_key;
    pthread_mutex_t item_lock;
    struct stat_item *items;
    struct cpu_stat *cpu_stats;
};

static struct root_stat *g_root;
static pthread_once_t *g_root_once = PTHREAD_ONCE_INIT;
static int g_item_len = DEFAULT_ITEM_LEN;

static void stat_root_init(void)
{
    int i;
    struct cpu_stat *cstat;

    g_root = (struct root_stat *)malloc(sizeof(*g_root));
    assert(g_root != NULL);

    g_root->cpu_count = sysconf(_SC_NPROCESSORS_CONF);
    g_root->item_len = g_item_len;
    g_root->item_next = 0;
    pthread_key_create(&g_root->item_key, NULL);
    pthread_mutex_init(&g_root->item_lock, NULL);

    g_root->items =
        (struct stat_item *)malloc(sizeof(struct stat_item) * g_root->item_len);
    assert(g_root->items != NULL);

    g_root->cpu_stats =
        (struct cpu_stat *)malloc(sizeof(struct cpu_stat) * g_root->cpu_count);
    assert(g_root->cpu_stats != NULL);
    for (i = 0; i < g_root->cpu_count; i++) {
        cstat = g_root->cpu_stats + i;
        pthread_mutex_init(&cstat->thread_lock, NULL);
        dlist_init(&cstat->thread_list);
    }
}

void stat_init(int item_len)
{
    g_item_len = item_len;
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
    pthrad_mutex_unlokc(&cpu->thread_lock);

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

unlock:
    pthread_mutex_unlock(&root->item_lock);
}

int stat_key_to_id(char *key)
{
    return 0;
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

static inline size_t _stat_sum_thread(int key_id)
{
    struct root_stat *root;
    struct thread_stat *thread;

    root = stat_get_root();
    assert(key_id >= 0 && key_id < root->item_next);

    thread = stat_get_thread(root);
    return thread->items[key_id];
}

size_t stat_sum_thread_key(char *key)
{
    int key_id = stat_key_to_id(key);
    return _stat_sum_thread(key_id);
}

size_t stat_sum_thread_id(int key_id)
{
    return _stat_sum_thread(key_id);
}

static inlien size_t __stat_sum_cpu(struct root_stat *root,
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

static inline size_t _stat_sum_cpu(int key_id)
{
    int cpu_id;
    struct root_stat *root;

    root = stat_get_root();
    assert(key_id >= 0 && key_id < root->item_next);

    cpu_id = sched_getcpu();

    return __stat_sum_cpu(root, key_id, cpu_id);
}

size_t stat_sum_cpu_key(char *key)
{
    int key_id = stat_key_to_id(key);
    return _stat_sum_cpu(key_id);
}

size_t stat_sum_cpu_id(int key_id)
{
    return _stat_sum_cpu(key_id);
}

static inline size_t _stat_sum(int key_id)
{
    int i;
    size_t sum = 0;
    struct root_stat *root;

    root = stat_get_root();
    assert(key_id >= 0 && key_id < root->item_next);

    for (i = 0; i < root->cpu_count; i++) {
        sum += __stat_sum_cpu(root, key_id, cpu_id);
    }

    return sum;
}

size_t stat_sum_key(char *key)
{
    int key_id = stat_key_to_id(key);
    return _stat_sum(key_id);
}

size_t stat_sum_id(int key_id)
{
    return _stat_sum(key_id);
}
