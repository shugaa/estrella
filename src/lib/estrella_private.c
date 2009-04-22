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

#include <time.h>
#include "estrella_private.h"

/* ######################################################################### */
/*                            TODO / Notes                                   */
/* ######################################################################### */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

/* ######################################################################### */
/*                           Private interface (Module)                      */
/* ######################################################################### */

/* ######################################################################### */
/*                           Implementation                                  */
/* ######################################################################### */

int estrella_usleep(unsigned long us, unsigned long *rem)
{
    int rc;
    struct timespec req;
    UNUSED(rem);

    req.tv_sec = us/(1000*1000);
    req.tv_nsec = (us%(1000*1000))*1000;

    rc = nanosleep(&req, NULL);
    if (rc != 0)
        return ESTRERR;

    return ESTROK;
}

int estrella_timestamp_get(estr_timestamp_t *ts)
{
    int rc = gettimeofday((struct timeval*)ts, NULL);
    if (rc != 0)
        return ESTRERR;
    
    return ESTROK;    
}

int estrella_timestamp_diffms(estr_timestamp_t *ts1, estr_timestamp_t *ts2, unsigned long *diff)
{
    unsigned long ms_passed = 0;
    unsigned long stv1_adds = 0;
    unsigned long stv1_ussub = 0;
    struct timeval *stv1, *stv2;

    stv1 = (struct timeval*)ts1;
    stv2 = (struct timeval*)ts2;

    if (stv1->tv_sec > stv2->tv_sec) {
        stv1 = (struct timeval*)ts2;
        stv2 = (struct timeval*)ts1;
    } 

    if (stv2->tv_sec != stv1->tv_sec) {
        ms_passed += ((1000*1000)-stv1->tv_usec)/1000;
        stv1_adds = 1;
        stv1_ussub = stv1->tv_usec;
    } else if (stv1->tv_usec > stv2->tv_usec) {
        stv1 = (struct timeval*)ts2;
        stv2 = (struct timeval*)ts1; 
    }

    ms_passed += (stv2->tv_sec - (stv1->tv_sec+stv1_adds))*1000;
    ms_passed += (stv2->tv_usec - (stv1->tv_usec-stv1_ussub))/1000;

    *diff = ms_passed; 

    return ESTROK;
}

int estrella_lock(estr_lock_t *lock)
{
    *((int*)lock) = 1;
    return ESTROK;
}

int estrella_unlock(estr_lock_t *lock)
{
    *((int*)lock) = 0;
    return ESTROK;
}

int estrella_islocked(estr_lock_t *lock)
{
    if (*((int*)lock) == 0)
        return 0;
    else
        return 1;
}

void *estrella_malloc(size_t size)
{
    return malloc(size);
}

void estrella_free(void *ptr)
{
    free(ptr);
}
