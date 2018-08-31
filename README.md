# SGX Branch Shadowing Mitigation


# Introduction 
This work presents a new defense against branch-shadowing to protect the control flow of the program running in an enclave. More precisely, we use compile-time modifications to convert all branch instructions into unconditional branches targeting our in-enclave trampoline code. Detailed description is available in our technical report, available at https://arxiv.org/abs/1808.06478.

# Prerequisites
-Intel SGX SDK for Linux

-LLVM compiler

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

## 3. Install and run the obfuscating compiler

```
cd llvm
git clone https://github.com/llvm-mirror/clang.git tools/clang
mkdir build
cd build
cmake ..
make
```

# Licence information
This code is released under Apache 2.0 and GPL 2.0 licenses. We are further using the following third-party code for which we claim no copyright:

[spdlog](https://github.com/gabime/spdlog) and [args.hxx](https://github.com/Taywee/args) licensed through MIT license.

[LLVM](https://llvm.org) and [Clang](https://clang.llvm.org) are under the University of 
Illinois/NCSA Open Source License.
