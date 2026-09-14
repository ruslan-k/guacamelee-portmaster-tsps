#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define HARDEXT_FBO_INDEX 13
#define HARDEXT_ESVERSION_INDEX 38
#define HARDEXT_MAXCOLORATTACH_INDEX 42
#define HARDEXT_MAXDRAWBUFFERS_INDEX 43

typedef void (*get_hardware_extensions_fn)(int);

static void *gl4es_handle(void)
{
    const char *path = getenv("GUACAMELEE_GL4ES_PATH");
    void *handle;

    if (!path || !*path)
        path = getenv("SDL_VIDEO_GL_DRIVER");
    if (!path || !*path)
        path = "libGL.so.1";

    handle = dlopen(path, RTLD_NOLOAD | RTLD_NOW | RTLD_LOCAL);
    if (!handle)
        fprintf(stderr, "GUA-HARDEXT FAIL dlopen %s: %s\\n", path, dlerror());
    return handle;
}

static int frontend_diag_enabled(void)
{
    const char *v = getenv("GUACAMELEE_FBO_LIFECYCLE");
    return v && *v && v[0] != '0';
}

static int frontend_rb_size_fix_enabled(void)
{
    const char *v = getenv("GUACAMELEE_FRONTEND_RB_SIZE_FIX");
    return v && *v && v[0] != '0';
}

__attribute__((constructor))
static void clear_preload_for_children(void)
{
    unsetenv("LD_PRELOAD");
}

static unsigned frontend_diag_count;

__attribute__((visibility("default")))
void glRenderbufferStorage(unsigned target, unsigned internalformat,
                           int width, int height)
{
    typedef void (*fn_t)(unsigned, unsigned, int, int);
    static fn_t real_fn;
    static void *handle;

    if (!handle)
        handle = gl4es_handle();
    if (!real_fn && handle)
        real_fn = (fn_t)dlsym(handle, "glRenderbufferStorage");
    if (frontend_rb_size_fix_enabled() && width == 0 && height == 0) {
        const char *sw = getenv("GUACAMELEE_FRONTEND_WIDTH");
        const char *sh = getenv("GUACAMELEE_FRONTEND_HEIGHT");
        width = (sw && *sw) ? atoi(sw) : 1024;
        height = (sh && *sh) ? atoi(sh) : 768;
        fprintf(stderr,
                "GUA-FRONTEND rb-size 0x0 -> %dx%d fmt=0x%x\\n",
                width, height, internalformat);
    }
    if (frontend_diag_enabled() && frontend_diag_count < 128)
        fprintf(stderr,
                "GUA-HARDEXT frontend rb-storage#%u target=0x%x "
                "fmt=0x%x size=%dx%d\\n",
                frontend_diag_count++, target, internalformat, width, height);
    if (real_fn)
        real_fn(target, internalformat, width, height);
}

__attribute__((visibility("default")))
void glFramebufferRenderbuffer(unsigned target, unsigned attachment,
                               unsigned renderbuffertarget,
                               unsigned renderbuffer)
{
    typedef void (*fn_t)(unsigned, unsigned, unsigned, unsigned);
    static fn_t real_fn;
    static void *handle;

    if (!handle)
        handle = gl4es_handle();
    if (!real_fn && handle)
        real_fn = (fn_t)dlsym(handle, "glFramebufferRenderbuffer");
    if (frontend_diag_enabled() && frontend_diag_count < 128)
        fprintf(stderr,
                "GUA-HARDEXT frontend fb-rb#%u target=0x%x "
                "attachment=0x%x rb-target=0x%x rb=%u\\n",
                frontend_diag_count++, target, attachment,
                renderbuffertarget, renderbuffer);
    if (real_fn)
        real_fn(target, attachment, renderbuffertarget, renderbuffer);
}

__attribute__((visibility("default")))
void glFramebufferTexture2D(unsigned target, unsigned attachment,
                            unsigned textarget, unsigned texture, int level)
{
    typedef void (*fn_t)(unsigned, unsigned, unsigned, unsigned, int);
    static fn_t real_fn;
    static void *handle;

    if (!handle)
        handle = gl4es_handle();
    if (!real_fn && handle)
        real_fn = (fn_t)dlsym(handle, "glFramebufferTexture2D");
    if (frontend_diag_enabled() && frontend_diag_count < 128)
        fprintf(stderr,
                "GUA-HARDEXT frontend fb-tex#%u target=0x%x "
                "attachment=0x%x tex-target=0x%x tex=%u level=%d\\n",
                frontend_diag_count++, target, attachment, textarget,
                texture, level);
    if (real_fn)
        real_fn(target, attachment, textarget, texture, level);
}

