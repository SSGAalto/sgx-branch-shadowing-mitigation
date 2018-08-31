/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_LOGGER_H
#define SAMPLE_LOGGER_H

#include <memory>
#include "../../shared/common.h"
#include "spdlog/spdlog.h"
#include "attributes.h"

#define logger_type std::shared_ptr<spdlog::logger>

std::shared_ptr<spdlog::logger> get_logger();

#if defined(__cplusplus)
extern "C" {
#endif


#if defined(__cplusplus)
}
#endif


#endif //SAMPLE_LOGGER_H
