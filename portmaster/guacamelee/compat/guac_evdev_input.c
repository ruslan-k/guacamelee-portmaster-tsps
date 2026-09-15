#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    uint32_t type, timestamp, window;
    uint8_t state, repeat, pad[2];
    int32_t scancode, sym;
    uint16_t mod, unused;
    uint8_t reserved[24];
} sdl_event_t;

typedef struct { int32_t sec, usec; uint16_t type, code; int32_t value; } ev_t;
static int evfd = -1;
static int (*real_poll)(sdl_event_t *);
static sdl_event_t pending[32];
static unsigned pn, pi;

static void init_input(void) {
    if (!real_poll) real_poll = dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (evfd >= 0) return;
    evfd = open("/dev/input/event6", O_RDONLY | O_NONBLOCK);
    if (evfd < 0) evfd = open("/dev/input/event4", O_RDONLY | O_NONBLOCK);
}
static void push_key(int down, int scan, int sym) {
    if (pn >= 32) return;
    fprintf(stderr, "GUA-EVDEV key=%d scan=%d sym=%d\\n", down, scan, sym);
    sdl_event_t *e = &pending[pn++]; memset(e, 0, sizeof(*e));
    e->type = down ? 0x300 : 0x301; e->state = down ? 1 : 0;
    e->scancode = scan; e->sym = sym;
}
static int axis_state[2];

static void read_input(void) {
    ev_t e;
    if (evfd < 0) return;
    while (read(evfd, &e, sizeof(e)) == (ssize_t)sizeof(e)) {
        if (e.type == 1) {
            int scan=0,sym=0;
            switch (e.code) {
            case 304: scan=4; sym='a'; break;
            case 305: scan=22; sym='s'; break;
            case 307: scan=7; sym='d'; break;
            case 308: scan=26; sym='w'; break;
            case 310: scan=225; sym=0; break;
            case 311: scan=8; sym='e'; break;
            case 312: scan=20; sym='q'; break;
            case 313: scan=21; sym='r'; break;
            case 316: scan=43; sym=9; break;
            case 314: scan=41; sym=27; break;
            case 315: scan=40; sym=13; break;
            default: break;
            }
            if (scan) push_key(e.value != 0, scan, sym);
        } else if (e.type == 3 && (e.code == 16 || e.code == 17)) {
            int ai = e.code == 16 ? 0 : 1;
            int next = e.value < 0 ? -1 : (e.value > 0 ? 1 : 0);
            int scan_neg = e.code == 16 ? 80 : 82;
            int scan_pos = e.code == 16 ? 79 : 81;
            int sym_neg = e.code == 16 ? 1073741904 : 1073741906;
            int sym_pos = e.code == 16 ? 1073741903 : 1073741905;
            if (next != axis_state[ai]) {
                if (axis_state[ai] < 0) push_key(0, scan_neg, sym_neg);
                if (axis_state[ai] > 0) push_key(0, scan_pos, sym_pos);
                if (next < 0) push_key(1, scan_neg, sym_neg);
                if (next > 0) push_key(1, scan_pos, sym_pos);
                axis_state[ai] = next;
            }
        }
    }
}
static int next_event(sdl_event_t *out) {
    if (real_poll && real_poll(out)) return 1;
    read_input();
    if (pi < pn) { *out = pending[pi++]; if (pi == pn) pi = pn = 0; return 1; }
    return 0;
}
int SDL_PollEvent(sdl_event_t *out) { init_input(); return next_event(out); }
void SDL_PumpEvents(void) { static void (*real_fn)(void); if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_PumpEvents"); if (real_fn) real_fn(); init_input(); read_input(); }
int SDL_PeepEvents(sdl_event_t *events, int num, int action, uint32_t min_type, uint32_t max_type) {
    static int (*real_fn)(sdl_event_t*,int,int,uint32_t,uint32_t);
    if (!real_fn) real_fn=dlsym(RTLD_NEXT,"SDL_PeepEvents");
    int count=real_fn ? real_fn(events,num,action,min_type,max_type) : 0;
    init_input(); read_input();
    if (action == 2 && events && num > count) {
        while (count < num && pi < pn) { sdl_event_t e=pending[pi++]; if (e.type>=min_type && e.type<=max_type) events[count++]=e; }
        if (pi == pn) pi=pn=0;
    }
    return count;
}
int SDL_WaitEventTimeout(sdl_event_t *out, int timeout) {
    init_input();
    int waited=0;
    do { if (next_event(out)) return 1; usleep(1000); waited++; } while (timeout < 0 || waited < timeout*1000);
    return 0;
}
int SDL_WaitEvent(sdl_event_t *out) { return SDL_WaitEventTimeout(out, -1); }
static void *evdev_reader(void *unused) { (void)unused; init_input(); while (1) { read_input(); usleep(1000); } return NULL; }
static int (*real_gc_button)(void *, int);
static int (*real_gc_axis)(void *, int);
static unsigned poll_logs;
int SDL_GameControllerGetButton(void *c, int b) {
    if (!real_gc_button) real_gc_button = dlsym(RTLD_NEXT, "SDL_GameControllerGetButton");
    int v = real_gc_button ? real_gc_button(c, b) : 0;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && poll_logs++ < 32)
        fprintf(stderr, "GUA-POLL button=%d value=%d controller=%p\\n", b, v, c);
    return v;
}
int SDL_GameControllerGetAxis(void *c, int a) {
    if (!real_gc_axis) real_gc_axis = dlsym(RTLD_NEXT, "SDL_GameControllerGetAxis");
    int v = real_gc_axis ? real_gc_axis(c, a) : 0;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && poll_logs++ < 32)
        fprintf(stderr, "GUA-POLL axis=%d value=%d controller=%p\\n", a, v, c);
    return v;
}

