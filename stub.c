#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dlist.h"
#include "statistics.h"

#define log(fmt, ...) \
    printf("[libstat] "fmt" [%s():%d]\n", ##__VA_ARGS__, __FUNCTION__, __LINE__)


void *stat_routine(void *data)
{
    int cpu_id = (int)(long)data;
    int round = cpu_id + 1;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);

    pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);

    while (round--) {
        STATISTICS_INC(inc);
        log("inc cpu_id %d sum %lu cpu %lu thread %lu", sched_getcpu(),
            STATISTICS_SUM(inc), STATISTICS_SUM_CPU(inc), STATISTICS_SUM_THREAD(inc));

        STATISTICS_INC(add);
        log("add cpu_id %d sum %lu cpu %lu thread %lu", sched_getcpu(),
            STATISTICS_SUM(add), STATISTICS_SUM_CPU(add), STATISTICS_SUM_THREAD(add));

        sleep(1);
    }
}

int main()
{
    int i;
    int cpus;
    pthread_t *pids;

    cpus = sysconf(_SC_NPROCESSORS_CONF);
    pids = (pthread_t *)malloc(sizeof(pthread_t) * cpus);
    if (pids == NULL) {
        log("malloc pids fails. %d:%s", errno, strerror(errno));
        return -1;
    }

    for (i = 0; i < cpus; i++) {
        pthread_create(pids + i, NULL, stat_routine, (void *)(long)i);
    }

    for (i = 0; i < cpus; i++) {
        pthread_join(pids[i], NULL);
    }

    log ("stat finish.");
    return 0;
}
