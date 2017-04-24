LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

isp_use2.0:=0
include $(LOCAL_PATH)/SprdCtrl.mk

ISP_HW_VER = 3v0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
ISP_HW_VER = 2v1

LOCAL_32_BIT_ONLY := true

isp_use2.0:=1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
endif

ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SRC_FILES+= \
	oem$(ISP_HW_VER)/isp_calibration/src/utest_camera.cpp

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/isp$(TARGET_BOARD_CAMERA_ISP_DIR)/common/inc \
	$(LOCAL_PATH)/isp$(TARGET_BOARD_CAMERA_ISP_DIR)/middleware/inc \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TOP)/vendor/sprd/external/kernel-headers \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/oem$(ISP_HW_VER)/inc \
	$(LOCAL_PATH)/oem$(ISP_HW_VER)/isp_calibration/inc \
	$(LOCAL_PATH)/tool/mtrace
endif


LOCAL_MODULE := utest_camera_$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SprdLib.mk

include $(BUILD_EXECUTABLE)

