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

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
ALG_DIR := ispalg/isp2.x
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
ALG_DIR := ispalg/isp2.x
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
ALG_DIR := ispalg/isp3.x
endif


LOCAL_CFLAGS += -fno-strict-aliasing -Wunused-variable -Wunused-function -Werror
LOCAL_CFLAGS += -DLOCAL_INCLUDE_ONLY

# ************************************************
# external header file
# ************************************************
LOCAL_C_INCLUDES := \
	$(TARGET_BSP_UAPI_PATH)/kernel/usr/include/video \
	$(LOCAL_PATH)/../../../common/inc \
	$(LOCAL_PATH)/../../../kernel_module/interface

# ************************************************
# internal header file
# ************************************************
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/smart \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/awb/inc \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/ae/inc \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/ae/sprd/ae/inc \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/common/inc/ \
	$(LOCAL_PATH)/../middleware/inc \
	$(LOCAL_PATH)/../calibration/inc \
	$(LOCAL_PATH)/../isp_tune \
	$(LOCAL_PATH)/../driver/inc

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

#LOCAL_SRC_FILES += $(call all-c-files-under, .)
LOCAL_SRC_FILES := isp_pm.c isp_blocks_cfg.c isp_com_alg.c isp_param_file_update.c
LOCAL_SRC_FILES += $(call all-c-files-under, blk_comm)

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
LOCAL_SRC_FILES += $(call all-c-files-under, blk_v25)
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
LOCAL_SRC_FILES += $(call all-c-files-under, blk_v26)
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
LOCAL_SRC_FILES += blk_v26/isp_blk_3dnr.c \
	blk_v26/isp_blk_bchs.c \
	blk_v26/isp_blk_bpc.c \
	blk_v26/isp_blk_cnr2.c \
	blk_v26/isp_blk_edge.c \
	blk_v26/isp_blk_nlm.c \
	blk_v26/isp_blk_posterize.c \
	blk_v26/isp_blk_sw3dnr.c \
	blk_v26/isp_blk_uv_div.c \
	blk_v26/isp_blk_ynr.c \
	blk_v26/isp_blk_yuv_noisefilter.c
LOCAL_SRC_FILES += $(call all-c-files-under, blk_v27)
endif

include $(LOCAL_PATH)/../../../SprdCtrl.mk

LOCAL_MODULE := libcampm

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libdl libcamcommon liblog


ifeq (1, 1) #(strip $(shell expr $(ANDROID_MAJOR_VER) \>= 8)))
LOCAL_PROPRIETARY_MODULE := true
endif

LOCAL_CFLAGS += -DTEST_ON_HAPS
include $(BUILD_SHARED_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
