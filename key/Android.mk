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

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror -Wno-error=format
LOCAL_LDFLAGS += -ldl
LOCAL_HEADER_LIBRARIES += liblog_headers
LOCAL_HEADER_LIBRARIES += jni_headers

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc/
BOARD_FILE_EXISTANCE := $(shell test -f $(LOCAL_PATH)/$(CHIP_NAME)/$(TARGET_BOARD)/key.c && echo EXIST)
ifeq ($(BOARD_FILE_EXISTANCE),EXIST)
$(warning 'Board Configure')
LOCAL_SRC_FILES += $(CHIP_NAME)/$(TARGET_BOARD)/key.c
else
$(warning 'Chip Configure')
LOCAL_SRC_FILES += $(CHIP_NAME)/key_common.c
endif
LOCAL_MODULE := libkey
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

