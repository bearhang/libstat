#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#ifdef __cplusplus
extern "C" {
#endif

size_t stat_sum_thread_key(char *key);
size_t stat_sum_thread_id(int key_id);
size_t stat_sum_cpu_key(char *key);
size_t stat_sum_cpu_id(int key_id);
size_t stat_sum_key(char *key);
size_t stat_sum_id(int key_id);

#define KEY_ID(key) __##key##id

#define STATISTICS_INC(key) do {    \
    static int KEY_ID(key) = -1;    \
    if (KEY_ID(key) == -1) {    \
        stat_key_init(#key, &KEY_ID(key));  \
    }   \
    stat_inc(KEY_ID(key));  \
} while (0)

#define STATISTICS_ADD(key, cnt) do {    \
    static int KEY_ID(key) = -1;    \
    if (KEY_ID(key) == -1) {    \
        stat_key_init(#key, &KEY_ID(key));  \
    }   \
    stat_add(KEY_ID(key), cnt);  \
} while (0)

#define STATISTICS_SUM(key) stat_sum_key(#key)
#define STATISTICS_SUM_CPU(key) stat_sum_cpu_key(#key)
#define STATISTICS_SUM_THREAD(key) stat_sum_thread_key(#key)

#ifdef __cplusplus
}
#endif

#endif
