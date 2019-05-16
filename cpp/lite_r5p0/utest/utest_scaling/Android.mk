LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    utest_scal.cpp \

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/modules/libmemion/ \
    $(TOP)/vendor/sprd/external/kernel-headers \
	$(LOCAL_PATH)/../../algo/inc \
	$(LOCAL_PATH)/../../driver/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(OEM_DIR)/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(ISPALG_DIR)/common/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(ISPDRV_DIR)/middleware/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(ISPDRV_DIR)/driver/inc \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(TOP)/vendor/sprd/modules/libcamera/common/inc \

ifeq ($(strip $(TARGET_BOARD_CAMERA_MODULAR)),true)
LOCAL_C_INCLUDES += \
		$(LOCAL_PATH)/../../../kernel_module/interface
endif

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr
LOCAL_SHARED_LIBRARIES := libcppdrv libmemion liblog libEGL libbinder libutils

#LOCAL_CFLAGS += -DTEST_ON_HAPS

LOCAL_MODULE:= utest_scal_$(TARGET_BOARD_PLATFORM)
LOCAL_32_BIT_ONLY := true
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin/hw

#ifeq ($(PLATFORM_VERSION),4.4.4)
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
#endif

include $(BUILD_EXECUTABLE)


