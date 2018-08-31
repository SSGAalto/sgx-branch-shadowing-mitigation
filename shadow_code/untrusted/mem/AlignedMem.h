/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_ALIGNEDMEM_H
#define SAMPLE_ALIGNEDMEM_H


#include <cstdlib>
#include <cstdint>
#include <spdlog/spdlog.h>

class AlignedMem {

public:
    AlignedMem(void *entry, size_t size, unsigned int align_bits);
    ~AlignedMem();

    void *as_ptr() const;
    uintptr_t as_uint() const;

    bool is_allocated() const;
    bool allocate();
    void unallocate();

private:
    void *mem = nullptr;
    void *entry = nullptr;
    void *target_entry = nullptr;
    size_t size = 0;
    unsigned int align_bits = 0;
};


#endif //SAMPLE_ALIGNEDMEM_H
