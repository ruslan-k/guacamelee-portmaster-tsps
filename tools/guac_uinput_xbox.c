#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t stop;
static void on_signal(int sig) { (void)sig; stop = 1; }
static int emit(int fd, uint16_t type, uint16_t code, int32_t value) {
    struct input_event e; memset(&e, 0, sizeof(e));
    e.type=type; e.code=code; e.value=value;
    return write(fd, &e, sizeof(e)) == (ssize_t)sizeof(e) ? 0 : -1;
}
static int key_code(uint16_t code) {
    switch (code) {
    case BTN_SOUTH: return BTN_A; case BTN_EAST: return BTN_B;
    case BTN_NORTH: return BTN_X; case BTN_WEST: return BTN_Y;
    case BTN_TL: return BTN_TL; case BTN_TR: return BTN_TR;
    case BTN_TL2: return BTN_TL2; case BTN_TR2: return BTN_TR2;
    case BTN_SELECT: return BTN_SELECT; case BTN_START: return BTN_START;
    case BTN_MODE: return BTN_MODE; case BTN_THUMBL: return BTN_THUMBL;
    case BTN_THUMBR: return BTN_THUMBR; default: return -1;
    }
}
static const char *find_source(char *out, size_t cap) {
    DIR *d = opendir("/sys/class/input"); struct dirent *e;
    const char *override = getenv("GUACAMELEE_INPUT_EVENT");
    if (override && *override) { snprintf(out, cap, "%s", override); return out; }
    if (!d) return NULL;
    while ((e = readdir(d))) {
        char p[128], name[128]; int fd, n;
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        snprintf(p, sizeof(p), "/sys/class/input/%s/device/name", e->d_name);
        fd = open(p, O_RDONLY); if (fd < 0) continue;
        n = (int)read(fd, name, sizeof(name)-1); close(fd);
        if (n <= 0) continue; name[n] = 0;
        if (strstr(name, "TRIMUI Player1")) {
            snprintf(out, cap, "/dev/input/%s", e->d_name); closedir(d);
            fprintf(stderr, "GUA-XBOX source=%s name=\"%s\"", out, name);
            return out;
        }
    }
    closedir(d); return NULL;
}
int main(int argc, char **argv) {
    char source[128]; const char *src = argc > 1 ? argv[1] : find_source(source, sizeof(source));
    int in, u, keys[] = {BTN_A,BTN_B,BTN_X,BTN_Y,BTN_TL,BTN_TR,BTN_TL2,BTN_TR2,
                         BTN_SELECT,BTN_START,BTN_MODE,BTN_THUMBL,BTN_THUMBR};
    struct uinput_user_dev dev; struct input_event e;
    signal(SIGTERM,on_signal); signal(SIGINT,on_signal);
    if (!src) { fprintf(stderr, "GUA-XBOX source not found\n"); return 2; }
    in=open(src,O_RDONLY|O_NONBLOCK); if(in<0){perror(src);return 2;}
    u=open("/dev/uinput",O_WRONLY|O_NONBLOCK); if(u<0){perror("uinput");return 2;}
    ioctl(u,UI_SET_EVBIT,EV_KEY); ioctl(u,UI_SET_EVBIT,EV_ABS); ioctl(u,UI_SET_EVBIT,EV_SYN);
    for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++) ioctl(u,UI_SET_KEYBIT,keys[i]);
    int abs[] = {ABS_X,ABS_Y,ABS_RX,ABS_RY,ABS_Z,ABS_RZ,ABS_HAT0X,ABS_HAT0Y};
    for(unsigned i=0;i<sizeof(abs)/sizeof(abs[0]);i++) ioctl(u,UI_SET_ABSBIT,abs[i]);
    memset(&dev,0,sizeof(dev)); snprintf(dev.name,sizeof(dev.name),"Microsoft X-Box 360 pad");
    dev.id.bustype=BUS_USB; dev.id.vendor=0x045e; dev.id.product=0x028e; dev.id.version=0x0114;
    for (int c=ABS_X;c<=ABS_RZ;c++) { dev.absmin[c]=-32768; dev.absmax[c]=32767; }
    dev.absmin[ABS_Z]=0; dev.absmax[ABS_Z]=255; dev.absmin[ABS_RZ]=0; dev.absmax[ABS_RZ]=255;
    dev.absmin[ABS_HAT0X]=-1; dev.absmax[ABS_HAT0X]=1; dev.absmin[ABS_HAT0Y]=-1; dev.absmax[ABS_HAT0Y]=1;
    if(write(u,&dev,sizeof(dev))!=(ssize_t)sizeof(dev)||ioctl(u,UI_DEV_CREATE)<0){perror("UI_DEV_CREATE");return 2;}
    fprintf(stderr,"GUA-XBOX ready source=%s name=\"Microsoft X-Box 360 pad\"\n",src);
    while(!stop) {
        ssize_t n=read(in,&e,sizeof(e));
        if(n<0 && (errno==EAGAIN||errno==EINTR)){usleep(1000);continue;}
        if(n!=(ssize_t)sizeof(e)) { if(n<0&&errno==EAGAIN) continue; usleep(1000); continue; }
        if(e.type==EV_KEY){int c=key_code(e.code); if(c>=0){emit(u,EV_KEY,c,e.value);emit(u,EV_SYN,SYN_REPORT,0);}}
        else if(e.type==EV_ABS){int c=-1; switch(e.code){case ABS_X:c=ABS_X;break;case ABS_Y:c=ABS_Y;break;case ABS_RX:c=ABS_RX;break;case ABS_RY:c=ABS_RY;break;case ABS_Z:c=ABS_Z;break;case ABS_RZ:c=ABS_RZ;break;case ABS_HAT0X:c=ABS_HAT0X;break;case ABS_HAT0Y:c=ABS_HAT0Y;break;} if(c>=0){emit(u,EV_ABS,c,e.value);emit(u,EV_SYN,SYN_REPORT,0);}}
    }
    ioctl(u,UI_DEV_DESTROY); close(u); close(in); return 0;
}
