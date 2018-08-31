/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGEAXANDONEINDIR_H
#define SAMPLE_SEGEAXANDONEINDIR_H

#include "ShadowSegment.h"

class SegEaxAndOneIndir : public ShadowSegment {
private:
    static constexpr const size_t s_hex_size = 6;
    static constexpr const char *const s_hex = ""
                                               "\x41\x8b\x06"       // mov (r14), %eax
                                               "\x83\xe0\x01";      // and $0x1, %eax
    static constexpr const char *const s_name = "SegEaxAndOneIndir";

public:
    SegEaxAndOneIndir() : ShadowSegment(s_hex_size) {}
    void *get_content() const override { return (void *) s_hex; }
    const char *const get_name() const override { return s_name; }

    static seg_ptr make()
    {
        return std::make_shared<SegEaxAndOneIndir>();
    }
};

#endif //SAMPLE_SEGEAXANDONEINDIR_H
