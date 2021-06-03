LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror -Wno-error=format
#LOCAL_LDFLAGS += -ldl

LOCAL_SRC_FILES := cmr_memory.c \
				BufferPool.cpp \
				PoolManager.cpp

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_C_INCLUDES := \
    $(TOP)/system/core/libutils/ \
    $(TOP)/system/core/libcutils/include/ \
    $(TOP)/system/core/libutils/include/utils/ \
    $(TOP)/system/core/libsystem/include/

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils

LOCAL_MODULE := libmempool
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
