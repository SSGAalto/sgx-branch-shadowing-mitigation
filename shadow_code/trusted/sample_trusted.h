/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include <stdlib.h>
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

void *e_get_victim_jne_src();
void *e_get_victim_jne_dst();
void *e_get_victim_ret_ret();
void *e_get_victim_ret_if();
void *e_get_victim_ret_else();

#if defined(__cplusplus)
}
#endif

#endif /* !_SAMPLE_H_ */
