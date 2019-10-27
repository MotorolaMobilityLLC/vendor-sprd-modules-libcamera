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

#define LOG_TAG "isp_pm"

#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#include <string.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include "ae_tuning_type.h"
#include "isp_mw.h"
#include "isp_pm.h"
#include "isp_blocks_cfg.h"
#include "isp_param_file_update.h"
#include "cmr_types.h"


struct blk_info {
	enum ISP_BLK_ID blk_id;
	cmr_u32 data_size;
};

/************************** Project adapt data START ****************************************************/

#ifdef CONFIG_ISP_2_5  // ---- for SharkL3

static struct blk_info blocks_array[] = {
	/* DCAM blocks */
	{ ISP_BLK_BLC, sizeof(struct sensor_blc_param) },
	{ ISP_BLK_RGB_GAIN, 0 },  // ?? sharkl3 get bytes 12. why ?
	{ ISP_BLK_RGB_AEM, sizeof(struct sensor_rgb_aem_param) },
	{ ISP_BLK_2D_LSC, 0 }, /* todo: should be parsed in lsc block init() */
	{ ISP_BLK_AWB_NEW, 0 },
	{ DCAM_BLK_RGB_AFM, 0 }, /* NR block */
	{ DCAM_BLK_BPC, 0 }, /* NR block */
	{ ISP_BLK_RGB_DITHER, 0 }, /* NR block */
	{ ISP_BLK_GRGB, 0 }, /* NR block */

	/*  ISP blocks */
	{ ISP_BLK_HSV, 0 }, /* size parsed in hsv block init() */
	{ ISP_BLK_HSV_NEW, 0 }, /* size parsed in hsv block init() */
	{ ISP_BLK_BRIGHT, sizeof(struct sensor_bright_param) },
	{ ISP_BLK_CONTRAST, sizeof(struct sensor_contrast_param) },
	{ ISP_BLK_SATURATION, sizeof(struct sensor_saturation_param) },
	{ ISP_BLK_HUE, sizeof(struct sensor_hue_param) },
	{ ISP_BLK_CCE, sizeof(struct sensor_cce_param) },
	{ ISP_BLK_CMC10, sizeof(struct sensor_cmc10_param) },
	{ ISP_BLK_RGB_GAMC, sizeof(struct sensor_frgb_gammac_param) },
	{ ISP_BLK_HIST2, 0 },
	{ ISP_BLK_IIRCNR_YRANDOM, sizeof(struct sensor_iircnr_yrandom_param) },
	{ ISP_BLK_POSTERIZE, sizeof(struct sensor_posterize_param) },
	{ ISP_BLK_Y_GAMMC, sizeof(struct sensor_y_gamma_param) },
	{ ISP_BLK_HIST, 0 },
	{ ISP_BLK_ANTI_FLICKER, 0 },

	{ DCAM_BLK_3DNR_PRE, 0 }, /* NR block */
	{ DCAM_BLK_3DNR_CAP, 0 }, /* NR block */
	{ DCAM_BLK_NLM, 0 }, /* NR block */
	{ ISP_BLK_UVDIV, 0 }, /* NR block */
	{ ISP_BLK_CFA, 0 }, /* NR block */
	{ ISP_BLK_YUV_PRECDN, 0 }, /* NR block */
	{ ISP_BLK_YNR, 0 }, /* NR block */
	{ ISP_BLK_EDGE, 0 }, /* NR block */
	{ ISP_BLK_UV_CDN, 0 }, /* NR block */
	{ ISP_BLK_UV_POSTCDN, 0 }, /* NR block */
	{ ISP_BLK_IIRCNR_IIR, 0 }, /* NR block */
	{ ISP_BLK_YUV_NOISEFILTER, 0 }, /* NR block */

	/* software algo blocks */
	{ ISP_BLK_CNR2, 0 }, /* NR block */
	{ ISP_BLK_YNRS, 0 }, /* NR block */
	{ ISP_BLK_AE_NEW, 0 },
	{ ISP_BLK_ALSC, 0 },
	{ ISP_BLK_AF_NEW, 0 },
	{ ISP_BLK_SMART, 0 },
	{ ISP_BLK_AFT, 0 },
	{ ISP_BLK_PDAF_TUNE, 0 },
	{ ISP_BLK_DUAL_FLASH, 0 },
	{ ISP_BLK_AE_SYNC, 0 },
	{ ISP_BLK_AE_ADAPT_PARAM, 0 },
	{ ISP_BLK_4IN1_PARAM, 0 },
	{ ISP_BLK_TOF_TUNE, 0 },
	{ ISP_BLK_ATM_TUNE, 0 },
};

struct isp_pm_nrblk_info nr_blocks_info [ISP_BLK_NR_MAX] = {
	{DCAM_BLK_RGB_AFM,		ISP_BLK_RGB_AFM_T, sizeof(struct sensor_rgb_afm_level) },
	{DCAM_BLK_BPC,			ISP_BLK_BPC_T, sizeof(struct sensor_bpc_level) },
	{ISP_BLK_RGB_DITHER,	ISP_BLK_RGB_DITHER_T,  sizeof(struct sensor_rgb_dither_level) },
	{ISP_BLK_GRGB,			ISP_BLK_GRGB_T,  sizeof(struct sensor_grgb_level) },

	{ DCAM_BLK_3DNR_PRE,	ISP_BLK_3DNR_PRE_T, sizeof(struct sensor_3dnr_level) },
	{ DCAM_BLK_3DNR_CAP,	ISP_BLK_3DNR_CAP_T, sizeof(struct sensor_3dnr_level) },
	{ ISP_BLK_UVDIV,			ISP_BLK_UVDIV_T, sizeof(struct sensor_cce_uvdiv_level) },
	{ ISP_BLK_CFA,			ISP_BLK_CFA_T,  sizeof(struct sensor_cfa_param_level) },
	{ ISP_BLK_YUV_PRECDN,	ISP_BLK_YUV_PRECDN_T, sizeof(struct sensor_yuv_precdn_level) },
	{ ISP_BLK_YNR,			ISP_BLK_YNR_T, sizeof(struct sensor_ynr_level) },
	{ ISP_BLK_EDGE,			ISP_BLK_EDGE_T, sizeof(struct sensor_ee_level) },
	{ ISP_BLK_UV_CDN,		ISP_BLK_CDN_T, sizeof(struct sensor_uv_cdn_level) },
	{ ISP_BLK_UV_POSTCDN,	ISP_BLK_POSTCDN_T, sizeof(struct sensor_uv_postcdn_level) },
	{ ISP_BLK_IIRCNR_IIR,		ISP_BLK_IIRCNR_T, sizeof(struct sensor_iircnr_level) },
	{ ISP_BLK_YUV_NOISEFILTER,	ISP_BLK_YUV_NOISEFILTER_T, sizeof(struct sensor_yuv_noisefilter_level) },
	{ ISP_BLK_CNR2,			ISP_BLK_CNR2_T, sizeof(struct sensor_cnr_level) },
	{ ISP_BLK_YNRS,			ISP_BLK_YNRS_T, sizeof(struct sensor_ynrs_level) },

	{ DCAM_BLK_NLM,			ISP_BLK_NLM_T, sizeof(struct sensor_nlm_level) },
	{ DCAM_BLK_NLM,			ISP_BLK_VST_T, sizeof(struct sensor_vst_level) },
	{ DCAM_BLK_NLM,			ISP_BLK_IVST_T, sizeof(struct sensor_ivst_level) },
};

#elif defined CONFIG_ISP_2_6 /* for SharkL5 */

static struct blk_info blocks_array[] = {
	/* DCAM blocks */
	{ ISP_BLK_BLC, sizeof(struct sensor_blc_param) },
	{ ISP_BLK_RGB_GAIN, sizeof(struct sensor_rgb_gain_param) },
	{ ISP_BLK_RGB_AEM, sizeof(struct sensor_rgb_aem_param) },
	{ ISP_BLK_2D_LSC, 0 }, /* todo: should be parsed in lsc block init() */
	{ ISP_BLK_AWB_NEW, 0 },
	{ DCAM_BLK_RGB_DITHER, 0 }, /* NR block */
	{ DCAM_BLK_BPC_V1, 0 }, /* NR block */
	{ DCAM_BLK_PPE, 0 }, /* NR block */
	{ DCAM_BLK_RGB_AFM_V1, 0 }, /* NR block */

	/*  ISP blocks */
	{ ISP_BLK_HSV, 0 }, /* parsed in hsv block init() */
	{ ISP_BLK_BCHS, sizeof(struct sensor_bchs_level) },
	{ ISP_BLK_CCE, sizeof(struct sensor_cce_param) },
	{ ISP_BLK_CMC10, sizeof(struct sensor_cmc10_param) },
	{ ISP_BLK_RGB_GAMC, sizeof(struct sensor_frgb_gammac_param) },
	{ ISP_BLK_HIST2, 0 }, // todo: should be sizeof(struct sensor_hists2_param)
	{ ISP_BLK_IIRCNR_YRANDOM, sizeof(struct sensor_iircnr_yrandom_param) },
	{ ISP_BLK_POSTERIZE, sizeof(struct sensor_posterize_param) },
	{ ISP_BLK_Y_GAMMC, sizeof(struct sensor_y_gamma_param) },
	{ ISP_BLK_3DNR, 0 }, /* NR block */
	{ ISP_BLK_CFA_V1, 0 }, /* NR block */
	{ ISP_BLK_EE_V1, 0 }, /* NR block */
	{ ISP_BLK_GRGB_V1, 0 }, /* NR block */
	{ ISP_BLK_IIRCNR_IIR_V1, 0 }, /* NR block */
	{ ISP_BLK_LTM, 0 }, /* NR block */
	{ ISP_BLK_NLM_V1, 0 }, /* NR block */
	{ ISP_BLK_IMBALANCE, 0 }, /* NR block */
	{ ISP_BLK_UVDIV_V1, 0 }, /* NR block */
	{ ISP_BLK_YNR_V1, 0 }, /* NR block */
	{ ISP_BLK_YUV_PRECDN_V1, 0 }, /* NR block */
	{ ISP_BLK_UV_CDN_V1, 0 }, /* NR block */
	{ ISP_BLK_UV_POSTCDN_V1, 0 }, /* NR block */
	{ ISP_BLK_YUV_NOISEFILTER_V1, 0 }, /* NR block */

	/* software algo blocks */
	{ ISP_BLK_CNR2_V1, 0 }, /* NR block */
	{ ISP_BLK_SW3DNR, 0 }, /* NR block */
	{ ISP_BLK_AE_NEW, 0 },
	{ ISP_BLK_ALSC, 0 },
	{ ISP_BLK_AF_NEW, 0 },
	{ ISP_BLK_SMART, 0 },
	{ ISP_BLK_AFT, 0 },
	{ ISP_BLK_PDAF_TUNE, 0 },
	{ ISP_BLK_DUAL_FLASH, 0 },
	{ ISP_BLK_AE_SYNC, 0 },
	{ ISP_BLK_AE_ADAPT_PARAM, 0 },
	{ ISP_BLK_4IN1_PARAM, 0 },
	{ ISP_BLK_TOF_TUNE, 0 },
	{ ISP_BLK_ATM_TUNE, 0 },
};

struct isp_pm_nrblk_info nr_blocks_info [ISP_BLK_NR_MAX] = {
	{ DCAM_BLK_RGB_AFM_V1,		ISP_BLK_RGB_AFM_T, sizeof(struct sensor_rgb_afm_level) },
	{ DCAM_BLK_BPC_V1,			ISP_BLK_BPC_T, sizeof(struct sensor_bpc_level) },
	{ DCAM_BLK_RGB_DITHER,		ISP_BLK_RGB_DITHER_T,  sizeof(struct sensor_rgb_dither_level) },
	{ DCAM_BLK_PPE,				ISP_BLK_GRGB_T,  sizeof(struct sensor_ppe_level) },
	{ ISP_BLK_GRGB_V1,			ISP_BLK_GRGB_T, sizeof(struct sensor_grgb_level) },
	{ ISP_BLK_3DNR,				ISP_BLK_3DNR_T, sizeof(struct sensor_3dnr_level) },
	{ ISP_BLK_UVDIV_V1,			ISP_BLK_UVDIV_T, sizeof(struct sensor_cce_uvdiv_level) },
	{ ISP_BLK_CFA_V1,			ISP_BLK_CFA_T,  sizeof(struct sensor_cfa_param_level) },
	{ ISP_BLK_YUV_PRECDN_V1,	ISP_BLK_YUV_PRECDN_T, sizeof(struct sensor_yuv_precdn_level) },
	{ ISP_BLK_YNR_V1,			ISP_BLK_YNR_T, sizeof(struct sensor_ynr_level) },
	{ ISP_BLK_EE_V1,				ISP_BLK_EDGE_T, sizeof(struct sensor_ee_level) },
	{ ISP_BLK_UV_CDN_V1,		ISP_BLK_CDN_T, sizeof(struct sensor_uv_cdn_level) },
	{ ISP_BLK_UV_POSTCDN_V1,	ISP_BLK_POSTCDN_T, sizeof(struct sensor_uv_postcdn_level) },
	{ ISP_BLK_IIRCNR_IIR_V1,		ISP_BLK_IIRCNR_T, sizeof(struct sensor_iircnr_level) },
	{ ISP_BLK_LTM,				ISP_BLK_LTM_T, sizeof(struct sensor_ltm_level) },
	{ ISP_BLK_IMBALANCE,			ISP_BLK_IMBALANCEE_T, sizeof(struct sensor_nlm_imbalance_level) },
	{ ISP_BLK_CNR2_V1,			ISP_BLK_CNR2_T, sizeof(struct sensor_cnr_level) },
	{ ISP_BLK_SW3DNR,			ISP_BLK_SW3DNR_T, sizeof(struct sensor_sw3dnr_level) },
	{ ISP_BLK_YUV_NOISEFILTER_V1,	ISP_BLK_YUV_NOISEFILTER_T, sizeof(struct sensor_yuv_noisefilter_level) },
	{ ISP_BLK_NLM_V1,			ISP_BLK_NLM_T, sizeof(struct sensor_nlm_level) },
	{ ISP_BLK_NLM_V1,			ISP_BLK_VST_T, sizeof(struct sensor_vst_level) },
	{ ISP_BLK_NLM_V1,			ISP_BLK_IVST_T, sizeof(struct sensor_ivst_level) },
};

#elif defined CONFIG_ISP_2_7 /* for SharkL5Pro */

static struct blk_info blocks_array[] = {
	/* DCAM blocks */
	{ ISP_BLK_BLC, sizeof(struct sensor_blc_param) },
	{ ISP_BLK_RGB_GAIN, sizeof(struct sensor_rgb_gain_param) },
	{ ISP_BLK_RGB_AEM, sizeof(struct sensor_rgb_aem_param) },
	{ ISP_BLK_2D_LSC, 0 }, /* todo: should be parsed in lsc block init() */
	{ ISP_BLK_AWB_NEW, 0 },
	{ DCAM_BLK_RGB_DITHER, 0 }, /* NR block */
	{ DCAM_BLK_BPC_V1, 0 }, /* NR block */
	{ DCAM_BLK_PPE, 0 }, /* NR block */
	{ DCAM_BLK_RGB_AFM_V1, 0 }, /* NR block */

