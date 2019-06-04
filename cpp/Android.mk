LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_BOARD_CAMERA_CPP_MODULAR_KERNEL), lite_r5p0)
include $(LOCAL_PATH)/lite_r5p0/Android.mk
endif
ifeq ($(TARGET_BOARD_CAMERA_CPP_MODULAR_KERNEL), lite_r6p0)
include $(LOCAL_PATH)/lite_r6p0/Android.mk
endif
