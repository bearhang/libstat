#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_ID(key) __##key##id

#define STATISTICS_INC(key) do {    \
    static int KEY_ID(key) = -1;    \
    if (KEY_ID(key) == -1) {    \
        stat_init_key(#key, &KEY_ID(key));  \
    }   \
    stat_inc(KEY_ID(key));  \
} while (0)

#define STATISTICS_ADD(key, cnt) do {    \
    static int KEY_ID(key) = -1;    \
    if (KEY_ID(key) == -1) {    \
        stat_init_key(#key, &KEY_ID(key));  \
    }   \
    stat_add(KEY_ID(key), cnt);  \
} while (0)

#define STATISTICS_SUM(key)
#define STATISTICS_SUM_CPU(key)
#define STATISTICS_READ(key)

#ifdef __cplusplus
}
#endif

#endif
