/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstring>
#include <mem/Pointers.h>
#include <misc/Logger.h>
#include "SegJump.h"

SegJump::SegJump(void *pos, seg_ptr target)
        : ShadowSegment(s_hex_size, pos)
{
    m_jump_target = target;
};

void SegJump::finalize()
{
    memcpy(m_hex, s_hex, s_hex_size);
    intptr_t diff = Pointers::get_diff(m_jump_target->get_pos(), get_end());
    assert((diff > -128 && diff < 128) && "jump distance is withing jmp limits");
    char jump = (char) diff;
    m_hex[s_hex_size-1] = jump;
    get_logger()->debug("%s: setting jump to %d", __FUNCTION__, jump);

    ShadowSegment::finalize();
}

std::shared_ptr<ShadowSegment> SegJump::make(void *pos, seg_ptr target)
{
    return std::make_shared<SegJump>(pos, target);
}
