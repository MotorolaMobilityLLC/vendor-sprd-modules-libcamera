ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif


# target source files
mmdvfs-objs :=

# target source files for r1p0
ifeq ($(MODULAR_KERNEL_TAG),r1p0)
SRC := $(shell find $(REL_PATH)/cpp -name "*.c")
SRC += $(shell find $(REL_PATH)/fd -name "*.c")
SRC += $(shell find $(REL_PATH)/isp -name "*.c")
SRC += $(shell find $(REL_PATH)/jpg -name "*.c")
SRC += $(shell find $(REL_PATH)/mtx -name "*.c")
SRC += $(shell find $(REL_PATH)/dcam -name "*.c")
SRC += $(shell find $(REL_PATH)/dcamaxi -name "*.c")
SRC += $(shell find $(REL_PATH)/mmsys -name "*.c")
SRC += $(shell find $(REL_PATH)/mmsys_comm -name "*.c")
SRC += $(shell find $(REL_PATH)/dvfs_driver/sharkl5 -name "*.c")

LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)
mmdvfs-objs += $(LOCAL_SRC:%.c=%.o)
# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/cpp
ccflags-y += -I$(REL_PATH)/fd
ccflags-y += -I$(REL_PATH)/isp
ccflags-y += -I$(REL_PATH)/jpg
ccflags-y += -I$(REL_PATH)/mtx
ccflags-y += -I$(REL_PATH)/dcam
ccflags-y += -I$(REL_PATH)/dcamaxi
ccflags-y += -I$(REL_PATH)/mmsys
ccflags-y += -I$(REL_PATH)/mmsys_comm
ccflags-y += -I$(REL_PATH)/dvfs_driver/sharkl5
endif

# target source files for roc1
ifeq ($(MODULAR_KERNEL_TAG),roc1)
SRC := $(shell find $(REL_PATH)/cpp -name "*.c")
SRC += $(shell find $(REL_PATH)/fd -name "*.c")
SRC += $(shell find $(REL_PATH)/isp -name "*.c")
SRC += $(shell find $(REL_PATH)/jpg -name "*.c")
SRC += $(shell find $(REL_PATH)/mtx -name "*.c")
SRC += $(shell find $(REL_PATH)/dcam -name "*.c")
SRC += $(shell find $(REL_PATH)/dcamaxi -name "*.c")
SRC += $(shell find $(REL_PATH)/mmsys -name "*.c")
SRC += $(shell find $(REL_PATH)/mmsys_comm -name "*.c")
SRC += $(shell find $(REL_PATH)/dvfs_driver/roc1 -name "*.c")

LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)
mmdvfs-objs += $(LOCAL_SRC:%.c=%.o)
# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/cpp
ccflags-y += -I$(REL_PATH)/fd
ccflags-y += -I$(REL_PATH)/isp
ccflags-y += -I$(REL_PATH)/jpg
ccflags-y += -I$(REL_PATH)/mtx
ccflags-y += -I$(REL_PATH)/dcam
ccflags-y += -I$(REL_PATH)/dcamaxi
ccflags-y += -I$(REL_PATH)/mmsys
ccflags-y += -I$(REL_PATH)/mmsys_comm
ccflags-y += -I$(REL_PATH)/dvfs_driver/roc1
endif

ccflags-y += -I$(REL_PATH)/../common/
ccflags-y += -I$(srctree)/drivers/devfreq/
# target
obj-m += mmdvfs.o

# HAPS only
ifeq ($(TEST_ON_HAPS),true)
ccflags-y += -DTEST_ON_HAPS
endif



modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@

