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
ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_PATH := $(call my-dir)
ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_SPRD_LIB)),true)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsprd_easy_hdr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/$(LOCAL_MODULE).so

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),1)
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsprd_easy_hdr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/$(LOCAL_MODULE).so

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),2)
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := x86_lib
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libsprdhdr
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM_64 := $(LOCAL_MODULE).so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/$(LOCAL_MODULE).so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/$(LOCAL_MODULE).so

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/sprd_hdr_adapter.cpp
LOCAL_MODULE := libsprdhdradapter
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS := -O3 -fno-strict-aliasing -fPIC -fvisibility=hidden -Wno-error=unused-parameter
LOCAL_SHARED_LIBRARIES := libcutils liblog

ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
else ifeq ($(strip $(TARGET_BOARD_SPRD_HDR_VERSION)),2)
LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB_VERSION_2
LOCAL_SHARED_LIBRARIES += libsprdhdr
endif

LOCAL_C_INCLUDES := \
         $(LOCAL_PATH)/inc \
		 $(LOCAL_PATH)/../inc \
         $(TOP)/system/core/include/cutils/ \
         $(TOP)/system/core/include/

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

ifneq ($(filter $(TARGET_BOARD_PLATFORM), ums512), )
LOCAL_CFLAGS += -DDEFAULT_RUNTYPE_VDSP
endif

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under, $(LOCAL_PATH))

endif
