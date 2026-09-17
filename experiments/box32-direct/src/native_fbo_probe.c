#include <SDL2/SDL.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <stdio.h>
#include <string.h>

static void err(const char *where) { printf("NATIVE_FBO %s err=0x%04x\n", where, glGetError()); }
static void attachment(const char *tag, GLenum a) {
    GLint type=0, name=0, w=0, h=0, fmt=0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, a, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type); err("query_type");
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, a, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name); err("query_name");
    if (type == GL_RENDERBUFFER) {
        glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)name); err("query_bind_rb");
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &w); err("query_rb_width");
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &h); err("query_rb_height");
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &fmt); err("query_rb_format");
    }
    printf("NATIVE_FBO %s type=0x%x name=%d w=%d h=%d fmt=0x%x\n", tag, type, name, w, h, fmt);
}
static void status(const char *tag) { printf("NATIVE_FBO %s status=0x%04x\n", tag, glCheckFramebufferStatus(GL_FRAMEBUFFER)); err("check_status"); }
static void ext(const char *e) { const char *x=(const char*)glGetString(GL_EXTENSIONS); printf("NATIVE_FBO extension %s=%s\n",e,(x&&strstr(x,e))?"yes":"no"); }

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { printf("NATIVE_FBO SDL_Init_FAIL err=%s\n",SDL_GetError()); return 2; }
    printf("NATIVE_FBO driver=%s err=%s\n",SDL_GetCurrentVideoDriver(),SDL_GetError());
    SDL_DisplayMode dm={0}; SDL_GetCurrentDisplayMode(0,&dm);
    printf("NATIVE_FBO display=%dx%d@%d err=%s\n",dm.w,dm.h,dm.refresh_rate,SDL_GetError());
    SDL_Window *w=SDL_CreateWindow("native-fbo-probe",0,0,1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);
    printf("NATIVE_FBO window=%p err=%s\n",(void*)w,SDL_GetError()); if(!w)return 3;
    SDL_GLContext c=SDL_GL_CreateContext(w); printf("NATIVE_FBO context=%p err=%s\n",c,SDL_GetError()); if(!c)return 4;
    int ww,hh,dw,dh; SDL_GetWindowSize(w,&ww,&hh); SDL_GL_GetDrawableSize(w,&dw,&dh);
    printf("NATIVE_FBO sizes window=%dx%d drawable=%dx%d\n",ww,hh,dw,dh);
    printf("NATIVE_FBO GL vendor=%s renderer=%s version=%s\n",glGetString(GL_VENDOR),glGetString(GL_RENDERER),glGetString(GL_VERSION));
    ext("GL_OES_rgb8_rgba8"); ext("GL_OES_packed_depth_stencil"); ext("GL_EXT_packed_depth_stencil"); ext("GL_OES_depth24"); ext("GL_OES_depth_texture");
    GLuint fbo=0,color=0,ds=0,texfbo=0,tex=0;
    glGenFramebuffers(1,&fbo); err("A_gen_fbo"); glBindFramebuffer(GL_FRAMEBUFFER,fbo); err("A_bind_fbo");
    glGenRenderbuffers(1,&color); err("A_gen_color"); glBindRenderbuffer(GL_RENDERBUFFER,color); err("A_bind_color");
    glRenderbufferStorage(GL_RENDERBUFFER,GL_RGBA8_OES,1280,720); err("A_color_storage");
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_RENDERBUFFER,color); err("A_color_attach");
    attachment("A_color",GL_COLOR_ATTACHMENT0); status("A");
    glGenRenderbuffers(1,&ds); err("B_gen_ds"); glBindRenderbuffer(GL_RENDERBUFFER,ds); err("B_bind_ds");
    glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH24_STENCIL8_OES,1280,720); err("B_ds_storage");
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,ds); err("B_depth_attach");
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_STENCIL_ATTACHMENT,GL_RENDERBUFFER,ds); err("B_stencil_attach");
    attachment("B_color",GL_COLOR_ATTACHMENT0); attachment("B_depth",GL_DEPTH_ATTACHMENT); attachment("B_stencil",GL_STENCIL_ATTACHMENT); status("B");
    glGenFramebuffers(1,&texfbo); err("C_gen_fbo"); glBindFramebuffer(GL_FRAMEBUFFER,texfbo); err("C_bind_fbo");
    glGenTextures(1,&tex); err("C_gen_tex"); glBindTexture(GL_TEXTURE_2D,tex); err("C_bind_tex");
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1280,720,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL); err("C_tex_image");
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,tex,0); err("C_tex_attach");
    attachment("C_color",GL_COLOR_ATTACHMENT0); status("C");
    glBindFramebuffer(GL_FRAMEBUFFER,0); err("unbind");
    printf("NATIVE_FBO hold\n"); SDL_Delay(3000); SDL_GL_DeleteContext(c); SDL_DestroyWindow(w); SDL_Quit(); return 0;
}
