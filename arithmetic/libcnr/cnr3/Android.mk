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

ifneq ($(filter $(strip $(TARGET_BOARD_PLATFORM)),ums512 sp9863a sp9832e ums312 sp7731e),)

ifeq ($(strip $(TARGET_BOARD_CAMERA_CNR_CAPTURE)),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsprdcnr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdcnr.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdcnr.so

LOCAL_CFLAGS += -DCAMERA_CNR3_ENABLE
LOCAL_CFLAGS += -DCAMERA_RADIUS_ENABLE

LOCAL_SHARED_LIBRARIES := libc libdl liblog libm
LOCAL_CHECK_ELF_FILES := false

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)

### adapter ###
include $(CLEAR_VARS)
LOCAL_SRC_FILES := ../src/sprd_yuv_denoise_adapter.cpp
LOCAL_MODULE := libsprdcnradapter
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -O3 -fno-strict-aliasing -fPIC -fvisibility=hidden
LOCAL_SHARED_LIBRARIES := libcutils liblog libsprdcnr libsprdcamalgassist

LOCAL_C_INCLUDES := \
         $(LOCAL_PATH)/../inc \
         $(LOCAL_PATH)/../../inc \
         $(TOP)/system/core/include/cutils/ \
         $(TOP)/system/core/include/ \
         $(LOCAL_PATH)/../../sprd_camalg_assist

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CFLAGS += -DCAMERA_CNR3_ENABLE
LOCAL_CFLAGS += -DCAMERA_RADIUS_ENABLE

#ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
#LOCAL_CFLAGS += -DDEFAULT_RUNTYPE_VDSP
#else
LOCAL_CFLAGS += -DDEFAULT_RUNTYPE_CPU
#endif

include $(BUILD_SHARED_LIBRARY)
endif
endif
