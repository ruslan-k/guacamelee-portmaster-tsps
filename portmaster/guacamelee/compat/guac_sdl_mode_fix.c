#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>

typedef struct { uint32_t format; int w, h, refresh; void *driverdata; } gua_mode_t;
typedef struct { int x, y, w, h; } gua_rect_t;
static void fix_mode(gua_mode_t *m) { if (m) { m->format=0; m->w=1024; m->h=768; m->refresh=60; m->driverdata=0; } }
int SDL_GetDesktopDisplayMode(int d, void *m) { static int (*real_fn)(int,void*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDesktopDisplayMode"); if (real_fn) real_fn(d,m); fix_mode((gua_mode_t*)m); return 0; }
int SDL_GetCurrentDisplayMode(int d, void *m) { static int (*real_fn)(int,void*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetCurrentDisplayMode"); if (real_fn) real_fn(d,m); fix_mode((gua_mode_t*)m); return 0; }
int SDL_GetDisplayBounds(int d, gua_rect_t *r) { static int (*real_fn)(int,gua_rect_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDisplayBounds"); if (real_fn) real_fn(d,r); if (r) { r->x=0; r->y=0; r->w=1024; r->h=768; } return 0; }
int SDL_GetDisplayUsableBounds(int d, gua_rect_t *r) { static int (*real_fn)(int,gua_rect_t*); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_GetDisplayUsableBounds"); if (real_fn) real_fn(d,r); if (r) { r->x=0; r->y=0; r->w=1024; r->h=768; } return 0; }
void *SDL_CreateWindow(const char *t,int x,int y,int w,int h,uint32_t flags) { static void *(*real_fn)(const char*,int,int,int,int,uint32_t); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_CreateWindow"); if (w<=0) w=1024; if (h<=0) h=768; return real_fn ? real_fn(t,x,y,w,h,flags) : 0; }
