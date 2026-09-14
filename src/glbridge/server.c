#define _GNU_SOURCE
#include "../nfsmw_frame.h"
#include "gl_api.h"
#include "ops.h"
#include "protocol.h"
#include "xport.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

struct tspgl_api G;

static int gl_diag = -1;
static int gl_diag_lifecycle = -1;
static unsigned gl_diag_ops;
static int gl_diag_link;
static int gl_diag_use;
static int gl_diag_draw;
static int gl_diag_swap;

static int gl_diag_enabled(void)
{
    const char *v;

    if (gl_diag >= 0)
        return gl_diag;
    v = getenv("GUACAMELEE_GL_DIAG");
    gl_diag = v && strcmp(v, "0") != 0;
    return gl_diag;
}

static int gl_diag_lifecycle_enabled(void)
{
    const char *v;

    if (gl_diag_lifecycle >= 0)
        return gl_diag_lifecycle && gl_diag_enabled();
    v = getenv("GUACAMELEE_FBO_LIFECYCLE");
    gl_diag_lifecycle = v && strcmp(v, "0") != 0;
    return gl_diag_lifecycle && gl_diag_enabled();
}

static const char *gl_diag_op_name(uint32_t op)
{
    switch (op) {
    case OP_glBindFramebuffer: return "glBindFramebuffer";
    case OP_glBindRenderbuffer: return "glBindRenderbuffer";
    case OP_glCheckFramebufferStatus: return "glCheckFramebufferStatus";
    case OP_glFramebufferRenderbuffer: return "glFramebufferRenderbuffer";
    case OP_glFramebufferTexture2D: return "glFramebufferTexture2D";
    case OP_glRenderbufferStorage: return "glRenderbufferStorage";
    case OP_glTexImage2D: return "glTexImage2D";
    case OP_glDeleteRenderbuffers: return "glDeleteRenderbuffers";
    case OP_glDeleteTextures: return "glDeleteTextures";
    case OP_glGetIntegerv: return "glGetIntegerv";
    case OP_glGetString: return "glGetString";
    case OP_glCreateShader: return "glCreateShader";
    case OP_glShaderSource: return "glShaderSource";
    case OP_glCompileShader: return "glCompileShader";
    case OP_glLinkProgram: return "glLinkProgram";
    case OP_glUseProgram: return "glUseProgram";
    case OP_glDrawArrays: return "glDrawArrays";
    case OP_glDrawElements: return "glDrawElements";
    case OP_glGetShaderiv: return "glGetShaderiv";
    case OP_glGetProgramiv: return "glGetProgramiv";
    case OP_glClear: return "glClear";
    case OP_glGenerateMipmap: return "glGenerateMipmap";
    case OP_glGetError: return "glGetError";
    case OP_glGetFramebufferAttachmentParameteriv:
        return "glGetFramebufferAttachmentParameteriv";
    case OP_glGetRenderbufferParameteriv: return "glGetRenderbufferParameteriv";
    case OP_glGetUniformLocation: return "glGetUniformLocation";
    case OP_glUniform1f: return "glUniform1f";
    case OP_glUniform1i: return "glUniform1i";
    case OP_glUniformMatrix4fv: return "glUniformMatrix4fv";
    case OP_glReadPixels: return "glReadPixels";
    default: return "other";
    }
}

static void gl_diag_op(uint32_t op, uint32_t len)
{
    if (!gl_diag_enabled() || gl_diag_ops >= 128)
        return;
    fprintf(stderr, "GUA-GL op#%u %s(%u) len=%u\n", gl_diag_ops++,
            gl_diag_op_name(op), op, len);
}

static unsigned gl_diag_fbo_ops;

static int gl_diag_is_fbo_op(uint32_t op)
{
    switch (op) {
    case OP_glBindFramebuffer:
    case OP_glBindRenderbuffer:
    case OP_glCheckFramebufferStatus:
    case OP_glFramebufferRenderbuffer:
    case OP_glFramebufferTexture2D:
    case OP_glCopyTexImage2D:
    case OP_glRenderbufferStorage:
    case OP_glGenFramebuffers:
    case OP_glGenRenderbuffers:
    case OP_glGetFramebufferAttachmentParameteriv:
    case OP_glGetRenderbufferParameteriv:
    case OP_glTexImage2D:
    case OP_glDeleteRenderbuffers:
    case OP_glDeleteTextures:
        return 1;
    default:
        return 0;
    }
}

static void gl_diag_fbo_call(uint32_t op, const uint8_t *in, uint32_t len)
{
    uint32_t a[5] = {0, 0, 0, 0, 0};
    unsigned n, i;

    if (!gl_diag_enabled() || !gl_diag_is_fbo_op(op) ||
        gl_diag_fbo_ops >= 256)
        return;
    n = len / 4u;
    if (n > 5)
        n = 5;
    for (i = 0; i < n; ++i)
        memcpy(&a[i], in + i * 4u, 4);
    fprintf(stderr,
            "GUA-FBO call#%u op=%u len=%u a0=0x%x a1=0x%x a2=%d a3=%d a4=%d\n",
            gl_diag_fbo_ops++, op, len, a[0], a[1], (int32_t)a[2],
            (int32_t)a[3], (int32_t)a[4]);
}

static void gl_diag_fbo_result(uint32_t op, const uint8_t *out,
                               uint32_t out_n, int rc)
{
    uint32_t v = 0;

    if (!gl_diag_enabled() || !gl_diag_is_fbo_op(op))
        return;
    if (out && out_n >= 4)
        memcpy(&v, out, 4);
    fprintf(stderr, "GUA-FBO result op=%u rc=%d out_n=%u value=0x%x\n",
            op, rc, out_n, v);
}

#define SDL_INIT_VIDEO 0x00000020u
#define SDL_INIT_JOYSTICK 0x00000200u
#define SDL_INIT_GAMECONTROLLER 0x00002000u
#define SDL_WINDOW_FULLSCREEN 0x00000001u
#define SDL_WINDOW_OPENGL 0x00000002u
#define SDL_WINDOW_SHOWN 0x00000004u
#define SDL_WINDOWPOS_UNDEFINED 0x1FFF0000u
#define SDL_GL_RED_SIZE 0
#define SDL_GL_GREEN_SIZE 1
#define SDL_GL_BLUE_SIZE 2
#define SDL_GL_ALPHA_SIZE 3
#define SDL_GL_DOUBLEBUFFER 5
#define SDL_GL_DEPTH_SIZE 6
#define SDL_GL_STENCIL_SIZE 7
#define SDL_GL_CONTEXT_MAJOR_VERSION 17
#define SDL_GL_CONTEXT_MINOR_VERSION 18
#define SDL_GL_CONTEXT_PROFILE_MASK 21
#define SDL_GL_CONTEXT_PROFILE_ES 4

#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_RGBA 0x1908
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DONT_CARE 0x1100
#define GL_BACK 0x0405
#define GL_READ_BUFFER 0x0C02
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_LINEAR 0x2601
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_NEAREST 0x2600
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_DEPTH_STENCIL 0x84F9
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_TEXTURE 0x1702
#define GL_NONE 0
#define GL_SCISSOR_TEST 0x0C11
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_RGB 0x1907
#define GL_ARRAY_BUFFER 0x8892
#define GL_TRIANGLE_STRIP 0x0005
#define GL_FLOAT 0x1406
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE0 0x84C0
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE 0x0B44
#define GL_BLEND 0x0BE2
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32 0x81A7
#define GL_DEPTH32F_STENCIL8 0x8CAD
#define GL_DEPTH_COMPONENT32F 0x8CAC

typedef struct sdl_event {
    uint32_t type;
    uint8_t pad[124];
} sdl_event;

struct sdl_api {
    void *lib;
    int (*init)(uint32_t);
    void (*quit)(void);
    const char *(*get_error)(void);
    const char *(*get_current_video_driver)(void);
    int (*set_hint)(const char *, const char *);
    void *(*create_window)(const char *, int, int, int, int, uint32_t);
    void (*destroy_window)(void *);
    int (*gl_set_attr)(int, int);
    void *(*gl_create_context)(void *);
    void (*gl_delete_context)(void *);
    int (*gl_make_current)(void *, void *);
    void (*gl_swap)(void *);
    void *(*gl_get_proc)(const char *);
    int (*gl_set_swap)(int);
    int (*show_cursor)(int);
    int (*poll_event)(sdl_event *);
    void (*delay)(uint32_t);
    int (*num_joysticks)(void);
    int (*is_game_controller)(int);
    void *(*controller_open)(int);
    const char *(*controller_name)(void *);
    uint8_t (*controller_get_button)(void *, int);
    int16_t (*controller_get_axis)(void *, int);
};

static struct sdl_api sdl;
static void *window;
static void *glctx;
static void *pad;
static uint8_t *frame_map;
static int frame_fd = -1;
static int listen_fd = -1;

int game_w = 1024;
int game_h = 768;
static int rb_zero_size;
static int fbo_texture_fallback;
static int unify_depth_stencil;
static int rb_format_fix;
static uint32_t depth_stencil_rb;
int win_w = 1280;
int win_h = 720;
static int present_letterbox;
uint32_t game_fbo;
uint32_t game_color;
uint32_t game_depth;
static void (*real_bind_fb)(uint32_t, uint32_t);
static void (*real_get_integerv)(uint32_t, int32_t *);
static uint32_t (*real_check_fb)(uint32_t);
static void (*real_draw_buffers)(int32_t, const uint32_t *);
static void (*real_read_buffer)(uint32_t);
static int depth_only_read_none;
static int zero_viewport;
static unsigned gl_diag_lifecycle_ops;
static uint32_t gl_diag_last_draw_fb;
static void (*real_tex_image)(uint32_t, int32_t, int32_t, int32_t, int32_t,
                              int32_t, uint32_t, uint32_t, const void *);
static void (*real_tex_parami)(uint32_t, uint32_t, int32_t);
static void (*real_compressed)(uint32_t, int32_t, uint32_t, int32_t, int32_t,
                               int32_t, int32_t, const void *);
static void (*real_gen_mipmap)(uint32_t);
static void (*real_delete_tex)(int32_t, const uint32_t *);
static uint8_t tex_npot[4096];
static int input_soon;
static int have_fb_fetch;
static int have_fb_fetch_nc;
static void (*real_tex_paramf)(uint32_t, uint32_t, float);
static int splash_live;
static uint32_t splash_tex;
static uint32_t splash_prog;
static int32_t splash_loc_pos;
static int32_t splash_loc_uv;

typedef void (*tspgl_debug_callback)(uint32_t, uint32_t, uint32_t,
                                      uint32_t, int32_t, const char *,
                                      const void *);
static int khr_debug_enabled;
static unsigned khr_seq;
static uint32_t khr_op;
static uint32_t khr_args[6];
static uint32_t khr_arg_count;
static uint32_t khr_fb;
static uint32_t khr_prog;

typedef struct {
    unsigned seq;
    uint32_t op;
    uint32_t args[6];
    uint32_t fb;
    uint32_t prog;
    uint32_t active_tex;
    uint32_t tex2d;
} op_ring_entry;
static op_ring_entry op_ring[64];
static unsigned op_ring_next;
static unsigned op_ring_count;
static unsigned op_ring_dumps;
static int pixel_probe_enabled;
static unsigned pixel_swap_count;
static int pixel_dump_enabled;
static int fbo_transition_diag = -1;
static uint32_t fbo_transition_prev[64];
static int gl_error_trace = -1;
static uint32_t deferred_gl_error;
static unsigned gl_error_trace_count;
static int title_draw_probe = -1;
static unsigned title_draw_probe_count;
static int swap_finish;
static int swap_interval;
static uint32_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)ts.tv_sec * 1000u + (uint32_t)(ts.tv_nsec / 1000000u);
}

static void wrap_bind_fb(uint32_t target, uint32_t fb);
static void wrap_get_integerv(uint32_t pname, int32_t *params);
static uint32_t wrap_check_fb(uint32_t target);

