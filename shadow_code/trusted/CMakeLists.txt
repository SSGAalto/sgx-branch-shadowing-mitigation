
set(sgx_srcs
        sample_trusted.cpp
        ../shared/common.h
        )

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "PreRelease")
    set(ENCLAVE_CONFIG_DisableDebug "0")
else()
    set(ENCLAVE_CONFIG_DisableDebug "1")
endif()

configure_file(${PROJECT_NAME}.config.xml.in ${PROJECT_NAME}.config.xml @ONLY)

# Setup separate target for simulated and HW backed build
set(hw_target ${PROJECT_NAME}_t)
set(sim_target ${PROJECT_NAME}_sim_t)

# These files are generated
set(config_xml ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.config.xml)
set(edge_file  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_t.c)

# The edl is static, so just use the in-source one
set(edl_file ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.edl)

add_custom_command(OUTPUT ${edge_file}
        COMMAND ${SGX_EDGER8R} --trusted ${edl_file}
        --search-path ${SGXSDK_MAIN_INCLUDE_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_library(${hw_target}  SHARED ${sgx_srcs} ${edge_file})
add_library(${sim_target} EXCLUDE_FROM_ALL SHARED ${sgx_srcs} ${edge_file})

add_dependencies(${hw_target} privatekey)
add_dependencies(${sim_target} privatekey)

include_directories(
        $(CMAKE_CURRENT_SOURCE_DIR)
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
        ${SGXSDK_INCLUDE_DIRS}
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SGX_COMMON_CFLAGS} -nostdinc -fvisibility=hidden -fpie -fstack-protector")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++03 -nostdinc++")

target_link_libraries(${hw_target} "${SGX_COMMON_CFLAGS} \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L${SGX_LIBRARY_PATH} \
    -Wl,--whole-archive -l${SGX_TRTS_LIB} -Wl,--no-whole-archive \
    -Wl,--start-group ${LIB_MBEDTLS_PATH} -lsgx_tstdc -lsgx_tstdcxx -lsgx_tcrypto -l${SGX_TSVC_LIB} -Wl,--end-group \
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
    -Wl,--defsym,__ImageBase=0")

target_link_libraries(${sim_target} "${SGX_COMMON_CFLAGS} \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L${SGX_LIBRARY_PATH} \
    -Wl,--whole-archive -l${SGX_TRTS_SIM_LIB} -Wl,--no-whole-archive \
    -Wl,--start-group ${LIB_MBEDTLS_PATH} -lsgx_tstdc -lsgx_tstdcxx -lsgx_tcrypto -l${SGX_TSVC_SIM_LIB} -Wl,--end-group \
    -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
    -Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
    -Wl,--defsym,__ImageBase=0")

#install(TARGETS ${hw_target} ${sim_target}
        #DESTINATION libs PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
#install(FILES ${edl_file} DESTINATION libs PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)

add_custom_command(TARGET ${hw_target} POST_BUILD
        COMMAND ${SGX_ENCLAVE_SIGNER} sign
        -key ${private_key} -config ${config_xml}
        -enclave lib${hw_target}.so
        -out ${hw_target}-signed.so
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Signing the enclave => ${hw_target}-signed.so")

add_custom_command(TARGET ${sim_target} POST_BUILD
        COMMAND ${SGX_ENCLAVE_SIGNER} sign
        -key ${private_key} -config ${config_xml}
        -enclave lib${sim_target}.so
        -out ${sim_target}-signed.so
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Signing the enclave => ${sim_target}-signed.so")
