ifeq ($(KDIR),)
$(error No KDIR found for platform $(TARGET_PLATFORM))
endif

# target source files
sprd_sensor-objs := sprd_sensor_core.o sprd_sensor_drv.o

# target source files for isp2.6
ifeq ($(CSI_VER),r2p1)
SRC := compat_sensor_drv.c
SRC += csi2/csi_sprd/receiver_r2p1/csi_api.c
SRC += csi2/csi_sprd/receiver_r2p1/csi_driver.c
SRC += csi2/csi_sprd/receiver_r2p1/dphy_drv.c

sprd_sensor-objs += $(SRC:%.c=%.o)

# TODO: refine these includes
ccflags-y += -I$(REL_PATH)/csi2/csi_sprd/receiver_r2p1/
ccflags-y += -I$(REL_PATH)/../core/isp2.6/inc/

# HAPS only
ifeq ($(TEST_ON_HAPS),true)
ccflags-y += -DTEST_ON_HAPS
endif

endif

# target
obj-m += sprd_sensor.o

# target includes
ccflags-y += -I$(REL_PATH)/../common
ccflags-y += -I$(REL_PATH)/../interface

modules:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

clean:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) M=$(CURDIR) $@

kernelrelease:
	$(MAKE) ARCH=$(ARCH) -C $(KDIR) $@