	/*  ISP blocks */
	{ ISP_BLK_HSV_NEW2, 0 }, /* parsed in hsv block init() */
	{ ISP_BLK_BCHS, sizeof(struct sensor_bchs_level) },
	{ ISP_BLK_CCE, sizeof(struct sensor_cce_param) },
	{ ISP_BLK_CMC10, sizeof(struct sensor_cmc10_param) },
	{ ISP_BLK_RGB_GAMC, sizeof(struct sensor_frgb_gammac_param) },
	{ ISP_BLK_HIST2, 0 }, // todo: should be sizeof(struct sensor_hists2_param)
	{ ISP_BLK_IIRCNR_YRANDOM, sizeof(struct sensor_iircnr_yrandom_param) },
	{ ISP_BLK_POSTERIZE, sizeof(struct sensor_posterize_param) },
	{ ISP_BLK_Y_GAMMC, sizeof(struct sensor_y_gamma_param) },
	{ ISP_BLK_3DNR, 0 }, /* NR block */
	{ ISP_BLK_CFA_V1, 0 }, /* NR block */
	{ ISP_BLK_EE_V1, 0 }, /* NR block */
	{ ISP_BLK_GRGB_V1, 0 }, /* NR block */
	{ ISP_BLK_IIRCNR_IIR_V1, 0 }, /* NR block */
	{ ISP_BLK_NLM_V2, 0 }, /* NR block */
	{ ISP_BLK_IMBALANCE_V1, 0 }, /* NR block */
	{ ISP_BLK_UVDIV_V1, 0 }, /* NR block */
	{ ISP_BLK_YNR_V1, 0 }, /* NR block */
	{ ISP_BLK_YUV_PRECDN_V1, 0 }, /* NR block */
	{ ISP_BLK_UV_CDN_V1, 0 }, /* NR block */
	{ ISP_BLK_UV_POSTCDN_V1, 0 }, /* NR block */
	{ ISP_BLK_YUV_NOISEFILTER_V1, 0 }, /* NR block */

	/* software algo blocks */
	{ ISP_BLK_CNR2_V1, 0 }, /* NR block */
	{ ISP_BLK_SW3DNR, 0 }, /* NR block */
	{ ISP_BLK_YNRS, 0 }, /* NR block */
	{ ISP_BLK_AE_NEW, 0 },
	{ ISP_BLK_ALSC, 0 },
	{ ISP_BLK_AF_NEW, 0 },
	{ ISP_BLK_SMART, 0 },
	{ ISP_BLK_AFT, 0 },
	{ ISP_BLK_PDAF_TUNE, 0 },
	{ ISP_BLK_DUAL_FLASH, 0 },
	{ ISP_BLK_AE_SYNC, 0 },
	{ ISP_BLK_AE_ADAPT_PARAM, 0 },
	{ ISP_BLK_4IN1_PARAM, 0 },
	{ ISP_BLK_TOF_TUNE, 0 },
	{ ISP_BLK_ATM_TUNE, 0 },
};

struct isp_pm_nrblk_info nr_blocks_info [ISP_BLK_NR_MAX] = {
	{ DCAM_BLK_RGB_AFM_V1,		ISP_BLK_RGB_AFM_T, sizeof(struct sensor_rgb_afm_level) },
	{ DCAM_BLK_BPC_V1,			ISP_BLK_BPC_T, sizeof(struct sensor_bpc_level) },
	{ DCAM_BLK_RGB_DITHER,		ISP_BLK_RGB_DITHER_T,  sizeof(struct sensor_rgb_dither_level) },
	{ DCAM_BLK_PPE,				ISP_BLK_GRGB_T,  sizeof(struct sensor_ppe_level) },
	{ ISP_BLK_GRGB_V1,			ISP_BLK_GRGB_T, sizeof(struct sensor_grgb_level) },
	{ ISP_BLK_3DNR,				ISP_BLK_3DNR_T, sizeof(struct sensor_3dnr_level) },
	{ ISP_BLK_UVDIV_V1,			ISP_BLK_UVDIV_T, sizeof(struct sensor_cce_uvdiv_level) },
	{ ISP_BLK_CFA_V1,			ISP_BLK_CFA_T,  sizeof(struct sensor_cfa_param_level) },
	{ ISP_BLK_YUV_PRECDN_V1,	ISP_BLK_YUV_PRECDN_T, sizeof(struct sensor_yuv_precdn_level) },
	{ ISP_BLK_YNR_V1,			ISP_BLK_YNR_T, sizeof(struct sensor_ynr_level) },
	{ ISP_BLK_EE_V1,				ISP_BLK_EDGE_T, sizeof(struct sensor_ee_level) },
	{ ISP_BLK_UV_CDN_V1,		ISP_BLK_CDN_T, sizeof(struct sensor_uv_cdn_level) },
	{ ISP_BLK_UV_POSTCDN_V1,	ISP_BLK_POSTCDN_T, sizeof(struct sensor_uv_postcdn_level) },
	{ ISP_BLK_IIRCNR_IIR_V1,		ISP_BLK_IIRCNR_T, sizeof(struct sensor_iircnr_level) },
	{ ISP_BLK_IMBALANCE_V1,			ISP_BLK_IMBALANCEE_T, sizeof(struct sensor_nlm_imbalance_level) },
	{ ISP_BLK_CNR2_V1,			ISP_BLK_CNR2_T, sizeof(struct sensor_cnr_level) },
	{ ISP_BLK_SW3DNR,			ISP_BLK_SW3DNR_T, sizeof(struct sensor_sw3dnr_level) },
	{ ISP_BLK_YUV_NOISEFILTER_V1,	ISP_BLK_YUV_NOISEFILTER_T, sizeof(struct sensor_yuv_noisefilter_level) },
	{ ISP_BLK_NLM_V2,			ISP_BLK_NLM_T, sizeof(struct sensor_nlm_level) },
	{ ISP_BLK_NLM_V2,			ISP_BLK_VST_T, sizeof(struct sensor_vst_level) },
	{ ISP_BLK_NLM_V2,			ISP_BLK_IVST_T, sizeof(struct sensor_ivst_level) },
	{ ISP_BLK_YNRS,				ISP_BLK_YNRS_T, sizeof(struct sensor_ynrs_level) },
};
#endif
/************************ Project adapt data END *******************************************/




static cmr_u32 search_modes[3][ISP_TUNE_MODE_MAX] = {
	{
		/* for PREVIEW */
		ISP_MODE_ID_PRV_0,
		ISP_MODE_ID_PRV_1,
		ISP_MODE_ID_PRV_2,
		ISP_MODE_ID_PRV_3,
		ISP_MODE_ID_VIDEO_0,
		ISP_MODE_ID_VIDEO_1,
		ISP_MODE_ID_VIDEO_2,
		ISP_MODE_ID_VIDEO_3,
		ISP_MODE_ID_CAP_0,
		ISP_MODE_ID_CAP_1,
		ISP_MODE_ID_CAP_2,
		ISP_MODE_ID_CAP_3,
		ISP_MODE_ID_MAX,
	},
	{
		/* for capture */
		ISP_MODE_ID_CAP_0,
		ISP_MODE_ID_CAP_1,
		ISP_MODE_ID_CAP_2,
		ISP_MODE_ID_CAP_3,
		ISP_MODE_ID_PRV_0,
		ISP_MODE_ID_PRV_1,
		ISP_MODE_ID_PRV_2,
		ISP_MODE_ID_PRV_3,
		ISP_MODE_ID_VIDEO_0,
		ISP_MODE_ID_VIDEO_1,
		ISP_MODE_ID_VIDEO_2,
		ISP_MODE_ID_VIDEO_3,
		ISP_MODE_ID_MAX,
	},
	{
		/* for video */
		ISP_MODE_ID_VIDEO_0,
		ISP_MODE_ID_VIDEO_1,
		ISP_MODE_ID_VIDEO_2,
		ISP_MODE_ID_VIDEO_3,
		ISP_MODE_ID_PRV_0,
		ISP_MODE_ID_PRV_1,
		ISP_MODE_ID_PRV_2,
		ISP_MODE_ID_PRV_3,
		ISP_MODE_ID_CAP_0,
		ISP_MODE_ID_CAP_1,
		ISP_MODE_ID_CAP_2,
		ISP_MODE_ID_CAP_3,
		ISP_MODE_ID_MAX,
	},
};

struct sensor_find_param_list params_list_ov12a[] =
{
    //custom	Scene	Mode	Size	   Block	 ID
	/*ov12a:  binning prev1 */
    {0x0000, 0x0000, 0x0000, 0x0800, 0x0600, 0x4002, 0x0002, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0800, 0x0600, 0x400a, 0x0002, 0x0000},
	/*ov12a:  binning cap1 */
    {0x0000, 0x0000, 0x0001, 0x0800, 0x0600, 0x4002, 0x0006, 0x0000},
    {0x0000, 0x0000, 0x0001, 0x0800, 0x0600, 0x400a, 0x0006, 0x0000},
	/*ov12a:  binning video1 */
    {0x0000, 0x0000, 0x0002, 0x0800, 0x0600, 0x4002, 0x000a, 0x0000},
    {0x0000, 0x0000, 0x0002, 0x0800, 0x0600, 0x400a, 0x000a, 0x0000},
	/*ov12a:  slw video2 (1280x720) */
    {0x0000, 0x0000, 0x0002, 0x0500, 0x02D0, 0x4002, 0x000b, 0x0000},
    {0x0000, 0x0000, 0x0002, 0x0500, 0x02D0, 0x400a, 0x000b, 0x0000},
};

struct sensor_find_param_list params_list_ov8856[] =
{
    //custom	Scene	Mode	Size	   Block	 ID

	/*ov8856:  binning prev1 */
    {0x0000, 0x0000, 0x0000, 0x0660, 0x04c8, 0x4002, 0x0002, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0660, 0x04c8, 0x400a, 0x0002, 0x0000},
	/*ov8856:  binning cap1 */
    {0x0000, 0x0000, 0x0001, 0x0660, 0x04c8, 0x4002, 0x0006, 0x0000},
    {0x0000, 0x0000, 0x0001, 0x0660, 0x04c8, 0x400a, 0x0006, 0x0000},
	/*ov8856:  binning video1 */
    {0x0000, 0x0000, 0x0002, 0x0660, 0x04c8, 0x4002, 0x000a, 0x0000},
    {0x0000, 0x0000, 0x0002, 0x0660, 0x04c8, 0x400a, 0x000a, 0x0000},
};

struct sensor_find_param_list params_list_ov5675[] =
{
    //custom	Scene	Mode	Size	   Block	 ID

	/*ov5675:  binning prev1 */
    {0x0000, 0x0000, 0x0000, 0x0510, 0x03cc, 0x4002, 0x0002, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0510, 0x03cc, 0x400a, 0x0002, 0x0000},
	/*ov5675:  binning cap1 */
    {0x0000, 0x0000, 0x0001, 0x0510, 0x03cc, 0x4002, 0x0006, 0x0000},
    {0x0000, 0x0000, 0x0001, 0x0510, 0x03cc, 0x400a, 0x0006, 0x0000},
	/*ov5675:  binning video1 */
    {0x0000, 0x0000, 0x0002, 0x0510, 0x03cc, 0x4002, 0x000a, 0x0000},
    {0x0000, 0x0000, 0x0002, 0x0510, 0x03cc, 0x400a, 0x000a, 0x0000},
};

/* todo: delete it later */
/* workaround for 4in1. It should be passed from source tuning data */
struct sensor_find_param_list params_list_ov16885[] =
{
    //custom	Scene	Mode	Size	   Block	 ID
	/*ov16885:  4in1 prev0 */
    {0x0001, 0x0000, 0x0000, 0x0920, 0x06d8, 0x4002, 0x0001, 0x0000},
    {0x0001, 0x0000, 0x0000, 0x0920, 0x06d8, 0x400a, 0x0001, 0x0000},
    {0x0001, 0x0000, 0x0000, 0x0920, 0x06d8, 0x4013, 0x0001, 0x0000},
    {0x0001, 0x0000, 0x0000, 0x0920, 0x06d8, 0x5009, 0x0001, 0x0000},

	/*ov16885:  binning prev1 */
    {0x0000, 0x0000, 0x0000, 0x0920, 0x06d8, 0x4002, 0x0002, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0920, 0x06d8, 0x400a, 0x0002, 0x0000},

	/*ov16885:  4in1 cap0(full size remosic) */
    {0x0000, 0x0000, 0x0001, 0x1240, 0x0db0, 0x5009, 0x0005, 0x0000},

	/*ov16885:  4in1 binning cap1() */
    {0x0001, 0x0000, 0x0001, 0x0920, 0x06d8, 0x4002, 0x0006, 0x0000},
    {0x0001, 0x0000, 0x0001, 0x0920, 0x06d8, 0x400a, 0x0006, 0x0000},

	/*ov16885:  binning video1 */
    {0x0000, 0x0000, 0x0002, 0x0920, 0x06d8, 0x4002, 0x000a, 0x0000},
    {0x0000, 0x0000, 0x0002, 0x0920, 0x06d8, 0x400a, 0x000a, 0x0000},

	/*ov16885:  slw video2 (1280x720) */
    {0x0000, 0x0000, 0x0002, 0x0500, 0x02D0, 0x4002, 0x000b, 0x0000},
    {0x0000, 0x0000, 0x0002, 0x0500, 0x02D0, 0x400a, 0x000b, 0x0000},
};

struct isp_pm_context {
	cmr_u32 magic_flag;
	cmr_u32 param_source;
	pthread_mutex_t pm_mutex;
	cmr_u32 is_4in1_sensor;
	cmr_u32 cam_4in1_mode;
	cmr_u32 noramosaic_4in1;
	cmr_u32 lowlight_flag;
	cmr_u32 mode_id;
	cmr_u32 prv_mode_id;
	cmr_u32 cap_mode_id;
	cmr_u32 param_search_list_size;
	struct sensor_find_param_list *param_search_list;
	struct isp_context cxt_array[PARAM_SET_MAX];
	struct isp_pm_blocks_param  blocks_param[PARAM_SET_MAX];
	struct isp_pm_param_data *getting_data_ptr[PARAM_SET_MAX];
	struct isp_pm_param_data single_block_data[ISP_TUNE_BLOCK_MAX];
	struct isp_pm_mode_param *tune_mode_array[ISP_TUNE_MODE_MAX];
};

