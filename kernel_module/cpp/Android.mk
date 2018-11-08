ifneq ($(strip $(TARGET_BOARD_CAMERA_MODULAR_KERNEL)),)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := sprd_cpp.ko
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/modules
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_STRIP_MODULE := keep_symbols

# delete .ko before making anything
LOCAL_PATH_CPP := $(shell pwd)/$(LOCAL_PATH)/$(TARGET_BOARD_CPP_VER)
$(shell rm $(LOCAL_PATH_CPP)/$(LOCAL_MODULE) -f)
$(shell rm $(shell find $(LOCAL_PATH_CPP) -name "*.o") -f)

include $(BUILD_PREBUILT)

#convert to absolute directory
PRODUCT_OUT_ABSOLUTE:=$(shell cd $(PRODUCT_OUT); pwd)

$(LOCAL_PATH)/sprd_cpp.ko: $(TARGET_PREBUILT_KERNEL)
	$(MAKE) -C $(shell dirname $@) MODULAR_KERNEL_TAG=$(TARGET_BOARD_CAMERA_MODULAR_KERNEL) CPP_VER=$(TARGET_BOARD_CPP_VER) REL_PATH=$(LOCAL_PATH_CPP) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) KDIR=$(PRODUCT_OUT_ABSOLUTE)/obj/KERNEL clean
	$(MAKE) -C $(shell dirname $@) MODULAR_KERNEL_TAG=$(TARGET_BOARD_CAMERA_MODULAR_KERNEL) CPP_VER=$(TARGET_BOARD_CPP_VER) REL_PATH=$(LOCAL_PATH_CPP) ARCH=$(TARGET_KERNEL_ARCH) CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) KDIR=$(PRODUCT_OUT_ABSOLUTE)/obj/KERNEL
	$(TARGET_STRIP) -d --strip-unneeded $@

endif