#define TSPGL_STAGE (4u * 1024u * 1024u)
static uint8_t tspgl_stage_mem[2][TSPGL_STAGE];
static int tspgl_stage_cur;
static uint32_t tspgl_stage_used;
static void **tspgl_stage_extra[2];
static int tspgl_stage_en[2];
static int tspgl_stage_cap[2];

static const void *tspgl_stage(const void *src, uint32_t n)
{
    uint32_t pad;
    uint8_t *base;
    int cur;
    void *p;
    static int logged_big;

    if (!src || n == 0)
        return src;
    pad = (n + 15u) & ~15u;
    cur = tspgl_stage_cur;
    base = tspgl_stage_mem[cur];
    if (tspgl_stage_used + pad <= TSPGL_STAGE) {
        memcpy(base + tspgl_stage_used, src, n);
        src = base + tspgl_stage_used;
        tspgl_stage_used += pad;
        return src;
    }
    /* Never return `src`: that pointer dies when handle_client frees the
     * socket buffer, while PowerVR may still DMA. Arena is 4MB; chicago
     * lightmaps are ~16MB RGBA, so they always take this path. */
    if (tspgl_stage_en[cur] == tspgl_stage_cap[cur]) {
        int nc = tspgl_stage_cap[cur] ? tspgl_stage_cap[cur] * 2 : 64;
        void **nb = realloc(tspgl_stage_extra[cur], (size_t)nc * sizeof(void *));
        if (!nb) {
            fprintf(stderr, "tspgl-srv: stage realloc fail n=%u extra=%d\n", n,
                    tspgl_stage_en[cur]);
            return src;
        }
        tspgl_stage_extra[cur] = nb;
        tspgl_stage_cap[cur] = nc;
    }
    p = malloc(n);
    if (!p) {
        fprintf(stderr, "tspgl-srv: stage malloc %u fail\n", n);
        return src;
    }
    memcpy(p, src, n);
    tspgl_stage_extra[cur][tspgl_stage_en[cur]++] = p;
    if (!logged_big && n > TSPGL_STAGE) {
        fprintf(stderr, "tspgl-srv: stage extra %u bytes (arena full, extras=%d)\n",
                n, tspgl_stage_en[cur]);
        logged_big = 1;
    }
    return p;
}

static void tspgl_stage_flip(void)
{
    int next = tspgl_stage_cur ^ 1;
    int i;
    for (i = 0; i < tspgl_stage_en[next]; ++i)
        free(tspgl_stage_extra[next][i]);
    tspgl_stage_en[next] = 0;
    tspgl_stage_cur = next;
    tspgl_stage_used = 0;
}

#include "server_gen.c"
#include "server_gl.c"

static void fbo_format_matrix(void)
{
    const char *v = getenv("GUACAMELEE_FBO_FORMAT_MATRIX");
    void (*gen_fb)(int32_t, uint32_t *) = (void *)G.glGenFramebuffers;
    void (*del_fb)(int32_t, const uint32_t *) = (void *)G.glDeleteFramebuffers;
    void (*bind_fb)(uint32_t, uint32_t) = (void *)G.glBindFramebuffer;
    void (*gen_rb)(int32_t, uint32_t *) = (void *)G.glGenRenderbuffers;
    void (*del_rb)(int32_t, const uint32_t *) = (void *)G.glDeleteRenderbuffers;
    void (*bind_rb)(uint32_t, uint32_t) = (void *)G.glBindRenderbuffer;
    void (*rb_storage)(uint32_t, uint32_t, int32_t, int32_t) =
        (void *)G.glRenderbufferStorage;
    void (*fb_rb)(uint32_t, uint32_t, uint32_t, uint32_t) =
        (void *)G.glFramebufferRenderbuffer;
    void (*gen_tex)(int32_t, uint32_t *) = (void *)G.glGenTextures;
    void (*del_tex)(int32_t, const uint32_t *) = (void *)G.glDeleteTextures;
    void (*bind_tex)(uint32_t, uint32_t) = (void *)G.glBindTexture;
    void (*tex_image)(uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                      uint32_t, uint32_t, const void *) = (void *)G.glTexImage2D;
    void (*fb_tex)(uint32_t, uint32_t, uint32_t, uint32_t, int32_t) =
        (void *)G.glFramebufferTexture2D;
    uint32_t (*check_fb)(uint32_t) = (void *)G.glCheckFramebufferStatus;
    uint32_t (*get_error)(void) = (void *)G.glGetError;
    uint32_t fbo, color_tex, color_rb, depth_rb, stencil_rb;
    int32_t e_storage, e_attach;

    if (!v || atoi(v) == 0 || !gen_fb || !del_fb || !bind_fb || !gen_rb ||
        !del_rb || !bind_rb || !rb_storage || !fb_rb || !gen_tex ||
        !del_tex || !bind_tex || !tex_image || !fb_tex || !check_fb ||
        !get_error)
        return;

#define MATRIX_ROW(name, color_kind, packed, combined, with_stencil) do {                 \
        color_tex = color_rb = depth_rb = stencil_rb = 0;                    \
        gen_fb(1, &fbo); bind_fb(GL_FRAMEBUFFER, fbo);                      \
        if (color_kind == 0) {                                               \
            gen_tex(1, &color_tex); bind_tex(GL_TEXTURE_2D, color_tex);     \
            tex_image(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 768, 0, GL_RGBA,     \
                      GL_UNSIGNED_BYTE, NULL);                               \
            e_storage = (int32_t)get_error();                                \
            fb_tex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,      \
                   color_tex, 0);                                             \
        } else {                                                              \
            gen_rb(1, &color_rb); bind_rb(GL_RENDERBUFFER, color_rb);       \
            rb_storage(GL_RENDERBUFFER, 0x8056, 1024, 768);                  \
            e_storage = (int32_t)get_error();                                \
            fb_rb(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,     \
                  color_rb);                                                  \
        }                                                                         \
        e_attach = (int32_t)get_error();                                    \
        gen_rb(1, &depth_rb); bind_rb(GL_RENDERBUFFER, depth_rb);            \
        rb_storage(GL_RENDERBUFFER, (packed) ? GL_DEPTH24_STENCIL8 :         \
                   GL_DEPTH_COMPONENT16, 1024, 768);                         \
        e_storage = (e_storage << 16) | (int32_t)(get_error() & 0xffff);     \
        fb_rb(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,          \
              depth_rb);                                                       \
        if (combined) fb_rb(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,     \
                            GL_RENDERBUFFER, depth_rb);                       \
        e_attach = (e_attach << 16) | (int32_t)(get_error() & 0xffff);       \
        if (!packed && with_stencil) {                                                         \
            gen_rb(1, &stencil_rb); bind_rb(GL_RENDERBUFFER, stencil_rb);    \
            rb_storage(GL_RENDERBUFFER, 0x8D48, 1024, 768);                  \
            e_storage = (e_storage << 16) | (int32_t)(get_error() & 0xffff); \
            fb_rb(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,    \
                  stencil_rb);                                                \
            e_attach = (e_attach << 16) | (int32_t)(get_error() & 0xffff);   \
        } else if (!combined) {                                                \
            fb_rb(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,    \
                  depth_rb);                                                   \
            e_attach = (e_attach << 16) | (int32_t)(get_error() & 0xffff);   \
        }                                                                       \
        fprintf(stderr, "GUA-FBO-MATRIX %s storage=0x%x attach=0x%x "        \
                "status=0x%x depth_rb=%u stencil_rb=%u\\n", name,          \
                (unsigned)e_storage, (unsigned)e_attach,                   \
                check_fb(GL_FRAMEBUFFER), depth_rb,                          \
                packed ? depth_rb : (stencil_rb));                           \
        if (color_kind == 0) del_tex(1, &color_tex);                         \
        if (color_kind != 0) del_rb(1, &color_rb);                            \
        if (!packed && with_stencil) del_rb(1, &stencil_rb);                                  \
        del_rb(1, &depth_rb); del_fb(1, &fbo);                                \
    } while (0)

    MATRIX_ROW("M1 color=RGBA depth=D16", 0, 0, 0, 0);
    MATRIX_ROW("M2 color=RGBA depth=D16 stencil=S8", 0, 0, 0, 1);
    MATRIX_ROW("M3 color=RGBA packed=D24S8 separate", 0, 1, 0, 0);
    MATRIX_ROW("M3b color=RGBA packed=D24S8 combined", 0, 1, 1, 0);
    MATRIX_ROW("M4 color=RGBA4 depth=D16 stencil=S8", 1, 0, 0, 1);
    MATRIX_ROW("M5 color=RGBA4 packed=D24S8 separate", 1, 1, 0, 0);
    MATRIX_ROW("M5b color=RGBA4 packed=D24S8 combined", 1, 1, 1, 0);
#undef MATRIX_ROW
    bind_fb(GL_FRAMEBUFFER, 0);
    fprintf(stderr, "GUA-FBO-MATRIX done\\n");
}

static int load_sdl(void)
{
    const char *candidates[] = {
        "/usr/trimui/lib/libSDL2-2.0.so.0",
        "libSDL2-2.0.so.0",
        "libSDL2.so",
    };
    size_t i;

    memset(&sdl, 0, sizeof(sdl));
    for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
        sdl.lib = dlopen(candidates[i], RTLD_NOW | RTLD_GLOBAL);
        if (sdl.lib)
            break;
    }
    if (!sdl.lib) {
        fprintf(stderr, "tspgl-srv: dlopen SDL2: %s\n", dlerror());
        return -1;
    }
#define LOAD(member, name)                                                     \
    do {                                                                       \
        sdl.member = dlsym(sdl.lib, name);                                     \
        if (!sdl.member) {                                                     \
            fprintf(stderr, "tspgl-srv: missing %s\n", name);                  \
            return -1;                                                         \
        }                                                                      \
    } while (0)
#define LOAD_OPT(member, name) sdl.member = dlsym(sdl.lib, name)
    LOAD(init, "SDL_Init");
    LOAD(quit, "SDL_Quit");
    LOAD(get_error, "SDL_GetError");
    LOAD_OPT(get_current_video_driver, "SDL_GetCurrentVideoDriver");
    LOAD(set_hint, "SDL_SetHint");
    LOAD(create_window, "SDL_CreateWindow");
    LOAD(destroy_window, "SDL_DestroyWindow");
    LOAD(gl_set_attr, "SDL_GL_SetAttribute");
    LOAD(gl_create_context, "SDL_GL_CreateContext");
    LOAD(gl_delete_context, "SDL_GL_DeleteContext");
    LOAD(gl_make_current, "SDL_GL_MakeCurrent");
    LOAD(gl_swap, "SDL_GL_SwapWindow");
    LOAD(gl_get_proc, "SDL_GL_GetProcAddress");
    LOAD_OPT(gl_set_swap, "SDL_GL_SetSwapInterval");
    LOAD(show_cursor, "SDL_ShowCursor");
    LOAD(poll_event, "SDL_PollEvent");
    LOAD(delay, "SDL_Delay");
    LOAD_OPT(num_joysticks, "SDL_NumJoysticks");
    LOAD_OPT(is_game_controller, "SDL_IsGameController");
    LOAD_OPT(controller_open, "SDL_GameControllerOpen");
    LOAD_OPT(controller_name, "SDL_GameControllerName");
    LOAD_OPT(controller_get_button, "SDL_GameControllerGetButton");
    LOAD_OPT(controller_get_axis, "SDL_GameControllerGetAxis");
#undef LOAD
#undef LOAD_OPT
    return 0;
}

static void *gl_get(const char *name)
{
    void *p = NULL;
    if (sdl.gl_get_proc)
        p = sdl.gl_get_proc(name);
    if (!p)
        p = dlsym(RTLD_DEFAULT, name);
    return p;
}

static void khr_debug_cb(uint32_t source, uint32_t type, uint32_t id,
                         uint32_t severity, int32_t length,
                         const char *message, const void *user)
{
    int n = length;
    (void)user;
    if (n < 0 || n > 512)
        n = 512;
    fprintf(stderr,
            "GUA-KHRDBG seq=%u op=%s/%u fb=%u prog=%u source=0x%x "
            "type=0x%x id=%u severity=0x%x args=%u,%u,%u,%u,%u,%u msg=%.*s\\n",
            khr_seq, gl_diag_op_name(khr_op), khr_op, khr_fb, khr_prog,
            source, type, id, severity, khr_args[0], khr_args[1], khr_args[2],
            khr_args[3], khr_args[4], khr_args[5], n,
            message ? message : "");
}

