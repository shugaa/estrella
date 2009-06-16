/*
 * USB2EPP driver version 0.1
 *
 * Copyright (C) 2009 Bjoern Rehm (bjoern@shugaa.de)
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License as
 *      published by the Free Software Foundation, version 2.
 *
 * This code is loosely based on Greg Kroah-Hartman's USB skeleton sample
 * driver.
 *
 * You will need to have the appropriate firmware loaded in order to use the
 * device.
 *
 * There is no averaging, smoothing or temperature compensation performed by
 * this driver yet. The user space 'estrella' implementation offers a few more
 * features at that point.
 *
 * The device is configured through a hand full of ioctl commands. Those are
 * quite self explanatory, have a look at the usb2epp_ioctl() function for
 * details.
 *
 * Measurement results are read directly from the device node in devfs. this
 * should return the usual 2051*sizeof(float) number of bytes for a single
 * measurement. Non-blocking I/O is supported as well, in which case you will
 * get -EBUSY for scans in progress.
 *
 */

#include "usb2epp.h"

/*
 * Types and defines    
 */

/* Device and product IDs for the USB2EPP devices */
#define USB2EPP_VENDOR_ID               (0x0bd7)
#define USB2EPP_PRODUCT_ID              (0xa012)

#ifdef CONFIG_USB_DYNAMIC_MINORS
#define USB2EPP_MINOR_BASE              (0)
#else
/* SKELETON_MINOR_BASE 192 + 32, not offical */
#define USB2EPP_MINOR_BASE              (224)     
#endif

/* Request and endpoint types */
#define USB2EPP_REQ_SETUP               (0xb4)
#define USB2EPP_REQ_STATUS              (0xb3)
#define USB2EPP_REQ_SCAN                (0xb2)
#define USB2EPP_BULK_IN_ENDPOINT        (0x88)

/* This is how many bytes we get from the device for a single scan */
#define USB2EPP_BULK_IN_SIZE            (4096)

#define to_usb2epp_dev(d) container_of(d, struct usb_usb2epp, kref)

/* Device states */
typedef enum {
        USB2EPP_STATE_IDLE      =       (0),
        USB2EPP_STATE_SCANNING,
        USB2EPP_STATE_TYPES,
} usb2epp_state_t;

/* Structure to hold all of our device specific stuff */
struct usb_usb2epp {
        /*
         * Device and locking info
         */
        struct usb_device       *udev;                  /* the usb device for this device */
        struct usb_interface    *interface;             /* the interface for this device */
        unsigned char           *bulk_in_buffer;        /* BULK_IN_SIZE bytes for the measurement results */
        int                     *result_buffer;         /* result buffer */
        size_t                  bulk_in_size;           /* the size of the receive buffer */
        int                     errors;                 /* the last request tanked */
        int                     open_count;             /* count the number of openers */
        struct kref             kref;                   /* object reference counter */
        struct mutex            io_mutex;               /* synchronize I/O */

        /*
         * Session data
         */
        usb2epp_state_t         state;                  /* Device state */
        int                     rate;                   /* Integration time */
        usb2epp_xtrate_t        xtrate;                 /* Resolution */
        usb2epp_xtmode_t        xtmode;                 /* External trigger mode */
        usb2epp_xsmooth_t       xsmooth;                /* Smoothing */
        usb2epp_tempcomp_t      tempcomp;               /* Temperature compensation */
        int                     scanstoavg;             /* Scans to average */
};

/*
 * Private module interface      
 */

/* "User" interface */
static void usb2epp_delete(struct kref *kref);
static int usb2epp_open(struct inode *inode, struct file *file);
static int usb2epp_release(struct inode *inode, struct file *file);
static int usb2epp_flush(struct file *file, fl_owner_t id);
static ssize_t usb2epp_read(struct file *file, char *buffer, size_t count, loff_t *ppos);
static ssize_t usb2epp_write(struct file *file, const char *user_buffer, size_t count, loff_t *ppos);
static long usb2epp_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* Kernel interface */
static int usb2epp_probe(struct usb_interface *interface, const struct usb_device_id *id);
static void usb2epp_disconnect(struct usb_interface *interface);
static int usb2epp_suspend(struct usb_interface *intf, pm_message_t message);
static int usb2epp_resume (struct usb_interface *intf);
static int usb2epp_pre_reset(struct usb_interface *intf);
static int usb2epp_post_reset(struct usb_interface *intf);

