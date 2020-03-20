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

LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Werror


# ************************************************
# external header file
# ************************************************

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
ISP_DIR := ../camdrv/isp2.3
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.4)
ISP_DIR := ../camdrv/isp2.4
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
ISP_DIR := ../camdrv/isp2.6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
ISP_DIR := ../camdrv/isp2.6
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
ISP_DIR := ../camdrv/isp2.6
endif


LOCAL_C_INCLUDES := \
	$(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../jpeg \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/../kernel_module/interface

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
	$(LOCAL_PATH)/$(ISP_DIR)/bridge \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae3.x/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae2.x/ae/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae3.x \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae2.x \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae2.x/flash/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae3.x/flash/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae2.x/hdr/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ae/sprd/ae3.x/hdr/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/atm/sprd/inc\
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
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/lsc/sprd/alsc2_1/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/lsc/sprd/alsc3_0/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/common/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/afl/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/smart \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/tof \
	$(LOCAL_PATH)/$(ISP_DIR)/utility \
	$(LOCAL_PATH)/$(ISP_DIR)/calibration/inc \
	$(LOCAL_PATH)/../sensor/inc \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ai/inc \
	$(LOCAL_PATH)/$(ISP_ALGO_DIR)/ai/sprd/inc

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

# don't modify this code
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_MODULE := libispalg

LOCAL_MODULE_TAGS := optional

LOCAL_HEADER_LIBRARIES += jni_headers

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_SHARED_LIBRARIES += liblog
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_SHARED_LIBRARIES += libcampm
LOCAL_SHARED_LIBRARIES += libcutils libutils libdl libcamcommon libxml2

LOCAL_SHARED_LIBRARIES += libcamsensor

LOCAL_SHARED_LIBRARIES += libdeflicker

LOCAL_SHARED_LIBRARIES += libae libae3.x libflash libhdr
LOCAL_SHARED_LIBRARIES += libawb libawb1

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
LOCAL_SHARED_LIBRARIES += libalsc
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
LOCAL_SHARED_LIBRARIES += libalsc
else
LOCAL_SHARED_LIBRARIES += liblsc libsprdlsc
endif

LOCAL_SHARED_LIBRARIES += libatm
LOCAL_SHARED_LIBRARIES += libSprdPdAlgo
LOCAL_SHARED_LIBRARIES += libsprdaic libsprdscenedetect

LOCAL_SHARED_LIBRARIES += libspcaftrigger
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.3)
LOCAL_SHARED_LIBRARIES += libspafv1_le
else
LOCAL_SHARED_LIBRARIES += libspafv1
endif



include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))

