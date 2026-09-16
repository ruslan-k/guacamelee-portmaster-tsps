#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DRM_IOCTL_BASE 'd'
#define DRM_IO(nr) _IO(DRM_IOCTL_BASE, nr)
#define DRM_IOCTL_SET_MASTER DRM_IO(0x1e)
#define DRM_IOCTL_DROP_MASTER DRM_IO(0x1f)

int main(void) {
    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        printf("DRM open failed errno=%d(%s)\n", errno, strerror(errno));
        return 2;
    }
    errno = 0;
    int set_rc = ioctl(fd, DRM_IOCTL_SET_MASTER, 0);
    int set_errno = errno;
    printf("DRM fd=%d setmaster_rc=%d errno=%d(%s)\n", fd, set_rc, set_errno, strerror(set_errno));
    if (set_rc == 0) ioctl(fd, DRM_IOCTL_DROP_MASTER, 0);
    close(fd);
    return set_rc == 0 ? 0 : 1;
}
