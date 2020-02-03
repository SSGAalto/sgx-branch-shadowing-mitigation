# SGX Branch Shadowing Mitigation

# Introduction 
This work presents a new defense against branch-shadowing to protect the control flow of the program running in an enclave. More precisely, we use compile-time modifications to convert all branch instructions into unconditional branches targeting our in-enclave trampoline code. 

Paper available at [*Mitigating Branch-Shadowing Attacks on Intel SGX using Control Flow Randomization*, 2018, SysTEX '18](https://doi.org/10.1145/3268935.3268940).

# Prerequisites

- Intel SGX SDK for Linux
- LLVM compiler

# Build and run the obfuscating compiler

## 1. Build modified LLVM

**Note:** LLVM modifications can be studied in the commit history. The [shadow-llvm](https://github.com/SSGAalto/sgx-branch-shadowing-mitigation/tree/shadow-llvm) branch contains the unmodified LLVM `release_60` source code on which the modifications are based.

```bash
cd llvm
git clone https://github.com/llvm-mirror/clang.git tools/clang
cd tools/clang
git checkout release_60
cd ../..
mkdir build
cd build
cmake ..
make
```

## 2. Build hardened example code

```bash
export CLANG=../llvm/build/bin/clang
cd victim_example
$CLANG -c victim.c -o victim-plain.o
objdump -D victim-plain.o -j .text
$CLANG -mllvm -x86-branch-conversion -mllvm -debug-only=x86-branch-conversion -c victim.c -o victim-mitigated.o
objdump -D victim-mitigated.o -j .text
$CLANG -mllvm -x86-branch-conversion -mllvm -x86-bc-dummy-instr -mllvm -debug-only=x86-branch-conversion -c victim.c -o victim-mitigated-dummy.o
objdump -D victim-mitigated-dummy.o -j .text
```

# Build and run branch shadowing attack test application

## 1. Install chardev device for reading LBR

```
cd module_lbr_chardev/build
cmake ..
make install_module
```

## 2. Install and run shadow test app

```
cd shadow_code/build
cmake ..
make
./app_hw -h             # for options
./run_enclave_jne.pl    # to run everything
```

# Licence information
This code is released under Apache 2.0 and GPL 2.0 licenses. We are further using the following third-party code for which we claim no copyright:

[spdlog](https://github.com/gabime/spdlog) and [args.hxx](https://github.com/Taywee/args) licensed through MIT license.

[LLVM](https://llvm.org) and [Clang](https://clang.llvm.org) are under the University of 
Illinois/NCSA Open Source License.
