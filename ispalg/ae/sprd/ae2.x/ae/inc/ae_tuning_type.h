/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_TUNING_TYPE_H_
#define _AE_TUNING_TYPE_H_
#include "ae_common.h"

#define AE_CVGN_NUM  4
#define AE_CFG_NUM 8
#define MULAES_CFG_NUM AE_CFG_NUM
#define REGION_CFG_NUM AE_CFG_NUM
#define FLAT_CFG_NUM AE_CFG_NUM
#define FACE_CFG_NUM AE_CFG_NUM
#define ABL_CFG_NUM AE_CFG_NUM
#define PCP_CFG_NUM AE_CFG_NUM
#define HM_CFG_NUM AE_CFG_NUM
#define AI_CFG_NUM AE_CFG_NUM
#define AE_EV_LEVEL_NUM 16
#define AE_FLICKER_NUM 2
#define AE_EXP_GAIN_TABLE_SIZE 512
#define AE_OFFSET_NUM 20
#define ISP_BAYER_CHNL_NUM 4
#define ISP_HIST_BIN_MX 1024
#define BV_STEP_NUM 3
#define AE_ISO_NUM_NEW 8
#define AE_SCENE_NUM   8
#define AE_WEIGHT_TABLE_NUM 3
#define AE_WEIGHT_TABLE_SIZE	1024
#define ISP_BAYER_CHNL_NUM 4
#define ISP_HIST_BIN_MX 1024
#define BV_STEP_NUM 3

/*ae common structure define*/
#ifndef FD_AE_PARAM_DEF
#define FD_AE_PARAM_DEF
struct ae1_face {
	cmr_u32 start_x;
	cmr_u32 start_y;
	cmr_u32 end_x;
	cmr_u32 end_y;				/*4 x 4bytes */
	cmr_s32 pose;				/* face pose: frontal, half-profile, full-profile */
	cmr_u32 face_lum;
	cmr_s32 angle;
};

struct ae1_face_info {
	cmr_u16 face_num;
	cmr_u16 reserved;			/*1 x 4bytes */
	cmr_u32 rect[1024];			/*1024 x 4bytes */
	struct ae1_face face_area[20];	/*20 x 5 * 4bytes */
};								/*1125 x 4bytes */

struct ae1_fd_param {
	struct ae1_face_info cur_info;	/*1125 x 4bytes */
	cmr_u8 update_flag;
	cmr_u8 enable_flag;
	cmr_u16 reserved;			/*1 x 4bytes */
	cmr_u16 img_width;
	cmr_u16 img_height;			/*1 x 4bytes */
};
#endif							/*1127 x 4bytes */

/*ae tuning parameter structure define*/
struct ae_stat_req {
	cmr_u32 mode;				//0:normal, 1:G(center area)
	cmr_u32 G_width;			//100:G mode(100x100)
};

struct ae_flash_tuning {
	cmr_u32 exposure_index;
};

struct touch_zone {
	cmr_u32 level_0_weight;
	cmr_u32 level_1_weight;
	cmr_u32 level_1_percent;	//x64
	cmr_u32 level_2_weight;
	cmr_u32 level_2_percent;	//x64
};

struct ae_ev_setting_item {
	cmr_s16 lum_diff;
	cmr_u8 stable_zone_in;
	cmr_u8 stable_zone_out;
};

struct ae_ev_table {
	struct ae_ev_setting_item ev_item[AE_EV_LEVEL_NUM];
	/* number of level */
	cmr_u32 diff_num;
	/* index of default */
	cmr_u32 default_level;
};

struct ae_exp_anti {
	cmr_u32 enable;
	cmr_u8 hist_thr[40];
	cmr_u8 hist_weight[40];
	cmr_u8 pos_lut[256];
	cmr_u8 hist_thr_num;
	cmr_u8 adjust_thr;
	cmr_u8 stab_conter;
	cmr_u8 reserved1;
	cmr_u32 reserved[175];
};

