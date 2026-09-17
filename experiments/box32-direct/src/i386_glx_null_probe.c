#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("GLXNULL before SDL_Init driver=%s\n", SDL_GetCurrentVideoDriver());
    if (SDL_Init(SDL_INIT_VIDEO)) { printf("GLXNULL SDL_Init err=%s\n",SDL_GetError()); return 2; }
    printf("GLXNULL SDL_Init driver=%s displays=%d err=%s\n",SDL_GetCurrentVideoDriver(),SDL_GetNumVideoDisplays(),SDL_GetError());
    SDL_Window *w=SDL_CreateWindow("tsps-i386-glx-null",0,0,1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    printf("GLXNULL CreateWindow=%p err=%s\n",(void*)w,SDL_GetError()); if(!w)return 3;
    SDL_GLContext c=SDL_GL_CreateContext(w); printf("GLXNULL CreateContext=%p err=%s\n",c,SDL_GetError()); if(!c)return 4;
    Display *dpy=glXGetCurrentDisplay(); printf("GLXNULL glXGetCurrentDisplay=%p err=%s\n",(void*)dpy,SDL_GetError());
    glClearColor(1,0,1,1);
    for(int i=1;i<=120;i++){glClear(GL_COLOR_BUFFER_BIT);SDL_GL_SwapWindow(w);if(!(i%30))printf("GLXNULL swap=%d err=%s\n",i,SDL_GetError());SDL_Delay(16);}
    printf("GLXNULL hold-after-120\n");
    SDL_Delay(5000);
    SDL_GL_DeleteContext(c);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
