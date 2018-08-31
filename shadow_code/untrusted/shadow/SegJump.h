/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGJUMP_H
#define SAMPLE_SEGJUMP_H

#include "ShadowSegment.h"

class SegJump : public ShadowSegment {
    static constexpr const size_t s_hex_size = 2;
    static constexpr const char *const s_hex = "\xeb\x00";
    static constexpr const char *const s_name = "SegJump";

public:
    static seg_ptr make(void *pos, seg_ptr target);


    SegJump(seg_ptr target) : SegJump(nullptr, target) {};
    SegJump(void *pos, seg_ptr target);

    void finalize() override;
    void *get_content() const override { return (void *) m_hex; }
    const char *const get_name() const override { return s_name; }

private:
    seg_ptr m_jump_target;
    char m_hex[s_hex_size];

};


#endif //SAMPLE_SEGJUMP_H
