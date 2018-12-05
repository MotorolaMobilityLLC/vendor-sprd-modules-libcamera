ifeq ($(strip $(TARGET_BOARD_CAMERA_SENSOR_MODULAR_KERNEL)),yes)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sprd_flash_drv.ko
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/modules
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_STRIP_MODULE := keep_symbols

#$(warning $(LOCAL_PATH))

# delete .ko before making anything
LOCAL_PATH_FLASH := $(shell pwd)/$(LOCAL_PATH)
$(shell rm $(LOCAL_PATH_FLASH)/$(LOCAL_MODULE) -f)
$(shell rm $(shell find $(LOCAL_PATH_FLASH) -name "*.o") -f)

#$(warning $(LOCAL_PATH_FLASH))

include $(BUILD_PREBUILT)

#convert to absolute directory
PRODUCT_OUT_ABSOLUTE:=$(shell cd $(PRODUCT_OUT); pwd)
#$(warning $(PRODUCT_OUT_ABSOLUTE))

$(LOCAL_PATH)/sprd_flash_drv.ko: $(TARGET_PREBUILT_KERNEL)
	$(MAKE) -C $(shell dirname $@) MODULAR_KERNEL_TAG=$(TARGET_BOARD_CAMERA_SENSOR_MODULAR_KERNEL) REL_PATH=$(LOCAL_PATH_FLASH) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) KDIR=$(PRODUCT_OUT_ABSOLUTE)/obj/KERNEL clean
	$(MAKE) -C $(shell dirname $@) MODULAR_KERNEL_TAG=$(TARGET_BOARD_CAMERA_SENSOR_MODULAR_KERNEL) REL_PATH=$(LOCAL_PATH_FLASH) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) KDIR=$(PRODUCT_OUT_ABSOLUTE)/obj/KERNEL
	$(TARGET_STRIP) -d --strip-unneeded $@

endif