static cmr_s32 isp_pm_check_handle(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (pm_cxt_ptr == PNULL) {
		ISP_LOGE("fail to get valid pm_cxt_ptr");
		rtn = ISP_ERROR;
		return rtn;
	}

	if (pm_cxt_ptr->magic_flag != ISP_PM_MAGIC_FLAG) {
		ISP_LOGE("fail to get valid param : magic_flag = 0x%x", pm_cxt_ptr->magic_flag);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_u32 check_blk_id_valid(cmr_u32 blk_id, cmr_u32 data_size)
{
	cmr_u32 i, blk_num;

	blk_num = sizeof(blocks_array) / sizeof(blocks_array[0]);
	for (i = 0; i < blk_num; i++) {
		if (blocks_array[i].blk_id != blk_id)
			continue;
		if (blocks_array[i].data_size != 0 && blocks_array[i].data_size != data_size) {
			ISP_LOGE("blk 0x%04x, data size %d != %d\n", blk_id, data_size, blocks_array[i].data_size);
			return 0;
		}
		return 1;
	}
	return 0;
}

static cmr_u32 isp_pm_check_skip_blk(cmr_u32 id)
{

	switch (id) {
	/* NR for SharkL5 */
	case DCAM_BLK_RGB_DITHER:
	case DCAM_BLK_BPC_V1:
	case DCAM_BLK_PPE:
	case DCAM_BLK_RGB_AFM_V1:
	case ISP_BLK_3DNR:
	case ISP_BLK_CFA_V1:
	case ISP_BLK_EE_V1:
	case ISP_BLK_GRGB_V1:
	case ISP_BLK_IIRCNR_IIR_V1:
	case ISP_BLK_LTM:
	case ISP_BLK_NLM_V1:
	case ISP_BLK_IMBALANCE:
	case ISP_BLK_UVDIV_V1:
	case ISP_BLK_YNR_V1:
	case ISP_BLK_YUV_PRECDN_V1:
	case ISP_BLK_UV_CDN_V1:
	case ISP_BLK_UV_POSTCDN_V1:
	case ISP_BLK_YUV_NOISEFILTER_V1:
	case ISP_BLK_CNR2_V1:
	case ISP_BLK_SW3DNR:
		return 1;
	/* NR for SharkL3 */
	case DCAM_BLK_RGB_AFM:
	case DCAM_BLK_BPC:
	case ISP_BLK_RGB_DITHER:
	case ISP_BLK_GRGB:
	case DCAM_BLK_NLM:
	case ISP_BLK_UVDIV:
	case ISP_BLK_CFA:
	case DCAM_BLK_3DNR_PRE:
	case DCAM_BLK_3DNR_CAP:
	case ISP_BLK_YUV_PRECDN:
	case ISP_BLK_YNR:
	case ISP_BLK_EDGE:
	case ISP_BLK_UV_CDN:
	case ISP_BLK_UV_POSTCDN:
	case ISP_BLK_IIRCNR_IIR:
	case ISP_BLK_YUV_NOISEFILTER:
	case ISP_BLK_CNR2:
	case ISP_BLK_YNRS:
		return 1;
	case ISP_BLK_IMBALANCE_V1:
	case ISP_BLK_NLM_V2:
	case ISP_BLK_PPE_V1:
	case ISP_BLK_YUV_LTM:
	case ISP_BLK_RGB_LTM:
	case ISP_BLK_RAW_GTM:
		return 1;
	default:
		break;
	}
	return 0;
}

static struct isp_pm_block_header *isp_pm_get_block_header(
	struct isp_pm_blocks_param *blk_param_ptr, cmr_u32 id, cmr_s32 *index)
{
	cmr_u32 i = 0, blk_num = 0;
	struct isp_pm_block_header *header_ptr = PNULL;
	struct isp_pm_block_header *blk_header = PNULL;

	blk_header = blk_param_ptr->header;
	blk_num = blk_param_ptr->block_num;
	*index = ISP_TUNE_BLOCK_MAX;

	for (i = 0; i < blk_num; i++) {
		if (id == blk_header[i].block_id) {
			header_ptr = (struct isp_pm_block_header *)&blk_header[i];
			*index = i;
			return header_ptr;
		}
	}

	return header_ptr;
}

static cmr_s32 isp_pm_context_init(cmr_handle handle, cmr_u32 set_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	cmr_u32 update_flag = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_blocks_param *blk_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	ISP_LOGV("enter.\n");
	isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
	blk_param_ptr = &pm_cxt_ptr->blocks_param[set_id];
	blk_header_array = blk_param_ptr->header;
	blk_num = blk_param_ptr->block_num;
	ISP_LOGD("%p %p  num %d\n", isp_cxt_ptr, blk_param_ptr,  blk_num);

	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if (pm_cxt_ptr->param_source == ISP_PARAM_FROM_TOOL) {
			/* skip NR blocks from tool update parameters */
			update_flag = !isp_pm_check_skip_blk(id);
			if (update_flag == 0) {
				ISP_LOGV("skip NR block 0x%x from tool update.", id);
				continue;
			}
		}
		ISP_LOGV("i %d, blk 0x%04x, cfgptr %p,  source_flag %x\n", i, id, blk_cfg_ptr, blk_header_ptr->source_flag);
		if (blk_cfg_ptr != PNULL && blk_cfg_ptr->ops) {
			ops = blk_cfg_ptr->ops;
			if (ops->init && blk_header_ptr->size > 0) {
				isp_cxt_start_addr = (intptr_t) isp_cxt_ptr;
				offset = blk_cfg_ptr->offset;
				blk_ptr = (void *)(isp_cxt_start_addr + offset);
				param_data_ptr = (void *)blk_header_ptr->absolute_addr;
				if (param_data_ptr == PNULL) {
					ISP_LOGE("fail to get valid param,  i:%d, block_id:0x%x, blk_addr:%p, param:%p",
						 i, id, blk_ptr, param_data_ptr);
					rtn = ISP_ERROR;
					return rtn;
				}
				if (blk_header_ptr->source_flag & ISP_PM_BLK_UPDATE) {
					blk_header_ptr->source_flag &= ~ISP_PM_BLK_UPDATE;
					rtn = ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &blk_param_ptr->resolution);
					if (rtn) {
						ISP_LOGW("init block 0x%04x rtn %d\n", id, rtn);
					}
				}
			}
		} else {
			ISP_LOGV("i = %d, id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p",
				i, id, blk_cfg_ptr, blk_header_ptr);
			blk_header_ptr->source_flag &= ~ISP_PM_BLK_UPDATE;
		}
	}

	return rtn;
}

static cmr_s32 isp_pm_context_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_blocks_param *blk_param_ptr = PNULL;

	ISP_LOGD("start.\n");
	for (j = 0; j < PARAM_SET_MAX; j++) {
		isp_cxt_ptr = &pm_cxt_ptr->cxt_array[j];
		blk_param_ptr = &pm_cxt_ptr->blocks_param[j];
		blk_header_array = blk_param_ptr->header;
		blk_num = blk_param_ptr->block_num;
		for (i = 0; i < blk_num; i++) {
			id = blk_header_array[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			if (PNULL != blk_cfg_ptr && blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->deinit) {
					isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					ops->deinit(blk_ptr);
				}
				ISP_LOGV("deinit block 0x%04x, set %d\n", id, j);
			} else {
				ISP_LOGV("i = %d, id = 0x%x, blk_cfg_ptr = %p", i, id, blk_cfg_ptr);
			}
		}
	}
	ISP_LOGD("done.\n");
	return rtn;
}

static cmr_s32 check_block_skip(struct isp_pm_context *pm_cxt_ptr,
		cmr_u32 set_id, cmr_u32 blk_id)
{
	struct isp_context *isp_cxt_prv = PNULL;

	if (set_id == PARAM_SET0)
		return 0;

	isp_cxt_prv = &pm_cxt_ptr->cxt_array[set_id];
	if (isp_cxt_prv->is_validate) {
		if (pm_cxt_ptr->cam_4in1_mode || pm_cxt_ptr->noramosaic_4in1)
			return 0;
		if (IS_DCAM_BLOCK(blk_id))
			return 1;
	}
	return 0;
}

static cmr_s32 isp_pm_set_block_param(struct isp_pm_context *pm_cxt_ptr,
	struct isp_pm_param_data *param_data_ptr, cmr_u32 set_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 cmd = 0, id = 0, offset = 0;
	cmr_u32 share_skip;
	cmr_s32 index = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_blocks_param *blk_param_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
	blk_param_ptr = &pm_cxt_ptr->blocks_param[set_id];

	if (isp_cxt_ptr->is_validate) {
		cmd = param_data_ptr->cmd;
		id = param_data_ptr->id;
		share_skip = check_block_skip(pm_cxt_ptr, set_id, id);
		if (share_skip)
			return rtn;

		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = isp_pm_get_block_header(blk_param_ptr, id, &index);
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr) && blk_cfg_ptr->ops) {
			ops = blk_cfg_ptr->ops;
			if (ops->set) {
				isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
				offset = blk_cfg_ptr->offset;
				blk_ptr = (void *)(isp_cxt_start_addr + offset);
				ops->set(blk_ptr, cmd, param_data_ptr->data_ptr, blk_header_ptr);
			}
		} else {
			ISP_LOGV("id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p", id, blk_cfg_ptr, blk_header_ptr);
		}
	}

	return rtn;
}

static cmr_s32 isp_pm_get_mode_block_param(struct isp_pm_context *pm_cxt_ptr,
	enum isp_pm_cmd cmd, struct isp_pm_param_data *param_data_ptr,
	cmr_u32 *param_counts, cmr_u32 block_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0, counts = 0;
	cmr_u32 id = 0, blk_num = 0;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
		mode_param_ptr = pm_cxt_ptr->tune_mode_array[j];
		if (mode_param_ptr == PNULL)
			continue;

		blk_header_array = mode_param_ptr->header;
		blk_num = mode_param_ptr->block_num;
		for (i = 0; i < blk_num; i++) {
			id = blk_header_array[i].block_id;
			if (block_id == id) {
				break;
			}
		}

		if (i < blk_num) {
			param_data_ptr->data_ptr = blk_header_array[i].absolute_addr;
			param_data_ptr->data_size = blk_header_array[i].size;
			param_data_ptr->user_data[0] = blk_header_array[i].bypass;
			param_data_ptr->cmd = cmd;
			param_data_ptr->mode_id = j;
			param_data_ptr->id = (j << 16) | block_id;
			param_data_ptr++;
			counts++;
		}
	}
	*param_counts = counts;

	return rtn;
}

static cmr_s32 isp_pm_get_single_block_param(struct isp_pm_context *pm_cxt_ptr,
	struct isp_pm_ioctl_input *blk_info_in, struct isp_pm_param_data *pm_param_data_ptr,
	cmr_u32 *param_counts, cmr_u32 *rtn_idx, cmr_u32 set_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 index = 0;
	cmr_u32 counts = 0;
	cmr_u32 cmd = 0, id = 0, offset = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_blocks_param *blk_param_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_param_data *block_param_data_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	//just support get only one block info
	if (blk_info_in->param_num > 1) {
		ISP_LOGE("fail to get valid param : param_num = %d", blk_info_in->param_num);
		rtn = ISP_ERROR;
		return rtn;
	}

	isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
	blk_param_ptr = &pm_cxt_ptr->blocks_param[set_id];

	if (isp_cxt_ptr->is_validate) {
		block_param_data_ptr = blk_info_in->param_data_ptr;
		cmd = block_param_data_ptr->cmd;
		id = block_param_data_ptr->id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = isp_pm_get_block_header(blk_param_ptr, id, &index);

		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr) && blk_cfg_ptr->ops) {
			ops = blk_cfg_ptr->ops;
			if (ops->get) {
				isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
				offset = blk_cfg_ptr->offset;
				blk_ptr = (void *)(isp_cxt_start_addr + offset);
				ops->get(blk_ptr, cmd, &pm_param_data_ptr[index], &blk_header_ptr->is_update);
				counts++;
			}
		} else {
			ISP_LOGV("id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p",
				id, blk_cfg_ptr, blk_header_ptr);
		}
		*rtn_idx = index;
		*param_counts = counts;
	} else {
		ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, blk_param_ptr = %p",
			isp_cxt_ptr, blk_param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_s32 isp_pm_get_setting_param(
	struct isp_pm_context *pm_cxt_ptr,
	struct isp_pm_param_data *param_data_ptr, cmr_u32 *param_counts,
	cmr_u32 all_setting_flag, cmr_u32 set_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, counts = 0;
	cmr_u32 is_update = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_blocks_param *blk_param_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
	blk_param_ptr = &pm_cxt_ptr->blocks_param[set_id];
	blk_header_array = blk_param_ptr->header;
	blk_num = blk_param_ptr->block_num;

	if (isp_cxt_ptr->is_validate) {
		for (i = 0; i < blk_num; i++) {
			id = blk_header_array[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			blk_header_ptr = &blk_header_array[i];
			is_update = 0;
			if ((ISP_ZERO != all_setting_flag) || (ISP_ZERO != blk_header_ptr->is_update))
				is_update = 1;
			if (is_update && blk_cfg_ptr && blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->get) {
					isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					param_data_ptr->id = id;
					ops->get(blk_ptr, ISP_PM_BLK_ISP_SETTING,
							param_data_ptr, &blk_header_ptr->is_update);
					param_data_ptr++;
					counts++;
				}
			}
		}
		*param_counts = counts;
	} else {
		ISP_LOGV("no valid param : %d, isp_cxt_ptr = %p, blk_param_ptr = %p",
			isp_cxt_ptr->is_validate, isp_cxt_ptr, blk_param_ptr);
	}

	return rtn;
}

static cmr_s32 isp_pm_get_all_blocks(cmr_handle handle,
	struct isp_pm_blocks_param *output,
	enum tuning_mode mode, enum tuning_scene_mode scene,
	enum tuning_custom define,  cmr_u32 img_w, cmr_u32 img_h,
	cmr_u32 update_always)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i, j, k, tail_idx, blk_id, blk_num, item_num;
	cmr_u32 param_mode_id, param_scene_id;
	cmr_u32 source_flag;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct sensor_find_param_list *item;
	struct isp_pm_mode_param *src_mode;
	struct isp_pm_block_header *dst_header, new_header;

	if ((update_always == 0) &&
		output->is_init && (output->mode == mode) &&
		(output->scene == scene) &&
		(output->cus_define == define) &&
		(output->resolution.w == img_w) &&
		(output->resolution.h == img_h)) {
		ISP_LOGD("total same, no need to change blocks.\n");
		return 0;
	}
	output->resolution.w = img_w;
	output->resolution.h = img_h;
	output->mode = mode;
	output->scene = scene;
	output->cus_define = define;
	if (pm_cxt_ptr->cam_4in1_mode && pm_cxt_ptr->lowlight_flag &&
                mode == WORKMODE_CAPTURE) {
                img_w /= 2;
                img_h /= 2;
                output->resolution.w = img_w;
                output->resolution.h = img_h;
        }

	tail_idx = output->block_num;
	blk_num = sizeof(blocks_array) / sizeof(blocks_array[0]);
	item_num = pm_cxt_ptr->param_search_list_size / sizeof(struct sensor_find_param_list);
	for (i = 0; i < blk_num; i++) {
		blk_id = blocks_array[i].blk_id;
		item = pm_cxt_ptr->param_search_list;
		param_mode_id = ISP_MODE_ID_COMMON;
		for (j = 0; j < item_num; j++, item++) {
			if ((item->custom == define) && (item->scene == scene) &&
				(item->mode == mode) && (item->size_w == img_w) &&
				(item->blk_id == blk_id)) {
				param_mode_id = item->param_mode_id;
				param_scene_id = item->param_scene_id;
				ISP_LOGV("blk 0x%04x,  j = %d, new_mode_id %d\n", blk_id, j, param_mode_id);
				break;
			}
		}

		src_mode = NULL;
		for (k = 0; k < ISP_TUNE_MODE_MAX; k++) {
			if (pm_cxt_ptr->tune_mode_array[k] == NULL)
				continue;
			if (pm_cxt_ptr->tune_mode_array[k]->mode_id == param_mode_id) {
				src_mode = pm_cxt_ptr->tune_mode_array[k];
				break;
			}
		}

		if (src_mode == NULL) {
			ISP_LOGE("blk 0x%04x is not found\n", blk_id);
			continue;
		}

		for (j = 0; j < src_mode->block_num; j++) {
			if (src_mode->header[j].block_id != blk_id)
				continue;
			new_header = src_mode->header[j];
			ISP_LOGD("get block 0x%04x from mode %d for %s, img size %d %d\n",
				blk_id, src_mode->mode_id,
				(output->mode == WORKMODE_PREVIEW) ? "PREV" :
				((output->mode == WORKMODE_VIDEO) ? "VIDEO" : "CAP"),
				img_w, img_h);
			break;
		}
		if (j >= src_mode->block_num) {
			ISP_LOGE("no block 0x%04x in mode %d", blk_id, src_mode->mode_id);
			continue;
		}

		dst_header = NULL;
		if (output->is_init) {
			for (j = 0; j < output->block_num; j++) {
				if (output->header[j].block_id != blk_id)
					continue;
				dst_header = &output->header[j];
				source_flag = dst_header->source_flag;
				*dst_header = new_header;
				dst_header->source_flag |= (source_flag & ISP_PM_BLK_UPDATE);
				if ((source_flag & (~ISP_PM_BLK_UPDATE)) != new_header.source_flag)
					dst_header->source_flag |= ISP_PM_BLK_UPDATE;
				if (update_always)
					dst_header->source_flag |= ISP_PM_BLK_UPDATE;
				ISP_LOGV("get exist block 0x%04x, j = %d,  mode %x => %x... %x\n", blk_id, j,
					source_flag, new_header.source_flag, dst_header->source_flag);
				break;
			}
		}
		if (dst_header == NULL) {
			dst_header = &output->header[tail_idx];
			*dst_header = new_header;
			dst_header->source_flag |= ISP_PM_BLK_UPDATE;
			tail_idx++;
			ISP_LOGV("new block 0x%04x, index %d\n", blk_id, tail_idx);
		}
	}

	output->block_num = tail_idx;
	output->is_init = 1;
	ISP_LOGD("mode %d  block num %d, total %d, img_size %d %d\n", output->mode,
		output->block_num, blk_num, img_w, img_h);

	return rtn;
}


