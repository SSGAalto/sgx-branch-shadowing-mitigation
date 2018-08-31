/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SHADOW_CODE_CONFIG_H
#define SHADOW_CODE_CONFIG_H

#define lbr_entry_bits 32

#include "spdlog/spdlog.h"

typedef std::shared_ptr<spdlog::logger> logger_t;

static inline logger_t get_ulogger() { return spdlog::get("console"); }
static inline logger_t get_elogger() { return spdlog::get("enclave"); }

#endif /* SHADOW_CODE_CONFIG_H */
