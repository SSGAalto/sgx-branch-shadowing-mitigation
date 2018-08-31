/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstring>
#include <mem/Pointers.h>
#include "SegJneEaxZero.h"

SegJneEaxZero::SegJneEaxZero(void *pos, seg_ptr target)
        : ShadowSegment(s_hex_size, pos), m_jump_target(target)
{}

void SegJneEaxZero::finalize()
{
    memcpy(m_hex, s_hex, s_hex_size);
    char jump = Pointers::get_diff(m_jump_target->get_pos(), get_end());
    m_hex[s_hex_size-1] = jump;

    ShadowSegment::finalize();
}

std::shared_ptr<ShadowSegment> SegJneEaxZero::make(void *pos, seg_ptr target)
{
    return std::make_shared<SegJneEaxZero>(Pointers::add_offset(pos, -jne_offset), target);
}