/* compatible for those without pm searching list */
static cmr_s32 isp_pm_get_all_blocks_compatible(cmr_handle handle,
	struct isp_pm_blocks_param *output, cmr_u32 mode_id,
	enum tuning_mode mode, enum tuning_scene_mode scene,
	enum tuning_custom define,  cmr_u32 img_w, cmr_u32 img_h,
	cmr_u32 update_always)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i, j, k, tail_idx, blk_id, blk_num;
	cmr_u32 param_mode_id;
	cmr_u32 source_flag;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_mode_param *src_mode;
	struct isp_pm_block_header *dst_header, new_header;

	if ((update_always == 0) &&
		output->is_init && (output->mode == mode) &&
		(output->compatible_mode_id == mode_id) &&
		(output->scene == scene) &&
		(output->cus_define == define) &&
		(output->resolution.w == img_w) &&
		(output->resolution.h == img_h)) {
		ISP_LOGD("total same, no need to change blocks.\n");
		return 0;
	}
	output->resolution.w = img_w;
	output->resolution.h = img_h;
	output->mode = mode;
	output->scene = scene;
	output->cus_define = define;
	output->compatible_mode_id = mode_id;
	if (pm_cxt_ptr->cam_4in1_mode && pm_cxt_ptr->lowlight_flag &&
		mode == WORKMODE_CAPTURE) {
		img_w /= 2;
		img_h /= 2;
		output->resolution.w = img_w;
		output->resolution.h = img_h;
	}

	tail_idx = output->block_num;
	blk_num = sizeof(blocks_array) / sizeof(blocks_array[0]);
	for (i = 0; i < blk_num; i++) {
		blk_id = blocks_array[i].blk_id;
		param_mode_id = mode_id;

		if (mode == WORKMODE_CAPTURE) {
			/* skip 3DNR_PRE block for capture mode */
			if (blk_id == DCAM_BLK_3DNR_PRE)
				continue;
		} else {
			/* skip 3DNR_CAP block for non-capture mode */
			if (blk_id == DCAM_BLK_3DNR_CAP)
				continue;
		}
retry:
		src_mode = NULL;
		for (k = 0; k < ISP_TUNE_MODE_MAX; k++) {
			if (pm_cxt_ptr->tune_mode_array[k] == NULL)
				continue;
			if (pm_cxt_ptr->tune_mode_array[k]->mode_id == param_mode_id) {
				src_mode = pm_cxt_ptr->tune_mode_array[k];
				break;
			}
		}

		if (src_mode == NULL) {
			if (param_mode_id != ISP_MODE_ID_COMMON) {
				ISP_LOGE("should not be here. mode %d is NULL\n", param_mode_id);
				param_mode_id = ISP_MODE_ID_COMMON;
				goto retry;
			} else {
				ISP_LOGE("should not be here. Common is NULL\n");
				return ISP_ERROR;
			}
		}

		for (j = 0; j < src_mode->block_num; j++) {
			if (src_mode->header[j].block_id != blk_id)
				continue;
			new_header = src_mode->header[j];
			ISP_LOGD("get block 0x%04x from mode %d for %s, img size %d %d\n",
				blk_id, src_mode->mode_id,
				(output->mode == WORKMODE_PREVIEW) ? "PREV" :
				((output->mode == WORKMODE_VIDEO) ? "VIDEO" : "CAP"),
				img_w, img_h);
			break;
		}

		if (j >= src_mode->block_num) {
			ISP_LOGV("no block 0x%04x in mode %d", blk_id, src_mode->mode_id);
			/* try to get BLK in common */
			if (param_mode_id != ISP_MODE_ID_COMMON) {
				param_mode_id = ISP_MODE_ID_COMMON;
				goto retry;
			}
			/* This BLK is not in both mode_id & Common. Continue to next BLK*/
			continue;
		}

		dst_header = NULL;
		if (output->is_init) {
			for (j = 0; j < output->block_num; j++) {
				if (output->header[j].block_id != blk_id)
					continue;
				dst_header = &output->header[j];
				source_flag = dst_header->source_flag;
				*dst_header = new_header;
				dst_header->source_flag |= (source_flag & ISP_PM_BLK_UPDATE);
				if ((source_flag & (~ISP_PM_BLK_UPDATE)) != new_header.source_flag)
					dst_header->source_flag |= ISP_PM_BLK_UPDATE;
				if (update_always)
					dst_header->source_flag |= ISP_PM_BLK_UPDATE;
				ISP_LOGV("get exist block 0x%04x, j = %d,  mode %x => %x... %x\n", blk_id, j,
					source_flag, new_header.source_flag, dst_header->source_flag);
				break;
			}
		}
		if (dst_header == NULL) {
			dst_header = &output->header[tail_idx];
			*dst_header = new_header;
			dst_header->source_flag |= ISP_PM_BLK_UPDATE;
			tail_idx++;
			ISP_LOGV("new block 0x%04x, index %d\n", blk_id, tail_idx);
		}
	}

	output->block_num = tail_idx;
	output->is_init = 1;
	ISP_LOGD("mode %d  block num %d, total %d, img_size %d %d\n", output->mode,
		output->block_num, blk_num, img_w, img_h);

	return rtn;
}


static cmr_s32 isp_pm_set_param(cmr_handle handle, enum isp_pm_cmd cmd, void *param_ptr, void *out_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 update_always[PARAM_SET_MAX];
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if ((PNULL == pm_cxt_ptr) || (PNULL == param_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, param_ptr = %p", pm_cxt_ptr, param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	switch (cmd) {
	case ISP_PM_CMD_SET_MODE:
	{
		cmr_u32 i, k, set_id;
		cmr_u32 *search, mode_id;
		struct pm_workmode_input *input = (struct pm_workmode_input *)param_ptr;
		struct pm_workmode_output *output = (struct pm_workmode_output *)out_ptr;
		struct isp_context *isp_cxt_ptr = PNULL;

		if (out_ptr == PNULL) {
			ISP_LOGE("fail to get output ptr.\n");
			rtn = ISP_ERROR;
			return rtn;
		}

		for (i  = 0; i < PARAM_SET_MAX; i++) {
			isp_cxt_ptr = &pm_cxt_ptr->cxt_array[i];
			update_always[i] = (isp_cxt_ptr->is_validate == 0) ? 1 : 0;
			isp_cxt_ptr->is_validate = 0;
		}

		pm_cxt_ptr->cam_4in1_mode = input->cam_4in1_mode;
		pm_cxt_ptr->noramosaic_4in1 = input->noramosaic_4in1;
		for (i  = 0; i < input->pm_sets_num && i < PARAM_SET_MAX; i++) {
			if (input->mode[i] >= WORKMODE_MAX)
				continue;

			if (pm_cxt_ptr->is_4in1_sensor) {
				if (input->mode[i] == WORKMODE_PREVIEW) {
					if (pm_cxt_ptr->cam_4in1_mode)
						output->mode_id[i] = ISP_MODE_ID_PRV_0;
					else
						output->mode_id[i] = ISP_MODE_ID_PRV_1;
					goto get_blocks;
				} else if (input->mode[i] == WORKMODE_VIDEO) {
					if (pm_cxt_ptr->cam_4in1_mode) {
						output->mode_id[i] = ISP_MODE_ID_VIDEO_0;
						goto get_blocks;
					}
					search = &search_modes[2][0];
					search++;
					goto search;
				} else if (input->mode[i] == WORKMODE_CAPTURE) {
					if (!pm_cxt_ptr->cam_4in1_mode)
						output->mode_id[i] = ISP_MODE_ID_CAP_2;
					else if (pm_cxt_ptr->lowlight_flag)
						output->mode_id[i] = ISP_MODE_ID_CAP_1;
					else
						output->mode_id[i] = ISP_MODE_ID_CAP_0;
					update_always[i] = 1;
					goto get_blocks;
				}
			}

			if (input->mode[i] == WORKMODE_PREVIEW)
				search = &search_modes[0][0];
			else if (input->mode[i] == WORKMODE_CAPTURE)
				search = &search_modes[1][0];
			else
				search = &search_modes[2][0];
search:
			output->mode_id[i] = ISP_MODE_ID_COMMON;
			for (k = 0; k < ISP_TUNE_MODE_MAX; k++) {
				mode_id = search[k];
				if (mode_id == ISP_MODE_ID_MAX)
					break;
				if (pm_cxt_ptr->tune_mode_array[mode_id] == PNULL)
					continue;

				ISP_LOGD("i %d, k %d, mode %d, mode_id %d.  4in1 %d,  size %d %d.  insize %d %d\n",
					i,  k, mode_id,
					pm_cxt_ptr->tune_mode_array[mode_id]->mode_id,
					pm_cxt_ptr->tune_mode_array[mode_id]->for_4in1,
					pm_cxt_ptr->tune_mode_array[mode_id]->resolution.w,
					pm_cxt_ptr->tune_mode_array[mode_id]->resolution.h,
					input->img_w[i], input->img_h[i]);

				if (pm_cxt_ptr->is_4in1_sensor && input->mode[i] == WORKMODE_PREVIEW) {
					/* todo:  workaround for 4in1 preview  */
					ISP_LOGD("i %d, k %d, is_4in1_sensor & preview. 4in1 %d %d. size %d in %d\n", i, k,
					pm_cxt_ptr->tune_mode_array[mode_id]->for_4in1,
					input->cam_4in1_mode,
					pm_cxt_ptr->tune_mode_array[mode_id]->resolution.w,
					input->img_w[i]);
					if ((pm_cxt_ptr->tune_mode_array[mode_id]->resolution.w == input->img_w[i]) &&
						(pm_cxt_ptr->tune_mode_array[mode_id]->for_4in1 == input->cam_4in1_mode)) {
						output->mode_id[i] = pm_cxt_ptr->tune_mode_array[mode_id]->mode_id;
						ISP_LOGD("i %d k %d, get mode %d\n", i, k, output->mode_id[i]);
						break;
					}
				} else  if (pm_cxt_ptr->tune_mode_array[mode_id]->resolution.w == input->img_w[i]) {
					output->mode_id[i] = pm_cxt_ptr->tune_mode_array[mode_id]->mode_id;
					ISP_LOGD("i %d k %d, get mode %d\n", i, k, output->mode_id[i]);
					break;
				}
			}
get_blocks:
			if (pm_cxt_ptr->param_search_list)
				isp_pm_get_all_blocks(handle, &pm_cxt_ptr->blocks_param[i],
					input->mode[i], input->scene[i], input->define[i],
					input->img_w[i], input->img_h[i], update_always[i]);
			else
				isp_pm_get_all_blocks_compatible(handle,
					&pm_cxt_ptr->blocks_param[i], output->mode_id[i],
					input->mode[i], input->scene[i], input->define[i],
					input->img_w[i], input->img_h[i], update_always[i]);
			set_id = i;
			isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
			isp_cxt_ptr->is_validate = 1;
			isp_cxt_ptr->mode_id = output->mode_id[i];
			rtn = isp_pm_context_init((cmr_handle)pm_cxt_ptr, set_id);
			if (rtn)
				return ISP_PARAM_ERROR;
			ISP_LOGD("pm context %p for set %d, mode_id %d 4in1 mode %d, size %d %d\n", isp_cxt_ptr,
				set_id, isp_cxt_ptr->mode_id, pm_cxt_ptr->cam_4in1_mode, input->img_w[i], input->img_h[i]);
		}
		break;
	}
	case ISP_PM_CMD_SET_LOWLIGHT_FLAG:
	{
		pm_cxt_ptr->lowlight_flag = *(cmr_u32 *)param_ptr;
		ISP_LOGV("lowlight_flag = %d\n", pm_cxt_ptr->lowlight_flag);
		if (pm_cxt_ptr->cam_4in1_mode) {
			cmr_u32 i, j, k;
			cmr_u32 *cap_mode;
			struct isp_pm_blocks_param *blk_param_ptr;
			struct isp_context *isp_cxt_ptr = PNULL;
			struct isp_pm_mode_param *cap, *comm;
			struct isp_pm_block_header *src_h, *dst_h;
#ifdef CONFIG_ISP_2_7
			const cmr_u32 tb_blk[] = {ISP_BLK_BLC, /* ISP_BLK_2D_LSC,*/ ISP_BLK_NLM_V2,
				/*ISP_BLK_SMART,*/ ISP_BLK_3DNR, DCAM_BLK_BPC_V1,
				ISP_BLK_UVDIV_V1, ISP_BLK_IIRCNR_IIR_V1,
				ISP_BLK_UV_CDN_V1, ISP_BLK_CFA_V1, ISP_BLK_CNR2_V1,
				ISP_BLK_EE_V1, ISP_BLK_GRGB_V1, ISP_BLK_IMBALANCE_V1,
				ISP_BLK_YUV_NOISEFILTER_V1, ISP_BLK_UV_POSTCDN_V1,
				ISP_BLK_YUV_PRECDN_V1, ISP_BLK_SW3DNR, ISP_BLK_YNR_V1
			}; /* blocks from cap1 */
#elif CONFIG_ISP_2_6
			const cmr_u32 tb_blk[] = {ISP_BLK_BLC, /* ISP_BLK_2D_LSC,*/ ISP_BLK_NLM_V1,
				/*ISP_BLK_SMART,*/ ISP_BLK_3DNR, DCAM_BLK_BPC_V1,
				ISP_BLK_UVDIV_V1, ISP_BLK_IIRCNR_IIR_V1,
				ISP_BLK_UV_CDN_V1, ISP_BLK_CFA_V1, ISP_BLK_CNR2_V1,
				ISP_BLK_EE_V1, ISP_BLK_GRGB_V1, ISP_BLK_IMBALANCE,
				ISP_BLK_LTM, ISP_BLK_YUV_NOISEFILTER_V1,
				ISP_BLK_UV_POSTCDN_V1, ISP_BLK_YUV_PRECDN_V1,
				ISP_BLK_SW3DNR, ISP_BLK_YNR_V1
			}; /* blocks from cap1 */
#elif CONFIG_ISP_2_5
			const cmr_u32 tb_blk[] = {ISP_BLK_BLC, /* ISP_BLK_2D_LSC,*/ ISP_BLK_NLM_V1,
				/*ISP_BLK_SMART,*/ ISP_BLK_3DNR, DCAM_BLK_BPC_V1,
				ISP_BLK_UVDIV_V1, ISP_BLK_IIRCNR_IIR_V1,
				ISP_BLK_UV_CDN_V1, ISP_BLK_CFA_V1, ISP_BLK_CNR2_V1,
				ISP_BLK_EE_V1, ISP_BLK_GRGB_V1, ISP_BLK_IMBALANCE,
				ISP_BLK_YUV_NOISEFILTER_V1,ISP_BLK_UV_POSTCDN_V1,
				ISP_BLK_YUV_PRECDN_V1,ISP_BLK_SW3DNR, ISP_BLK_YNR_V1
			}; /* blocks from cap1 */
#endif
			for (i = 0; i < PARAM_SET_MAX; i++) {
				blk_param_ptr = &pm_cxt_ptr->blocks_param[i];
				isp_cxt_ptr = &pm_cxt_ptr->cxt_array[i];
				if (!isp_cxt_ptr->is_validate || blk_param_ptr->mode != WORKMODE_CAPTURE)
					continue;
				if (pm_cxt_ptr->lowlight_flag)
					isp_cxt_ptr->mode_id = ISP_MODE_ID_CAP_1;
				else
					isp_cxt_ptr->mode_id = ISP_MODE_ID_CAP_0;
				pm_cxt_ptr->cap_mode_id = isp_cxt_ptr->mode_id;

				if (out_ptr) {
					cap_mode = (cmr_u32 *)out_ptr;
					*cap_mode = isp_cxt_ptr->mode_id;
				}
				ISP_LOGD("lowlight %d, NR mode %d\n", pm_cxt_ptr->lowlight_flag, isp_cxt_ptr->mode_id);
				for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
					cap = pm_cxt_ptr->tune_mode_array[j];
					if (cap && cap->mode_id == isp_cxt_ptr->mode_id)
						break;
					cap = NULL;
				}
				for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
                                        comm = pm_cxt_ptr->tune_mode_array[j];
                                        if (comm && comm->mode_id == ISP_MODE_ID_COMMON)
                                                break;
                                        comm = NULL;
                                }
				if (comm == NULL || cap == NULL) {
					ISP_LOGE("NULL point %p %p\n", cap, comm);
					continue;
				}
				for (k = 0; k < sizeof(tb_blk) / sizeof(tb_blk[0]); k++) {
					for (j = 0; j < ISP_TUNE_BLOCK_MAX; j++) {
						dst_h = &blk_param_ptr->header[j];
						if (dst_h->block_id == tb_blk[k])
							break;
					}
					if (j >= ISP_TUNE_BLOCK_MAX)
						continue;
					for (j = 0; j < ISP_TUNE_BLOCK_MAX && cap; j++) {
						src_h = &cap->header[j];
						if (src_h && src_h->block_id == tb_blk[k])
							break;
					}
					if (src_h && j < ISP_TUNE_BLOCK_MAX) {
						memcpy(dst_h, src_h, sizeof(struct isp_pm_block_header));
						dst_h->is_update = 1;
						dst_h->source_flag |= ISP_PM_BLK_UPDATE;
						continue;
					}
					for (j = 0; j < ISP_TUNE_BLOCK_MAX && comm; j++) {
                                                src_h = &comm->header[j];
                                                if (src_h && src_h->block_id == tb_blk[k])
                                                        break;
                                        }
					if (src_h && i < ISP_TUNE_BLOCK_MAX) {
                                                memcpy(dst_h, src_h, sizeof(struct isp_pm_block_header));
                                                dst_h->is_update = 1;
						dst_h->source_flag |= ISP_PM_BLK_UPDATE;
                                                continue;
                                        }
				}
			}
			isp_pm_context_init(pm_cxt_ptr, PARAM_SET1);
		}
		break;
	}
	case ISP_PM_CMD_SET_AWB:
	case ISP_PM_CMD_SET_AE:
	case ISP_PM_CMD_SET_AF:
	case ISP_PM_CMD_SET_OTHERS:
	case ISP_PM_CMD_SET_SPECIAL_EFFECT:
	{
		cmr_u32 i = 0;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr = ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			ISP_LOGE("fail to get valid param_data_ptr");
			rtn = ISP_ERROR;
			return rtn;
		}
		for (i = 0; i < ioctrl_input_ptr->param_num; i++, param_data_ptr++) {
			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, PARAM_SET0);
			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, PARAM_SET1);
		}
		break;
	}
	case ISP_PM_CMD_SET_GRID0:
	case ISP_PM_CMD_SET_GRID1:
	{
		cmr_u32 set_id = PARAM_SET0;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr = ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			ISP_LOGE("fail to get valid param_data_ptr");
			rtn = ISP_ERROR;
			return rtn;
		}
		set_id = (cmd == ISP_PM_CMD_SET_GRID0) ? PARAM_SET0 : PARAM_SET1;
		rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, set_id);
		break;
	}
	case ISP_PM_CMD_SET_SMART:
	{
		cmr_u32 i = 0;
		cmr_u32 set_id;
		struct isp_context *isp_cxt_ptr = PNULL;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr = ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			ISP_LOGE("fail to get valid param_data_ptr");
			rtn = ISP_ERROR;
			return rtn;
		}

		for (i = 0; i < ioctrl_input_ptr->param_num; i++, param_data_ptr++) {
			for (set_id = PARAM_SET0; set_id < PARAM_SET_MAX; set_id++) {
				isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
				if (isp_cxt_ptr->is_validate &&
					(isp_cxt_ptr->mode_id == param_data_ptr->mode_id)) {
					break;
				}
			}
			if (set_id == PARAM_SET_MAX) {
				ISP_LOGV("fail to set smart for mode %d\n", param_data_ptr->mode_id);
				continue;
			}

			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, set_id);
		}
		break;
	}
	case ISP_PM_CMD_SET_PARAM_SOURCE:
	{
		cmr_u32 *param_source = (cmr_u32 *)param_ptr;
		pm_cxt_ptr->param_source = *param_source;
		break;
	}
	default:
		break;
	}

	return rtn;
}


