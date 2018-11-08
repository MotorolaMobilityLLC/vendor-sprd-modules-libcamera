ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

#DLIBS = -lm

# target source files
sprd_cpp-objs :=

# target source files for cpp
SRC := $(shell find $(REL_PATH) -name "*.c")
LOCAL_SRC := $(SRC:$(REL_PATH)/%=$(CPP_VER)/%)

sprd_cpp-objs += $(LOCAL_SRC:%.c=%.o)

# TODO: refine these includes
#ccflags-y += -I$(srctree)/drivers/misc/sprd_camera/common
#ccflags-y += -I$(srctree)/drivers/staging/android/ion

# TODO: temp for HAPS
ccflags-y += -DTEST_ON_HAPS


# target
obj-m += sprd_cpp.o

# target includes
ccflags-y += -I$(REL_PATH)/../../common
ccflags-y += -I$(REL_PATH)/../../interface
ccflags-y += -I$(srctree)/drivers/staging/android/ion

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@

