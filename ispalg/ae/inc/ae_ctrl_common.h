/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef _AE_CTRL_COMMON_H_
#define _AE_CTRL_COMMON_H_

struct ae_senseor_out {
	cmr_s8 stable;
	cmr_s8 f_stable;
	cmr_u8 near_stable;
	cmr_s16 cur_index;			/*the current index of ae table in ae now: 1~1024 */
	cmr_u32 exposure_time;		/*exposure time, unit: 0.1us */
	float cur_fps;				/*current fps:1~120 */
	cmr_u16 cur_exp_line;		/*current exposure line: the value is related to the resolution */
	cmr_u16 cur_dummy;			/*dummy line: the value is related to the resolution & fps */
	cmr_s16 cur_again;			/*current analog gain */
	cmr_s16 cur_dgain;			/*current digital gain */
	cmr_s32 cur_bv;
	cmr_u32 frm_len;
	cmr_u32 frm_len_def;
	cmr_u8 binning_mode;
};

struct ae_lib_calc_out_2_x {
	cmr_u32 version;			/*version No. for this structure */
	cmr_s16 cur_lum;			/*the lum of image:0 ~255 */
	cmr_s16 cur_lum_avg;		/*the lum without weight of image:0 ~255*/
	cmr_s16 target_lum;			/*the ae target lum: 0 ~255 */
	cmr_s16 target_range_in;		/*ae target lum stable zone: 0~255 */
	cmr_s16 target_range_out;
	cmr_s16 target_zone;		/*ae target lum stable zone: 0~255 */
	cmr_s16 target_lum_ori;		/*the ae target lum(original): 0 ~255 */
	cmr_s16 target_zone_ori;	/*the ae target lum stable zone(original):0~255 */
	cmr_u32 frame_id;
	cmr_s16 cur_bv;				/*bv parameter */
	cmr_s16 cur_bv_nonmatch;
	cmr_s16 *histogram;			/*luma histogram of current frame */
	//for flash
	cmr_s32 flash_effect;
	cmr_s8 flash_status;
	cmr_s16 mflash_exp_line;
	cmr_s16 mflash_dummy;
	cmr_s16 mflash_gain;
	//for touch
	cmr_s8 tcAE_status;
	cmr_s8 tcRls_flag;
	//for face debug
	cmr_u32 face_lum;
	void *pmulaes;
	void *pflat;
	void *pregion;
	void *pai;
	void *pabl;
	void *ppcp;
	void *phm;
	void *pns;
	void *ptc;					/*Bethany add touch info to debug info */
	void *pface_ae;
	struct ae_senseor_out wts;
	cmr_handle log;
	cmr_u32 flag4idx;
	cmr_u32 face_stable;
	cmr_u32 face_enable;
	cmr_u32 face_trigger;
	cmr_u32 target_offset;
	cmr_u32 privated_data;
	cmr_u32 abl_weighting;
	cmr_s32 evd_value;
};

struct ae_lib_calc_out_3_x {		//	ae_lib_calc_out
	cmr_u32 frame_id;
	cmr_u32 stable;
	cmr_u32 face_stable;
	cmr_u32 face_enable;
	cmr_u32 near_stable;
	cmr_s32 cur_bv;
	cmr_s32 cur_bv_nonmatch;
	cmr_u16 cur_lum;			/*the lum of image:0 ~255 */
	cmr_u16 cur_lum_avg;	/*the lum without weight of image:0 ~255*/
	cmr_u16 target_lum;
	cmr_u16 base_target_lum;		//no face AE target luma
	cmr_u16 stab_zone_in;
	cmr_u16 stab_zone_out;
	cmr_u32 flash_status;
	cmr_s8 touch_status;
	float cur_fps;				/*current fps:1~120 */
	cmr_u16 abl_confidence;
	cmr_s32 evd_value;
	struct ae_ev_setting_param ev_setting;
	struct ae_rgbgamma_curve gamma_curve;/*will be used in future*/
	struct ae_ygamma_curve ygamma_curve;/*will be used in future*/
	/*AEM ROI setting*/
	struct ae_point_type aem_roi_st;
	struct ae_size aem_blk_size;
	/*Bayer Hist ROI setting*/

	struct ae_rect adjust_hist_roi;

	/*APEX parameters*/
	float bv;
	float av;
	float tv;
	float sv;
	/*mlog information(LCD display)*/
	void *log_buf;
	cmr_u32 log_len;
	/*debug information(JPG Exif)*/
	void *debug_info;
	cmr_u32 debug_len;
	/*privated information*/
	cmr_u32 privated_data;

};
#if 0
struct ae_lib_calc_out_3_x  {
	cmr_u32 frame_id;
	cmr_u32 stable;
	cmr_u32 face_stable;
	cmr_u32 face_enable;
	cmr_u32 near_stable;
	cmr_s32 cur_bv;
	cmr_s32 cur_bv_nonmatch;
	cmr_u16 cur_lum;			/*the lum of image:0 ~255 */
	cmr_u16 cur_lum_avg;	/*the lum without weight of image:0 ~255*/
	cmr_u16 target_lum;
	cmr_u16 stab_zone_in;
	cmr_u16 stab_zone_out;
	cmr_u32 flash_status;
	cmr_s8 touch_status;
	float cur_fps;				/*current fps:1~120 */
	cmr_u16 abl_confidence;
	cmr_s32 evd_value;
	struct ae_ev_setting_param ev_setting;
	struct ae_rgbgamma_curve gamma_curve;/*will be used in future*/
	struct ae_ygamma_curve ygamma_curve;/*will be used in future*/
	/*AEM ROI setting*/
	struct ae_point_type aem_roi_st;
	struct ae_size aem_blk_size;
	/*Bayer Hist ROI setting*/
	//struct ae_rect adjust_hist_roi;
	/*APEX parameters*/
	float bv;
	float av;
	float tv;
	float sv;
	/*mlog information(LCD display)*/
	void *log_buf;
	cmr_u32 log_len;
	/*debug information(JPG Exif)*/
	void *debug_info;
	cmr_u32 debug_len;
	/*privated information*/
	cmr_u32 privated_data;
};
#endif
struct ae_lib_calc_out_common   {
	struct ae_lib_calc_out_2_x ae_result_2_x;
	struct ae_lib_calc_out_3_x ae_result_3_x;
};

#endif
