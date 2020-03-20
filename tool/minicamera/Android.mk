LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg
ISPDRV_DIR := camdrv/isp2.3
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.4)
HAL_DIR := hal3_2v4
OEM_DIR := oem2v4
ISPALG_DIR := ispalg
ISPDRV_DIR := camdrv/isp2.4
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg
ISPDRV_DIR := camdrv/isp2.6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg
ISPDRV_DIR := camdrv/isp2.6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
HAL_DIR := hal3_2v6
OEM_DIR := oem2v6
ISPALG_DIR := ispalg
ISPDRV_DIR := camdrv/isp2.6
endif

LOCAL_C_INCLUDES := \
       $(LOCAL_PATH)/../../$(ISPDRV_DIR)/isp_tune \
       $(LOCAL_PATH)/../../$(ISPALG_DIR)/common/inc \
       $(LOCAL_PATH)/../../$(ISPDRV_DIR)/middleware/inc \
       $(LOCAL_PATH)/../../$(ISPDRV_DIR)/driver/inc \
       $(LOCAL_PATH)/../../sensor/inc


LOCAL_C_INCLUDES += \
       $(LOCAL_PATH)/../.. \
       $(LOCAL_PATH)/../../common/inc \
       $(LOCAL_PATH)/../../$(OEM_DIR)/inc \
       $(LOCAL_PATH)/../../hal1.0/inc \
       $(LOCAL_PATH)/../../$(HAL_DIR) \
       $(LOCAL_PATH)/../../arithmetic/facebeauty/inc \
       $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
       $(TOP)/vendor/sprd/external/kernel-headers \
       $(TOP)/vendor/sprd/modules/libmemion \
       $(TOP)/kernel/include/video \
       $(TOP)/kernel/include/uapi/video \
       $(TOP)/system/media/camera/include \
       $(TOP)/system/core/include \
       $(LOCAL_PATH)/../../kernel_module/interface

LOCAL_SHARED_LIBRARIES := libcutils liblog libcamoem libcamcommon libmemion libcamsensor libxml2

include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_32_BIT_ONLY := true
LOCAL_SRC_FILES := minicamera.cpp
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := minicamera

include $(BUILD_EXECUTABLE)
