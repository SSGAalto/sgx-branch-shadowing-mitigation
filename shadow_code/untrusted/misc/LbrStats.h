/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_LBRSTATS_H
#define SAMPLE_LBRSTATS_H

#include <list>
#include "MiniStatistics.h"

class LbrStats {
public:
    LbrStats() = default;

    void add_from_lbr_data(struct lbr_data *, void *src_address);

    void add_measurement(bool hit, unsigned int cycles);
    int get_count() { return m_count; }
    double get_cycles_mean() { return m_cycles.mean(); }
    double get_cycles_stddev() { return m_cycles.stddev(); }
    double get_hits_mean() { return m_hits.mean(); }
    double get_hits_stddev() { return m_hits.stddev(); };

private:

    MiniStatistics m_hits;
    MiniStatistics m_cycles;
    int m_count;
};

typedef std::shared_ptr<LbrStats> LbrStats_p;

#endif //SAMPLE_LBRSTATS_H