static int (*real_num_joy)(void);
static void *(*real_gc_open)(int);
static void *(*real_joy_open)(int);
static const char *(*real_gc_name)(int);
static const char *(*real_joy_name)(int);
static unsigned enum_logs;
int SDL_NumJoysticks(void) {
    if (!real_num_joy) real_num_joy = dlsym(RTLD_NEXT, "SDL_NumJoysticks");
    int v = real_num_joy ? real_num_joy() : 0;
    if (getenv("GUACAMELEE_FORCE_ONE_JOYSTICK") && v == 0) v = 1;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && enum_logs++ < 16) fprintf(stderr, "GUA-ENUM SDL_NumJoysticks=%d\\n", v);
    return v;
}
void *SDL_GameControllerOpen(int i) {
    if (!real_gc_open) real_gc_open = dlsym(RTLD_NEXT, "SDL_GameControllerOpen");
    void *v = real_gc_open ? real_gc_open(i) : NULL;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && enum_logs++ < 16) fprintf(stderr, "GUA-ENUM SDL_GameControllerOpen index=%d result=%p\\n", i, v);
    return v;
}
void *SDL_JoystickOpen(int i) {
    if (!real_joy_open) real_joy_open = dlsym(RTLD_NEXT, "SDL_JoystickOpen");
    void *v = real_joy_open ? real_joy_open(i) : NULL;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && enum_logs++ < 16) fprintf(stderr, "GUA-ENUM SDL_JoystickOpen index=%d result=%p\\n", i, v);
    return v;
}
const char *SDL_GameControllerNameForIndex(int i) {
    if (!real_gc_name) real_gc_name = dlsym(RTLD_NEXT, "SDL_GameControllerNameForIndex");
    const char *v = real_gc_name ? real_gc_name(i) : NULL;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && enum_logs++ < 16) fprintf(stderr, "GUA-ENUM SDL_GameControllerNameForIndex index=%d name=%s\\n", i, v ? v : "(null)");
    return v;
}
const char *SDL_JoystickNameForIndex(int i) {
    if (!real_joy_name) real_joy_name = dlsym(RTLD_NEXT, "SDL_JoystickNameForIndex");
    const char *v = real_joy_name ? real_joy_name(i) : NULL;
    if (getenv("GUACAMELEE_INPUT_POLL_DIAG") && enum_logs++ < 16) fprintf(stderr, "GUA-ENUM SDL_JoystickNameForIndex index=%d name=%s\\n", i, v ? v : "(null)");
    return v;
}

__attribute__((constructor)) static void start_evdev_reader(void) { pthread_t t; if (pthread_create(&t, NULL, evdev_reader, NULL) == 0) pthread_detach(t); }