static void khr_debug_setup(void)
{
    const char *v = getenv("GUACAMELEE_KHR_DEBUG");
    void (*callback)(tspgl_debug_callback, const void *);
    void (*control)(uint32_t, uint32_t, uint32_t, int32_t,
                    const uint32_t *, uint32_t);
    void *cbp;
    void *ctlp;

    khr_debug_enabled = v && strcmp(v, "0") != 0;
    if (!khr_debug_enabled)
        return;
    cbp = gl_get("glDebugMessageCallback");
    ctlp = gl_get("glDebugMessageControl");
    if (!cbp || !ctlp) {
        cbp = gl_get("glDebugMessageCallbackKHR");
        ctlp = gl_get("glDebugMessageControlKHR");
    }
    if (!cbp || !ctlp) {
        fprintf(stderr, "GUA-KHRDBG unavailable callback=%p control=%p\\n",
                cbp, ctlp);
        return;
    }
    callback = (void (*)(tspgl_debug_callback, const void *))cbp;
    control = (void (*)(uint32_t, uint32_t, uint32_t, int32_t,
                        const uint32_t *, uint32_t))ctlp;
    callback(khr_debug_cb, NULL);
    control(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, 1);
    if (G.glEnable) {
        G.glEnable(GL_DEBUG_OUTPUT);
        G.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
    fprintf(stderr, "GUA-KHRDBG enabled callback=%p control=%p\\n", cbp, ctlp);
}

static void op_ring_capture(uint32_t seq, uint32_t op, const uint8_t *in,
                            uint32_t len)
{
    op_ring_entry *e;
    unsigned i, n;
    int32_t x;
    const char *v = getenv("GUACAMELEE_OP_RING");
    if (!v || strcmp(v, "0") == 0)
        return;
    e = &op_ring[op_ring_next];
    memset(e, 0, sizeof(*e));
    e->seq = seq;
    e->op = op;
    n = len / 4u;
    if (n > 6)
        n = 6;
    for (i = 0; i < n; ++i)
        memcpy(&e->args[i], in + i * 4u, 4);
    if (real_get_integerv) {
        real_get_integerv(GL_FRAMEBUFFER_BINDING, &x);
        e->fb = x > 0 ? (uint32_t)x : 0;
        real_get_integerv(GL_CURRENT_PROGRAM, &x);
        e->prog = x > 0 ? (uint32_t)x : 0;
        real_get_integerv(GL_ACTIVE_TEXTURE, &x);
        e->active_tex = x > 0 ? (uint32_t)x : 0;
        real_get_integerv(GL_TEXTURE_BINDING_2D, &x);
        e->tex2d = x > 0 ? (uint32_t)x : 0;
    }
    op_ring_next = (op_ring_next + 1u) % 64u;
    if (op_ring_count < 64u)
        op_ring_count++;
}

static void op_ring_dump(uint32_t error)
{
    unsigned start, i;
    if (error != 0x0502 || op_ring_dumps >= 8 || !op_ring_count)
        return;
    start = (op_ring_next + 64u - op_ring_count) % 64u;
    fprintf(stderr, "GUA-RING error=0x%x count=%u dump=%u\\n", error,
            op_ring_count, op_ring_dumps++);
    for (i = 0; i < op_ring_count; ++i) {
        op_ring_entry *e = &op_ring[(start + i) % 64u];
        fprintf(stderr,
                "GUA-RING seq=%u op=%s/%u args=%u,%u,%u,%u,%u,%u "
                "fb=%u prog=%u active=0x%x tex2d=%u\\n",
                e->seq, gl_diag_op_name(e->op), e->op, e->args[0], e->args[1],
                e->args[2], e->args[3], e->args[4], e->args[5], e->fb,
                e->prog, e->active_tex, e->tex2d);
    }
}

static uint8_t *open_frame(void)
{
    uint8_t *map;
    int fd = open(NFSMW_FRAME_PATH, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        fprintf(stderr, "tspgl-srv: open frame errno=%d\n", errno);
        return NULL;
    }
    if (ftruncate(fd, (off_t)NFSMW_FRAME_FILE_SIZE) != 0) {
        fprintf(stderr, "tspgl-srv: ftruncate errno=%d\n", errno);
        close(fd);
        return NULL;
    }
    map = mmap(NULL, NFSMW_FRAME_FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
               fd, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr, "tspgl-srv: mmap errno=%d\n", errno);
        close(fd);
        return NULL;
    }
    frame_fd = fd;
    return map;
}

static void *open_pad(void)
{
    int n, i;
    void *p;
    if (!sdl.num_joysticks || !sdl.is_game_controller || !sdl.controller_open)
        return NULL;
    n = sdl.num_joysticks();
    for (i = 0; i < n; ++i) {
        if (sdl.is_game_controller(i) == 0)
            continue;
        p = sdl.controller_open(i);
        if (p) {
            const char *name = sdl.controller_name ? sdl.controller_name(p) : NULL;
            fprintf(stderr, "tspgl-srv: pad=%s index=%d joysticks=%d\n",
                    name ? name : "?", i, n);
            return p;
        }
    }
    fprintf(stderr, "tspgl-srv: no game controller (joysticks=%d)\n", n);
    return NULL;
}

static int16_t axis_deadzone(int32_t v, int dz)
{
    if (v > -dz && v < dz)
        return 0;
    if (v > 32767)
        v = 32767;
    if (v < -32768)
        v = -32768;
    return (int16_t)v;
}

static void publish_input(uint32_t *hdr)
{
    uint32_t buttons = 0;
    int i;

    if (!pad || !sdl.controller_get_button || !sdl.controller_get_axis)
        return;
    for (i = 0; i < 15; ++i) {
        if (sdl.controller_get_button(pad, i))
            buttons |= 1u << i;
    }
    /* Select toggles the overlay cursor in the 32-bit runtime. Pass it
     * through; Select+Start remains the quit chord there. */
    hdr[NFSMW_HDR_BUTTONS] = buttons;
    for (i = 0; i < 6; ++i) {
        int32_t v = (int32_t)sdl.controller_get_axis(pad, i);
        hdr[NFSMW_HDR_AXIS0 + i] =
            (uint32_t)(int32_t)axis_deadzone(v, i < 4 ? 10000 : 4000);
    }
    hdr[NFSMW_HDR_PAD_SEQ] = hdr[NFSMW_HDR_PAD_SEQ] + 1u;
}

static int create_game_fbo(void)
{
    void (*gen_tex)(int32_t, uint32_t *) = G.glGenTextures;
    void (*bind_tex)(uint32_t, uint32_t) = G.glBindTexture;
    void (*tex_parami)(uint32_t, uint32_t, int32_t) = G.glTexParameteri;
    void (*tex_image)(uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                      uint32_t, uint32_t, const void *);
    void (*gen_rb)(int32_t, uint32_t *) = G.glGenRenderbuffers;
    void (*bind_rb)(uint32_t, uint32_t) = G.glBindRenderbuffer;
    void (*rb_store)(uint32_t, uint32_t, int32_t, int32_t) =
        G.glRenderbufferStorage;
    void (*gen_fb)(int32_t, uint32_t *) = G.glGenFramebuffers;
    void (*bind_fb)(uint32_t, uint32_t) = G.glBindFramebuffer;
    void (*fb_tex)(uint32_t, uint32_t, uint32_t, uint32_t, int32_t) =
        G.glFramebufferTexture2D;
    void (*fb_rb)(uint32_t, uint32_t, uint32_t, uint32_t) =
        G.glFramebufferRenderbuffer;
    uint32_t (*check)(uint32_t) = G.glCheckFramebufferStatus;
    uint32_t status;

    tex_image = (void *)gl_get("glTexImage2D");
    if (!gen_tex || !bind_tex || !tex_parami || !tex_image || !gen_rb ||
        !bind_rb || !rb_store || !gen_fb || !bind_fb || !fb_tex || !fb_rb ||
        !check) {
        fprintf(stderr, "tspgl-srv: missing FBO entry points\n");
        return -1;
    }

    gen_tex(1, &game_color);
    bind_tex(GL_TEXTURE_2D, game_color);
    tex_parami(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int32_t)GL_NEAREST);
    tex_parami(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int32_t)GL_NEAREST);
    tex_parami(GL_TEXTURE_2D, 0x2802, (int32_t)GL_CLAMP_TO_EDGE);
    tex_parami(GL_TEXTURE_2D, 0x2803, (int32_t)GL_CLAMP_TO_EDGE);
    tex_image(GL_TEXTURE_2D, 0, (int32_t)GL_RGBA, game_w, game_h, 0, GL_RGBA,
              GL_UNSIGNED_BYTE, NULL);

    gen_rb(1, &game_depth);
    bind_rb(GL_RENDERBUFFER, game_depth);
    rb_store(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, game_w, game_h);
    if (G.glGetError && G.glGetError() != 0) {
        rb_store(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, game_w, game_h);
        fprintf(stderr, "tspgl-srv: depth fallback DEPTH_COMPONENT16\n");
    }

    gen_fb(1, &game_fbo);
    bind_fb(GL_FRAMEBUFFER, game_fbo);
    fb_tex(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, game_color, 0);
    fb_rb(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
          game_depth);
    status = check(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fb_rb(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, game_depth);
        status = check(GL_FRAMEBUFFER);
    }
    fprintf(stderr, "tspgl-srv: game FBO %u color=%u depth=%u %dx%d status=0x%x\n",
            game_fbo, game_color, game_depth, game_w, game_h, status);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        return -1;
    return 0;
}

static uint32_t wrap_check_fb(uint32_t target)
{
    static unsigned wrap_diag;
    uint32_t st;
    if (!real_check_fb)
        return 0;
    st = real_check_fb(target);
    if (gl_diag_enabled() && wrap_diag < 32)
        fprintf(stderr, "GUA-FBO wrap-check#%u target=0x%x initial=0x%x\\n",
                wrap_diag++, target, st);
    if (st == GL_FRAMEBUFFER_COMPLETE)
        return st;
    if (real_draw_buffers) {
        void (*get_att)(uint32_t, uint32_t, uint32_t, int32_t *) =
            (void *)G.glGetFramebufferAttachmentParameteriv;
        int32_t color_type = 0;
        uint32_t none = GL_NONE;
        uint32_t color = GL_COLOR_ATTACHMENT0;

        /* A color attachment that is still incomplete must not get
         * DrawBuffers(NONE): the shadow/reflection texture stays black and
         * is projected onto the road. */
        if (get_att) {
            get_att(target, GL_COLOR_ATTACHMENT0,
                    GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &color_type);
            if (gl_diag_enabled() && wrap_diag < 32)
                fprintf(stderr,
                        "GUA-FBO wrap-color-type=0x%x target=0x%x\\n",
                        (unsigned)color_type, target);
            if (color_type != (int32_t)GL_NONE)
                return st;
        }
        if (depth_only_read_none && real_read_buffer)
            real_read_buffer(GL_NONE);
        real_draw_buffers(1, &none);
        st = real_check_fb(target);
        if (gl_diag_enabled() && wrap_diag < 32)
            fprintf(stderr, "GUA-FBO wrap-after-none=0x%x target=0x%x\\n",
                    st, target);
        if (st == GL_FRAMEBUFFER_COMPLETE) {
            static int once;
            if (!once) {
                fprintf(stderr,
                        "tspgl-srv: depth-only FBO via DrawBuffers(NONE)\n");
                once = 1;
            }
            return st;
        }
        if (depth_only_read_none && real_read_buffer)
            real_read_buffer(GL_COLOR_ATTACHMENT0);
        real_draw_buffers(1, &color);
        st = real_check_fb(target);
    }
    return st;
}

