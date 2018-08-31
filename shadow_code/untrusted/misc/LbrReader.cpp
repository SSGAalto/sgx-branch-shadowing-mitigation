/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <string>
#include <sys/stat.h>
#include <zconf.h>
#include <fcntl.h>
#include <cstring>
#include <cassert>
#include <config.h>
#include <spdlog/spdlog.h>

#include "LbrReader.h"

LbrReader::LbrReader(const std::string device_filename)
: m_dev_fn(device_filename)
{}

LbrReader::~LbrReader()
{
    if (m_lbr_data != nullptr) {
        free(m_lbr_data);
    }
    close_device();
}

bool LbrReader::device_file_exists()
{
    struct stat buffer = {'\0'};
    return (stat(m_dev_fn.c_str(), &buffer) == 0);
}

bool LbrReader::open_device()
{
    auto logger = get_ulogger();

    if (!device_file_exists()) {
        logger->warn("open aborted, cannot find device file %s\n", m_dev_fn.c_str());
        return false;
    }

    logger->debug("Trying to open %s", c_str());
    m_fd_lbr = open(c_str(), O_RDONLY);
    if (m_fd_lbr < 0) {
        logger->critical("Failed to open %s: %s (%d)",
                         c_str(), strerror(m_fd_lbr), m_fd_lbr);
        return false;
    }

    return true;
}

void LbrReader::dump_lbr()
{
    auto logger = get_ulogger();
    assert(m_fd_lbr > 0 && "LBR file descriptor is larger than 0");
    logger->debug("Preparing to dump LBR using fd %d", m_fd_lbr);
    dump_lbr_inline(m_fd_lbr);
}

void LbrReader::close_device()
{
    if (m_fd_lbr > 0)
        close(m_fd_lbr);
    m_fd_lbr = -1;
}

bool LbrReader::read_lbr() {
    auto logger = get_ulogger();
    if (m_fd_lbr < 1)
        return false;

    if (m_lbr_data == nullptr) {
        m_lbr_data = (struct lbr_data *)malloc(sizeof(struct lbr_data) * lbr_count);
        if (m_lbr_data == nullptr) {
            logger->critical("Failed to allocated memory for LBR data");
            abort();
        }
        logger->debug("allocated lbr data at %p", (void *)m_lbr_data);
    }

    for (int i = 0; i < lbr_count; i++) {
        read(m_fd_lbr, &(m_lbr_data[i]), sizeof(struct lbr_data));
    }
    logger->debug("stored lbr data at %p", (void *)m_lbr_data);
}

void LbrReader::print_lbr_data()
{
    print_lbr_data(nullptr, nullptr);
}

void LbrReader::print_lbr_data(void *src, void *shadow_src)
{
    for (int i = 0; i < lbr_count; i++) {
        auto data = &(m_lbr_data[i]);

        auto *i_src = reinterpret_cast<void *>(lbr_data_get_from(data));

        const char *color = (i_src == src ?
                        "\x1B[32m" // green
                        : (i_src == shadow_src ?
                          "\x1B[34m" // blue
                          : "\x1B[0m"));

        printf("%s0x%016lx -> 0x%016lx [%s] cycles: %u\x1B[0m\n",
               color,
               lbr_data_get_from(data),
               lbr_data_get_to(data),
               lbr_data_get_mispred(data) != 0 ? "MISS" : "____",
               lbr_data_get_cycle_count(data)
        );
    }

}

struct lbr_data *LbrReader::get_data_ptr()
{
    assert(m_lbr_data != nullptr && "struct lbr_data * is NULL, check return value of read_lbr!!");
    return m_lbr_data;
}


