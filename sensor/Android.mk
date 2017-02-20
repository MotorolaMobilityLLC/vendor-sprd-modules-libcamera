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

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

ISP_HW_VER = 3v0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
ISP_HW_VER = 2v1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),3)
ISP_HW_VER = 3v0
endif

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/dummy \
	$(LOCAL_PATH)/../oem$(ISP_HW_VER)/inc \
	$(LOCAL_PATH)/../oem$(ISP_HW_VER)/isp_calibration/inc


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../isp2.1/middleware/inc \
	$(LOCAL_PATH)/../isp2.1/isp_tune \
	$(LOCAL_PATH)/../isp2.1/calibration \
	$(LOCAL_PATH)/../isp2.1/driver/inc \
	$(LOCAL_PATH)/../isp2.1/param_manager \
	$(LOCAL_PATH)/../isp2.1/ae/inc \
	$(LOCAL_PATH)/../isp2.1/ae/sprd_ae/inc \
	$(LOCAL_PATH)/../isp2.1/awb/inc \
	$(LOCAL_PATH)/../isp2.1/awb/alc_awb/inc \
	$(LOCAL_PATH)/../isp2.1/awb/sprd_awb/inc \
	$(LOCAL_PATH)/../isp2.1/af/inc \
	$(LOCAL_PATH)/../isp2.1/af/sprd_af/inc \
	$(LOCAL_PATH)/../isp2.1/af/sft_af/inc \
	$(LOCAL_PATH)/../isp2.1/af/alc_af/inc \
	$(LOCAL_PATH)/../isp2.1/lsc/inc \
	$(LOCAL_PATH)/../isp2.1/common/inc/ \
	$(LOCAL_PATH)/../isp2.1/afl/inc \
	$(LOCAL_PATH)/../isp2.1/smart \
	$(LOCAL_PATH)/../isp2.1/utility \
	$(LOCAL_PATH)/../isp2.1/calibration/inc

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

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

include $(LOCAL_PATH)/../SprdCtrl.mk
include $(LOCAL_PATH)/af_drv/Sprdroid.mk
include $(LOCAL_PATH)/otp_drv/Sprdroid.mk
include $(LOCAL_PATH)/sensor_drv/Sprdroid.mk

LOCAL_SRC_FILES += \
	dummy/isp_otp_calibration.c \
	sensor_cfg.c \
	sensor_drv_u.c

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
	LOCAL_C_INCLUDES += \
						ois

	LOCAL_SRC_FILES+= \
						ois/OIS_func.c \
						ois/OIS_user.c \
						ois/OIS_main.c
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/al3200
LOCAL_SRC_FILES += al3200/al3200.c
endif


LOCAL_MODULE := libcamsensor

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libcamcommon libdl libcamsensortuning

include $(BUILD_SHARED_LIBRARY)

include $(wildcard $(LOCAL_PATH)/*/*/*/*/Android.mk)
