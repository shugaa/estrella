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

/* Uncomment if on your platform there's no clock_gettime() */
#define ESTRELLA_TEST_TIMING

/* Uncomment if you dont want to have the scan result printed */
//#define ESTRELLA_TEST_RESULT

int timestamp_diffus(struct timespec *ts1, struct timespec *ts2, unsigned long *diff)
{
    unsigned long us_passed = 0;
    unsigned long stv1_adds = 0;
    unsigned long stv1_nssub = 0;
    struct timespec *stv1, *stv2;

    stv1 = ts1;
    stv2 = ts2;

    if (stv1->tv_sec > stv2->tv_sec) {
        stv1 = ts2;
        stv2 = ts1;
    } 

    if (stv2->tv_sec != stv1->tv_sec) {
        us_passed += ((1000*1000*1000)-stv1->tv_nsec)/1000;
        stv1_adds = 1;
        stv1_nssub = stv1->tv_nsec;
    } else if (stv1->tv_nsec > stv2->tv_nsec) {
        stv1 = ts2;
        stv2 = ts1; 
    }

    us_passed += (stv2->tv_sec - (stv1->tv_sec+stv1_adds))*1000*1000;
    us_passed += (stv2->tv_nsec - (stv1->tv_nsec-stv1_nssub))/1000;

    *diff = us_passed; 

    return ESTROK;
}

int main(int argc, char *argv[]) 
{
    int rc;
    dll_list_t devices;
    void *device = NULL;
    unsigned int numdevices = 0;
    estrella_session_t esession;
    int i;
    float buffer[2051];

#ifdef ESTRELLA_TEST_TIMING
    struct timespec tp_start1, tp_start2, tp_res1, tp_res2;
    unsigned long usec_start;
    unsigned long usec_res;
    unsigned long diff;
#endif

    dll_init(&devices);

    rc = estrella_find_devices(&devices);
    if (rc != 0) {
        printf("Unable to search for usb devices\n");
        dll_clear(&devices);
        return 1;
    }
   
    rc = dll_count(&devices, &numdevices);
    if ((rc != EDLLOK) || (numdevices == 0)) {
        printf("No devices found\n");
        dll_clear(&devices);
        return 1;
    }

    /* get the first of the found devices. */
    rc = dll_get(&devices, &device, NULL, 0);
    if (rc != EDLLOK) {
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

    rc = estrella_rate(&esession, 50, ESTR_XRES_HIGH);
    if (rc != 0) {
        printf("Unable to set rate\n");
        estrella_close(&esession);
        dll_clear(&devices);
        return 1;
    }

    usec_start = 0;
    usec_res = 0;
    for(i=0;i<1000;i++) {

        printf("i: %d\n", i);

#ifdef ESTRELLA_TEST_TIMING
        clock_gettime(CLOCK_MONOTONIC, &tp_start1);
#endif

        rc = estrella_async_scan(&esession);
        if (rc != ESTROK)
            break;

#ifdef ESTRELLA_TEST_TIMING
        clock_gettime(CLOCK_MONOTONIC, &tp_start2);
        clock_gettime(CLOCK_MONOTONIC, &tp_res1);
#endif

        rc = estrella_async_result(&esession, buffer);
        if (rc != ESTROK)
            break;

#ifdef ESTRELLA_TEST_TIMING
        clock_gettime(CLOCK_MONOTONIC, &tp_res2);
        timestamp_diffus(&tp_start1, &tp_start2, &diff);
        usec_start += diff;
        timestamp_diffus(&tp_res1, &tp_res2, &diff);
        usec_res += diff;
        printf("usec_res: %lu\n", usec_res);
#endif

    }

    if (rc != ESTROK) {
        printf("Unable to scan\n");
        estrella_close(&esession);
        dll_clear(&devices);
        return 1;
    }

#ifdef ESTRELLA_TEST_TIMING
    printf("tart scan took %luus, getting results took %luus\n", usec_start/1000, usec_res/(1000));
#endif

#ifdef ESTRELLA_TEST_RESULT
    printf("[");
    for (i=0;i<2051;i++)
        printf("%f,\n", buffer[i]);
    printf("]\n");
#endif

    estrella_close(&esession);
    dll_clear(&devices);

    return 0;
}
