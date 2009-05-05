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

#include <string.h>

#include "estrella.h"
#include "estrella_usb.h"
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

int estrella_num_devices(int *num)
{
    int rc;
    unsigned int listsize;
    dll_list_t devices;

    if (!num)
        return ESTRINV;

    rc = dll_new(&devices);
    if (rc != EDLLOK)
        return ESTRERR;

    rc = estrella_find_devices(&devices);
    if (rc != ESTROK) {
        dll_clear(&devices);
        return ESTRERR;
    }

    rc = dll_count(&devices, &listsize);
    if (rc != EDLLOK) {
        dll_clear(&devices);
        return ESTRERR;
    }

    *num = (int)listsize;
    dll_clear(&devices);

    return ESTROK;
}

int estrella_get_device(estrella_dev_t *dev, int num)
{
    int rc;
    unsigned int listsize;
    dll_list_t devices;
    void *devtmp;

    if (!dev)
        return ESTRINV;

    rc = dll_new(&devices);
    if (rc != EDLLOK)
        return ESTRERR;

    rc = estrella_find_devices(&devices);
    if (rc != ESTROK) {
        dll_clear(&devices);
        return ESTRERR;
    }

    rc = dll_count(&devices, &listsize);
    if (rc != EDLLOK) {
        dll_clear(&devices);
        return ESTRERR;
    }

    if ((num >= (int)listsize) || (num < 0)) {
        dll_clear(&devices);
        return ESTRINV;
    }

    rc = dll_get(&devices, (void**)&devtmp, num);
    if (rc != EDLLOK) {
        dll_clear(&devices);
        return ESTRERR;
    }

    memcpy(dev, (estrella_dev_t*)devtmp, sizeof(estrella_dev_t));
    dll_clear(&devices);

    return ESTROK;
}

int estrella_init(estrella_session_t *session, estrella_dev_t *dev)
{
    int rc;

    if (!session)
        return ESTRINV;

    if (!dev)
        return ESTRINV;

    memset(session, 0, sizeof(estrella_session_t));

    memcpy(&(session->dev), dev, sizeof(estrella_dev_t));

    /* Initialize device and session here */
    if (dev->devicetype == ESTRELLA_DEV_USB)
        rc = estrella_usb_init(session, dev);
    else
        rc = ESTRNOTIMPL;

    if (rc == ESTRNOTIMPL)
        return rc;
    else if (rc != ESTROK)
        return ESTRERR;

    /* These should be the default settings */
    session->xtrate = ESTR_XRES_HIGH;
    session->rate = 18;
    session->scanstoavg = 1;
    session->xsmooth = ESTR_XSMOOTH_NONE;
    session->tempcomp = ESTR_TEMPCOMP_OFF;
    session->xtmode = ESTR_XTMODE_NORMAL;
    estrella_unlock(&session->lock);

    return ESTROK;
}

int estrella_close(estrella_session_t *session)
{
    int rc;

    /* Some very basic sanity checking */
    if (!session)
        return ESTRINV;
    
    /* Detach this session's device */
    if (session->dev.devicetype == ESTRELLA_DEV_USB)
        rc = estrella_usb_close(session);
    else 
        rc = ESTRNOTIMPL;

    if (rc == ESTRNOTIMPL)
        return rc;
    else if (rc != ESTROK)
        return ESTRERR;

    return ESTROK;
}

int estrella_mode(estrella_session_t *session, estr_xtmode_t xtmode)
{
    if (!session)
        return ESTRINV;

    if ((xtmode >= ESTR_XTMODE_TYPES) || (xtmode < 0))
        return ESTRINV;

    session->xtmode = xtmode;

    return ESTROK;
}

