/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_SHADOWSEGMENT_H
#define SAMPLE_SHADOWSEGMENT_H

#include <cstdlib>
#include <memory>

class ShadowSegment;

typedef std::shared_ptr<ShadowSegment> seg_ptr;

class ShadowSegment {

public:
    virtual void finalize() {};
    virtual void* get_content() const = 0;
    virtual const char *const get_name() const = 0;
    size_t get_size() const { return m_size; }

    bool has_pos() const;
    void *get_pos() const;
    void *get_end() const;

    void set_pos(void *ptr, long offset);
    void set_pos(void *ptr) { set_pos(ptr, 0); }
    void set_end(void *ptr, long offset);
    void set_end(void *ptr) { set_end(ptr, 0); }

    void apply_mask(uintptr_t mask);
    intptr_t apply_base(void *const ptr, uintptr_t mask);
    void apply_offset(intptr_t offset);

protected:
    explicit ShadowSegment(size_t size) : ShadowSegment(size, nullptr) {};
    explicit ShadowSegment(size_t size, void *pos);

private:
    const size_t m_size = 0;
    void *m_pos = nullptr;
};

#endif //SAMPLE_SHADOWSEGMENT_H
