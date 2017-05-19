LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), sp9853i)

include $(LOCAL_PATH)/hal3dummy/Android.mk

else

include $(LOCAL_PATH)/Camera.mk
#include $(LOCAL_PATH)/Camera_Utest.mk
#include $(LOCAL_PATH)/Utest_jpeg.mk

# include $(call all-subdir-makefiles)
include $(call first-makefiles-under,$(LOCAL_PATH))

endif
