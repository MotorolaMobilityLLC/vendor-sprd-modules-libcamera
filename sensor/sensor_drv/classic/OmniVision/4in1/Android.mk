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
ifeq ($(strip $(TARGET_BOARD_SENSOR_OV4C)),true)
LOCAL_PATH := $(call my-dir)
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := lib/x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libfcell
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32  := $(LIB_PATH)/armeabi-v7a/libfcell.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)/arm64-v8a/libfcell.so
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE := $(LOCAL_PATH)/inc/fcell.h
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)
#include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE:= libsprd_fcell
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS:= optional
LOCAL_MULTILIB := both
LOCAL_SRC_FILES := sprd_fcell_process.c
#LOCAL_SRC_FILES_64 := sprd_fcell_process.c
#LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
#LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
#LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_CFLAGS :=  -O3 #-fno-strict-aliasing -fPIC -fvisibility=hidden -nodefaultlibs
LOCAL_SHARED_LIBRARIES += libfcell
LOCAL_EXPORT_C_INCLUDE := $(LOCAL_PATH)/inc/sprd_fcell.h
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_SHARED_LIBRARY)
endif