struct ae_auto_iso_tab {
	cmr_u16 tbl[AE_FLICKER_NUM][AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_ev_cali_param {
	cmr_u32 index;
	cmr_u32 lux;
	cmr_u32 lv;
};

struct ae_ev_cali {
	cmr_u32 num;
	cmr_u32 min_lum;			// close all the module of after awb module
	struct ae_ev_cali_param tab[16];	// cali EV sequence is low to high
};

struct ae_convergence_parm {
	cmr_u32 highcount;
	cmr_u32 lowcount;
	cmr_u32 highlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 lowlum_offset_default[AE_OFFSET_NUM];
	cmr_u32 highlum_index[AE_OFFSET_NUM];
	cmr_u32 lowlum_index[AE_OFFSET_NUM];
};

struct ae_exp_gain_table {
	cmr_s32 min_index;
	cmr_s32 max_index;
	cmr_u32 exposure[AE_EXP_GAIN_TABLE_SIZE]; /*512 * 4bytes*/
	cmr_u32 dummy[AE_EXP_GAIN_TABLE_SIZE]; /*512 * 4bytes*/
	cmr_u16 again[AE_EXP_GAIN_TABLE_SIZE]; /*256 * 4bytes*/
	cmr_u16 dgain[AE_EXP_GAIN_TABLE_SIZE]; /*256 * 4bytes*/
};

struct ae_weight_table {
	cmr_u8 weight[AE_WEIGHT_TABLE_SIZE];
};

struct ae_scene_info {
	cmr_u32 enable;
	cmr_u32 scene_mode;
	cmr_u32 target_lum;
	cmr_u32 iso_index;
	cmr_u32 ev_offset;
	cmr_u32 max_fps;
	cmr_u32 min_fps;
	cmr_u32 weight_mode;
	cmr_u8 table_enable;
	cmr_u8 exp_tbl_mode;
	cmr_u16 reserved0;
	cmr_u32 reserved1;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM];
};

struct ae_sensor_cfg {
	cmr_u16 max_gain;			/*sensor max gain */
	cmr_u16 min_gain;			/*sensor min gain */
	cmr_u8 gain_precision;
	cmr_u8 exp_skip_num;
	cmr_u8 gain_skip_num;
	cmr_u8 min_exp_line;
};

struct ae_lv_calibration {
	cmr_u16 lux_value;
	cmr_s16 bv_value;
};

struct ae_flash_tuning_param {
	cmr_u8 skip_num;
	cmr_u8 target_lum;
	cmr_u8 adjust_ratio;		/* 1x --> 32 */
	cmr_u8 reserved;
};

struct ae_thrd_param {
	cmr_s16 thr_up;
	cmr_s16 thr_down;
};

struct ae_param_tmp_001 {
	cmr_u32 version;
	cmr_u32 verify;
	cmr_u32 alg_id;
	cmr_u32 target_lum;
	cmr_u32 target_lum_zone;	// x16
	cmr_u32 convergence_speed;	// x16
	cmr_u32 flicker_index;
	cmr_u32 min_line;
	cmr_u32 start_index;
	cmr_u32 exp_skip_num;
	cmr_u32 gain_skip_num;
	struct ae_stat_req stat_req;
	struct ae_flash_tuning flash_tuning;
	struct touch_zone touch_param;
	struct ae_ev_table ev_table;
};

struct ae_param_tmp_002 {
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
};

struct mulaes_cfg {
	cmr_s16 x_idx;
	cmr_s16 y_lum;
};

struct mulaes_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;			/*1 * 4bytes */
	struct mulaes_cfg cfg[MULAES_CFG_NUM];	/*8 * 4bytes */
};								/*9 * 4bytes */

typedef struct {
	struct ae_range region_thrd[6];	/*u d l r */
	cmr_s16 up_max;
	cmr_s16 dwn_max;
	cmr_s16 vote_region[6];		/*u d l r */
} region_cfg;					/*16 * 4bytes */

struct region_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;			/*1 * 4bytes */
	region_cfg cfg_info[REGION_CFG_NUM];	/*total 8 group: 128 * 4bytes */
	struct ae_piecewise_func input_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func u_out_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func d_out_piecewise;	/*17 * 4bytes */
};								/*180 * 4bytes */

typedef struct {
	cmr_s16 thrd[2];
	cmr_s16 offset[2];
} flat_cfg;						/*2 * 4bytes */

struct flat_tuning_param {
	/*1 * 4bytes */
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 reserved;
	/*flat tune param; total 8 group */
	flat_cfg cfg_info[FLAT_CFG_NUM];	/*16 * 4bytes */
	struct ae_piecewise_func out_piecewise;	/*17 * 4bytes */
	struct ae_piecewise_func in_piecewise;	/*17 * 4bytes */
};								/*51 * 4bytes */

struct face_cfg {
	cmr_s16 x_idx;
	cmr_u8 y_lum;
	cmr_u8 up_limit;
	cmr_u8 down_limit;
	cmr_u8 ratio_block;			//10~100 will trans 0~1
	cmr_u8 ratio_position;		//10~100 will trans 0~1
	cmr_u8 max_with_ratio;		//10~100 will trans 0~1
};

