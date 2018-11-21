LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
       $(LOCAL_PATH)/../../$(ISPDRV_DIR)/isp_tune \
       $(LOCAL_PATH)/../../$(ISPALG_DIR)/common/inc \
       $(LOCAL_PATH)/../../$(ISPDRV_DIR)/middleware/inc \
       $(LOCAL_PATH)/../../$(ISPDRV_DIR)/driver/inc

LOCAL_C_INCLUDES += \
       $(LOCAL_PATH)/../.. \
       $(LOCAL_PATH)/../../common/inc \
       $(LOCAL_PATH)/../../$(OEM_DIR)/inc \
       $(LOCAL_PATH)/../../hal1.0/inc \
       $(LOCAL_PATH)/../../$(HAL_DIR) \
       $(LOCAL_PATH)/../../arithmetic/facebeauty/inc \
       $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
       $(TOP)/vendor/sprd/external/kernel-headers \
       $(TOP)/vendor/sprd/modules/libmemion \
       $(TOP)/kernel/include/video \
       $(TOP)/kernel/include/uapi/video \
       $(TOP)/system/media/camera/include \
       $(TOP)/system/core/include

ifeq ($(strip $(TARGET_BOARD_CAMERA_MODULAR)),true)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../kernel_module/interface
endif

LOCAL_SHARED_LIBRARIES := libcutils liblog libcamoem libcamcommon libmemion

include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_32_BIT_ONLY := true
LOCAL_SRC_FILES := minicamera.cpp
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := minicamera
LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
