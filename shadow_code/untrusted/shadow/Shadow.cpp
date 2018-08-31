/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <sys/mman.h>
#include <misc/Logger.h>
#include <cassert>
#include <iostream>
#include "Shadow.h"
#include "SegNop.h"
#include "mem/Pointers.h"
#include "SegEaxWrite.h"
#include "SegRetq.h"
#include "SegEaxAndOne.h"
#include "SegJneEaxZero.h"
#include "SegJump.h"

Shadow::Shadow(unsigned int align_bits, void *s, void *d)
: m_align_bits(align_bits), m_mask(Pointers::get_mask(align_bits))
{
    static auto the_logger = spdlog::stdout_color_mt("Shadow");
    logger = the_logger;
    m_vic_src = s;
    m_vic_dst = d;
}

Shadow::~Shadow()
{
    if (m_mem != nullptr)
        m_mem->unallocate();
}
void Shadow::add_entry(std::shared_ptr<ShadowSegment> segment)
{
    assert(m_entry_segment == nullptr);
    m_entry_segment = segment;
    add_segment(segment);
}

void Shadow::add_segment(std::shared_ptr<ShadowSegment> segment)
{
    assert(!m_finalized);
    assert((m_segments.empty() || segment->get_pos() == nullptr ||
           m_segments.back()->get_pos() < segment->get_pos()) && "segment order is correct");

    if (segment->has_pos()) {
        /* If the segment has a position we can place it and any
         * loose segments. */

        logger->debug("%s: placing positioned segment %s", __FUNCTION__, segment->get_name());
        place_segment(segment);
    } else {
        /* If the segment doesn't have a location we store it as a loose
         * segment and instead store it for later. */
        logger->debug("%s: saving loose segment %s", __FUNCTION__, segment->get_name());
        m_loose_segments.push_back(segment);
    }
}

/**
 * Place the given element and any loose elements.
 *
 * This also tries to space any elements evenly. The spacing thing isn't strictly necessary, but
 * it makes the resulting code look a bit nicer and perhaps easier to debug.
 *
 * @param segment
 */
void Shadow::place_segment(std::shared_ptr<ShadowSegment> segment)
{
    assert(!m_finalized);
    // TODO: Detect bad placements where segments overwrite each-other

    /* If we don't have any loose segments we can just place this and be done */
    if (m_loose_segments.empty()) {
        logger->debug("%s: no loose segments to place", __FUNCTION__);
        m_segments.push_back(segment);
        return;
    }

    long spacing = s_segment_spacing;
    if (m_segments.empty()) {
        SPDLOG_TRACE(get_ulogger(), "No segments placed yet, using default spacing (%d)", spacing);
    } else {
        SPDLOG_TRACE(get_ulogger(), "All segments finalized");
        /* If we have prior placed segments then just space any in-between elements evenly. */

        /* This is the total space we need to fill */
        long space = (long)segment->get_pos() - (long)m_segments.back()->get_end();
        SPDLOG_TRACE(get_ulogger(), "Total space needed available is %d", space);

        /* Calculate total size of segments to determine free space. */
        long total_size = 0;
        for (const auto s : m_loose_segments)
            total_size += s->get_size();
        SPDLOG_TRACE(get_ulogger(), "Size of contained elements is %d", total_size);

        assert(total_size <= space && "elements fit into available space");
        spacing = (space - total_size) / m_loose_segments.size();
        SPDLOG_TRACE(get_ulogger(), "Setting spacing to %d based on already placed segments", spacing);
    }


    /* Reverse iterating, since we're not guaranteed to have a "first" */
    auto prev = segment;
    logger->debug("%s: positioning loose segments (spacing %d)", __FUNCTION__, spacing);
    assert(spacing < 10000 && "spacing is within sane range");
    for (auto it = m_loose_segments.rbegin(); it != m_loose_segments.rend(); it++) {
        auto prev_pos = prev->get_pos();
        (*it)->set_end(prev_pos, -spacing);
        prev = *it;
    }

    for (const auto s : m_loose_segments)
        m_segments.push_back(s);

    /* Finally, plop in the placed segments */
    m_segments.push_back(segment);

    m_loose_segments.clear();
}

