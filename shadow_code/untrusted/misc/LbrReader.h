/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_LBRREADER_H
#define SAMPLE_LBRREADER_H

#include <string>

#include "attributes.h"
#include "util.h"
#include "lbr_tools.h"

class LbrReader {
public:
    static constexpr const char *const device_filename = "/dev/lbr_dumper";

    always_inline
    static inline void dump_lbr_inline(int lbr_fd);

    LbrReader() : LbrReader(device_filename) {}
    explicit LbrReader(std::string device_filename);
    ~LbrReader();

    bool device_file_exists();
    bool open_device();
    void close_device();
    void dump_lbr();

    bool read_lbr();
    void print_lbr_data();

    struct lbr_data *get_data_ptr();

    int get_fd() { return m_fd_lbr; }
    char const *c_str() { return m_dev_fn.c_str(); }

    void print_lbr_data(void *src, void* shadow_src);

private:

    struct lbr_data *m_lbr_data = nullptr;
    std::string m_dev_fn;
    int m_fd_lbr = -1;
};

typedef std::shared_ptr<LbrReader> LbrReader_p;

always_inline
inline void LbrReader::dump_lbr_inline(const int lbr_fd)
{
    asm volatile (""
                  lotsa_nops
                  lotsa_nops
                  lotsa_nops
                  "mov %[fd], %%edi\n\t"
                  "mov %[val], %%esi\n\t"
                  "mov %[sc_num], %%eax\n\t"
                  "syscall\n\t"
    : : [fd] "g" (lbr_fd), [val] "g" (1), [sc_num] "g" (0x10)
    : "eax", "edi", "esi" );
}


#endif //SAMPLE_LBRREADER_H
