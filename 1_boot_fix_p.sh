#!/bin/bash

TARGET=p

TOOLS_DIR=~/android/TOOLS

CFG_FILE=$TOOLS_DIR/BOOT_FIX/$TARGET/10or_G_defconfig

KERNEL_DTS_DIR=arch/arm64/boot/dts/qcom

PROBLEM_DTS=$KERNEL_DTS_DIR/msm8953-qrd-sku3.dts
PATCH_DTS=$TOOLS_DIR/BOOT_FIX/$TARGET/msm8953-qrd-sku3.dts

echo -e "\nWelcome.\n"
if [[ -f "$PROBLEM_DTS" ]]
then
    echo -e "\nPROBLEM DTS FOUND.\n"
    echo -e "\nPROBLEM DTS BACKED UP.\n"
    mv $PROBLEM_DTS $PROBLEM_DTS.bak
    echo -e "\nPROBLEM DTS PATCHED...\n"
    cp $PATCH_DTS $KERNEL_DTS_DIR
fi
echo -e "\nCOPYING DEFCONFIG...\n"
cp $CFG_FILE arch/arm64/configs/
echo -e "\nCompleted.\n"
