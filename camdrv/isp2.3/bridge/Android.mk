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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../../../common/inc

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/smart \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/awb/inc \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/ae/inc \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/ae/sprd/ae/inc \
	$(LOCAL_PATH)/../../../ispalg/isp2.x/common/inc/ \
	$(LOCAL_PATH)/../middleware/inc \
	$(LOCAL_PATH)/../param_manager \
	$(LOCAL_PATH)/../driver/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

# don't modify this code
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

include $(LOCAL_PATH)/../../../SprdCtrl.mk

LOCAL_MODULE := libcambr

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl liblog libcamcommon

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
