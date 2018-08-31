/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <sys/mman.h>
#include <cassert>
#include "AlignedMem.h"
#include "misc/Logger.h"

AlignedMem::AlignedMem(void *entry, size_t size, unsigned int align_bits)
: align_bits(align_bits),
  target_entry(entry),
  size((1UL << align_bits) + size)
{}

AlignedMem::~AlignedMem()
{
    unallocate();
}

bool AlignedMem::allocate()
{
    assert(mem == nullptr);
    auto logger = spdlog::get("console");

    const uintptr_t mask = (1UL << align_bits) - 1UL;

    logger->trace("allocating %ld memory", size);
    mem = mmap(nullptr, size,
                     PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE,
                     0, 0);

    if (mem == nullptr) {
        logger->critical("AlignedMem::allocate failed to allocate memory");
        return false;
    }
    SPDLOG_TRACE(logger, "memory allocated at %p to 0x%016lx", mem, (uintptr_t)mem + size);
    SPDLOG_TRACE(logger, "aligning entry with %p (m_mask: %x, bits: %u)", target_entry, mask, align_bits);

    entry = (void *)(((uintptr_t)(mem) & ~mask) + ((uintptr_t)target_entry & mask));

    if (entry < mem) {
        entry = (void *)((uintptr_t)entry + mask);
    }

    SPDLOG_TRACE(logger, "entry is now set to %p", entry);

    return true;
}

void AlignedMem::unallocate()
{
    if (mem != nullptr) {
        munmap(mem, size);
        mem = nullptr;
        size = 0;
    }
}

bool AlignedMem::is_allocated() const
{
    return (mem != nullptr);
}

void *AlignedMem::as_ptr() const
{
    assert(mem != nullptr && "trying to get pointer before allocation");
    return entry;
}

uintptr_t AlignedMem::as_uint() const
{
    return reinterpret_cast<uintptr_t>(entry);
}
