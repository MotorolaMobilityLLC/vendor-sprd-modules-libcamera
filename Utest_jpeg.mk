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


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
endif

ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SRC_FILES+= \
	oem$(ISP_HW_VER)/isp_calibration/src/utest_jpeg.cpp
endif


LOCAL_MODULE := utest_jpeg_$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SprdLib.mk

include $(BUILD_EXECUTABLE)

