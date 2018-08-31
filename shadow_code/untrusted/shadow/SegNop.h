/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGNOP_H
#define SAMPLE_SEGNOP_H


#include "ShadowSegment.h"

class SegNop : public ShadowSegment {
public:
    static constexpr const size_t s_hex_size = 1;
    static constexpr const char *const s_hex = "\x90";
    static constexpr const char *const s_name = "SegNop";

public:
    static seg_ptr make() { return std::make_shared<SegNop>(1); }
    static seg_ptr make(size_t nop_count) { return std::make_shared<SegNop>(nop_count); }

public:
    SegNop(size_t nop_count);
    void finalize() override;
    const char *const get_name() const override { return s_name; }
    void *get_content() const override { return m_hex; };

private:
    char *m_hex = nullptr;
    size_t m_nop_count;
};


#endif //SAMPLE_SEGNOP_H
