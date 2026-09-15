#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>

typedef struct { uint32_t format; int w, h, refresh; void *driverdata; } gua_mode_t;
typedef struct { int x, y, w, h; } gua_rect_t;
static void fix_mode(gua_mode_t *m) { if (m) { m->w=1280; m->h=720; if (m->refresh <= 0) m->refresh=60; } }
int SDL_GetNumVideoDisplays(void) { static int (*real_fn)(void); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetNumVideoDisplays"); { int rc=real_fn ? real_fn() : 0; return rc > 0 ? rc : 1; } }
int SDL_GetNumDisplayModes(int d) { (void)d; return 1; }
int SDL_GetDisplayMode(int d, int index, gua_mode_t *m) { (void)d; if (index != 0 || !m) return -1; m->format=0; m->w=1280; m->h=720; m->refresh=60; m->driverdata=0; return 0; }
const gua_mode_t *SDL_GetClosestDisplayMode(int d, const gua_mode_t *requested, gua_mode_t *closest) { (void)d; if (!closest) return 0; if (requested) *closest=*requested; closest->w=1280; closest->h=720; if (closest->refresh <= 0) closest->refresh=60; return closest; }
int SDL_GetDesktopDisplayMode(int d, void *m) { static int (*real_fn)(int,void*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDesktopDisplayMode"); { int rc=real_fn ? real_fn(d,m) : -1; fix_mode((gua_mode_t*)m); return rc == 0 ? 0 : 0; } }
int SDL_GetCurrentDisplayMode(int d, void *m) { static int (*real_fn)(int,void*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetCurrentDisplayMode"); { int rc=real_fn ? real_fn(d,m) : -1; fix_mode((gua_mode_t*)m); return rc == 0 ? 0 : 0; } }
int SDL_GetDisplayBounds(int d, gua_rect_t *r) { static int (*real_fn)(int,gua_rect_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDisplayBounds"); { int rc=real_fn ? real_fn(d,r) : -1; if (r) { r->x=0; r->y=0; r->w=1280; r->h=720; } return rc == 0 ? 0 : 0; } }
int SDL_GetDisplayUsableBounds(int d, gua_rect_t *r) { static int (*real_fn)(int,gua_rect_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDisplayUsableBounds"); { int rc=real_fn ? real_fn(d,r) : -1; if (r) { r->x=0; r->y=0; r->w=1280; r->h=720; } return rc == 0 ? 0 : 0; } }
void SDL_GetWindowSize(void *window, int *w, int *h) { static void (*real_fn)(void*,int*,int*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetWindowSize"); if (real_fn) real_fn(window,w,h); if (w) *w=1280; if (h) *h=720; }
void SDL_GL_GetDrawableSize(void *window, int *w, int *h) { static void (*real_fn)(void*,int*,int*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GL_GetDrawableSize"); if (real_fn) real_fn(window,w,h); if (w) *w=1280; if (h) *h=720; }
int SDL_SetWindowSize(void *window, int w, int h) { static void (*real_fn)(void*,int,int); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_SetWindowSize"); if (real_fn) real_fn(window,1280,720); return 0; }
int SDL_SetWindowDisplayMode(void *window, const gua_mode_t *requested) { static int (*real_fn)(void*,const gua_mode_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_SetWindowDisplayMode"); gua_mode_t m=requested ? *requested : (gua_mode_t){0}; m.w=1280; m.h=720; if (real_fn) return real_fn(window,&m); return 0; }
int SDL_GetWindowDisplayMode(void *window, gua_mode_t *m) { static int (*real_fn)(void*,gua_mode_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetWindowDisplayMode"); int rc=real_fn ? real_fn(window,m) : -1; fix_mode(m); return rc == 0 ? 0 : 0; }
void *SDL_CreateWindow(const char *t,int x,int y,int w,int h,uint32_t flags) { static void *(*real_fn)(const char*,int,int,int,int,uint32_t); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_CreateWindow"); if (w<=0) w=1280; if (h<=0) h=720; return real_fn ? real_fn(t,x,y,w,h,flags) : 0; }
