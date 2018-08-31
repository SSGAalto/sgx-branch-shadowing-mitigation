/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SHADOW_H
#define SAMPLE_SHADOW_H

#include <memory>
#include <list>

#include "ShadowSegment.h"
#include "mem/AlignedMem.h"
#include "config.h"
#include "misc/attributes.h"

class Shadow {
public:
    static const uintptr_t s_segment_spacing = 2;

    always_inline
    static inline int execute_ptr(const void *ptr, int input);
    static bool is_jne(void *ptr);

private:
    static void fill_with_nops(void *start, void *end);

    
public:

    explicit Shadow(unsigned int align_bits, void *vic_src, void *vic_dst);
    ~Shadow();

    void add_segment(std::shared_ptr<ShadowSegment> segment);
    void add_entry(std::shared_ptr<ShadowSegment> segment);
    void finalize();

    inline void *get_entry_ptr() { return m_entry_segment->get_pos(); };

    size_t get_size();

    void *get_aligned_ptr(const void *unaligned_ptr);
    unsigned char get_aligned_char(const void *unaligned_ptr);

    void *get_vic_src() { return m_vic_src; }
    void *get_vic_dst() { return m_vic_dst; }

    void verbose_execute(int input);
    inline int execute(int input);

private:
    explicit Shadow() : Shadow(lbr_entry_bits, nullptr, nullptr) { assert(false); };

    std::shared_ptr<spdlog::logger> logger;
    std::shared_ptr<AlignedMem> m_mem;
    const unsigned int m_align_bits;
    const uintptr_t m_mask;

    std::list<std::shared_ptr<ShadowSegment>> m_segments;
    std::list<std::shared_ptr<ShadowSegment>> m_loose_segments;

    bool m_finalized = false;

    void *m_vic_src;
    void *m_vic_dst;

    void place_segment(std::shared_ptr<ShadowSegment> segment);

    std::shared_ptr<ShadowSegment> m_entry_segment;
};

typedef std::shared_ptr<Shadow> Shadow_p;

inline int Shadow::execute(int input)
{
    return execute_ptr(get_entry_ptr(), input);
}

always_inline
inline int Shadow::execute_ptr(const void *ptr, int input) {
    int retval = -1;
    asm volatile(""
                 "call *%[entry]\n\t"
    : "=a" (retval)
    : "g" (input), [entry] "g" (ptr)
    : "rbx", "rcx", "rdx", "r14", "r13", "r12"
    );
    return retval;
}

#endif //SAMPLE_SHADOW_H
