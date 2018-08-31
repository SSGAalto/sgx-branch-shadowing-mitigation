/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SEGJNEEAXZERO_H
#define SAMPLE_SEGJNEEAXZERO_H


#include "ShadowSegment.h"

class SegJneEaxZero : public ShadowSegment {
    static constexpr const size_t s_hex_size = 4;
    static constexpr const char *const s_hex =
            "\x85\xc0"      /* test %eax, %eax  */
            "\x75\x00";     /* jne 0 */
    static constexpr const char *const s_name = "SegJneEaxZero";

public:
    static constexpr const uintptr_t jne_offset = 2;

    SegJneEaxZero(seg_ptr target) : SegJneEaxZero(nullptr, target) {};
    SegJneEaxZero(void *pos, seg_ptr target);

    void finalize() override;
    void *get_content() const override { return (void *) m_hex; }

    static std::shared_ptr<ShadowSegment> make(void *pVoid, seg_ptr shared_ptr);
    const char *const get_name() const override { return s_name; }


private:
    seg_ptr m_jump_target;
    char m_hex[s_hex_size];

};


#endif //SAMPLE_SEGJNEEAXZERO_H
