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

typedef void (*isp_buf_cfg_evt_cb)(cmr_int evt, void *data, cmr_u32 data_len,
                                   void *privdata);
enum sns_cmd_section { CMD_SNS_OTP, CMD_SNS_IC, CMD_SNS_AF };

enum sns_cmd_section_start {
    CMD_SNS_OTP_START = CMD_SNS_OTP << 8,
    CMD_SNS_IC_START = CMD_SNS_IC << 8,
    CMD_SNS_AF_START = CMD_SNS_AF << 8,
};

enum sns_cmd {
    // OTP_DATA_COMPATIBLE_CONVERT
    CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT =
        CMD_SNS_OTP_START, /*include 256 sub OTP cmd*/
    CMD_SNS_OTP_GET_VENDOR_ID,

    CMD_SNS_IC_DEFAULT = CMD_SNS_IC_START, /*include 256 sub IC cmd*/
    CMD_SNS_IC_WRITE_MULTI_AE,
    CMD_SNS_IC_GET_EBD_PARSE_DATA,
    CMD_SNS_IC_GET_CCT_DATA,

    CMD_SNS_AF_SET_BEST_MODE = CMD_SNS_AF_START, /*include 256 sub AF cmd*/
    CMD_SNS_AF_GET_TEST_MODE,
    CMD_SNS_AF_SET_TEST_MODE,
    CMD_SNS_AF_GET_POS_INFO
};

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
struct sensor_raw_resolution_info {
    cmr_u16 start_x;
    cmr_u16 start_y;
    cmr_u16 width;
    cmr_u16 height;
    cmr_u32 line_time;
    cmr_u32 frame_line;
};

struct sensor_vcm_info {
    uint16_t pos;
    uint16_t slave_addr;
    uint16_t cmd_len;
    uint8_t cmd_val[8];
};

struct af_pose_dis {
    cmr_u32 up2hori;
    cmr_u32 hori2down;
};

struct drv_fov_info {
    float physical_size[2];
    float focal_lengths;
};

struct sensor_ex_info {
    cmr_u32 f_num;
    cmr_u32 focal_length;
    cmr_u32 max_fps;
    cmr_u32 max_adgain;
    cmr_u32 ois_supported;
    cmr_u32 pdaf_supported;
    cmr_u32 embedded_line_enable;
    cmr_u32 exp_valid_frame_num;
    cmr_u32 clamp_level;
    cmr_u32 adgain_valid_frame_num;
    cmr_u32 preview_skip_num;
    cmr_u32 capture_skip_num;
    struct drv_fov_info fov_info;
    cmr_s8 *name;
    cmr_s8 *sensor_version_info;
    struct af_pose_dis pos_dis;
    cmr_u32 *sns_binning_factor;
    cmr_u32 cct_supported;
    cmr_u32 tof_supported;
};

struct sensor_raw_resolution_info_tab {
    cmr_u32 image_pattern;
    struct sensor_raw_resolution_info tab[10];
};

