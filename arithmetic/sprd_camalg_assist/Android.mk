LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libsprdcamalgassist
LOCAL_SRC_FILES := sprd_camalg_assist.cpp

LOCAL_C_INCLUDES := vendor/sprd/modules/libmemion \
		    vendor/sprd/external/kernel-headers

LOCAL_SHARED_LIBRARIES := liblog libmemion libutils libcutils
LOCAL_CFLAGS += -Wall -Wextra -Wno-date-time -fvisibility=hidden

LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
