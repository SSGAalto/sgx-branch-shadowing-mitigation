/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include "Logger.h"

#include "sample_u.h"

#include "Enclave.h"

Enclave::~Enclave()
{
    if (m_eid != 0) {
        sgx_destroy_enclave(m_eid);
        m_eid = 0;
    }

    if (m_token_fp != nullptr)
        fclose(m_token_fp);
}

/* Initialize the enclave:
 *   Step 1: retrive the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
sgx_enclave_id_t Enclave::initialize_enclave() {
    logger_type logger = get_logger();

    sgx_launch_token_t token = {0};
    sgx_status_t ret;
    int updated = 0;

    /* Step 1: retrive the launch token saved by last transaction */
    if (!open_token()) {
        logger->critical("Failed to create/open the token file %s",
                         m_token_path.c_str());
        abort();
    }

    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    logger->info("Enclave at %s", m_enclave_fn.c_str());
    logger->debug("Token at %s", m_token_path.c_str());
    logger->debug("Using SGX_DEBUG_FLAG=%d", m_debug_flag);
    ret = sgx_create_enclave(m_enclave_fn.c_str(), m_debug_flag, &token,
                             &updated, &m_eid, nullptr);

    /* Just bail out on failure */
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        close_token();
        m_eid = 0;
        return 0;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated)
        save_token(&token);

    close_token();
    return m_eid;
}

bool Enclave::open_token()
{
    auto logger = get_logger();

    const std::string home_dir = getpwuid(getuid())->pw_dir;

    if (home_dir.length() > 0 && (home_dir.length() + strlen("/") +
                                  m_token_fn.length() + 2 <= FILENAME_MAX)) {
        /* compose the token path */
        m_token_path = home_dir + "/" + m_token_fn;
    } else {
        /* if token path is too long or $HOME is NULL */
        m_token_path = m_token_fn;
    }

    m_token_fp = fopen(m_token_path.c_str(), "rb");
    if (m_token_fp == nullptr) {
        m_token_fp = fopen(m_token_path.c_str(), "wb");
        if (m_token_fp == nullptr) {
            logger->warn("%s: failed to create/open the launch token file "
                                 "\"%s\".\n", __FUNCTION__, m_token_path.c_str());
            return false;
        }
    }

    return true;
}

bool Enclave::close_token()
{
    if (m_token_fp != nullptr) {
        fclose(m_token_fp);
        m_token_fp = nullptr;
    }
}

bool Enclave::save_token(sgx_launch_token_t *token)
{
    logger_type logger = get_logger();

    if (m_token_fp == nullptr) {
        logger->debug("Cannot save token, token_fp is null");
        return 0;
    }

    m_token_fp = freopen(m_token_path.c_str(), "wb", m_token_fp);
    if (m_token_fp != nullptr)  {
        size_t write_num = fwrite(token, 1, sizeof(*token), m_token_fp);

        if (write_num != sizeof(*token))
            logger->warn("%s: failed to save launch token to \"%s\".\n",
                         __FUNCTION__, m_token_path.c_str());
    }
}

bool Enclave::exfiltrate_addresses()
{
    auto logger = get_logger();
    sgx_status_t ret;

    logger->debug("calling ecall_setup");
    ret = ecall_setup(m_eid);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return false;
    }
    return true;
}

int Enclave::test_victim_jne(int input)
{
    int retval = -1;
    if (SGX_SUCCESS != ecall_victim_jne_test(m_eid, &retval, input))
        get_logger()->critical("%s: ecall_victim_jne_test failed");
    return retval;
}

int Enclave::test_victim_ret(int input)
{
    int retval = -1;
    if (SGX_SUCCESS != ecall_victim_ret_test(m_eid, &retval, input))
        get_logger()->critical("%s: ecall_victim_ret_test failed");
    return retval;
}
