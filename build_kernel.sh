#!/bin/bash

# Do not touch this.

source build_helper.sh

[ $1 == 'n' ] && nontreble
[[ $1 == 't' || $1 == 'd' ]] && treble

[ $1 == 'n' ] && make O=out ARCH=arm64 g_nontreble_defconfig
[ $1 == 't' ] && make O=out ARCH=arm64 g_treble_defconfig

[[ $1 == 'n' || $1 == 't' ]] && pcmake -j$(nproc --all)
[ $1 == 'd' ] && pcmake dtbs

rmconf

[ $1 == 'n' ] && ./anykernel/build_nontreble_zip.sh
[ $1 == 't' ] && ./anykernel/build_treble_zip.sh
