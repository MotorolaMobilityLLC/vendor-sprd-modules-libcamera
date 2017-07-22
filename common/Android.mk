LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror -Wno-unused-parameter

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc/

LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

LOCAL_MODULE := libcamcommon

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils liblog

ifeq ($(PLATFORM_VERSION),8.0.0)
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/
LOCAL_PROPRIETARY_MODULE := true
endif


include $(BUILD_SHARED_LIBRARY)
