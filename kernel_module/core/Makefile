ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

# target source files
sprd_camera-objs :=

# target source files for isp2.6
ifeq ($(ISP_VER),isp2.6)

ifeq ($(CONFIG_COMPAT),true)
# compat ioctl for 32-bit user
SRC := $(shell find $(REL_PATH)/isp2.6 -name "*.c")
ccflags-y += -DCONFIG_COMPAT
else
SRC := $(shell find $(REL_PATH)/isp2.6 -name "*.c"  ! -name "compat_cam_drv.c")
endif

LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)

sprd_camera-objs += $(LOCAL_SRC:%.c=%.o)

# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/../sensor/
ccflags-y += -I$(REL_PATH)/isp2.6/inc/
ccflags-y += -I$(REL_PATH)/../flash/flash_drv/

# HAPS only
ifeq ($(TEST_ON_HAPS),true)
ccflags-y += -DTEST_ON_HAPS
endif

endif

# target
obj-m += sprd_camera.o

# target includes
ccflags-y += -I$(REL_PATH)/../common
ccflags-y += -I$(REL_PATH)/../interface
ccflags-y += -I$(srctree)/drivers/staging/android/ion

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@

