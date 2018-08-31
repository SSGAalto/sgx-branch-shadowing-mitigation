/*Authors: Hans Liljestrand and Shohreh Hosseinzadeh
Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
This code is released under Apache 2.0 and GPL 2.0 licenses.*/

#ifndef PERF_ATTR_H
#define PERF_ATTR_H

#include <string.h>
#include <linux/perf_event.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/syscall.h>
#include <unistd.h>

#include "attributes.h"

/* %edi needs to have the fd for event, will mess %rcx and %esi */
#define enable_branch_misses_str ""                     \
    "mov %%rax, %%r15\n\t"                              \
    "mov %%rcx, %%r14\n\t"                              \
    "mov $0x2400, %%esi\n\t"                            \
    "mov $0x10, %%eax\n\t"                              \
    "syscall\n\t"                                       \
    "mov %%r15, %%rax\n\t"                              \
    "mov %%r14, %%rcx\n\t"

/* %edi needs to have the fd for event, will mess %rcx and %esi */
#define disable_branch_misses_str ""                    \
    "mov %%rax, %%r15\n\t"                              \
    "mov %%rcx, %%r14\n\t"                              \
    "mov $0x2401, %%esi\n\t"                            \
    "mov $0x10, %%eax\n\t"                              \
    "syscall\n\t"                                       \
    "mov %%r15, %%rax\n\t"                              \
    "mov %%r14, %%rcx\n\t"

#define branch_misses_asm_clobbers "%rax", "%r15", "%esi", "%edi"

#define enable_branch_misses_hex ""									\
	"\x49\x89\xc7"         /* mov    %rax,%r15 */					\
	"\x49\x89\xce"         /* mov    %rcx,%r14 */					\
	"\xbe\x00\x24\x00\x00" /* mov    $0x2400,%esi */    			\
	"\xb8\x10\x00\x00\x00" /* mov    $0x10,%eax */      			\
	"\x0f\x05"             /* syscall (clobbers %rax, rcx, r15) */	\
	"\x4c\x89\xf1"         /* mov    %r14,%rcx */                   \
	"\x4c\x89\xf8"         /* mov    %r15,%rax */

#define disable_branch_misses_hex ""								\
	"\x49\x89\xc7"         /* mov    %rax,%r15 */					\
	"\x49\x89\xce"         /* mov    %rcx,%r14 */					\
	"\xbe\x01\x24\x00\x00" /* mov    $0x2401,%esi */    			\
	"\xb8\x10\x00\x00\x00" /* mov    $0x10,%eax */      			\
	"\x0f\x05"             /* syscall (clobbers %rax, rcx, r15) */	\
	"\x4c\x89\xf1"         /* mov    %r14,%rcx */                   \
	"\x4c\x89\xf8"         /* mov    %r15,%rax */

#define enable_branch_misses_hex_size (sizeof(enable_branch_misses_hex) - 1)
#define disable_branch_misses_hex_size (sizeof(disable_branch_misses_hex) - 1)

#ifdef __cplusplus
extern "C" {
#endif

always_inline
inline static void
pe_disable(const int fd)
{
    __asm__ volatile(""
            "mov %[Action], %%esi\n\t"
            "mov %[Fd], %%edi\n\t"
            "mov $0x10, %%eax\n\t"
            "syscall\n\t"
            : :
            [Action] "g" (PERF_EVENT_IOC_DISABLE),
            [Fd] "m" (fd)
            : );
}

always_inline
inline static void
pe_enable(const int fd)
{
    __asm__ volatile(""
            "mov %[Action], %%esi\n\t"
            "mov %[Fd], %%edi\n\t"
            "mov $0x10, %%eax\n\t"
            "syscall\n\t"
            : :
            [Action] "g" (PERF_EVENT_IOC_ENABLE),
            [Fd] "m" (fd)
            : );
}

always_inline
inline static void
pe_reset(const int fd)
{
    __asm__ volatile(""
            "mov %[Action], %%esi\n\t"
            "mov %[Fd], %%edi\n\t"
            "mov $0x10, %%eax\n\t"
            "syscall\n\t"
            : :
            [Action] "g" (PERF_EVENT_IOC_RESET),
            [Fd] "m" (fd)
            : );
}

always_inline inline static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
        int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
            group_fd, flags);

    return ret;
}

always_inline
inline static long
pe_read(const int fd)
{
    long long retval;
    read(fd, &retval, sizeof(long long));
    return (long)retval;
}

always_inline
inline static int
pe_read_and_open(struct perf_event_attr *pe, int pe_config)
{
    memset(pe, 0, sizeof(struct perf_event_attr));
    pe->type = PERF_TYPE_HARDWARE;
    pe->size = sizeof(struct perf_event_attr);
    pe->config = pe_config;
    pe->disabled = 1;
    pe->exclude_kernel = 1;
    pe->exclude_hv = 1;

    return perf_event_open(pe, 0, -1, -1, 0);
}

#ifdef __cplusplus
}
#endif
#endif /* !PERF_ATTR_H */
