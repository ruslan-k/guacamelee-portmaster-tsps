/* AArch64 diagnostic dlopen stub; not an X11 implementation. */
#include <stdio.h>

static void trace(const char *name) {
    fprintf(stderr, "BOX32-X11-STUB %s\n", name);
    fflush(stderr);
}

void *XOpenDisplay(const char *name) { trace("XOpenDisplay"); (void)name; return 0; }
int XCloseDisplay(void *dpy) { trace("XCloseDisplay"); (void)dpy; return 0; }
int XInitThreads(void) { trace("XInitThreads"); return 1; }
void *XSetErrorHandler(void *handler) { trace("XSetErrorHandler"); return handler; }
void *XSetIOErrorHandler(void *handler) { trace("XSetIOErrorHandler"); return handler; }
int XDefaultScreen(void *dpy) { trace("XDefaultScreen"); (void)dpy; return 0; }
const char *XDisplayString(void *dpy) { trace("XDisplayString"); (void)dpy; return ""; }
