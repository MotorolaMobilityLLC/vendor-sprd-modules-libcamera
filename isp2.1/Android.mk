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

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_DIR)),2.1)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_CFLAGS += -fno-strict-aliasing -Werror
LOCAL_CFLAGS += -fno-strict-aliasing

#AE_WORK_MOD_V0: Old ae algorithm + slow converge
#AE_WORK_MOD_V1: new ae algorithm + slow converge
#AE_WORK_MOD_V2: new ae algorithm + fast converge
LOCAL_CFLAGS += -DAE_WORK_MOD_V0

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../oem2v1/inc \
	$(LOCAL_PATH)/../oem2v1/isp_calibration/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/middleware/inc \
	$(LOCAL_PATH)/isp_tune \
	$(LOCAL_PATH)/calibration \
	$(LOCAL_PATH)/driver/inc \
	$(LOCAL_PATH)/param_manager \
	$(LOCAL_PATH)/ae/inc \
	$(LOCAL_PATH)/ae/sprd_ae/inc \
	$(LOCAL_PATH)/awb/inc \
	$(LOCAL_PATH)/awb/alc_awb/inc \
	$(LOCAL_PATH)/awb/sprd_awb/inc \
	$(LOCAL_PATH)/af/inc \
	$(LOCAL_PATH)/af/sprd_af/afv0/inc \
	$(LOCAL_PATH)/af/sprd_af/afv1/inc \
	$(LOCAL_PATH)/af/sprd_af/aft/inc \
	$(LOCAL_PATH)/af/sft_af/inc \
	$(LOCAL_PATH)/af/alc_af/inc \
	$(LOCAL_PATH)/pdaf/inc \
	$(LOCAL_PATH)/pdaf/sprd/inc \
	$(LOCAL_PATH)/lsc/inc \
	$(LOCAL_PATH)/common/inc/ \
	$(LOCAL_PATH)/afl/inc \
	$(LOCAL_PATH)/smart \
	$(LOCAL_PATH)/utility \
	$(LOCAL_PATH)/calibration/inc \

LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr

# don't modify this code
#LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g)
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH) -name 'backup' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g)

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_MODULE := libcamisp

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl libcamcommon

LOCAL_SHARED_LIBRARIES += libspaf libawb1 liblsc libcalibration libae
LOCAL_SHARED_LIBRARIES += libAF libsft_af_ctrl libaf_tune
LOCAL_SHARED_LIBRARIES += libaf_running
LOCAL_SHARED_LIBRARIES += libcamsensor

LOCAL_SHARED_LIBRARIES += libspafv1 libcutils
LOCAL_SHARED_LIBRARIES += libspcaftrigger

LOCAL_SHARED_LIBRARIES += libdeflicker
LOCAL_SHARED_LIBRARIES += libSprdPdAlgo

ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),true)
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AE
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
LOCAL_SHARED_LIBRARIES += libAl_Ais libAl_Ais_Sp
else
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
endif

include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
endif
