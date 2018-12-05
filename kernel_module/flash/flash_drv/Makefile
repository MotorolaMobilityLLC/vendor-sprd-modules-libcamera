
ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

# target source files
sprd_flash_drv-objs := flash_drv.o

# target source files for isp2.6
ifeq ($(MODULAR_KERNEL_TAG),yes)

# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/../../interface

# TODO: temp for HAPS
ccflags-y += -DTEST_ON_HAPS
endif


# target
obj-m += sprd_flash_drv.o

# target includes

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@
