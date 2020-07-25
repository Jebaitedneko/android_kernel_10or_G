#!/bin/bash

source ~/.bashrc && source ~/.profile
export LC_ALL=C && export USE_CCACHE=1
ccache -M 100G

[ -d "out" ] && rm -rf out || mkdir -p out

make O=out ARCH=arm64 msm8937-perf_defconfig

#make O=out ARCH=arm64 g_non-treble_defconfig

PATH="${HOME}/android/TOOLS/proton-clang/bin:${PATH}" \
make                  O=out \
                      ARCH=arm64 \
                      CC="ccache clang" \
                      AR=llvm-ar \
                      NM=llvm-nm \
                      LD=ld.lld \
                      OBJCOPY=llvm-objcopy \
                      OBJDUMP=llvm-objdump \
                      STRIP=llvm-strip \
                      CROSS_COMPILE=aarch64-linux-gnu- \
                      CROSS_COMPILE_ARM32=arm-linux-gnueabi- \
                      CONFIG_NO_ERROR_ON_MISMATCH=y \
                      -j8

