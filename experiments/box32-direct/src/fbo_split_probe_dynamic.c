#include <dlfcn.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
typedef unsigned int GLenum, GLuint, GLbitfield; typedef int GLint, GLsizei; typedef unsigned char GLboolean, GLubyte; typedef float GLfloat; typedef void GLvoid; typedef void *SDL_Window, *SDL_GLContext;
#define GL_NO_ERROR 0
#define GL_FRAMEBUFFER 0x8D40
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_TEXTURE_2D 0x0DE1
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_NEAREST 0x2600
#define GL_COLOR_BUFFER_BIT 0x4000
#define SDL_INIT_VIDEO 0x20
#define SDL_WINDOW_OPENGL 0x00000002
#define SDL_WINDOW_FULLSCREEN 0x00000001
#define SDL_WINDOW_HIDDEN 0x00000008
#define SDL_GL_CONTEXT_PROFILE_MASK 0x9126

typedef int (*PFN_SDL_Init)(unsigned); typedef const char *(*PFN_SDL_GetError)(void); typedef SDL_Window *(*PFN_SDL_CreateWindow)(const char*,int,int,int,int,unsigned); typedef SDL_GLContext (*PFN_SDL_GL_CreateContext)(SDL_Window*); typedef void (*PFN_SDL_GL_DeleteContext)(SDL_GLContext); typedef void (*PFN_SDL_DestroyWindow)(SDL_Window*); typedef void (*PFN_SDL_Quit)(void);
typedef void (*PFN_ClearColor)(GLfloat,GLfloat,GLfloat,GLfloat); typedef void (*PFN_Clear)(GLbitfield); typedef GLenum (*PFN_GetError)(void); typedef void (*PFN_GenTextures)(GLsizei,GLuint*); typedef void (*PFN_BindTexture)(GLenum,GLuint); typedef void (*PFN_TexParameteri)(GLenum,GLenum,GLint); typedef void (*PFN_TexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*); typedef void (*PFN_GenFramebuffers)(GLsizei,GLuint*); typedef void (*PFN_BindFramebuffer)(GLenum,GLuint); typedef void (*PFN_FramebufferTexture2D)(GLenum,GLenum,GLenum,GLuint,GLint); typedef void (*PFN_BlitFramebuffer)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum); typedef void (*PFN_ReadPixels)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*); typedef void (*PFN_DeleteFramebuffers)(GLsizei,const GLuint*); typedef void (*PFN_DeleteTextures)(GLsizei,const GLuint*);
#define LOAD(h,n) do { n=(void*)dlsym(h,#n); if(!n){fprintf(stderr,"MISSING %s\n",#n);return 3;} } while(0)
int main(void){
 void *s=dlopen("libSDL2-2.0.so.0",RTLD_NOW|RTLD_GLOBAL), *g=dlopen("libGL.so.1",RTLD_NOW|RTLD_GLOBAL); if(!s||!g){fprintf(stderr,"DLOPEN s=%s g=%s\n",dlerror(),dlerror());return 2;}
 PFN_SDL_Init SDL_Init; PFN_SDL_GetError SDL_GetError; PFN_SDL_CreateWindow SDL_CreateWindow; PFN_SDL_GL_CreateContext SDL_GL_CreateContext; PFN_SDL_GL_DeleteContext SDL_GL_DeleteContext; PFN_SDL_DestroyWindow SDL_DestroyWindow; PFN_SDL_Quit SDL_Quit; LOAD(s,SDL_Init);LOAD(s,SDL_GetError);LOAD(s,SDL_CreateWindow);LOAD(s,SDL_GL_CreateContext);LOAD(s,SDL_GL_DeleteContext);LOAD(s,SDL_DestroyWindow);LOAD(s,SDL_Quit);
 PFN_ClearColor glClearColor;PFN_Clear glClear;PFN_GetError glGetError;PFN_GenTextures glGenTextures;PFN_BindTexture glBindTexture;PFN_TexParameteri glTexParameteri;PFN_TexImage2D glTexImage2D;PFN_GenFramebuffers glGenFramebuffers;PFN_BindFramebuffer glBindFramebuffer;PFN_FramebufferTexture2D glFramebufferTexture2D;PFN_BlitFramebuffer glBlitFramebuffer;PFN_ReadPixels glReadPixels;PFN_DeleteFramebuffers glDeleteFramebuffers;PFN_DeleteTextures glDeleteTextures;
 LOAD(g,glClearColor);LOAD(g,glClear);LOAD(g,glGetError);LOAD(g,glGenTextures);LOAD(g,glBindTexture);LOAD(g,glTexParameteri);LOAD(g,glTexImage2D);LOAD(g,glGenFramebuffers);LOAD(g,glBindFramebuffer);LOAD(g,glFramebufferTexture2D);LOAD(g,glBlitFramebuffer);LOAD(g,glReadPixels);LOAD(g,glDeleteFramebuffers);LOAD(g,glDeleteTextures);
 if(SDL_Init(SDL_INIT_VIDEO)){fprintf(stderr,"SDL_Init %s\n",SDL_GetError());return 4;} SDL_Window *w=SDL_CreateWindow("fbo-split",0,0,1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN); if(!w){fprintf(stderr,"Window %s\n",SDL_GetError());return 4;} SDL_GLContext c=SDL_GL_CreateContext(w);if(!c){fprintf(stderr,"Context %s\n",SDL_GetError());return 4;}
 GLuint ta,tb,fa,fb;glGenTextures(1,&ta);glGenTextures(1,&tb); for(GLuint t=ta;t<=tb;t++){glBindTexture(GL_TEXTURE_2D,t);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,0);} glGenFramebuffers(1,&fa);glGenFramebuffers(1,&fb);glBindFramebuffer(GL_FRAMEBUFFER,fa);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,ta,0);glBindFramebuffer(GL_FRAMEBUFFER,fb);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tb,0);
 unsigned char p[4];glBindFramebuffer(GL_FRAMEBUFFER,fa);glClearColor(.25f,.5f,.75f,1);glClear(GL_COLOR_BUFFER_BIT);glBindFramebuffer(GL_READ_FRAMEBUFFER,fa);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fb);glBlitFramebuffer(0,0,64,64,0,0,64,64,GL_COLOR_BUFFER_BIT,GL_NEAREST);glBindFramebuffer(GL_FRAMEBUFFER,fb);glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);int pass1=p[0]==64&&p[1]==128&&p[2]==191&&p[3]==255;printf("BOX32_SPLIT_FBO_BLIT_CONTROL=%s rgba=%u,%u,%u,%u\n",pass1?"PASS":"FAIL",p[0],p[1],p[2],p[3]);
 glBindFramebuffer(GL_FRAMEBUFFER,0);glClearColor(.9f,.1f,.2f,1);glClear(GL_COLOR_BUFFER_BIT);glBindFramebuffer(GL_READ_FRAMEBUFFER,fa);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,fb);glBlitFramebuffer(0,0,64,64,0,0,64,64,GL_COLOR_BUFFER_BIT,GL_NEAREST);glBindFramebuffer(GL_FRAMEBUFFER,fb);glReadPixels(32,32,1,1,GL_RGBA,GL_UNSIGNED_BYTE,p);int pass2=p[0]==64&&p[1]==128&&p[2]==191&&p[3]==255;printf("DEFAULT_FBO_VS_READ_FBO_CONTROL=%s rgba=%u,%u,%u,%u\n",pass2?"PASS":"FAIL",p[0],p[1],p[2],p[3]);printf("FBO_PROBE_GL_ERROR=0x%x\n",glGetError());glDeleteFramebuffers(1,&fa);glDeleteFramebuffers(1,&fb);glDeleteTextures(1,&ta);glDeleteTextures(1,&tb);SDL_GL_DeleteContext(c);SDL_DestroyWindow(w);SDL_Quit();return pass1&&pass2?0:1;
}
