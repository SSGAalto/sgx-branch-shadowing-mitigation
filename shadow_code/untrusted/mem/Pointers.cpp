/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <emmintrin.h>
#include "Pointers.h"
#include "misc/Logger.h"

void *Pointers::set_masked_base(const void *ptr, const void *base, const uintptr_t mask)
{
    auto ptr_masked = (uintptr_t )apply_mask(ptr, mask);
    auto base_masked = (uintptr_t )apply_rmask(base, mask);
    auto retval =  reinterpret_cast<void *>(ptr_masked | base_masked);

    get_logger()->debug("%s: %p -> 0x%016lx --- %p -> 0x%016lx", __FUNCTION__,
                        ptr, ptr_masked, base, base_masked);

    get_logger()->debug("%s: 0x%016lx | 0x%016lx => %p", __FUNCTION__,
                        ptr_masked, base_masked, retval);

    return retval;
}

inline void Pointers::flush_indirect_pointer(void **ptr)
{
    _mm_clflush(*ptr);
    _mm_clflush(ptr);
}

void **Pointers::get_indirect_pointer(uintptr_t input)
{
    auto ptr_inner = malloc(sizeof(void *));
    auto ptr_outer = (void **)malloc(sizeof(void *));

    *ptr_outer = ptr_inner;

    set_indirect_pointer_value(ptr_outer, input);
    flush_indirect_pointer(ptr_outer);
    return ptr_outer;
}

void Pointers::set_indirect_pointer_value(void **ptr, uintptr_t value)
{
    void *ptr_inner = *ptr;
    *((uintptr_t *)ptr_inner) = value;
}

uintptr_t Pointers::get_indirect_pointer_value(void **ptr)
{
    return *((uintptr_t *)*ptr);
}

void Pointers::free_indirect_pointer(void **ptr)
{
    if (ptr != nullptr) {
        free(*ptr);
        free(ptr);
    }
}
