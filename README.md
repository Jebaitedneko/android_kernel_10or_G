# android_kernel_10or_G
Kernel sources for 10or G

generated defconfig = 10or_G_defconfig

#nougat
>source: https://android-library.s3.ap-south-1.amazonaws.com/10or/10orG_Nougat_Opensource_Release_20171120.zip
>fixed kconfig warn
>switched to generated defconfig
>dtb edit
    
#oreo
>source: https://source.codeaurora.org/quic/la/kernel/msm-3.18
>branch: LA.UM.6.6.c25-01000-89xx.0
>switched to generated defconfig
>dtb edit
    
#pie
>source: https://github.com/android-linux-stable/msm-3.18
>branch: kernel.lnx.3.18.r34-rel
>misc: LA.UM.7.6.r1-05500-89xx.0
>switched to generated defconfig (from oreo)
>dtb edit (from oreo)