__attribute__((visibility("default")))
void glTexImage2D(unsigned target, int level, int internalformat,
                  int width, int height, int border, unsigned format,
                  unsigned type, const void *pixels)
{
    typedef void (*fn_t)(unsigned, int, int, int, int, int, unsigned,
                         unsigned, const void *);
    static fn_t real_fn;
    static void *handle;

    if (!handle)
        handle = gl4es_handle();
    if (!real_fn && handle)
        real_fn = (fn_t)dlsym(handle, "glTexImage2D");
    if (frontend_diag_enabled() && frontend_diag_count < 128)
        fprintf(stderr,
                "GUA-HARDEXT frontend tex-image#%u target=0x%x level=%d "
                "ifmt=0x%x size=%dx%d format=0x%x type=0x%x\\n",
                frontend_diag_count++, target, level,
                (unsigned)internalformat, width, height, format, type);
    if (real_fn)
        real_fn(target, level, internalformat, width, height, border,
                format, type, pixels);
}

__attribute__((visibility("default")))
void glCopyTexImage2D(unsigned target, int level, unsigned internalformat,
                      int x, int y, int width, int height, int border)
{
    typedef void (*fn_t)(unsigned, int, unsigned, int, int, int, int, int);
    static fn_t real_fn;
    static void *handle;

    if (!handle)
        handle = gl4es_handle();
    if (!real_fn && handle)
        real_fn = (fn_t)dlsym(handle, "glCopyTexImage2D");
    if (frontend_diag_enabled() && frontend_diag_count < 128)
        fprintf(stderr,
                "GUA-HARDEXT frontend tex-copy#%u target=0x%x level=%d "
                "ifmt=0x%x xy=%d,%d size=%dx%d border=%d\\n",
                frontend_diag_count++, target, level, internalformat,
                x, y, width, height, border);
    if (real_fn)
        real_fn(target, level, internalformat, x, y, width, height, border);
}

__attribute__((visibility("default")))
void GetHardwareExtensions(int notest)
{
    static get_hardware_extensions_fn real_get_hardware_extensions;
    static void *handle;
    static int logged;
    int *hardext;
    int before;

    if (!handle)
        handle = gl4es_handle();
    if (!handle)
        return;

    if (!real_get_hardware_extensions) {
        real_get_hardware_extensions = (get_hardware_extensions_fn)
            dlsym(handle, "GetHardwareExtensions");
        if (!real_get_hardware_extensions) {
            fprintf(stderr, "GUA-HARDEXT FAIL real GetHardwareExtensions: %s\\n",
                    dlerror());
            return;
        }
    }

    real_get_hardware_extensions(notest);

    hardext = (int *)dlsym(handle, "hardext");
    if (!hardext) {
        fprintf(stderr, "GUA-HARDEXT FAIL hardext: %s\n", dlerror());
        return;
    }

    before = hardext[HARDEXT_MAXCOLORATTACH_INDEX];
    if (notest &&
        hardext[HARDEXT_ESVERSION_INDEX] == 2 &&
        hardext[HARDEXT_FBO_INDEX] != 0 &&
        hardext[HARDEXT_MAXCOLORATTACH_INDEX] < 1) {
        hardext[HARDEXT_MAXCOLORATTACH_INDEX] = 1;
    }

    if (!logged) {
        fprintf(stderr,
                "GUA-HARDEXT notest=%d es=%d fbo=%d "
                "maxcolorattach=%d->%d maxdrawbuffers=%d\n",
                notest,
                hardext[HARDEXT_ESVERSION_INDEX],
                hardext[HARDEXT_FBO_INDEX],
                before,
                hardext[HARDEXT_MAXCOLORATTACH_INDEX],
                hardext[HARDEXT_MAXDRAWBUFFERS_INDEX]);
        logged = 1;
    }
}
