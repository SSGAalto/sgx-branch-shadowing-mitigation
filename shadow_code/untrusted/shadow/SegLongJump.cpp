/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include "SegLongJump.h"

#include <cstring>
#include <mem/Pointers.h>
#include <misc/Logger.h>

SegLongJump::SegLongJump(void *pos, seg_ptr target)
        : ShadowSegment(s_hex_size, pos)
{
    m_jump_target = target;
};

void SegLongJump::finalize()
{
    memcpy(m_hex, s_hex, s_hex_size);

    intptr_t diff = Pointers::get_diff(m_jump_target->get_pos(), get_end());

    char *jump = (char*)&diff;

    for (int i = 1; i < 5; i++)
        m_hex[i] = jump[i-1];

    get_logger()->debug("%s: setting jump to %d", __FUNCTION__, diff);
    ShadowSegment::finalize();
}

std::shared_ptr<ShadowSegment> SegLongJump::make(void *pos, seg_ptr target)
{
    return std::make_shared<SegLongJump>(pos, target);
}
