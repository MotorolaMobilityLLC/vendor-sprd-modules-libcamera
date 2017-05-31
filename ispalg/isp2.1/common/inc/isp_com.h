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

#ifndef _ISP_COM_H_
#define _ISP_COM_H_

#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#endif

#include "sprd_isp_r6p10.h"
#include "isp_type.h"
#include "isp_app.h"
#include "sensor_raw.h"
#include <string.h>
#include <stdlib.h>


#ifdef	 __cplusplus
extern "C" {
#endif

#define CAMERA_DUMP_PATH  "/data/misc/cameraserver/"
#define ISP_SLICE_WIN_NUM 0x0b
#define ISP_SLICE_WIN_NUM_V1 0x18
#define ISP_CMC_NUM 0x09
#define ISP_COLOR_TEMPRATURE_NUM 0x09
#define ISP_RAWAEM_BUF_SIZE   (4*3*1024)
#define ISP_BQ_AEM_CNT                      3
#define ISP_INPUT_SIZE_NUM_MAX 0x09
#define ISP_GAMMA_SAMPLE_NUM 26
#define ISP_CCE_COEF_COLOR_CAST 0
#define ISP_CCE_COEF_GAIN_OFFSET 1
#define CLIP(in, bottom, top) {if(in<bottom) in=bottom; if(in>top) in=top;}
typedef cmr_int(*io_fun) (cmr_handle isp_alg_handle, void *param_ptr, cmr_s32(*call_back) ());

enum isp_fetch_format {
	ISP_FETCH_YUV422_3FRAME = 0x00,
	ISP_FETCH_YUYV,
	ISP_FETCH_UYVY,
	ISP_FETCH_YVYU,
	ISP_FETCH_VYUY,
	ISP_FETCH_YUV422_2FRAME,
	ISP_FETCH_YVU422_2FRAME,
	ISP_FETCH_NORMAL_RAW10,
	ISP_FETCH_CSI2_RAW10,
	ISP_FETCH_FORMAT_MAX
};

enum isp_store_format {
	ISP_STORE_UYVY = 0x00,
	ISP_STORE_YUV422_2FRAME,
	ISP_STORE_YVU422_2FRAME,
	ISP_STORE_YUV422_3FRAME,
	ISP_STORE_YUV420_2FRAME,
	ISP_STORE_YVU420_2FRAME,
	ISP_STORE_YUV420_3FRAME,
	ISP_STORE_FORMAT_MAX
};

enum isp_work_mode {
	ISP_SINGLE_MODE = 0x00,
	ISP_CONTINUE_MODE,
};

enum isp_match_mode {
	ISP_CAP_MODE = 0x00,
	ISP_EMC_MODE,
	ISP_DCAM_MODE,
	ISP_SIMULATION_MODE,
};

enum isp_endian {
	ISP_ENDIAN_LITTLE = 0x00,
	ISP_ENDIAN_BIG,
	ISP_ENDIAN_HALFBIG,
	ISP_ENDIAN_HALFLITTLE,
	ISP_ENDIAN_MAX
};

enum isp_bit_reorder {
	ISP_LSB = 0x00,
};

enum isp_slice_type {
	ISP_FETCH = 0x00,
};

enum isp_slice_type_v1 {
	ISP_LSC_V1 = 0x00,
	ISP_CSC_V1,
	ISP_BDN_V1,
	ISP_PWD_V1,
	ISP_LENS_V1,
	ISP_AEM_V1,
	ISP_Y_AEM_V1,
	ISP_RGB_AFM_V1,
	ISP_Y_AFM_V1,
	ISP_HISTS_V1,
	ISP_DISPATCH_V1,
	ISP_FETCH_V1,
	ISP_STORE_V1,
	ISP_SLICE_TYPE_MAX_V1
};

enum isp_slice_pos_info {
	ISP_SLICE_MID,
	ISP_SLICE_LAST,
};

struct isp_slice_param {
	enum isp_slice_pos_info pos_info;
	cmr_u32 slice_line;
	cmr_u32 complete_line;
	cmr_u32 all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM];
	cmr_u32 edge_info;
};

