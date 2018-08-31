/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_RETURNSEGMENT_H
#define SAMPLE_RETURNSEGMENT_H

#include "ShadowSegment.h"

class SegRetq : public ShadowSegment {
public:
    static seg_ptr make(void *pos, size_t nops);

private:
    static constexpr const size_t s_hex_size = 1;
    static constexpr const char *const s_hex = "\xc3"; // retq
    static constexpr const size_t s_nop_hex_size = 1;
    static constexpr const char *const s_nop_hex = "\x90";

    static constexpr const char *const s_name = "SegRetq";

public:
    SegRetq() : SegRetq(nullptr, 1) {};
    ~SegRetq();
    SegRetq(void *pos, size_t nops);

    void finalize() override;
    void *get_content() const override { return (void *) m_hex; }
    const char *const get_name() const override { return s_name; }

private:

    char *m_hex = nullptr;
    size_t m_nops;
};

#endif //SAMPLE_RETURNSEGMENT_H
