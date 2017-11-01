LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifneq ($(strip $(TARGET_PRODUCT)), sp7731e_1h10_native)
include $(LOCAL_PATH)/Camera.mk
endif
#include $(LOCAL_PATH)/Camera_Utest.mk
#include $(LOCAL_PATH)/Utest_jpeg.mk

# include $(call all-subdir-makefiles)
ifneq ($(strip $(TARGET_PRODUCT)), sp7731e_1h10_native)
include $(call first-makefiles-under,$(LOCAL_PATH))
endif
