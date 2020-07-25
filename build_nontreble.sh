#!/bin/bash

# Do not touch this.

source build_helper.sh

# Function call to generate nontreble defconfig.

nontreble

# Set nontreble defconfig.

make O=out ARCH=arm64 g_nontreble_defconfig

# Function call to build kernel with Proton-Clang + Optimized flags

pcmake -j8

# Function call to build kernel modules with Proton-Clang + Optimized flags

pcmod

# Function call to remove generated config.

rmconf

./anykernel/build_nontreble_zip.sh 
