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

/** @file estrella_usb.h
 *
 * @brief Private USB device interface
 *
 * Implements USB device communications
 *
 * */

#ifndef _ESTRELLA_USB_H
#define _ESTRELLA_USB_H

#include <stddef.h>
#include "estrella.h"
#include "estrella_private.h"

/* ######################################################################### */
/*                            TODO / Notes                                   */
/* ######################################################################### */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

/** This is pretty much the representation of a URB. */
typedef struct 
{
    int requesttype;
    int request;
    int value;
    int index;

    int size;
    unsigned char *data;
} estrella_usb_request_t;

/* ######################################################################### */
/*                           Private interface (Lib)                         */
/* ######################################################################### */

/** Find USB devices
 *
 * If necessary firmware upload and reenumeration is being performed
 * transparently.
 *
 * TODO: I have a bit of a bad feeling about pushing this firmware to every
 * cypress chip around. Other manufacturers are eventually using the same IC and
 * bad things will happen if we throw a wrong firmware at it. Correct me if I'm
 * wrong. 
 *
 * So maybe it would be a better idea to provide a simple EPP2000 firmware
 * loading tool. The user could then decide on his own to load firmware to
 * /proc/bus/usb/foo/bar if he's sure it's a EPP2000 device.
 *
 * Anyway, since the USB LPT adapter cable is the only cypress device on my USB
 * bus I'll stick with the automatic firmware uploading for now.
 *
 * @param devices       An initialized dll_list_t which is to be populated with
 *                      found devices
 *
 * @return ESTROK       No errors occured
 * @return ESTRERR      Device discovery failed    
 */
int estrella_usb_find_devices(dll_list_t *devices);

/** Initialize a usb connected device
 *
 * @param session       Pointer to a session which is to be bound to the
 *                      supplied usb device
 * @param dev           Device to be used in this session
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplie dinput argument is invalid 
 * @return ESTRERR      Device initialization failed 
 */
int estrella_usb_init(estrella_session_t *session, estrella_dev_t *device);

/** Close a usb device session
 *
 * This basically just detaches from libusb. Which is important because it frees
 * the device for use in another session
 *
 * @param session       Session to detach the usb device from
 *
 * @return ESTROK       No errors occured
 */
int estrella_usb_close(estrella_session_t *session);

/** Set rate and xtrate
 *
 * @param session       Session for which to set these parameters
 * @param rate          Detector integration time
 * @param xtrate        x timing resolution
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplie dinput argument is invalid 
 * @return ESTRERR      Failed to set rate 
 */
int estrella_usb_rate(estrella_session_t *session, int rate, int xtrate);

/** Perform a scan
 *
 * @param session       Session to be used in this scan
 * @param buffer        result buffer, array of float, 2051 elements wide
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplie dinput argument is invalid 
 * @return ESTRTIMEOUT  Scan timed out
 * @return ESTRERR      Scan failed
 */
int estrella_usb_scan(estrella_session_t *session, float *buffer);

#endif /* _ESTRELLA_USB_H */

