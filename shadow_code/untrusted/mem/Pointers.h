/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_POINTERS_H
#define SAMPLE_POINTERS_H


#include <cstdint>
#include <cassert>

class Pointers {
public:
    static inline intptr_t get_diff(const void *a, const void *b);
    static inline void *add_offset(const void *ptr, intptr_t offset);
    static inline uintptr_t get_mask(int bits);
    static inline void *apply_mask(const void *ptr, uintptr_t mask);
    static inline void *apply_rmask(const void *ptr, uintptr_t mask);

    static void *set_masked_base(const void *ptr, const void *base, const uintptr_t mask);

    static void **get_indirect_pointer(uintptr_t input);
    static void set_indirect_pointer_value(void **ptr, uintptr_t value);
    static uintptr_t get_indirect_pointer_value(void **ptr);
    static void flush_indirect_pointer(void **ptr);
    static void free_indirect_pointer(void **ptr);

};

inline uintptr_t Pointers::get_mask(const int bits)
{
    assert(bits >= 0);
    return (1UL << bits) - 1UL;
}

inline intptr_t Pointers::get_diff(const void *a, const void *b)
{
    assert(a != nullptr);
    assert(b != nullptr);
    return ((intptr_t)a - (intptr_t)b);
}

inline void *Pointers::add_offset(const void *ptr, intptr_t offset)
{
    assert(ptr != nullptr);
    return reinterpret_cast<void *>((intptr_t) ptr + offset);
}

inline void *Pointers::apply_mask(const void *ptr, uintptr_t mask)
{
    assert(ptr != nullptr);
    return reinterpret_cast<void *>((uintptr_t)ptr & mask);
}

inline void *Pointers::apply_rmask(const void *ptr, uintptr_t mask)
{
    assert(ptr != nullptr);
    return reinterpret_cast<void *>((uintptr_t)ptr & ~mask);
}

#endif //SAMPLE_POINTERS_H
