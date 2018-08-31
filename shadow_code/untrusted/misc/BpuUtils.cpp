/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstdlib>
#include <emmintrin.h>
#include "BpuUtils.h"
#include "Logger.h"
#include "mem/AlignedMem.h"
#include "config.h"

/* Keep this separate to ease debugging */
static inline void enter_the_slide(void *entry)
{
    asm volatile (""
                  "call *%[Entry]"
    : : [Entry] "g" (entry) : );
}

/* Keep this separate to ease debugging */
static inline void enter_the_repeat(void *entry, unsigned int val)
{
    asm volatile ("mov %[Val], %%eax\n\t"
                  "call *%[Entry]\n\t"
    : :
    [Val] "g"(val),
    [Entry] "g"(entry)
    : "eax", "rcx" );
}

void BpuUtils::mess_btb(void *const target)
{
    assert(target != nullptr && "make sure target is not NULL");
    auto logger = get_logger();
    logger->debug("%s: Preparing to mess with predictor", __FUNCTION__);
    do_sequential_jumps(ASSUMED_BTB_ENTRY_COUNT, (void*)((uintptr_t) target - 128));
    do_repeated_jumps(target);
    logger->debug("%s: Done jumping here and there", __FUNCTION__);
}

void BpuUtils::do_sequential_jumps(const size_t entries, void *target)
{
    auto logger = get_logger();
    logger->trace("Creating and executing %d entry jump-slide", entries);

    const size_t jump_size = entries * 6;

    auto mem = new AlignedMem(target, jump_size + 20, lbr_entry_bits);
    if (! mem->allocate()) {
        logger->critical("Failed to allocate memory");
        abort();
    }

    auto *const jump_slide = static_cast<char *>(mem->as_ptr());

    /* Need to do with two offsets since a jmp is two bytes */
    for (int offset = 0; offset < 2; offset++) {
        for (int i = offset; i < jump_size - 6; i += 6) {
            jump_slide[i + 0] = '\xeb'; // jmp +2 --------\ entry
            jump_slide[i + 1] = '\x02'; //                |
            jump_slide[i + 2] = '\xeb'; // <-----------\  |
            jump_slide[i + 3] = '\x02'; // jmp +2 --\  |  |
            jump_slide[i + 4] = '\xeb'; // <--------|--|--/
            jump_slide[i + 5] = '\xfc'; // jmp -2 --|--/
                                        // <--------/

            /* Enter jmpq *%rax to jump back to %rax */
            jump_slide[i + 6] = '\xc3';
        }

        void *entry = &(jump_slide[offset]);

        get_logger()->trace("%s: entering slide at %p %p (%d)", __FUNCTION__,
                            mem->as_ptr(), entry, offset);
                            //&(jump_slide[offset]), offset);

        // enter_the_slide(&(jump_slide[offset]));
        enter_the_slide(entry);
    }

    mem->unallocate(); // Do manually to prevent GB trigger
}

void BpuUtils::do_repeated_jumps(void *target)
{
    auto logger = get_logger();
    logger->trace("doing repeated random jumps at %p", __FUNCTION__, target);

    constexpr const size_t func_size = 22;
    constexpr const char *func =
            "\x83\xe0\x01"                  /* 3 and $0x1, %eax   */
            "\x85\xc0"                      /* 2 test %eax, %eax  */
            "\x75\x07"                      /* 2 jne 7 */
            "\x48\xc7\xc1\x01\x00\x00\x00"  /* 7 mov    $0x1,%rcx */
            "\x48\xc7\xc1\x02\x00\x00\x00"  /* 7 mov    $0x2,%rcx */
            "\xc3";                         /* 1 retq */

    auto mem = new AlignedMem(target, strlen(func), lbr_entry_bits);
    if (! mem->allocate()) {
        logger->critical("Failed to allocate memory");
        abort();
    }

    memcpy(mem->as_ptr(), func, func_size);

    logger->debug("%s: starting the jump loop", __FUNCTION__);
    for (int j = 0; j < 10; j++) {
        auto val = static_cast<unsigned int>(rand());

        for (unsigned int i = RAND_MAX; i > 0; i = i >> 1UL, val = val >> 1UL) {
            enter_the_repeat(mem->as_ptr(), val);
        }
    }

    mem->unallocate();
}

