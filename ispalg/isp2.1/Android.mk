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

#ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Werror

#AE_WORK_MOD_V0: Old ae algorithm + slow converge
#AE_WORK_MOD_V1: new ae algorithm + slow converge
#AE_WORK_MOD_V2: new ae algorithm + fast converge
LOCAL_CFLAGS += -DAE_WORK_MOD_V0

# ************************************************
# external header file
# ************************************************
ISP_DIR := ../../camdrv/isp2.1

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../../common/inc \
	$(LOCAL_PATH)/../../oem2v1/inc \
	$(LOCAL_PATH)/../../oem2v1/isp_calibration/inc \
	$(LOCAL_PATH)/../../jpeg/inc \
	$(LOCAL_PATH)/../../vsp/inc \
	$(LOCAL_PATH)/../../tool/mtrace

# ************************************************
# internal header file
# ************************************************
ISP_ALGO_DIR := .

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/$(ISP_DIR)/middleware/inc \
	$(LOCAL_PATH)/$(ISP_DIR)/isp_tune \
	$(LOCAL_PATH)/$(ISP_DIR)/calibration \
	$(LOCAL_PATH)/$(ISP_DIR)/driver/inc \
	$(LOCAL_PATH)/$(ISP_DIR)/param_manager \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/flash/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/awb/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/awb/alc_awb/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/awb/sprd/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/sprd/afv1/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/sprd/aft/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/sft_af/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/af/alc_af/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/pdaf/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/pdaf/sprd/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/lsc/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/common/inc/ \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/afl/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/smart \
	$(LOCAL_PATH)/$(ISP_DIR)/utility \
	$(LOCAL_PATH)/$(ISP_DIR)/calibration/inc

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

# don't modify this code
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

include $(LOCAL_PATH)/../../SprdCtrl.mk

LOCAL_MODULE := libCamAlgo

LOCAL_MODULE_TAGS := optional
#LOCAL_ALLOW_UNDEFINED_SYMBOLS := true

LOCAL_SHARED_LIBRARIES += libcampm

LOCAL_SHARED_LIBRARIES += libcutils libutils libdl libcamcommon
#LOCAL_SHARED_LIBRARIES := libcamcommon

LOCAL_SHARED_LIBRARIES += libawb1 liblsc libae libflash libsprdlsc
#LOCAL_SHARED_LIBRARIES += libAF libsft_af_ctrl libaf_tune
#LOCAL_SHARED_LIBRARIES += libaf_running
LOCAL_SHARED_LIBRARIES += libcamsensor

LOCAL_SHARED_LIBRARIES += libspafv1 libspcaftrigger

LOCAL_SHARED_LIBRARIES += libdeflicker
LOCAL_SHARED_LIBRARIES += libSprdPdAlgo

ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),true)
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AE
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
LOCAL_SHARED_LIBRARIES += libAl_Ais libAl_Ais_Sp
else
#LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_3AMOD)),1)
include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
endif

#endif
