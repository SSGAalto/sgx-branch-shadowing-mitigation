/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstring>
#include <config.h>
#include <spdlog/spdlog.h>
#include <mem/Pointers.h>
#include "SegSetPosInReg.h"

SegSetPosInReg::SegSetPosInReg(void *pos, seg_ptr target, void **target_ptr, unsigned char reg)
: ShadowSegment(s_hex_size, pos),
  m_jump_target(target), m_target_ptr(target_ptr), m_register(reg)
{}

seg_ptr SegSetPosInReg::make(void *pos, seg_ptr target, void **target_ptr, unsigned char reg)
{
    return std::make_shared<SegSetPosInReg>(pos, target, target_ptr, reg);
}

void SegSetPosInReg::finalize()
{
    auto logger = get_ulogger();

    SPDLOG_TRACE(logger, "Copying s_hex into m_hex");
    memcpy(m_hex, s_hex, s_hex_size);

    m_hex[1] = (char)(0xb0 | m_register);

    if (m_jump_target != nullptr) {
        void *target = m_jump_target->get_pos();

        SPDLOG_TRACE(logger, "Jump target is 0x%016lx", (uintptr_t) target);
        Pointers::set_indirect_pointer_value(m_target_ptr, reinterpret_cast<uintptr_t>(target));
    }

    SPDLOG_TRACE(logger, "Setting m_target_ptr to 0x%016lx", (uintptr_t) *m_target_ptr);

    int start = s_hex_size-8;
    for (int i = 0; i < 8; i++) {
        SPDLOG_TRACE(logger, "Writing 0x%016lx m_hex[%d] <= 0x%02x",
                     (uintptr_t)m_hex, start+i, ((char *)m_target_ptr)[i]);
        m_hex[start+i] = ((char*)m_target_ptr)[i];
    }

    logger->debug("%s finalized, pointer is at %p and points to 0x%lx",
                 get_name(), *m_target_ptr, Pointers::get_indirect_pointer_value(m_target_ptr));
    ShadowSegment::finalize();
}