void Shadow::finalize()
{
    assert(!m_finalized);
    assert(m_entry_segment != nullptr && "add_entry must be called exactly once before finalize");

    logger->debug("%s: placing loose segments", __FUNCTION__);
    if (!m_loose_segments.empty()) {
        if (m_segments.empty()) {
            logger->critical("%s: fail", __FUNCTION__);
            abort();
        }

        auto prev = m_segments.back();
        for (const auto s : m_loose_segments) {
            s->set_pos(prev->get_end(), s_segment_spacing);
            m_segments.push_back(s);
            prev = s;
        }
    }

    auto first = m_segments.front();

    logger->debug("%s: preparing shadow area at %p (size %d)",
                  __FUNCTION__, first->get_pos(), get_size());
    m_mem = std::make_shared<AlignedMem>(first->get_pos(), get_size(), m_align_bits);
    if (!m_mem->allocate())
        abort();

    intptr_t offset = first->apply_base(m_mem->as_ptr(), m_mask);

    /* First set all the final memory location. */
    logger->debug("%s: repositioning segments for allocated memory, offset is %lx", __FUNCTION__, offset);
    for (auto it = ++(m_segments.begin()); it != m_segments.end(); it++)
        (*it)->apply_offset(offset); /* NOTE: don't reapply to first! */

    /* Fil the used area with NOPs. */
    logger->debug("%s: filling used shadow area with NOPs (%p - %p)",
                  __FUNCTION__, first->get_pos(), m_segments.back()->get_end());
    fill_with_nops(first->get_pos(), m_segments.back()->get_end());

    /* Then finalize, this calculates potential relative jumps, and such. */
    logger->debug("%s: finalizing segments", __FUNCTION__);
    for (auto s : m_segments) {
        SPDLOG_TRACE(get_ulogger(), "Calling %s->finalize()", s->get_name());
        s->finalize();
        SPDLOG_TRACE(get_ulogger(), "%s->finalize() done", s->get_name());
    }
    SPDLOG_TRACE(get_ulogger(), "All segments finalized");

    /* Then write into memory (these must be separate to ensure forward jumps are correct) */
    logger->debug("%s: writing segments to allocated memory", __FUNCTION__);
    void *last_end = nullptr;
    for (auto s : m_segments) {
        logger->debug("%s: writing to %p <- %s",__FUNCTION__, s->get_pos(), s->get_name());
        assert(last_end <= s->get_pos());
        memcpy(s->get_pos(), s->get_content(), s->get_size());
        last_end = s->get_pos();
    }
    SPDLOG_TRACE(get_ulogger(), "All segments written");

    logger->debug("finalize done");
    m_finalized = true;
}

size_t Shadow::get_size()
{
    return (uintptr_t)m_segments.back()->get_pos() - (uintptr_t )m_segments.front()->get_end();
}

void Shadow::fill_with_nops(void *start, void *end)
{
    auto c_start = static_cast<char *>(start);
    auto c_end = static_cast<char *>(end);

    for (auto c = c_start; c != c_end; c++) {
        *c = SegNop::s_hex[0];
    }
}

void Shadow::verbose_execute(int input)
{
    logger->info("calling      0x%016lx with input %d", get_entry_ptr(), input);
    int retval = execute(input);
    logger->info("return value was %d", retval);
}

bool Shadow::is_jne(void *ptr)
{
    auto c_ptr = static_cast<char *>(ptr);
    return *c_ptr == 0x75;
}

void *Shadow::get_aligned_ptr(const void *unaligned_ptr) {
    assert(m_finalized);
    const void *start = m_segments.front()->get_pos();
    const void *end = m_segments.back()->get_end();
    void *aligned_ptr = Pointers::set_masked_base(unaligned_ptr, m_entry_segment->get_pos(), m_mask);

    if (aligned_ptr < start)
        aligned_ptr = Pointers::add_offset(aligned_ptr, m_mask + 1);

    if (aligned_ptr < start || aligned_ptr > end) {
        logger->critical("%s: 0x%016lx -> 0x%016lx not within used memory",
                         __FUNCTION__, unaligned_ptr, aligned_ptr);
        return nullptr;
    }

    return aligned_ptr;
}

unsigned char Shadow::get_aligned_char(const void *unaligned_ptr)
{
    return *((unsigned char *)get_aligned_ptr(unaligned_ptr));
}