struct isp_slice_param_v1 {
	enum isp_slice_pos_info pos_info;
	cmr_u32 slice_line;
	cmr_u32 complete_line;
	cmr_u32 all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM_V1];
	cmr_u32 edge_info;
};

struct isp_io_ctrl_fun {
	enum isp_ctrl_cmd cmd;
	io_fun io_ctrl;
};

struct isp_awbm_param {
	cmr_u32 bypass;
	struct isp_pos win_start;
	struct isp_size win_size;
};

struct isp_ae_grgb_statistic_info {
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
};

struct isp_awb_statistic_info {
	cmr_u32 r_info[1024];
	cmr_u32 g_info[1024];
	cmr_u32 b_info[1024];
};

struct isp_binning_statistic_info {
	cmr_u32 *r_info;
	cmr_u32 *g_info;
	cmr_u32 *b_info;
	struct isp_size binning_size;
};

enum isp_ctrl_mode {
	ISP_CTRL_SET = 0x00,
	ISP_CTRL_GET,
	ISP_CTRL_MODE_MAX
};

struct isp_af_ctrl {
	enum isp_ctrl_mode mode;
	cmr_u32 step;
	cmr_u32 num;
	cmr_u32 stat_value[9];
};

struct isp_data_param {
	enum isp_work_mode work_mode;
	enum isp_match_mode input;
	enum isp_format input_format;
	cmr_u32 format_pattern;
	struct isp_size input_size;
	struct isp_addr input_addr;
	struct isp_addr input_vir;
	enum isp_format output_format;
	enum isp_match_mode output;
	struct isp_addr output_addr;
	cmr_u16 slice_height;
	cmr_u32 input_fd;
};

struct isp_interface_param_v1 {
	struct isp_data_param data;

	struct isp_dev_fetch_info fetch;
	struct isp_dev_store_info store;
	struct isp_dev_dispatch_info dispatch;
	struct isp_dev_arbiter_info arbiter;
	struct isp_dev_common_info com;
	struct isp_size src;
	struct isp_slice_param_v1 slice;
};

struct isp_system {
	isp_handle caller_id;
	proc_callback callback;
	cmr_u32 isp_callback_bypass;
	pthread_t monitor_thread;
	isp_handle monitor_queue;
	cmr_u32 monitor_status;

	isp_handle thread_ctrl;
	isp_handle thread_afl_proc;
	isp_handle thread_af_proc;
	isp_handle thread_awb_proc;
	struct isp_ops ops;
};

struct isp_otp_info {
	struct isp_data_info lsc;
	struct isp_data_info awb;

	void *lsc_random;
	void *lsc_golden;
	cmr_u32 width;
	cmr_u32 height;
};

struct isp_ae_info {
	cmr_s32 bv;
	float gain;
	float exposure;
	float f_value;
	cmr_u32 stable;
};

struct isp_statis_mem_info {
	cmr_uint isp_statis_mem_size;
	cmr_uint isp_statis_mem_num;
	cmr_uint isp_statis_k_addr;
	cmr_uint isp_statis_u_addr;
	cmr_uint isp_statis_alloc_flag;
	cmr_s32 statis_mfd;
	cmr_s32 statis_buf_dev_fd;

	cmr_uint isp_lsc_mem_size;
	cmr_uint isp_lsc_mem_num;
	cmr_uint isp_lsc_physaddr;
	cmr_uint isp_lsc_virtaddr;
	cmr_s32 lsc_mfd;

	void *buffer_client_data;
	void *cb_of_malloc;
	void *cb_of_free;
};

struct isp_statis_info {
	cmr_u32 irq_type;
	cmr_u32 irq_property;
	cmr_u32 phy_addr;
	cmr_u32 vir_addr;
	cmr_u32 kaddr[2];
	cmr_u32 buf_size;
	cmr_s32 mfd;
};

