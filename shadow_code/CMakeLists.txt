# Author: Hans Liljestrand, Shohreh Hosseinzadeh
# Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
# This code is released under Apache 2.0 license

cmake_minimum_required (VERSION 2.8.11)
project (sample)

set(CMAKE_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES} PreRelease)

# Default to debug build
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Default location for private key (a dummy key is generated if this doesn't exist)
set(private_key ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}_private.pem)

# Set the output path
set(output_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
if (NOT DEFINED output_path)
    set(output_path ${CMAKE_BINARY_DIR})
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${output_path})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${output_path}/lib)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${output_path}/lib)
set(SGX_ENCLAVE_OUTPUT_DIRECTORY ${output_path}/trusted)

# Load in custom cmake stuff
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# Find the sgxsdk
find_package(SGXSDK REQUIRED)
#message(STATUS "SGX_COMMON_CFLAGS: ${SGX_COMMON_CFLAGS}")
#message(STATUS "SGX_SDK: ${SGX_SDK}")
#message(STATUS "SGX_ARCH: ${SGX_ARCH}")
#message(STATUS "SGX_LIBRARY_PATH: ${SGX_LIBRARY_PATH}")
#message(STATUS "SGX_ENCLAVE_SIGNER: ${SGX_ENCLAVE_SIGNER}")
#message(STATUS "SGX_EDGER8R: ${SGX_EDGER8R}")
#message(STATUS "SGXSDK_URTS_SIM_LIB: ${SGXSDK_URTS_SIM_LIB}")
#message(STATUS "SGXSDK_USVC_SIM_LIB: ${SGXSDK_USVC_SIM_LIB}")
#message(STATUS "SGXSDK_URTS_LIB: ${SGXSDK_URTS_LIB}")
#message(STATUS "SGXSDK_USVC_LIB: ${SGXSDK_USVC_LIB}")

# If we don't have a ${private_key}, then just create a dummy one
if(NOT EXISTS ${private_key})
    set(private_key ${output_path}/${PROJECT_NAME}_private.pem)

    add_custom_command(OUTPUT ${private_key}
            COMMAND openssl genrsa -3 -out ${private_key} 3072
            COMMENT "WARNING: generating new private key")

    add_custom_target(privatekey ALL DEPENDS ${private_key})
else()
    add_custom_target(privatekey ALL)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/shared/build.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/shared/build.h @ONLY)

add_subdirectory(trusted)
add_subdirectory(untrusted)
add_subdirectory(scripts)
