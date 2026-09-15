#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint32_t type, timestamp, window;
    uint8_t state, repeat, pad[2];
    int32_t scancode, sym;
    uint16_t mod, unused;
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
int SDL_PollEvent(sdl_event_t *out) {
    init_input();
    if (real_poll && real_poll(out)) return 1;
    read_input();
    if (pi < pn) { *out = pending[pi++]; if (pi == pn) pi = pn = 0; return 1; }
    return 0;
}
