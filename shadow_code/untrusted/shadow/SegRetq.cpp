/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstring>
#include "SegRetq.h"
#include "SegNop.h"

SegRetq::~SegRetq()
{
    if (m_hex != nullptr)
        free(m_hex);
}

SegRetq::SegRetq(void *pos, size_t nops)
: ShadowSegment(s_hex_size + nops * s_nop_hex_size, pos), m_nops(nops)
{};

std::shared_ptr<ShadowSegment> SegRetq::make(void *pos, size_t nops)
{
    return std::make_shared<SegRetq>(pos, nops);
}

void SegRetq::finalize()
{
    size_t size = s_hex_size + m_nops * s_nop_hex_size;
    m_hex = (char *)malloc(size);

    for (int i = 0; i < size; i += s_nop_hex_size) {
        m_hex[i] = s_nop_hex[i%s_nop_hex_size];
    }

    memcpy(&m_hex[size-s_hex_size], s_hex, s_hex_size);

    ShadowSegment::finalize();
}


