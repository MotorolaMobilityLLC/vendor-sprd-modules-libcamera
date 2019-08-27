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
ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfacebeauty
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfacebeauty.so
LOCAL_MODULE_STEM_64 := libsprdfacebeauty.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfacebeauty.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfacebeauty.so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfacebeauty_vdsp
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfacebeauty_vdsp.so
LOCAL_MODULE_STEM_64 := libsprdfacebeauty_vdsp.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfacebeauty_vdsp.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfacebeauty_vdsp.so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += liblog_headers

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/inc \
        $(LOCAL_PATH)/../inc

LOCAL_SRC_FILES += camera_face_beauty.c

LOCAL_CFLAGS += -DCONFIG_FACE_BEAUTY
include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_SHARED_LIBRARIES:= \
     liblog \
     libcutils \
     libsprdfacebeauty

LOCAL_MODULE := libcamfb
LOCAL_MODULE_TAGS := optional

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += liblog_headers

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/inc \
        $(LOCAL_PATH)/../inc

LOCAL_SRC_FILES += sprd_facebeauty_adapter.cpp

LOCAL_CFLAGS += -DCONFIG_FACE_BEAUTY
include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_SHARED_LIBRARIES:= \
     liblog \
     libcutils \
     libsprdfacebeauty \
     libsprdfacebeauty_vdsp

LOCAL_MODULE := libcamfacebeauty
LOCAL_MODULE_TAGS := optional

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

endif
