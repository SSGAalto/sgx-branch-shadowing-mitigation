/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGJUMPR13_H
#define SAMPLE_SEGJUMPR13_H

#include "ShadowSegment.h"

class SegJumpR13 : public ShadowSegment {
    static constexpr const size_t s_hex_size = 4;
    static constexpr const char *const s_hex = "\x41\xff\x65\x00"; // jmpq   *0x0(%r13)
    static constexpr const char *const s_name = "SegJumpR13";

public:
    static seg_ptr make(void *pos);

    SegJumpR13(void *pos);

    void finalize() override;
    void *get_content() const override { return (void *) s_hex; }
    const char *const get_name() const override { return s_name; }

private:

};

#endif //SAMPLE_SEGJUMPR13_H