static void gl_diag_fbo_lifecycle(uint32_t op, const uint8_t *in,
                                  uint32_t len)
{
    uint32_t u[5] = {0, 0, 0, 0, 0};
    int32_t fb = 0, rb = 0, prog = 0, vp[4] = {0, 0, 0, 0};
    unsigned i, n;

    if (!gl_diag_lifecycle_enabled() || gl_diag_lifecycle_ops >= 2000 ||
        (!gl_diag_is_fbo_op(op) && op != OP_glDrawArrays &&
         op != OP_glDrawElements))
        return;
    n = len / 4u;
    if (n > 5)
        n = 5;
    for (i = 0; i < n; ++i)
        memcpy(&u[i], in + i * 4u, 4);
    if (real_get_integerv) {
        real_get_integerv(GL_FRAMEBUFFER_BINDING, &fb);
        real_get_integerv(GL_RENDERBUFFER_BINDING, &rb);
        real_get_integerv(GL_CURRENT_PROGRAM, &prog);
    }
    if (op == OP_glBindFramebuffer) {
        fprintf(stderr, "GUA-FBO bind-fb requested=%u actual=%d\\n",
                u[1], (int)fb);
    } else if (op == OP_glBindRenderbuffer) {
        fprintf(stderr, "GUA-FBO bind-rb requested=%u actual=%d\\n",
                u[1], (int)rb);
    } else if (op == OP_glCopyTexImage2D && real_get_integerv) {
        int32_t tex = 0;
        real_get_integerv(GL_TEXTURE_BINDING_2D, &tex);
        fprintf(stderr,
                "GUA-TEX copy fb=%d tex=%d target=0x%x level=%d "
                "ifmt=0x%x xy=%d,%d size=%dx%d border=%d\\n",
                (int)fb, (int)tex, u[0], (int32_t)u[1], u[2],
                (int32_t)u[3], (int32_t)u[4], (int32_t)u[5],
                (int32_t)u[6], (int32_t)u[7]);
    } else if (op == OP_glRenderbufferStorage && G.glGetRenderbufferParameteriv) {
        int32_t w = 0, h = 0, fmt = 0;
        void (*get_rb)(uint32_t, uint32_t, int32_t *) =
            (void *)G.glGetRenderbufferParameteriv;
        get_rb(GL_RENDERBUFFER, 0x8D42, &w);
        get_rb(GL_RENDERBUFFER, 0x8D43, &h);
        get_rb(GL_RENDERBUFFER, 0x8D44, &fmt);
        fprintf(stderr,
                "GUA-FBO rb-state fb=%d rb=%d fmt=0x%x requested=%dx%d actual=%dx%d internal=0x%x\\n",
                (int)fb, (int)rb, u[1], (int32_t)u[2], (int32_t)u[3],
                (int)w, (int)h, (unsigned)fmt);
    } else if (op == OP_glFramebufferRenderbuffer ||
               op == OP_glFramebufferTexture2D) {
        int32_t type = 0, name = 0, level = 0;
        void (*get_att)(uint32_t, uint32_t, uint32_t, int32_t *) =
            (void *)G.glGetFramebufferAttachmentParameteriv;
        if (get_att) {
            get_att(GL_FRAMEBUFFER, u[1], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                    &type);
            get_att(GL_FRAMEBUFFER, u[1], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                    &name);
            if (type == (int32_t)GL_TEXTURE)
                get_att(GL_FRAMEBUFFER, u[1], GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,
                        &level);
        }
        fprintf(stderr,
                "GUA-FBO attachment fb=%d slot=0x%x type=0x%x name=%d level=%d\\n",
                (int)fb, u[1], (unsigned)type, (int)name, (int)level);
    } else if (op == OP_glCheckFramebufferStatus && real_check_fb) {
        fprintf(stderr, "GUA-FBO status fb=%d status=0x%x\\n", (int)fb,
                real_check_fb(GL_FRAMEBUFFER));
    } else if (op == OP_glDrawArrays || op == OP_glDrawElements) {
        if (real_get_integerv)
            real_get_integerv(GL_VIEWPORT, vp);
        gl_diag_last_draw_fb = (uint32_t)fb;
        fprintf(stderr, "GUA-DRAW fb=%d program=%d viewport=%d,%d,%d,%d op=%s\\n",
                (int)fb, (int)prog, vp[0], vp[1], vp[2], vp[3],
                op == OP_glDrawArrays ? "DrawArrays" : "DrawElements");
    }
    gl_diag_lifecycle_ops++;
}

static int gl_error_trace_enabled(void)
{
    const char *v;
    if (gl_error_trace >= 0)
        return gl_error_trace;
    v = getenv("GUACAMELEE_GL_ERROR_TRACE");
    gl_error_trace = v && strcmp(v, "0") != 0;
    return gl_error_trace;
}

static void gl_error_trace_after(uint32_t op, uint32_t seq)
{
    uint32_t (*get_error)(void);
    uint32_t err;
    int32_t fb = 0, prog = 0;
    if (!gl_error_trace_enabled() || op == OP_glGetError ||
        !G.glGetError || gl_error_trace_count >= 256)
        return;
    get_error = (void *)G.glGetError;
    err = get_error();
    if (!err)
        return;
    if (real_get_integerv) {
        real_get_integerv(GL_FRAMEBUFFER_BINDING, &fb);
        real_get_integerv(GL_CURRENT_PROGRAM, &prog);
    }
    fprintf(stderr, "GUA-GL-ERR seq=%u op=%s/%u fb=%d prog=%d error=0x%x\\n",
            seq, gl_diag_op_name(op), op, (int)fb, (int)prog, err);
    ++gl_error_trace_count;
    deferred_gl_error = err;
}

static void gl_title_draw_probe(uint32_t op, uint32_t seq)
{
    const char *v;
    int32_t fb = 0;
    uint8_t pixels[16 * 16 * 4];
    unsigned i, nonblack = 0;
    uint64_t hash = 1469598103934665603ULL;
    void (*read_pixels)(int32_t, int32_t, int32_t, int32_t, uint32_t,
                        uint32_t, void *) = (void *)G.glReadPixels;

    if (title_draw_probe < 0) {
        v = getenv("GUACAMELEE_TITLE_DRAW_PROBE");
        title_draw_probe = v && strcmp(v, "0") != 0;
    }
    if (!title_draw_probe || title_draw_probe_count >= 16 ||
        (op != OP_glDrawArrays && op != OP_glDrawElements) ||
        !real_get_integerv || !read_pixels)
        return;
    real_get_integerv(GL_FRAMEBUFFER_BINDING, &fb);
    if (fb != 2)
        return;
    memset(pixels, 0, sizeof(pixels));
    read_pixels(0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    for (i = 0; i < sizeof(pixels); ++i) {
        hash ^= pixels[i];
        hash *= 1099511628211ULL;
        if ((i & 3u) != 3u && pixels[i] != 0)
            ++nonblack;
    }
    fprintf(stderr, "GUA-TITLE-PIX draw=%u seq=%u op=%s fb=%d nonblack=%u/768 "
            "hash=%016llx\\n", title_draw_probe_count++, seq,
            op == OP_glDrawArrays ? "DrawArrays" : "DrawElements", (int)fb,
            nonblack, (unsigned long long)hash);
}

static void gl_diag_fbo_transition(uint32_t op, const uint8_t *in,
                                    uint32_t len)
{
    static const uint32_t slots[] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT,
                                     GL_STENCIL_ATTACHMENT};
    uint32_t a[8] = {0};
    int32_t fb = 0, old_rb = 0;
    uint32_t status, old;
    unsigned i, n;
    const char *v;
    void (*get_att)(uint32_t, uint32_t, uint32_t, int32_t *) =
        (void *)G.glGetFramebufferAttachmentParameteriv;
    void (*get_rb)(uint32_t, uint32_t, int32_t *) =
        (void *)G.glGetRenderbufferParameteriv;
    void (*bind_rb)(uint32_t, uint32_t) = (void *)G.glBindRenderbuffer;

    if (fbo_transition_diag < 0) {
        v = getenv("GUACAMELEE_FBO_TRANSITION_DIAG");
        fbo_transition_diag = v && strcmp(v, "0") != 0;
    }
    if (!fbo_transition_diag || !real_check_fb || !real_get_integerv ||
        !get_att || !G.glGetRenderbufferParameteriv || !bind_rb ||
        !gl_diag_is_fbo_op(op) ||
        (op != OP_glFramebufferRenderbuffer &&
         op != OP_glFramebufferTexture2D && op != OP_glRenderbufferStorage &&
         op != OP_glTexImage2D && op != OP_glCopyTexImage2D &&
         op != OP_glDeleteRenderbuffers && op != OP_glDeleteTextures))
        return;
    n = len / 4u;
    if (n > 8u)
        n = 8u;
    for (i = 0; i < n; ++i)
        memcpy(&a[i], in + i * 4u, 4u);
    real_get_integerv(GL_FRAMEBUFFER_BINDING, &fb);
    if (fb <= 0 || fb >= (int32_t)(sizeof(fbo_transition_prev) /
                                    sizeof(fbo_transition_prev[0])))
        return;
    status = real_check_fb(GL_FRAMEBUFFER);
    old = fbo_transition_prev[fb];
    fbo_transition_prev[fb] = status;
    if (old == status)
        return;
    real_get_integerv(GL_RENDERBUFFER_BINDING, &old_rb);
    fprintf(stderr,
            "GUA-FBO-TRANS seq=%u trigger=%s/%u args=%x,%x,%x,%x "
            "fb=%d status=0x%x->0x%x ",
            gl_diag_lifecycle_ops, gl_diag_op_name(op), op, a[0], a[1], a[2],
            a[3], (int)fb, old, status);
    for (i = 0; i < 3; ++i) {
        int32_t type = 0, name = 0, w = 0, h = 0, fmt = 0;
        get_att(GL_FRAMEBUFFER, slots[i], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                &type);
        get_att(GL_FRAMEBUFFER, slots[i], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                &name);
        if (type == (int32_t)GL_RENDERBUFFER && name > 0) {
            bind_rb(GL_RENDERBUFFER, (uint32_t)name);
            get_rb(GL_RENDERBUFFER, 0x8D42, &w);
            get_rb(GL_RENDERBUFFER, 0x8D43, &h);
            get_rb(GL_RENDERBUFFER, 0x8D44, &fmt);
        }
        fprintf(stderr, "%s:type=0x%x id=%d size=%dx%d ifmt=0x%x ",
                i == 0 ? "color" : (i == 1 ? "depth" : "stencil"),
                (unsigned)type, (int)name, (int)w, (int)h, (unsigned)fmt);
    }
    if (old_rb > 0)
        bind_rb(GL_RENDERBUFFER, (uint32_t)old_rb);
    fprintf(stderr, "ds=%s\\n",
            ({
                int32_t dt = 0, st = 0, dn = 0, sn = 0;
                get_att(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &dt);
                get_att(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &st);
                get_att(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &dn);
                get_att(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &sn);
                (dt == (int32_t)GL_RENDERBUFFER &&
                 st == (int32_t)GL_RENDERBUFFER && dn == sn) ? "same" : "different";
            }));
}

static void wrap_bind_fb(uint32_t target, uint32_t fb)
{
    if (fb == 0)
        fb = game_fbo;
    if (real_bind_fb)
        real_bind_fb(target, fb);
}

static void wrap_get_integerv(uint32_t pname, int32_t *params)
{
    if (real_get_integerv)
        real_get_integerv(pname, params);
    if (params && pname == GL_FRAMEBUFFER_BINDING &&
        (uint32_t)params[0] == game_fbo)
        params[0] = 0;
}

static int is_depth_internal(int32_t fmt)
{
    switch ((uint32_t)fmt) {
    case GL_DEPTH_COMPONENT:
    case GL_DEPTH_COMPONENT16:
    case GL_DEPTH_COMPONENT24:
    case GL_DEPTH_COMPONENT32:
    case GL_DEPTH24_STENCIL8:
    case GL_DEPTH_STENCIL:
    case GL_DEPTH_COMPONENT32F:
    case GL_DEPTH32F_STENCIL8:
        return 1;
    default:
        return 0;
    }
}

