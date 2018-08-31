/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <stdint.h>
#include <config.h>

#include "spdlog/spdlog.h"
#include "misc/LbrStats.h"
#include "misc/lbr_tools.h"

void LbrStats::add_measurement(const bool hit, const unsigned int cycles)
{
    m_hits.add(hit ? 1 : 0);
    m_cycles.add(cycles);
    m_count++;
}

void LbrStats::add_from_lbr_data(struct lbr_data *data, void *src_address)
{
    auto logger = get_ulogger();
    bool found = false;
    bool hit = false;
    unsigned int cycles = 0;

    for (int i = 0; i < lbr_count; i++) {
        struct lbr_data *d = &(data[i]);

        if (lbr_data_get_from(d) == (uintptr_t) src_address) {
            found = true;
            hit = lbr_data_get_mispred(d) != 0;
            cycles = lbr_data_get_cycle_count(d);
        }
    }

    if (found) {
        SPDLOG_TRACE(logger, "adding measurement %d %d", hit, cycles);
        add_measurement(hit, cycles);
    } else {
        logger->warn("unable to find requested data in LBR");
    }
}


