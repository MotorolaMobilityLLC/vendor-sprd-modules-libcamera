ifeq ($(strip $(TARGET_BOARD_CPP_UTEST)),lite_r6p0)
LOCAL_PATH := $(call my-dir)
$(warning $(LOCAL_PATH))

include $(CLEAR_VARS)

$(shell cp -rf $(LOCAL_PATH)/data/like $(TARGET_OUT_DATA))

include $(call all-makefiles-under, $(LOCAL_PATH))
endif
