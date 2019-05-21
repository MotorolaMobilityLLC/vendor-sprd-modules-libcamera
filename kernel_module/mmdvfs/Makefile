ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif


# target source files
mmdvfs-objs :=

# target source files for r1p0
ifeq ($(MODULAR_KERNEL_TAG),r1p0)
SRC := $(shell find $(REL_PATH)/r1p0/cpp -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/fd -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/isp -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/jpg -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/mtx -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/dcam -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/dcamaxi -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/mmsys -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/mmsys_comm -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/dvfs_driver/sharkl5 -name "*.c")

LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)

mmdvfs-objs += $(LOCAL_SRC:%.c=%.o)
# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/r1p0/cpp
ccflags-y += -I$(REL_PATH)/r1p0/fd
ccflags-y += -I$(REL_PATH)/r1p0/isp
ccflags-y += -I$(REL_PATH)/r1p0/jpg
ccflags-y += -I$(REL_PATH)/r1p0/mtx
ccflags-y += -I$(REL_PATH)/r1p0/dcam
ccflags-y += -I$(REL_PATH)/r1p0/dcamaxi
ccflags-y += -I$(REL_PATH)/r1p0/mmsys
ccflags-y += -I$(REL_PATH)/r1p0/mmsys_comm
ccflags-y += -I$(REL_PATH)/r1p0/dvfs_driver/sharkl5
endif

# target source files for roc1
ifeq ($(MODULAR_KERNEL_TAG),roc1)
SRC := $(shell find $(REL_PATH)/r1p0/cpp -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/fd -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/isp -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/jpg -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/mtx -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/dcam -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/dcamaxi -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/mmsys -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/mmsys_comm -name "*.c")
SRC += $(shell find $(REL_PATH)/r1p0/dvfs_driver/roc1 -name "*.c")

LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)
mmdvfs-objs += $(LOCAL_SRC:%.c=%.o)
# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/r1p0/cpp
ccflags-y += -I$(REL_PATH)/r1p0/fd
ccflags-y += -I$(REL_PATH)/r1p0/isp
ccflags-y += -I$(REL_PATH)/r1p0/jpg
ccflags-y += -I$(REL_PATH)/r1p0/mtx
ccflags-y += -I$(REL_PATH)/r1p0/dcam
ccflags-y += -I$(REL_PATH)/r1p0/dcamaxi
ccflags-y += -I$(REL_PATH)/r1p0/mmsys
ccflags-y += -I$(REL_PATH)/r1p0/mmsys_comm
ccflags-y += -I$(REL_PATH)/r1p0/dvfs_driver/roc1
endif

# target source files for sharkl5pro
ifeq ($(MODULAR_KERNEL_TAG),sharkl5pro)
SRC := $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/cpp -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/fd -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/isp -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/jpg -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/mtx -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/dcam -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/dcamaxi -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/mmsys -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/mmsys_comm -name "*.c")
SRC += $(shell find $(REL_PATH)/$(MODULAR_KERNEL_TAG)/driver -name "*.c")

LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)
mmdvfs-objs += $(LOCAL_SRC:%.c=%.o)
# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/cpp
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/fd
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/isp
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/jpg
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/mtx
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/dcam
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/dcamaxi
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/mmsys
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/mmsys_comm
ccflags-y += -I$(REL_PATH)/$(MODULAR_KERNEL_TAG)/driver
endif


ccflags-y += -I$(REL_PATH)/../common/
ccflags-y += -I$(srctree)/drivers/devfreq/
# target
obj-m += mmdvfs.o

# HAPS only
ifeq ($(TEST_ON_HAPS),true)
ccflags-y += -DTEST_ON_HAPS
endif

$(warning $(CURDIR))

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@

