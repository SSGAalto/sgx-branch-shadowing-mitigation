/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGSETPOSINR13_H
#define SAMPLE_SEGSETPOSINR13_H

#include "ShadowSegment.h"

class SegSetPosInReg : public ShadowSegment {
    static constexpr const size_t s_hex_size = 10;
    static constexpr const char *const s_hex = "\x49\xbd\xef\xcd\xab\x89\x67\x45\x23\x01"; // movabs $0x123456789abcdef,%r13
    static constexpr const char *const s_name = "SegSetPosInReg";

public:
    SegSetPosInReg(void *pos, seg_ptr target, void **target_ptr, unsigned char register_num);

    static seg_ptr make(void *pos, seg_ptr target, void **target_ptr, unsigned char register_num);

    void finalize() override;
    void *get_content() const override { return (void *) m_hex; }
    const char *const get_name() const override { return s_name; }

private:

    seg_ptr m_jump_target = nullptr;
    unsigned char m_register;
    char m_hex[s_hex_size];

    void **m_target_ptr;
};


#endif //SAMPLE_SEGSETPOSINR13_H
