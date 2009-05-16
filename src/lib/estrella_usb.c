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
#include "estrella_private.h"
#include "estrella_usb.h"

#include <string.h>

/* ######################################################################### */
/*                            TODO / Notes                                   */
/* ######################################################################### */

/* ######################################################################### */
/*                            Types & Defines                                */
/* ######################################################################### */

/* When a scan takes (rate + PRV_DELAY)ms we consider it a timeout */
#define PRV_DELAY           (100)

/* This is how long we wait (ms) for devices to reenumerate after firmware
 * upload */
#define PRV_WAITFORDEVICE   (2000)

struct usb_ident {
    int id_vendor;
    int id_product;
};

/* ######################################################################### */
/*                           Private interface (Module)                      */
/* ######################################################################### */

static int prv_usb_preup(void);
static int prv_usb_upload_firmware(struct usb_device *dev);
static int prv_usb_find_devices(dll_list_t *devices);
static int prv_usb_get_handle(estrella_dev_t *device, struct usb_dev_handle **handle);
static int prv_usb_device_info(struct usb_device *dev, estrella_dev_t *device);

/* ######################################################################### */
/*                           Implementation                                  */
/* ######################################################################### */

/* To upload the firmware to the usb devices we pretty much just roll back the
 * sniffed transactions from the windows driver. These are the necessary
 * requests. */
extern estrella_usb_request_t usb_preup_req[];
extern size_t usb_preup_req_size;

/* These are the uninitialized usb devices on the bus. */
static struct usb_ident usb_devices_preup[] = {
    {0x04b4,0x8613},    
};

/* This is how they look after firmware upload and reenumeration */
static struct usb_ident usb_devices[] = {
    {0x0bd7,0xa012},
};

int prv_usb_device_info(struct usb_device *dev, estrella_dev_t *device)
{
    int rc;
    unsigned char buf[18];
    struct usb_dev_handle *usb_handle = NULL;

    usb_handle = usb_open(dev);
    if (!usb_handle)
        return ESTRERR;

    rc = usb_get_descriptor(usb_handle, 0x01, 0x00, (void*)buf, sizeof(buf));
    if (rc < 0) {
        usb_close(usb_handle);
        return ESTRERR;
    }

    /* Set product and vendor id */
    device->spec.usb.vendorid = (unsigned short)buf[11];
    device->spec.usb.vendorid = device->spec.usb.vendorid << 8;
    device->spec.usb.vendorid |= (unsigned short)buf[10];

    device->spec.usb.productid = (unsigned short)buf[9];
    device->spec.usb.productid = device->spec.usb.productid << 8;
    device->spec.usb.productid |= (unsigned short)buf[8];

    /* Set manufacturer/product/serial number strings */
    rc = usb_get_string_simple(usb_handle, (int)buf[14], device->spec.usb.manufacturer, sizeof(device->spec.usb.manufacturer));
    if (rc < 0) {
        usb_close(usb_handle);
        return ESTRERR;
    }
    rc = usb_get_string_simple(usb_handle, (int)buf[15], device->spec.usb.product, sizeof(device->spec.usb.product));
    if (rc < 0) {
        usb_close(usb_handle);
        return ESTRERR;
    }

    /* We don't get a serial number usually so a failure here is not lethal */
    rc = usb_get_string_simple(usb_handle, (int)buf[16], device->spec.usb.serialnumber, sizeof(device->spec.usb.serialnumber));
    if (rc < 0) {
        strncpy(device->spec.usb.serialnumber, "?", sizeof(device->spec.usb.serialnumber));
    }

    usb_close(usb_handle);

    return ESTROK;
}

int prv_usb_get_handle(estrella_dev_t *device, struct usb_dev_handle **handle)
{
    struct usb_bus *usb_bus;
    struct usb_device *dev;
    struct usb_dev_handle *usb_handle = NULL;

    /* update bus and device information */
    usb_find_busses();
    usb_find_devices();

    /* Find the requested bus */
    for (usb_bus=usb_busses;usb_bus;usb_bus=usb_bus->next) {
        if (strcmp(usb_bus->dirname, device->spec.usb.bus) == 0)
            break;
    }

    if (usb_bus == NULL)
        return ESTRINV;

    /* Find the requested device */
    for (dev=usb_bus->devices;dev;dev=dev->next) {
        if (dev->devnum == device->spec.usb.devnum)
            break;
    }

    if (dev == NULL)
        return ESTRINV;

    /* Open the device */
    usb_handle = usb_open(dev);
    if (!usb_handle)
        return ESTRERR;

    /* Return the handle */
    *handle = usb_handle;

    return ESTROK;
}