struct face_tuning_param {
	cmr_u8 face_tuning_enable;
	cmr_u8 face_target;			//except to get the face lum
	cmr_u8 face_tuning_lum1;	// scope is [0,256]
	cmr_u8 face_tuning_lum2;	//if face lum > this value, offset will set to be 0
	cmr_u16 cur_offset_weight;	//10~100 will trans 0~1
	cmr_u16 up_face_offset;		//limit up face offset
	cmr_u16 down_face_offset;	//limit down face offset
	cmr_u8 ratio_block;			//10~100 will trans 0~1
	cmr_u8 ratio_position;		//10~100 will trans 0~1
	cmr_u8 max_with_ratio;		//10~100 will trans 0~1
	cmr_u8 num;
	struct face_cfg cfg[FACE_CFG_NUM];	/*8 * 6bytes */
	cmr_u8 face_disappear_count;
	cmr_u8 face_block_threshold;
	cmr_u8 ae_thd;
	cmr_u8 face_low_thd;
	cmr_u8 face_high_thd;
	cmr_u8 aem_4_face;
	cmr_u16 reserved[2];		//?
};

struct face_tuning_param_adv {
	cmr_u8 u4face_trigger_sensitivity1;	//face ae trigger sensitivity lum thrd .The bigger the number, the more sensitive it is.3
	cmr_u8 u4face_trigger_sensitivity2;	//face ae trigger sensitivity num thrd.The small the number, the more sensitive it is.6
	cmr_u8 u4face_trigger_sensitivity3;	//face ae trigger sensitivity num thrd.The small the number, the more sensitive it is.4
	cmr_u8 u4face_trigger_sensitivity4;	//face ae calculate sensitivity num thrd.The small the number, the more calculate it is.3
	cmr_u8 u4face_frame_thrd;		//count for no face detect thrd
	cmr_u8 u4smooth_weight[5];		//face ae offset smooth weight
	cmr_u8 u4abl_face_offset;			//abl up limit offset
	cmr_u8 u4abl_offset_thrd;			//abl up limit offset thrd
	cmr_u8 u4face_roi_ratio;			//ratio for calculate small face roi
	cmr_u8 u4face_weight1;			//small face roi luma weight
	cmr_u8 u4face_weight2;			//big face roi luma weight
	cmr_u8 u4small_weight_thrd;		//change face small roi from abl weight thrd
	cmr_u8 u4small_weight_raise;		//change face small roi from abl weight value
	cmr_u8 u4offset_ratio_thrd;		//change face offset from roi size thrd
	cmr_u8 u4offset_ratio_value;		//change face offset from roi size value
	/*face id unlock */
	cmr_u8 u4fdunlock_enable;
	cmr_u8 u4fdunlock_face_target;	//face target in face id unlock mode
	cmr_u8 u4fdunlock_face_weight1;	//small face roi luma weight in face id unlock mode
	cmr_u8 u4fdunlock_face_weight2;	//big face roi luma weight in face id unlock mode
	cmr_u8 u4fdunlock_face_roi_ratio;	//ratio for calculate small face roi in face id unlock mode
	cmr_u8 u4fdunlock_unlinear_cancel;	//cancel face offset with unlinear in face id unlock mode
	cmr_u8 u4fdunlock_range_high;
	cmr_u8 u4fdunlock_range_low;
	cmr_u8 reserved1;
	cmr_u32 reserved[43];
};

struct ae_touch_param {
	cmr_u8 win2_weight;			//for touch ae
	cmr_u8 enable;				//for touch ae
	cmr_u8 win1_weight;			//for touch ae
	cmr_u8 max_offset;			//for touch ae
	struct ae_size touch_tuning_win;	//for touch ae
};

struct ae_hdr_tuning_param {
	cmr_s32 ev_minus_offset;
	cmr_s32 ev_plus_offset;
};

struct ae_flash_swith_param {
	cmr_s32 flash_open_thr;
	cmr_s32 flash_close_thr;
};

struct ae_flash_control_param {
	cmr_u8 pre_flash_skip;
	cmr_u8 aem_effect_delay;
	cmr_u8 pre_open_count;
	cmr_u8 pre_close_count;
	cmr_u8 main_set_count;
	cmr_u8 main_capture_count;
	cmr_u8 main_flash_notify_delay;
	cmr_u8 flash_frameskip_count;
};

