#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>

typedef struct { uint32_t format; int w, h, refresh; void *driverdata; } gua_mode_t;
typedef struct { int x, y, w, h; } gua_rect_t;
static void fix_mode(gua_mode_t *m) { if (m && (m->w <= 0 || m->h <= 0)) { m->format=0; m->w=1024; m->h=768; if (m->refresh <= 0) m->refresh=60; m->driverdata=0; } }
int SDL_GetNumVideoDisplays(void) { static int (*real_fn)(void); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetNumVideoDisplays"); { int rc=real_fn ? real_fn() : 0; return rc > 0 ? rc : 1; } }
int SDL_GetDesktopDisplayMode(int d, void *m) { static int (*real_fn)(int,void*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDesktopDisplayMode"); { int rc=real_fn ? real_fn(d,m) : -1; if (rc != 0 || !m || ((gua_mode_t *)m)->w <= 0 || ((gua_mode_t *)m)->h <= 0) fix_mode((gua_mode_t*)m); return rc == 0 ? 0 : 0; } }
int SDL_GetCurrentDisplayMode(int d, void *m) { static int (*real_fn)(int,void*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetCurrentDisplayMode"); { int rc=real_fn ? real_fn(d,m) : -1; if (rc != 0 || !m || ((gua_mode_t *)m)->w <= 0 || ((gua_mode_t *)m)->h <= 0) fix_mode((gua_mode_t*)m); return rc == 0 ? 0 : 0; } }
int SDL_GetDisplayBounds(int d, gua_rect_t *r) { static int (*real_fn)(int,gua_rect_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDisplayBounds"); { int rc=real_fn ? real_fn(d,r) : -1; if (rc != 0 || !r || r->w <= 0 || r->h <= 0) { if (r) { r->x=0; r->y=0; r->w=1024; r->h=768; } return 0; } return rc; } }
int SDL_GetDisplayUsableBounds(int d, gua_rect_t *r) { static int (*real_fn)(int,gua_rect_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDisplayUsableBounds"); { int rc=real_fn ? real_fn(d,r) : -1; if (rc != 0 || !r || r->w <= 0 || r->h <= 0) { if (r) { r->x=0; r->y=0; r->w=1024; r->h=768; } return 0; } return rc; } }
void *SDL_CreateWindow(const char *t,int x,int y,int w,int h,uint32_t flags) { static void *(*real_fn)(const char*,int,int,int,int,uint32_t); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_CreateWindow"); if (w<=0) w=1024; if (h<=0) h=768; return real_fn ? real_fn(t,x,y,w,h,flags) : 0; }
