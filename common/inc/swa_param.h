/*-------------------------------------------------------------------*/
/*  Copyright(C) 2020 by Unisoc                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/

#ifndef _SWA_PARAM_API_H_
#define _SWA_PARAM_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "cmr_types.h"


/*========= for HDR ========*/

#define HDR_CAP_MAX 3

struct swa_hdr_param {
	uint32_t pic_w;
	uint32_t pic_h;
	uint32_t fmt;
	float ev[HDR_CAP_MAX];
};

/*========= for HDR end  ========*/



/*========= for MFNR ========*/

/* sw 3DNR param */
/* used to pass sw3dnr param from tuning array to HAL->3dnr adapt
  * must keep consistent with struct ( sensor_sw3dnr_level) in sensor_raw_xxx.h
  * should not be modified except sensor_raw_xxx.h changes corresponding structure */
struct isp_mfnr_info {
	cmr_s32 threshold[4];
	cmr_s32 slope[4];
	cmr_u16 searchWindow_x;
	cmr_u16 searchWindow_y;
	cmr_s32 recur_str;
	cmr_s32 match_ratio_sad;
	cmr_s32 match_ratio_pro;
	cmr_s32 feat_thr;
	cmr_s32 zone_size;
	cmr_s32 luma_ratio_high;
	cmr_s32 luma_ratio_low;
	cmr_s32 reserverd[16];
};

struct isp_sw3dnr_info {
	cmr_s32 threshold[4];
	cmr_s32 slope[4];
	cmr_u16 searchWindow_x;
	cmr_u16 searchWindow_y;
	cmr_s32 recur_str;
	cmr_s32 match_ratio_sad;
	cmr_s32 match_ratio_pro;
	cmr_s32 feat_thr;
	cmr_s32 zone_size;
	cmr_s32 luma_ratio_high;
	cmr_s32 luma_ratio_low;
	cmr_s32 reserverd[16];
};
/*========= for MFNR  end ========*/


/*========= for YNRs ========*/
struct isp_sw_ynrs_params {
	uint8_t lumi_thresh[2];
	cmr_u8 gf_rnr_ratio[5];
	cmr_u8 gf_addback_enable[5];
	cmr_u8 gf_addback_ratio[5];
	cmr_u8 gf_addback_clip[5];
	cmr_u16 Radius_factor;
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 gf_epsilon[5][3];
	cmr_u16 gf_enable[5];
	cmr_u16 gf_radius[5];
	cmr_u16 gf_rnr_offset[5];
	cmr_u16 bypass;
	cmr_u8 reserved[2];
};

struct isp_ynrs_info {
	cmr_u16 bypass;
	cmr_u16 radius_base;
	struct isp_sw_ynrs_params ynrs_param;
};
/*========= for YNRs end ========*/


/*========= for CNR2.0 =========*/
struct isp_sw_filter_weights
{
	cmr_u8 distWeight[9];
	cmr_u8 rangWeight[128];
};

struct isp_sw_cnr2_info {
	cmr_u8 filter_en[4];
	cmr_u8 rangTh[4][2];
	struct isp_sw_filter_weights weight[4][2];
};

struct isp_cnr2_info {
	cmr_u32 bypass;
	struct isp_sw_cnr2_info param;
};
/*========= for CNR2.0 end =========*/


/*========= for CNR3.0 =========*/
#define CNR3_LAYER_NUM 5

struct isp_sw_multilayer_param {
	cmr_u8 lowpass_filter_en;
	cmr_u8 denoise_radial_en;
	cmr_u8 order[3];
	cmr_u16 imgCenterX;
	cmr_u16 imgCenterY;
	cmr_u16 slope;
	cmr_u16 baseRadius;
	cmr_u16 minRatio;
	cmr_u16 luma_th[2];
	float sigma[3];
};

struct isp_sw_cnr3_info {
	cmr_u16 bypass;
	cmr_u16 baseRadius;
	struct isp_sw_multilayer_param param_layer[CNR3_LAYER_NUM];
};
/*========= for CNR3.0 end =========*/


/*========= for DRE   =========*/
struct isp_predre_param {
	cmr_s32 enable;
	cmr_s32 imgKey_setting_mode;
	cmr_s32 tarNorm_setting_mode;
	cmr_s32 target_norm;
	cmr_s32 imagekey;
	cmr_s32 min_per;
	cmr_s32 max_per;
	cmr_s32 stat_step;
	cmr_s32 low_thresh;
	cmr_s32 high_thresh;
	cmr_s32 tarCoeff;
};

struct isp_postdre_param {
	cmr_s32 enable;
	cmr_s32 strength;
	cmr_s32 texture_counter_en;
	cmr_s32 text_point_thres;
	cmr_s32 text_prop_thres;
	cmr_s32 tile_num_auto;
	cmr_s32 tile_num_x;
	cmr_s32 tile_num_y;
};

struct isp_dre_level {
	struct isp_predre_param predre_param;
	struct isp_postdre_param postdre_param;
};
/*========= for DRE end =========*/

/*========= for DRE_pro  =========*/
struct isp_predre_pro_param {
	cmr_s32 enable;
	cmr_s32 imgKey_setting_mode;
	cmr_s32 tarNorm_setting_mode;
	cmr_s32 target_norm;
	cmr_s32 imagekey;
	cmr_s32 min_per;
	cmr_s32 max_per;
	cmr_s32 stat_step ;
	cmr_s32 low_thresh;
	cmr_s32 high_thresh;
	cmr_s32 uv_gain_ratio;
	cmr_s32 tarCoeff;
};