struct sensor_raw_ioctrl {
    cmr_handle caller_handler;
    cmr_int (*set_focus)(cmr_handle caller_handler, cmr_u32 param);
    cmr_int (*get_pos)(cmr_handle caller_handler,
                       struct sensor_vcm_info *param);
    cmr_int (*set_exposure)(cmr_handle caller_handler, cmr_u32 param);
    cmr_int (*set_gain)(cmr_handle caller_handler, cmr_u32 param);
    cmr_int (*ext_fuc)(cmr_handle caller_handler, void *param);
    cmr_int (*write_i2c)(cmr_handle caller_handler, cmr_u16 slave_addr,
                         cmr_u8 *cmd, cmr_u16 cmd_length);
    cmr_int (*read_i2c)(cmr_handle caller_handler, cmr_u16 slave_addr,
                        cmr_u8 *cmd, cmr_u16 cmd_length);
    cmr_int (*ex_set_exposure)(cmr_handle caller_handler, cmr_uint param);
    cmr_int (*read_aec_info)(cmr_handle caller_handler, void *param);
    cmr_int (*write_aec_info)(cmr_handle caller_handler, void *param);
#if 1
    // af control and DVT test funcs valid only af_enable works
    cmr_int (*set_pos)(cmr_handle caller_handler, cmr_u32 pos);
    cmr_int (*get_otp)(cmr_handle caller_handler, uint16_t *inf,
                       uint16_t *macro);
    cmr_int (*get_motor_pos)(cmr_handle caller_handler, cmr_u16 *pos);
    cmr_int (*set_motor_bestmode)(cmr_handle caller_handler);
    cmr_int (*get_test_vcm_mode)(cmr_handle caller_handler);
    cmr_int (*set_test_vcm_mode)(cmr_handle caller_handler, char *vcm_mode);
#endif
    cmr_int (*sns_ioctl)(cmr_handle caller_handler, enum sns_cmd cmd,
                         void *param);
};

struct sensor_data_info {
    void *data_ptr;
    cmr_u32 size;
    void *sub_data_ptr;
    cmr_u32 sub_size;
    struct sensor_raw_info *sn_raw_info;
    struct isp_data_info isp_init_data[MAX_MODE_NUM];
    struct isp_data_info isp_update_data[MAX_MODE_NUM]; /*for isp_tool*/
    cmr_u8 dualcam_cali_lib_type;
};

struct sensor_otp_data_info {
    cmr_u32 data_size;
    void *data_addr;
};

struct sensor_otp_section_info {
    struct sensor_otp_data_info rdm_info;
    struct sensor_otp_data_info gld_info;
};

struct sensor_otp_module_info {
    cmr_u8 year;
    cmr_u8 month;
    cmr_u8 day;
    cmr_u8 mid;
    cmr_u8 lens_id;
    cmr_u8 vcm_id;
    cmr_u8 driver_ic_id;
};

struct sensor_otp_iso_awb_info {
    cmr_u16 iso;
    cmr_u16 gain_r;
    cmr_u16 gain_g;
    cmr_u16 gain_b;
};

struct sensor_otp_lsc_info {
    cmr_u8 *lsc_data_addr;
    cmr_u16 lsc_data_size;
    cmr_u32 full_img_width;
    cmr_u32 full_img_height;
    cmr_u32 lsc_otp_grid;
};

struct sensor_otp_ae_info {
    cmr_u16 ae_target_lum;
    cmr_u64 gain_1x_exp;
    cmr_u64 gain_2x_exp;
    cmr_u64 gain_4x_exp;
    cmr_u64 gain_8x_exp;
    cmr_u64 reserve;
};

struct sensor_otp_awb_info {
    cmr_u32 otp_golden_r;
    cmr_u32 otp_golden_g;
    cmr_u32 otp_golden_b;
    cmr_u32 otp_random_r;
    cmr_u32 otp_random_g;
    cmr_u32 otp_random_b;
};

struct sensor_otp_af_info {
    cmr_u8 flag;
    cmr_u16 infinite_cali;
    cmr_u16 macro_cali;
    cmr_u16 afc_direction;
    /*for dual camera*/
    cmr_s32 vcm_step;
    cmr_u16 vcm_step_min;
    cmr_u16 vcm_step_max;
    cmr_u16 af_60cm_pos;
};

struct sensor_otp_pdaf_info {
    cmr_u8 *pdaf_data_addr;
    cmr_u16 pdaf_data_size;
};

struct point {
    uint16_t x;
    uint16_t y;
};

struct sensor_otp_optCenter_info {
    struct point R;
    struct point GR;
    struct point GB;
    struct point B;
};

