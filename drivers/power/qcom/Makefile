obj-$(CONFIG_MSM_PM)		+= msm-pm.o pm-data.o
obj-$(CONFIG_MSM_IDLE_STATS)	+= lpm-stats.o
obj-$(CONFIG_MSM_NOPM)		+= no-pm.o
obj-$(CONFIG_PM)                += pm-boot.o
obj-$(CONFIG_APSS_CORE_EA)	+= msm-core.o

ifeq ($(CONFIG_DEBUG_FS), y)
obj-$(CONFIG_APPS_CORE_EA)	+= debug_core.o
endif

obj-$(CONFIG_MSM_APM)		+= apm.o