int prv_usb_preup(void)
{
    struct usb_bus *usb_bus;
    struct usb_device *dev;
    int i;

    /* Update bus and device information */
    usb_find_busses();
    usb_find_devices();

    /* Look at every device */
    for (usb_bus=usb_busses;usb_bus;usb_bus=usb_bus->next) {
        for (dev=usb_bus->devices;dev;dev=dev->next) {

            /* Check if the device is one of those that need firmware loaded */
            for (i=0;i<(sizeof(usb_devices_preup)/sizeof(struct usb_ident));i++) {

                if ((dev->descriptor.idVendor == usb_devices_preup[i].id_vendor) &&
                    (dev->descriptor.idProduct == usb_devices_preup[i].id_product)) {

                    /* Load the firmware. We don't really care if it fails or
                     * succeeds at this point, we'll see later. */
                    prv_usb_upload_firmware(dev);
                }
            }
        }
    }

    return ESTROK;
}

int prv_usb_upload_firmware(struct usb_device *dev)
{
    struct usb_dev_handle *usb_handle = NULL;
    int rc;
    int i;

    if (!dev)
        return ESTRINV;

    /* Open the device */
    usb_handle = usb_open(dev);
    if (usb_handle == NULL)
        return ESTRERR;

    /* Replay the captured USB traffic from the windows driver */
    for (i=0;i<(usb_preup_req_size/sizeof(estrella_usb_request_t));i++) {
        rc = usb_control_msg(
                usb_handle,
                usb_preup_req[i].requesttype | USB_TYPE_VENDOR, 
                usb_preup_req[i].request, 
                usb_preup_req[i].value, 
                usb_preup_req[i].index, 
                (char*)usb_preup_req[i].data, 
                usb_preup_req[i].size, 
                5000);

        if (rc < 0) {
            usb_close(usb_handle);
            return ESTRERR;
        }
    }    

    /* Close the device */
    usb_close(usb_handle);
    return ESTROK;
}

int prv_usb_find_devices(dll_list_t *devices)
{
    struct usb_bus *usb_bus = NULL;
    struct usb_device *dev = NULL;
    int rc, i;

    /* update device and bus information */
    usb_find_busses();
    usb_find_devices();

    /* Have a look at each device */
    for (usb_bus=usb_busses;usb_bus;usb_bus=usb_bus->next) {
        for (dev=usb_bus->devices;dev;dev=dev->next) {

            /* Check if the device is one of ours */
            for (i=0;i<(sizeof(usb_devices)/sizeof(struct usb_ident));i++) {

                if ((dev->descriptor.idVendor == usb_devices[i].id_vendor) &&
                    (dev->descriptor.idProduct == usb_devices[i].id_product)) {

                    void *tmpdev = NULL;
                    estrella_dev_t *newdev = NULL;

                    /* Create a new list entry for this device */
                    rc = dll_append(devices, &tmpdev, sizeof(estrella_dev_t));
                    if (rc != EDLLOK) {
                        dll_clear(devices);
                        return ESTRNOMEM;
                    }
        
                    newdev = (estrella_dev_t*)tmpdev;
        
                    /* Very basic device info */
                    newdev->devicetype = ESTRELLA_DEV_USB;
                    newdev->spec.usb.devnum = dev->devnum;
                    strncpy(newdev->spec.usb.bus, usb_bus->dirname, ESTRELLA_PATH_MAX);

                    /* Add some additional info from the device descriptor */
                    rc = prv_usb_device_info(dev, newdev);
                    if (rc != ESTROK) {
                        dll_clear(devices);
                        return ESTRERR;
                    }
                }
            }
        }
    }

    return ESTROK;
}

int estrella_usb_find_devices(dll_list_t *devices)
{
    int rc;

    /* Call usb_init, we don't know if the library has already been initialized */
    usb_init();
  
    /* Iterate all the busses and devices and in a first run try to find
     * uninitialized USB devices. If we find any we try to provide them with the
     * necessary firmware.*/
    rc = prv_usb_preup();
    if (rc != ESTROK)
        return ESTRERR;

    /* We need to wait a little for all the devices to reenumerate and show up
     * on the bus. This is not particularly nice but I don't really know what
     * else to do... apart from polling all busses for reenumerated devices
     * maybe */
    estrella_usleep(PRV_WAITFORDEVICE*1000, NULL);

    /* Correctly initialized devices now show up on the bus with a different
     * vendor/product ID. We're now going to search for those and return them */
    rc = prv_usb_find_devices(devices);
    if (rc != ESTROK)
        return ESTRERR;

    return 0;
}