enum otp_vendor_type {
    OTP_VENDOR_SINGLE = 0,      /*ONE CAMERA SENSOR ONE OTP*/
    OTP_VENDOR_SINGLE_CAM_DUAL, /*TWO CAMERA SENSOR AND ONE OTP,JUST FOR DUAL
                                   CAMERA*/
    OTP_VENDOR_DUAL_CAM_DUAL,   /*TWO CAMERA SENSOR AND TWO OTP,JUST FOR DUAL
                                   CAMERA*/
    OTP_VENDOR_NONE,            /*CAMERA NOT SOPPROT OTP*/
    OTP_VENDOR_MAX
};

struct sensor_single_otp_info {
    cmr_u8 program_flag;
    struct sensor_otp_section_info *module_info;
    struct sensor_otp_section_info *iso_awb_info;
    struct sensor_otp_section_info *lsc_info;
    struct sensor_otp_section_info *af_info;
    struct sensor_otp_section_info *pdaf_info;
    /*spc:sesor pixel calibration,used by pdaf*/
    struct sensor_otp_section_info *spc_info;
    struct sensor_otp_section_info *optical_center_info;
    struct sensor_otp_section_info *ae_info;
    cmr_u16 checksum;
};

struct sensor_dual_otp_info {
    cmr_u8 dual_flag; /*for 3ddata calibration flag*/
    struct sensor_data_info data_3d;
    struct sensor_otp_section_info *master_module_info;
    struct sensor_otp_section_info *master_af_info;
    struct sensor_otp_section_info *master_iso_awb_info;
    struct sensor_otp_section_info *master_optical_center_info;
    struct sensor_otp_section_info *master_lsc_info;
    struct sensor_otp_section_info *master_pdaf_info;
    struct sensor_otp_section_info *master_ae_info;
    struct sensor_otp_section_info *master_dualcam_info;
    struct sensor_otp_section_info *slave_module_info;
    struct sensor_otp_section_info *slave_af_info;
    struct sensor_otp_section_info *slave_iso_awb_info;
    struct sensor_otp_section_info *slave_optical_center_info;
    struct sensor_otp_section_info *slave_lsc_info;
    struct sensor_otp_section_info *slave_ae_info;
};
struct sensor_otp_cust_info {
    struct sensor_data_info total_otp;
    enum otp_vendor_type otp_vendor;
    struct sensor_single_otp_info single_otp;
    struct sensor_dual_otp_info dual_otp;
};

enum sensor_pdaf_type {
    SENSOR_PDAF_DISABLED = 0,
    SENSOR_PDAF_TYPE1_ENABLE,
    SENSOR_PDAF_TYPE2_ENABLE,
    SENSOR_PDAF_TYPE3_ENABLE,
    SENSOR_DUAL_PDAF_ENABLE,
    SENSOR_PDAF_MAX
};

enum sensor_vendor_type {
    SENSOR_VENDOR_SS_BEGIN,
    SENSOR_VENDOR_S5K3L8XXM3,
    SENSOR_VENDOR_S5K3P8SM,
    SENSOR_VENDOR_SS_END,

    SENSOR_VENDOR_IMX_BEGIN,
    SENSOR_VENDOR_IMX258,
    SENSOR_VENDOR_IMX258_TYPE2,
    SENSOR_VENDOR_IMX258_TYPE3,
    SENSOR_VENDOR_IMX362_DUAL_PD,
    SENSOR_VENDOR_IMX_END,

    SENSOR_VENDOR_OV_BEGIN,
    SENSOR_VENDOR_OV13855,
    SENSOR_VENDOR_OV12A10,
    SENSOR_VENDOR_OV_END,
};

struct pd_pos_info {
    cmr_u16 pd_pos_x;
    cmr_u16 pd_pos_y;
};

enum {
    DATA_RAW10,
    DATA_BYTE2,
};

struct sensor_pdaf_type2_info {
    cmr_u32 data_type;
    cmr_u32 data_format;
    cmr_u32 width;
    cmr_u32 height;
    cmr_u32 pd_size;
};