/* Driver utility functions */
static int usb2epp_scan_setup(struct usb_usb2epp *dev);
static int usb2epp_scan_start(struct usb_usb2epp *dev);
static int usb2epp_scan_iscomplete(struct usb_usb2epp *dev);

/* Safe setter functions for user data */
static int usb2epp_session_set_rate(struct usb_usb2epp *dev, int rate);
static int usb2epp_session_set_xtrate(struct usb_usb2epp *dev, int xtrate);
static int usb2epp_session_set_xsmooth(struct usb_usb2epp *dev, int xsmooth);
static int usb2epp_session_set_xtmode(struct usb_usb2epp *dev, int xtmode);
static int usb2epp_session_set_scanstoavg(struct usb_usb2epp *dev, int scanstoavg);

/* Module init and exit */
static int __init usb_usb2epp_init(void);
static void __exit usb_usb2epp_exit(void);

/*
 * Module globals   
 */

/* This driver is responsible for the following devices */
static struct usb_device_id usb2epp_table [] = {
        { USB_DEVICE(USB2EPP_VENDOR_ID, USB2EPP_PRODUCT_ID) },
        { },
};

/* File operations */
static const struct file_operations usb2epp_fops = {
        .owner =                THIS_MODULE,
        .read =                 usb2epp_read,
        .write =                usb2epp_write,
        .open =                 usb2epp_open,
        .release =              usb2epp_release,
        .flush =                usb2epp_flush,
        .unlocked_ioctl =       usb2epp_ioctl,
};

/* USB2EPP device class */
static struct usb_class_driver usb2epp_class = {
        .name =                 "usb2epp%d",
        .fops =                 &usb2epp_fops,
        .minor_base =           USB2EPP_MINOR_BASE,
};

/* Driver instance */
static struct usb_driver usb2epp_driver = {
        .name =                 "usb2epp",
        .probe =                usb2epp_probe,
        .disconnect =           usb2epp_disconnect,
        .suspend =              usb2epp_suspend,
        .resume =               usb2epp_resume,
        .pre_reset =            usb2epp_pre_reset,
        .post_reset =           usb2epp_post_reset,
        .id_table =             usb2epp_table,
        .supports_autosuspend = 1,
};

/*
 * Implementation  
 */

static void usb2epp_delete(struct kref *kref)
{
        struct usb_usb2epp *dev = to_usb2epp_dev(kref);

        usb_put_dev(dev->udev);

        if (dev->bulk_in_buffer)
                kfree(dev->bulk_in_buffer);
        if (dev->result_buffer)
                kfree(dev->result_buffer);
                
        kfree(dev);
}

static int usb2epp_open(struct inode *inode, struct file *file)
{
        struct usb_usb2epp *dev = NULL;
        struct usb_interface *interface = NULL;
        int subminor;
        int rc = 0;

        subminor = iminor(inode);

        /* Find the interface corresponding to subminor */
        interface = usb_find_interface(&usb2epp_driver, subminor);
        if (!interface) {
                err ("%s - error, can't find device for minor %d",
                     __func__, subminor);
                rc = -ENODEV;
                goto exit;
        }

        /* Get the device instance */
        dev = usb_get_intfdata(interface);
        if (!dev) {
                rc = -ENODEV;
                goto exit;
        }

        /* Increment our usage count for the device and acquire the I/O lock */
        kref_get(&dev->kref);
        mutex_lock(&dev->io_mutex);

        /* Access is exclusive */
        if (dev->open_count >= 1) {
                rc = -EBUSY;
                mutex_unlock(&dev->io_mutex);
                kref_put(&dev->kref, usb2epp_delete);
                goto exit;
        }

        /* Prevent autosuspend */
        rc = usb_autopm_get_interface(interface);
        if (rc != 0) {
                mutex_unlock(&dev->io_mutex);
                kref_put(&dev->kref, usb2epp_delete);
                goto exit;
        }

        /* Everything went well, increase the open counter, store the reference
         * to our device instance and unlock I/O */
        dev->open_count++;
        file->private_data = dev;
        mutex_unlock(&dev->io_mutex);

exit:
        return rc;
}