/* GLES3 defaults depth textures to COMPARE_REF_TO_TEXTURE. A GLES2 title
 * samples them with sampler2D and gets 0 → pitch-black shadows/puddles. */
static int is_pot(int32_t n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

static int is_mip_filter(int32_t p)
{
    return p == (int32_t)GL_NEAREST_MIPMAP_NEAREST ||
           p == (int32_t)GL_LINEAR_MIPMAP_NEAREST ||
           p == (int32_t)GL_NEAREST_MIPMAP_LINEAR ||
           p == (int32_t)GL_LINEAR_MIPMAP_LINEAR;
}

static uint32_t param_target(uint32_t target)
{
    if (target == GL_TEXTURE_CUBE_MAP ||
        (target >= 0x8515u && target <= 0x851Au))
        return GL_TEXTURE_CUBE_MAP;
    return GL_TEXTURE_2D;
}

static uint32_t bound_tex(uint32_t ptarget)
{
    int32_t id = 0;
    if (!real_get_integerv)
        return 0;
    real_get_integerv(ptarget == GL_TEXTURE_CUBE_MAP ?
                      GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D,
                      &id);
    return id > 0 ? (uint32_t)id : 0;
}

static int tex_is_npot(uint32_t id)
{
    return id && id < 4096u && tex_npot[id];
}

static void mark_npot(uint32_t id, int npot)
{
    if (id && id < 4096u)
        tex_npot[id] = npot ? 1 : 0;
}

static void sanitize_npot(uint32_t ptarget)
{
    if (!real_tex_parami)
        return;
    real_tex_parami(ptarget, GL_TEXTURE_MIN_FILTER, (int32_t)GL_LINEAR);
    real_tex_parami(ptarget, GL_TEXTURE_MAG_FILTER, (int32_t)GL_LINEAR);
    real_tex_parami(ptarget, GL_TEXTURE_WRAP_S, (int32_t)GL_CLAMP_TO_EDGE);
    real_tex_parami(ptarget, GL_TEXTURE_WRAP_T, (int32_t)GL_CLAMP_TO_EDGE);
    real_tex_parami(ptarget, GL_TEXTURE_MAX_LEVEL, 0);
}

static void wrap_tex_image2d(uint32_t target, int32_t level, int32_t ifmt,
                             int32_t w, int32_t h, int32_t border,
                             uint32_t format, uint32_t type,
                             const void *pixels)
{
    uint32_t ptarget = param_target(target);
    uint32_t id;
    int npot;

    if (real_tex_image)
        real_tex_image(target, level, ifmt, w, h, border, format, type, pixels);
    if (gl_diag_lifecycle_enabled() && level == 0) {
        static unsigned tex_diag;
        int32_t fb = 0;
        if (real_get_integerv)
            real_get_integerv(GL_FRAMEBUFFER_BINDING, &fb);
        if (tex_diag < 256)
            fprintf(stderr,
                    "GUA-TEX image#%u fb=%d tex=%u target=0x%x "
                    "level=%d requested=%dx%d ifmt=0x%x format=0x%x type=0x%x\\n",
                    tex_diag++, (int)fb, bound_tex(param_target(target)), target,
                    (int)level, (int)w, (int)h, (unsigned)ifmt,
                    (unsigned)format, (unsigned)type);
    }
    if (is_depth_internal(ifmt) && real_tex_parami)
        real_tex_parami(ptarget, GL_TEXTURE_COMPARE_MODE, (int32_t)GL_NONE);
    if (level != 0)
        return;
    id = bound_tex(ptarget);
    npot = !is_pot(w) || !is_pot(h);
    mark_npot(id, npot);
    if (npot) {
        static int once;
        sanitize_npot(ptarget);
        if (!once) {
            fprintf(stderr, "tspgl-srv: NPOT %dx%d id=%u -> LINEAR+CLAMP\n",
                    (int)w, (int)h, id);
            once = 1;
        }
    }
}

static void wrap_compressed(uint32_t target, int32_t level, uint32_t ifmt,
                            int32_t w, int32_t h, int32_t border,
                            int32_t imageSize, const void *data)
{
    uint32_t ptarget = param_target(target);
    uint32_t id;
    int npot;

    if (real_compressed)
        real_compressed(target, level, ifmt, w, h, border, imageSize, data);
    if (level != 0)
        return;
    id = bound_tex(ptarget);
    npot = !is_pot(w) || !is_pot(h);
    mark_npot(id, npot);
    if (npot)
        sanitize_npot(ptarget);
}

static int npot_fix_param(uint32_t id, uint32_t pname, int32_t *param)
{
    if (!tex_is_npot(id))
        return 0;
    if (pname == GL_TEXTURE_MIN_FILTER && is_mip_filter(*param)) {
        *param = (int32_t)GL_LINEAR;
        return 1;
    }
    if (pname == GL_TEXTURE_WRAP_S || pname == GL_TEXTURE_WRAP_T) {
        *param = (int32_t)GL_CLAMP_TO_EDGE;
        return 1;
    }
    if (pname == GL_TEXTURE_MAX_LEVEL) {
        *param = 0;
        return 1;
    }
    return 0;
}

static void wrap_tex_parami(uint32_t target, uint32_t pname, int32_t param)
{
    uint32_t ptarget = param_target(target);
    uint32_t id = bound_tex(ptarget);

    npot_fix_param(id, pname, &param);
    if (real_tex_parami)
        real_tex_parami(target, pname, param);
}

static void wrap_tex_paramf(uint32_t target, uint32_t pname, float param)
{
    uint32_t ptarget = param_target(target);
    uint32_t id = bound_tex(ptarget);
    int32_t ip = (int32_t)param;

    if (npot_fix_param(id, pname, &ip))
        param = (float)ip;
    if (real_tex_paramf)
        real_tex_paramf(target, pname, param);
}

static void wrap_gen_mipmap(uint32_t target)
{
    uint32_t ptarget = param_target(target);
    if (tex_is_npot(bound_tex(ptarget)))
        return;
    if (real_gen_mipmap)
        real_gen_mipmap(target);
}

static void wrap_delete_tex(int32_t n, const uint32_t *ids)
{
    int32_t i;
    if (real_delete_tex)
        real_delete_tex(n, ids);
    if (!ids)
        return;
    for (i = 0; i < n; ++i)
        if (ids[i] && ids[i] < 4096u)
            tex_npot[ids[i]] = 0;
}

static const char splash_vs[] =
    "#version 100\n"
    "attribute vec2 a_pos;\n"
    "attribute vec2 a_uv;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "  v_uv = a_uv;\n"
    "}\n";

static const char splash_fs[] =
    "#version 100\n"
    "precision mediump float;\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(u_tex, v_uv);\n"
    "}\n";

static FILE *open_splash_rgb(void)
{
    static const char *const paths[] = {
        "splash.rgb",
        "/mnt/SDCARD/Data/ports/gof2/splash.rgb",
    };
    size_t i;

    for (i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        FILE *file = fopen(paths[i], "rb");
        if (file)
            return file;
    }
    return NULL;
}

static uint32_t splash_compile(uint32_t type, const char *src)
{
    uint32_t shader;
    int32_t ok = 0;
    void (*source)(uint32_t, int32_t, const char *const *, const int32_t *) =
        (void *)G.glShaderSource;
    void (*getiv)(uint32_t, uint32_t, int32_t *) = (void *)G.glGetShaderiv;
    const char *ptr = src;

    shader = G.glCreateShader(type);
    if (!shader || !source)
        return 0;
    source(shader, 1, &ptr, NULL);
    G.glCompileShader(shader);
    if (getiv)
        getiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
        return 0;
    return shader;
}

static int splash_setup(void)
{
    FILE *file;
    unsigned char *pixels;
    size_t want = (size_t)1280 * 720 * 3;
    size_t got;
    uint32_t vs;
    uint32_t fs;
    int32_t linked = 0;
    void (*tex_image)(uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                      uint32_t, uint32_t, const void *);
    void (*store)(uint32_t, int32_t) = (void *)G.glPixelStorei;
    int32_t (*get_attr)(uint32_t, const char *) = (void *)G.glGetAttribLocation;
    int32_t (*get_uni)(uint32_t, const char *) = (void *)G.glGetUniformLocation;
    void (*get_prog)(uint32_t, uint32_t, int32_t *) = (void *)G.glGetProgramiv;

    tex_image = real_tex_image;
    if (!tex_image)
        tex_image = (void *)G.glTexImage2D;
    if (!G.glCreateShader || !G.glCreateProgram || !G.glGenTextures ||
        !tex_image || !get_attr || !get_uni)
        return -1;
    file = open_splash_rgb();
    if (!file) {
        fprintf(stderr, "tspgl-srv: splash.rgb missing\n");
        return -1;
    }
    pixels = malloc(want);
    if (!pixels) {
        fclose(file);
        return -1;
    }
    got = fread(pixels, 1, want, file);
    fclose(file);
    if (got != want) {
        free(pixels);
        fprintf(stderr, "tspgl-srv: splash.rgb size=%zu want=%zu\n", got, want);
        return -1;
    }

    vs = splash_compile(GL_VERTEX_SHADER, splash_vs);
    fs = splash_compile(GL_FRAGMENT_SHADER, splash_fs);
    splash_prog = G.glCreateProgram();
    if (!vs || !fs || !splash_prog) {
        free(pixels);
        return -1;
    }
    G.glAttachShader(splash_prog, vs);
    G.glAttachShader(splash_prog, fs);
    G.glLinkProgram(splash_prog);
    if (get_prog)
        get_prog(splash_prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        free(pixels);
        return -1;
    }
    splash_loc_pos = get_attr(splash_prog, "a_pos");
    splash_loc_uv = get_attr(splash_prog, "a_uv");
    G.glUseProgram(splash_prog);
    G.glUniform1i(get_uni(splash_prog, "u_tex"), 0);

    G.glGenTextures(1, &splash_tex);
    G.glBindTexture(GL_TEXTURE_2D, splash_tex);
    if (store)
        store(GL_UNPACK_ALIGNMENT, 1);
    tex_image(GL_TEXTURE_2D, 0, (int32_t)GL_RGB, 1280, 720, 0, GL_RGB,
              GL_UNSIGNED_BYTE, pixels);
    free(pixels);
    if (real_tex_parami) {
        real_tex_parami(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int32_t)GL_LINEAR);
        real_tex_parami(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int32_t)GL_LINEAR);
        real_tex_parami(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int32_t)GL_CLAMP_TO_EDGE);
        real_tex_parami(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int32_t)GL_CLAMP_TO_EDGE);
    } else if (G.glTexParameteri) {
        G.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (int32_t)GL_LINEAR);
        G.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (int32_t)GL_LINEAR);
        G.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (int32_t)GL_CLAMP_TO_EDGE);
        G.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (int32_t)GL_CLAMP_TO_EDGE);
    }
    splash_live = 1;
    return 0;
}

