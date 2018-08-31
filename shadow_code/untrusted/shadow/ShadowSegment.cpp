/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */


#include <misc/Logger.h>
#include <config.h>
#include "ShadowSegment.h"

ShadowSegment::ShadowSegment(size_t size, void *pos)
        : m_size(size), m_pos(pos)
{}

bool ShadowSegment::has_pos() const
{
    return (m_pos != nullptr);
}

void *ShadowSegment::get_pos() const
{
    return m_pos;
}

void *ShadowSegment::get_end() const
{
    return reinterpret_cast<void *>((uintptr_t )m_pos + (uintptr_t)m_size);
}

void ShadowSegment::set_pos(void *ptr, long offset)
{
    SPDLOG_TRACE(get_ulogger(), "set_pos(%p, %ld", ptr, offset);
    assert((((uintptr_t )ptr & 0xff000000000000UL) == 0UL) && "pointer is valid");
                                  //0x5565ae75eaa3
    m_pos = reinterpret_cast<void *>((uintptr_t )ptr + offset);
}

void ShadowSegment::set_end(void *ptr, long offset)
{
    m_pos = reinterpret_cast<void *>((uintptr_t )ptr - m_size + offset);
}

void ShadowSegment::apply_mask(uintptr_t mask)
{
    m_pos = reinterpret_cast<void *>(
            (uintptr_t)m_pos & mask
    );
}

intptr_t ShadowSegment::apply_base(void *ptr, const uintptr_t mask)
{
    void *old_pos = m_pos;
    m_pos = reinterpret_cast<void *>(
            ((uintptr_t)m_pos & mask) | ((uintptr_t)ptr & ~mask)
    );

    get_logger()->debug("%s: %p -> %p (m_mask: 0x%016lx )", __FUNCTION__, old_pos, m_pos, mask);

    return (intptr_t)m_pos - (intptr_t)old_pos;
}

void ShadowSegment::apply_offset(intptr_t offset)
{
    m_pos = reinterpret_cast<void *>((intptr_t)m_pos + offset);
}
