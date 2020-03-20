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
ifeq ($(strip $(TARGET_BOARD_CAMERA_CPP_USER_DRIVER)),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_BOARD_CAMERA_CPP_MODULAR_KERNEL)),lite_r5p0)
LOCAL_CFLAGS += -DCPP_LITE_R5P0
endif
LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Wunused-function  -Werror
LOCAL_CFLAGS += -DLOCAL_INCLUDE_ONLY
LOCAL_CFLAGS += -DTEST_ON_HAPS

# ************************************************
# external header file
# ************************************************
#define some folder here,use them when hal = hal3dummy
ifeq ($(strip $(TARGET_BOARD_CAMERA_FUNCTION_DUMMY)), true)
	OEM_DIR:=oem2v6
	ISPALG_DIR:=ispalg
	ISPDRV_DIR:=camdrv/isp2.7
endif

LOCAL_C_INCLUDES := \
	$(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
	$(LOCAL_PATH)/../../common/inc \
	$(LOCAL_PATH)/../../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../../$(ISPALG_DIR)/common/inc \
	$(LOCAL_PATH)/../../$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/../../$(ISPDRV_DIR)/driver/inc \
	$(LOCAL_PATH)/../../kernel_module/interface

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/algo/inc \
	$(LOCAL_PATH)/driver/inc

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SRC_FILES := $(call all-c-files-under, driver) \
	$(call all-c-files-under, algo)

LOCAL_MODULE := libcppdrv

LOCAL_MODULE_TAGS := optional

LOCAL_LDLIBS := -lm

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl liblog

LOCAL_SHARED_LIBRARIES += libcamcommon

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under, $(LOCAL_PATH))
endif
