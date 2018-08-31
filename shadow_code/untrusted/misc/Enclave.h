/*
 * Author: Hans Liljestrand, Shohreh Hosseinzadeh
 * Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
 * This code is released under Apache 2.0 license
 */

#ifndef SAMPLE_ENCLAVE_INIT_H
#define SAMPLE_ENCLAVE_INIT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>

#include "sgx_error.h"       /* sgx_status_t */
#include "sgx_eid.h"     /* sgx_enclave_id_t */
#include <sgx_urts.h>
#include <iostream>
#include "Enclave.h"

#include "sample_u.h"

class Enclave {
public:
    static constexpr const char *const TOKEN_FILENAME = "enclave.token";
    static constexpr const char *const SAMPLE_FILENAME = SGX_ENCLAVE_NAME;

    Enclave() = default;
    ~Enclave();

    sgx_enclave_id_t initialize_enclave();

    bool exfiltrate_addresses();

    int test_victim_jne(int input);
    int test_victim_ret(int input);

    inline void call_victim_jne(int input);
    inline void call_victim_ret(int input);

    static inline std::string sgx_status_tostring(enum _status_t);
    static void print_error_message(sgx_status_t err);

    inline void set_debug_flag(int d) { m_debug_flag = d; }
    sgx_enclave_id_t get_eid() { return m_eid; }

private:
    bool save_token(sgx_launch_token_t *);
    bool open_token();
    bool close_token();

    sgx_enclave_id_t m_eid = 0;
    FILE *m_token_fp = nullptr;
    int m_debug_flag = SGX_DEBUG_FLAG;
    std::string m_token_path;
    std::string m_token_fn = TOKEN_FILENAME;
    std::string m_enclave_fn = SAMPLE_FILENAME;
};

typedef std::shared_ptr<Enclave> Enclave_p;

/* Check error conditions for loading enclave */
inline void Enclave::print_error_message(sgx_status_t ret)
{
    std::cout << sgx_status_tostring(ret) << "\n";
}

inline void Enclave::call_victim_jne(const int input)
{
    ecall_victim_jne(m_eid, input);
}

inline void Enclave::call_victim_ret(const int input)
{
    ecall_victim_ret(m_eid, input);
}

