/*
 * USB2EPP driver version 0.1
 *
 * Copyright (C) 2009 Bjoern Rehm (bjoern@shugaa.de)
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License as
 *      published by the Free Software Foundation, version 2.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <asm/uaccess.h>
#include <linux/usb.h>
#include <linux/mutex.h>

/* Supported ioctls */
enum usb2epp_ioctls {
        USB2EPP_IOCTL_RATE      = (0),
        USB2EPP_IOCTL_XTRATE,
        USB2EPP_IOCTL_XTMODE,
        USB2EPP_IOCTL_XSMOOTH,
        USB2EPP_IOCTL_SCANSTOAVG,
        USB2EPP_IOCTL_TYPES,
};

/* USB2EPP operation modes */
typedef enum {
        USB2EPP_XTMODE_NORMAL   = (0),
        USB2EPP_XTMODE_TRIGGER,
        USB2EPP_XTMODE_TYPES,
} usb2epp_xtmode_t;
 
/* USB2EPP xsmoothing setting */
typedef enum {
        USB2EPP_XSMOOTH_NONE    = (0),
        USB2EPP_XSMOOTH_5PX,
        USB2EPP_XSMOOTH_9PX,
        USB2EPP_XSMOOTH_17PX,
        USB2EPP_XSMOOTH_33PX,
        USB2EPP_XSMOOTH_TYPES,
} usb2epp_xsmooth_t;
 
/* USB2EPP temperature compensation */
typedef enum {
        USB2EPP_TEMPCOMP_OFF    = (0),
        USB2EPP_TEMPCOMP_ON,
        USB2EPP_TEMPCOMP_TYPES,
} usb2epp_tempcomp_t;
 
/* Xtiming resolution parameters */
typedef enum {
        USB2EPP_XTRATE_LOW      = (0),
        USB2EPP_XTRATE_MEDIUM,
        USB2EPP_XTRATE_HIGH,
        USB2EPP_XTRATE_TYPES,
} usb2epp_xtrate_t;