static long usb2epp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        int rc = 0;
        struct usb_usb2epp *dev;

        /* Get the device */
        dev = (struct usb_usb2epp*)file->private_data;
        if (dev == NULL)
                return -ENODEV;

        /* Lock I/O */
        mutex_lock(&dev->io_mutex);

        if (!dev->interface) {          /* disconnect() was called */
                rc = -ENODEV;
                goto exit;
        }

        /* Make sure we're not scanning right now */
        if (dev->state != USB2EPP_STATE_IDLE) {
                rc = -EBUSY;
                goto exit;
        }

        /* See what we got... */
        switch (cmd) {
                case USB2EPP_IOCTL_RATE:
                        rc = usb2epp_session_set_rate(dev, (int)arg);
                        if (rc == 0)
                                rc = usb2epp_scan_setup(dev);
                        break;
                case USB2EPP_IOCTL_XTRATE:
                        rc = usb2epp_session_set_xtrate(dev, (int)arg);
                        if (rc == 0)
                                rc = usb2epp_scan_setup(dev);
                        break;
                case USB2EPP_IOCTL_XTMODE:
                        rc = usb2epp_session_set_xtmode(dev, (int)arg);
                        break;
                case USB2EPP_IOCTL_XSMOOTH:
                        rc = usb2epp_session_set_xsmooth(dev, (int)arg);
                        break;
                case USB2EPP_IOCTL_SCANSTOAVG:
                        rc = usb2epp_session_set_scanstoavg(dev, (int)arg);
                        break;
                default:
                        rc = -EINVAL;
                        break;
        }

exit:
        /* Return our I/O lock and return */
        mutex_unlock(&dev->io_mutex);
        return rc;
}

static int usb2epp_release(struct inode *inode, struct file *file)
{
        struct usb_usb2epp *dev;

        dev = (struct usb_usb2epp*)file->private_data;
        if (dev == NULL)
                return -ENODEV;

        /* Allow the device to be autosuspended */
        mutex_lock(&dev->io_mutex);
        if (!--dev->open_count && dev->interface)
                usb_autopm_put_interface(dev->interface);
        mutex_unlock(&dev->io_mutex);

        /* Decrement the count on our device */
        kref_put(&dev->kref, usb2epp_delete);
        return 0;
}

static int usb2epp_flush(struct file *file, fl_owner_t id)
{
        struct usb_usb2epp *dev;
        int res;

        dev = (struct usb_usb2epp*)file->private_data;
        if (dev == NULL)
                return -ENODEV;

        /* wait for io to stop */
        mutex_lock(&dev->io_mutex);

        /* read out errors, leave subsequent opens a clean slate */
        res = dev->errors ? (dev->errors == -EPIPE ? -EPIPE : -EIO) : 0;
        dev->errors = 0;

        mutex_unlock(&dev->io_mutex);

        return res;
}

