/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstdarg>
#include "Logger.h"

#if defined(__cplusplus)
extern "C" {
#endif
void ocall_log(const int level, const char *str)
{
    static auto logger =  spdlog::stdout_color_mt("enclave");
    switch(level) {
        case DEBUGL_DEBUG:
            logger->debug(str);
            break;
        case DEBUGL_INFO:
            logger->info(str);
            break;
        case DEBUGL_WARN:
            logger->warn(str);
            break;
        case DEBUGL_CRITICAL:
            logger->critical(str);
            break;
        default:
            logger->critical("unknown logger level %d: %s", level, str);
    }
}
#if defined(__cplusplus)
}
#endif

deprecated
std::shared_ptr<spdlog::logger> get_logger()
{
    return spdlog::get("console");
}




