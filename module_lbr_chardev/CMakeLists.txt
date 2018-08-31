#Authors: Hans Liljestrand and Shohreh Hosseinzadeh  
#Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
#This code is released under Apache 2.0 and GPL 2.0 licenses.


cmake_minimum_required(VERSION 3.0)
project(lbr_chardev)


set(module_build_dir ${CMAKE_CURRENT_BINARY_DIR}/module_build)
set(module_name lbr_dumper.ko)

set(module_src_files
        module/attributes.h
        module/lbr_dumper.h
        module/lbr_dumper.c
        module/lbr_tools.h
        module/lbr_tools.c
        module/perf_attr.h
        )

add_custom_target(install_module
        COMMAND sudo insmod ${module_name} && sudo ${CMAKE_CURRENT_SOURCE_DIR}/scripts/setup_mknod.pl
        DEPENDS module
        WORKING_DIRECTORY ${module_build_dir}
)

add_custom_target(uninstall_module
        COMMAND sudo rmmod lbr_dumper && sudo rm /dev/lbr_dumper)

add_custom_target(module ALL
        COMMAND make
        DEPENDS module_build_dir
        WORKING_DIRECTORY ${module_build_dir}
        SOURCES ${module_src_files})

add_custom_target(module_build_dir
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/module ${module_build_dir})

add_custom_command(TARGET module POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${module_build_dir}/${module_name} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(trigger_dump tester/trigger_dump.c)
