#include <SDL2/SDL.h>
#include <GL/gl.h>
#include <stdio.h>
int main(void) {
    printf("I386 before SDL_Init driver=%s\n", SDL_GetCurrentVideoDriver());
    if (SDL_Init(SDL_INIT_VIDEO)) { printf("I386 SDL_Init err=%s\n",SDL_GetError()); return 2; }
    printf("I386 driver=%s displays=%d err=%s\n",SDL_GetCurrentVideoDriver(),SDL_GetNumVideoDisplays(),SDL_GetError());
    SDL_Window *w=SDL_CreateWindow("tsps-i386-probe",0,0,1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    printf("I386 CreateWindow=%p err=%s\n",(void*)w,SDL_GetError()); if(!w)return 3;
    SDL_GLContext c=SDL_GL_CreateContext(w); printf("I386 CreateContext=%p err=%s\n",c,SDL_GetError()); if(!c)return 4;
    printf("I386 GL=%s/%s/%s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
    for(int i=1;i<=120;i++){glClearColor(1,0,1,1);glClear(GL_COLOR_BUFFER_BIT);SDL_GL_SwapWindow(w);if(!(i%30))printf("I386 swap=%d err=%s\n",i,SDL_GetError());SDL_Delay(16);}
    SDL_GL_DeleteContext(c);SDL_DestroyWindow(w);SDL_Quit();return 0;
}