static ssize_t usb2epp_read(struct file *file, char *buffer, size_t count, loff_t *ppos)
{
        int rc;
        struct usb_usb2epp *dev;
        int bytes_read, bytes_total;
        int i;

        /* Get our session and lock I/O */
        dev = (struct usb_usb2epp*)file->private_data;
        mutex_lock(&dev->io_mutex);

        /* disconnect() was called */
        if (!dev->interface) { 
                rc = -ENODEV;
                goto exit;
        }

        /* Start a scan if we're currently idle */
        if (dev->state == USB2EPP_STATE_IDLE) {
                rc = usb2epp_scan_start(dev);

                /* Exit on error */
                if (rc != 0)
                        goto exit;

                /* Exit if non blocking I/O has been requested */
                if ((file->f_flags & O_NONBLOCK) > 0)
                        goto exit;

                dev->state = USB2EPP_STATE_SCANNING;
        }

        /* Check for completion of the scan. If we're in blocking I/O mode we busy loop here. This
         * might not be a very good idea. */
        rc = 0;
        for(;;) {
                /* Exit the loop if complete */
                rc = usb2epp_scan_iscomplete(dev);
                if (rc == 0)
                        break;

                /* Perform only a single query in non-blocking I/O mode */
                if ((file->f_flags & O_NONBLOCK) > 0) {
                        rc = -EBUSY; 
                        break;
                }
        }

        /* Scan is not complete */
        if (rc != 0)
                goto exit;

        /* We'll now try to get USB2EPP_BULK_IN_SIZE bytes from the device */
        bytes_total = 0;
        do {
                /* do a blocking bulk read to get data from the device */
                rc = usb_bulk_msg(dev->udev,
                                usb_rcvbulkpipe(dev->udev, USB2EPP_BULK_IN_ENDPOINT),
                                (void*)&dev->bulk_in_buffer[bytes_total],
                                min(dev->bulk_in_size, (size_t)USB2EPP_BULK_IN_SIZE),
                                &bytes_read, 5000);
                if (rc < 0)
                        break;

                bytes_total += bytes_read;
        } while (bytes_total < USB2EPP_BULK_IN_SIZE); 

        /* usb_bulk_msg() screwed up */
        if (rc < 0)
                goto exit;

        /* We got the scan results, return to idle state */
        dev->state = USB2EPP_STATE_IDLE;

        /* Fill the buffer and send back */
        for (i=2; i<4096;i+=2) {
                unsigned int val = 0;
                val |= dev->bulk_in_buffer[i+1];
                val = (val << 8);
                val |= dev->bulk_in_buffer[i];

                dev->result_buffer[(i-2)/2] = (int)val;
        }
        for (i=2047;i<2051;i++)
                dev->result_buffer[i] = 0.0;

        /* Copy the data to userspace */
        rc = copy_to_user(buffer, (const void *)dev->result_buffer, 2051*sizeof(int));

        /* return either an error code or the number ob bytes copied */
        if (rc != 0)
                rc = -EFAULT;
        else
                rc = 2051*sizeof(int);
exit:
        /* Release our I/O lock and return */
        mutex_unlock(&dev->io_mutex);
        return rc;
}

static ssize_t usb2epp_write(struct file *file, const char *user_buffer, size_t count, loff_t *ppos)
{
        /* There's no data to be transferred _to_ the device really */
        return 0;
}

static int usb2epp_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
        struct usb_usb2epp *dev = NULL;
        struct usb_host_interface *iface_desc = NULL;
        struct usb_endpoint_descriptor *endpoint = NULL;
        int i;
        int rc = -ENOMEM;

        /* Allocate memory for our device state and initialize it */
        dev = kzalloc(sizeof(*dev), GFP_KERNEL);
        if (!dev) {
                err("Out of memory");
                goto error;
        }

        /* Init the reference counter and I/O lock */
        kref_init(&dev->kref);
        mutex_init(&dev->io_mutex);

        /* Attach this driver to the device */
        dev->udev = usb_get_dev(interface_to_usbdev(interface));
        dev->interface = interface;

        /* Go search for endpoint 0x88 */
        iface_desc = interface->cur_altsetting;
        for (i=0; i<iface_desc->desc.bNumEndpoints; i++) {
                endpoint = &iface_desc->endpoint[i].desc;

                /* Not a bulk endpoint */
                if (!usb_endpoint_is_bulk_in(endpoint))
                        continue;
                /* Not our data endpoint */
                if (endpoint->bEndpointAddress != USB2EPP_BULK_IN_ENDPOINT)
                        continue;

                /* USB2EPP_BULK_IN_ENDPOINT does exist on this interface */
                break;
        }

        /* Bulk endpoint not found */
        if (i == iface_desc->desc.bNumEndpoints)
                goto error;
                
        /* Remember the maximum bulk packet size */
        dev->bulk_in_size = le16_to_cpu(endpoint->wMaxPacketSize);

        dev->bulk_in_buffer = NULL;
        dev->result_buffer = NULL;
        
        /* Allocate buffers for raw result data and the return values */
        dev->bulk_in_buffer = kmalloc(USB2EPP_BULK_IN_SIZE, GFP_KERNEL);
        if (!dev->bulk_in_buffer) {
                err("Could not allocate bulk_in_buffer");
                goto error;
        }
        dev->result_buffer = (int*)kmalloc(2051*sizeof(int), GFP_KERNEL);
        if (!dev->result_buffer) {
                err("Could not allocate bulk_in_buffer");
                goto error;
        }

        /* Save our data pointer in this interface device */
        usb_set_intfdata(interface, dev);

        /* we can register the device now, as it is ready */
        rc = usb_register_dev(interface, &usb2epp_class);
        if (rc != 0) {
                /* something prevented us from registering this driver */
                err("Not able to get a minor for this device.");
                usb_set_intfdata(interface, NULL);
                goto error;
        }

        /* Initial session setup */
        dev->xtrate = USB2EPP_XTRATE_HIGH;
        dev->rate = 18;
        dev->scanstoavg = 1;
        dev->xsmooth = USB2EPP_XSMOOTH_NONE;
        dev->tempcomp = USB2EPP_TEMPCOMP_OFF;
        dev->xtmode = USB2EPP_XTMODE_NORMAL;

        /* We're idle initially */
        dev->state = USB2EPP_STATE_IDLE; 

        /* TODO: Eventually configure the device here initially */

        /* let the user know what node this device is now attached to */
        info("USB2EPP device now attached to usb2epp%d", interface->minor);

        /* Return success */
        return 0;