static void splash_present(void)
{
    static const float quad[] = {
        -1.f, -1.f, 0.f, 1.f,
         1.f, -1.f, 1.f, 1.f,
        -1.f,  1.f, 0.f, 0.f,
         1.f,  1.f, 1.f, 0.f,
    };
    void (*bind_buf)(uint32_t, uint32_t) = (void *)G.glBindBuffer;
    void (*attrib)(uint32_t, int32_t, uint32_t, uint32_t, int32_t, const void *) =
        (void *)G.glVertexAttribPointer;
    void (*draw)(uint32_t, int32_t, int32_t) = (void *)G.glDrawArrays;
    void (*enable_attr)(uint32_t) = (void *)G.glEnableVertexAttribArray;

    if (!splash_live || !splash_prog || !splash_tex || !attrib || !draw ||
        !enable_attr)
        return;
    if (real_bind_fb)
        real_bind_fb(GL_FRAMEBUFFER, 0);
    else if (G.glBindFramebuffer)
        G.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (G.glViewport)
        G.glViewport(0, 0, win_w, win_h);
    if (G.glDisable) {
        G.glDisable(GL_DEPTH_TEST);
        G.glDisable(GL_CULL_FACE);
        G.glDisable(GL_BLEND);
        G.glDisable(GL_SCISSOR_TEST);
    }
    if (G.glColorMask)
        G.glColorMask(1u, 1u, 1u, 1u);
    if (G.glDepthMask)
        G.glDepthMask(0);
    if (bind_buf)
        bind_buf(GL_ARRAY_BUFFER, 0);
    G.glUseProgram(splash_prog);
    if (G.glActiveTexture)
        G.glActiveTexture(GL_TEXTURE0);
    G.glBindTexture(GL_TEXTURE_2D, splash_tex);
    enable_attr((uint32_t)splash_loc_pos);
    enable_attr((uint32_t)splash_loc_uv);
    attrib((uint32_t)splash_loc_pos, 2, GL_FLOAT, 0, 16, quad);
    attrib((uint32_t)splash_loc_uv, 2, GL_FLOAT, 0, 16, quad + 2);
    draw(GL_TRIANGLE_STRIP, 0, 4);
    sdl.gl_swap(window);
    if (G.glDepthMask)
        G.glDepthMask(1);
    if (game_fbo && real_bind_fb)
        real_bind_fb(GL_FRAMEBUFFER, game_fbo);
    else if (game_fbo && G.glBindFramebuffer)
        G.glBindFramebuffer(GL_FRAMEBUFFER, game_fbo);
}

static void present_draw_cursor(int dx, int dy, int dw, int dh)
{
    uint32_t *hdr;
    int gx, gy, sx, sy, gl_y;
    int arm;
    int32_t mask[4] = { 1, 1, 1, 1 };
    int32_t box[4] = { 0, 0, 0, 0 };
    uint32_t was_scissor = 0;

    if (!frame_map || !G.glScissor || !G.glClear || !G.glClearColor)
        return;
    hdr = (uint32_t *)frame_map;
    if (hdr[NFSMW_HDR_CURSOR_ON] == 0)
        return;
    gx = (int)(int32_t)hdr[NFSMW_HDR_CURSOR_X];
    gy = (int)(int32_t)hdr[NFSMW_HDR_CURSOR_Y];
    if (dw < 1)
        dw = win_w;
    if (dh < 1)
        dh = win_h;
    sx = dx + gx * dw / (game_w > 0 ? game_w : 1);
    sy = dy + gy * dh / (game_h > 0 ? game_h : 1);
    gl_y = win_h - 1 - sy;
    arm = 16;
    if (real_get_integerv) {
        real_get_integerv(GL_COLOR_WRITEMASK, mask);
        real_get_integerv(GL_SCISSOR_BOX, box);
    }
    if (G.glIsEnabled)
        was_scissor = G.glIsEnabled(GL_SCISSOR_TEST);
    if (G.glEnable)
        G.glEnable(GL_SCISSOR_TEST);
    if (G.glColorMask)
        G.glColorMask(1u, 1u, 1u, 1u);
    G.glClearColor(1.0f, 0.82f, 0.05f, 1.0f);
    G.glScissor(sx - arm, gl_y - 1, arm * 2 + 1, 3);
    G.glClear(GL_COLOR_BUFFER_BIT);
    G.glScissor(sx - 1, gl_y - arm, 3, arm * 2 + 1);
    G.glClear(GL_COLOR_BUFFER_BIT);
    if (real_get_integerv && G.glColorMask)
        G.glColorMask((uint32_t)mask[0], (uint32_t)mask[1],
                      (uint32_t)mask[2], (uint32_t)mask[3]);
    if (real_get_integerv && G.glScissor)
        G.glScissor(box[0], box[1], box[2], box[3]);
    if (!was_scissor && G.glDisable)
        G.glDisable(GL_SCISSOR_TEST);
}

static void pixel_probe_target(const char *stage, uint32_t fb, int w, int h,
                               unsigned swap)
{
    struct region { int x, y, w, h; } r[5];
    uint8_t pixels[32 * 32 * 4];
    uint64_t hash = 1469598103934665603ULL;
    unsigned nonblack = 0, total = 0, i, j;
    unsigned minv = 255, maxv = 0;
    int32_t old_read = 0, old_draw = 0, old_buf = GL_BACK, old_pack = 4;
    void (*read_pixels)(int32_t, int32_t, int32_t, int32_t, uint32_t,
                        uint32_t, void *) = (void *)G.glReadPixels;
    void (*pixel_store)(uint32_t, int32_t) = (void *)G.glPixelStorei;

    if (!pixel_probe_enabled || !read_pixels || !real_bind_fb || w < 32 || h < 32)
        return;
    if (real_get_integerv) {
        real_get_integerv(GL_READ_FRAMEBUFFER_BINDING, &old_read);
        real_get_integerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_draw);
        real_get_integerv(GL_READ_BUFFER, &old_buf);
        real_get_integerv(GL_PACK_ALIGNMENT, &old_pack);
    }
    r[0] = (struct region){w / 2 - 16, h / 2 - 16, 32, 32};
    r[1] = (struct region){w / 4 - 8, h / 4 - 8, 16, 16};
    r[2] = (struct region){3 * w / 4 - 8, h / 4 - 8, 16, 16};
    r[3] = (struct region){w / 4 - 8, 3 * h / 4 - 8, 16, 16};
    r[4] = (struct region){3 * w / 4 - 8, 3 * h / 4 - 8, 16, 16};
    real_bind_fb(GL_READ_FRAMEBUFFER, fb);
    if (real_read_buffer)
        real_read_buffer(fb ? GL_COLOR_ATTACHMENT0 : GL_BACK);
    if (pixel_store)
        pixel_store(GL_PACK_ALIGNMENT, 1);
    for (i = 0; i < 5; ++i) {
        size_t bytes = (size_t)r[i].w * (size_t)r[i].h * 4u;
        memset(pixels, 0, sizeof(pixels));
        read_pixels(r[i].x, r[i].y, r[i].w, r[i].h, GL_RGBA,
                    GL_UNSIGNED_BYTE, pixels);
        for (j = 0; j < bytes; ++j) {
            hash ^= pixels[j];
            hash *= 1099511628211ULL;
            if ((j & 3u) != 3u) {
                if (pixels[j] != 0)
                    nonblack++;
                if (pixels[j] < minv)
                    minv = pixels[j];
                if (pixels[j] > maxv)
                    maxv = pixels[j];
                total++;
            }
        }
    }
    if (pixel_dump_enabled && swap == 10) {
        size_t full_bytes = (size_t)w * (size_t)h * 4u;
        uint8_t *full = malloc(full_bytes);
        if (full) {
            char path[96];
            FILE *ppm;
            uint64_t full_hash = 1469598103934665603ULL;
            size_t k;
            read_pixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, full);
            for (k = 0; k < full_bytes; ++k) {
                full_hash ^= full[k];
                full_hash *= 1099511628211ULL;
            }
            snprintf(path, sizeof(path), "/tmp/tspgl-pixel-%s.ppm", stage);
            ppm = fopen(path, "wb");
            if (ppm) {
                int y;
                fprintf(ppm, "P6\n%d %d\n255\n", w, h);
                for (y = h - 1; y >= 0; --y) {
                    int x;
                    uint8_t *row = full + (size_t)y * (size_t)w * 4u;
                    for (x = 0; x < w; ++x)
                        fwrite(row + (size_t)x * 4u, 1u, 3u, ppm);
                }
                fclose(ppm);
                fprintf(stderr, "GUA-PIX dump swap=%u stage=%s path=%s "
                        "hash=%016llx\\n", swap, stage, path,
                        (unsigned long long)full_hash);
            }
            free(full);
        }
    }
    fprintf(stderr,
            "GUA-PIX swap=%u stage=%s fb=%u size=%dx%d hash=%016llx "
            "nonblack=%u/%u min=%u max=%u\\n", swap, stage, fb, w, h,
            (unsigned long long)hash, nonblack, total, minv, maxv);
    if (pixel_store)
        pixel_store(GL_PACK_ALIGNMENT, old_pack);
    if (real_read_buffer)
        real_read_buffer((uint32_t)old_buf);
    real_bind_fb(GL_READ_FRAMEBUFFER, old_read > 0 ? (uint32_t)old_read : 0);
    real_bind_fb(GL_DRAW_FRAMEBUFFER, old_draw > 0 ? (uint32_t)old_draw : 0);
}

static int pixel_probe_swap(unsigned swap)
{
    return swap == 1 || swap == 2 || swap == 3 || swap == 10 || swap == 30 ||
           swap == 300;
}

static void present_swap(void)
{
    void (*blit)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
                 int32_t, uint32_t, uint32_t) = G.glBlitFramebuffer;
    int dw, dh, dx, dy;
    unsigned swap = ++pixel_swap_count;

    if (!game_fbo) {
        present_draw_cursor(0, 0, win_w, win_h);
        sdl.gl_swap(window);
        tspgl_stage_flip();
        input_soon = 1;
        return;
    }
    int32_t vp[4];
    int32_t sc[4];
    uint32_t scissor_on = 0;

    vp[0] = 0;
    vp[1] = 0;
    vp[2] = game_w;
    vp[3] = game_h;
    sc[0] = 0;
    sc[1] = 0;
    sc[2] = game_w;
    sc[3] = game_h;

    if (present_letterbox) {
        if ((long)win_w * (long)game_h <= (long)win_h * (long)game_w) {
            dw = win_w;
            dh = (int)((long)game_h * win_w / game_w);
        } else {
            dh = win_h;
            dw = (int)((long)game_w * win_h / game_h);
        }
        if (dw < 1)
            dw = 1;
        if (dh < 1)
            dh = 1;
        dx = (win_w - dw) / 2;
        dy = (win_h - dh) / 2;
    } else {
        dw = win_w;
        dh = win_h;
        dx = 0;
        dy = 0;
    }

    if (real_get_integerv) {
        memset(vp, 0, sizeof(vp));
        memset(sc, 0, sizeof(sc));
        vp[2] = game_w;
        vp[3] = game_h;
        sc[2] = game_w;
        sc[3] = game_h;
        real_get_integerv(GL_VIEWPORT, vp);
        real_get_integerv(GL_SCISSOR_BOX, sc);
    }
    if (G.glIsEnabled)
        scissor_on = G.glIsEnabled(GL_SCISSOR_TEST);
    if (scissor_on && G.glDisable)
        G.glDisable(GL_SCISSOR_TEST);

    if (pixel_probe_swap(swap)) {
        if (gl_diag_last_draw_fb)
            pixel_probe_target("app", gl_diag_last_draw_fb, game_w, game_h, swap);
        pixel_probe_target("game", game_fbo, game_w, game_h, swap);
    }

    if (real_bind_fb) {
        real_bind_fb(GL_READ_FRAMEBUFFER, game_fbo);
        real_bind_fb(GL_DRAW_FRAMEBUFFER, 0);
    }
    if (G.glViewport)
        G.glViewport(0, 0, win_w, win_h);
    if (gl_diag_lifecycle_enabled()) {
        int32_t actual_fb = 0;
        if (real_get_integerv)
            real_get_integerv(GL_FRAMEBUFFER_BINDING, &actual_fb);
        fprintf(stderr, "GUA-SWAP swap=%u bound-fb=%d game-fbo=%u last-draw-fb=%u\\n",
                swap, (int)actual_fb, game_fbo, gl_diag_last_draw_fb);
    }
    if (blit)
        blit(0, 0, game_w, game_h, dx, dy, dx + dw, dy + dh,
             GL_COLOR_BUFFER_BIT, GL_NEAREST);
    if (pixel_probe_swap(swap))
        pixel_probe_target("window", 0, win_w, win_h, swap);
    if (swap_finish && G.glFinish)
        G.glFinish();
    present_draw_cursor(dx, dy, dw, dh);
    sdl.gl_swap(window);
    tspgl_stage_flip();
    input_soon = 1;
    if (real_bind_fb)
        real_bind_fb(GL_FRAMEBUFFER, game_fbo);
    if (G.glViewport)
        G.glViewport(vp[0], vp[1], vp[2], vp[3]);
    if (G.glScissor)
        G.glScissor(sc[0], sc[1], sc[2], sc[3]);
    if (scissor_on && G.glEnable)
        G.glEnable(GL_SCISSOR_TEST);
    else if (!scissor_on && G.glDisable)
        G.glDisable(GL_SCISSOR_TEST);
}