static cmr_s32 isp_pm_get_param(cmr_handle handle, enum isp_pm_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 block_id = 0;
	cmr_u32 param_counts = 0;
	struct isp_pm_param_data *param_data_ptr = PNULL;
	struct isp_video_start *param_ptr = PNULL;
	struct isp_pm_ioctl_output *result_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if ((PNULL == pm_cxt_ptr) || (PNULL == out_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, out_ptr = %p", pm_cxt_ptr, out_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	if (cmd == ISP_PM_CMD_GET_DV_MODEID_BY_FPS ||
		cmd == ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION ||
		cmd == ISP_PM_CMD_GET_PRV_MODEID_BY_RESOLUTION ||
		cmd == ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION) {
		cmr_u32 *search, mode;

		if (cmd == ISP_PM_CMD_GET_PRV_MODEID_BY_RESOLUTION)
			search = &search_modes[0][0];
		else if(cmd == ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION)
			search = &search_modes[1][0];
		else
			search = &search_modes[2][0];
		param_ptr = (struct isp_video_start *)in_ptr;
		*((cmr_s32 *)out_ptr) = ISP_MODE_ID_COMMON;
		for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
			mode = search[i];
			if (mode == ISP_MODE_ID_MAX)
				break;
			if (pm_cxt_ptr->tune_mode_array[mode] == PNULL)
				continue;
			ISP_LOGD("i %d, mode %d, mode_id %d.  size %d %d.  insize %d %d\n", i, mode,
				pm_cxt_ptr->tune_mode_array[mode]->mode_id,
				pm_cxt_ptr->tune_mode_array[mode]->resolution.w,
				pm_cxt_ptr->tune_mode_array[mode]->resolution.h,
				param_ptr->size.w, param_ptr->size.h);
			if (cmd == ISP_PM_CMD_GET_DV_MODEID_BY_FPS) {
				if (pm_cxt_ptr->tune_mode_array[mode]->fps == (cmr_u32)(*(cmr_s32 *)in_ptr)) {
					*((cmr_s32 *)out_ptr) = pm_cxt_ptr->tune_mode_array[mode]->mode_id;
					break;
				}
			} else {
				if (pm_cxt_ptr->tune_mode_array[mode]->resolution.w == param_ptr->size.w) {
					*((cmr_s32 *)out_ptr) = pm_cxt_ptr->tune_mode_array[mode]->mode_id;
					break;
				}
			}
		}
		ISP_LOGD("get mode %d for cmd %d\n", *(cmr_s32 *)out_ptr, cmd);
	} else if (cmd == ISP_PM_CMD_GET_ISP_SETTING || cmd == ISP_PM_CMD_GET_ISP_ALL_SETTING) {
		cmr_u32 all_setting_flag = 0;
		struct isp_pm_ioctl_output_param *result_param_ptr = PNULL;
		result_param_ptr = (struct isp_pm_ioctl_output_param *)out_ptr;

		if (ISP_PM_CMD_GET_ISP_SETTING == cmd) {
			all_setting_flag = 0;
		} else if (ISP_PM_CMD_GET_ISP_ALL_SETTING == cmd) {
			all_setting_flag = 1;
		}

		param_counts = 0;
		param_data_ptr = pm_cxt_ptr->getting_data_ptr[0];
		rtn = isp_pm_get_setting_param(pm_cxt_ptr,
				param_data_ptr, &param_counts,
				all_setting_flag, PARAM_SET0);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to do isp_pm_get_setting_param");
			rtn = ISP_ERROR;
			return rtn;
		}
		result_param_ptr->prv_param_data = pm_cxt_ptr->getting_data_ptr[0];
		result_param_ptr->prv_param_num = param_counts;
		result_param_ptr->prv_param_data->mode_id = pm_cxt_ptr->prv_mode_id;

		param_counts = 0;
		param_data_ptr = pm_cxt_ptr->getting_data_ptr[1];
		rtn = isp_pm_get_setting_param(pm_cxt_ptr,
				param_data_ptr, &param_counts,
				all_setting_flag, PARAM_SET1);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGV("fail to do isp_pm_get_setting_param");
			rtn = ISP_ERROR;
			return rtn;
		}
		result_param_ptr->cap_param_data = pm_cxt_ptr->getting_data_ptr[1];
		result_param_ptr->cap_param_num = param_counts;
		result_param_ptr->cap_param_data->mode_id = pm_cxt_ptr->cap_mode_id;
	} else if (cmd == ISP_PM_CMD_GET_SINGLE_SETTING ||
		cmd == ISP_PM_CMD_GET_CAP_SINGLE_SETTING) {
		cmr_u32 blk_idx = 0;
		cmr_u32 set_id;

		if (in_ptr == NULL) {
			ISP_LOGE("null input ptr.\n");
			return ISP_ERROR;
		}

		param_data_ptr = pm_cxt_ptr->single_block_data;
		set_id = (cmd == ISP_PM_CMD_GET_SINGLE_SETTING) ? PARAM_SET0 : PARAM_SET1;
		rtn = isp_pm_get_single_block_param(pm_cxt_ptr,
				(struct isp_pm_ioctl_input *)in_ptr, param_data_ptr,
				&param_counts, &blk_idx, set_id);
		if (ISP_SUCCESS != rtn || blk_idx == ISP_TUNE_BLOCK_MAX) {
			ISP_LOGV("fail to do isp_pm_get_single_block_param");
			rtn = ISP_ERROR;
			return rtn;
		}

		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_data = &pm_cxt_ptr->single_block_data[blk_idx];
		result_ptr->param_num = param_counts;
		result_ptr->param_data->mode_id = pm_cxt_ptr->mode_id;

	} else {
		switch (cmd) {
		case ISP_PM_CMD_GET_INIT_AE:
			block_id = ISP_BLK_AE_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_ALSC:
			block_id = ISP_BLK_ALSC;
			break;
		case ISP_PM_CMD_GET_INIT_AWB:
			block_id = ISP_BLK_AWB_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_AF:
		case ISP_PM_CMD_GET_INIT_AF_NEW:
			block_id = ISP_BLK_AF_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_SMART:
			block_id = ISP_BLK_SMART;
			break;
		case ISP_PM_CMD_GET_INIT_AFT:
			block_id = ISP_BLK_AFT;
			break;
		case ISP_PM_CMD_GET_INIT_PDAF:
			block_id = ISP_BLK_PDAF_TUNE;
			break;
		case ISP_PM_CMD_GET_INIT_DUAL_FLASH:
			block_id = ISP_BLK_DUAL_FLASH;
			break;
		case ISP_PM_CMD_GET_AE_SYNC:
			block_id = ISP_BLK_AE_SYNC;
			break;
		case ISP_PM_CMD_GET_AE_ADAPT_PARAM:
			block_id = ISP_BLK_AE_ADAPT_PARAM;
			break;
		case ISP_PM_CMD_GET_4IN1_PARAM:
			block_id = ISP_BLK_4IN1_PARAM;
			break;
		case ISP_PM_CMD_GET_INIT_TOF:
			block_id = ISP_BLK_TOF_TUNE;
			break;
		case ISP_PM_CMD_GET_ATM_PARAM:
			block_id = ISP_BLK_ATM_TUNE;
			break;
		default:
			break;
		}

		if (block_id != 0) {
			param_data_ptr = pm_cxt_ptr->getting_data_ptr[0];
			rtn = isp_pm_get_mode_block_param(pm_cxt_ptr, cmd,
					param_data_ptr, &param_counts, block_id);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_get_mode_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}

			result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
			result_ptr->param_data = pm_cxt_ptr->getting_data_ptr[0];
			result_ptr->param_num = param_counts;
		}
	}

	return rtn;
}

static cmr_s32 isp_pm_mode_list_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
		if (pm_cxt_ptr->tune_mode_array[i]) {
			free(pm_cxt_ptr->tune_mode_array[i]);
			pm_cxt_ptr->tune_mode_array[i] = PNULL;
		}
	}

	for (i = 0; i < PARAM_SET_MAX; i++) {
		if (pm_cxt_ptr->getting_data_ptr[i] == PNULL)
			continue;
		free(pm_cxt_ptr->getting_data_ptr[i]);
		pm_cxt_ptr->getting_data_ptr[i] = PNULL;
	}

	ISP_LOGV("isp_pm_param_list_deinit : done");

	return rtn;
}


