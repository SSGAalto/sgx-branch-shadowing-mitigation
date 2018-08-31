/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_BRANCH_SHADOWING_H
#define SAMPLE_BRANCH_SHADOWING_H

#include <linux/perf_event.h>
#include <shadow/Shadow.h>

#include "misc/Enclave.h"
#include "misc/LbrReader.h"
#include "misc/LbrStats.h"
#include "misc/Logger.h"

class BranchShadow {
public:
    static std::shared_ptr<BranchShadow> getInstance();

public:

    BranchShadow();
    ~BranchShadow();

    void set_training_rounds(int val) { m_training_rounds = val; }
    void set_sgx_debug_flag(int val) { m_enclave_sgx_debug_flag = val; }
    void set_dump_lbr_to_stdout(bool val) { m_dump_lbr = val; };
    void set_use_indirect_targets(bool val) { m_use_indirect_targets = val; };
    void prepare_enclave();
    void prepare_lbr();

    void print_config();
    bool sanity_check_victims();

    /*
    uint64_t run_enc(sgx_status_t(*victim_func)(sgx_enclave_id_t, int),
                     struct shadow_area *, int victim_input, int shadow_input);
    uint64_t run_plain(int (*victim_func)(int), struct shadow_area *sa,
                       int victim_input, int shadow_input);
    */

    uint64_t t1_run_jne(int victim_input, int shadow_input);
    uint64_t t2_run_ret(int victim_input, int shadow_input);
    uint64_t t3_run_enc_jne(int victim_input, int shadow_input);
    uint64_t t4_run_enc_ret(int victim_input, int shadow_input);
    uint64_t t5_run_ret2(int victim_input, int shadow_input);
    uint64_t t6_run_ret2_check(int victim_input, int shadow_input);
    uint64_t t7_run_ret_jmp(int victim_input, int shadow_input);
    uint64_t t8_run_ret_jmp_to_other(int victim_input, int shadow_input);
    uint64_t t9_run_jne_jmp(int victim_input, int shadow_input);
    uint64_t t10_run_ret_reverse_shadow_input(int victim_input, int shadow_input);
    uint64_t t11_run_ret_jmp(int victim_input, int shadow_input);
    uint64_t t12_run_enc_ret_jmp(int victim_input, int shadow_input);
    uint64_t t13_run_enc_ret(int victim_input, int shadow_input);

private:

    void prepare_attack(void *src, void *dst, int shadow_input);
    uintptr_t finish_attack(int retval);

    void *setup_jne_shadow();
    void *setup_jmp_shadow();
    void *setup_jmp_shadow_rev();

    /**
     * Number of least significant bits used for LBR addressing.
     */
    const logger_type logger = get_logger();
    std::shared_ptr<Enclave> m_enclave;
    std::shared_ptr<LbrReader> m_lbrReader;
    std::shared_ptr<LbrStats> m_stats;

    int m_lbr_fd = 0;
    void *m_src = nullptr;
    void *m_dst = nullptr;
    sgx_enclave_id_t m_eid = 0;

    void **m_indirect_target_ptr = nullptr;
    void **m_indirect_shadow_input = nullptr;

    bool m_dump_lbr = false;
    bool m_use_indirect_targets = false;

    int m_enclave_sgx_debug_flag = -1;
    int m_training_rounds = 100;

    std::shared_ptr<Shadow> m_shadow;
    void *m_shadow_src;
};


#endif //SAMPLE_BRANCH_SHADOWING_H
