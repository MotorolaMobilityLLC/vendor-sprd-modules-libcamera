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

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../../common/inc \
	$(LOCAL_PATH)/../../oem2v1/inc \
	$(LOCAL_PATH)/../../oem2v1/isp_calibration/inc \
	$(LOCAL_PATH)/../../jpeg/inc \
	$(LOCAL_PATH)/../../vsp/inc \
	$(LOCAL_PATH)/../../tool/mtrace \
	$(LOCAL_PATH)/../../ispalg/isp2.1/ae/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/ae/sprd/ae/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/ae/flash/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/awb/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/awb/alc_awb/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/awb/sprd/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/af/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/af/sprd/afv1/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/af/sprd/aft/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/af/sft_af/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/af/alc_af/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/lsc/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/common/inc/ \
        $(LOCAL_PATH)/../../ispalg/isp2.1/afl/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/smart \
        $(LOCAL_PATH)/../../ispalg/isp2.1/pdaf/inc \
        $(LOCAL_PATH)/../../ispalg/isp2.1/pdaf/sprd/inc

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/middleware/inc \
	$(LOCAL_PATH)/isp_tune \
	$(LOCAL_PATH)/calibration \
	$(LOCAL_PATH)/driver/inc \
	$(LOCAL_PATH)/param_manager \
	$(LOCAL_PATH)/param_parse \
	$(LOCAL_PATH)/calibration/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

# don't modify this code
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name 'param_manager' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_MODULE := libcamdrv

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl libcamcommon libcampm

LOCAL_SHARED_LIBRARIES += libcamsensor #libcalibration

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
endif
