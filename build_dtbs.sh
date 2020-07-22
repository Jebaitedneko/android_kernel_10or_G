#!/bin/bash

# Do not touch this.

source build_helper.sh

# Uncomment either of the two to switch.

nontreble
#treble

# Uncomment either of the two to switch.

make O=out ARCH=arm64 g_nontreble_defconfig
#make O=out ARCH=arm64 g_treble_defconfig

# Function call to build DTB(s).

pcmake dtbs
                 
# Function call to remove generated config.

rmconf
