/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include "SegEaxWrite.h"

SegEaxWrite::SegEaxWrite(void* pos, int32_t val)
        : ShadowSegment(s_hex_size, pos)
{
    for (int i = 0; i < s_hex_size; i++)
        m_hex[i] = s_hex[i];

    for (int i = 1; i < 5; i++, val = (val >> 8))
        m_hex[i] = (char)val;
}

void *SegEaxWrite::get_content() const
{
    return (char *)m_hex;
};
