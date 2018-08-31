/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_BPUUTILS_H
#define SAMPLE_BPUUTILS_H

// #define ASSUMED_BTB_ENTRY_COUNT 4096
#define ASSUMED_BTB_ENTRY_COUNT 8192


class BpuUtils {
public:
    static void mess_btb(void *);
    static void do_sequential_jumps(size_t, void *);
    static void do_repeated_jumps(void *);
};


#endif //SAMPLE_BPUUTILS_H