typedef struct {
	/* isp_ctrl private */
#ifndef WIN32
	struct isp_system system;
#endif
	cmr_u32 camera_id;
	uint isp_mode;

	//new param
	void *dev_access_handle;
	void *alg_fw_handle;
	//void *isp_otp_handle;

	/* isp_driver */
	void *handle_device;

	/* 4A algorithm */
	void *handle_ae;
	void *handle_af;
	void *handle_awb;
	void *handle_smart;
	void *handle_lsc_adv;

	/* isp param manager */
	void *handle_pm;

	/* sensor param */
	cmr_u32 param_index;
	struct sensor_raw_resolution_info input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
	struct sensor_raw_ioctrl *ioctrl_ptr;

	cmr_u32 alc_awb;
	cmr_s32 awb_pg_flag;
	cmr_u8 *log_alc_awb;
	cmr_u32 log_alc_awb_size;
	cmr_u8 *log_alc_lsc;
	cmr_u32 log_alc_lsc_size;
	cmr_u8 *log_alc;
	cmr_u32 log_alc_size;
	cmr_u8 *log_alc_ae;
	cmr_u32 log_alc_ae_size;

	struct awb_lib_fun *awb_lib_fun;
	struct ae_lib_fun *ae_lib_fun;
	struct af_lib_fun *af_lib_fun;

	struct isp_ops ops;

	struct sensor_raw_info *sn_raw_info;

	/*for new param struct */
	struct isp_data_info isp_init_data[MAX_MODE_NUM];
	struct isp_data_info isp_update_data[MAX_MODE_NUM];	/*for isp_tool */

	cmr_u32 gamma_sof_cnt;
	cmr_u32 gamma_sof_cnt_eb;
	cmr_u32 update_gamma_eb;
	cmr_u32 mode_flag;
	cmr_u32 scene_flag;
	cmr_u32 multi_nr_flag;
	cmr_s8 *sensor_name;
} isp_ctrl_context;

#define ISP_NLC_POINTER_NUM 29
#define ISP_NLC_POINTER_L_NUM 27

struct isp_blc_offset {
	cmr_u16 r;
	cmr_u16 gr;
	cmr_u16 gb;
	cmr_u16 b;
};

struct isp_blc_param {
	struct isp_sample_point_info cur_idx;
	struct isp_dev_blc_info cur;
	struct isp_blc_offset offset[SENSOR_BLC_NUM];
};

struct isp_lsc_info {
	struct isp_sample_point_info cur_idx;
	void *data_ptr;
	void *param_ptr;
	cmr_u32 len;
	cmr_u32 grid;
	cmr_u32 gain_w;
	cmr_u32 gain_h;
};

struct isp_lsc_ctrl_in {
	cmr_u32 image_pattern;
	struct isp_lsc_info *lsc_input;
};

struct isp_lnc_map {
	cmr_u32 ct;
	cmr_u32 grid_mode;
	cmr_u32 grid_pitch;
	cmr_u32 gain_w;
	cmr_u32 gain_h;
	cmr_u32 grid;
	void *param_addr;
	cmr_u32 len;
};

struct isp_lnc_param {
	cmr_u32 update_flag;
	struct isp_dev_lsc_info cur;
	struct isp_sample_point_info cur_index_info;	/*for two lsc parameters to interplate */
	struct isp_lnc_map map;	//current lsc map
	struct isp_lnc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	struct isp_size resolution;
	cmr_u32 tab_num;
	cmr_u32 lnc_param_max_size;
};

struct isp_awbc_cfg {
	cmr_u32 r_gain;
	cmr_u32 g_gain;
	cmr_u32 b_gain;
	cmr_u32 r_offset;
	cmr_u32 g_offset;
	cmr_u32 b_offset;
};

struct isp_ae_statistic_info {
	cmr_u32 y[1024];
};

struct isp_ae_param {
	struct isp_ae_statistic_info stat_info;
};

struct afl_ctrl_proc_out {
	cmr_int flag;
	cmr_int cur_flicker;
};

