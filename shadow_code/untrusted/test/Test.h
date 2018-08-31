/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_TEST_H
#define SAMPLE_TEST_H

#include <cstdint>
#include <string>

#include "config.h"
#include "misc/Enclave.h"
#include "misc/LbrReader.h"
#include "misc/LbrStats.h"
#include "shadow/Shadow.h"

class Test {

public:
    Test() = default;

    virtual const std::string get_name() const = 0;
    virtual const std::string get_desc() const = 0;
    virtual void run() = 0;

private:
    void prepare_attack(void *src, void *dst);
    uintptr_t finish_attack(int retval);

    void *setup_jne_shadow();
    void *setup_jmp_shadow();
    void *setup_jmp_shadow_rev();

    logger_t log = get_ulogger();

    Enclave_p m_enclave;
    LbrReader_p m_lbrReader;
    LbrStats_p m_stats;
    Shadow_p m_shadow;

    bool m_dump_lbr = false;
    bool m_use_indirect_targets = false;
    int m_enclave_sgx_debug_flag = -1;
    int m_training_rounds = 10;

    int m_lbr_fd = 0;
    void *m_shadow_src;
    void *m_src = nullptr;
    void *m_dst = nullptr;
    sgx_enclave_id_t m_eid = 0;

    void **m_indirect_target_ptr = nullptr;
    void **m_indirect_shadow_input = nullptr;
};


#endif //SAMPLE_TEST_H
