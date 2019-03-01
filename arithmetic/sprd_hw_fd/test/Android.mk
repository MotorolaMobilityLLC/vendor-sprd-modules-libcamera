# Copyright (C) 2012 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_BUILD_VARIANT),userdebug)

include $(CLEAR_VARS)
LOCAL_MODULE := fdhwdemo

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../inc
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libmemion
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/kernel-headers

LOCAL_SHARED_LIBRARIES += libsprdfd_hw libcutils libutils libmemion

LOCAL_CFLAGS := -O3

#LOCAL_CFLAGS += -DFD_DEBUG

LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES := sprd_fd_hw_test.cpp
LOCAL_LDFLAGS := -fPIE -pie
LOCAL_LDLIBS += -llog
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)
endif