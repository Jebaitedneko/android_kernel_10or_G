#!/bin/bash

# Do not touch this.

source build_helper.sh

# Function call to generate treble defconfig.

treble

# Set treble defconfig.

make O=out ARCH=arm64 g_treble_defconfig

# Function call to build kernel with Proton-Clang + Optimized flags

pcmake -j8

# Function call to remove generated config.

rmconf

./anykernel/build_treble_zip.sh
