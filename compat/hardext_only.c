#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define HARDEXT_FBO_INDEX 13
#define HARDEXT_ESVERSION_INDEX 38
#define HARDEXT_MAXCOLORATTACH_INDEX 42
#define HARDEXT_MAXDRAWBUFFERS_INDEX 43

typedef void (*get_hardware_extensions_fn)(int);

static void *loaded_gl4es(void)
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
    static void *handle;
    static get_hardware_extensions_fn real_fn;
    static int logged;
    int *hardext;
    int before;

    if (!handle) handle = loaded_gl4es();
    if (!handle) {
        fprintf(stderr, "GUA-HARDEXT-ONLY FAIL dlopen\\n");
        return;
    }
    if (!real_fn) real_fn = (get_hardware_extensions_fn)dlsym(handle, "GetHardwareExtensions");
    if (!real_fn) {
        fprintf(stderr, "GUA-HARDEXT-ONLY FAIL symbol\\n");
        return;
    }
    real_fn(notest);
    hardext = (int *)dlsym(handle, "hardext");
    if (!hardext) {
        fprintf(stderr, "GUA-HARDEXT-ONLY FAIL hardext\\n");
        return;
    }
    before = hardext[HARDEXT_MAXCOLORATTACH_INDEX];
    if (notest && hardext[HARDEXT_ESVERSION_INDEX] == 2 &&
        hardext[HARDEXT_FBO_INDEX] != 0 && before < 1)
        hardext[HARDEXT_MAXCOLORATTACH_INDEX] = 1;
    if (!logged) {
        fprintf(stderr,
                "GUA-HARDEXT-ONLY notest=%d es=%d fbo=%d maxcolorattach=%d->%d maxdrawbuffers=%d\\n",
                notest, hardext[HARDEXT_ESVERSION_INDEX],
                hardext[HARDEXT_FBO_INDEX], before,
                hardext[HARDEXT_MAXCOLORATTACH_INDEX],
                hardext[HARDEXT_MAXDRAWBUFFERS_INDEX]);
        logged = 1;
    }
}
