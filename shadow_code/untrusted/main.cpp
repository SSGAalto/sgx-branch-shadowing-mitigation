/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <iostream>
#include "args.hxx"

#include "misc/Logger.h"
#include "misc/util.h"
#include "BranchShadow.h"
#include "../shared/build.h"

/* Application entry */
int main(int argc, char *argv[])
{
    try {
        /* Setup argument parsing */
        args::ArgumentParser parser("Branch shadowing tester", "Will the code work, or will it not, nobody knows :(");
        args::HelpFlag help(parser, "help", "Display this help menu",
                            {'h', "help"});
        args::Group logger_args(parser, "This group is all exclusive:",
                                args::Group::Validators::DontCare);
        args::Flag f_debug(logger_args, "debug", "Enable debug output",
                           {'d', "debug"});
        args::Flag f_minimal(logger_args, "minimal", "Use minimal output",
                             {'m', "minimal"});
        args::Flag f_quiet(logger_args, "quiet", "Disable most output",
                           {'q', "quiet"});
        args::Flag f_no_sgx(parser, "no_sgx", "Do not use SGX",
                            {'n'});
        args::Flag f_dump_lbr(parser, "dump_lbr", "Dump LBR to stdout",
                            {'l'});
        args::Flag f_no_indirect(parser, "no_indirect", "Do not use indirect pointers",
                              {'i'});
        args::ValueFlag<int> f_victim_input(parser, "victim_input", "Input for the victim function",
                                            {'v'});
        args::ValueFlag<int> f_shadow_input(parser, "shadow_input", "Input for the shadow function",
                                            {'s'});
        args::ValueFlag<int> f_test_type(parser, "test_num",
                                         "The test to run (see main.cpp)\n"
                                         "1: non-enclave jne shadow "
                                         "(default)\n"
                                         "2: non-enclave ret shadow\n"
                                         "3: enclave jne shadow\n"
                                         "4: enclave ret shadow\n"
                                         "5: test EENTER/EEXIT",
                                         {'t'});
        args::ValueFlag<int> f_override_sgx_debug(parser, "overrdie_sgx_debug_flag",
                                                  "Override the SGX_DEBUG_FLAG passed into"
                                                  " sgx_enclave_create", {'o'});

        try {
            parser.ParseCLI(argc, argv);
        } catch (args::Help) {
            std::cout << parser;
            return 0;
        } catch (args::ParseError e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return 1;
        } catch (args::ValidationError e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return 1;
        }


#ifdef SPDLOG_TRACE_ON
        spdlog::set_level(spdlog::level::trace);
#define warn_trace() { \
        logger->warn("SPDLOG_TRACE_ON (disable in lib/spdlog/tweakme.h)"); \
        logger->critical("This can affect test results due to extra logging calls!"); \
        logger->warn("Disable by uncommenting SPDLOG_TRACE_ON in lib/spdlog/tweakme.h"); \
}
#else
#define warn_trace()
        spdlog::set_pattern("[%L] [%n] %v");
        if (f_debug) {
            spdlog::set_level(spdlog::level::debug);
        } else if (f_quiet) {
            spdlog::set_level(spdlog::level::warn);
        } else if (f_minimal) {
            spdlog::set_level(spdlog::level::critical);
        } else {
            spdlog::set_level(spdlog::level::info);
        }
#endif
        /* Initialize logging */
        const auto logger = spdlog::stdout_color_st("console");
        warn_trace();
        logger->info("This is a %s build", CMAKE_BUILD_TYPE);

        /* chdir to where our binary is */
        chwd_to_binary(argv[0]);

        /* Either set the test config to default or use commandline args */
        int victim_input = (f_victim_input ? args::get(f_victim_input) : 1);
        int shadow_input = (f_shadow_input ? args::get(f_shadow_input) : 1);
        int test_type = (f_test_type ? args::get(f_test_type) : 1);

        if (f_no_sgx) {
            if (test_type < 1 || test_type > 2) {
                logger->critical("Bad test type with option -n");
                abort();
            }
        }

        /* Get the BranchShadow object */
        logger->debug("Getting BranchShadow object");
        auto bs = BranchShadow::getInstance();

        /* Configure the BranchShadow object */
        bs->set_training_rounds(100);
        if (f_override_sgx_debug)
            bs->set_sgx_debug_flag(args::get(f_override_sgx_debug));
        if (f_dump_lbr)
            bs->set_dump_lbr_to_stdout(true);
        bs->set_use_indirect_targets(f_no_indirect ? false : true);

        /* Run setup and print config */
        bs->prepare_lbr();
        if (!f_no_sgx)
            bs->prepare_enclave();
        bs->print_config();

        /* Sanity check */
        bs->sanity_check_victims();

        logger->info("Running on CPU %d, using inputs v: %d, s: %d",
                     smp_processor_id(), victim_input, shadow_input);

        uint64_t retval = -1;
        switch (test_type) {
            case 1:
                retval = bs->t1_run_jne(victim_input, shadow_input);
                break;
            case 2:
                retval = bs->t2_run_ret(victim_input, shadow_input);
                break;
            case 3:
                retval = bs->t3_run_enc_jne(victim_input, shadow_input);
                break;
            case 4:
                retval = bs->t4_run_enc_ret(victim_input, shadow_input);
                break;
            case 5:
                retval = bs->t5_run_ret2(victim_input, shadow_input);
                break;
            case 6:
                retval = bs->t6_run_ret2_check(victim_input, shadow_input);
                break;
            case 7:
                retval = bs->t7_run_ret_jmp(victim_input, shadow_input);
                break;
            case 8:
                retval = bs->t8_run_ret_jmp_to_other(victim_input, shadow_input);
                break;
            case 9:
                retval = bs->t9_run_jne_jmp(victim_input, shadow_input);
                break;
            case 10:
                retval = bs->t10_run_ret_reverse_shadow_input(victim_input, shadow_input);
                break;
            case 11:
                retval = bs->t11_run_ret_jmp(victim_input, shadow_input);
                break;
            case 12:
                retval = bs->t12_run_enc_ret_jmp(victim_input, shadow_input);
                break;
            case 13:
                retval = bs->t13_run_enc_ret(victim_input, shadow_input);
                break;
            default:
                logger->critical("Bad test type %d, executing default test\n", test_type);
                abort();
        }

        if (f_minimal) {
            std::cout << std::hex << retval;
        }

        warn_trace();
        return 0;
    }
    catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return 1;
    }

}

