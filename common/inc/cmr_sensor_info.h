/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _CMR_SENSOR_INFO_H_
#define _CMR_SENSOR_INFO_H_
#include "cmr_types.h"

#define MAX_MODE_NUM 16

#define AE_FLICKER_NUM 2
#define AE_ISO_NUM_NEW 8
#define AE_WEIGHT_TABLE_NUM 3
#define AE_SCENE_NUM 8
#define SNR_NAME_MAX_LEN 64

#define SENSOR_DUAL_OTP_TOTAL_SIZE 8192
#define SENSOR_DUAL_OTP_MASTER_SLAVE_SIZE 2074
#define SENSOR_DUAL_OTP_DATA3D_SIZE 2142
#define SENSOR_DUAL_OTP_MASTER_SLAVE_OFFSET 1702
#define SENSOR_DUAL_OTP_DATA3D_OFFSET 4384

typedef void (*isp_buf_cfg_evt_cb)(cmr_int evt, void *data, cmr_u32 data_len, void *privdata);

struct isp_block_header {
	cmr_u8 block_name[8];

	cmr_u32 block_id;
	cmr_u32 version_id;
	cmr_u32 param_id;

	cmr_u32 bypass;

	cmr_u32 size;
	cmr_u32 offset;

	cmr_u32 reserved[4];
};

struct isp_mode_param {
	cmr_u32 version_id;

	cmr_u8 mode_name[8];

	cmr_u32 mode_id;
	cmr_u32 block_num;
	cmr_u32 size;
	cmr_u32 width;
	cmr_u32 height;

	cmr_u32 fps;
	cmr_u32 reserved[3];

	struct isp_block_header block_header[256];
};

struct isp_mode_param_info {
	cmr_u8 *addr;
	/* by bytes */
	cmr_u32 len;
};

struct third_lib_info {
	uint32_t product_id;
	uint32_t product_name_high;
	uint32_t product_name_low;
	uint32_t version_id;
	uint32_t reserve[4];
};

struct sensor_libuse_info {
	struct third_lib_info awb_lib_info;
	struct third_lib_info ae_lib_info;
	struct third_lib_info af_lib_info;
	struct third_lib_info lsc_lib_info;
	uint32_t reserve[32];
};

#if 0
typedef struct {
	u8 ucSensorMode;	/*FR,  binning_1, binning_2 */
	SCINFO_SENSOR_MODULE_TYPE ucSensorMouduleType;	/*IMX_219, OV4688 */
	u16 uwWidth;		/*Width of Raw image */
	u16 uwHeight;		/*Height of Raw image */
	u16 uwFrameRate;	/* FPS */
	u32 udLineTime;		/*line time x100 */
	SCINFO_COLOR_ORDER nColorOrder;	/*color order */
	u16 uwClampLevel;	/*sensor's clamp level */
} SCINFO_MODE_INFO_ISP;
#endif

struct sensor_version_info {
	cmr_u32 version_id;
	cmr_u32 srtuct_size;
	cmr_u32 reserve;
};

struct sensor_raw_resolution_info {
	cmr_u16 start_x;
	cmr_u16 start_y;
	cmr_u16 width;
	cmr_u16 height;
	cmr_u16 line_time;
	cmr_u16 frame_line;
};

struct af_pose_dis {
	cmr_u32 up2hori;
	cmr_u32 hori2down;
};

struct sensor_ex_info {
	cmr_u32 f_num;
	cmr_u32 focal_length;
	cmr_u32 max_fps;
	cmr_u32 max_adgain;
	cmr_u32 ois_supported;
	cmr_u32 pdaf_supported;
	cmr_u32 exp_valid_frame_num;
	cmr_u32 clamp_level;
	cmr_u32 adgain_valid_frame_num;
	cmr_u32 preview_skip_num;
	cmr_u32 capture_skip_num;
	cmr_s8 *name;
	cmr_s8 *sensor_version_info;
	struct af_pose_dis pos_dis;
};

struct sensor_raw_resolution_info_tab {
	cmr_u32 image_pattern;
	struct sensor_raw_resolution_info tab[10];
};

struct sensor_raw_ioctrl {
	cmr_handle caller_handler;
	cmr_u32(*set_focus) (cmr_handle caller_handler, cmr_u32 param);
	cmr_u32(*set_exposure) (cmr_handle caller_handler, cmr_u32 param);
	cmr_u32(*set_gain) (cmr_handle caller_handler, cmr_u32 param);
	cmr_u32(*ext_fuc) (cmr_handle caller_handler, void *param);
	cmr_int(*write_i2c) (cmr_handle caller_handler, cmr_u16 slave_addr, cmr_u8 * cmd,
			     cmr_u16 cmd_length);
	cmr_int(*read_i2c) (cmr_handle caller_handler, cmr_u16 slave_addr, cmr_u8 * cmd,
			    cmr_u16 cmd_length);
	uint32_t(*ex_set_exposure) (cmr_handle caller_handler, uint32_t param);
};

/*************new***************************/
struct sensor_fix_param_mode_info {
	uint32_t version_id;
	uint32_t mode_id;
	uint32_t width;
	uint32_t height;
	uint32_t reserved[8];
};

