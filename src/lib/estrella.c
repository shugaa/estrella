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

#include "estrella.h"
#include "estrella_usb.h"
#include "estrella_private.h" 

#include <string.h>

/* ######################################################################### */
/*                            TODO / Notes                                   */
/* ######################################################################### */

/* the estrella_init() as well as the estrella_close() function manipulate the
 * session store. So in order to make this library reentrant we should
 * eventually put some semaphores around that code which adds or deletes
 * elements. and manipulates global flags */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

#define FLAG_SESSINIT           (1<<0)

/* ######################################################################### */
/*                           Private interface (Module)                      */
/* ######################################################################### */
int prv_find_session(estrella_session_handle_t session, estrella_session_t **psession);

/* ######################################################################### */
/*                           Implementation                                  */
/* ######################################################################### */

static dll_list_t sessions;
static int flags = 0;

int prv_find_session(estrella_session_handle_t session, estrella_session_t **psession)
{
    dll_iterator_t it;
    void *tmpsession = NULL;
    unsigned char found = 0;

    if (!psession)
        return ESTRINV;

    /* Try to find a session with the given handle */
    dll_iterator_new(&it, &sessions);
    while (dll_iterator_next(&it, &tmpsession) == EDLLOK) {
        if (((estrella_session_t*)tmpsession)->handle == session) {
            found = 1;
            break;
        }
    }
    dll_iterator_free(&it);

    /* Return the found session */
    if (found > 0) {
        *psession = tmpsession;
        return ESTROK;
    }

    return ESTRERR;
}

int estrella_find_devices(dll_list_t *devices)
{
    int rc;

    /* Find USB connected devices and add the to the list */
    rc = estrella_usb_find_devices(devices);
    if (rc != ESTROK)
        return ESTRERR;

    /* We might now try to find LPT devices... */

    return ESTROK;
}

int estrella_init(estrella_session_handle_t *session, estrella_dev_t *dev)
{
    int rc;
    void *tmpsession = NULL;
    estrella_session_t *newsession = NULL;
    estrella_session_handle_t newhandle;
    dll_iterator_t it;

    /* Somebody wants to open a new session. So first of all we need to check
     * wheter we need to initialize our session store or if this has happened
     * already. */
    if ((flags & FLAG_SESSINIT) == 0) {
        dll_init();
        dll_new(&sessions);
        flags |= FLAG_SESSINIT;
    }

    /* Find a session handle that we can use */
    newhandle = 0;
    dll_iterator_new(&it, &sessions);
    while (dll_iterator_next(&it, &tmpsession) == EDLLOK) {
        if (((estrella_session_t*)tmpsession)->handle >= newhandle)
            newhandle = ((estrella_session_t*)tmpsession)->handle + 1;
    }
    dll_iterator_free(&it);

    /* Create a new session */
    rc = dll_append(&sessions, &tmpsession, sizeof(estrella_session_t));
    if (rc != EDLLOK)
        return ESTRNOMEM;

    /* The tmpsession newsession stuff is necessary in order not to break gcc's
     * strict aliasing rules if enabled */
    newsession = (estrella_session_t*)tmpsession;
    newsession->handle = newhandle;
    memcpy(&(newsession->dev), dev, sizeof(estrella_dev_t));

    /* Initialize device and session here */
    if (dev->devicetype == ESTRELLA_DEV_USB) 
        rc = estrella_usb_init(newsession, dev);

    if (rc != ESTROK)
        return ESTRERR;

    /* These should be the default settings */
    newsession->xtrate = ESTR_XRES_HIGH;
    newsession->rate = 18;
    newsession->scanstoavg = 1;
    newsession->xsmooth = ESTR_XSMOOTH_NONE;
    newsession->tempcomp = ESTR_TEMPCOMP_OFF;
    newsession->xtmode = ESTR_XTMODE_NORMAL;

    /* Return a handle to the new session */
    *session = newhandle;

    return ESTROK;
}

int estrella_close(estrella_session_handle_t session)
{
    int rc;
    void *tmpsession = NULL;
    dll_iterator_t it;
    unsigned int numsessions;
    unsigned char found = 0;
    int i;

    /* Some very basic sanity checking */
    if ((flags & FLAG_SESSINIT) == 0)
        return ESTRINV;
    
    /* Find the session referenced by the given handle */
    i = 0;
    dll_iterator_new(&it, &sessions);
    while (dll_iterator_next(&it, &tmpsession) == EDLLOK) {
        if (((estrella_session_t*)tmpsession)->handle == session) {
            found = 1;    
            break;
        }

        i++;
    }
    dll_iterator_free(&it); 

    if (found < 1)
        return ESTRINV;

    /* Detach this session's device */
    if (((estrella_session_t*)tmpsession)->dev.devicetype == ESTRELLA_DEV_USB)
        estrella_usb_close((estrella_session_t*)tmpsession);

    /* Destroy the session */
    rc = dll_remove(&sessions, i);
    if (rc != EDLLOK)
        return ESTRERR;

    /* Check if this was the last session. If so we might as well destroy our
     * session store */
    rc = dll_count(&sessions, &numsessions);
    if (rc != EDLLOK)
        return ESTRERR;

    if (numsessions == 0) {
        dll_free(&sessions);
        flags &= ~FLAG_SESSINIT;
    }

    return ESTROK;
}

