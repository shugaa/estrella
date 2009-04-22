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
 * extensible to drive LPT connected equipment. Provided the IEEE-1284
 * communications protocol is known of course.
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

/* BR: Personally I would really like to hide most of the implementation details
 * from the client. Most of all the contents of estrella sessions. Unfortunately
 * this is not possible when sessions are supposed to be allocated client side
 * as it is right now. The compiler needs to know the size of the session
 * struct. The only way out would be to allocate sessions inside the library
 * which eventually is a bit dangerous with regard to memory management. Yet
 * another option would be to provide a fixed size session store in the lib but
 * this would of course limit the number of sessions the user can create. */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

/* Maximum path string length */
#define ESTRELLA_PATH_MAX   (256)

/* Library error codes */
#define ESTROK              (0) 
#define ESTRERR             (1)
#define ESTRINV             (2)
#define ESTRNOMEM           (3)
#define ESTRTIMEOUT         (4)
#define ESTRNOTIMPL         (5)
#define ESTRALREADY         (6)

/* This is not a thread safe locking mechanism but for our simple purposes
 * (locking sessions during async scans) it should be enough, at least for now
 * and for my purposes;) */
typedef int estr_lock_t;

/* NOTE: The *_TYPES entry must always be last in the following enums. */

/** Estrella operation modes */
typedef enum {
    ESTR_XTMODE_NORMAL    = (0),
    ESTR_XTMODE_TRIGGER,
    ESTR_XTMODE_TYPES,
} estr_xtmode_t;

/** Estrella xsmoothing setting */
typedef enum {
    ESTR_XSMOOTH_NONE     = (0),
    ESTR_XSMOOTH_5PX,
    ESTR_XSMOOTH_9PX,
    ESTR_XSMOOTH_17PX,
    ESTR_XSMOOTH_33PX,
    ESTR_XSMOOTH_TYPES,
} estr_xsmooth_t;

/** Estrella temperature compensation */
typedef enum {
    ESTR_TEMPCOMP_OFF     = (0),
    ESTR_TEMPCOMP_ON,
    ESTR_TEMPCOMP_TYPES,
} estr_tempcomp_t;

/** Xtiming resolution parameters */
typedef enum {
    ESTR_XRES_LOW         = (0),
    ESTR_XRES_MEDIUM,
    ESTR_XRES_HIGH,
    ESTR_XRES_TYPES,
} estr_xtrate_t;

/** Indicates the device type.
 *
 * Spectrometers may be connected to the computer through USB or the parallel
 * port. While at the moment this driver can only handle USB connected devices
 * ist should be fairly easy to implement IEEE-1284 communications. Provided ou
 * know about the protocol being used. */
typedef enum {
    ESTRELLA_DEV_USB,
    ESTRELLA_DEV_LPT,
} estrella_devicetype_t;

/** USB device information. */
typedef struct {
    char bus[ESTRELLA_PATH_MAX];
    unsigned char devnum;

    unsigned short vendorid;
    unsigned short productid;

    char manufacturer[128];
    char product[128];
    char serialnumber[32];
} estrella_usbdev_t;

/** Generic device information.
 *
 * This type encapsulates USB as well as EPP devices. */
typedef struct {
    estrella_devicetype_t devicetype;
    union {
        estrella_usbdev_t usb;
        /* Add IEEE-1284 specifics here. */
    } spec;
} estrella_dev_t;

/** Session type.
 *
 * A session is always associated with a specifc device and holds pretty much
 * all the necessary management information. */
typedef struct {
    int rate;
    int scanstoavg;
    estr_xtmode_t xtmode;
    estr_xtrate_t xtrate;
    estr_xsmooth_t xsmooth;
    estr_tempcomp_t tempcomp;

    estrella_dev_t dev;
    union {
        struct usb_dev_handle *usb_dev_handle; 
        /* Add IEEE-1284 handle here */
    } spec;

    /* Used to lock sessions during asynchronous scannning operations */
    estr_lock_t lock;
} estrella_session_t;

/* ######################################################################### */
/*                           Public interface                                */
/* ######################################################################### */

/** Find devices connected to the host
 *
 * If necessary initialization (namely firmware loading) is being performed
 * transparently. This function gives us a set of estrella_dev_t items, which
 * we can use to create sessions using estrella_init().
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

/** Find out about the number of spectrometer devices in the system
 *
 * This function basically just calls estrella_find_devices() and returns the
 * size of the list. It's provided for convenience in environments where it's
 * difficult to handle dll_list_t items. In regular C/C++ code we should be
 * using estrella_find_devices() to minimize overhead.
 *
 * @param num           Returns the number of devices found
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      Invalid parameter
 * @return ESTRERR      Device scan failed
 */
int estrella_num_devices(int *num);

/** Get a device
 *
 * Like estrella_num_devices() this is just a convenience function. It calls
 * estrella_find_devices() under the hood and returns device 'num' from the list.
 *
 * @param dev           Pointer to an estrella_device_t struct which is to be
 *                      populated with device information
 * @param num           The requested device number
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      Invalid parameter
 * @return ESTRERR      Device not found
 */
int estrella_get_device(estrella_dev_t *dev, int num);

