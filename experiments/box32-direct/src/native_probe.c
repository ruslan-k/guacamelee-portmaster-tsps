#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <stdio.h>

int main(void) {
    int rc = SDL_Init(SDL_INIT_VIDEO);
    printf("NATIVE SDL_Init rc=%d err=%s driver=%s\n", rc, SDL_GetError(), SDL_GetCurrentVideoDriver());
    SDL_DisplayMode dm = {0};
    rc = SDL_GetCurrentDisplayMode(0, &dm);
    printf("NATIVE display rc=%d mode=%dx%d@%d err=%s\n", rc, dm.w, dm.h, dm.refresh_rate, SDL_GetError());
    SDL_Window *w = SDL_CreateWindow("tsps-native-probe", 0, 0, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    printf("NATIVE CreateWindow=%p err=%s\n", (void*)w, SDL_GetError());
    if (!w) return 2;
    SDL_GLContext c = SDL_GL_CreateContext(w);
    printf("NATIVE CreateContext=%p err=%s\n", c, SDL_GetError());
    if (!c) return 3;
    int ww=0,hh=0,dw=0,dh=0; SDL_GetWindowSize(w,&ww,&hh); SDL_GL_GetDrawableSize(w,&dw,&dh);
    EGLDisplay edpy = eglGetCurrentDisplay();
    EGLContext ectx = eglGetCurrentContext();
    EGLSurface edraw = eglGetCurrentSurface(EGL_DRAW);
    EGLSurface eread = eglGetCurrentSurface(EGL_READ);
    const char *ev = edpy != EGL_NO_DISPLAY ? eglQueryString(edpy, EGL_VENDOR) : "(none)";
    const char *ever = edpy != EGL_NO_DISPLAY ? eglQueryString(edpy, EGL_VERSION) : "(none)";
    printf("NATIVE window=%dx%d drawable=%dx%d\n", ww,hh,dw,dh);
    printf("NATIVE egl display=%p context=%p draw=%p read=%p error=0x%x vendor=%s version=%s\n",
      edpy, ectx, edraw, eread, eglGetError(), ev, ever);
    printf("NATIVE GL vendor=%s renderer=%s version=%s\n",
      (const char*)glGetString(GL_VENDOR),(const char*)glGetString(GL_RENDERER),(const char*)glGetString(GL_VERSION));
    for (int i=1;i<=120;i++) { glClearColor(1,0,1,1); glClear(GL_COLOR_BUFFER_BIT); SDL_GL_SwapWindow(w); if (!(i%30)) printf("NATIVE swap=%d err=%s\n",i,SDL_GetError()); SDL_Delay(16); }
    SDL_GL_DeleteContext(c); SDL_DestroyWindow(w); SDL_Quit(); return 0;
}