inline std::string Enclave::sgx_status_tostring(enum _status_t status)
{
    switch(status) {
    case SGX_SUCCESS: return "SGX_SUCCESS:  Success";
    case SGX_ERROR_UNEXPECTED: return "SGX_ERROR_UNEXPECTED: Unexpected error";
    case SGX_ERROR_INVALID_PARAMETER: return "SGX_ERROR_INVALID_PARAMETER:  The parameter is incorrect";
    case SGX_ERROR_OUT_OF_MEMORY: return "SGX_ERROR_OUT_OF_MEMORY:  Not enough memory is available to complete this operation";
    case SGX_ERROR_ENCLAVE_LOST: return "SGX_ERROR_ENCLAVE_LOST:  Enclave lost after power transition or used in child process created by linux:fork()";
    case SGX_ERROR_INVALID_STATE: return "SGX_ERROR_INVALID_STATE:  SGX API is invoked in incorrect order or state";
    case SGX_ERROR_INVALID_FUNCTION: return "SGX_ERROR_INVALID_FUNCTION:  The ecall/ocall index is invalid";
    case SGX_ERROR_OUT_OF_TCS: return "SGX_ERROR_OUT_OF_TCS:  The enclave is out of TCS";
    case SGX_ERROR_ENCLAVE_CRASHED: return "SGX_ERROR_ENCLAVE_CRASHED:  The enclave is crashed";
    case SGX_ERROR_ECALL_NOT_ALLOWED: return "SGX_ERROR_ECALL_NOT_ALLOWED:  The ECALL is not allowed at this time, e.g. ecall is blocked by the dynamic entry table, or nested ecall is not allowed during initialization";
    case SGX_ERROR_OCALL_NOT_ALLOWED: return "SGX_ERROR_OCALL_NOT_ALLOWED:  The OCALL is not allowed at this time, e.g. ocall is not allowed during exception handling";
    case SGX_ERROR_STACK_OVERRUN: return "SGX_ERROR_STACK_OVERRUN:  The enclave is running out of stack";
    case SGX_ERROR_UNDEFINED_SYMBOL: return "SGX_ERROR_UNDEFINED_SYMBOL:  The enclave image has undefined symbol.";
    case SGX_ERROR_INVALID_ENCLAVE: return "SGX_ERROR_INVALID_ENCLAVE:  The enclave image is not correct.";
    case SGX_ERROR_INVALID_ENCLAVE_ID: return "SGX_ERROR_INVALID_ENCLAVE_ID:  The enclave id is invalid";
    case SGX_ERROR_INVALID_SIGNATURE: return "SGX_ERROR_INVALID_SIGNATURE:  The signature is invalid";
    case SGX_ERROR_NDEBUG_ENCLAVE: return "SGX_ERROR_NDEBUG_ENCLAVE:  The enclave is signed as product enclave, and can not be created as debuggable enclave.";
    case SGX_ERROR_OUT_OF_EPC: return "SGX_ERROR_OUT_OF_EPC:  Not enough EPC is available to load the enclave";
    case SGX_ERROR_NO_DEVICE: return "SGX_ERROR_NO_DEVICE:  Can't open SGX device";
    case SGX_ERROR_MEMORY_MAP_CONFLICT: return "SGX_ERROR_MEMORY_MAP_CONFLICT: Page mapping failed in driver";
    case SGX_ERROR_INVALID_METADATA: return "SGX_ERROR_INVALID_METADATA:  The metadata is incorrect.";
    case SGX_ERROR_DEVICE_BUSY: return "SGX_ERROR_DEVICE_BUSY:  Device is busy, mostly EINIT failed.";
    case SGX_ERROR_INVALID_VERSION: return "SGX_ERROR_INVALID_VERSION:  Metadata version is inconsistent between uRTS and sgx_sign or uRTS is incompatible with current platform.";
    case SGX_ERROR_MODE_INCOMPATIBLE: return "SGX_ERROR_MODE_INCOMPATIBLE:  The target enclave 32/64 bit mode or sim/hw mode is incompatible with the mode of current uRTS.";
    case SGX_ERROR_ENCLAVE_FILE_ACCESS: return "SGX_ERROR_ENCLAVE_FILE_ACCESS:  Can't open enclave file.";
    case SGX_ERROR_INVALID_MISC: return "SGX_ERROR_INVALID_MISC:  The MiscSelct/MiscMask settings are not correct";
   // case SGX_ERROR_INVALID_LAUNCH_TOKEN: return "SGX_ERROR_INVALID_LAUNCH_TOKEN:  The launch token is not correct";
    case SGX_ERROR_MAC_MISMATCH: return "SGX_ERROR_MAC_MISMATCH:  Indicates verification error for reports, sealed datas, etc";
    case SGX_ERROR_INVALID_ATTRIBUTE: return "SGX_ERROR_INVALID_ATTRIBUTE:  The enclave is not authorized";
    case SGX_ERROR_INVALID_CPUSVN: return "SGX_ERROR_INVALID_CPUSVN:  The cpu svn is beyond platform's cpu svn value";
    case SGX_ERROR_INVALID_ISVSVN: return "SGX_ERROR_INVALID_ISVSVN:  The isv svn is greater than the enclave's isv svn";
    case SGX_ERROR_INVALID_KEYNAME: return "SGX_ERROR_INVALID_KEYNAME:  The key name is an unsupported value ";
    case SGX_ERROR_SERVICE_UNAVAILABLE: return "SGX_ERROR_SERVICE_UNAVAILABLE:  Indicates aesm didn't respond or the requested service is not supported";
    case SGX_ERROR_SERVICE_TIMEOUT: return "SGX_ERROR_SERVICE_TIMEOUT:  The request to aesm timed out";
    case SGX_ERROR_AE_INVALID_EPIDBLOB: return "SGX_ERROR_AE_INVALID_EPIDBLOB:  Indicates epid blob verification error";
    case SGX_ERROR_SERVICE_INVALID_PRIVILEGE: return "SGX_ERROR_SERVICE_INVALID_PRIVILEGE:  Enclave has no privilege to get launch token";
    case SGX_ERROR_EPID_MEMBER_REVOKED: return "SGX_ERROR_EPID_MEMBER_REVOKED:  The EPID group membership is revoked.";
    case SGX_ERROR_UPDATE_NEEDED: return "SGX_ERROR_UPDATE_NEEDED:  SGX needs to be updated";
    case SGX_ERROR_NETWORK_FAILURE: return "SGX_ERROR_NETWORK_FAILURE:  Network connecting or proxy setting issue is encountered";
    case SGX_ERROR_AE_SESSION_INVALID: return "SGX_ERROR_AE_SESSION_INVALID:  Session is invalid or ended by server";
    case SGX_ERROR_BUSY: return "SGX_ERROR_BUSY:  The requested service is temporarily not availabe";
    case SGX_ERROR_MC_NOT_FOUND: return "SGX_ERROR_MC_NOT_FOUND:  The Monotonic Counter doesn't exist or has been invalided";
    case SGX_ERROR_MC_NO_ACCESS_RIGHT: return "SGX_ERROR_MC_NO_ACCESS_RIGHT:  Caller doesn't have the access right to specified VMC";
    case SGX_ERROR_MC_USED_UP: return "SGX_ERROR_MC_USED_UP:  Monotonic counters are used out";
    case SGX_ERROR_MC_OVER_QUOTA: return "SGX_ERROR_MC_OVER_QUOTA:  Monotonic counters exceeds quota limitation";
    case SGX_ERROR_KDF_MISMATCH: return "SGX_ERROR_KDF_MISMATCH:  Key derivation function doesn't match during key exchange";
    case SGX_ERROR_UNRECOGNIZED_PLATFORM: return "SGX_ERROR_UNRECOGNIZED_PLATFORM:  EPID Provisioning failed due to platform not recognized by backend serve";
    //case SGX_ERROR_NO_PRIVILEGE: return "SGX_ERROR_NO_PRIVILEGE:  Not enough privilege to perform the operation";
    case SGX_ERROR_FILE_BAD_STATUS: return "SGX_ERROR_FILE_BAD_STATUS:  The file is in bad status, run sgx_clearerr to try and fix it";
    case SGX_ERROR_FILE_NO_KEY_ID: return "SGX_ERROR_FILE_NO_KEY_ID:  The Key ID field is all zeros, can't re-generate the encryption key";
    case SGX_ERROR_FILE_NAME_MISMATCH: return "SGX_ERROR_FILE_NAME_MISMATCH:  The current file name is different then the original file name (not allowed, substitution attack)";
    case SGX_ERROR_FILE_NOT_SGX_FILE: return "SGX_ERROR_FILE_NOT_SGX_FILE:  The file is not an SGX file";
    case SGX_ERROR_FILE_CANT_OPEN_RECOVERY_FILE: return "SGX_ERROR_FILE_CANT_OPEN_RECOVERY_FILE:  A recovery file can't be opened, so flush operation can't continue (only used when no EXXX is returned) ";
    case SGX_ERROR_FILE_CANT_WRITE_RECOVERY_FILE: return "SGX_ERROR_FILE_CANT_WRITE_RECOVERY_FILE:  A recovery file can't be written, so flush operation can't continue (only used when no EXXX is returned) ";
    case SGX_ERROR_FILE_RECOVERY_NEEDED: return "SGX_ERROR_FILE_RECOVERY_NEEDED:  When openeing the file, recovery is needed, but the recovery process failed";
    case SGX_ERROR_FILE_FLUSH_FAILED: return "SGX_ERROR_FILE_FLUSH_FAILED:  fflush operation (to disk) failed (only used when no EXXX is returned)";
    case SGX_ERROR_FILE_CLOSE_FAILED: return "SGX_ERROR_FILE_CLOSE_FAILED:  fclose operation (to disk) failed (only used when no EXXX is returned)";
    default: return "UNKNOWN_SGX_ERROR";
}
    
}

#endif //SAMPLE_ENCLAVE_INIT_H
