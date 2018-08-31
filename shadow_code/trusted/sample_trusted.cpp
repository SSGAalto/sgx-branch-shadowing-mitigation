/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include "sample_trusted.h"

#include <stdarg.h>

#include <cstdio>
#include <stdint.h>
#include <stdint.h>
#include "../shared/common.h"

/* Ignore CannotResolve errors for generated files. */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "CannotResolve"
#include "sample_t.h"
#pragma clang diagnostic pop

#define VICTIM_FUNC(x) e_##x
#include "../shared/victim_shared.c"

void ecall_setup()
{
    ocall_set_branch_victim_jne(
            static_cast<uint64_t>((long) &e_victim_jne),
            static_cast<uint64_t>((long) e_get_victim_jne_src()),
            static_cast<uint64_t>((long) e_get_victim_jne_dst())
    );

    ocall_set_branch_victim_ret(
            static_cast<uint64_t>((long) &e_victim_ret),
            static_cast<uint64_t>((long) e_get_victim_ret_retpoline()),
            static_cast<uint64_t>((long) e_get_victim_ret_if()),
            static_cast<uint64_t>((long) e_get_victim_ret_else())
    );
}

void ecall_victim_jne(int number)
{
    e_victim_jne(number);
}

void ecall_victim_ret(int number)
{
    e_victim_ret(number);
}

int ecall_victim_jne_test(int input)
{
    return e_victim_jne(input);
}

int ecall_victim_ret_test(int input)
{
    return e_victim_ret(input);
}
