set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SGX_COMMON_CFLAGS} -fPIC -Wno-attributes")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11")

set(hw_target app_hw)
set(sim_target app_sim)

set(enclave_hw ${SGX_ENCLAVE_OUTPUT_DIRECTORY}/${PROJECT_NAME}_t-signed.so)
set(enclave_sim ${SGX_ENCLAVE_OUTPUT_DIRECTORY}/${PROJECT_NAME}_sim_t-signed.so)

set(src_files
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_u.c
        misc/lbr_tools.h
        misc/attributes.h
        main.cpp misc/Logger.cpp
        BranchShadow.cpp BranchShadow.h
        mem/AlignedMem.cpp mem/AlignedMem.h
        misc/Enclave.cpp misc/Enclave.h
        misc/BpuUtils.cpp misc/BpuUtils.h
        misc/LbrReader.cpp misc/LbrReader.h
        misc/Logger.cpp misc/Logger.h
        victim.c victim.h
        BranchShadow.cpp BranchShadow.h
        misc/util.cpp misc/util.h
        shadow/Shadow.cpp shadow/Shadow.h
        shadow/ShadowSegment.cpp shadow/ShadowSegment.h
        shadow/SegRetq.cpp shadow/SegRetq.h
        shadow/SegEaxWrite.cpp shadow/SegEaxWrite.h
        shadow/SegEaxAndOne.cpp shadow/SegEaxAndOne.h
        shadow/SegJneEaxZero.cpp shadow/SegJneEaxZero.h
        shadow/SegNop.cpp shadow/SegNop.h
        shadow/SegJump.cpp shadow/SegJump.h
        mem/Pointers.cpp mem/Pointers.h
        misc/MiniStatistics.cpp misc/MiniStatistics.h
        shadow/SegNop.cpp shadow/SegNop.h
        misc/MiniStatistics.cpp misc/MiniStatistics.h
        shadow/SegJumpR13.cpp shadow/SegJumpR13.h
        shadow/SegSetPosInReg.cpp shadow/SegSetPosInReg.h
        shadow/SegLongJump.cpp shadow/SegLongJump.h
        test/Test.cpp test/Test.h
        shadow/SegEaxAndOneIndir.cpp shadow/SegEaxAndOneIndir.h
        misc/LbrStats.cpp misc/LbrStats.h)

add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_u.c
        COMMAND ${SGX_EDGER8R} --untrusted ${PROJECT_NAME}.edl
        --search-path ${PROJECT_SOURCE_DIR}/trusted
        --search-path ${SGX_SDK}/include
        COMMENT "Generating ${PROJECT_NAME}_u.{c,h}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${hw_target} ${src_files})
add_executable(${sim_target} EXCLUDE_FROM_ALL ${src_files})

target_compile_definitions(${hw_target} PUBLIC SGX_ENCLAVE_NAME="${enclave_hw}")
target_compile_definitions(${sim_target} PUBLIC SGX_ENCLAVE_NAME="${enclave_sim}")

include_directories(
        $(CMAKE_CURRENT_SOURCE_DIR)
        ${PROJECT_SOURCE_DIR}/config
        ${PROJECT_SOURCE_DIR}/lib
        ${PROJECT_SOURCE_DIR}/untrusted
        ${CMAKE_CURRENT_BINARY_DIR}
        ${SGX_SDK}/include
)

target_link_libraries(${hw_target}
        ${SGXSDK_URTS_LIB}
        ${SGXSDK_USVC_LIB}
        pthread)

target_link_libraries(${sim_target}
        ${SGXSDK_URTS_SIM_LIB}
        ${SGXSDK_USVC_SIM_LIB}
        pthread)

add_dependencies(${hw_target} ${PROJECT_NAME}_t)
add_dependencies(${sim_target} ${PROJECT_NAME}_sim_t)
