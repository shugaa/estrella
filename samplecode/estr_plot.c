/*
* Copyright (c) 2009, Björn Rehm (bjoern@shugaa.de)
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
*  * Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the author nor the names of its contributors may be
*    used to endorse or promote products derived from this software without
*    specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <gpif.h>
#include <dll_list.h>
#include <estrella.h>

#define C1 (0.5)
#define C2 (0.5)
#define C3 (0.5)

#define ESTRELLA_RESOLUTION     (ESTR_XRES_HIGH)
#define ESTRELLA_RATE           (50)

int estr_plot(void)
{
        int rc;
        int i;
        size_t len;
        char buf[256];
        gpif_session_t session;
        estrella_session_t esession;
        dll_list_t devices;
        void *device = NULL;
        unsigned int numdevices = 0;
        float data[2051];
        float xrange[2051];

        char *const myargv[] = {
                "gnuplot",
                "-noraise",
                "-persist",
                NULL
        };

        /* Create a device list for estrella */
        dll_init(&devices);

        /* Try to setup an estrella session */
        rc = estrella_find_devices(&devices);
        if (rc != 0) {
                printf("Unable to search for USB devices\n");
                dll_clear(&devices);
                return 1;
        }

        rc = dll_count(&devices, &numdevices);
        if ((rc != EDLLOK) || (numdevices == 0)) {
                printf("No devices found\n");
                dll_clear(&devices);
                return 1;
        }

        rc = dll_get(&devices, &device, NULL, 0);
        if (rc != EDLLOK) {
                printf("Cannot access estrella device 0\n");
                dll_clear(&devices);
                return 1;
        }

        rc = estrella_init(&esession, (estrella_dev_t*)device);
        if (rc != 0) {
                printf("Unable to create session\n");
                estrella_close(&esession);
                dll_clear(&devices);
                return 1;
        }

        rc = estrella_update(&esession, 1, ESTR_XSMOOTH_NONE, ESTR_TEMPCOMP_OFF);
        if (rc != 0) {
                printf("Unable to set data processing configuration\n");
                estrella_close(&esession);
                dll_clear(&devices);
                return 1;
        }

        rc = estrella_rate(&esession, ESTRELLA_RATE, ESTRELLA_RESOLUTION);
        if (rc != 0) {
                printf("Unable to set rate\n");
                estrella_close(&esession);
                dll_clear(&devices);
                return 1;
        }

        rc = estrella_scan(&esession, data);
        if (rc != 0) {
                printf("Scan failed\n");
                estrella_close(&esession);
                dll_clear(&devices);
                return 1;
        }

        /* Close estrella, we're done with it */
        estrella_close(&esession);
        dll_clear(&devices);

        /* Establish a gnuplot session */
        rc = gpif_init(&session, myargv);
        if (rc != EGPIFOK) {
                printf("Couldn't establish gpif session\n");
                return 1;
        }

        /* Calculate xaxis */
        for (i=0;i<2051;i++) {
                xrange[i] =
                        ((C2/4.0)*(float)(i*i)) + 
                        ((C1/2.0)*(float)i) +
                        (C3);
        }

        /* Setup Gnuplot */
        snprintf(buf, sizeof(buf), "unset mouse\nset terminal x11 1\nset multiplot\nset mouse\n");
        len = strlen(buf);
        rc = gpif_write(&session, (const char*)buf, &len);
        if(rc != EGPIFOK) {
                printf("Couldn't write to gnuplot\n");
                gpif_close(&session);
                return 1;
        }

        snprintf(buf, sizeof(buf), "set yrange [0:3000]\nset xrange [%f:%f]\n", xrange[0], xrange[2050]);
        len = strlen(buf);
        rc = gpif_write(&session, (const char*)buf, &len);
        if(rc != EGPIFOK) {
                printf("Couldn't write to gnuplot\n");
                gpif_close(&session);
                return 1;
        }

        strncpy(buf, "set grid xtics ytics layerdefault back linetype 0 linecolor rgb '#808080'\n", sizeof(buf));
        len = strlen(buf);
        rc = gpif_write(&session, (const char*)buf, &len);
        if(rc != EGPIFOK) {
                printf("Couldn't write to gnuplot\n");
                gpif_close(&session);
                return 1;
        }

        /* Prepare for plotting */
        snprintf(buf, sizeof(buf), "plot '-' smooth csplines notitle axes x1y1 with lines linecolor rgb '#FF0000' linewidth 2\n");
        len = strlen(buf);
        rc = gpif_write(&session, (const char*)buf, &len);
        if(rc != EGPIFOK) {
                printf("Couldn't write to gnuplot\n");
                gpif_close(&session);
                return 1;
        }

        /* Plot data */
        for (i=0;i<2051;i++) {
                if (i < 2050)
                        snprintf(buf, sizeof(buf), "%f %f\n", xrange[i], data[i]);
                else
                        snprintf(buf, sizeof(buf), "%f %f\ne\n", xrange[i], data[i]);

                len = strlen(buf);
                rc = gpif_write(&session, (const char*)buf, &len);
                if(rc != EGPIFOK) {
                        printf("Couldn't write to gnuplot\n");
                        gpif_close(&session);
                        return 1;
                }
        }

        /* Close gpif session */
        gpif_close(&session);

        return 0;
}

int main(void)
{
        int rc;
        rc = estr_plot();
        return rc;
}

