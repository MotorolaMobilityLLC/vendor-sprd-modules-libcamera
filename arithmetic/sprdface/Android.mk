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
ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), arm arm64))
LIB_PATH := lib/lib
else ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH), x86 x86_64))
LIB_PATH := lib/x86_lib
endif


#SPRD face detect
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfd
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfd.so
LOCAL_MODULE_STEM_64 := libsprdfd.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfd.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfd.so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)


# SPRD face alignment library
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfa
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfa.so
LOCAL_MODULE_STEM_64 := libsprdfa.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfa.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfa.so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_PREBUILT)
ifeq ($(strip $(TARGET_BOARD_SPRD_FD_VERSION)),1)
# SPRD face attribute recognition (smile detection) library
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfarcnn
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfarcnn.so
LOCAL_MODULE_STEM_64 := libsprdfarcnn.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfarcnn.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfarcnn.so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_MODULE := libsprdfar
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsprdfar.so
LOCAL_MODULE_STEM_64 := libsprdfar.so
LOCAL_SRC_FILES_32 := $(LIB_PATH)/libsprdfar.so
LOCAL_SRC_FILES_64 := $(LIB_PATH)64/libsprdfar.so
LOCAL_MODULE_TAGS := optional
ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif
include $(BUILD_PREBUILT)
endif
endif
