#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
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
    case BTN_SOUTH: return BTN_A;
    case BTN_EAST: return BTN_B;
    case BTN_NORTH: return BTN_X;
    case BTN_WEST: return BTN_Y;
    case BTN_TL: return BTN_TL;
    case BTN_TR: return BTN_TR;
    case BTN_TL2: return BTN_TL2;
    case BTN_TR2: return BTN_TR2;
    case BTN_SELECT: return BTN_SELECT;
    case BTN_START: return BTN_START;
    case BTN_MODE: return BTN_MODE;
    default: return -1;
    }
}
int main(void) {
    signal(SIGTERM,on_signal); signal(SIGINT,on_signal);
    int in=open("/dev/input/event4",O_RDONLY); if(in<0){perror("event4");return 2;}
    int u=open("/dev/uinput",O_WRONLY|O_NONBLOCK); if(u<0){perror("uinput");return 2;}
    ioctl(u,UI_SET_EVBIT,EV_KEY); ioctl(u,UI_SET_EVBIT,EV_ABS); ioctl(u,UI_SET_EVBIT,EV_SYN);
    int keys[]={BTN_A,BTN_B,BTN_X,BTN_Y,BTN_TL,BTN_TR,BTN_TL2,BTN_TR2,BTN_SELECT,BTN_START,BTN_MODE};
    for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++) ioctl(u,UI_SET_KEYBIT,keys[i]);
    ioctl(u,UI_SET_ABSBIT,ABS_X); ioctl(u,UI_SET_ABSBIT,ABS_Y); ioctl(u,UI_SET_ABSBIT,ABS_HAT0X); ioctl(u,UI_SET_ABSBIT,ABS_HAT0Y);
    struct uinput_user_dev d; memset(&d,0,sizeof(d));
    snprintf(d.name,sizeof(d.name),"Microsoft X-Box 360 pad"); d.id.bustype=BUS_USB; d.id.vendor=0x045e; d.id.product=0x028e; d.id.version=0x0114;
    d.absmin[ABS_X]=-32768; d.absmax[ABS_X]=32767; d.absmin[ABS_Y]=-32768; d.absmax[ABS_Y]=32767;
    d.absmin[ABS_HAT0X]=-1; d.absmax[ABS_HAT0X]=1; d.absmin[ABS_HAT0Y]=-1; d.absmax[ABS_HAT0Y]=1;
    if(write(u,&d,sizeof(d))!=(ssize_t)sizeof(d)||ioctl(u,UI_DEV_CREATE)<0){perror("UI_DEV_CREATE");return 2;}
    fprintf(stderr,"GUA-XBOX ready event4 -> Microsoft X-Box 360 pad\n");
    struct input_event e;
    while(!stop) {
        ssize_t n=read(in,&e,sizeof(e));
        if(n<0 && (errno==EAGAIN||errno==EINTR)){usleep(1000);continue;}
        if(n!=(ssize_t)sizeof(e)) { if(n<0&&errno==EAGAIN) continue; usleep(1000); continue; }
        if(e.type==EV_KEY){int c=key_code(e.code); if(c>=0){emit(u,EV_KEY,c,e.value);emit(u,EV_SYN,SYN_REPORT,0);}}
        else if(e.type==EV_ABS){int c=-1; if(e.code==ABS_X)c=ABS_X; else if(e.code==ABS_Y)c=ABS_Y; else if(e.code==ABS_HAT0X)c=ABS_HAT0X; else if(e.code==ABS_HAT0Y)c=ABS_HAT0Y; if(c>=0){emit(u,EV_ABS,c,e.value);emit(u,EV_SYN,SYN_REPORT,0);}}
    }
    ioctl(u,UI_DEV_DESTROY); close(u); close(in); return 0;
}