static cmr_s32 debug_save_nr_data(void *dataptr, cmr_u32 datalen,
	cmr_u32 unit_len, cmr_u32 level_num,  cmr_u32 nr_type,
	cmr_s8 *sensor_name, cmr_u32 mode_id, cmr_u32 scene_id)
{
	int fd;
	char file_name[256];

	datalen = (datalen < (unit_len * level_num)) ? datalen : (unit_len * level_num);
	if (nr_type >= ISP_BLK_NR_MAX) {
		ISP_LOGE("Invalid nr type %d\n", nr_type);
		return ISP_ERROR;
	}

	sprintf(file_name, "%sdump/%s_%s_%s_%s_param.bin", CAMERA_DUMP_PATH,
		sensor_name,  nr_mode_name[mode_id],
		nr_scene_name[scene_id], nr_param_name[nr_type]);
	fd = open(file_name, O_RDWR | O_CREAT, 0);
	if (fd > 0) {
		write(fd, dataptr, datalen);
		close(fd);
		ISP_LOGD("NR type %d,  base %p   size %d, %d, levels %d, save to file %s\n",
			nr_type, dataptr, datalen, unit_len, level_num, file_name);
	}

	return 0;
}


cmr_s32 check_nr_blks(cmr_u32 blk_id, cmr_u32 *nr_type, cmr_u32 *unit_size)
{
	cmr_u32 i;

	for (i = 0; i < sizeof(nr_blocks_info)/sizeof(nr_blocks_info[0]); i++) {
		if (nr_blocks_info[i].blk_id == blk_id) {
			*nr_type = nr_blocks_info[i].nr_type;
			*unit_size = nr_blocks_info[i].unit_size;
			return 0;
		}
	}
	return -1;
}

static cmr_s32 isp_pm_mode_list_init(cmr_handle handle,
	struct isp_pm_init_input *input, struct isp_pm_init_output *output)
{
	cmr_s32 rtn = ISP_SUCCESS;

	cmr_u32 i = 0, j = 0, k = 0;
	cmr_u32 max_num = 0;

	cmr_u32 extend_offset = 0;
	cmr_u32 data_area_size = 0;
	cmr_u32 size = 0;
	cmr_u32 add_ae_len = 0,ae_end_len = 16, add_awb_len = 0, add_lnc_len = 0;
	cmr_u32 nr_scene_map;
	cmr_u32 nr_scene_id;

	struct isp_mode_param *src_mod_ptr = PNULL;
	struct isp_pm_mode_param *dst_mod_ptr = PNULL;
	struct isp_block_header *src_header = PNULL;
	struct isp_pm_block_header *dst_header = PNULL;
	struct isp_pm_block_header *hsv_header, *hsv_new_header;
	cmr_u8 *src_data_ptr = PNULL;
	cmr_u8 *dst_data_ptr = PNULL;

	struct sensor_raw_fix_info *fix_data_ptr = PNULL;
	struct sensor_nr_fix_info *nr_fix_ptr = PNULL;
	struct sensor_nr_scene_map_param *nr_scene_map_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_level_number_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_default_level_ptr = PNULL;
	struct nr_set_group_unit *nr_ptr = PNULL;
	struct isp_pm_nr_header_param *dst_nlm_data = PNULL;
	struct isp_pm_nr_simple_header_param *dst_blk_data = PNULL;
	cmr_u32 multi_nr_flag = 0;
	cmr_u32 isp_blk_nr_type = ISP_BLK_NR_MAX;
	intptr_t nr_set_addr = 0;
	cmr_u32 nr_set_size = 0, nr_unit_size = 0;
	cmr_u32 nr_mode_offset[ISP_BLK_NR_MAX];
	cmr_u32 nr_data_len[ISP_BLK_NR_MAX];
	cmr_u32 nr_blk_id[ISP_BLK_NR_MAX];
	char value[PROPERTY_VALUE_MAX] = { 0x00 };
	cmr_u32 val, dump_nrdata = 0;
	cmr_s8 *sensor_name;

	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if ((PNULL == pm_cxt_ptr) || (PNULL == input)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, input = %p",
			pm_cxt_ptr, input);
		rtn = ISP_ERROR;
		return rtn;
	}

	if (input->sensor_raw_info_ptr == PNULL) {
		ISP_LOGE("fail to get valid sensor_raw_info_ptr(null)\n");
		return ISP_ERROR;
	}

	multi_nr_flag = SENSOR_MULTI_MODE_FLAG;//SENSOR_DEFAULT_MODE_FLAG
	if (output)
		output->multi_nr_flag = multi_nr_flag;

	if (pm_cxt_ptr->param_source != ISP_PARAM_FROM_TOOL) {
		pm_cxt_ptr->is_4in1_sensor = input->is_4in1_sensor;
		ISP_LOGD("is_4in1_sensor = %d", pm_cxt_ptr->is_4in1_sensor);
	}

	nr_fix_ptr = input->nr_fix_info;
	nr_scene_map_ptr = (struct sensor_nr_scene_map_param *)(nr_fix_ptr->nr_scene_ptr);
	nr_level_number_ptr = (struct sensor_nr_level_map_param *)(nr_fix_ptr->nr_level_number_ptr);
	nr_default_level_ptr = (struct sensor_nr_level_map_param *)(nr_fix_ptr->nr_default_level_ptr);
	memset(&nr_mode_offset[0], 0, sizeof(nr_mode_offset));
	memset(&nr_data_len[0], 0, sizeof(nr_data_len));
	memset(&nr_blk_id[0], 0, sizeof(nr_blk_id));

	property_get("debug.isp.pm.dump.nr", value, "0");
	val = atoi(value);
	if (val < 2)
		dump_nrdata = val;
	ISP_LOGD("dump isp pm nr %d\n", dump_nrdata);
	sensor_name = input->sensor_raw_info_ptr->version_info->sensor_ver_name.sensor_name;

	ISP_LOGD("sensor_name %s\n", sensor_name);

#ifdef CONFIG_ISP_2_6
	pm_cxt_ptr->param_search_list = input->sensor_raw_info_ptr->param_list_info.list_ptr;
	pm_cxt_ptr->param_search_list_size = input->sensor_raw_info_ptr->param_list_info.list_len;
#endif
	if (pm_cxt_ptr->param_search_list == PNULL || pm_cxt_ptr->param_search_list_size == 0) {
		ISP_LOGE("specified pm searching list. %p, %d\n",
			pm_cxt_ptr->param_search_list, pm_cxt_ptr->param_search_list_size);
		pm_cxt_ptr->param_search_list = PNULL;
		pm_cxt_ptr->param_search_list_size = 0;
		goto start_parse;
	} else {
		ISP_LOGD("specified pm searching list. %p, %d\n",
			pm_cxt_ptr->param_search_list, pm_cxt_ptr->param_search_list_size);
		goto start_parse;
	}

	/* temp solution to get list here */
	/* if param_search_lists NULL passed from input->sensor_raw_info_ptr */
	if (!strncmp((const char *)sensor_name, "ov12a10", 7)) {
		pm_cxt_ptr->param_search_list = &params_list_ov12a[0];
		pm_cxt_ptr->param_search_list_size = sizeof(params_list_ov12a);
		ISP_LOGD("ov12a10  %p, size = %d\n", pm_cxt_ptr->param_search_list, pm_cxt_ptr->param_search_list_size);
	} else if (!strncmp((const char *)sensor_name, "ov8856", 6)) {
		pm_cxt_ptr->param_search_list = &params_list_ov8856[0];
		pm_cxt_ptr->param_search_list_size = sizeof(params_list_ov8856);
		ISP_LOGD("ov8856  %p, size = %d\n", pm_cxt_ptr->param_search_list, pm_cxt_ptr->param_search_list_size);
	} else if (!strncmp((const char *)sensor_name, "ov5675", 6)) {
		pm_cxt_ptr->param_search_list = &params_list_ov5675[0];
		pm_cxt_ptr->param_search_list_size = sizeof(params_list_ov5675);
		ISP_LOGD("ov5675  %p, size = %d\n", pm_cxt_ptr->param_search_list, pm_cxt_ptr->param_search_list_size);
	} else if (!strncmp((const char *)sensor_name, "ov16885", 6)) {
		pm_cxt_ptr->param_search_list = &params_list_ov16885[0];
		pm_cxt_ptr->param_search_list_size = sizeof(params_list_ov16885);
		ISP_LOGD("ov16885  %p, size = %d\n", pm_cxt_ptr->param_search_list, pm_cxt_ptr->param_search_list_size);
	}

