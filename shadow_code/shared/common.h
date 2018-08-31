/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_COMMON_H
#define SAMPLE_COMMON_H

#include <stdio.h>

#define DEBUGL_DEBUG 0
#define DEBUGL_INFO 1
#define DEBUGL_WARN 2
#define DEBUGL_CRITICAL 3
#define DEBUGL_NONE 10

static inline void dump_mem(
        char *buf, size_t buf_size, int byte_count, char *ptr)
{
    for (int i = 0; i < byte_count; i++, ptr++) {
        snprintf(buf, buf_size, "%s 0x%2hhx", buf, *ptr);
    }
}

#endif
