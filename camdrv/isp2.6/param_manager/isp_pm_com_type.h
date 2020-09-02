
/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_PM_COM_TYPE_H_
#define _ISP_PM_COM_TYPE_H_

#include "cmr_types.h"
#include "isp_type.h"

#ifdef	 __cplusplus
extern "C" {
#endif


#define ISP_TUNE_MODE_MAX 16
#define ISP_TUNE_BLOCK_MAX 64
#define ISP_TUNE_MODE_INVALID 0xff
#define ISP_PM_MAGIC_FLAG        0xFFEE5511
#define ISP_PM_BLK_UPDATE   (1 << 31)

#define PM_VER_CHIP_MASK		(0xFFFF0000)
#define PM_VER_SW_MASK		(0x0000FFFF)

enum {
	PM_CHIP_VER_V25 = 0x00090000,
	PM_CHIP_VER_V26 = 0x000A0000,
	PM_CHIP_VER_V27 = 0x000B0000,
};

enum {
	PM_VER_SW_V25 = 0x00000007,
	PM_VER_SW_V26 = 0x00000008,
	PM_VER_SW_V27 = 0x00000009,
};

#ifndef MAX
#define MAX(a, b) ((a > b) ? a : b)
#endif
#ifndef MIN
#define MIN(a, b) ((a < b) ? a : b)
#endif

enum {
	PARAM_SET0 = 0,
	PARAM_SET1,
	PARAM_SET2,
	PARAM_SET_MAX
};

enum isp_pm_blk_cmd {
	ISP_PM_BLK_ISP_SETTING = 0x0000,
	ISP_PM_BLK_SMART_SETTING,
	ISP_PM_BLK_SCENE_MODE,
	ISP_PM_BLK_SPECIAL_EFFECT,

	ISP_PM_BLK_BLC_BASE = 0x0100,
	ISP_PM_BLK_BLC_BYPASS,
	ISP_PM_BLK_BLC_OFFSET,
	ISP_PM_BLK_BLC_OFFSET_GB,

	ISP_PM_BLK_GBL_GAIN_BASE = 0x0200,
	ISP_PM_BLK_GBL_GAIN_BYPASS,
	ISP_PM_BLK_GBL_GAIN,

	ISP_PM_BLK_RGB_DITHER_BASE = 0x0300,
	ISP_PM_BLK_RGB_DITHER_BYPASS,
	ISP_PM_BLK_RGB_DITHER_CFG,

	ISP_PM_BLK_LSC_BASE = 0x0400,
	ISP_PM_BLK_LSC_MEM_ADDR,
	ISP_PM_BLK_LSC_INFO,
	ISP_PM_BLK_LSC_GET_LSCTAB,
	ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE = (1 << 0),
	ISP_PM_BLK_LSC_UPDATE_MASK_PARAM = (1 << 1),
	ISP_PM_BLK_LSC_UPDATE_GRID,

	ISP_PM_BLK_BPC_BASE = 0x0500,
	ISP_PM_BLK_BPC_BYPASS,
	ISP_PM_BLK_BPC,
	ISP_PM_BLK_BPC_MODE,
	ISP_PM_BLK_BPC_THRD,
	ISP_PM_BLK_BPC_MAP_ADDR,

	ISP_PM_BLK_HIST_BASE = 0x0600,
	ISP_PM_BLK_HIST_BYPASS,
	ISP_PM_BLK_HIST,
	ISP_PM_BLK_HIST_RATIO,
	ISP_PM_BLK_HIST_MODE,
	ISP_PM_BLK_HIST_RANGE,

	ISP_PM_BLK_AEM_BASE = 0x0680,
	ISP_PM_BLK_AEM_WIN,

	ISP_PM_BLK_AWBC_BASE = 0x0700,
	ISP_PM_BLK_AWBC_BYPASS,
	ISP_PM_BLK_AWBC,

	ISP_PM_BLK_RGB_AFM_BASE = 0x0800,
	ISP_PM_BLK_RGB_AFM_BYPASS,

	ISP_PM_BLK_GRGB_BASE = 0x0900,
	ISP_PM_BLK_GRGB_BYPASS,
	ISP_PM_BLK_GRGB_GAIN,

	ISP_PM_BLK_CFA_BASE = 0x0A00,
	ISP_PM_BLK_CFA,
	ISP_PM_BLK_CFA_BYPASS,

	ISP_PM_BLK_GAMMA_BASE = 0x0B00,
	ISP_PM_BLK_GAMMA_BYPASS,
	ISP_PM_BLK_GAMMA,
	ISP_PM_BLK_GAMMA_TAB,
	ISP_PM_BLK_GAMMA_CUR,

	ISP_PM_BLK_YGAMMA_BASE = 0x0C00,
	ISP_PM_BLK_YGAMMA_BYPSS,
	ISP_PM_BLK_YGAMMA,

	ISP_PM_BLK_CCE_BASE = 0x0D00,
	ISP_PM_BLK_CCE,

	ISP_PM_BLK_UV_DIV_BASE = 0x0E00,
	ISP_PM_BLK_UV_DIV_BYPSS,
	ISP_PM_BLK_UV_DIV,

	ISP_PM_BLK_BCHS_BASE = 0x0F00,
	ISP_PM_BLK_BCHS_BYPASS,
	ISP_PM_BLK_BRIGHT_BYPASS,
	ISP_PM_BLK_BRIGHT,
	ISP_PM_BLK_CONTRAST_BYPASS,
	ISP_PM_BLK_CONTRAST,
	ISP_PM_BLK_SATURATION_BYPASS,
	ISP_PM_BLK_SATURATION,
	ISP_PM_BLK_HUE_BYPASS,
	ISP_PM_BLK_HUE,

	ISP_PM_BLK_EDGE_BASE = 0x1000,
	ISP_PM_BLK_EDGE_BYPASS,
	ISP_PM_BLK_EDGE,
	ISP_PM_BLK_EDGE_STRENGTH,
	ISP_PM_BLK_EDGE_DETAIL_THRD,
	ISP_PM_BLK_EDGE_SMOOTH_THRD,

	ISP_PM_BLK_NLC_BASE = 0x1100,
	ISP_PM_BLK_NLC_BYPASS,
	ISP_PM_BLK_NLC,

	ISP_PM_BLK_VST_BASE = 0x1200,
	ISP_PM_BLK_VST_BYPASS,

	ISP_PM_BLK_NLM_BASE = 0x1300,
	ISP_PM_BLK_NLM_BYPASS,
	ISP_PM_BLK_NLM_STRENGTH_LEVEL,
	ISP_PM_BLK_NLM_FDR_UPDATE,

	ISP_PM_BLK_IVST_BASE = 0x1400,
	ISP_PM_BLK_IVST_BYPASS,

	ISP_PM_BLK_CMC10_BASE = 0x1500,
	ISP_PM_BLK_CMC10_BYPASS,
	ISP_PM_BLK_CMC10,

	ISP_PM_BLK_PSTRZ_BASE = 0x1600,
	ISP_PM_BLK_PSTRZ_BYPASS,

	ISP_PM_BLK_YUV_PRECDN_BASE = 0x1700,
	ISP_PM_BLK_YUV_PRECDN_BYPASS,
	ISP_PM_BLK_YUV_PRECDN_STRENGTH_LEVEL,

	ISP_PM_BLK_HIST2_BASE = 0x1800,
	ISP_PM_BLK_HIST2_BYPASS,

	ISP_PM_BLK_UV_CDN_BASE_V1 = 0x1900,
	ISP_PM_BLK_UV_CDN_BYPASS_V1,

	ISP_PM_BLK_YUV_PRE_CDN_BASE = 0x1A00,
	ISP_PM_BLK_YUV_PRE_CDN_BYPASS,
	ISP_PM_BLK_YUV_PRE_CDN_STRENGTH_LEVEL,

	ISP_PM_BLK_IIR_NR_BASE = 0x1B00,
	ISP_PM_BLK_IIR_NR_STRENGTH_LEVEL,

	ISP_PM_BLK_IIR_YRANDOM_BASE = 0x1C00,
	ISP_PM_BLK_IIR_YRANDOM_BYPASS,

	ISP_PM_BLK_UV_POST_CDN_BASE = 0x1D00,
	ISP_PM_BLK_UV_POST_CDN_BYPASS,
	ISP_PM_BLK_UV_POST_CDN_STRENGTH_LEVEL,

	ISP_PM_BLK_PDAF_BASE = 0x1E00,
	ISP_PM_BLK_PDAF_BYPASS,
	ISP_PM_BLK_PDAF_GAIN,
	ISP_PM_BLK_PDAF_TUNE,

	ISP_PM_BLK_YNR_BASE = 0x1F00,
	ISP_PM_BLK_YNR_BYPASS,
	ISP_PM_BLK_YNR_GAIN,

	ISP_PM_BLK_3D_NR_BASE = 0x2000,
	ISP_PM_BLK_3D_NR_BYPASS,
	ISP_PM_BLK_3D_NR_STRENGTH_LEVEL,

	ISP_PM_BLK_YUV_NOISEFILTER_BASE = 0x2100,
	ISP_PM_BLK_YUV_NOISEFILTER_BYPASS,
	ISP_PM_BLK_YUV_NOISEFILTER_STRENGTH_LEVEL,

	ISP_PM_BLK_CNR2_BASE = (ISP_PM_BLK_YUV_NOISEFILTER_BASE + 0x100),
	ISP_PM_BLK_CNR2_LEVEL_INFO,

	ISP_PM_BLK_AE_ADAPT_BASE = (ISP_PM_BLK_CNR2_BASE + 0x100),
	ISP_PM_BLK_AE_ADAPT,

	ISP_PM_BLK_CNR3_BASE = (ISP_PM_BLK_AE_ADAPT_BASE + 0x100),
	ISP_PM_BLK_CNR3_LEVEL_INFO,

	ISP_PM_BLK_GTM_BASE = (ISP_PM_BLK_CNR3_BASE + 0x100),
	ISP_PM_BLK_GTM_STATUS,

	ISP_PM_BLK_AI_SCENE_BASE= (ISP_PM_BLK_GTM_BASE + 0x100),
	ISP_PM_BLK_AI_SCENE_BCHS_PARAM,
	ISP_PM_BLK_AI_SCENE_HSV_PARAM,
	ISP_PM_BLK_AI_SCENE_EE_PARAM,
	ISP_PM_BLK_AI_SCENE_SMOOTH,
	ISP_PM_BLK_AI_SCENE_UPDATE_HSV,
	ISP_PM_BLK_AI_SCENE_UPDATE_EE,
	ISP_PM_BLK_AI_SCENE_UPDATE_BCHS,
};

struct isp_pm_nrblk_info {
	cmr_u32 blk_id;
	cmr_u32 nr_type;
	cmr_u32 unit_size;
};

struct isp_pm_param_data {
	cmr_u32 mode_id;
	cmr_u32 id;
	cmr_u32 cmd;
	void *data_ptr;
	cmr_u32 data_size;
	cmr_u8 user_data[4];
};

struct isp_pm_block_header {
	char name[8];
	cmr_u32 block_id;
	cmr_u32 version_id;
	cmr_u32 param_id;
	cmr_u32 size;
	cmr_u32 bypass;
	cmr_u32 is_update;
	cmr_u32 source_flag;
	void *absolute_addr;
	cmr_u32 mode_id;
};

struct isp_pm_mode_param {
	char mode_name[8];
	cmr_u32 mode_id;
	cmr_u32 block_num;
	struct isp_size resolution;
	cmr_u32 fps;
	cmr_u32 for_4in1;
	struct isp_pm_block_header header[ISP_TUNE_BLOCK_MAX];
	cmr_u32 data_area[0];
};

struct isp_pm_blocks_param {
	cmr_u32 is_init;
	cmr_u32 block_num;
	struct isp_size resolution;
	cmr_u32 cus_define;
	cmr_u32 scene;
	cmr_u32 mode;
	cmr_u32 compatible_mode_id;
	struct isp_pm_block_header header[ISP_TUNE_BLOCK_MAX];
};

struct isp_pm_nr_header_param {
	cmr_u32 level_number;
	cmr_u32 default_strength_level;
	cmr_u32 nr_mode_setting;
	cmr_uint *multi_nr_map_ptr;
	cmr_uint *param_ptr;
	cmr_uint *param1_ptr;
	cmr_uint *param2_ptr;
	cmr_uint *param3_ptr;
};

struct isp_pm_nr_simple_header_param {
	cmr_u32 level_number;
	cmr_u32 default_strength_level;
	cmr_u32 nr_mode_setting;
	cmr_uint *multi_nr_map_ptr;
	cmr_uint *param_ptr;
	cmr_u32 param_size;
};

#ifdef	 __cplusplus
}
#endif
#endif
