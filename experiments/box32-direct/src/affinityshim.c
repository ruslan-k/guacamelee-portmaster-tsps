#include <pthread.h>

/* Game-scoped diagnostic workaround.
 * Guacamelee's affinity masks are valid on TSPS, but Box64's native
 * attr mutation makes the following wrapped pthread_create return EINVAL.
 * Do not mutate the attribute; leave host-default affinity in place. */
int pthread_attr_setaffinity_np(void *attr, unsigned int cpusetsize,
                                const void *cpuset)
{
    (void)attr;
    (void)cpusetsize;
    (void)cpuset;
    return 0;
}
