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
ifeq ($(strip $(TARGET_BOARD_SENSOR_OV4C)),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/../../../../../SprdCtrl.mk

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../inc \
                    $(LOCAL_PATH)/../../../../../$(OEM_DIR)/inc \
                    $(LOCAL_PATH)/../../../../../common/inc \
                    $(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/isp_tune \
                    $(LOCAL_PATH)/../../../../../$(ISPALG_DIR)/common/inc \
                    $(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/middleware/inc \
                    $(LOCAL_PATH)/../../../../../$(ISPDRV_DIR)/driver/inc \
                    $(LOCAL_PATH)/../../../../../kernel_module/interface \
                    $(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
                    $(LOCAL_PATH)/../../../../otp_parser \
                    $(LOCAL_PATH)/../4in1/inc \
                    sensor_ov16885_mipi_raw.h


LOCAL_SRC_FILES := sensor_ov16885_mipi_raw.c

LOCAL_SHARED_LIBRARIES := libcutils libcamcommon libdl libutils libcamsensor liblog libxml2 libcam_otp_parser \
                          libsprd_fcell

ifeq (1, $(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_MODULE := libsensor_ov16885
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

#include $(call first-makefiles-under,$(LOCAL_PATH))
endif
