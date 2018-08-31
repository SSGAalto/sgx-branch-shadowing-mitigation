#include <stdint.h>
#include <stdio.h>
#include <sample_u.h>
#include <assert.h>
#include <stdlib.h>

#include "victim.h"

static void *victim_enc_jne_func = NULL;
static void *victim_enc_jne_src = NULL;
static void *victim_enc_jne_dst = NULL;
static void *victim_enc_ret_func = NULL;
static void *victim_enc_ret_ret = NULL;
static void *victim_enc_ret_if = NULL;
static void *victim_enc_ret_else = NULL;

void ocall_set_branch_victim_jne(uint64_t func, uint64_t src, uint64_t dst)
{
    victim_enc_jne_func = (void *)func;
    victim_enc_jne_src = (void *)src;
    victim_enc_jne_dst = (void *)dst;

    assert((void *)src != NULL && "src is non-NULL");
    assert((void *)dst != NULL && "dst is non-NULL");
}

void ocall_set_branch_victim_ret(
        uint64_t func, uint64_t r, uint64_t i, uint64_t e)
{
    victim_enc_ret_func = (void *)func;
    victim_enc_ret_ret = (void *)r;
    victim_enc_ret_if = (void *)i;
    victim_enc_ret_else = (void *)e;
}
void *get_victim_enc_ret_ret(void) { return victim_enc_ret_ret; }
void *get_victim_enc_ret_if(void) { return victim_enc_ret_if; }
void *get_victim_enc_ret_else(void) { return victim_enc_ret_else; }

void *get_victim_enc_jne_src(void)
{
    return victim_enc_jne_src;
}

void *get_victim_enc_jne_dst(void)
{
    return victim_enc_jne_dst;
}

#define VICTIM_FUNC(x) x
#include "../shared/victim_shared.c"

