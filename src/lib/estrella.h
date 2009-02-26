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

/** @file estrella.h
 *
 * @brief Public driver interface
 *
 * */

/*! \mainpage Estrella device driver for the Stellarnet EPP2000 series of spectrometers.
 *
 * This driver is implemented as a userspace library on top of libusb. It has
 * been written with Linux as a target OS in mind but porting to any libusb
 * supported platform should be easy. If it takes any effort at all.
 *
 * Estrella can only handle USB devices at the moment but should be easily
 * extensible to drive LPT connected equipment. Provided the LPT communications
 * protocol is known of course.
 * 
 * Feedback to bjoern@shugaa.de is always highly appreciated.
 */


#ifndef _ESTRELLA_H
#define _ESTRELLA_H

#include <stddef.h>
#include <usb.h>
#include <dll_list.h>

/* ######################################################################### */
/*                            TODO / Notes                                   */
/* ######################################################################### */

/* return value handling needs to be improved significantly */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

#define ESTRELLA_PATH_MAX           (256)

/* Library error codes */
#define ESTROK              (0) 
#define ESTRERR             (1)
#define ESTRINV             (2)
#define ESTRNOMEM           (3)
#define ESTRTIMEOUT         (4)

/** Indicates the device type.
 *
 * Spectrometers may be connected to the computer through USB or the parallel
 * port. While at the moment this driver can only handle USB connected devices
 * ist should be fairly easy to implement LPT communications. Provided you know
 * about the protocol of course. */
typedef enum {
    ESTRELLA_DEV_USB,
    ESTRELLA_DEV_LPT,
} estrella_devicetype_t;

/** USB device information. */
typedef struct {
    char bus[ESTRELLA_PATH_MAX];
    unsigned char devnum;
} estrella_usbdev_t;

/** Generic device information.
 *
 * This type encapsulates USB as well as EPP devices. */
typedef struct {
    estrella_devicetype_t devicetype;
    union {
        estrella_usbdev_t usb;
        /* You might want to add LPT specifics here. */
    } spec;
} estrella_dev_t;

/** Session handle.
 *
 * Used by the client to identify his session. */
typedef int estrella_session_handle_t;

/* ######################################################################### */
/*                           Public interface                                */
/* ######################################################################### */

/** Find devices connected to the host
 *
 * If necessary initialization (namely firmware loading) is being performed
 * transparently. This function gives you a set of estrella_dev_t items, which
 * you can use to create sessions using estrella_init().
 *
 * Note that if no devices are found the returned list is empty but the function
 * call returns no errors.
 *
 * @param devices       An initialized dll_list_t instance which is to be
 *                      populated with found devices.
 *
 * @return ESTROK       No errors occured
 * @return ESTRERR      Device discovery failed
 */
int estrella_find_devices(dll_list_t *devices);

/** Initialize a session on a device
 *
 * Session is pretty much what the original driver means with "channel". This
 * implementation provides the advantage though that you actually know which
 * device you're talking to. In case of USB you get the vendor and product id.
 *
 * @param session       Returns a handle to a freshly initialized session
 * @param dev           Device to be used in this session
 *
 * @return ESTROK       No errors occured
 * @return ESTRERR      Could not create a session for this device
 * @return ESTRNOMEM    Couldn't create session due to lack of memory
 */
int estrella_init(estrella_session_handle_t *session, estrella_dev_t *dev);

/** Destroy a session
 *
 * It's pretty much vital to call this function when you're done using a session.
 * All resources are being freed here.
 *
 * @param session       Session to be destroyed
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      The supplied session handle is invalid
 * @return ESTRERR      Session could not be destroyed
 */
int estrella_close(estrella_session_handle_t session);

/** Set integration time and timing resolution clock rate
 *
 * @param session       Session
 * @param rate          Detector integration rate in ms (2-65500)
 * @param xtrate        0-2, where 2 is the slowest but provides the highest resolution
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 * @return ESTRERR      Could not set rate
 */
int estrella_rate(estrella_session_handle_t session, int rate, int xtrate);

/** Acquire a spectral scan
 *
 * @param session       Session
 * @param buffer        Array of float, 2051 elements wide
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 * @return ESTRTIMEOUT  Scan timed out
 * @return ESTRERR      Scan failed
 */
int estrella_scan(estrella_session_handle_t session, float *buffer);

/** Set data processing configuration
 *
 * TODO: xsmoothing and temperature compensation have not yet been implemented
 *
 * @param scanstoavg    Scans to perform and average (1-99)
 * @param xsmooth       (0=none, 1=5px, 2=9px, 3=17px, 4=33px)
 * @param tempcomp      0=off, 1=on
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 */
int estrella_update(estrella_session_handle_t session, int scanstoavg, int xsmooth, int tempcomp);

#endif /* _ESTRELLA_H */