struct pd_vch2_info {
    cmr_u32 bypass;
    cmr_u32 vch2_vc;
    cmr_u32 vch2_data_type;
    cmr_u32 vch2_mode;
};

struct sensor_pdaf_info {
    cmr_u16 pd_offset_x;
    cmr_u16 pd_offset_y;
    cmr_u16 pd_pitch_x;
    cmr_u16 pd_pitch_y;
    cmr_u16 pd_density_x;
    cmr_u16 pd_density_y;
    cmr_u16 pd_block_num_x;
    cmr_u16 pd_block_num_y;
    cmr_u16 pd_pos_size;
    struct pd_pos_info *pd_pos_r;
    struct pd_pos_info *pd_pos_l;
    cmr_u16 pd_end_x;
    cmr_u16 pd_end_y;
    cmr_u16 pd_block_w;
    cmr_u16 pd_block_h;
    cmr_u16 *pd_is_right;
    cmr_u16 *pd_pos_row;
    cmr_u16 *pd_pos_col;
    enum sensor_vendor_type vendor_type;
    cmr_u32 data_type;
    struct sensor_pdaf_type2_info type2_info;
    cmr_u32 sns_orientation; // 0: Normal, 1:Mirror+Flip
    cmr_u32 *sns_mode;       // sensor mode for pd
    struct pd_vch2_info vch2_info;
};

struct sensor_ebd_data_info {
    cmr_u32 data_type;
    cmr_u32 data_format;
    cmr_u32 data_size;
};

struct ebd_vch_info {
    cmr_u32 bypass;
    cmr_u32 vch_id;
    cmr_u32 vch_data_type;
    cmr_u32 vch_mode; // 0: none;for pdaf type3 1: use datatype 2:use vch
};

struct ebd_parse_data {
    cmr_u8 frame_count;
    cmr_u16 shutter;
    cmr_u16 again;
    cmr_u16 dgain_gr;
    cmr_u16 dgain_r;
    cmr_u16 dgain_b;
    cmr_u16 dgain_gb;
    cmr_uint gain;
};

struct sensor_embedded_info {
    cmr_u16 frame_count_valid;
    cmr_u16 shutter_valid;
    cmr_u16 again_valid;
    cmr_u16 dgain_valid;
    struct ebd_parse_data parse_data;
    struct ebd_vch_info vc_info;
    struct sensor_ebd_data_info embedded_data_info;
    cmr_u8 *embedded_data;
    cmr_u32 *sns_mode; // sensor mode for ebd
};

struct sensor_4in1_info {
    cmr_u32 is_4in1_supported;
    cmr_u32 limited_4in1_width;
    cmr_u32 limited_4in1_height;
    cmr_u32 *sns_mode; // sensor mode for 4in1
};

struct frame_4in1_info {
    cmr_int im_addr_in;
    cmr_int im_addr_out;
};

struct sensor_ex_exposure {
    cmr_u32 exposure;
    cmr_u32 dummy;
    cmr_u32 size_index;
};

struct sensor_i2c_reg_tab {
    struct sensor_reg_tag *settings;
    uint16_t size;
};

struct sensor_aec_i2c_tag {
    uint16_t slave_addr;
    uint16_t addr_bits_type;
    uint16_t data_bits_type;
    struct sensor_i2c_reg_tab *shutter;
    struct sensor_i2c_reg_tab *again;
    struct sensor_i2c_reg_tab *dgain;
    struct sensor_i2c_reg_tab *frame_length;
};

struct sensor_aec_reg_info {
    struct sensor_ex_exposure exp;
    cmr_u32 gain;
    struct sensor_aec_i2c_tag *aec_i2c_info_out;
};

struct sensor_multi_ae_info {
    cmr_int camera_id;
    cmr_handle handle;
    cmr_u32 count;
    cmr_u32 gain;
    struct sensor_ex_exposure exp;
};

#endif