struct sensor_fix_param_block_info {
	uint32_t version;
	uint32_t block_id;
	uint32_t reserved[8];
};

struct sensor_mode_fix_param {
	uint32_t *mode_info;
	uint32_t len;
};

struct sensor_block_fix_param {
	uint32_t *block_info;
	uint32_t len;
};

struct ae_exp_gain_tab {
	uint32_t *index;
	uint32_t index_len;
	uint32_t *exposure;
	uint32_t exposure_len;
	uint32_t *dummy;
	uint32_t dummy_len;
	uint16_t *again;
	uint32_t again_len;
	uint16_t *dgain;
	uint32_t dgain_len;
};

struct ae_scene_exp_gain_tab {
	uint32_t *scene_info;
	uint32_t scene_info_len;
	uint32_t *index;
	uint32_t index_len;
	uint32_t *exposure;
	uint32_t exposure_len;
	uint32_t *dummy;
	uint32_t dummy_len;
	uint16_t *again;
	uint32_t again_len;
	uint16_t *dgain;
	uint32_t dgain_len;
};

struct ae_weight_tab {
	uint8_t *weight_table;
	uint32_t len;
};

struct ae_auto_iso_tab_v1 {
	uint16_t *auto_iso_tab;
	uint32_t len;
};

struct sensor_ae_tab {
	struct sensor_block_fix_param block;
	struct ae_exp_gain_tab ae_tab[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_tab weight_tab[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_exp_gain_tab scene_tab[AE_SCENE_NUM][AE_FLICKER_NUM];
	struct ae_auto_iso_tab_v1 auto_iso_tab[AE_FLICKER_NUM];
};

/*******************************new***************/

struct sensor_lens_map_info {
	uint32_t envi;
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
};

struct sensor_lens_map {
	uint32_t *map_info;
	uint32_t map_info_len;
	uint16_t *lnc_addr;
	uint32_t lnc_len;
};

struct sensor_lsc_map {
	struct sensor_block_fix_param block;
	struct sensor_lens_map map[9];
};

struct sensor_awb_map {
	uint16_t *addr;
	uint32_t len;		//by bytes
};

struct sensor_awb_weight {
	uint8_t *addr;
	uint32_t weight_len;
	uint16_t *size;
	uint32_t size_param_len;
};

struct sensor_awb_map_weight_param {
	struct sensor_block_fix_param block;
	struct sensor_awb_map awb_map;
	struct sensor_awb_weight awb_weight;
};

struct sensor_raw_fix_info {
	struct sensor_mode_fix_param mode;
	struct sensor_ae_tab ae;
	struct sensor_lsc_map lnc;
	struct sensor_awb_map_weight_param awb;
};

struct sensor_raw_note_info {
	uint8_t *note;
	uint32_t node_len;
};

struct sensor_raw_info {
	struct sensor_version_info *version_info;
	struct isp_mode_param_info mode_ptr[MAX_MODE_NUM];
	struct sensor_raw_resolution_info_tab *resolution_info_ptr;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct sensor_libuse_info *libuse_info;
	struct sensor_raw_fix_info *fix_ptr[MAX_MODE_NUM];
	struct sensor_raw_note_info note_ptr[MAX_MODE_NUM];
};

struct sensor_otp_module_info {
	cmr_u8 year;
	cmr_u8 month;
	cmr_u8 day;
	cmr_u8 mid;
	cmr_u8 lens_id;
	cmr_u8 vcm_id;
	cmr_u8 driver_ic_id;
	cmr_u8 factory_id;
	cmr_u8 calibration_version;
	cmr_u8 ir_bg_id;
	cmr_u8 ois_id;
	cmr_u8 cal_direction;
	cmr_u8 section_size;
};

struct sensor_otp_iso_awb_info {
	cmr_u16 iso;
	cmr_u16 gain_r;
	cmr_u16 gain_g;
	cmr_u16 gain_b;
	cmr_u8 section_size;
};

struct sensor_otp_lsc_info {
	cmr_u8 *lsc_data_addr;
	cmr_u16 lsc_data_size;
};

struct sensor_otp_af_info {
	cmr_u8 flag;
	cmr_u16 infinite_cali;
	cmr_u16 macro_cali;
	cmr_u8 section_size;
};
struct sensor_otp_pdaf_info {
	cmr_u8 *pdaf_data_addr;
	cmr_u16 pdaf_data_size;
};

struct sensor_otp_cust_info {
	cmr_u8 program_flag;
	struct sensor_otp_module_info module_info;
	struct sensor_otp_iso_awb_info isp_awb_info;
	struct sensor_otp_lsc_info lsc_info;
	struct sensor_otp_af_info af_info;
	struct sensor_otp_pdaf_info pdaf_info;
	cmr_u16 checksum;
};

struct sensor_data_info {
	void *data_ptr;
	uint32_t size;
};

struct sensor_dual_otp_info {
	struct sensor_data_info dual_otp;
	struct sensor_data_info master_slave_otp;
	struct sensor_data_info data_3d;
};
#endif
