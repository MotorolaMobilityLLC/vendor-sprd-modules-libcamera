LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror -Wno-unused-parameter

ifeq ($(strip $(TARGET_BOARD_IS_SC_FPGA)),true)
LOCAL_CFLAGS += -DSC_FPGA=1
else
LOCAL_CFLAGS += -DSC_FPGA=0
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc/

LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

LOCAL_MODULE := libcamcommon

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_SHARED_LIBRARY)
