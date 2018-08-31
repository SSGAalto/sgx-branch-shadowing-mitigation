/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef VICTIM_H
#define VICTIM_H

#include <sgx_eid.h>

#include "misc/attributes.h"

#ifdef __cplusplus
extern "C" {
#endif

int victim_jne(int input);
void *get_victim_jne_src(void);
void *get_victim_jne_dst(void);

void *get_victim_enc_jne_src(void);
void *get_victim_enc_jne_dst(void);

void *get_victim_enc_ret_ret(void);
void *get_victim_enc_ret_if(void);
void *get_victim_enc_ret_else(void);

noinline int victim_ret(int number);
void *get_victim_ret_retpoline(void);
void *get_victim_ret_else(void);
void *get_victim_ret_if(void);

#ifdef __cplusplus
}
#endif
#endif /* !VICTIM_H */