struct isp_anti_flicker_cfg {
	cmr_u32 bypass;
	cmr_u32 mode;
	cmr_u32 skip_frame_num;
	cmr_u32 line_step;
	cmr_u32 frame_num;
	cmr_u32 vheight;
	cmr_u32 start_col;
	cmr_u32 end_col;
	void *addr;
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	cmr_handle dev_handle;
	cmr_uint vir_addr;
	cmr_uint height;
	cmr_uint width;
	struct afl_ctrl_proc_out proc_out;
	isp_afl_cb afl_set_cb;
};

struct isp_af_statistic_info {
	cmr_u64 info[32];
	cmr_u32 info_tshark3[105];
};

struct isp_afm_param {
	struct isp_af_statistic_info cur;
};

struct isp_af_param {
	struct isp_afm_param afm;
};

struct isp_gamma_curve_info {
	cmr_u32 axis[2][ISP_GAMMA_SAMPLE_NUM];
};

struct isp_gamma_info {
	cmr_u16 axis[2][26];
	cmr_u8 index[28];
};

struct isp_bright_cfg {
	cmr_u32 factor;
};

struct isp_bright_param {
	cmr_u32 cur_index;
	struct isp_dev_brightness_info cur;
	cmr_u8 bright_tab[16];
	cmr_u8 scene_mode_tab[MAX_SCENEMODE_NUM];
};

//for contrast
struct isp_contrast_cfg {
	cmr_u32 factor;
};

struct isp_contrast_param {
	cmr_u32 cur_index;
	struct isp_dev_contrast_info cur;
	cmr_u8 tab[16];
	cmr_u8 scene_mode_tab[MAX_SCENEMODE_NUM];
};

struct isp_saturation_cfg {
	cmr_u32 factor;
};

struct isp_hue_cfg {
	cmr_u32 factor;
};

struct isp_edge_cfg {
	cmr_u32 factor;
};

struct isp_flash_attrib_param {
	cmr_u32 r_sum;
	cmr_u32 gr_sum;
	cmr_u32 gb_sum;
	cmr_u32 b_sum;
};

struct isp_flash_attrib_cali {
	struct isp_flash_attrib_param global;
	struct isp_flash_attrib_param random;
};

struct isp_flash_info {
	cmr_u32 lum_ratio;
	cmr_u32 r_ratio;
	cmr_u32 g_ratio;
	cmr_u32 b_ratio;
	cmr_u32 auto_flash_thr;
};

struct isp_flash_param {
	struct isp_flash_info cur;
	struct isp_flash_attrib_cali attrib;
};
struct isp_dualflash_param {
	cmr_u8 version;
	cmr_u8 alg_id;
	cmr_u8 flashLevelNum1;
	cmr_u8 flashLevelNum2;
	cmr_u8 preflahLevel1;
	cmr_u8 preflahLevel2;
	cmr_u16 preflashBrightness;
	cmr_u16 brightnessTarget;
	cmr_u16 brightnessTargetMax;
	cmr_u16 foregroundRatioHigh;
	cmr_u16 foregroundRatioLow;
	cmr_u8 flashMask[1024];
	cmr_u16 brightnessTable[1024];
	cmr_u16 rTable[1024];
	cmr_u16 bTable[1024];
};

struct isp_dual_flash_param {
	struct isp_dualflash_param cur;
};

struct isp_envi_detect_param {
	cmr_u32 enable;
	struct isp_range envi_range[SENSOR_ENVI_NUM];
};

struct isp_pre_global_gain_param {
	struct isp_dev_pre_glb_gain_info cur;
};

struct isp_postblc_param {
	struct isp_dev_post_blc_info cur;
	struct isp_sample_point_info cur_idx;
	struct sensor_blc_offset offset[SENSOR_BLC_NUM];
};

struct isp_pdaf_correction_param {
	struct pdaf_correction_param cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_pdaf_extraction_param {
	struct pdaf_extraction_param cur;
};

struct isp_dev_noise_filter_param {
	struct isp_dev_noise_filter_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_rgb_gain_param {
	struct isp_dev_rgb_gain_info cur;
};

struct isp_nlc_param {
	struct isp_dev_nlc_info cur;
};

struct isp_2d_lsc_param {
	struct isp_sample_point_info cur_index_info;
	struct isp_dev_2d_lsc_info cur;
	struct isp_data_info final_lsc_param;	//store the resulted lsc params
	struct isp_lnc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	cmr_u32 tab_num;
	struct isp_lsc_info lsc_info;
	struct isp_size resolution;
	cmr_u32 update_flag;
	cmr_u32 is_init;

