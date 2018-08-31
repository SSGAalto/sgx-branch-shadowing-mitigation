/*Authors: Hans Liljestrand and Shohreh Hosseinzadeh
Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
This code is released under Apache 2.0 and GPL 2.0 licenses.*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <asm/special_insns.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "lbr_dumper.h"
#include "lbr_tools.h"

MODULE_LICENSE("GPL")t ;
MODULE_AUTHOR("Anonymous");

static int major_num = -1;

/* Declare the character device callbacks */

struct file_operations Fops = {
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .compat_ioctl = device_ioctl,
    .open = device_open,
    .release = device_release,
};

static size_t lbr_data_index = 0;
static size_t lbr_data_size = sizeof(struct lbr_data) * lbr_count;
static struct lbr_data *lbr_data = NULL;

int __init start_function(void)
{
    major_num = register_chrdev(0, DEVICE_NAME, &Fops);
    if (major_num < 0) {
        printk(KERN_ALERT "Registering the device failed with %d\n", major_num);
        return major_num;
    }

    all_enable_LBR();

    lbr_data = kmalloc(lbr_data_size, GFP_KERNEL);

    if (lbr_data == NULL) {
        printk(KERN_INFO "Failed to dump LBR data, due to kmalloc fail\n");
    }

    printk(KERN_INFO "Registration okay, create device with\n");
    printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_FILE_NAME, major_num);

    return 0;
}

void __exit end_function(void)
{
    all_disable_LBR();
    if (lbr_data != NULL) {
        kfree(lbr_data);
    }
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "%s stopped\n", DEVICE_NAME);
}

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
    barrier();
    lbr_disable_inline();

    if (lbr_data != NULL)
        dump_LBR(lbr_data);
    lbr_data_index = 0;

    barrier();
    all_enable_LBR();

    return 0;
}

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t device_read(struct file *file, char __user * buffer,
        size_t length, loff_t * offset)
{
    int bytes_read = 0;

    if (lbr_data_index == lbr_data_size)
        lbr_data_index = 0;

    while (length && lbr_data_index < lbr_data_size) {
        put_user(((char *)lbr_data)[lbr_data_index++], buffer++);

        length--;
        bytes_read++;
    }

    return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user * buffer,
        size_t length, loff_t * offset)
{
    return 0;
}


module_init(start_function);
module_exit(end_function);

/* Ugly hack for KBuild changes, or something */
#include "lbr_tools.c"