error:
        if (dev != NULL)
                kref_put(&dev->kref, usb2epp_delete);

        return rc;
}

static void usb2epp_disconnect(struct usb_interface *interface)
{
        struct usb_usb2epp *dev;
        int minor = interface->minor;

        /* Save our session before the interface is destroyed */
        dev = usb_get_intfdata(interface);
        usb_set_intfdata(interface, NULL);

        /* give back our minor */
        usb_deregister_dev(interface, &usb2epp_class);

        /* prevent more I/O from starting */
        mutex_lock(&dev->io_mutex);
        dev->interface = NULL;
        mutex_unlock(&dev->io_mutex);

        /* decrement our usage count */
        kref_put(&dev->kref, usb2epp_delete);

        info("USB2EPP #%d now disconnected", minor);
}

static int usb2epp_suspend(struct usb_interface *intf, pm_message_t message)
{
        return 0;
}

static int usb2epp_resume(struct usb_interface *intf)
{
        return 0;
}

static int usb2epp_pre_reset(struct usb_interface *intf)
{
        struct usb_usb2epp *dev = usb_get_intfdata(intf);

        mutex_lock(&dev->io_mutex);

        return 0;
}

static int usb2epp_post_reset(struct usb_interface *intf)
{
        struct usb_usb2epp *dev = usb_get_intfdata(intf);

        /* we are sure no URBs are active - no locking needed */
        dev->errors = -EPIPE;

        /* Back to idle state after reset */
        dev->state = USB2EPP_STATE_IDLE;

        mutex_unlock(&dev->io_mutex);

        return 0;
}

static int __init usb_usb2epp_init(void)
{
        int rc;

        /* register this driver with the USB subsystem */
        rc = usb_register(&usb2epp_driver);
        if (rc != 0)
                err("usb_register failed. Error number %d", rc);

        return rc;
}

static void __exit usb_usb2epp_exit(void)
{
        /* deregister this driver with the USB subsystem */
        usb_deregister(&usb2epp_driver);
}

static int usb2epp_session_set_rate(struct usb_usb2epp *dev, int rate)
{
        if ((rate < 2) || (rate > 65500))
                return -EINVAL;

        dev->rate = rate;

        return 0;
}

static int usb2epp_session_set_xtrate(struct usb_usb2epp *dev, int xtrate)
{
        if ((xtrate < 0) || (xtrate >= USB2EPP_XTRATE_TYPES))
                return -EINVAL;

        dev->xtrate = xtrate;

        return 0;
}

