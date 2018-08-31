/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cmath>
#include "MiniStatistics.h"

unsigned int MiniStatistics::count()
{
    return m_count;
}

void MiniStatistics::add(double new_val)
{
    m_count++;
    m_vals.push_back(new_val);
    m_total += new_val;
}

double MiniStatistics::mean()
{
    return m_total / m_count;
}

double MiniStatistics::stddev()
{
    const double avg = mean();

    double sdiffs = 0.0;

    for (auto v : m_vals) {
        sdiffs += (v - avg, 2);
    }

    return sqrt(sdiffs / m_count);
}
