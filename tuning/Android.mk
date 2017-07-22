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

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter #-Werror

ISP_HW_VER = 3v0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/dummy \
	$(LOCAL_PATH)/../oem$(ISP_HW_VER)/inc \
	$(LOCAL_PATH)/../oem$(ISP_HW_VER)/isp_calibration/inc

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ISPALG_DIR = ispalg/isp2.x
ISPDRV_DIR = ispdrv/isp2.1
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/param_manager \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/ae/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/ae/sprd_ae/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/alc_awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/sprd_awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/sprd_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/sft_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/alc_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/lsc/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/common/inc/ \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/afl/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/smart \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/utility \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration/inc

endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.2)
ISPALG_DIR = ispalg/isp2.x
ISPDRV_DIR = ispdrv/isp2.2
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/param_manager \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/ae/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/ae/sprd_ae/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/alc_awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/sprd_awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/sprd_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/sft_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/alc_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/lsc/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/common/inc/ \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/afl/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/smart \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/utility \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.3)
ISPALG_DIR = ispalg/isp2.x
ISPDRV_DIR = ispdrv/isp2.3
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/param_manager \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/ae/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/ae/sprd_ae/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/alc_awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/awb/sprd_awb/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/sprd_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/sft_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/af/alc_af/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/lsc/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/common/inc/ \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/afl/inc \
	$(LOCAL_PATH)/../$(ISPALG_DIR)/smart \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/utility \
	$(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../isp3.0/dummy \
	$(LOCAL_PATH)/../isp3.0/common/inc \
	$(LOCAL_PATH)/../isp3.0/middleware/inc \
	$(LOCAL_PATH)/../isp3.0/vendor_common/altek/inc \
	$(LOCAL_PATH)/../isp3.0/vendor_common/inc \
	$(LOCAL_PATH)/../isp3.0/awb/inc \
	$(LOCAL_PATH)/../isp3.0/awb/altek/inc \
	$(LOCAL_PATH)/../isp3.0/af/inc \
	$(LOCAL_PATH)/../isp3.0/af/altek/inc \
	$(LOCAL_PATH)/../isp3.0/ae/inc \
	$(LOCAL_PATH)/../isp3.0/ae/altek/inc \
	$(LOCAL_PATH)/../isp3.0/afl/inc \
	$(LOCAL_PATH)/../isp3.0/afl/altek/inc
endif

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_SRC_FILES += \
				sensor_s5k3l2xx_raw_param_v3.c \
				sensor_s5k3p3sm_raw_param.c \
				sensor_s5k4h5yc_raw_param_v3.c \
				sensor_s5k4h8yx_raw_param_v3.c \
				sensor_s5k5e3yx_raw_param_v3.c


LOCAL_MODULE := libcamsensortuning

LOCAL_MODULE_TAGS := optional

ifeq ($(PLATFORM_VERSION),8.0.0)
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/lib/
LOCAL_PROPRIETARY_MODULE := true
endif

include $(BUILD_SHARED_LIBRARY)
