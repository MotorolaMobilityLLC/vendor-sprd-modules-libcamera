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
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH_32 := armeabi-v7a
LIB_PATH_64 := arm64-v8a
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH_32 := x86_lib
LIB_PATH_64 := x86_lib64
endif

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE := libawb1
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH_32)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH_64)/$(LOCAL_MODULE).so
ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_MODULE_STEM := $(notdir $(LIB_PATH_32)/$(LOCAL_MODULE).so)
LOCAL_SRC_FILES := $(LIB_PATH_32)/$(LOCAL_MODULE).so
endif
LOCAL_SHARED_LIBRARIES +=liblog

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH_32 := armeabi-v7a
LIB_PATH_64 := arm64-v8a
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH_32 := x86_lib
LIB_PATH_64 := x86_lib64
endif

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE := libawb
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH_32)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH_64)/$(LOCAL_MODULE).so
ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_MODULE_STEM := $(notdir $(LIB_PATH_32)/$(LOCAL_MODULE).so)
LOCAL_SRC_FILES := $(LIB_PATH_32)/$(LOCAL_MODULE).so
endif
LOCAL_SHARED_LIBRARIES +=liblog

include $(BUILD_PREBUILT)

