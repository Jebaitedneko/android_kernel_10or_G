#!/bin/bash

configdir=$(pwd)/arch/arm64/configs

tcdir=${HOME}/android/TOOLS/proton-clang

[ -d $tcdir ] \
&& echo -e "\nProton-Clang Present.\n" \
|| echo -e "\nProton-Clang Not Present. Downloading Around 500MB...\n" \
| mkdir -p $tcdir \
| git clone --depth=1 https://github.com/kdrag0n/proton-clang $tcdir \
| echo "Done."

echo -e "\nChecking Clang Version...\n"
PATH="$tcdir/bin:${PATH}" \
clang --version
echo -e "\n"

source ~/.bashrc && source ~/.profile
export LC_ALL=C && export USE_CCACHE=1
ccache -M 100G
echo -e "\nStarting Build...\n"

[ -d out ] && rm -rf out || mkdir -p out

treble() {
cp $configdir/msm8937-perf_defconfig $configdir/g_treble_defconfig
}

nontreble() {
cp $configdir/msm8937-perf_defconfig $configdir/g_nontreble_defconfig
echo "CONFIG_MACH_NONTREBLE_DTS=y" >> $configdir/g_nontreble_defconfig
echo "CONFIG_PRONTO_WLAN=m" >> $configdir/g_nontreble_defconfig
}

rmconf() {
[ -f $configdir/g_treble_defconfig ] && rm -rf $configdir/g_treble_defconfig
[ -f $configdir/g_nontreble_defconfig ] && rm -rf $configdir/g_nontreble_defconfig
}

pcmake() {
PATH="$tcdir/bin:${PATH}" \
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
                      $1 $2 $3
}

pcmod() {
[ -d "modules" ] && rm -rf modules || mkdir -p modules

pcmake INSTALL_MOD_PATH=../modules \
       INSTALL_MOD_STRIP=1 \
       modules_install
}