struct ae_video_set_fps_param {
	cmr_s32 ae_video_fps_thr_low;
	cmr_s32 ae_video_fps_thr_high;
};

struct hist_meter_range_type{
	cmr_u32 num;
	struct ae_range range[AE_PIECEWISE_MAX_NUM];
};

struct bv_evd_ranges_type {
	cmr_u32 num;
	cmr_s32 bv[AE_PIECEWISE_MAX_NUM];
	cmr_s32 evd[AE_PIECEWISE_MAX_NUM];
	cmr_s32 bright_ref[AE_PIECEWISE_MAX_NUM][AE_PIECEWISE_MAX_NUM];
	cmr_s32 dark_ref[AE_PIECEWISE_MAX_NUM][AE_PIECEWISE_MAX_NUM];
	cmr_s32 tar_adj_ratio_min[AE_PIECEWISE_MAX_NUM][AE_PIECEWISE_MAX_NUM];
	cmr_s32 tar_adj_ratio_max[AE_PIECEWISE_MAX_NUM][AE_PIECEWISE_MAX_NUM];
};

struct ae_alg_aoe {
	cmr_u32 OE_str;
	cmr_u32 lowend;
	cmr_u32 highend;
};

struct hist_metering_tuning_param{
	cmr_u32 enable;
	cmr_u32 debug_lv;
	cmr_u32 speed;
	struct ae_ranges_type bv_range; /*zone define*/
	struct hist_meter_range_type bright_ref;
	struct hist_meter_range_type dark_ref;
	struct ae_alg_rgb_gain bright_percent[BV_STEP_NUM];
	struct ae_alg_rgb_gain dark_percent[BV_STEP_NUM];
	struct ae_alg_rgb_gain final_bright_percent;
	struct ae_alg_rgb_gain bright_percent4ev;
	struct ae_alg_rgb_gain dark_percent4ev;
	struct hist_meter_range_type tar_adj_ratio;
	struct ae_alg_rgb_gain bright_threshold;
	struct ae_alg_rgb_gain dark_threshold;
	struct ae_alg_aoe aoe_param;
	cmr_u16 lux_value;
	cmr_s16 bv_value;
	cmr_u16 bright_offset_weight;
	cmr_u16 dark_offset_weight;
	cmr_u16 bright_target_weight;
	cmr_u16 dark_target_weight;
	struct bv_evd_ranges_type bv_evd_range;
};


struct target_weight{
	struct ae_target_weight_func weight_cfg;/*6*4bytes*/
	//cmr_u16 ratio4base;
	//cmr_u16 ratio4hm;
	cmr_u32 reserved;/*1 * 4bytes*/
};/*2 * 4bytes---->7*4bytes*/
/*
struct hm_ratio_cfg{
	cmr_u32 evd_low;
	cmr_u32 evd_high;
	cmr_u16 ratio0;
	cmr_u16 ratio1;
	cmr_u32 reserved;////1 * 4bytes
};///4 * 4bytes
*/
struct hm_basic_cfg {
	struct ae_piecewise_func bv_ratio_cfg;/*17*4bytes*/
	struct ae_piecewise_func evd_ratio_cfg[HM_CFG_NUM];/*8 * 17*4bytes*/
	struct ae_piecewise_func br_pcnt_cfg;/*bright region pcent config*//*17*4bytes*/
};/*170*4bytes*/

struct hm_aoe_cfg {
	cmr_u16 strength;
	cmr_u16 oe_thrd;
	cmr_u32 oeratio_lowbnd;
	cmr_u32 oeratio_highbnd;
	struct ae_piecewise_func bv_ratio_cfg;/*17*4bytes*/
};/*20*4bytes*/

struct hm_coe_cfg {
	cmr_u32 tar;/*1 * 4bytes*/
	cmr_u32 tar4center;/*1 * 4bytes*/
	cmr_u32 pcent;/*1 * 4bytes*/
	cmr_u32 pcent4center;/*1 * 4bytes*/
	cmr_u32 weight4coe;
	struct ae_piecewise_func bg_y_ratio;/*17*4bytes*/
	struct ae_piecewise_func evd_ratio;/*17*4bytes*/
};/*39*4bytes*/