int estrella_mode(estrella_session_handle_t session, estr_xtmode_t xtmode)
{
    int rc;
    estrella_session_t *psession = NULL;

    /* Find the session referenced by the supplied handle */
    rc = prv_find_session(session, &psession);
    if (rc != ESTROK)
        return ESTRINV;

    if ((xtmode >= ESTR_XTMODE_TYPES) || (xtmode < 0))
        return ESTRINV;

    psession->xtmode = xtmode;

    return ESTROK;
}

int estrella_rate(estrella_session_handle_t session, int rate, estr_xtrate_t xtrate)
{
    int rc;
    estrella_session_t *psession = NULL;

    /* Find the session referenced by the supplied handle */
    rc = prv_find_session(session, &psession);
    if (rc != ESTROK)
        return ESTRINV;

    /* Check xtrate parameter validity */
    if ((xtrate >= ESTR_XRES_TYPES) || (xtrate < 0))
        return ESTRINV;

    /* We can not go lower than 2 ms and not longer than 65500 */
    if ((rate < 2) || (rate > 65500))
        return ESTRINV;

    /* Talk to the device and set the rate. In the original driver xtrate only
     * get's set when you set the rate. So we have created a compound command
     * here which makes sure the device knows about rate and xtrate at any time. */
    if (psession->dev.devicetype == ESTRELLA_DEV_USB)
        rc = estrella_usb_rate(psession, rate, xtrate);
    else
        rc = 0;

    if (rc != ESTROK)
        return ESTRERR;

    /* Save the parameters */
    psession->rate = rate;
    psession->xtrate = xtrate;

    return ESTROK;
}

int estrella_scan(estrella_session_handle_t session, float *buffer)
{
    int rc, i;
    estrella_session_t *psession = NULL;
    float tmpbuf[2051];

    /* TODO: xsmooth and tempcomp still need to be implemented */

    if (!buffer)
        return ESTRINV;

    /* Find the session referenced by the supplied handle */
    rc = prv_find_session(session, &psession);
    if (rc != ESTROK)
        return ESTRINV;

    /* Start a scan */
    for (i=0;i<psession->scanstoavg;i++) {
        int j;
        float *mybuf;

        /* We ususally write to buffer directly, tmbbuf is only use if we have
         * to average across multiple scans */
        if (i == 0)
            mybuf = buffer;
        else
            mybuf = tmpbuf;


        if (psession->dev.devicetype == ESTRELLA_DEV_USB)
            rc = estrella_usb_scan(psession, mybuf);
        else
            rc = ESTROK;

        /* Break on error */
        if (rc != ESTROK)
            break;

        /* No average handling on the first run */
        if (i == 0)
            continue;

        /* If we have to perform multiple scans the results are added to buffer.
         * The averaging happens only when all scans are complete. Which of
         * course poses a problem regarding the float value range. */
        for (j=0;j<2051;j++)
            buffer[j] += mybuf[j];
    }

    if (rc == ESTRTIMEOUT)
        return rc;
    else if (rc != ESTROK)
        return ESTRERR;

    /* Now check if we need to average or not. This is not necessary if there
     * was only one scan to perform anyway. */
    if (psession->scanstoavg > 1)
        for (i=0;i<2051;i++)
            buffer[i] =  buffer[i]/(float)psession->scanstoavg;

    return ESTROK;
}

int estrella_update(estrella_session_handle_t session, int scanstoavg, estr_xsmooth_t xsmooth, estr_tempcomp_t tempcomp)
{
    int rc;
    estrella_session_t *psession = NULL;

    /* Find the session referenced by the supplied handle */
    rc = prv_find_session(session, &psession);
    if (rc != ESTROK)
        return ESTRINV;

    /* Check validity of input parameters */
    if ((scanstoavg > 99) || (scanstoavg < 1))
        return ESTRINV;
    
    if ((xsmooth >= ESTR_XSMOOTH_TYPES) || (xsmooth < 0))
        return ESTRINV;

    if ((tempcomp >= ESTR_TEMPCOMP_TYPES) || (tempcomp < 0))
        return ESTRINV;

    /* Set the new parameters */
    psession->scanstoavg = scanstoavg;
    psession->xsmooth = xsmooth;
    psession->tempcomp = tempcomp;

    return ESTROK;
}

