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

#include <stdio.h>
#include <time.h>

#include "estrella.h"

int main(int argc, char *argv[]) 
{
    int rc;
    dll_list_t devices;
    void *device = NULL;
    unsigned int numdevices = 0;
    estrella_session_handle_t esession;
    int i;
    float buffer[2051];

    struct timespec tp1, tp2;

    dll_init();
    dll_new(&devices);

    rc = estrella_find_devices(&devices);
    if (rc != 0) {
        printf("Unable to search for usb devices\n");
        return 1;
    }
   
    rc = dll_count(&devices, &numdevices);
    if ((rc != EDLLOK) || (numdevices == 0)) {
        printf("No devices found\n");
        return 1;
    }

    /* get the first of the found devices. */
    rc = dll_get(&devices, &device, 0);
    if (rc != EDLLOK) {
        return 1;
    }

    rc = estrella_init(&esession, (estrella_dev_t*)device);
    if (rc != 0) {
        printf("Unable to create session\n");
        estrella_close(esession);
        return 1;
    }

    rc = estrella_update(esession, 1, 0, 0);
    if (rc != 0) {
        printf("Unable to set data processing configuration\n");
        estrella_close(esession);
        return 1;
    }

    rc = estrella_rate(esession, 20, 0);
    if (rc != 0) {
        printf("Unable to set rate\n");
        estrella_close(esession);
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &tp1);
    for(i=0;i<2000;i++) 
        rc = estrella_scan(esession, buffer);

    clock_gettime(CLOCK_MONOTONIC, &tp2);

    if (rc != 0) {
        printf("Unable to scan\n");
        estrella_close(esession);
        return 1;
    }

    tp2.tv_sec -= tp1.tv_sec;
    tp2.tv_nsec -= tp1.tv_nsec;

    printf("scanning took %ds, %dus\n", (unsigned int)tp2.tv_sec, (unsigned int)(tp2.tv_nsec/(1000)));


//BR     for (i=0;i<2000;i++)
//BR         printf("item %d: %f\n", i, buffer[i]);

    estrella_close(esession);

    dll_free(&devices);

    return 0;
}
