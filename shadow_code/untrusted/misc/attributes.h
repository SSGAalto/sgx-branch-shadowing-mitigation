/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#ifndef noinline
#define noinline __attribute__((noinline))
#endif

#ifndef always_inline
#define always_inline __attribute__((always_inline))
#endif

#ifndef deprecated
#define deprecated __attribute__((deprecated))
#endif

#endif /* !ATTRIBUTES_H */
