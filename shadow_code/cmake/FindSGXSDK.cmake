# Author: Hans Liljestrand, Shohreh Hosseinzadeh
# Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
# This code is released under Apache 2.0 license

# File taken from https://github.com/bl4ck5un/mbedtls-SGX/blob/master/cmake/FindSGXSDK.cmake
# Licensed under the Apache License 2.0

# Try to find SGX SDK
#  SGXSDK_FOUND - system has sgx
#  SGXSDK_INCLUDE_DIRS - the sgxsdk include directory
#  SGXSDK_LIBRARIES - Link these to use sgxsdk
#

FIND_PATH(SGX_SDK sgxsdk "/opt/intel")
if (${SGX_SDK} EQUAL SGX_SDK-NOTFOUND)
  message(FATAL_ERROR "Could not find SGX SDK.")
else()
  set(SGX_SDK "${SGX_SDK}/sgxsdk")
  message("Found SGX SDK at ${SGX_SDK}")
endif()

# get arch information
execute_process(COMMAND getconf LONG_BIT OUTPUT_VARIABLE VAR_LONG_BIT)
string(STRIP ${VAR_LONG_BIT} VAR_LONG_BIT)
if (VAR_LONG_BIT STREQUAL 32)
    set(SGX_ARCH x86)
elseif(CMAKE_CXX_FLAGS MATCHES -m32)
    set(SGX_ARCH x86)
elseif (VAR_LONG_BIT STREQUAL 64)
    set(SGX_ARCH x64)
elseif(CMAKE_CXX_FLAGS MATCHES -m64)
    set(SGX_ARCH x64)
endif()
#messag(STATUS "VAR_LONG_BIT: \"${VAR_LONG_BIT}\"")
#

if (SGX_ARCH STREQUAL x86)
    set(SGX_COMMON_CFLAGS -m32)
    set(SGX_LIBRARY_PATH ${SGX_SDK}/lib)
elseif (SGX_ARCH STREQUAL x64)
    set(SGX_COMMON_CFLAGS -m64)
    set(SGX_LIBRARY_PATH ${SGX_SDK}/lib64)
else ()
    message(FATAL_ERROR "Unknown SGX architecture \"${SGX_ARCH}\"")
endif ()

find_path(SGXSDK_MAIN_INCLUDE_DIR sgx.h "${SGX_SDK}/include")
set(SGXSDK_INCLUDE_DIRS
        ${SGXSDK_MAIN_INCLUDE_DIR}
        ${SGXSDK_MAIN_INCLUDE_DIR}/stlport
        ${SGXSDK_MAIN_INCLUDE_DIR}/tlibc)

if (SGX_ARCH STREQUAL x86)
    find_library(SGXSDK_LIBRARIES libsgx_urts.so "${SGX_SDK}/lib")
    find_library(SGXSDK_URTS_SIM_LIB    libsgx_urts_sim.so "${SGX_SDK}/lib")
    find_library(SGXSDK_USVC_SIM_LIB    libsgx_uae_service_sim.so "${SGX_SDK}/lib")
    find_library(SGXSDK_URTS_LIB        libsgx_urts.so "${SGX_SDK}/lib")
    find_library(SGXSDK_USVC_LIB        libsgx_uae_service.so "${SGX_SDK}/lib")
elseif (SGX_ARCH STREQUAL x64)
    find_library(SGXSDK_LIBRARIES       libsgx_urts.so "${SGX_SDK}/lib64")
    find_library(SGXSDK_URTS_SIM_LIB    libsgx_urts_sim.so "${SGX_SDK}/lib64")
    find_library(SGXSDK_USVC_SIM_LIB    libsgx_uae_service_sim.so "${SGX_SDK}/lib64")
    find_library(SGXSDK_URTS_LIB        libsgx_urts.so "${SGX_SDK}/lib64")
    find_library(SGXSDK_USVC_LIB        libsgx_uae_service.so "${SGX_SDK}/lib64")
else ()
    message(FATAL_ERROR "Unknown SGX architecture ${SGX_ARCH}")
endif ()

# handle the QUIETLY and REQUIRED arguments and set LibODB_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SGXSDK DEFAULT_MSG
        SGXSDK_URTS_SIM_LIB SGXSDK_USVC_SIM_LIB
        SGXSDK_URTS_LIB SGXSDK_USVC_LIB
        SGXSDK_INCLUDE_DIRS SGXSDK_LIBRARIES)

MARK_AS_ADVANCED(SGXSDK_LIBRARIES SGXSDK_INCLUDE_DIRS)

set(SGX_ENCLAVE_SIGNER ${SGX_SDK}/bin/${SGX_ARCH}/sgx_sign)
set(SGX_EDGER8R ${SGX_SDK}/bin/${SGX_ARCH}/sgx_edger8r)

set(SGX_URTS_LIB sgx_urts)
set(SGX_USVC_LIB sgx_uae_service)
set(SGX_TRTS_LIB sgx_trts)
set(SGX_TSVC_LIB sgx_tservice)
set(SGX_URTS_SIM_LIB sgx_urts_sim)
set(SGX_USVC_SIM_LIB sgx_uae_service_sim)
set(SGX_TRTS_SIM_LIB sgx_trts_sim)
set(SGX_TSVC_SIM_LIB sgx_tservice_sim)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "PreRelease")
    set(SGX_COMMON_CFLAGS "${SGX_COMMON_CFLAGS} -O0 -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDEBUG -UNDEBUG -DEDEBUG")
else()
    set(SGX_COMMON_CFLAGS "${SGX_COMMON_CFLAGS} -O2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -UDEBUG -DNDEBUG -UEDEBUG")
endif()