struct ae_hm_tuning_param {
	/*hist metering basic*/
	cmr_u16 magic;
	cmr_u8 enable;/*bit0:hm basic alg; bit1:aoe alg; bit2: coe alg*/
	cmr_u8 reserved;/*1 * 4bytes*/
	//cmr_u32 weight;/*histogram algorithm weight*//*1 * 4bytes*/
	cmr_u32 evd_dr_pcent;/*the dark regio define for EVD*/
	cmr_u32 evd_br_pcent;/*the bright regio define for EVD*/
	cmr_u16 highdy_tag_thd;/*target to high dynamic */
	cmr_u16 flat_tag_thd;/*target to fllat */
	struct hm_basic_cfg basic_cfg;/*170*4bytes*/
	/*anti_overexposure*/
	struct hm_aoe_cfg aoe_cfg;/*20*4bytes*/
	/*central overexposure*/	
	struct hm_coe_cfg coe_cfg;	/*39*4bytes*/
	//struct target_weight ratio;/*2*4bytes*/
	struct target_weight weight_lut;/*7*4bytes*/
	//struct hm_ratio_cfg flat4ratio; /*4*4bytes*/
};/*240 * 4bytes*/

struct ns_basic_cfg {
	cmr_u16 base_pcnet;/*night normal percent*/
	cmr_u16 reserved;/*1 * 4bytes*/
	struct ae_piecewise_func thrd;/*night normal scene*//*17*4bytes*/
};/*18 * 4bytes*/

struct ns_bt_cfg {
	cmr_u16 bt_tone_pcent;/*bright tone percent*/
	cmr_u16 flat_bt_pcent;/*bright tone percent for night flat*/
	cmr_u16 flat_dk_pcent;/*dark tone percent for night flat*/
	cmr_u16 reserved;/*2 * 4bytes*/
	struct ae_piecewise_func bt_thrd;/*bright tone*//*17*4bytes*/	
	struct ae_piecewise_func sky_prob;/*17*4bytes*/
};/*36 * 4 bytes*/

struct ns_dk_cfg {
	cmr_u16 dark_pcent;/*dark percent for night dark tone*/
	cmr_u16 dark_thrd;/*dark thrd for night tone*/
	cmr_u16  dark_thrd_max;/*the max value of dark thrd for night tone**/
	cmr_u16 reserved;// 2 * 4bytes
};/*2 * 4 bytes*/

struct ae_nsm_tuning_param {
	cmr_u8 magic[3];
	cmr_u8 enable;/*1*4bytes*/
	cmr_u16 flat_thrd;/*night flat thrd*/
	cmr_u16 cdf_pcent;/*1*4bytes*/
	struct ae_piecewise_func flat_prob;/*17*4bytes*/
	struct ae_piecewise_func bv_prob;/*17*4bytes*/
	struct ae_piecewise_func cdf_prob;/*17*4bytes*/
	struct ns_basic_cfg basic_cfg;/*18 * 4bytes*/
	struct ns_bt_cfg bt_cfg;/*36 * 4 bytes*/
	struct ns_dk_cfg dk_cfg;/*2 * 4 bytes*/
};/*109 * 4bytes*/

struct ae_monitor_tuning_param {
	cmr_s32 ae_monitor_win_num_w;
	cmr_s32 ae_monitor_win_num_h;
	cmr_s32 ae_monitor_win_size_w;
	cmr_s32 ae_monitor_win_size_h;
	cmr_s32 ae_trim_start_x;
	cmr_s32 ae_trim_start_y;
	cmr_s32 reserved;
};

struct ai_cfg {
	cmr_s16 x_idx;
	cmr_s16 y_lum; /*1 * 4bytes*/
	cmr_u32 reserved;
};

struct ai_tuning_param {
	cmr_u16 enable;
	cmr_u16 num; 					/*1 * 4bytes*/
	cmr_u32 reserved[3]; 				/*3 * 4bytes*/
	struct ai_cfg cfg_ai[AI_CFG_NUM];	/*16 * 4bytes*/
};

struct ae_ai_tuning_param {
	struct ai_tuning_param backlight_param;
	struct ai_tuning_param sky_param;
	struct ai_tuning_param foliage_param;
	struct ai_tuning_param night_param;
	struct ai_tuning_param outdoor_param;
	struct ai_tuning_param indoor_param;
	struct ai_tuning_param food_param;
	struct ai_tuning_param document_param;
	struct ai_tuning_param sunriseset_param;
	struct ai_tuning_param snow_param;
	struct ai_tuning_param others_param;
	cmr_u32 reserved[20]; /*20 * 4bytes*/
};
	
typedef struct {
	cmr_u16 lv;
	cmr_u16 evd_thrd[2];
	cmr_u16 strength_lv;
	cmr_u16 strength_evd[2];
} abl_cfg;	

