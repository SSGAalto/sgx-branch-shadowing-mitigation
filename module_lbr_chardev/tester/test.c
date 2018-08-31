/*Authors: Hans Liljestrand and Shohreh Hosseinzadeh
Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
This code is released under Apache 2.0 and GPL 2.0 licenses.*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <x86intrin.h>


#include "lbr_chardev.h"
#include "getcpu.h"
#include "perf_attr.h"

#define IF_LBR(x) x
/* #define IF_LBR(x) */

int enter_the_slide(void *entry)
{
    void *exit_label = &&exit;

    __asm__(""
            "mov %[Exit], %%rax\n\t"
            "jmp *%[Entry]\n\t"
            : : [Entry] "g" (entry), [Exit] "g" (exit_label) : "%rax");
exit:
    return 0;
}

int empty_btb(int offset)
{
    int i;
    size_t size = 4096 * 6;

    char *jump_slide =  mmap(NULL, size + offset,
            PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE,
            0, 0);

    for (i = offset; i < size; i += 6) {
        /* jmp 2 */
        jump_slide[i+0] = '\xeb';
        jump_slide[i+1] = '\x02';
        /* jmp 2 */
        jump_slide[i+2] = '\xeb';
        jump_slide[i+3] = '\x02';
        /* jmp -2 */
        jump_slide[i+4] = '\xeb';
        jump_slide[i+5] = '\xfc';
    }

    /* jmpq *%rax */
    jump_slide[size-7] = '\xff';
    jump_slide[size-6] = '\xe0';

    printf("entering jmp slide at 0x%016lx\n", (long)jump_slide);
    enter_the_slide(jump_slide);
    munmap(jump_slide, size);
}

int **global_int_pointer;

int tester(int fd, int ***input, int retval)
{
    if (***input % 2 == 0) {
        retval = (((retval + 1) * retval) * 7) * (retval * 7814);
    } else {
        retval = (((retval + 1) * retval) * 7) * (retval * 7814);
    }

    return retval;
}

int main(int argc, char **argv)
{
    int fd_lbr, input_retval, result;
    /* int fd_counter; */
    int *malloc_int_pointer;
    int ***input_ptr;
    /* struct perf_event_attr pe_m; */

    fd_lbr = -1;
    /* fd_counter = pe_read_and_open(&pe_m, PERF_COUNT_HW_BRANCH_INSTRUCTIONS); */

    printf("tester is at 0x%016lx\n", (long)&tester);
    printf("main is at   0x%016lx, running on %d\n", (long)&main, get_cpu_id());
    printf("loop is at   0x%016lx\n", (long)&&test_loop);
    printf("        to   0x%016lx\n", (long)&&loop_exit);

    empty_btb(0);

    malloc_int_pointer = malloc(sizeof(int));
    global_int_pointer = &malloc_int_pointer;
    input_ptr = &global_int_pointer;

    result = 0;
    *malloc_int_pointer = (argc > 1 ? atoi(argv[1]) : 99);
    input_retval = (argc > 1 ? atoi(argv[1]) : 123);

    IF_LBR(fd_lbr = open(DEVICE_FILE_NAME, 0));
    if (fd_lbr < 0) {
        printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
        IF_LBR(exit(-1));
    }

    empty_btb(0);
    empty_btb(1);
    /* empty_btb(2); */
    /* empty_btb(3); */

    usleep(1000);
test_loop:
    for (int i = 0; i < 2; i++) {
        *malloc_int_pointer += 1;
        /* pe_reset(fd_counter); */
        _mm_lfence();
        _mm_clflush((void *)malloc_int_pointer);
        _mm_clflush(global_int_pointer);
        /* pe_enable(fd_counter); */
        result += tester(fd_lbr, input_ptr, input_retval);
        /* pe_disable(fd_counter); */
        /* printf("Got me seom %ld misses\n", pe_read(fd_counter)); */
    }

loop_exit:
    IF_LBR(__asm__ volatile(""
                "mov %[Fd], %%edi\n\t"
                "mov $0x10, %%eax\n\t"
                "syscall\n\t"
                : : [Fd] "g" (fd_lbr) : "%eax", "%edi"));

    _mm_lfence();
    printf("input is %d, final result is %d\n", ***input_ptr, result);

    IF_LBR(close(fd_lbr));
    free(malloc_int_pointer);
    return 0;
}