static int full_write(int fd, const void *buf, size_t n)
{
    const uint8_t *p = buf;
    while (n) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += (size_t)w;
        n -= (size_t)w;
    }
    return 0;
}

static int full_read(int fd, void *buf, size_t n)
{
    uint8_t *p = buf;
    while (n) {
        ssize_t r = read(fd, p, n);
        if (r < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (r == 0)
            return -1;
        p += (size_t)r;
        n -= (size_t)r;
    }
    return 0;
}

static uint8_t inbuf_s[512 * 1024];
static uint8_t outbuf_s[2 * 1024 * 1024];

static int drain_bytes(int fd, uint32_t n)
{
    uint8_t junk[4096];
    while (n) {
        uint32_t c = n > sizeof(junk) ? (uint32_t)sizeof(junk) : n;
        if (full_read(fd, junk, c) != 0)
            return -1;
        n -= c;
    }
    return 0;
}

static int handle_client(int fd)
{
    struct tspgl_hdr h, r;
    uint32_t out_n = 0;
    int rc;
    uint8_t *in = inbuf_s;
    uint8_t *out = outbuf_s;
    uint8_t *in_h = NULL;
    int ret = -1;

    if (full_read(fd, &h, sizeof(h)) != 0)
        return -1;
    r.magic = TSPGL_MAGIC;
    r.seq = h.seq;
    r.op = TSPGL_OK;
    r.len = 0;
    if (h.magic != TSPGL_MAGIC) {
        fprintf(stderr, "tspgl-srv: bad magic 0x%x fd=%d\n", h.magic, fd);
        return -1;
    }
    if (h.len > TSPGL_MAX_BLOB) {
        fprintf(stderr, "tspgl-srv: blob %u too big op=%u\n", h.len, h.op);
        if (drain_bytes(fd, h.len) != 0)
            return -1;
        if (tspgl_needs_reply(h.op)) {
            r.op = TSPGL_ERR;
            if (full_write(fd, &r, sizeof(r)) != 0)
                return -1;
        }
        return 0;
    }
    if (h.len > sizeof(inbuf_s)) {
        in_h = malloc(h.len);
        if (!in_h) {
            fprintf(stderr, "tspgl-srv: OOM len=%u op=%u\n", h.len, h.op);
            drain_bytes(fd, h.len);
            return -1;
        }
        in = in_h;
    }
    if (h.len && full_read(fd, in, h.len) != 0)
        goto out_free;

    gl_diag_op(h.op, h.len);
    if (h.op == OP_PING) {
        rc = 0;
    } else if (h.op == OP_EGL_SWAP) {
        splash_live = 0;
        if (gl_diag_enabled() && !gl_diag_swap) {
            fprintf(stderr, "GUA-GL first eglSwapBuffers\n");
            gl_diag_swap = 1;
        }
        present_swap();
        rc = 0;
    } else {
        const uint8_t *dispatch_in = in;
        uint8_t vp_in[16];
        gl_diag_fbo_call(h.op, in, h.len);
        if (zero_viewport && h.op == OP_glViewport && h.len == sizeof(vp_in)) {
            uint32_t *u;
            memcpy(vp_in, in, sizeof(vp_in));
            u = (uint32_t *)vp_in;
            if (u[2] == 0 && u[3] == 0) {
                u[2] = (uint32_t)game_w;
                u[3] = (uint32_t)game_h;
                dispatch_in = vp_in;
                fprintf(stderr, "GUA-FBO zero-viewport override %dx%d\\n",
                        game_w, game_h);
            }
        }
        if (khr_debug_enabled || getenv("GUACAMELEE_OP_RING")) {
            uint32_t i, n = h.len / 4u;
            int32_t x = 0;
            khr_seq = h.seq;
            khr_op = h.op;
            memset(khr_args, 0, sizeof(khr_args));
            if (n > 6)
                n = 6;
            for (i = 0; i < n; ++i)
                memcpy(&khr_args[i], in + i * 4u, 4);
            if (real_get_integerv) {
                real_get_integerv(GL_FRAMEBUFFER_BINDING, &x);
                khr_fb = x > 0 ? (uint32_t)x : 0;
                real_get_integerv(GL_CURRENT_PROGRAM, &x);
                khr_prog = x > 0 ? (uint32_t)x : 0;
            }
            op_ring_capture(h.seq, h.op, in, h.len);
        }
        rc = tspgl_dispatch_simple(h.op, (const uint32_t *)dispatch_in,
                                   h.len, (uint32_t *)out, &out_n);
        if (rc != 0)
            rc = tspgl_dispatch_special(h.op, dispatch_in, h.len, out,
                                         &out_n);
        if (h.op == OP_glGetError && out_n >= 4) {
            uint32_t err = 0;
            memcpy(&err, out, 4);
            op_ring_dump(err);
        }
        gl_diag_fbo_result(h.op, out, out_n, rc);
        gl_error_trace_after(h.op, h.seq);
        if (h.op == OP_glGetError && out_n >= 4 && deferred_gl_error) {
            memcpy(out, &deferred_gl_error, 4);
            deferred_gl_error = 0;
        }
        gl_diag_fbo_lifecycle(h.op, in, h.len);
        gl_diag_fbo_transition(h.op, in, h.len);
        gl_title_draw_probe(h.op, h.seq);
        if (h.op == OP_glFinish)
            tspgl_stage_flip();
        if (rc != 0) {
            fprintf(stderr, "tspgl-srv: unknown op %u len=%u\n", h.op, h.len);
            r.op = TSPGL_ERR;
            out_n = 0;
        } else if (h.op == OP_glCompileShader && h.len >= 4 && G.glGetShaderiv) {
            uint32_t shader;
            int32_t ok = 1;
            char slog[1024];
            int32_t slen = 0;
            void (*getiv)(uint32_t, uint32_t, int32_t *) = (void *)G.glGetShaderiv;
            void (*getlog)(uint32_t, int32_t, int32_t *, char *) =
                (void *)G.glGetShaderInfoLog;
            memcpy(&shader, in, 4);
            getiv(shader, 0x8B81, &ok);
            if (!ok) {
                memset(slog, 0, sizeof(slog));
                if (getlog)
                    getlog(shader, (int32_t)sizeof(slog) - 1, &slen, slog);
                fprintf(stderr, "GUA-GL shader-compile FAIL id=%u\n", shader);
                fprintf(stderr, "GUA-GL shader-log: %s\n", slog);
            }
        } else if (h.op == OP_glLinkProgram && h.len >= 4 && G.glGetProgramiv) {
            uint32_t prog;
            int32_t ok = 1;
            char plog[1024];
            int32_t plen = 0;
            void (*getiv)(uint32_t, uint32_t, int32_t *) = (void *)G.glGetProgramiv;
            void (*getlog)(uint32_t, int32_t, int32_t *, char *) =
                (void *)G.glGetProgramInfoLog;
            memcpy(&prog, in, 4);
            getiv(prog, 0x8B82, &ok);
            if (!ok) {
                memset(plog, 0, sizeof(plog));
                if (getlog)
                    getlog(prog, (int32_t)sizeof(plog) - 1, &plen, plog);
                fprintf(stderr, "GUA-GL program-link FAIL id=%u\n", prog);
                fprintf(stderr, "GUA-GL program-log: %s\n", plog);
            } else if (gl_diag_enabled() && !gl_diag_link) {
                fprintf(stderr, "GUA-GL first glLinkProgram success id=%u\n", prog);
                gl_diag_link = 1;
            }
        }
        if (rc == 0 && gl_diag_enabled() && h.len >= 4) {
            uint32_t object;

            memcpy(&object, in, 4);
            if (h.op == OP_glUseProgram && object != 0 && !gl_diag_use) {
                fprintf(stderr, "GUA-GL first glUseProgram id=%u\n", object);
                gl_diag_use = 1;
            } else if ((h.op == OP_glDrawArrays || h.op == OP_glDrawElements) &&
                       !gl_diag_draw) {
                fprintf(stderr, "GUA-GL first glDraw %s\n",
                        gl_diag_op_name(h.op));
                gl_diag_draw = 1;
            }
        }
    }
    (void)rc;
    if (!tspgl_needs_reply(h.op)) {
        ret = 0;
        goto out_free;
    }
    if (out_n > sizeof(outbuf_s)) {
        fprintf(stderr, "tspgl-srv: out %u truncated\n", out_n);
        out_n = (uint32_t)sizeof(outbuf_s);
    }
    r.len = out_n;
    if (full_write(fd, &r, sizeof(r)) != 0)
        goto out_free;
    if (r.len && full_write(fd, out, r.len) != 0)
        goto out_free;
    ret = 0;
out_free:
    free(in_h);
    return ret;
}

static int setup_listen(void)
{
    struct sockaddr_un addr;
    int fd;
    unlink(TSPGL_SOCK);
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        fprintf(stderr, "tspgl-srv: socket errno=%d\n", errno);
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TSPGL_SOCK, sizeof(addr.sun_path) - 1);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "tspgl-srv: bind %s errno=%d\n", TSPGL_SOCK, errno);
        close(fd);
        return -1;
    }
    if (listen(fd, 8) != 0) {
        fprintf(stderr, "tspgl-srv: listen errno=%d\n", errno);
        close(fd);
        return -1;
    }
    listen_fd = fd;
    fprintf(stderr, "tspgl-srv: listen %s\n", TSPGL_SOCK);
    return 0;
}