start_parse:
	for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
		cmr_u32 mode_data_size;
		extend_offset = 0;

		src_mod_ptr = (struct isp_mode_param *)input->tuning_data[i].data_ptr;
		if (PNULL == src_mod_ptr)
			continue;
		src_header = (struct isp_block_header *)src_mod_ptr->block_header;

		data_area_size = src_mod_ptr->size - sizeof(struct isp_mode_param);
		size = data_area_size + sizeof(struct isp_pm_mode_param);

		fix_data_ptr = input->fix_data[i];

		mode_data_size = input->tuning_data[i].size;
		ISP_LOGD("size hdr %d, data_size %d,  size hdr1 %d, size1 %d\n", (cmr_u32)sizeof(struct isp_mode_param),
			data_area_size, (cmr_u32)sizeof(struct isp_pm_mode_param), size);
		ISP_LOGD("mode %d, ptr %p, size %d, blknum %d, fixptr %p. img size %d %d\n", src_mod_ptr->mode_id,
			src_mod_ptr, mode_data_size, src_mod_ptr->block_num, fix_data_ptr, src_mod_ptr->width, src_mod_ptr->height);

		add_ae_len = fix_data_ptr->ae.ae_param.ae_len;
		add_lnc_len = fix_data_ptr->lnc.lnc_param.lnc_len;
		add_awb_len = fix_data_ptr->awb.awb_param.awb_len;
		size += add_ae_len + add_lnc_len + add_awb_len;

		nr_ptr = (struct nr_set_group_unit *)&(fix_data_ptr->nr.nr_set_group);
		for (k = 0; k < sizeof(struct sensor_nr_set_group_param) / sizeof(struct nr_set_group_unit); k++) {
			if (PNULL != nr_ptr[k].nr_ptr) {
				size += sizeof(struct isp_pm_nr_simple_header_param);
			}
		}

		if (PNULL != pm_cxt_ptr->tune_mode_array[i]) {
			free(pm_cxt_ptr->tune_mode_array[i]);
			pm_cxt_ptr->tune_mode_array[i] = PNULL;
		}
		pm_cxt_ptr->tune_mode_array[i] = (struct isp_pm_mode_param *)malloc(size);
		if (PNULL == pm_cxt_ptr->tune_mode_array[i]) {
			ISP_LOGE("fail to malloc tune_mode_array : i = %d", i);
			rtn = ISP_ERROR;
			goto init_param_list_error_exit;
		}
		memset((void *)pm_cxt_ptr->tune_mode_array[i], 0x00, size);

		dst_mod_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[i];
		dst_header = (struct isp_pm_block_header *)dst_mod_ptr->header;
		hsv_header = hsv_new_header = PNULL;

		for (j = 0; j < src_mod_ptr->block_num; j++) {
			if (!check_blk_id_valid(src_header[j].block_id, src_header[j].size)) {
				ISP_LOGD("discard blk 0x%04x for mode %d\n", src_header[j].block_id, src_mod_ptr->mode_id);
				continue;
			}

			dst_header[j].is_update = 0;
			dst_header[j].source_flag = i;
			dst_header[j].bypass = src_header[j].bypass;
			dst_header[j].block_id = src_header[j].block_id;
			dst_header[j].param_id = src_header[j].param_id;
			dst_header[j].version_id = src_header[j].version_id;
			dst_header[j].size = src_header[j].size;

			size = src_header[j].offset - sizeof(struct isp_mode_param);
			size = size + sizeof(struct isp_pm_mode_param);

			src_data_ptr = (cmr_u8 *)((intptr_t)src_mod_ptr + src_header[j].offset);
			dst_data_ptr = (cmr_u8 *)((intptr_t)dst_mod_ptr + size + extend_offset);

			dst_header[j].absolute_addr = (void *)dst_data_ptr;
			dst_header[j].mode_id = i;
			ISP_LOGV("j %d, blk 0x%04x, bypass %d, size %d, offset 0x%x, ptr0 %p, pt1 %p, real_off %ld\n", j,
				dst_header[j].block_id, src_header[j].bypass, dst_header[j].size, src_header[j].offset, src_data_ptr,
				dst_data_ptr, (cmr_uint)src_data_ptr - (cmr_uint)src_mod_ptr);

			memcpy((void *)dst_data_ptr, (void *)src_data_ptr, src_header[j].size);
			memcpy((void *)dst_header[j].name, (void *)src_header[j].block_name, sizeof(dst_header[j].name));

			switch (src_header[j].block_id) {
			case ISP_BLK_HSV:
			{
				hsv_header = &dst_header[j];
				ISP_LOGD("block hsv \n");
				break;
			}
			case ISP_BLK_HSV_NEW:
			{
				hsv_new_header = &dst_header[j];
				ISP_LOGD("block hsv new\n");
				break;
			}
			case ISP_BLK_2D_LSC:
			{
				extend_offset += add_lnc_len;
				dst_header[j].size = src_header[j].size + add_lnc_len;
				memcpy((void *)(dst_data_ptr + src_header[j].size),
					(void *)(fix_data_ptr->lnc.lnc_param.lnc), add_lnc_len);
				break;
			}
			case ISP_BLK_AE_NEW:
			{
				extend_offset += add_ae_len;
				dst_header[j].size = src_header[j].size + add_ae_len;
				if (src_mod_ptr->version_id >= ISP_TOOL_VERSION_ID) {
					memcpy((void *)(dst_data_ptr + src_header[j].size - ae_end_len),
						(void *)(fix_data_ptr->ae.ae_param.ae), add_ae_len);
					memcpy((void *)(dst_data_ptr + src_header[j].size - ae_end_len + add_ae_len),
						(void *)(src_data_ptr + src_header[j].size - ae_end_len),ae_end_len);
				} else {
					memcpy((void *)(dst_data_ptr + sizeof(struct ae_param_tmp_001)),
						(void *)(fix_data_ptr->ae.ae_param.ae), add_ae_len);
					memcpy((void *)(dst_data_ptr + sizeof(struct ae_param_tmp_001) + add_ae_len),
						(void *)(src_data_ptr + sizeof(struct ae_param_tmp_001)),
						(src_header[j].size - sizeof(struct ae_param_tmp_001)));
				}
				break;
			}
			case ISP_BLK_AWB_NEW:
			{
				break;
			}
			case DCAM_BLK_NLM:
			case ISP_BLK_NLM_V1:
			case ISP_BLK_NLM_V2:
			{
				dst_nlm_data = (struct isp_pm_nr_header_param *)dst_data_ptr;
				memset((void *)dst_nlm_data, 0x00, sizeof(struct isp_pm_nr_header_param));

				dst_nlm_data->level_number = nr_level_number_ptr->nr_level_map[ISP_BLK_NLM_T];
				dst_nlm_data->default_strength_level = nr_default_level_ptr->nr_level_map[ISP_BLK_NLM_T];
				dst_nlm_data->nr_mode_setting = multi_nr_flag;
				dst_nlm_data->multi_nr_map_ptr = (cmr_uint *)&(nr_scene_map_ptr->nr_scene_map[0]);
				dst_nlm_data->param_ptr = (cmr_uint *)fix_data_ptr->nr.nr_set_group.nlm;
				dst_nlm_data->param1_ptr = (cmr_uint *)fix_data_ptr->nr.nr_set_group.vst;
				dst_nlm_data->param2_ptr = (cmr_uint *)fix_data_ptr->nr.nr_set_group.ivst;

				extend_offset += 3 * sizeof(struct isp_pm_nr_simple_header_param);
				dst_header[j].size = 3 * sizeof(struct isp_pm_nr_simple_header_param);
				nr_data_len[ISP_BLK_NLM_T] = fix_data_ptr->nr.nr_set_group.nlm_len;
				nr_data_len[ISP_BLK_VST_T] = fix_data_ptr->nr.nr_set_group.vst_len;
				nr_data_len[ISP_BLK_IVST_T] = fix_data_ptr->nr.nr_set_group.ivst_len;
				nr_blk_id[ISP_BLK_NLM_T] = src_header[j].block_id;
				nr_scene_map = nr_scene_map_ptr->nr_scene_map[src_mod_ptr->mode_id];
				for (nr_scene_id = ISP_SCENEMODE_AUTO; nr_scene_id < ISP_SCENEMODE_MAX; nr_scene_id++) {
					if ((nr_scene_map & (1 << nr_scene_id)) == 0)
						continue;
					ISP_LOGV("ISP_BLK_NLM, mode %d, scene_id %d\n", src_mod_ptr->mode_id, nr_scene_id);
					if (dump_nrdata) {
						debug_save_nr_data(fix_data_ptr->nr.nr_set_group.nlm + nr_mode_offset[ISP_BLK_NLM_T],
							fix_data_ptr->nr.nr_set_group.nlm_len,
							sizeof(struct sensor_nlm_level),
							nr_level_number_ptr->nr_level_map[ISP_BLK_NLM_T],
							ISP_BLK_NLM_T,
							sensor_name, src_mod_ptr->mode_id, nr_scene_id);

						debug_save_nr_data(fix_data_ptr->nr.nr_set_group.vst + nr_mode_offset[ISP_BLK_VST_T],
							fix_data_ptr->nr.nr_set_group.vst_len,
							sizeof(struct sensor_vst_level),
							nr_level_number_ptr->nr_level_map[ISP_BLK_VST_T],
							ISP_BLK_VST_T,
							sensor_name, src_mod_ptr->mode_id, nr_scene_id);

						debug_save_nr_data(fix_data_ptr->nr.nr_set_group.ivst + nr_mode_offset[ISP_BLK_IVST_T],
							fix_data_ptr->nr.nr_set_group.ivst_len,
							sizeof(struct sensor_ivst_level),
							nr_level_number_ptr->nr_level_map[ISP_BLK_IVST_T],
							ISP_BLK_IVST_T,
							sensor_name, src_mod_ptr->mode_id, nr_scene_id);
					}
					nr_mode_offset[ISP_BLK_NLM_T] += sizeof(struct sensor_nlm_level) * nr_level_number_ptr->nr_level_map[ISP_BLK_NLM_T];
					nr_mode_offset[ISP_BLK_VST_T] += sizeof(struct sensor_vst_level) * nr_level_number_ptr->nr_level_map[ISP_BLK_VST_T];
					nr_mode_offset[ISP_BLK_IVST_T] += sizeof(struct sensor_ivst_level) * nr_level_number_ptr->nr_level_map[ISP_BLK_IVST_T];
				}
				break;
			}
#if 0
			case DCAM_BLK_RGB_DITHER:
			case ISP_BLK_RGB_DITHER:
			{
				isp_blk_nr_type = ISP_BLK_RGB_DITHER_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.rgb_dither);
				nr_set_size = fix_data_ptr->nr.nr_set_group.rgb_dither_len;
				nr_unit_size = sizeof(struct sensor_rgb_dither_level);
				nr_blk_id[ISP_BLK_RGB_DITHER_T] = src_header[j].block_id;
				break;
			}
			case DCAM_BLK_BPC_V1:
			case DCAM_BLK_BPC:
			{
				isp_blk_nr_type = ISP_BLK_BPC_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.bpc);
				nr_set_size = fix_data_ptr->nr.nr_set_group.bpc_len;
				nr_unit_size = sizeof(struct sensor_bpc_level);
				nr_blk_id[ISP_BLK_BPC_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_GRGB_V1:
			case ISP_BLK_GRGB:
			{
				isp_blk_nr_type = ISP_BLK_GRGB_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.grgb);
				nr_set_size = fix_data_ptr->nr.nr_set_group.grgb_len;
				nr_unit_size = sizeof(struct sensor_grgb_level);
				nr_blk_id[ISP_BLK_GRGB_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_CFA:
			case ISP_BLK_CFA_V1:
			{
				isp_blk_nr_type = ISP_BLK_CFA_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.cfa);
				nr_set_size = fix_data_ptr->nr.nr_set_group.cfa_len;
				nr_unit_size = sizeof(struct sensor_cfa_param_level);
				nr_blk_id[ISP_BLK_CFA_T] = src_header[j].block_id;
				break;
			}
			case DCAM_BLK_RGB_AFM_V1: /* sharkl5 param use this block id for afm */
			case DCAM_BLK_RGB_AFM: /* sharkl3 param use this block id for afm */
			{
				isp_blk_nr_type = ISP_BLK_RGB_AFM_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.rgb_afm);
				nr_set_size = fix_data_ptr->nr.nr_set_group.rgb_afm_len;
				nr_unit_size = sizeof(struct sensor_rgb_afm_level);
				nr_blk_id[ISP_BLK_RGB_AFM_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_UVDIV:
			case ISP_BLK_UVDIV_V1:
			{
				isp_blk_nr_type = ISP_BLK_UVDIV_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.uvdiv);
				nr_set_size = fix_data_ptr->nr.nr_set_group.uvdiv_len;
				nr_unit_size = sizeof(struct sensor_cce_uvdiv_level);
				nr_blk_id[ISP_BLK_UVDIV_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_YUV_PRECDN:
			case ISP_BLK_YUV_PRECDN_V1:
			{
				isp_blk_nr_type = ISP_BLK_YUV_PRECDN_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.yuv_precdn);
				nr_set_size = fix_data_ptr->nr.nr_set_group.yuv_precdn_len;
				nr_unit_size = sizeof(struct sensor_yuv_precdn_level);
				nr_blk_id[ISP_BLK_YUV_PRECDN_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_YNR:
			case ISP_BLK_YNR_V1:
			{
				isp_blk_nr_type = ISP_BLK_YNR_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.ynr);
				nr_set_size = fix_data_ptr->nr.nr_set_group.ynr_len;
				nr_unit_size = sizeof(struct sensor_ynr_level);
				nr_blk_id[ISP_BLK_YNR_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_EDGE:
			case ISP_BLK_EE_V1:
			{
				isp_blk_nr_type = ISP_BLK_EDGE_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.edge);
				nr_set_size = fix_data_ptr->nr.nr_set_group.edge_len;
				nr_unit_size = sizeof(struct sensor_ee_level);
				nr_blk_id[ISP_BLK_EDGE_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_UV_CDN:
			case ISP_BLK_UV_CDN_V1:
			{
				isp_blk_nr_type = ISP_BLK_CDN_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.cdn);
				nr_set_size = fix_data_ptr->nr.nr_set_group.cdn_len;
				nr_unit_size = sizeof(struct sensor_uv_cdn_level);
				nr_blk_id[ISP_BLK_CDN_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_UV_POSTCDN:
			case ISP_BLK_UV_POSTCDN_V1:
			{
				isp_blk_nr_type = ISP_BLK_POSTCDN_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.postcdn);
				nr_set_size = fix_data_ptr->nr.nr_set_group.postcdn_len;
				nr_unit_size = sizeof(struct sensor_uv_postcdn_level);
				nr_blk_id[ISP_BLK_POSTCDN_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_IIRCNR_IIR:
			case ISP_BLK_IIRCNR_IIR_V1:
			{
				isp_blk_nr_type = ISP_BLK_IIRCNR_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.iircnr);
				nr_set_size = fix_data_ptr->nr.nr_set_group.iircnr_len;
				nr_unit_size = sizeof(struct sensor_iircnr_level);
				nr_blk_id[ISP_BLK_IIRCNR_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_YUV_NOISEFILTER:
			case ISP_BLK_YUV_NOISEFILTER_V1:
			{
				isp_blk_nr_type = ISP_BLK_YUV_NOISEFILTER_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.yuv_noisefilter);
				nr_set_size = fix_data_ptr->nr.nr_set_group.yuv_noisefilter_len;
				nr_unit_size = sizeof(struct sensor_yuv_noisefilter_level);
				nr_blk_id[ISP_BLK_YUV_NOISEFILTER_T] = src_header[j].block_id;
				break;
			}
			case ISP_BLK_CNR2:
			case ISP_BLK_CNR2_V1:
			{
				isp_blk_nr_type = ISP_BLK_CNR2_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.cnr2);
				nr_set_size = fix_data_ptr->nr.nr_set_group.cnr2_len;
				nr_unit_size = sizeof(struct sensor_cnr_level);
				nr_blk_id[ISP_BLK_CNR2_T] = src_header[j].block_id;
				break;
			}
#ifdef CONFIG_ISP_2_5
			case DCAM_BLK_3DNR_PRE:
			{
				isp_blk_nr_type = ISP_BLK_3DNR_PRE_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.nr3d_pre);
				nr_set_size = fix_data_ptr->nr.nr_set_group.nr3d_pre_len;
				nr_unit_size = sizeof(struct sensor_3dnr_level);
				nr_blk_id[ISP_BLK_3DNR_PRE_T] = DCAM_BLK_3DNR_PRE;
				break;
			}
			case DCAM_BLK_3DNR_CAP:
			{
				isp_blk_nr_type = ISP_BLK_3DNR_CAP_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.nr3d_cap);
				nr_set_size = fix_data_ptr->nr.nr_set_group.nr3d_cap_len;
				nr_unit_size = sizeof(struct sensor_3dnr_level);
				nr_blk_id[ISP_BLK_3DNR_CAP_T] = DCAM_BLK_3DNR_CAP;
				break;
			}
#endif
#ifdef CONFIG_ISP_2_6
			case ISP_BLK_3DNR:
			{
				isp_blk_nr_type = ISP_BLK_3DNR_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.nr3d);
				nr_set_size = fix_data_ptr->nr.nr_set_group.nr3d_len;
				nr_unit_size = sizeof(struct sensor_3dnr_level);
				nr_blk_id[ISP_BLK_3DNR_T] = ISP_BLK_3DNR;
				break;
			}
			case ISP_BLK_SW3DNR:
			{
				isp_blk_nr_type = ISP_BLK_SW3DNR_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.sw_3dnr);
				nr_set_size = fix_data_ptr->nr.nr_set_group.sw_3dnr_len;
				nr_unit_size = sizeof(struct sensor_sw3dnr_level);
				nr_blk_id[ISP_BLK_SW3DNR_T] = ISP_BLK_SW3DNR;
				break;
			}
			case DCAM_BLK_PPE:
			{
				isp_blk_nr_type = ISP_BLK_PPE_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.ppe);
				nr_set_size = fix_data_ptr->nr.nr_set_group.ppe_len;
				nr_unit_size = sizeof(struct sensor_ppe_level);
				nr_blk_id[ISP_BLK_PPE_T] = DCAM_BLK_PPE;
				break;
			}
			case ISP_BLK_IMBALANCE:
			{
				isp_blk_nr_type = ISP_BLK_IMBALANCEE_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.imblance);
				nr_set_size = fix_data_ptr->nr.nr_set_group.imblance_len;
				nr_unit_size = sizeof(struct sensor_nlm_imbalance_level);
				nr_blk_id[ISP_BLK_IMBALANCEE_T] = ISP_BLK_IMBALANCE;
				break;
			}
			case ISP_BLK_LTM:
			{
				isp_blk_nr_type = ISP_BLK_LTM_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.ltm);
				nr_set_size = fix_data_ptr->nr.nr_set_group.ltm_len;
				nr_unit_size = sizeof(struct sensor_ltm_level);
				nr_blk_id[ISP_BLK_LTM_T] = ISP_BLK_LTM;
				break;
			}
#endif
#endif

			default:
			{
				cmr_s32 ret;
				struct nr_set_group_unit *nr_ptr;

				nr_ptr = (struct nr_set_group_unit *)&(fix_data_ptr->nr.nr_set_group);

				ret = check_nr_blks(src_header[j].block_id, &isp_blk_nr_type, &nr_unit_size);
				if (ret == 0) {
					nr_set_addr = (intptr_t)nr_ptr[isp_blk_nr_type].nr_ptr;
					nr_set_size = nr_ptr[isp_blk_nr_type].nr_len;
					nr_blk_id[isp_blk_nr_type] = src_header[j].block_id;

					dst_blk_data = (struct isp_pm_nr_simple_header_param *)dst_data_ptr;
					memset((void *)dst_blk_data, 0x00, sizeof(struct isp_pm_nr_simple_header_param));

					dst_blk_data->level_number = nr_level_number_ptr->nr_level_map[isp_blk_nr_type];
					dst_blk_data->default_strength_level = nr_default_level_ptr->nr_level_map[isp_blk_nr_type];
					dst_blk_data->nr_mode_setting = multi_nr_flag;
					dst_blk_data->multi_nr_map_ptr = (cmr_uint *)&(nr_scene_map_ptr->nr_scene_map[0]);
					dst_blk_data->param_ptr = (cmr_uint *)nr_set_addr;

					extend_offset += sizeof(struct isp_pm_nr_simple_header_param);
					dst_header[j].size = sizeof(struct isp_pm_nr_simple_header_param);

					nr_data_len[isp_blk_nr_type] = nr_set_size;
					nr_set_size = nr_unit_size * dst_blk_data->level_number;
					nr_scene_map = nr_scene_map_ptr->nr_scene_map[src_mod_ptr->mode_id];
					for (nr_scene_id = ISP_SCENEMODE_AUTO; nr_scene_id < ISP_SCENEMODE_MAX; nr_scene_id++) {
						if ((nr_scene_map & (1 << nr_scene_id)) == 0)
							continue;
						ISP_LOGV("blk id 0x%x, mode %d, scene_id %d\n", src_header[j].block_id, src_mod_ptr->mode_id, nr_scene_id);
						if (dump_nrdata)
							debug_save_nr_data((void *)(nr_set_addr + nr_mode_offset[isp_blk_nr_type]),
								nr_set_size,
								nr_unit_size,
								dst_blk_data->level_number,
								isp_blk_nr_type,
								sensor_name, src_mod_ptr->mode_id, nr_scene_id);
						nr_mode_offset[isp_blk_nr_type] += nr_set_size;
					}
				}
				break;
			}
			}
