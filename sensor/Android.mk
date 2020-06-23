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
LOCAL_PATH_BAK:= $(call my-dir)
LOCAL_SENSOR_PATH:= $(call my-dir)
LOCAL_OTP_PATH:= $(call my-dir)
LOCAL_VCM_PATH:= $(call my-dir)

ISPALG_DIR := ispalg

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Wno-error=format#-Werror

LOCAL_C_INCLUDES := \
    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
    $(LOCAL_PATH)/../common/inc \
    $(LOCAL_PATH)/../jpeg \
    $(LOCAL_PATH)/../vsp/inc \
    $(LOCAL_PATH)/../tool/mtrace \
    $(LOCAL_PATH)/dummy \
    $(LOCAL_PATH)/../$(OEM_DIR)/inc \
    $(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/otp_parser \
    $(LOCAL_PATH)/../kernel_module/interface

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/middleware/inc \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/isp_tune \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/calibration \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/driver/inc \
    $(LOCAL_PATH)/../$(ISPDRV_DIR)/param_manager \
    $(LOCAL_PATH)/../$(ISPALG_DIR)/ae/inc \
    $(LOCAL_PATH)/../$(ISPALG_DIR)/ae/sprd/ae2.x/ae/inc \
    $(LOCAL_PATH)/../$(ISPALG_DIR)/ae/sprd/ae3.x/ae/inc \
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

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

include $(LOCAL_PATH)/../SprdCtrl.mk
include $(LOCAL_PATH)/hw_drv/Sprdroid.mk
include $(LOCAL_PATH)/af_drv/Sprdroid.mk
include $(LOCAL_PATH)/otp_drv/Sprdroid.mk
include $(LOCAL_PATH)/sensor_drv/Sprdroid.mk
include $(LOCAL_PATH)/tuning_param/Sprdroid.mk
#include $(LOCAL_PATH)/tuning_param/tunning_lib_cfg.mk


LOCAL_SRC_FILES += \
    dummy/isp_otp_calibration.c \
    sensor_drv_u.c \
    sensor_drv_xml_parse.c

ifeq ($(strip $(TARGET_CAMERA_OIS_FUNC)),true)
LOCAL_C_INCLUDES += ois
LOCAL_SRC_FILES+= \
    ois/OIS_func.c \
    ois/OIS_user.c \
    ois/OIS_main.c
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/otp_cali/otp_cali.h
LOCAL_SRC_FILES += otp_cali/otp_cali.c

ifeq ($(strip $(TARGET_CAMERA_SENSOR_CCT)),"ams_tcs3430")
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ams/tcs3430/tcs_3430_drv.h
LOCAL_SRC_FILES+= ams/tcs3430/tcs_3430_drv.c
endif

ifeq ($(strip $(TARGET_BOARD_CONFIG_CAMERA_RT_REFOCUS)),true)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/al3200
LOCAL_SRC_FILES += al3200/al3200.c
endif

LOCAL_MODULE := libcamsensor

LOCAL_MODULE_TAGS := optional

LOCAL_HEADER_LIBRARIES += libutils_headers

LOCAL_SHARED_LIBRARIES := libcutils libcamcommon libdl libxml2

LOCAL_SHARED_LIBRARIES += liblog libcam_otp_parser

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 7)))
LOCAL_CFLAGS += -DCONFIG_USE_CAMERASERVER_PROC
endif

ifeq ($(strip $(TARGET_BOARD_SENSOR_OV8856_TELE)),true)
LOCAL_CFLAGS += -DSENSOR_OV8856_TELE
endif

ifeq ($(strip $(TARGET_BOARD_SENSOR_OV8856_H_MIRROR)),true)
LOCAL_CFLAGS += -DSENSOR_OV8856_H_MIRROR
endif

include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/otp_parser/Android.mk
LOCAL_PATH:= $(LOCAL_PATH_BAK)

include $(wildcard $(LOCAL_OTP_PATH)/otp_drv/*/*/Android.mk)
include $(wildcard $(LOCAL_VCM_PATH)/af_drv/*/Android.mk)
include $(wildcard $(LOCAL_SENSOR_PATH)/*/*/*/*/Android.mk)
LOCAL_PATH:= $(LOCAL_PATH_BAK)
#include $(call all-subdir-makefiles)
#include $(call first-makefiles-under,$(LOCAL_PATH))