int estrella_rate(estrella_session_t *session, int rate, estr_xtrate_t xtrate)
{
    int rc;

    if (!session)
        return ESTRINV;

    /* Check xtrate parameter validity */
    if ((xtrate >= ESTR_XRES_TYPES) || (xtrate < 0))
        return ESTRINV;

    /* We can not go lower than 2 ms (at least the device I got can't, other's
     * obviously can) and not longer than 65500 */
    if ((rate < 2) || (rate > 65500))
        return ESTRINV;

    /* Talk to the device and set the rate. In the original driver xtrate only
     * get's set when you set the rate. So we have created a compound command
     * here which makes sure the device knows about rate and xtrate at any time. */
    if (session->dev.devicetype == ESTRELLA_DEV_USB)
        rc = estrella_usb_rate(session, rate, xtrate);
    else
        rc = ESTRNOTIMPL;

    if (rc == ESTRNOTIMPL)
        return rc;
    else if (rc != ESTROK)
        return ESTRERR;

    /* Save the parameters */
    session->rate = rate;
    session->xtrate = xtrate;

    return ESTROK;
}

int estrella_async_scan(estrella_session_t *session)
{
    int rc;

    if (!session)
        return ESTRINV;

    /* If this session is locked another async scan is currently in progress */
    if (estrella_islocked(&session->lock) == 1)
        return ESTRERR;

    if (session->dev.devicetype == ESTRELLA_DEV_USB)
        rc = estrella_usb_scan_init(session);
    else
        rc = ESTRNOTIMPL;

    if (rc == ESTRNOTIMPL)
        return rc;
    else if (rc != ESTROK)
        return ESTRERR;

    /* Lock this session until results have been fetched */
    estrella_lock(&session->lock);

    return ESTROK;
}

int estrella_async_result(estrella_session_t *session, float *buffer)
{
    int rc;

    if (!session)
        return ESTRINV;

    if (!buffer)
        return ESTRINV;

    /* If this session is not locked no async scan has been started */
    if (estrella_islocked(&session->lock) == 0)
        return ESTRERR;

    if (session->dev.devicetype == ESTRELLA_DEV_USB) {
        rc = estrella_usb_scan_result(session, buffer);

        /* No matter if success or error we need to unlock the session again */
        estrella_unlock(&session->lock);
    } else
        rc = ESTRNOTIMPL;

    if (rc == ESTRNOTIMPL)
        return rc;
    else if (rc != ESTROK)
        return ESTRERR;

    return ESTROK;
}

int estrella_scan(estrella_session_t *session, float *buffer)
{
    int rc, i;
    float tmpbuf[2051];

    /* TODO: xsmooth and tempcomp still need to be implemented */

    if (!session)
        return ESTRINV;

    if (!buffer)
        return ESTRINV;

    rc = ESTROK;

    /* Start a scan */
    for (i=0;i<session->scanstoavg;i++) {
        int j;
        float *mybuf;

        /* We ususally write to buffer directly, tmbbuf is only used if we have
         * to average across multiple scans */
        if (i == 0)
            mybuf = buffer;
        else
            mybuf = tmpbuf;

        if (session->dev.devicetype == ESTRELLA_DEV_USB) {
            rc = estrella_usb_scan_init(session);
            if (rc == ESTROK)
                rc = estrella_usb_scan_result(session, mybuf);
        } else
            rc = ESTRNOTIMPL;

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

    switch(rc) {
        case ESTRTIMEOUT:
            return rc;
        case ESTRNOTIMPL:
            return rc;
        case ESTROK:
            break;
        default:
            return ESTRERR;
            break;
    }

    /* Now check if we need to average or not. This is not necessary if there
     * was only one scan to perform anyway. */
    if (session->scanstoavg > 1)
        for (i=0;i<2051;i++)
            buffer[i] =  buffer[i]/(float)session->scanstoavg;

    return ESTROK;
}

int estrella_update(estrella_session_t *session, int scanstoavg, estr_xsmooth_t xsmooth, estr_tempcomp_t tempcomp)
{
    /* Check validity of input parameters */
    if (!session)
        return ESTRINV;

    if ((scanstoavg > 99) || (scanstoavg < 1))
        return ESTRINV;
    
    if ((xsmooth >= ESTR_XSMOOTH_TYPES) || (xsmooth < 0))
        return ESTRINV;

    if ((tempcomp >= ESTR_TEMPCOMP_TYPES) || (tempcomp < 0))
        return ESTRINV;

    /* Set the new parameters */
    session->scanstoavg = scanstoavg;
    session->xsmooth = xsmooth;
    session->tempcomp = tempcomp;

    return ESTROK;
}

