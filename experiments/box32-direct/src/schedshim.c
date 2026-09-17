/* i386 diagnostic shim for the Box32 pthread_setschedprio deadlock. */
typedef unsigned long pthread_t;
int pthread_setschedprio(pthread_t thread, int priority) {
    (void)thread;
    (void)priority;
    return 0;
}
