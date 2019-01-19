ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

# target source files
vdsp-objs :=

# target source files for ceva1.0
ifeq ($(VDSP_VER),ceva1.0)
SRC := $(shell find $(REL_PATH)/ceva1.0 -name "*.c")
LOCAL_SRC := $(SRC:$(REL_PATH)/%=%)

vdsp-objs += $(LOCAL_SRC:%.c=%.o)

# target includes
ccflags-y += -I$(REL_PATH)/ceva1.0/inc/

endif

# target
obj-m += vdsp.o

# target includes
ccflags-y += -I$(REL_PATH)/../sensor/
ccflags-y += -I$(REL_PATH)/../common
ccflags-y += -I$(REL_PATH)/../interface
ccflags-y += -I$(srctree)/drivers/staging/android/ion

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@

