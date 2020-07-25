ifneq ($(CONFIG_MACH_NONTREBLE_DTS),y)
dtb-$(CONFIG_ARCH_MSM8953) += msm8953-qrd-sku3-holland2.dtb
else
dtb-$(CONFIG_ARCH_MSM8953) += msm8953-qrd-sku3-holland2-nontreble.dtb
endif

ifeq ($(CONFIG_ARM64),y)
always          := $(dtb-y)
subdir-y        := $(dts-dirs)
else
targets += dtbs
targets += $(addprefix ../, $(dtb-y))

$(obj)/../%.dtb: $(src)/%.dts FORCE
        $(call if_changed_dep,dtc)

dtbs: $(addprefix $(obj)/,$(dtb-y))
endif
clean-files := *.dtb
