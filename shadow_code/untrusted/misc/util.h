/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_UTIL_H
#define SAMPLE_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define lotsa_nops "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t" \
    "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"

#define do_inline_ioctl(fd, action) { \
asm (""                    \
    "mov %[Action], %%esi\n\t" \
    "mov %[Fd], %%edi\n\t"     \
    "mov $0x10, %%eax\n\t"     \
    "syscall\n\t"              \
    : :                        \
    [Action] "r" (action),     \
    [Fd] "r" (fd)              \
    : "%edi", "%eax" "%esi");  \
}

/** Get currently running processor id */
int smp_processor_id(void);

/** Pass in argv[0] to change cwd to wherever the calling binary is located */
void chwd_to_binary(char *);

#ifdef __cplusplus
}
#endif

#endif //SAMPLE_UTIL_H