/** Initialize a session on a device
 *
 * 'Session' is pretty much what 'channel' is for the windows driver. At least
 * in our case we can get some basic info about the device we're dealing with
 * prior to binding it to a session. This is not the case for the windows driver
 * I believe.
 *
 * @param session       A pointer to an estrella_session_t which is to be
 *                      initialized
 * @param dev           Device to be used in this session
 *
 * @return ESTROK       No errors occured
 * @return ESTRERR      Could not create a session for this device
 * @return ESTRNOMEM    Couldn't create session due to lack of memory
 * @return ESTRNOTIMPL  Function has not been implemented for this device
 */
int estrella_init(estrella_session_t *session, estrella_dev_t *dev);

/** Destroy a session
 *
 * Be nice and destroy the session when we're done using it.
 *
 * @param session       Session to be destroyed
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      The supplied session handle is invalid
 * @return ESTRERR      Session could not be destroyed
 * @return ESTRNOTIMPL  Function has not been implemented for this device
 */
int estrella_close(estrella_session_t *session);

/** Set integration time and timing resolution clock rate
 *
 * @param session       Session
 * @param rate          Detector integration rate in ms (2-65500)
 * @param xtrate        ESTR_XRES_LOW/MEDIUM/HIGH. ESTR_XRES_HIGH is the slowest
 *                      but provides the highest resolution.
 *                      According to the manufacturer we should be using
 *                      integration times >30ms when operating in medium or high
 *                      resolution mode. In fact we should be fine with only
 *                      15ms in normal resolution mode. 
 *                      This has got to to with the clock slowdown in higher
 *                      resolution modes and the way data is being pushed out of
 *                      the ccd array. Have a look at the data sheet for details.
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 * @return ESTRERR      Could not set rate
 * @return ESTRNOTIMPL  Function has not been implemented for this device
 */
int estrella_rate(estrella_session_t *session, int rate, estr_xtrate_t xtrate);

/** Set the mode of operation
 *
 * In normal operation mode estrella_scan() will return ESTRTIMEOUT if it does not
 * receive any reply from the spectrometer device in time. When controlling
 * operations using a trigger input estrella will wait until it receives any
 * data from the device, no matter how long that might take.
 *
 * The manufacturer's documentation on this topic is rather sparse. This is what
 * I know:
 *  - Setting rate and xtrate works as usual
 *  - The spectormeter triggers with the rising edge (TTL level)
 *  - The trigger pulse's high slope should be no shorter than 1ms
 *  - With every trigger pulse the spectrometer will start scanning and return
 *    the results
 *
 * @param session       Session
 * @param xtmode        ESTR_XTMODE_NORMAL: Normal operation, ESTR_XTMODE_TRIGGER: External trigger
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 */
int estrella_mode(estrella_session_t *session, estr_xtmode_t xtmode);

/** Acquire a spectral scan
 *
 * @param session       Session
 * @param buffer        Array of float, 2051 elements wide
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 * @return ESTRTIMEOUT  Scan timed out (in normal operations mode)
 * @return ESTRNOTIMPL  Function has not been implemented for this device
 * @return ESTRERR      Scan failed
 */
int estrella_scan(estrella_session_t *session, float *buffer);

/** Acquire a spectral scan asynchronously
 *
 * estrella_async_scan() will just tell the spectrometer to start scanning but
 * it won't wait for the results of the scan, thus not blocking the calling
 * thread/process. Results can be fetched using estrella_async_result().
 *
 * This function won't do any averaging over multiple scans obviously, neither
 * does estrella_async_result(). You will either have to use the compound
 * estrella_scan() function or perform any averaging etc. in your client code.
 *
 * Note that estrella will not allow another scan to be started on this session
 * until results have been fetched using estrella_async_result().
 *
 * @param session       Session
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 * @return ESTRNOTIMPL  Function has not been implemented for this device
 * @return ESTRERR      Start of scan failed
 */
int estrella_async_scan(estrella_session_t *session);

/** Query results for an asynchronously started scan
 *
 * This function fetches the results for a scan initiated by
 * estrella_async_scan(). It will wait until either the device returns any data
 * or it times out.
 *
 * @param session       Session
 * @param buffer        Array of float, 2051 elements wide
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 * @return ESTRTIMEOUT  Scan timed out (in normal operations mode)
 * @return ESTRNOTIMPL  Operation has not been implemented for this device
 * @return ESTRERR      Scan failed
 */
int estrella_async_result(estrella_session_t *session, float *buffer);

/** Set data processing configuration
 *
 * TODO: xsmoothing and temperature compensation have not yet been implemented
 *
 * @oaram session       Session
 * @param scanstoavg    Scans to perform and average (1-99)
 * @param xsmooth       Smoothing. For possible values have a look at ESTR_XSMOOTH*
 * @param tempcomp      Temperature compensation. ESTR_TEMPCOMP_OFF or ESTR_TEMPCOMP_ON
 *
 * @return ESTROK       No errors occured
 * @return ESTRINV      A supplied input argument is invalid
 */
int estrella_update(estrella_session_t *session, int scanstoavg, estr_xsmooth_t xsmooth, estr_tempcomp_t tempcomp);

#endif /* _ESTRELLA_H */

