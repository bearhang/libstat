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

void stat_root_init(int item_len)
{
    int i;
    struct cpu_stat *cstat;

    g_root = (struct root_stat *)malloc(sizeof(*g_root));
    assert(g_root != NULL);

    g_root->cpu_count = sysconf(_SC_NPROCESSORS_CONF);
    g_root->item_len = item_len;
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

static inline struct root_stat *stat_get_root(void)
{
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

void stat_init_key(char *key, int *key_id)
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
