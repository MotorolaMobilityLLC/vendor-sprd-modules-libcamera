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
ifeq ($(strip $(TARGET_BOARD_VDSP_UTEST)), true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    utest_vdsp.cpp \

LOCAL_C_INCLUDES:= \
	$(TOP)/vendor/sprd/modules/libmemion/ \
    $(TOP)/vendor/sprd/external/kernel-headers \
	$(TOP)/vendor/sprd/modules/libcamera/$(OEM_DIR)/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(ISPALG_DIR)/common/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(ISPDRV_DIR)/middleware/inc \
	$(TOP)/vendor/sprd/modules/libcamera/$(ISPDRV_DIR)/driver/inc \
	$(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
	$(TOP)/vendor/sprd/modules/libcamera/common/inc \
	$(TOP)/vendor/sprd/modules/libcamera/arithmetic/inc

ifeq ($(strip $(TARGET_BOARD_CAMERA_MODULAR)),true)
LOCAL_C_INCLUDES += \
		$(TOP)/vendor/sprd/modules/libcamera/kernel_module/interface
endif

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SHARED_LIBRARIES :=libmemion liblog libEGL libbinder libutils
LOCAL_SHARED_LIBRARIES += libcamcommon libcamoem
ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_SHARED_LIBRARIES += libsprdhdr
endif

LOCAL_MODULE := utest_vdsp
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/bin/hw
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true

include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE := src1_yuv_420.raw
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/pic/
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
endif

