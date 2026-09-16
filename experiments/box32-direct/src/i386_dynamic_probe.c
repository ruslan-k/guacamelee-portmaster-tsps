#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

typedef struct SDL_Window SDL_Window;
typedef void *SDL_GLContext;
typedef int (*fn_init)(uint32_t);
typedef const char *(*fn_err)(void);
typedef SDL_Window *(*fn_window)(const char *, int, int, int, int, uint32_t);
typedef SDL_GLContext (*fn_ctx)(SDL_Window *);
typedef void (*fn_swap)(SDL_Window *);
typedef void (*fn_delay)(uint32_t);
typedef void (*fn_destroy)(SDL_Window *);
typedef void (*fn_delctx)(SDL_GLContext);
typedef void (*fn_quit)(void);
typedef const unsigned char *(*fn_glstr)(unsigned);
typedef void (*fn_clearcolor)(float,float,float,float);
typedef void (*fn_clear)(unsigned);
typedef void *(*fn_eglzero)(void);
typedef void *(*fn_eglcurrent)(unsigned);

#define SDL_INIT_VIDEO 0x00000020u
#define SDL_WINDOW_FULLSCREEN 0x00000001u
#define SDL_WINDOW_OPENGL 0x00000002u
#define GL_COLOR_BUFFER_BIT 0x00004000u
#define GL_VENDOR 0x1f00u
#define GL_RENDERER 0x1f01u
#define GL_VERSION 0x1f02u
#define EGL_DRAW 0x3059u
#define EGL_READ 0x3058u

static void egl_state(void *egl) {
    fn_eglzero getd=(fn_eglzero)dlsym(egl,"eglGetCurrentDisplay");
    fn_eglzero getc=(fn_eglzero)dlsym(egl,"eglGetCurrentContext");
    fn_eglcurrent getdraw=(fn_eglcurrent)dlsym(egl,"eglGetCurrentSurface");
    printf("PROBE EGL display=%p context=%p draw=%p read=%p\n",getd(),getc(),getdraw(EGL_DRAW),getdraw(EGL_READ));
}

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    void *s=dlopen("libSDL2-2.0.so.0", RTLD_NOW|RTLD_GLOBAL);
    void *g=dlopen("libGL.so.1", RTLD_NOW|RTLD_GLOBAL);
    void *e=dlopen("libEGL.so.1", RTLD_NOW|RTLD_GLOBAL);
    printf("PROBE handles s=%p gl=%p egl=%p\n",s,g,e);
    if(!s || !g || !e) return 10;
    fn_init init=(fn_init)dlsym(s,"SDL_Init"); fn_err err=(fn_err)dlsym(s,"SDL_GetError");
    fn_window window=(fn_window)dlsym(s,"SDL_CreateWindow"); fn_ctx create=(fn_ctx)dlsym(s,"SDL_GL_CreateContext");
    fn_swap swap=(fn_swap)dlsym(s,"SDL_GL_SwapWindow"); fn_delay delay=(fn_delay)dlsym(s,"SDL_Delay");
    fn_destroy destroy=(fn_destroy)dlsym(s,"SDL_DestroyWindow"); fn_delctx delctx=(fn_delctx)dlsym(s,"SDL_GL_DeleteContext"); fn_quit quit=(fn_quit)dlsym(s,"SDL_Quit");
    fn_glstr glstr=(fn_glstr)dlsym(g,"glGetString"); fn_clearcolor cc=(fn_clearcolor)dlsym(g,"glClearColor"); fn_clear clr=(fn_clear)dlsym(g,"glClear");
    printf("PROBE symbols init=%p window=%p create=%p swap=%p glGetString=%p\n",init,window,create,swap,glstr);
    if(init(SDL_INIT_VIDEO)) { printf("PROBE SDL_Init failed %s\n",err()); return 2; }
    SDL_Window *w=window("box32-detection-probe",0,0,1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    printf("PROBE window=%p err=%s\n",w,err()); if(!w)return 3;
    SDL_GLContext c=create(w); printf("PROBE context=%p err=%s\n",c,err()); if(!c)return 4;
    egl_state(e);
    printf("PROBE GL vendor=%s renderer=%s version=%s\n",glstr(GL_VENDOR),glstr(GL_RENDERER),glstr(GL_VERSION));
    egl_state(e);
    for(int i=1;i<=30;i++){cc(1,0,1,1);clr(GL_COLOR_BUFFER_BIT);swap(w);if(!(i%10))printf("PROBE swap=%d err=%s\n",i,err());delay(16);}
    printf("PROBE hold\n"); delay(1500);
    delctx(c); destroy(w); quit(); return 0;
}
