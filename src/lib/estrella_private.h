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

#ifndef _ESTRELLA_PRIVATE_H
#define _ESTRELLA_PRIVATE_H

#include <sys/time.h>
#include "estrella.h"

/* ######################################################################### */
/*                            TODO / Notes                                   */
/* ######################################################################### */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

typedef struct timeval estr_timestamp_t;

/* ######################################################################### */
/*                           Private interface (Lib)                         */
/* ######################################################################### */

/** Sleep microseconds
 *
 * TODO: rem does not yet return the remainder
 *
 * @param us            Number of microseconds to sleep
 * @param rem           If we failed to sleep for the requested number of
 *                      microseconds rem returns the time remaining.
 *
 * @return ESTROK       Success
 * @return ESTRERR      Failed to sleep for the given number of us
 */
int estrella_usleep(unsigned long us, unsigned long *rem);

/** Get a timestamp
 *
 * @param ts            Timestamp variable
 *
 * @return ESTROK       Success
 * @return ESTRERR      Error
 */
int estrella_timestamp_get(estr_timestamp_t *ts);

/** Get the difference in milliseconds between two timestamps
 *
 * @param ts1           Pointer to first timestamp
 * @parma ts2           Pointer to second timestamp
 * @param diff          Returns the difference in milliseconds
 *
 * @return ESTROK       Diff successfully calculated
 * @return ESTRERR      Diff could not be calculated
 */
int estrella_timestamp_diffms(estr_timestamp_t *ts1, estr_timestamp_t *ts2, unsigned long *diff);

/** Close a lock
 *
 * @param lock          The lock to be locked
 *
 * @return ESTROK       Successfully locked
 * @return ESTRERR      Unable to lock
 */
int estrella_lock(estr_lock_t *lock);

/** Unlock a lock
 *
 * @param lock          The lock to be unlocked
 *
 * @return ESTROK       Successfully unlocked
 * @return ESTRERR      Unable to unlock
 */
int estrella_unlock(estr_lock_t *lock);

/** Check a lock
 *
 * @param lock          The lock to be checked
 *
 * @return 1            Locked
 * @return 0            Not locked
 */
int estrella_islocked(estr_lock_t *lock);

/** Allocate memory 
 *
 * @param size          Number of bytes to alloc
 *
 * @return NULL         Memory allocation failed
 * @return ptr          Pointer to allocated memory area
 */
void *estrella_malloc(size_t size);

/** Free memory allocated by estrella_malloc()
 *
 * @param ptr           Pointer to memory which is to be freed
 */
void estrella_free(void *ptr);

#endif /* _ESTRELLA_PRIVATE_H */

