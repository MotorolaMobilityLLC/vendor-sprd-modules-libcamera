LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

isp_use2.0:=0
include $(LOCAL_PATH)/SprdCtrl.mk

ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SRC_FILES+= \
	oem2v0/isp_calibration/src/utest_camera.cpp
endif


LOCAL_MODULE := utest_camera_$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS := optional

include $(LOCAL_PATH)/SprdLib.mk

include $(BUILD_EXECUTABLE)

