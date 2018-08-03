LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sp9863a)
# sharkl3 bringup by pass camera
else

ifeq ($(strip $(TARGET_BOARD_CAMERA_FUNCTION_DUMMY)), true)
include $(LOCAL_PATH)/hal3dummy/Android.mk
else
include $(LOCAL_PATH)/Camera.mk
# include $(call all-subdir-makefiles)
include $(call first-makefiles-under,$(LOCAL_PATH))
endif

endif
