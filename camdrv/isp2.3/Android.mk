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

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Wunused-function -Werror
LOCAL_CFLAGS += -DLOCAL_INCLUDE_ONLY

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
	$(LOCAL_PATH)/../../common/inc \
	$(LOCAL_PATH)/../../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../../ispalg/ae/inc \
	$(LOCAL_PATH)/../../ispalg/ae/sprd/ae2.x/ae/inc \
	$(LOCAL_PATH)/../../ispalg/ae/sprd/ae3.x/ae/inc \
	$(LOCAL_PATH)/../../ispalg/ae/sprd/ae2.x/flash/inc \
	$(LOCAL_PATH)/../../ispalg/ae/sprd/ae3.x/flash/inc \
	$(LOCAL_PATH)/../../ispalg/awb/inc \
	$(LOCAL_PATH)/../../ispalg/awb/sprd/awb2.1/inc \
	$(LOCAL_PATH)/../../ispalg/awb/sprd/awb3.0/inc \
	$(LOCAL_PATH)/../../ispalg/awb/sprd/awb3.2/inc \
	$(LOCAL_PATH)/../../ispalg/af/inc \
	$(LOCAL_PATH)/../../ispalg/af/sprd/afv1/inc \
	$(LOCAL_PATH)/../../ispalg/af/sprd/aft/inc \
	$(LOCAL_PATH)/../../ispalg/lsc/inc \
	$(LOCAL_PATH)/../../ispalg/common/inc/ \
	$(LOCAL_PATH)/../../ispalg/afl/inc \
	$(LOCAL_PATH)/../../ispalg/smart \
	$(LOCAL_PATH)/../../ispalg/tof \
	$(LOCAL_PATH)/../../ispalg/pdaf/inc \
	$(LOCAL_PATH)/../../ispalg/pdaf/sprd/inc \
	$(LOCAL_PATH)/../../sensor/inc \
	$(LOCAL_PATH)/../../kernel_module/interface

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/middleware/inc \
	$(LOCAL_PATH)/isp_tune \
	$(LOCAL_PATH)/driver/inc \
	$(LOCAL_PATH)/param_manager \
	$(LOCAL_PATH)/bridge \
	$(LOCAL_PATH)/param_parse

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

LOCAL_SRC_FILES := $(call all-c-files-under, driver) \
    $(call all-c-files-under, isp_tune) \
    $(call all-c-files-under, middleware) \
    $(call all-c-files-under, param_parse)

include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_MODULE := libcamdrv

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl liblog

LOCAL_SHARED_LIBRARIES += libcamsensor libcambr libcamcommon libcampm libxml2

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
endif
