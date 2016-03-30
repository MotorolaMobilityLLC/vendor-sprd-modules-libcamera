#
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/video \
	$(LOCAL_PATH)/../common/inc

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/middleware/inc \
	$(LOCAL_PATH)/vendor_common/altek/inc \
	$(LOCAL_PATH)/vendor_common/inc \
	$(LOCAL_PATH)/awb/inc \
	$(LOCAL_PATH)/awb/altek/inc \
	$(LOCAL_PATH)/af/inc \
	$(LOCAL_PATH)/af/altek/inc \
	$(LOCAL_PATH)/ae/inc \
	$(LOCAL_PATH)/ae/altek/inc \
	$(LOCAL_PATH)/afl/inc \
	$(LOCAL_PATH)/afl/altek/inc \
	$(LOCAL_PATH)/driver

# don't modify this code
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_FLICKER)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AFL_AUTO_DETECTION
endif

LOCAL_MODULE := libcamisp

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libdl libcamcommon

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
endif
