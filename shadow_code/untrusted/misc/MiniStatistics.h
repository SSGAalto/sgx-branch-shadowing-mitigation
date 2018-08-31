/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_MINISTATISTICS_H
#define SAMPLE_MINISTATISTICS_H

#include <list>

class MiniStatistics {
public:
    MiniStatistics() = default;

    void add(double);

    unsigned int count();
    double mean();
    double stddev();

private:

    std::list<double> m_vals;
    unsigned int m_count = 0;
    double m_total = 0.0;
};


#endif //SAMPLE_MINISTATISTICS_H