int estrella_usb_init(estrella_session_t *session, estrella_dev_t *device)
{
    int rc;
    struct usb_dev_handle *handle;

    /* Data for the initializing control message */
    unsigned char estrella_init_req_data[] = {0x00,0x12,0x10,0x1f,0xe0,0x40}; 
    estrella_usb_request_t usb_init_req = {
        0x00,
        0xb4,
        0x0000,
        0x0000,
        sizeof(estrella_init_req_data),
        estrella_init_req_data,
    };

    /* First of all get the device handle */
    rc = prv_usb_get_handle(device, &handle);
    if (rc != ESTROK)
        return ESTRINV;

    /* The sniffed USB traffic from the windows driver shows that the host gets
     * the device and config descriptors before it selects a configuration and
     * claims an interface. Since there's obviously only one configuration and
     * one interface available to choose from we skip this step here. At least
     * that's the case for my device. 
     * TODO: This is probably necessary to handle other devices. */

    /* Select configuration 0x01 */
    rc = usb_set_configuration(handle, 0x01);
    if (rc < ESTROK) {
        return ESTRERR;
    }

    /* Claim interface 0x00 */
    rc = usb_claim_interface(handle, 0x00);
    if (rc < ESTROK) {
        return ESTRERR;
    }

    /* What follows now is another control transfer which presumably performs
     * initial device setup, obviously. */
    rc = usb_control_msg(
            handle,
            usb_init_req.requesttype | USB_TYPE_VENDOR, 
            usb_init_req.request, 
            usb_init_req.value, 
            usb_init_req.index, 
            (char*)usb_init_req.data, 
            usb_init_req.size, 
            5000);
    if (rc < 0) {
        return ESTRERR;
    }

    /* Store the handle inside this session */
    session->spec.usb_dev_handle = handle;

    return ESTROK;
}

int estrella_usb_rate(estrella_session_t *session, int rate, estr_xtrate_t xtrate)
{
    int rc;

    /* These are the control request and data which are being used to set the
     * integration time. From what I can see from the sniffed windows driver log
     * data[0] and data[1] hold the integration time in milliseconds. If this 
     * value is >= 5 then data[3] is decremented by 1 to 0x1f. Also we obviously 
     * can't supply values <= 1 (on _my_ device, it's supposed to work on
     * others). So, 2 ms integration time is the minimum. */
    unsigned char estrella_rate_req_data[] = {0x00,0x00,0x04,0x20,0xe0,0x40}; 
    estrella_usb_request_t usb_rate_req = {
        0x00,
        0xb4,
        0x0000,
        0x0000,
        sizeof(estrella_rate_req_data),
        estrella_rate_req_data,
    };

    if (session->spec.usb_dev_handle == NULL)
        return ESTRINV;

    /* For whatever reason we have to subtract 1 from estrella_init_req_data[3]
     * in case we want to use integration times >= 5 ms. */
    estrella_rate_req_data[1] = (unsigned char)(rate & 0xFF);
    estrella_rate_req_data[0] = (unsigned char)((rate >> 8) & 0xFF);

    if (rate >= 5)
        estrella_rate_req_data[3] -= 1;

    /* Adjust x timing resolution. It's either 0x04, 0x08 or 0x10 in data[2].
     * This is pretty much all I could figure out from the sniffed logs. There
     * must be more to it though, since the rate seems to be adjusted, too when
     * you select xtrate 1 or 2. I just can't seem to find out what's going on
     * there. */
    if (xtrate == ESTR_XRES_MEDIUM)
        estrella_rate_req_data[2] = 0x08;
    else if (xtrate == ESTR_XRES_HIGH) 
        estrella_rate_req_data[2] = 0x10;

    rc = usb_control_msg(
            session->spec.usb_dev_handle,
            usb_rate_req.requesttype | USB_TYPE_VENDOR, 
            usb_rate_req.request, 
            usb_rate_req.value, 
            usb_rate_req.index, 
            (char*)usb_rate_req.data, 
            usb_rate_req.size, 
            5000);
    if (rc < 0)
        return ESTRERR;

    return ESTROK;
}

