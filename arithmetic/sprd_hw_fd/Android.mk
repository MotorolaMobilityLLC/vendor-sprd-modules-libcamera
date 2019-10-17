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

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib/lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := lib/x86_lib
endif


include $(CLEAR_VARS)



ifeq ($(TARGET_BOARD_CAMERA_FD_MODULAR_KERNEL),fd1.0)
LOCAL_SRC_FILES := src/sprd_fd_hw_api.cpp
else
LOCAL_SRC_FILES := src/sprd_fd_hw_api2.cpp
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libmemion
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/external/kernel-headers
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libcamera/kernel_module/interface

LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libmemion
#LOCAL_CFLAGS += -DFD_DEBUG
LOCAL_CFLAGS += -Wall -Wextra -Wno-date-time

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_CFLAGS += -DLOG_DEBUG
endif

LOCAL_MODULE := libsprdfd_hw
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))

