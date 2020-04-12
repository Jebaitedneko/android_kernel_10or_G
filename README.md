# android_kernel_10or_G
Kernel sources for 10or G

nougat
    -source from tenor website
    -fixed kconfig warn
    -switched to generated defconfig
    -dtb edit
    
oreo
    -source from https://source.codeaurora.org/quic/la/kernel/msm-3.18 branch LA.UM.6.6.c25-01000-89xx.0
    -switched to generated defconfig
    -dtb edit
    
pie
    -source from https://github.com/android-linux-stable/msm-3.18 branch kernel.lnx.3.18.r34-rel
    -caf merge LA.UM.7.6.r1-05500-89xx.0 was already incorporated
    -switched to generated defconfig (from oreo)
    -dtb edit (from oreo)
