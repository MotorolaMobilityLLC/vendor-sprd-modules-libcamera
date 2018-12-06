ifneq ($(strip $(TARGET_BOARD_CPP_UTEST)),)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

$(shell cp -rf $(LOCAL_PATH)/data/like $(TARGET_OUT_DATA))

include $(call all-makefiles-under, $(LOCAL_PATH))
endif