#if 0
			if (src_header[j].block_id == DCAM_BLK_RGB_DITHER
				|| src_header[j].block_id == DCAM_BLK_BPC_V1
				|| src_header[j].block_id == DCAM_BLK_PPE
				|| src_header[j].block_id == DCAM_BLK_RGB_AFM_V1
				|| src_header[j].block_id == ISP_BLK_GRGB_V1
				|| src_header[j].block_id == ISP_BLK_CFA_V1
				|| src_header[j].block_id == ISP_BLK_UVDIV_V1
				|| src_header[j].block_id == ISP_BLK_3DNR
				|| src_header[j].block_id == ISP_BLK_SW3DNR
				|| src_header[j].block_id == ISP_BLK_YUV_PRECDN_V1
				|| src_header[j].block_id == ISP_BLK_YNR_V1
				|| src_header[j].block_id == ISP_BLK_EE_V1
				|| src_header[j].block_id == ISP_BLK_UV_CDN_V1
				|| src_header[j].block_id == ISP_BLK_UV_POSTCDN_V1
				|| src_header[j].block_id == ISP_BLK_IIRCNR_IIR_V1
				|| src_header[j].block_id == ISP_BLK_YUV_NOISEFILTER_V1
				|| src_header[j].block_id == ISP_BLK_IMBALANCE
				|| src_header[j].block_id == ISP_BLK_LTM
				|| src_header[j].block_id == ISP_BLK_CNR2_V1
				/* sharkl3 NR blocks */
				|| src_header[j].block_id == DCAM_BLK_RGB_AFM
				|| src_header[j].block_id == DCAM_BLK_BPC
				|| src_header[j].block_id == ISP_BLK_RGB_DITHER
				|| src_header[j].block_id == ISP_BLK_GRGB
				|| src_header[j].block_id == ISP_BLK_UVDIV
				|| src_header[j].block_id == ISP_BLK_CFA
				|| src_header[j].block_id == DCAM_BLK_3DNR_PRE
				|| src_header[j].block_id == DCAM_BLK_3DNR_CAP
				|| src_header[j].block_id == ISP_BLK_YUV_PRECDN
				|| src_header[j].block_id == ISP_BLK_YNR
				|| src_header[j].block_id == ISP_BLK_EDGE
				|| src_header[j].block_id == ISP_BLK_UV_CDN
				|| src_header[j].block_id == ISP_BLK_UV_POSTCDN
				|| src_header[j].block_id == ISP_BLK_IIRCNR_IIR
				|| src_header[j].block_id == ISP_BLK_YUV_NOISEFILTER
				|| src_header[j].block_id == ISP_BLK_CNR2) {

				dst_blk_data = (struct isp_pm_nr_simple_header_param *)dst_data_ptr;
				memset((void *)dst_blk_data, 0x00, sizeof(struct isp_pm_nr_simple_header_param));

				dst_blk_data->level_number = nr_level_number_ptr->nr_level_map[isp_blk_nr_type];
				dst_blk_data->default_strength_level = nr_default_level_ptr->nr_level_map[isp_blk_nr_type];
				dst_blk_data->nr_mode_setting = multi_nr_flag;
				dst_blk_data->multi_nr_map_ptr = (cmr_uint *)&(nr_scene_map_ptr->nr_scene_map[0]);
				dst_blk_data->param_ptr = (cmr_uint *)nr_set_addr;

				extend_offset += sizeof(struct isp_pm_nr_simple_header_param);
				dst_header[j].size = sizeof(struct isp_pm_nr_simple_header_param);

				nr_data_len[isp_blk_nr_type] = nr_set_size;
				nr_set_size = nr_unit_size * dst_blk_data->level_number;
				nr_scene_map = nr_scene_map_ptr->nr_scene_map[src_mod_ptr->mode_id];
				for (nr_scene_id = ISP_SCENEMODE_AUTO; nr_scene_id < ISP_SCENEMODE_MAX; nr_scene_id++) {
					if ((nr_scene_map & (1 << nr_scene_id)) == 0)
						continue;
					ISP_LOGV("blk id 0x%x, mode %d, scene_id %d\n", src_header[j].block_id, src_mod_ptr->mode_id, nr_scene_id);
					if (dump_nrdata)
						debug_save_nr_data((void *)(nr_set_addr + nr_mode_offset[isp_blk_nr_type]),
							nr_set_size,
							nr_unit_size,
							dst_blk_data->level_number,
							isp_blk_nr_type,
							sensor_name, src_mod_ptr->mode_id, nr_scene_id);
					nr_mode_offset[isp_blk_nr_type] += nr_set_size;
				}
			}
#endif

		}

		/* ISP_BLK_HSV & ISP_BLK_HSV_NEW only one is required */
		/* If there is new hsv, old one will be discarded */
		if (hsv_header && hsv_new_header) {
			hsv_header->block_id = 0;
			ISP_LOGD("discard hsv because of hsv_new\n");
		}

		if (max_num < src_mod_ptr->block_num)
			max_num = src_mod_ptr->block_num;

		dst_mod_ptr->block_num = src_mod_ptr->block_num;
		dst_mod_ptr->mode_id = src_mod_ptr->mode_id;
		dst_mod_ptr->resolution.w = src_mod_ptr->width;
		dst_mod_ptr->resolution.h = src_mod_ptr->height;
		dst_mod_ptr->fps = src_mod_ptr->fps;

		memcpy((void *)dst_mod_ptr->mode_name,
			(void *)src_mod_ptr->mode_name, sizeof(src_mod_ptr->mode_name));

		ISP_LOGV("mode = %d, mode_name = %s, param modify_time : %d",
			i, src_mod_ptr->mode_name, src_mod_ptr->reserved[0]);
	}

	/* check if NR block data is legal or not. If not, discard it */
	for (i = 0; i < ISP_BLK_NR_MAX; i++) {
		if (nr_blk_id[i] == 0)
			continue;
		if (nr_blk_id[i] == ISP_BLK_NLM_V1 || nr_blk_id[i] == DCAM_BLK_NLM || nr_blk_id[i] == ISP_BLK_NLM_V2) {
			if ((nr_data_len[i] == nr_mode_offset[i]) &&
				(nr_data_len[ISP_BLK_VST_T] == nr_mode_offset[ISP_BLK_VST_T]) &&
				(nr_data_len[ISP_BLK_IVST_T] == nr_mode_offset[ISP_BLK_IVST_T]))
				continue;
			else
				ISP_LOGE("NLM data mismatch, nlm %d %d, vst %d %d, ivst %d %d\n",
					nr_data_len[i], nr_mode_offset[i],
					nr_data_len[ISP_BLK_VST_T], nr_mode_offset[ISP_BLK_VST_T],
					nr_data_len[ISP_BLK_IVST_T], nr_mode_offset[ISP_BLK_IVST_T]);
		} else if (nr_data_len[i] == nr_mode_offset[i]) {
			continue;
		} else {
			ISP_LOGE("nr blk 0x%04x data mismatch: %d %d\n", nr_blk_id[i], nr_data_len[i], nr_mode_offset[i]);
		}

		/* data size mismatched, discard the block in all mode */
		for (k = 0; k < ISP_TUNE_MODE_MAX; k++) {
			dst_mod_ptr = pm_cxt_ptr->tune_mode_array[k];
			if (dst_mod_ptr == PNULL)
				continue;
			dst_header = (struct isp_pm_block_header *)dst_mod_ptr->header;
			for (j = 0; j < dst_mod_ptr->block_num; j++) {
				if (dst_header[j].block_id == nr_blk_id[i]) {
					dst_header[j].block_id = 0;
					ISP_LOGE("discard NR blk 0x%04x for mode %d\n", nr_blk_id[i], k);
					break;
				}
			}
		}
	}

	size = ISP_TUNE_BLOCK_MAX * sizeof(struct isp_pm_param_data);
	ISP_LOGD("temp data size: %d\n", size);

	pm_cxt_ptr->getting_data_ptr[0] = (struct isp_pm_param_data *)malloc(size);
	if (PNULL == pm_cxt_ptr->getting_data_ptr[0]) {
		ISP_LOGE("fail to malloc getting_data_ptr[0]");
		rtn = ISP_ERROR;
		goto init_param_list_error_exit;
	}
	memset((void *)pm_cxt_ptr->getting_data_ptr[0], 0x00, size);

	pm_cxt_ptr->getting_data_ptr[1] = (struct isp_pm_param_data *)malloc(size);
	if (PNULL == pm_cxt_ptr->getting_data_ptr[1]) {
		ISP_LOGE("fail to malloc getting_data_ptr[1]");
		rtn = ISP_ERROR;
		goto init_param_list_error_exit;
	}
	memset((void *)pm_cxt_ptr->getting_data_ptr[1], 0x00, size);

	ISP_LOGV("isp_pm_param_list_init : done");

	return rtn;

init_param_list_error_exit:

	isp_pm_mode_list_deinit((cmr_handle)pm_cxt_ptr);

	return rtn;
}


static cmr_s32 isp_pm_param_init_and_update(cmr_handle handle,
	struct isp_pm_init_input *input, struct isp_pm_init_output *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 set_id;
	cmr_u32 img_w = 960, img_h = 720;
	struct isp_pm_mode_param *common;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	rtn = isp_pm_mode_list_init(handle, input, output);
	if (rtn)
		return rtn;
	common = pm_cxt_ptr->tune_mode_array[ISP_MODE_ID_COMMON];
	if (common) {
		img_w = common->resolution.w;
		img_h = common->resolution.h;
	}
	ISP_LOGD("img size %d %d\n", img_w, img_h);

	if (pm_cxt_ptr->param_source != ISP_PARAM_FROM_TOOL) {
		memset(pm_cxt_ptr->cxt_array,  0, sizeof(pm_cxt_ptr->cxt_array));
		memset(pm_cxt_ptr->blocks_param, 0, sizeof(pm_cxt_ptr->blocks_param));

		/* init one param set for preview */
		if (pm_cxt_ptr->param_search_list) {
			isp_pm_get_all_blocks(handle,
				&pm_cxt_ptr->blocks_param[PARAM_SET0],
				WORKMODE_PREVIEW,
				SCENEMODE_NROMAL,
				DEFMODE_DEFAULT,
				img_w, img_h, 1);
			isp_pm_get_all_blocks(handle,
				&pm_cxt_ptr->blocks_param[PARAM_SET1],
				WORKMODE_CAPTURE,
				SCENEMODE_NROMAL,
				DEFMODE_DEFAULT,
				img_w, img_h, 1);
		} else {
			isp_pm_get_all_blocks_compatible(handle,
				&pm_cxt_ptr->blocks_param[PARAM_SET0],
				ISP_MODE_ID_COMMON,
				WORKMODE_PREVIEW,
				SCENEMODE_NROMAL,
				DEFMODE_DEFAULT,
				img_w, img_h, 1);
			isp_pm_get_all_blocks_compatible(handle,
				&pm_cxt_ptr->blocks_param[PARAM_SET1],
				ISP_MODE_ID_COMMON,
				WORKMODE_CAPTURE,
				SCENEMODE_NROMAL,
				DEFMODE_DEFAULT,
				img_w, img_h, 1);
		}
		for (set_id = 0; set_id < PARAM_SET_MAX; set_id++) {
			isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
			isp_cxt_ptr->is_validate = 1;
			isp_cxt_ptr->mode_id = ISP_MODE_ID_COMMON;
			ISP_LOGD("pm set %d, context %p\n", set_id, isp_cxt_ptr);
			rtn |= isp_pm_context_init((cmr_handle)pm_cxt_ptr, set_id);
		}
		if (rtn)
			goto init_error;
	} else {
		cmr_u32 img_w;
		cmr_u32 img_h;
		enum tuning_mode mode;
		enum tuning_scene_mode scene;
		enum tuning_custom define;
		struct isp_pm_blocks_param *blk_param_ptr;
		for (set_id = 0; set_id < PARAM_SET_MAX; set_id++) {
			isp_cxt_ptr = &pm_cxt_ptr->cxt_array[set_id];
			if (isp_cxt_ptr->is_validate == 0)
				continue;
			blk_param_ptr = &pm_cxt_ptr->blocks_param[set_id];
			mode = blk_param_ptr->mode;
			scene = blk_param_ptr->scene;
			define = blk_param_ptr->cus_define;
			img_w = blk_param_ptr->resolution.w;
			img_h = blk_param_ptr->resolution.h;
			ISP_LOGD("update %d from tools (%d %d %d %d %d)\n", set_id, mode, scene, define, img_w, img_h);
			memset(blk_param_ptr, 0, sizeof(struct isp_pm_blocks_param));
			if (pm_cxt_ptr->param_search_list)
				isp_pm_get_all_blocks(handle, blk_param_ptr, mode, scene, define, img_w, img_h, 1);
			else
				isp_pm_get_all_blocks_compatible(handle,
					blk_param_ptr, isp_cxt_ptr->mode_id,
					mode, scene, define, img_w, img_h, 1);
			rtn = isp_pm_context_init((cmr_handle)pm_cxt_ptr, set_id);
			if (rtn)
				goto init_error;
		}
	}

	ISP_LOGD("done.\n");
	return ISP_SUCCESS;

init_error:
	ISP_LOGE("failed\n");
	return rtn;
}

cmr_handle isp_pm_init(struct isp_pm_init_input *input, struct isp_pm_init_output *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = PNULL;

	if ((PNULL == input) || (PNULL == output)) {
		ISP_LOGE("fail to get valid param : input = %p, output = %p", input, output);
		goto init_error_exit;
	}

	rtn = isp_pm_raw_para_update_from_file(input->sensor_raw_info_ptr);

	pm_cxt_ptr = (struct isp_pm_context *)malloc(sizeof(struct isp_pm_context));
	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to malloc pm_cxt_ptr");
		goto init_error_exit;
	}
	memset((void *)pm_cxt_ptr, 0x00, sizeof(struct isp_pm_context));

	pm_cxt_ptr->magic_flag = ISP_PM_MAGIC_FLAG;
	pthread_mutex_init(&pm_cxt_ptr->pm_mutex, NULL);

	pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
	rtn = isp_pm_param_init_and_update((cmr_handle)pm_cxt_ptr, input, output);
	pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to isp_pm_param_list_init");
		goto init_error_exit;
	}

	ISP_LOGI("done\n");
	return (cmr_handle)pm_cxt_ptr;

init_error_exit:
	if (PNULL != pm_cxt_ptr) {
		isp_pm_context_deinit((cmr_handle)pm_cxt_ptr);
		isp_pm_mode_list_deinit((cmr_handle)pm_cxt_ptr);
		pthread_mutex_destroy(&pm_cxt_ptr->pm_mutex);
		free(pm_cxt_ptr);
		pm_cxt_ptr = PNULL;
	}
	return PNULL;
}

cmr_s32 isp_pm_ioctl(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		return rtn;
	}

	/* Be careful to use LOCK/UNLOCK. They should be called in pair */
	if (cmd == ISP_PM_CMD_LOCK) {
		pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
		return rtn;
	} else if (cmd == ISP_PM_CMD_UNLOCK) {
		pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
		return rtn;
	}

	switch ((cmd & isp_pm_cmd_mask)) {
	case ISP_PM_CMD_SET_BASE:
		pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
		rtn = isp_pm_set_param((cmr_handle)pm_cxt_ptr, cmd, input, output);
		pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
		break;
	case ISP_PM_CMD_GET_BASE:
	case ISP_PM_CMD_GET_THIRD_PART_BASE:
		pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
		rtn = isp_pm_get_param((cmr_handle)pm_cxt_ptr, cmd, input, output);
		pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
		break;
	default:
		break;
	}

	return rtn;
}

cmr_s32 isp_pm_update(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == input) {
		ISP_LOGE("fail to get valid input param");
		rtn = ISP_ERROR;
		return rtn;
	}

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		return rtn;
	}

	if (ISP_PM_CMD_UPDATE_ALL_PARAMS == cmd) {
		pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
		rtn = isp_pm_param_init_and_update((cmr_handle)pm_cxt_ptr, input, output);
		pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
	}

	ISP_LOGI("done\n");
	return rtn;
}

cmr_s32 isp_pm_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		rtn = ISP_ERROR;
		return rtn;
	}

	pthread_mutex_destroy(&pm_cxt_ptr->pm_mutex);
	isp_pm_context_deinit((cmr_handle)pm_cxt_ptr);
	isp_pm_mode_list_deinit((cmr_handle)pm_cxt_ptr);
	free(pm_cxt_ptr);
	pm_cxt_ptr = PNULL;

	ISP_LOGI("done\n");
	return rtn;
}