	void *tmp_ptr_a;
	void *tmp_ptr_b;
};

struct isp_1d_lsc_param {
	struct isp_sample_point_info cur_index_info;
	struct isp_dev_1d_lsc_info cur;
	struct sensor_1d_lsc_map map[SENSOR_LENS_NUM];
};

struct isp_binning4awb_param {
	struct isp_dev_binning4awb_info cur;
};

struct isp_awb_param {
	cmr_u32 ct_value;
	struct isp_dev_awb_info cur;
	struct isp_awb_statistic_info stat;
	struct isp_data_info awb_statistics[4];
};

struct isp_rgb_aem_param {
	struct isp_dev_raw_aem_info cur;
	struct isp_awb_statistic_info stat;
};

struct isp_rgb_afm_param {
	struct isp_dev_rgb_afm_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *param_ptr1;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_bpc_param {
	struct isp_dev_bpc_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_grgb_param {
	struct isp_dev_grgb_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_ynr_param {
	struct isp_dev_ynr_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_3d_nr_pre_param {
	struct isp_dev_3dnr_pre_param_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_3d_nr_cap_param {
	struct isp_dev_3dnr_cap_param_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_nlm_param {
	cmr_u32 cur_level;
	cmr_u32 level_num;
	struct isp_dev_nlm_info cur;
	struct isp_data_info vst_map;
	struct isp_data_info ivst_map;
	cmr_uint *nlm_ptr;
	cmr_uint *vst_ptr;
	cmr_uint *ivst_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_cfa_param {
	struct isp_dev_cfa_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_cmc10_param {
	struct isp_dev_cmc10_info cur;
	struct isp_sample_point_info cur_idx_info;
	cmr_u16 matrix[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	cmr_u16 result_cmc[SENSOR_CMC_POINT_NUM];
	cmr_u16 reserved;
	cmr_u32 reduce_percent;	//reduce saturation.
};

struct isp_frgb_gamc_param {
	struct isp_dev_gamma_info cur;
	struct sensor_gamma_curve final_curve;
	struct isp_sample_point_info cur_idx;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
};

struct isp_cce_param {
	struct isp_dev_cce_info cur;
	/*R/G/B coef to change cce */
	cmr_s32 cur_level[2];
	/*0: color cast, 1: gain offset */
	cmr_u16 cce_coef[2][3];
	cmr_u16 bakup_cce_coef[3];
	cmr_u32 prv_idx;
	cmr_u32 cur_idx;
	cmr_u32 is_specialeffect;
	struct isp_dev_cce_info cce_tab[16];
	struct isp_dev_cce_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_cce_uvdiv_param {
	struct isp_dev_uvd_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_hsv_param {
	struct isp_dev_hsv_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_data_info final_map;
	struct isp_data_info map[SENSOR_HSV_NUM];
	struct isp_data_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_yuv_pre_cdn_param {
	struct isp_dev_yuv_precdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_posterize_param {
	struct isp_dev_posterize_info cur;
	struct isp_dev_posterize_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_yiq_afl_param_v1 {
	struct isp_dev_anti_flicker_info cur;
};

struct isp_yiq_afl_param_v3 {
	struct isp_dev_anti_flicker_new_info cur;
};

struct isp_rgb_dither_param {
	struct isp_dev_rgb_dither_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_hist_param {
	struct isp_dev_hist_info cur;
};

struct isp_hist2_param {
	struct isp_dev_hist2_info cur;
};

struct isp_uv_cdn_param {
	struct isp_dev_yuv_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_edge_param {
	struct isp_dev_edge_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_chrom_saturation_param {
	struct isp_dev_csa_info cur;
	cmr_u32 cur_u_idx;
	cmr_u32 cur_v_idx;
	cmr_u8 tab[2][SENSOR_LEVEL_NUM];
	cmr_u8 scene_mode_tab[2][MAX_SCENEMODE_NUM];
};

struct isp_hue_param {
	struct isp_dev_hue_info cur;
	cmr_u32 cur_idx;
	cmr_s16 tab[SENSOR_LEVEL_NUM];
};

struct isp_uv_postcdn_param {
	struct isp_dev_post_cdn_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_yuv_ygamma_param {
	struct isp_dev_ygamma_info cur;
	cmr_u32 cur_idx;
	struct isp_sample_point_info cur_idx_weight;
	struct sensor_gamma_curve final_curve;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_ydelay_param {
	struct isp_dev_ydelay_info cur;
};

struct isp_iircnr_iir_param {
	struct isp_dev_iircnr_info cur;
	cmr_u32 cur_level;
	cmr_u32 level_num;
	cmr_uint *param_ptr;
	cmr_uint *scene_ptr;
	cmr_u32 nr_mode_setting;
};

struct isp_iircnr_yrandom_param {
	struct isp_dev_yrandom_info cur;
};

struct isp_context {
	cmr_u32 is_validate;
	cmr_u32 mode_id;


// 3A owner:
//      struct isp_awb_param awb;
	struct isp_ae_param ae;
	struct isp_af_param af;
	struct isp_smart_param smart;
	struct isp_sft_af_param sft_af;
	struct isp_aft_param aft;
	struct isp_alsc_param alsc;

//isp related tuning block
	struct isp_bright_param bright;
	struct isp_contrast_param contrast;
	struct isp_flash_param flash;
	struct isp_dualflash_param dual_flash;
	struct isp_envi_detect_param envi_detect;

	struct isp_pre_global_gain_param pre_gbl_gain;
	struct isp_blc_param blc;
	struct isp_postblc_param post_blc;
	struct isp_rgb_gain_param rgb_gain;
	struct isp_nlc_param nlc;
	struct isp_2d_lsc_param lsc_2d;
	struct isp_1d_lsc_param lsc_1d;
	struct isp_binning4awb_param binning4awb;
	struct isp_awb_param awb;
	struct isp_rgb_aem_param aem;
	struct isp_rgb_afm_param afm;
	struct isp_bpc_param bpc;

	struct isp_grgb_param grgb;
	struct isp_ynr_param ynr;
	struct isp_pdaf_correction_param pdaf_correct;
	struct isp_pdaf_extraction_param pdaf_extraction;
	struct isp_nlm_param nlm;
	struct isp_cfa_param cfa;
	struct isp_cmc10_param cmc10;
	struct isp_frgb_gamc_param frgb_gamc;

	struct isp_cce_param cce;
	struct isp_hsv_param hsv;
	struct isp_yuv_pre_cdn_param yuv_pre_cdn;
	struct isp_posterize_param posterize;
	struct isp_yiq_afl_param_v1 yiq_afl_v1;
	struct isp_yiq_afl_param_v3 yiq_afl_v3;
	struct isp_rgb_dither_param rgb_dither;

	struct isp_hist_param hist;
	struct isp_hist2_param hist2;
	struct isp_uv_cdn_param uv_cdn;
	struct isp_edge_param edge;
	struct isp_chrom_saturation_param saturation;
	struct isp_hue_param hue;
	struct isp_uv_postcdn_param uv_postcdn;
	struct isp_yuv_ygamma_param yuv_ygamma;
	struct isp_ydelay_param ydelay;
	struct isp_iircnr_iir_param iircnr_iir;
	struct isp_iircnr_yrandom_param iircnr_yrandom;
	struct isp_cce_uvdiv_param uv_div;
	struct isp_dev_noise_filter_param yuv_noisefilter;
	struct isp_3d_nr_pre_param nr_3d_pre;
	struct isp_3d_nr_cap_param nr_3d_cap;

};

#ifdef	 __cplusplus
}
#endif
#endif
