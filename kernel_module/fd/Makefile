ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

# target source files
sprd_camera-objs :=

# target source files for fd1.0
ifeq ($(FD_VER),fd1.0)
SRC := $(shell find $(REL_PATH)/fd1.0 -name "*.c")
LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)

sprd_fd-objs += $(LOCAL_SRC:%.c=%.o)

# target includes
ccflags-y += -I$(REL_PATH)/../core/isp2.6/inc/

else ifeq ($(FD_VER),fd2.0)
SRC := $(shell find $(REL_PATH)/fd2.0 -name "*.c")
LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)

sprd_fd-objs += $(LOCAL_SRC:%.c=%.o)

# target includes
#need to check when SharkL5Pro source code porting to vendor
ccflags-y += -I$(REL_PATH)/../core/isp2.6/inc/
endif

# target
obj-m += sprd_fd.o

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

