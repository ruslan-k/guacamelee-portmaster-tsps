#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <time.h>

typedef struct SDL_Window SDL_Window;
typedef void (*swap_fn)(SDL_Window *);

void SDL_GL_SwapWindow(SDL_Window *window) {
    static swap_fn real_swap;
    static unsigned count;
    struct timespec ts;
    if (!real_swap) real_swap = (swap_fn)dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
    clock_gettime(CLOCK_MONOTONIC, &ts);
    fprintf(stderr, "GUA-SWAP t=%lld.%03ld n=%u window=%p\n", (long long)ts.tv_sec, ts.tv_nsec/1000000L, ++count, (void*)window);
    fflush(stderr);
    if (real_swap) real_swap(window);
}