int main(void)
{
    const char *ew, *eh;
    uint32_t *hdr;
    FILE *ready;
    struct pollfd pf[9];
    int ncli = 0;
    int cli[8];
    unsigned wait_pad = 0;
    int i;

    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    fprintf(stderr, "tspgl-srv: start (32->64 GLES bridge)\n");

    ew = getenv("TSPGL_WIDTH");
    eh = getenv("TSPGL_HEIGHT");
    if (ew)
        game_w = atoi(ew);
    if (eh)
        game_h = atoi(eh);
    if (game_w < 1)
        game_w = 1024;
    if (game_h < 1)
        game_h = 768;
    {
        const char *rd = getenv("TSPGL_DEPTH_ONLY_READ_NONE");
        depth_only_read_none = rd && atoi(rd) != 0;
    }
    {
        const char *zv = getenv("TSPGL_ZERO_VIEWPORT");
        zero_viewport = zv && atoi(zv) != 0;
    }
    {
        const char *rz = getenv("TSPGL_RB_ZERO_SIZE");
        rb_zero_size = rz && atoi(rz) != 0;
    }
    {
        const char *ft = getenv("TSPGL_FBO_TEXTURE_FALLBACK");
        fbo_texture_fallback = ft && atoi(ft) != 0;
    }
    {
        const char *ud = getenv("TSPGL_UNIFY_DEPTH_STENCIL");
        unify_depth_stencil = ud && atoi(ud) != 0;
    }
    {
        const char *rf = getenv("TSPGL_RB_FORMAT_FIX");
        rb_format_fix = rf && atoi(rf) != 0;
    }
    {
        const char *pp = getenv("GUACAMELEE_PIXEL_PROBE");
        const char *pd = getenv("GUACAMELEE_PIXEL_DUMP");
        pixel_probe_enabled = pp && atoi(pp) != 0;
        pixel_dump_enabled = pd && atoi(pd) != 0;
    }
    {
        const char *or = getenv("GUACAMELEE_OP_RING");
        if (or && atoi(or) != 0)
            fprintf(stderr, "GUA-RING enabled\\n");
    }
    {
        const char *sf = getenv("TSPGL_SWAP_FINISH");
        swap_finish = sf && atoi(sf) != 0;
    }
    {
        const char *si = getenv("TSPGL_SWAP_INTERVAL");
        swap_interval = si ? atoi(si) : 0;
        if (swap_interval < 0)
            swap_interval = 0;
    }
    {
        const char *mode = getenv("TSPGL_PRESENT");
#ifdef TSPGL_DEFAULT_LETTERBOX
        present_letterbox = 1;
#endif
        if (mode != NULL) {
            if (strcmp(mode, "letterbox") == 0 || strcmp(mode, "fit") == 0)
                present_letterbox = 1;
            else if (strcmp(mode, "stretch") == 0 || strcmp(mode, "fill") == 0)
                present_letterbox = 0;
        }
        fprintf(stderr, "tspgl-srv: present %s %dx%d -> %dx%d\n",
                present_letterbox ? "letterbox" : "stretch", game_w, game_h,
                win_w, win_h);
    }

    frame_map = open_frame();
    if (!frame_map)
        return 1;
    hdr = (uint32_t *)frame_map;
    hdr[NFSMW_HDR_MAGIC] = NFSMW_FRAME_MAGIC;

    if (load_sdl() != 0)
        return 1;
    if (sdl.set_hint) {
        sdl.set_hint("SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS", "0");
        sdl.set_hint("SDL_HINT_RENDER_VSYNC", "0");
        sdl.set_hint("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1");
        sdl.set_hint("SDL_JOYSTICK_HIDAPI", "0");
    }
    if (sdl.init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) !=
        0) {
        fprintf(stderr, "tspgl-srv: SDL_Init+pad failed (%s), video only\n",
                sdl.get_error());
        if (sdl.init(SDL_INIT_VIDEO) != 0) {
            fprintf(stderr, "tspgl-srv: SDL_Init: %s\n", sdl.get_error());
            return 1;
        }
    }
    sdl.gl_set_attr(SDL_GL_RED_SIZE, 8);
    sdl.gl_set_attr(SDL_GL_GREEN_SIZE, 8);
    sdl.gl_set_attr(SDL_GL_BLUE_SIZE, 8);
    sdl.gl_set_attr(SDL_GL_ALPHA_SIZE, 8);
    sdl.gl_set_attr(SDL_GL_DOUBLEBUFFER, 1);
    sdl.gl_set_attr(SDL_GL_DEPTH_SIZE, 24);
    sdl.gl_set_attr(SDL_GL_STENCIL_SIZE, 8);
    sdl.gl_set_attr(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    sdl.gl_set_attr(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    sdl.gl_set_attr(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    window = sdl.create_window("tsp-glbridge", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, win_w, win_h,
                               SDL_WINDOW_FULLSCREEN | SDL_WINDOW_SHOWN |
                                   SDL_WINDOW_OPENGL);
    if (!window) {
        fprintf(stderr, "tspgl-srv: CreateWindow: %s\n", sdl.get_error());
        return 1;
    }
    glctx = sdl.gl_create_context(window);
    if (!glctx) {
        fprintf(stderr, "tspgl-srv: GLES3 context failed (%s), try ES2\n",
                sdl.get_error());
        sdl.gl_set_attr(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        sdl.gl_set_attr(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        glctx = sdl.gl_create_context(window);
    }
    if (!glctx) {
        fprintf(stderr, "tspgl-srv: CreateContext: %s\n", sdl.get_error());
        return 1;
    }
    if (sdl.gl_make_current(window, glctx) != 0) {
        fprintf(stderr, "tspgl-srv: MakeCurrent: %s\n", sdl.get_error());
        return 1;
    }
    if (sdl.gl_set_swap)
        sdl.gl_set_swap(swap_interval);
    sdl.show_cursor(0);
    fprintf(stderr, "tspgl-srv: window %dx%d GL context ok driver=%s swap_interval=%d\n",
            win_w, win_h,
            sdl.get_current_video_driver ? sdl.get_current_video_driver() : "unknown",
            swap_interval);

    memset(&G, 0, sizeof(G));
    tspgl_load_simple(gl_get);
    tspgl_load_specials(gl_get);
    if (!G.glBlitFramebuffer)
        G.glBlitFramebuffer = (void *)gl_get("glBlitFramebuffer");
    if (!G.glGenVertexArrays)
        G.glGenVertexArrays = (void *)gl_get("glGenVertexArraysOES");
    if (!G.glDeleteVertexArrays)
        G.glDeleteVertexArrays = (void *)gl_get("glDeleteVertexArraysOES");
    if (!G.glBindVertexArray)
        G.glBindVertexArray = gl_get("glBindVertexArrayOES");
    if (!G.glIsVertexArray)
        G.glIsVertexArray = gl_get("glIsVertexArrayOES");
    fbo_format_matrix();
    khr_debug_setup();
    {
        const uint8_t *(*gs)(uint32_t) = (void *)G.glGetString;
        const char *ven = "", *ren = "", *ver = "", *ext = "";
        if (gs) {
            if (gs(0x1F00))
                ven = (const char *)gs(0x1F00);
            if (gs(0x1F01))
                ren = (const char *)gs(0x1F01);
            if (gs(0x1F02))
                ver = (const char *)gs(0x1F02);
            if (gs(0x1F03))
                ext = (const char *)gs(0x1F03);
        }
        fprintf(stderr, "tspgl-srv: GL vendor=%s renderer=%s version=%s\n", ven,
                ren, ver);
        fprintf(stderr, "tspgl-srv: GLext %s\n", ext);
        have_fb_fetch = 0;
        have_fb_fetch_nc = 0;
        if (ext) {
            const char *e = ext;
            while (*e) {
                const char *n = e;
                size_t nl;
                while (*e && *e != ' ')
                    e++;
                nl = (size_t)(e - n);
                if (nl == sizeof("GL_EXT_shader_framebuffer_fetch") - 1 &&
                    strncmp(n, "GL_EXT_shader_framebuffer_fetch", nl) == 0)
                    have_fb_fetch = 1;
                if (nl == sizeof("GL_EXT_shader_framebuffer_fetch_non_coherent") - 1 &&
                    strncmp(n, "GL_EXT_shader_framebuffer_fetch_non_coherent", nl) == 0)
                    have_fb_fetch_nc = 1;
                while (*e == ' ')
                    e++;
            }
        }
        fprintf(stderr, "tspgl-srv: fb_fetch=%d noncoherent=%d\n", have_fb_fetch,
                have_fb_fetch_nc);
    }

    if (splash_setup() == 0) {
        splash_present();
        splash_present();
    }

    if (create_game_fbo() != 0) {
        fprintf(stderr, "tspgl-srv: FBO failed, default framebuffer\n");
        game_fbo = 0;
    }
    real_bind_fb = G.glBindFramebuffer;
    if (game_fbo)
        G.glBindFramebuffer = wrap_bind_fb;
    real_get_integerv = (void (*)(uint32_t, int32_t *))(uintptr_t)G.glGetIntegerv;
    G.glGetIntegerv = (void *)(uintptr_t)wrap_get_integerv;
    real_check_fb = G.glCheckFramebufferStatus;
    G.glCheckFramebufferStatus = wrap_check_fb;
    real_draw_buffers = (void (*)(int32_t, const uint32_t *))(uintptr_t)gl_get(
        "glDrawBuffers");
    real_read_buffer = (void (*)(uint32_t))(uintptr_t)gl_get("glReadBuffer");
    real_tex_image = (void *)G.glTexImage2D;
    if (real_tex_image)
        G.glTexImage2D = (void *)(uintptr_t)wrap_tex_image2d;
    real_tex_parami = G.glTexParameteri;
    if (real_tex_parami)
        G.glTexParameteri = wrap_tex_parami;
    real_tex_paramf = G.glTexParameterf;
    if (real_tex_paramf)
        G.glTexParameterf = wrap_tex_paramf;
    real_compressed = (void *)G.glCompressedTexImage2D;
    if (real_compressed)
        G.glCompressedTexImage2D = (void *)(uintptr_t)wrap_compressed;
    real_gen_mipmap = G.glGenerateMipmap;
    if (real_gen_mipmap)
        G.glGenerateMipmap = wrap_gen_mipmap;
    real_delete_tex = G.glDeleteTextures;
    if (real_delete_tex)
        G.glDeleteTextures = wrap_delete_tex;
    if (real_bind_fb && game_fbo)
        real_bind_fb(GL_FRAMEBUFFER, game_fbo);
    if (G.glViewport)
        G.glViewport(0, 0, game_w, game_h);
    if (splash_live) {
        splash_present();
        splash_present();
    }

    if (setup_listen() != 0)
        return 1;

    pad = open_pad();
    hdr[NFSMW_HDR_READY] = 1;
    ready = fopen("/tmp/nfsmw.present.ready", "w");
    if (ready) {
        fputs("ok\n", ready);
        fclose(ready);
    }
    fprintf(stderr, "tspgl-srv: ready\n");

    memset(cli, -1, sizeof(cli));
    {
        uint32_t last_sdl = 0;
        for (;;) {
            sdl_event ev;
            int nfds;
            int ret;
            uint32_t t = now_ms();

            if (!ncli || input_soon || (t - last_sdl) >= 8u) {
                while (sdl.poll_event(&ev)) {
                    if (ev.type == 0x100)
                        goto done;
                }
                if (!pad) {
                    ++wait_pad;
                    if ((wait_pad % 30u) == 0u)
                        pad = open_pad();
                }
                publish_input(hdr);
                last_sdl = t;
                input_soon = 0;
            }

            pf[0].fd = listen_fd;
            pf[0].events = POLLIN;
            pf[0].revents = 0;
            nfds = 1;
            for (i = 0; i < ncli; ++i) {
                pf[nfds].fd = cli[i];
                pf[nfds].events = POLLIN;
                pf[nfds].revents = 0;
                ++nfds;
            }
            ret = poll(pf, (nfds_t)nfds, ncli ? 8 : 50);
            if (splash_live && ncli == 0)
                splash_present();
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "tspgl-srv: poll errno=%d\n", errno);
            break;
        }
        {
            int nwas = ncli;
            if (pf[0].revents & POLLIN) {
                int cfd = accept(listen_fd, NULL, NULL);
                if (cfd >= 0) {
                    if (ncli < 8) {
                        int buf = 1024 * 1024;
                        setsockopt(cfd, SOL_SOCKET, SO_SNDBUF, &buf, sizeof(buf));
                        setsockopt(cfd, SOL_SOCKET, SO_RCVBUF, &buf, sizeof(buf));
                        cli[ncli++] = cfd;
                        fprintf(stderr, "tspgl-srv: client fd=%d n=%d\n", cfd,
                                ncli);
                    } else {
                        close(cfd);
                    }
                }
            }
            for (i = 0; i < nwas; ++i) {
                if (!(pf[i + 1].revents & (POLLIN | POLLHUP | POLLERR)))
                    continue;
                if (handle_client(cli[i]) != 0) {
                    fprintf(stderr, "tspgl-srv: client fd=%d drop\n",
                            cli[i]);
                    close(cli[i]);
                    cli[i] = -1;
                }
            }
            {
                int spin;
                for (spin = 0; spin < 64; ++spin) {
                    int more_work = 0;
                    for (i = 0; i < nwas; ++i) {
                        struct pollfd more;
                        if (cli[i] < 0)
                            continue;
                        more.fd = cli[i];
                        more.events = POLLIN;
                        more.revents = 0;
                        if (poll(&more, 1, 0) <= 0 ||
                            !(more.revents & POLLIN))
                            continue;
                        if (handle_client(cli[i]) != 0) {
                            fprintf(stderr, "tspgl-srv: client fd=%d drop\n",
                                    cli[i]);
                            close(cli[i]);
                            cli[i] = -1;
                        } else
                            more_work = 1;
                    }
                    if (!more_work)
                        break;
                }
            }
            {
                int w = 0;
                for (i = 0; i < ncli; ++i) {
                    if (cli[i] >= 0)
                        cli[w++] = cli[i];
                }
                ncli = w;
            }
        }
        }
    }

done:
    unlink("/tmp/nfsmw.present.ready");
    unlink(TSPGL_SOCK);
    if (glctx)
        sdl.gl_delete_context(glctx);
    if (window)
        sdl.destroy_window(window);
    sdl.quit();
    fprintf(stderr, "tspgl-srv: exit\n");
    return 0;
}
