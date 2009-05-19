#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main (int argc, char *argv[])
{
        int rc;
        int i;
        int usbfd;
        float buffer[2051];

        usbfd = open("/dev/usb2epp0", O_RDWR);
        if (usbfd < 0) {
                printf("open fscked up\n");
                return 1;
        }

        rc = ioctl(usbfd, 0, 20);
        if (rc != 0) {
                printf("ioctl fscked up\n");
                return 1;
        }

        rc = read(usbfd, buffer, 2051*sizeof(float));
        if (rc < 0) {
                printf("read fscked up\n");
                return 1;
        }

        for (i = 0;i<2051;i++)
                printf("buffer[%d]: %f\n", i, buffer[i]);

        return 0;
}
