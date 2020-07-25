#!/bin/bash
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
                      -j8 \
                      modules

[ -d "modules" ] && rm -rf modules || mkdir -p modules

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
                      -j8 \
                      INSTALL_MOD_PATH=../modules \
                      INSTALL_MOD_STRIP=1 \
                      modules_install
