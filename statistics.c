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
    struct stat_item *items;
    struct cpu_stat *cpu_stats;
};