struct isp_postdre_pro_param {
	cmr_s32 enable;
	cmr_s32 strength;
	cmr_s32 texture_counter_en;
	cmr_s32 text_point_thres;
	cmr_s32 text_prop_thres;
	cmr_s32 tile_num_auto;
	cmr_s32 tile_num_x;
	cmr_s32 tile_num_y;
	cmr_s32 text_point_alpha;
};

struct isp_dre_pro_level {
	struct isp_predre_pro_param predre_pro_param;
	struct isp_postdre_pro_param postdre_pro_param;
};
/*========= for DRE_pro end =========*/



/*========= for filter ========*/
struct swa_filter_param {
	uint32_t version;
	uint32_t filter_type;
	uint32_t orientation;
	uint32_t flip_on;
	uint32_t is_front;
	uint32_t pic_w;
	uint32_t pic_h;
};
/*========= for filter end ========*/



/*========= for face beauty ========*/
enum {
	ISP_FB_SKINTONE_DEFAULT,
	ISP_FB_SKINTONE_YELLOW,
	ISP_FB_SKINTONE_WHITE,
	ISP_FB_SKINTONE_BLACK,
	ISP_FB_SKINTONE_INDIAN,
	ISP_FB_SKINTONE_NUM
};

struct isp_fb_level {
	cmr_u8 skinSmoothLevel[11];
	cmr_u8 skinSmoothDefaultLevel;
	cmr_u8 skinTextureHiFreqLevel[11];
	cmr_u8 skinTextureHiFreqDefaultLevel;
	cmr_u8 skinTextureLoFreqLevel[11];
	cmr_u8 skinTextureLoFreqDefaultLevel;
	cmr_u8 skinSmoothRadiusCoeff[11];
	cmr_u8 skinSmoothRadiusCoeffDefaultLevel;
	cmr_u8 skinBrightLevel[11];
	cmr_u8 skinBrightDefaultLevel;
	cmr_u8 largeEyeLevel[11];
	cmr_u8 largeEyeDefaultLevel;
	cmr_u8 slimFaceLevel[11];
	cmr_u8 slimFaceDefaultLevel;
	cmr_u8 skinColorLevel[11];
	cmr_u8 skinColorDefaultLevel;
	cmr_u8 lipColorLevel[11];
	cmr_u8 lipColorDefaultLevel;
};

struct isp_fb_param {
	cmr_u8 removeBlemishFlag;
	cmr_u8 blemishSizeThrCoeff;
	cmr_u8 skinColorType;
	cmr_u8 lipColorType;
	struct isp_fb_level fb_layer;
};

struct isp_fb_param_info {
	struct isp_fb_param fb_param[ISP_FB_SKINTONE_NUM];
};

struct isp_beauty_levels {
	uint8_t blemishLevel;
	uint8_t smoothLevel;
	uint8_t skinColor;
	uint8_t skinLevel;
	uint8_t brightLevel;
	uint8_t lipColor;
	uint8_t lipLevel;
	uint8_t slimLevel;
	uint8_t largeLevel;
	uint8_t reserved[3];
};

struct isp_fb_info {
	uint16_t bypass;
	uint16_t param_valid;
	struct isp_fb_param_info param;
	struct isp_beauty_levels levels;
};
/*========= for face beauty end ========*/


/*========= for face detect ========*/
struct isp_face_info {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
	cmr_u32 brightness;
	cmr_s32 pose;
	cmr_s32 angle;
	cmr_s32 yaw_angle;
	cmr_s32 roll_angle;
	cmr_u32 score;
	cmr_u32 id;
};

struct isp_face_area {
	cmr_u16 type;
	cmr_u16 face_num;
	cmr_u16 frame_width;
	cmr_u16 frame_height;
	struct isp_face_info face_info[10];
	cmr_u32 frame_id;
	cmr_s64 timestamp;
};
/*========= for face detect end ========*/

struct swa_filter_info {
	uint32_t version;
	uint32_t filter_type;
};

struct swa_watermark_info {
	int flag;
	char time_text[32];
};

struct swa_init_data {
	uint32_t cam_id;
	struct img_size sensor_max;
	struct img_size sensor_size;
	struct img_size frame_size;
	struct img_rect frame_crop;
	uint32_t sn_fmt;
	uint32_t pic_fmt;
	uint32_t frm_total_num;
	uint32_t ae_again;
	void *pri_data;
};

struct swa_common_info {
	int32_t cam_id;
	int32_t dgain;
	int32_t again;
	int32_t total_gain;
	int32_t sn_gain;
	int32_t exp_time;
	int32_t bv;
	int32_t iso;
	int32_t ct;
	uint32_t angle;
	uint32_t rotation;
	uint32_t flip_on;
	uint32_t is_front;
	float zoom_ratio;
};

struct swa_frame_param {
	struct swa_common_info common_param;
	struct swa_hdr_param hdr_param;
	struct swa_filter_info filter_param;
	struct swa_watermark_info wm_param;
	struct isp_ynrs_info ynrs_info;
	struct isp_cnr2_info cnr2_info;
	struct isp_sw_cnr3_info cnr3_info;
	struct isp_dre_level dre_param;
	struct isp_dre_pro_level dre_pro_param;
	struct isp_face_area face_param;
	struct isp_fb_info fb_info;
};


#ifdef __cplusplus
}
#endif
#endif
