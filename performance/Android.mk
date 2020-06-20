LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -fno-strict-aliasing -Werror -Wno-unused-parameter
LOCAL_C_INCLUDES := $(LOCAL_PATH)/
LOCAL_HEADER_LIBRARIES := liblog_headers libutils_headers
LOCAL_SHARED_LIBRARIES := liblog libutils
LOCAL_SRC_FILES := libloader.cpp
LOCAL_MODULE := libcamperf
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)

