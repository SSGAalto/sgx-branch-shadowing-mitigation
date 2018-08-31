/*Authors: Hans Liljestrand and Shohreh Hosseinzadeh
Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
This code is released under Apache 2.0 and GPL 2.0 licenses.*/

#include "lbr_tools.h"

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

static struct task_struct *ts1;
static struct task_struct *ts2;
static struct task_struct *ts3;
static struct task_struct *ts4;

static DEFINE_SPINLOCK(printer);

int enable_LBR(void *d) {
    lbr_enable_inline();
    lbr_filter_inline(0, 1);
    /* printk(KERN_INFO "LBR enalbed and fileterd on cpu %d\n", smp_processor_id()); */
    return 0;
}

int disable_LBR(void *d) {
    lbr_disable_inline();
    lbr_filter_inline(0, 0);
    /* printk(KERN_INFO "LBR disabled on cpu %d\n", smp_processor_id()); */
    return 0;
}

int dump_LBR(struct lbr_data *lbr_data)
{
    unsigned long flags;
    int i;

    read_lbr(lbr_data);

    spin_lock_irqsave(&printer, flags);
    printk(KERN_INFO "<cpu%d>\n", smp_processor_id());

    for (i = 0; i < lbr_count; i++) {
#ifdef LBR_VERBOSE_PRINT
        printk(KERN_INFO
                "0x%016lx -> 0x%016lx [%s] (cycles: %u) %08x %08x %08x %08x\n",
                lbr_data_get_from(&lbr_data[i]),
                lbr_data_get_to(&lbr_data[i]),
                lbr_data_get_mispred(&lbr_data[i]) != 0 ? "MISS" : "____",
                lbr_data_get_cycle_count(&lbr_data[i]),
                lbr_data[i].dxf,
                lbr_data[i].axf,
                lbr_data[i].dxt,
                lbr_data[i].axt
                );
#else
        printk(KERN_INFO "0x%016lx -> 0x%016lx [%s] cycles: %u\n",
                lbr_data_get_from(&lbr_data[i]),
                lbr_data_get_to(&lbr_data[i]),
                lbr_data_get_mispred(&lbr_data[i]) != 0 ? "MISS" : "____",
                lbr_data_get_cycle_count(&lbr_data[i])
                );
#endif /* LBR_VERBOSE_PRINT */
    }

    printk(KERN_INFO "</cpu%d>\n", smp_processor_id());
    spin_unlock_irqrestore(&printer, flags);

    return 2;
}

void all_disable_LBR(void)
{
    ts1=kthread_create(disable_LBR,NULL,"KTH1");
    kthread_bind(ts1,0);
    ts2=kthread_create(disable_LBR,NULL,"KTH2");
    kthread_bind(ts2,1);
    ts3=kthread_create(disable_LBR,NULL,"KTH3");
    kthread_bind(ts3,2);
    ts4=kthread_create(disable_LBR,NULL,"KTH4");
    kthread_bind(ts4,3);

    if (!IS_ERR(ts1) && !IS_ERR(ts2) && !IS_ERR(ts3) && !IS_ERR(ts4)) {
        wake_up_process(ts1);
        wake_up_process(ts2);
        wake_up_process(ts3);
        wake_up_process(ts4);
    } else {
        printk(KERN_INFO "Failed to bind thread to CPUn");
    }
}

void all_enable_LBR(void)
{
    ts1=kthread_create(enable_LBR,NULL,"KTH1");
    kthread_bind(ts1,0);
    ts2=kthread_create(enable_LBR,NULL,"KTH2");
    kthread_bind(ts2,1);
    ts3=kthread_create(enable_LBR,NULL,"KTH3");
    kthread_bind(ts3,2);
    ts4=kthread_create(enable_LBR,NULL,"KTH4");
    kthread_bind(ts4,3);

    if (!IS_ERR(ts1) && !IS_ERR(ts2) && !IS_ERR(ts3) && !IS_ERR(ts4)) {
        wake_up_process(ts1);
        wake_up_process(ts2);
        wake_up_process(ts3);
        wake_up_process(ts4);
    } else {
        printk(KERN_INFO "Failed to bind thread to CPUn");
    }
}
