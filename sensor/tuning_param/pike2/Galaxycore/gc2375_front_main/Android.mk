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
# ---------------------------------------------------------------------------
#                      Make the shared library
# ---------------------------------------------------------------------------
ifeq ($(strip $(CHIP_NAME)),pike2)
PARAM_LIST := $(TUNING_PARAM_LIST)
PARAM_LIST :=$(strip $(subst ",,$(PARAM_LIST)))
PARAM_NAME := $(notdir $(call my-dir))
FOUND_NAME := $(findstring $(PARAM_NAME),$(PARAM_LIST))
ifeq ($(strip $(PARAM_NAME)),$(FOUND_NAME))

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../../SprdCtrl.mk

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/common/inc \
		$(LOCAL_PATH)/../../../../../common/inc \
		$(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/ae/sprd/ae/inc


LOCAL_SRC_FILES := param_manager.c

LOCAL_SHARED_LIBRARIES := libcutils libdl libutils

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE           := libparam_$(PARAM_NAME)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

#include $(call first-makefiles-under,$(LOCAL_PATH))
endif
endif