int estrella_usb_scan_init(estrella_session_t *session)
{
    int rc;

    /* Scan start request */
    estrella_usb_request_t usb_scan_req = {
        0x00,
        0xb2,
        0x0000,
        0x0000,
        0,
        NULL,
    };

    if (session->spec.usb_dev_handle == NULL)
        return ESTRINV;

    /* Start the scan */
    rc = usb_control_msg(
            session->spec.usb_dev_handle,
            usb_scan_req.requesttype | USB_TYPE_VENDOR, 
            usb_scan_req.request, 
            usb_scan_req.value, 
            usb_scan_req.index, 
            (char*)usb_scan_req.data, 
            usb_scan_req.size, 
            5000);
    if (rc < 0)
        return ESTRERR;

    return ESTROK;
}


int estrella_usb_scan_result(estrella_session_t *session, float *buffer)
{
    int rc;
    int i;
    unsigned char response;
    unsigned char progress[2];
    unsigned char scanbuf[4096];
    estr_timestamp_t ts_start, ts_current;

    /* Data is being read from this endpoint adress */
    int endpoint_bulk_in = 0x88;

    /* Scan progress request. The windows driver sends this multiple times,
     * usually the device answers with [0xb3,0x00]. In case we receive
     * [0xb3,0x01] we need to initiate a bulk transfer to get the data from the
     * device. */
    estrella_usb_request_t usb_progress_req = {
        0x00,
        0xb3,
        0x0000,
        0x0000,
        sizeof(progress),
        progress,
    };

    /* Time at beginning of scan */
    rc = estrella_timestamp_get(&ts_start);
    if (rc != ESTROK)
        return ESTRERR;

    response = 0;
    while (1==1) {

        rc = usb_control_msg(
                session->spec.usb_dev_handle,
                usb_progress_req.requesttype | USB_TYPE_VENDOR | USB_ENDPOINT_IN, 
                usb_progress_req.request, 
                usb_progress_req.value, 
                usb_progress_req.index, 
                (char*)usb_progress_req.data, 
                usb_progress_req.size, 
                5000);
        if (rc < 0) {
            break;    
        }

        /* If we're in normal operation mode we eventually need to break with a
         * timeout. This is being checked here. */
        if (session->xtmode != ESTR_XTMODE_TRIGGER) {
            unsigned long mspassed;

            rc = estrella_timestamp_get(&ts_current);
            if (rc != ESTROK)
                return ESTRERR;

            rc = estrella_timestamp_diffms(&ts_start, &ts_current, &mspassed);
            if (rc != ESTROK)
                return ESTRERR;

            if (mspassed >= (session->rate + PRV_DELAY))
                break;
        }

        /* Either we're done scanning or we wait a bit to do the next request
         * for completion. */
        if (progress[1] == 0x01) {
            response = 1;
            break;
        } else {
            /* We don't care if this succeeds of fails, just go on to the next
             * ieration. */
            estrella_usleep(500, NULL);
        }
    }

    /* We did not get a valid response from the device. Return a timeout only in
     * normal operations mode */
    if (response != 1) {
        if (session->xtmode != ESTR_XTMODE_TRIGGER)
            return ESTRTIMEOUT;
        else 
            return ESTRERR;
    }
        
    /* Now get the data */
    rc = usb_bulk_read(
            session->spec.usb_dev_handle, 
            endpoint_bulk_in, 
            (char*)scanbuf, 
            sizeof(scanbuf), 
            4096);
    if (rc < 0)
        return ESTRERR;

    /* Put the data that we got into the supplied buffer. For all I can see
     * we're getting 2 bytes per value, which makes a total of 2048. I dont't
     * know why the original API requests a 2051 elements buffer. Here's what
     * they do anyway:
     * Leave out the first value from the result set, which leaves us with 2047
     * items. Those values are put into the result buffer from 0 to 2046. The
     * remaining indices 2047 to 2050 are simply set to 0.
     * No idea why it has to be a float buffer in the first place but well... */
    for (i=2; i<4096;i+=2) {
        unsigned short val = 0;
        val |= scanbuf[i+1];
        val = (val << 8);
        val |= scanbuf[i];

        buffer[(i-2)/2] = (float)val;
    }
    for (i=2047;i<2051;i++)
        buffer[i] = 0.0;

    return ESTROK;
}

int estrella_usb_close(estrella_session_t *session)
{
    /* Releasing the interface probably isn't necessary but it won't hurt either */
    if (session->spec.usb_dev_handle) {
        usb_release_interface(session->spec.usb_dev_handle, 0x00);
        usb_close(session->spec.usb_dev_handle);
    }

    return ESTROK;
}

