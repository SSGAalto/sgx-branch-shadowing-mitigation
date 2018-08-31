/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGEAXANDONE_H
#define SAMPLE_SEGEAXANDONE_H

#include "ShadowSegment.h"

class SegEaxAndOne : public ShadowSegment {
private:
    static constexpr const size_t s_hex_size = 3;
    static constexpr const char *const s_hex = "\x83\xe0\x01";
    static constexpr const char *const s_name = "SegEaxAndOne";

public:
    SegEaxAndOne() : ShadowSegment(s_hex_size) {}
    void *get_content() const override { return (void *) s_hex; }
    const char *const get_name() const override { return s_name; }

    static seg_ptr make()
    {
        return std::make_shared<SegEaxAndOne>();
    }
};


#endif //SAMPLE_SEGEAXANDONE_H