struct abl_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 target_limit_low;
	cmr_u16 target_limit_high;
	cmr_u16 base_angle;
	abl_cfg cfg_info[ABL_CFG_NUM]; /*24 * 4bytes*/
	struct ae_piecewise_func in_piecewise; /*17 * 4bytes*/
	cmr_u32 abl_weight;
	cmr_u8 center_tar_lum;
	cmr_u8 weight_inner_o;
	cmr_u8 weight_inner_d;
	cmr_u8 weight_inner_c;
	cmr_u32 ev_diff_thrd_h;
	cmr_u32 ev_diff_thrd_l;
};

typedef struct {
	cmr_s16 offset[2];
} pcp_cfg;

struct pcp_tuning_param {
	cmr_u8 enable;
	cmr_u8 num;
	cmr_u16 red_lum_tar;
	cmr_u16 green_lum_tar;
	cmr_u16 blue_lum_tar;
	pcp_cfg cfg_info[PCP_CFG_NUM];
	struct ae_piecewise_func in_piecewise; /*17 * 4bytes*/
};

struct ae_tuning_param {		//total bytes must be 312696
	cmr_u32 version;
	cmr_u32 verify;
	cmr_u32 alg_id;
	cmr_u32 target_lum;
	cmr_u32 target_lum_zone;	// x16
	cmr_u8 convergence_speed;
	cmr_u8 iso_special_mode;
	cmr_u8 fast_convergence_disab;
	cmr_u8 reserved1;
	cmr_u32 flicker_index;
	cmr_u32 min_line;
	cmr_u32 start_index;
	cmr_u32 exp_skip_num;
	cmr_u32 gain_skip_num;
	struct ae_stat_req stat_req;
	struct ae_flash_tuning flash_tuning;
	struct touch_zone touch_param;
	struct ae_ev_table ev_table;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_exp_gain_table backup_ae_table[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_table weight_table[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_info scene_info[AE_SCENE_NUM];
	struct ae_auto_iso_tab auto_iso_tab;
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
	struct ae_touch_param touch_info;	/*it is in here,just for compatible; 3 * 4bytes */
	//struct ae_face_tune_param face_info;
	struct ae_range dc_fps;/*normal dc preview fps, 100x-->1x*/
	struct ae_range dv_fps;/*video fps, 100x-->1x*/
	cmr_u32 reserved0[2];
	/*13 * 4bytes */
	cmr_u8 monitor_mode;		/*0: single, 1: continue */
	cmr_u8 ae_tbl_exp_mode;		/*0: ae table exposure is exposure time; 1: ae table exposure is exposure line */
	cmr_u8 enter_skip_num;		/*AE alg skip frame as entering camera */
	cmr_u8 cnvg_stride_ev_num;
	cmr_s8 cnvg_stride_ev[32];
	cmr_s8 stable_zone_ev[16];

	struct ae_sensor_cfg sensor_cfg;	/*sensor cfg information: 2 * 4bytes */

	struct ae_lv_calibration lv_cali;	/*1 * 4bytes */
	/*scene detect and others alg */
	/*for touch info */

	struct flat_tuning_param flat_param;	/*51 * 4bytes */
	struct ae_flash_tuning_param flash_param;	/*1 * 4bytes */
	struct region_tuning_param region_param;	/*180 * 4bytes */
	struct mulaes_tuning_param mulaes_param;	/*9 * 4bytes */
	struct face_tuning_param face_param;
	struct ae_hdr_tuning_param hdr_param;
	struct ae_flash_swith_param flash_swith_param;
	struct ae_flash_control_param flash_control_param;
	struct ae_video_set_fps_param ae_video_fps;
	struct ae_monitor_tuning_param monitor_param;
	struct ae_ai_tuning_param ai_param;  /*240 * 4bytes*/
	struct abl_tuning_param abl_param;   /*47 * 4bytes*/
	struct pcp_tuning_param pcp_param; /*27 * 4bytes*/
	struct ae_hm_tuning_param hm_param; /*240 * 4bytes*/
	struct ae_nsm_tuning_param ns_param; /*109 * 4bytes*/
	struct ae_thrd_param threednr_ctrl_param;/*3DNR control param*/
	struct ae_thrd_param fourcell_ctrl_param;/*4in1 control param*/
	struct ae_thrd_param auto_flash_ctrl_param;/*auto flash control param*/
	struct face_tuning_param_adv face_param_adv;/*50 * 4bytes*/
	cmr_u32 reserved[1295];
};
#endif
