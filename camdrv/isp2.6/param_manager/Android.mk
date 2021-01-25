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

ALG_DIR := ispalg

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
	$(LOCAL_PATH)/../../../$(ALG_DIR)/ae/sprd/ae2.x/ae/inc \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/ae/sprd/ae3.x/ae/inc \
	$(LOCAL_PATH)/../../../$(ALG_DIR)/common/inc/ \
	$(LOCAL_PATH)/../middleware/inc \
	$(LOCAL_PATH)/../calibration/inc \
	$(LOCAL_PATH)/../isp_tune \
	$(LOCAL_PATH)/../driver/inc

#LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_BSP_UAPI_PATH)/kernel/usr

#LOCAL_SRC_FILES += $(call all-c-files-under, .)
LOCAL_SRC_FILES := isp_pm.c isp_blocks_cfg.c isp_com_alg.c isp_param_file_update.c

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.5)
LOCAL_SRC_FILES += blk_list/isp_block_com.c \
	blk_list/isp_blk_2d_lsc.c \
	blk_list/isp_blk_3dnr.c \
	blk_list/isp_blk_aem.c \
	blk_list/isp_blk_ae_adapt.c \
	blk_list/isp_blk_afm.c \
	blk_list/isp_blk_ai_pro.c \
	blk_list/isp_blk_awb_new.c \
	blk_list/isp_blk_blc.c \
	blk_list/isp_blk_bpc.c \
	blk_list/isp_blk_brightness.c \
	blk_list/isp_blk_cce.c \
	blk_list/isp_blk_cfa.c \
	blk_list/isp_blk_cmc10.c \
	blk_list/isp_blk_cnr2.c \
	blk_list/isp_blk_cnr3.c \
	blk_list/isp_blk_contrast.c \
	blk_list/isp_blk_dre.c \
	blk_list/isp_blk_dre_pro.c \
	blk_list/isp_blk_edge.c \
	blk_list/isp_blk_fb.c \
	blk_list/isp_blk_frgb_gamc.c \
	blk_list/isp_blk_grgb.c \
	blk_list/isp_blk_hsv.c \
	blk_list/isp_blk_hsv_new.c \
	blk_list/isp_blk_hue.c \
	blk_list/isp_blk_iircnr_iir.c \
	blk_list/isp_blk_iir_yrandom.c \
	blk_list/isp_blk_mfnr.c \
	blk_list/isp_blk_nlm.c \
	blk_list/isp_blk_posterize.c \
	blk_list/isp_blk_rgb_dither.c \
	blk_list/isp_blk_rgb_gain.c \
	blk_list/isp_blk_saturation.c \
	blk_list/isp_blk_uv_cdn.c \
	blk_list/isp_blk_uv_div.c \
	blk_list/isp_blk_uv_postcdn.c \
	blk_list/isp_blk_ynr.c \
	blk_list/isp_blk_ynrs.c \
	blk_list/isp_blk_yuv_noisefilter.c \
	blk_list/isp_blk_yuv_precdn.c \
	blk_list/isp_blk_yuv_ygamma.c
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.6)
LOCAL_SRC_FILES += blk_list/isp_block_com.c \
	blk_list/isp_blk_2d_lsc.c \
	blk_list/isp_blk_3dnr_v1.c \
	blk_list/isp_blk_aem.c \
	blk_list/isp_blk_afm.c \
	blk_list/isp_blk_awb_new.c \
	blk_list/isp_blk_bchs.c \
	blk_list/isp_blk_blc.c \
	blk_list/isp_blk_bpc_v1.c \
	blk_list/isp_blk_cce.c \
	blk_list/isp_blk_cfa.c \
	blk_list/isp_blk_cmc10.c \
	blk_list/isp_blk_cnr2_v1.c \
	blk_list/isp_blk_cnr3.c \
	blk_list/isp_blk_dre_pro.c \
	blk_list/isp_blk_edge_v1.c \
	blk_list/isp_blk_fb.c \
	blk_list/isp_blk_frgb_gamc.c \
	blk_list/isp_blk_grgb.c \
	blk_list/isp_blk_hsv_v1.c \
	blk_list/isp_blk_iircnr_iir.c \
	blk_list/isp_blk_iir_yrandom.c \
	blk_list/isp_blk_imblance.c \
	blk_list/isp_blk_ltm.c \
	blk_list/isp_blk_mfnr.c \
	blk_list/isp_blk_nlm_v1.c \
	blk_list/isp_blk_posterize_v1.c \
	blk_list/isp_blk_ppe.c \
	blk_list/isp_blk_rgb_dither.c \
	blk_list/isp_blk_rgb_gain.c \
	blk_list/isp_blk_sw3dnr.c \
	blk_list/isp_blk_uv_cdn.c \
	blk_list/isp_blk_uv_div_v1.c \
	blk_list/isp_blk_uv_postcdn.c \
	blk_list/isp_blk_ynr_v1.c \
	blk_list/isp_blk_ynrs_v1.c \
	blk_list/isp_blk_yuv_noisefilter_v1.c \
	blk_list/isp_blk_yuv_precdn.c \
	blk_list/isp_blk_yuv_ygamma.c
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.7)
LOCAL_SRC_FILES += blk_list/isp_block_com.c \
	blk_list/isp_blk_2d_lsc.c \
	blk_list/isp_blk_3dnr_v1.c \
	blk_list/isp_blk_aem.c \
	blk_list/isp_blk_ae_adapt.c \
	blk_list/isp_blk_afm.c \
	blk_list/isp_blk_ai_pro_v1.c \
	blk_list/isp_blk_awb_new.c \
	blk_list/isp_blk_bchs.c \
	blk_list/isp_blk_blc.c \
	blk_list/isp_blk_bpc_v1.c \
	blk_list/isp_blk_cce.c \
	blk_list/isp_blk_cfa.c \
	blk_list/isp_blk_cmc10.c \
	blk_list/isp_blk_cnr2_v1.c \
	blk_list/isp_blk_cnr3.c \
	blk_list/isp_blk_dre.c \
	blk_list/isp_blk_dre_pro.c \
	blk_list/isp_blk_edge_v1.c \
	blk_list/isp_blk_fb.c \
	blk_list/isp_blk_frgb_gamc.c \
	blk_list/isp_blk_grgb.c \
	blk_list/isp_blk_hsv_v2.c \
	blk_list/isp_blk_iircnr_iir.c \
	blk_list/isp_blk_iir_yrandom.c \
	blk_list/isp_blk_imblance_v1.c \
	blk_list/isp_blk_mfnr.c \
	blk_list/isp_blk_nlm_v1.c \
	blk_list/isp_blk_posterize_v1.c \
	blk_list/isp_blk_ppe_v1.c \
	blk_list/isp_blk_raw_gtm.c \
	blk_list/isp_blk_rgb_dither.c \
	blk_list/isp_blk_rgb_gain.c \
	blk_list/isp_blk_rgb_ltm.c \
	blk_list/isp_blk_sw3dnr.c \
	blk_list/isp_blk_uv_cdn.c \
	blk_list/isp_blk_uv_div_v1.c \
	blk_list/isp_blk_uv_postcdn.c \
	blk_list/isp_blk_ynr_v1.c \
	blk_list/isp_blk_ynrs_v1.c \
	blk_list/isp_blk_yuv_ltm.c \
	blk_list/isp_blk_yuv_noisefilter_v1.c \
	blk_list/isp_blk_yuv_precdn.c \
	blk_list/isp_blk_yuv_ygamma_v1.c
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.8)
LOCAL_SRC_FILES += blk_list/isp_block_com.c \
	blk_list/isp_blk_2d_lsc_v1.c \
	blk_list/isp_blk_3dnr_v1.c \
	blk_list/isp_blk_aem.c \
	blk_list/isp_blk_ae_adapt.c \
	blk_list/isp_blk_afm.c \
	blk_list/isp_blk_awb_new.c \
	blk_list/isp_blk_bchs.c \
	blk_list/isp_blk_blc.c \
	blk_list/isp_blk_bpc_v1.c \
	blk_list/isp_blk_cce.c \
	blk_list/isp_blk_cfa.c \
	blk_list/isp_blk_cmc10.c \
	blk_list/isp_blk_cnr2_v1.c \
	blk_list/isp_blk_cnr3.c \
	blk_list/isp_blk_dre.c \
	blk_list/isp_blk_dre_pro.c \
	blk_list/isp_blk_edge_v1.c \
	blk_list/isp_blk_fb.c \
	blk_list/isp_blk_frgb_gamc.c \
	blk_list/isp_blk_iircnr_iir.c \
	blk_list/isp_blk_iir_yrandom.c \
	blk_list/isp_blk_imblance_v2.c \
	blk_list/isp_blk_mfnr.c \
	blk_list/isp_blk_nlm_v1.c \
	blk_list/isp_blk_ppe_v1.c \
	blk_list/isp_blk_rgb_dither.c \
	blk_list/isp_blk_rgb_gain.c \
	blk_list/isp_blk_rgb_ltm.c \
	blk_list/isp_blk_sw3dnr.c \
	blk_list/isp_blk_uv_cdn.c \
	blk_list/isp_blk_uv_div_v1.c \
	blk_list/isp_blk_uv_postcdn.c \
	blk_list/isp_blk_ynr_v1.c \
	blk_list/isp_blk_ynrs_v1.c \
	blk_list/isp_blk_yuv_noisefilter_v1.c \
	blk_list/isp_blk_yuv_precdn.c \
	blk_list/isp_blk_yuv_ygamma_v1.c \
	blk_list/isp_blk_hsv_v3.c \
	blk_list/isp_blk_ai_pro_v2.c \
	blk_list/isp_blk_raw_gtm.c
else ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_VERSION)),2.9)
LOCAL_SRC_FILES += blk_list/isp_block_com.c \
	blk_list/isp_blk_aem.c \
	blk_list/isp_blk_ae_adapt.c \
	blk_list/isp_blk_awb_new.c \
	blk_list/isp_blk_bchs.c \
	blk_list/isp_blk_blc.c \
	blk_list/isp_blk_cce.c \
	blk_list/isp_blk_cmc10.c \
	blk_list/isp_blk_cnr3.c \
	blk_list/isp_blk_dre.c \
	blk_list/isp_blk_dre_pro.c \
	blk_list/isp_blk_fb.c \
	blk_list/isp_blk_frgb_gamc.c \
	blk_list/isp_blk_iir_yrandom.c \
	blk_list/isp_blk_mfnr.c \
	blk_list/isp_blk_nlm_v1.c \
	blk_list/isp_blk_ppe_v1.c \
	blk_list/isp_blk_rgb_dither.c \
	blk_list/isp_blk_rgb_gain.c \
	blk_list/isp_blk_sw3dnr.c \
	blk_list/isp_blk_uv_cdn.c \
	blk_list/isp_blk_ynrs_v1.c \
	blk_list/isp_blk_rgb_ltm.c \
	blk_list/isp_blk_yuv_noisefilter_v1.c \
	blk_list/isp_blk_yuv_ygamma_v1.c
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
