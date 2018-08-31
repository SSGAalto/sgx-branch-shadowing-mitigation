/*Authors: Hans Liljestrand and Shohreh Hosseinzadeh
Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
This code is released under Apache 2.0 and GPL 2.0 licenses.*/

#ifndef LBR_CHARDEV_H
#define LBR_CHARDEV_H

#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <asm/special_insns.h>
#include <linux/spinlock.h>
#include <linux/slab.h>

#include "lbr_tools.h"

#define DEVICE_NAME "lbr_dumper"

/*
 * The name of the device file
 */
#define DEVICE_FILE_NAME "lbr_dumper"

/* chardev.c */
int __init start_function(void);
void __exit end_function(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);
static long device_ioctl(struct file *, unsigned int, unsigned long);


/*
 * The major device number. We can't rely on dynamic
 * registration any more, because ioctls need to know
 * it.
 */
/* #define MAJOR_NUM 100 /1* FIXME: This probably isn't safe!!!! *1/ */

/*
 * Set the message of the device driver
 */
/* #define IOCTL_SET_MSG _IOR(MAJOR_NUM, 0, char *) */
/*
 * _IOR means that we're creating an ioctl command
 * number for passing information from a user process
 * to the kernel module.
 *
 * The first arguments, MAJOR_NUM, is the major device
 * number we're using.
 *
 * The second argument is the number of the command
 * (there could be several with different meanings).
 *
 * The third argument is the type we want to get from
 * the process to the kernel.
 */

/*
 * Get the message of the device driver
 */
/* #define IOCTL_GET_MSG _IOR(MAJOR_NUM, 1, char *) */
/*
 * This IOCTL is used for output, to get the message
 * of the device driver. However, we still need the
 * buffer to place the message in to be input,
 * as it is allocated by the process.
 */

/*
 * Get the n'th byte of the message
 */
/* #define IOCTL_GET_NTH_BYTE _IOWR(MAJOR_NUM, 2, int) */
/*
 * The IOCTL is used for both input and output. It
 * receives from the user a number, n, and returns
 * Message[n].
 */


#endif /* !LBR_CHARDEV_H */
