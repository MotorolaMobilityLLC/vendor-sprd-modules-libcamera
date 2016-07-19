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

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/dummy \
	$(LOCAL_PATH)/../oem2v0/inc \
	$(LOCAL_PATH)/../oem2v0/isp_calibration/inc

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
				ov5640/sensor_ov5640_mipi.c \
				ov5640/sensor_ov5640_mipi_raw.c \
				s5k3p3sm/sensor_s5k3p3sm_mipi_raw.c \
				imx230/sensor_imx230_mipi_raw.c \
				imx258/sensor_imx258_mipi_raw.c \
				imx132/sensor_imx132_mipi_raw.c \
				ov2680/sensor_ov2680_mipi_raw.c \
				af_bu64297gwz.c \
				s5k4h8yx/sensor_s5k4h8yx_mipi_raw.c \
				s5k5e3yx/sensor_s5k5e3yx_mipi_raw.c \
				vcm/vcm_dw9800.c \
				s5k3l2xx/sensor_s5k3l2xx_mipi_raw.c \
				s5k3l8xxm3/sensor_s5k3l8xxm3_mipi_raw.c\
				sp2509/sensor_sp2509_mipi_raw.c \
				ov8856/sensor_ov8856_mipi_raw.c \
				ov8856s/sensor_ov8856s_mipi_raw.c \
				vcm/vcm_dw9807.c \
				vcm/vcm_ak7371.c \
				ov13870/sensor_ov13870_mipi_raw.c \
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

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RE_FOCUS)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/al3200
LOCAL_SRC_FILES += al3200/al3200.c
endif


LOCAL_MODULE := libcamsensor

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libcamcommon libdl libcamsensortuning

include $(BUILD_SHARED_LIBRARY)

include $(wildcard $(LOCAL_PATH)/*/Android.mk)
