#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gpif.h>
#include <unistd.h>

#define SHOWCASE

int main (int argc, char *argv[])
{
        int rc;
        int i;
        int usbfd;
        size_t len;
        int buffer[2051];
        char buf[256];
        gpif_session_t session;

        char *const myargv[] = {
                "gnuplot",
                "-noraise",
                "-persist",
                NULL
        };

        if (argc <= 1) {
                printf("Incorrect number of arguments\n");
                return 1;
        }

        usbfd = open(argv[1], O_RDONLY);
        if (usbfd < 0) {
                perror("open");
                return 1;
        }

        rc = ioctl(usbfd, 0, 20);
        if (rc != 0) {
                perror("ioctl");
                close(usbfd);
                return 1;
        }

        /* This is for the thesis presentation only, trigger the data generator once to
         * get a readout */
#ifdef SHOWCASE
        {
                int iowfd;
                unsigned char buf[] = {
                        0xff,
                        0x00,
                        0x00,
                        0x00,
                };

                if((iowfd = open( "/dev/usb/iowarrior0", O_RDWR)) < 0 ) {
                        perror( "iowarrior io-pins open failed" );
                        return 1;
                }

                /* Pull high */
                rc = write(iowfd, buf, 4);
                if (rc<4) {
                        perror("write");
                        return 1;
                }

                /* Pull low */
                buf[0] = 0x00;
                buf[3] = 0xff;
                rc = write(iowfd, buf, 4);
                if (rc<4) {
                        perror("write");
                        return 1;
                }
        }    
#endif


        rc = read(usbfd, buffer, 2051*sizeof(int));
        if (rc < 0) {
                perror("read");
                close(usbfd);
                return 1;
        }

        close(usbfd);

        /* Establish a gnuplot session and plot the results */
        rc = gpif_init(&session, myargv);
        if (rc != EGPIFOK) {
                printf("Failed to create Gnuplot session\n");
                return 1;
        }

        strncpy(buf, "set grid xtics ytics layerdefault back linetype 0 linecolor rgb '#808080'\n", sizeof(buf));
        len = strlen(buf);
        rc = gpif_write(&session, (const char*)buf, &len);
        if(rc != EGPIFOK) {
                printf("Failed to setup plot environment\n");
                gpif_close(&session);
                return 1;
        }

        len = snprintf(buf, sizeof(buf), "plot '-' using 1:($2) '%%lf %%lf' smooth csplines notitle axes x1y1 with lines linecolor rgb '#FF0000' linewidth 2\n");
        rc = gpif_write(&session, (const char*)buf, &len);
        if(rc != EGPIFOK) {
                printf("Failed to create new plot\n");
                gpif_close(&session);
                return 1;
        }

        for (i=0;i<2051;i++) {
                if (i < 2050)
                        len = snprintf(buf, sizeof(buf), "%d %d\n", i, buffer[i]);
                else
                        len = snprintf(buf, sizeof(buf), "%d %d\ne\n", i, buffer[i]);

                rc = gpif_write(&session, (const char*)buf, &len);
                if(rc != EGPIFOK) {
                        printf("Failed to plot data item %d\n", i);
                        gpif_close(&session);
                        return 1;
                }
        }

        /*
         * for (i = 0;i<2051;i++)
         *       printf("buffer[%d]: %f\n", i, buffer[i]);
         */

        gpif_close(&session);
        return 0;
}

