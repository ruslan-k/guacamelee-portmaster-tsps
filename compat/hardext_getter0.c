#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define GL_MAX_COLOR_ATTACHMENTS 0x8CDFu

typedef void (*get_hw_fn)(int);
typedef void (*get_int_fn)(unsigned, int *);

static void *gl4es_handle(void)
{
    const char *path = getenv("GUACAMELEE_GL4ES_PATH");
    if (!path || !*path) path = getenv("SDL_VIDEO_GL_DRIVER");
    if (!path || !*path) path = "libGL.so.1";
    return dlopen(path, RTLD_NOLOAD | RTLD_NOW | RTLD_LOCAL);
}

__attribute__((constructor))
static void clear_preload_for_children(void)
{
    unsetenv("LD_PRELOAD");
}

__attribute__((visibility("default")))
void GetHardwareExtensions(int notest)
{
    static void *h;
    static get_hw_fn real_fn;
    static int *hardext;
    if (!h) h = gl4es_handle();
    if (!h) return;
    if (!real_fn) real_fn = (get_hw_fn)dlsym(h, "GetHardwareExtensions");
    if (!real_fn) return;
    real_fn(notest);
    hardext = (int *)dlsym(h, "hardext");
    if (hardext && notest && hardext[38] == 2 && hardext[13] != 0 && hardext[42] < 1)
        hardext[42] = 1;
    fprintf(stderr, "GUA-HARDEXT getter0 maxcolorattach=1 internal, game getter=0\n");
}

__attribute__((visibility("default")))
void glGetIntegerv(unsigned pname, int *params)
{
    static void *h;
    static get_int_fn real_fn;
    if (!h) h = gl4es_handle();
    if (!real_fn && h) real_fn = (get_int_fn)dlsym(h, "glGetIntegerv");
    if (real_fn) real_fn(pname, params);
    if (pname == GL_MAX_COLOR_ATTACHMENTS && params) {
        *params = 0;
        fprintf(stderr, "GUA-HARDEXT getter0 pname=0x%x value=0\n", pname);
    }
}
