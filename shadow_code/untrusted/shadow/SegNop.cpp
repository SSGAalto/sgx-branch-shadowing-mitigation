/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include "SegNop.h"

SegNop::SegNop(size_t nop_count)
        : ShadowSegment(s_hex_size * nop_count), m_nop_count(nop_count)
{

}

void SegNop::finalize()
{
    size_t size = s_hex_size + m_nop_count;
    m_hex = (char *)malloc(size);

    for (int i = 0; i < size; i += s_hex_size) {
        m_hex[i] = s_hex[i%s_hex_size];
    }

    ShadowSegment::finalize();
}