static int usb2epp_session_set_xsmooth(struct usb_usb2epp *dev, int xsmooth)
{
        if ((xsmooth < 0) || (xsmooth >= USB2EPP_XSMOOTH_TYPES))
                return -EINVAL;

        dev->xsmooth = xsmooth;

        return 0;
}

static int usb2epp_session_set_xtmode(struct usb_usb2epp *dev, int xtmode)
{
        if ((xtmode < 0) || (xtmode >= USB2EPP_XTMODE_TYPES))
                return -EINVAL;
        
        dev->xtmode = xtmode;

        return 0;
}

static int usb2epp_session_set_scanstoavg(struct usb_usb2epp *dev, int scanstoavg)
{
        if (scanstoavg <= 0)
                return -EINVAL;

        return 0;
}

static int usb2epp_scan_start(struct usb_usb2epp *dev)
{
        int rc; 

        rc = (int)usb_control_msg(dev->udev,
	        usb_sndctrlpipe(dev->udev, 0),
		USB2EPP_REQ_SCAN,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0x0000,
		0x0000, 
                NULL,
                0,
		5000);

        if (rc < 0)
                rc = -EIO;
        else
                rc = 0;

        return rc;
}

static int usb2epp_scan_iscomplete(struct usb_usb2epp *dev)
{
        int rc; 
        unsigned char response[2];

        rc = (int)usb_control_msg(dev->udev,
	        usb_rcvctrlpipe(dev->udev, 0),
		USB2EPP_REQ_STATUS,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0x0000,
		0x0000, 
                response,
                sizeof(response),
		5000);

        if (rc < 0) {
                rc = -EIO;
                goto exit;
        }

        if (response[1] == 0x01)
                rc = 0;
        else
                rc = -EBUSY;

exit:
        return rc;
}

static int usb2epp_scan_setup(struct usb_usb2epp *dev)
{
        int rc;
 
        /* 
         * These are the control request and data which are being used to set the
         * integration time. From what I can see from the sniffed windows driver log
         * data[0] and data[1] hold the integration time in milliseconds. If this
         * value is >= 5 then data[3] is decremented by 1 to 0x1f. Also we obviously
         * can't supply values <= 1 (on _my_ device, it's supposed to work on
         * others). So, 2 ms integration time is the minimum. 
         */
        unsigned char controlword[] = {0x00,0x00,0x04,0x20,0xe0,0x40};
    
        /* 
         * For whatever reason we have to subtract 1 from
         * estrella_init_req_data[3] in case we want to use integration times >=
         * 5 ms. 
         */
        controlword[1] = (unsigned char)(dev->rate & 0xFF);
        controlword[0] = (unsigned char)((dev->rate >> 8) & 0xFF);

        if (dev->rate >= 5)
                controlword[3] -= 1;
 
        /* 
         * Adjust x timing resolution. It's either 0x04, 0x08 or 0x10 in data[2].
         * This is pretty much all I could figure out from the sniffed logs. There
         * must be more to it though, since the rate seems to be adjusted, too when
         * you select xtrate 1 or 2. I just can't seem to find out what's going on
         * there. Actually the rate adjustment is very much linear with a slope of
         * about 0.7 for medium and 0.6 for high resolution. Don't now if we should
         * follow suit on this one. It does not really seem necessary anyway. 
         */
        if (dev->xtrate == USB2EPP_XTRATE_MEDIUM)
                controlword[2] = 0x08;
        else if (dev->xtrate == USB2EPP_XTRATE_HIGH)
                controlword[2] = 0x10;

        rc = (int)usb_control_msg(dev->udev,
	        usb_sndctrlpipe(dev->udev, 0),
		USB2EPP_REQ_SETUP,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		0x0000,
		0x0000, 
                controlword,
                sizeof(controlword),
		5000);

        if (rc != (int)sizeof(controlword))
                rc = -EIO;
        else
                rc = 0;

        return rc;
}

MODULE_DEVICE_TABLE(usb, usb2epp_table);
MODULE_LICENSE("GPL");

module_init(usb_usb2epp_init);
module_exit(usb_usb2epp_exit);

