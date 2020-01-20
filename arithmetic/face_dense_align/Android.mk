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
ifneq ($(TARGET_BOARD_PORTRAIT_SINGLE_SUPPORT)_$(TARGET_BOARD_PORTRAIT_SUPPORT), false_false)
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libdfa
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libdfa.so
LOCAL_MODULE_STEM_64 := libdfa.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libdfa.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libdfa.so
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_CHECK_ELF_FILES := false
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)


include $(CLEAR_VARS)

LOCAL_HEADER_LIBRARIES += libutils_headers
LOCAL_HEADER_LIBRARIES += liblog_headers

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/inc \
        $(LOCAL_PATH)/../inc

LOCAL_SRC_FILES += camera_face_dense_align.c

#LOCAL_CFLAGS += -DCONFIG_FACE_DFA
include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_SHARED_LIBRARIES:= \
     liblog \
     libcutils \
     libdfa

LOCAL_MODULE := libcamdfa
LOCAL_MODULE_TAGS := optional

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

endif
