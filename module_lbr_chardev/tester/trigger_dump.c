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

#define DEVICE_FILE_NAME "/dev/lbr_dumper"

#include "../module/lbr_tools.h"

int main(int argc, char **argv)
{
    int fd_lbr = open(DEVICE_FILE_NAME, 0);

    struct lbr_data data;

    if (fd_lbr < 0) {
        printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
        exit(-1);
    } else {
        __asm__ volatile(""
                "mov %[Fd], %%edi\n\t"
                "mov $0x10, %%eax\n\t"
                "syscall\n\t"
                : : [Fd] "g" (fd_lbr) : "%eax", "%edi");

        for (int i = 0; i < 32; i++) {
            read(fd_lbr, &data, sizeof(struct lbr_data));

            printf("0x%016lx -> 0x%016lx [%s] cycles: %u\n",
                    lbr_data_get_from(&data),
                    lbr_data_get_to(&data),
                    lbr_data_get_mispred(&data) != 0 ? "MISS" : "____",
                    lbr_data_get_cycle_count(&data)
                  );
        }

        close(fd_lbr);
    }
    return 0;
}
