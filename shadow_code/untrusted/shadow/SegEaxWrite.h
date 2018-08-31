/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGEAXWRITEONE_H
#define SAMPLE_SEGEAXWRITEONE_H


#include "ShadowSegment.h"

class SegEaxWrite: public ShadowSegment {
public:
    static seg_ptr make(void *pos, int32_t val) { return std::make_shared<SegEaxWrite>(pos, val);}
private:
    static constexpr const size_t s_hex_size = 5;
    static constexpr const char *const s_hex = "\xb8\x01\x00\x00\x00"; // retq
    static constexpr const char *const s_name = "SegEaxWrite";

public:
    SegEaxWrite(void *pos, int32_t val);
    void *get_content() const override;
    const char *const get_name() const override { return s_name; }

private:
    char m_hex[s_hex_size];
    
};


#endif //SAMPLE_SEGEAXWRITEONE_H
