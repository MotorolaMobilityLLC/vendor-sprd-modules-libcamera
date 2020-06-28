#ifndef __TEST_DRV_ISP_PARAMETER_H__
#define __TEST_DRV_ISP_PARAMETER_H__

#include "json2drv.h"
#include "../framework/test_common_header.h"
#include <deque>
using namespace std;
typedef signed char int8;
typedef signed short int16;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long  uint64;


//int isp_init_config(struct host_info_t *host_info)
//const char * isp_parameter_place_to_param_name(unsigned int place);

#define POSTERIZE_NUM 129
#define CROP 0
#define REMOVE_OVERLAP_ONLY 1
//#define __CHIP_L5__
#define STRING_SIZE		256
#define IMAGE_LIST_SIZE	100
#define SLICE_SIZE_MAX		100
#define PROCESS_QUEUE_MAX				100
#define MAX_BADPIXEL_NUM				0x800000//0x100000
#define PINGPONG						1
#define MAX_FUSION_FRAME_NUM			10
#define LTM_MAX_BIN						8*8*64
#define LTM_TILE_BIN					128
#define ZZHDR_NUM_HIST_BINS				128
#define GTM_HIST_BIN_NUM            128
#define MAX_SLICE_NUM                              16

#define HOR_SUB_WIN_MAX_NUM 20
#define VER_SUB_WIN_MAX_NUM 15
#define HOR_SUB_WIN_MIN_NUM 4
#define VER_SUB_WIN_MIN_NUM 3

#define FALSE 0
#define TRUE 1

#define ISP_DDR_BASE				(0x80000000)
#define ISP_DDR_LEN					(0x40000000) //(0x20000000)
#define ISP_DDR_MAX					(ISP_DDR_BASE + ISP_DDR_LEN)

enum EXIT_STATUS {
	EXIT_PARAM_INVALID = 0x10,
	EXIT_FILE_INVALID,
	EXIT_DDR_ACCESS_INVALID,
	EXIT_TYPE_INVALID,
	EXIT_MEM_INVALID,
	EXIT_CMDL_INVALID,
	EXIT_CALL_INVALID,
	EXIT_DDR_OVERFLOW,
};

enum MemoryType
{
	REG,
	DDR,
};

enum LTM_ID {
	LTM_RGB,
	LTM_YUV,
	LTM_NUM,
};

enum ISP_PATH_ID {
	ISP_PATH_P0,
	ISP_PATH_P1,
	ISP_PATH_C0,
	ISP_PATH_C1,
	ISP_PATH_NUM
};

enum DCAM_PATH_ID {
	DCAM_PATH_0,
	DCAM_PATH_1,
	DCAM_PATH_2,
	DCAM_PATH_NUM
};

enum SCALER_ID {
	SCALER_CAP_PRE,
	SCALER_VID,
	SCALER_NUM,
};

enum PINGPONG_ID{
	PING,
	PANG,
	PINGPANG,
};

struct aem_data_info{
	unsigned int aem_ue;  // g/r/b channel under exposure
	unsigned int aem_ae;  // g/r/b channel normal exposure
	unsigned int aem_oe; // g/r/b channel over exposure
	unsigned int aem_ue_cnt; // g/r/b channel under exposure count
	unsigned int aem_oe_cnt;// g/r/b channel over exposure count
};

struct afm_data_info{
	unsigned int afm_fv0;
	unsigned int afm_fv1;
	unsigned int afm_lum;
	unsigned int reserve;
};

typedef struct _int_slice {
	uint32 int_clr0;
} isp_fw_int_slice;

typedef struct _int_block {
	uint32	int_en0;
	uint32	all_done_src_ctrl;
	uint32	all_done_ctrl;
	uint32	skip_ctrl_en;
    uint32	sdw_done_int_cnt_num;
    uint32	sdw_done_skip_cnt_num;
    uint32	all_done_int_cnt_num;
    uint32	all_done_skip_cnt_num;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_int_slice *slice_param;
#endif
} isp_fw_int_block;

typedef struct _cfg_block {
	uint32 cfg_en;
	uint32 cfg_addr;
	uint32 cfg_vaddr;
	uint32 tm_bypass;
	uint32 cap_cmd_ready_mode;
	uint32 sdw_mode;
	uint32 pixel_ready;
	uint32 auto_cg_en;
} isp_fw_cfg_block;

typedef struct _arbiter_block {
	uint32	fetch_bit_reorder;
	uint32	fetch_endian;
	uint32	word_change;
} isp_fw_arbiter_block;

typedef struct _common_slice {
	uint32 nr3_path_sel;
	uint32 store_out_path_sel;
	uint32 scl_pre_cap_path_sel;
	uint32 scl_vid_path_sel;
	uint32	scl_thu_path_sel;
	uint32	fetch_path_sel;
} isp_fw_common_slice;

typedef struct _common_block {
	uint32	fetch_color_space_sel;
	uint32	store_color_space_sel;
	uint32	fmcu0_path_sel;
	uint32	fmcu1_path_sel;
	uint32	nr3_path_sel;
	uint32	storeB_path_sel;
	uint32	scl_pre_cap_path_sel;
	uint32	scl_vid_path_sel;
	uint32	scl_thu_path_sel;
	uint32	cg_dis[4];
	uint32	gclk_ctrl[4];
	uint32	fetch_path_sel;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_common_slice *slice_param;
#endif
} isp_fw_common_block;

typedef struct _store_slice {
	uint32 store_addr[4][3];
	uint32 slice_out_width;
	uint32 slice_out_height;
	uint32 overlap_up;
	uint32 overlap_down;
	uint32 overlap_left;
	uint32 overlap_right;
	uint32 shadow_clr;
	uint32 slice_offset;
} isp_fw_store_slice;

typedef struct _store_block {
	uint32	store_bypass;
	uint32	domain_sel;
	uint32	store_shadow_clr_sel;
	uint32  YUVOutputFormat;
	uint32	mono_color_mode;
	uint32	store_color_format;
	uint32	store_endian;
	uint32	store_width;
	uint32  store_height;
	uint32  store_addr[4][3];
	uint32  store_pitch[3];
	uint32	store_size[3];
	uint32	mirror_en;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_store_slice *slice_param;
#endif
	uint32	speed_2x;
	uint32	total_word;
	uint32	path_sel;
	uint32	base_align;
	uint32	burst_len_sel;
	uint32	trim_en;
	uint32	trim_start_x;
	uint32	trim_start_y;
	uint32	trim_size_x;
	uint32	trim_size_y;
	uint32	FBC_enable;
	uint32	FBC_const_color_en;
	uint32	FBC_const_color_0;
	uint32	FBC_const_color_1;
	uint32	normal_store_en;
} isp_fw_store_block;

typedef struct _fetch_slice {
	uint32 slice_addr[3];
	uint32 slice_width;
	uint32 slice_height;
	uint32 mipi_byte_info;
	uint32 mipi_word_info;
} isp_fw_fetch_slice;

typedef struct _fetch_block {
	uint32	fetch_bypass;
	uint32	fetch_width;
	uint32	fetch_height;
	uint32  fetch_addr[3];
	uint32	fetch_size[3];
	uint32  fetch_pitch[3];
	uint32  YUVInputFormat;
	uint32	fetch_color_format;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_fetch_slice *slice_param;
#endif
	uint32	general_crop_en;
	uint32	general_crop_mode;
	uint32	general_crop_start_x;
	uint32	general_crop_start_y;
	uint32	general_crop_width;
	uint32	general_crop_height;
	uint32	hblank_num;
	uint32	hblank_num_en;
	uint32	yuv420to422_bypass;
} isp_fw_fetch_block;

typedef struct _dispatch_slice {
	uint32	RAW_MODE;
	uint32	slice_width;
	uint32	slice_height;
} isp_fw_dispatch_slice;

typedef struct _dispatch_block {
	uint32	dispatch_height_dly_num_ch0;
	uint32	dispatch_width_dly_num_ch0;
	uint32	dispatch_dbg_mode_ch0;
	uint32	dispatch_ready_width_ch0;
	uint32	dispatch_nready_cfg_ch0;
	uint32	dispatch_nready_width_ch0;
	uint32	dispatch_width_flash_mode;
	uint32	dispatch_width_dly_num_flash;
	uint32	dispatch_done_line_dly_num;
	uint32	dispatch_pipe_nfull_num;
	uint32	dispatch_pipe_flush_num;
	uint32	dispatch_pipe_hblank_num;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_dispatch_slice *slice_param;
#endif
} isp_fw_dispatch_block;

typedef struct _fmcu_block {
	uint32	FMCU_mode;
	uint32  FMCU_SEL;
	//uint32  FMCU1_SEL;
	uint32	cmd_num[IMAGE_LIST_SIZE];
	uint32	addr[IMAGE_LIST_SIZE];
	uint32	unit_num;
	uint32	slow_motion_cfg_count;
	uint32	slow_motion_cycle_num;
	uint32	fmcu_check;
} isp_fw_fmcu_block;

typedef struct _iommu_block {
	uint32 IOMMU_en;
       uint64 page_table_address;
	uint32 page_length;
	uint64 start_virtual_address;
	uint32 default_err_page_number;
} isp_fw_iommu_block;

typedef struct _slice_pos {
	uint32	start_col;
	uint32	start_row;
	uint32	end_col;
	uint32	end_row;
} isp_fw_slice_pos;

typedef struct _slice_overlap {
	uint32	overlap_up;
	uint32	overlap_down;
	uint32	overlap_left;
	uint32	overlap_right;
} isp_fw_slice_overlap;

typedef struct _PipeLine_Size{
	uint32 image_width;
	uint32 image_height;
	uint32 slice_width_left;
	uint32 slice_width_right;
	uint32 slice_bypass[SLICE_SIZE_MAX];
	isp_fw_slice_pos slice_pos[SLICE_SIZE_MAX];
	isp_fw_slice_overlap slice_overlap_array[SLICE_SIZE_MAX];
}PipeLine_Size;

typedef struct _general_block {
	uint32  input_mode;
	uint32  output_mode;
	uint32  img_num;
	uint32	ProcessingMode;
	uint32	fetch_endian;
	uint32	byte_format;
	uint32	mipi_endian;
	uint32  general_image_bayer_mipi;
	uint32	bayer_pattern;
	uint32  isp_level;
	//config size
	uint32  general_image_width;
	uint32  general_image_height;
	//raw domain size
	uint32	raw_width;
	uint32	raw_height;
	//rgb/y domain size
	uint32	rgb_width;
	uint32	rgb_height;
	//yuv domain size
	uint32	yuv_width;
	uint32	yuv_height;
    //fast stop
    uint32	Fast_stop_en;
	//process queue
	uint32	start_num;
	uint32	MAX_PROCESS_NUM;
	uint32	process_queue[PROCESS_QUEUE_MAX];
	uint32	call_queue[PROCESS_QUEUE_MAX];
	uint32	general_image_bayer_mode;
	//for fw self check
	uint32	general_param_lowlevel_en;
	uint32	general_vector_rtl_level;
	//uint32	dcam_bypass;
	uint32  general_offline_cfg_overlap_en;
	uint32  general_offline_cfg_overlap_left;
	uint32  general_offline_cfg_overlap_right;
	uint32  general_offline_cfg_overlap_up;
	uint32  general_offline_cfg_overlap_down;
	uint32  general_image_bit_depth;
	//pipeline_size
	PipeLine_Size before_crop0_size;
	PipeLine_Size after_crop0_size;
	PipeLine_Size after_crop1_size;
	PipeLine_Size after_crop2_size;
	PipeLine_Size after_crop3_size;
	PipeLine_Size after_rds_size;

} isp_fw_general_block;

typedef struct _frame_pos {
	uint32	start_col;
	uint32	start_row;
	uint32	end_col;
	uint32	end_row;
} isp_fw_frame_pos;


typedef struct _slice {
	isp_fw_slice_pos			slice_pos_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap		slice_overlap_array[SLICE_SIZE_MAX];
	isp_fw_slice_pos			yuv_slice_pos_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap		yuv_slice_overlap_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap		overlap_array[SLICE_SIZE_MAX];

	uint32	slice_row_num;
	uint32	slice_col_num;
	uint32	slice_num;
	uint32	slice_height;
	uint32	slice_width;

	uint32	overlap_up;
	uint32	overlap_down;
	uint32	overlap_left;
	uint32	overlap_right;

	uint32	raw_overlap_up;
	uint32	raw_overlap_down;
	uint32	raw_overlap_left;
	uint32	raw_overlap_right;
	uint32	yuv_overlap_up;
	uint32	yuv_overlap_down;
	uint32	yuv_overlap_left;
	uint32	yuv_overlap_right;
	uint32	rgb10_overlap_up;
	uint32	rgb10_overlap_down;
	uint32	rgb10_overlap_left;
	uint32	rgb10_overlap_right;
	char	overlap_file[STRING_SIZE];
} isp_fw_slice;

typedef struct _dcam_slice_part_info {
	uint32 slice_en;
	uint32 slice_mode;
	uint32 slice_part;
}dcam_slice_part_info;

typedef struct _dcam_slice {
	isp_fw_slice_pos slice_pos_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap slice_overlap_array[SLICE_SIZE_MAX];

	isp_fw_slice_pos yuv_slice_pos_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap yuv_slice_overlap_array[SLICE_SIZE_MAX];

	dcam_slice_part_info dcam_slice_part[SLICE_SIZE_MAX];
	uint32 slice_row_num;
	uint32 slice_col_num;
	uint32 slice_num;
	uint32 slice_height;
	uint32 slice_width;

	uint32 overlap_up;
	uint32 overlap_down;
	uint32 overlap_left;
	uint32 overlap_right;

	uint32 raw_overlap_up;
	uint32 raw_overlap_down;
	uint32 raw_overlap_left;
	uint32 raw_overlap_right;
	uint32 yuv_overlap_up;
	uint32 yuv_overlap_down;
	uint32 yuv_overlap_left;
	uint32 yuv_overlap_right;
	uint32 rgb10_overlap_up;
	uint32 rgb10_overlap_down;
	uint32 rgb10_overlap_left;
	uint32 rgb10_overlap_right;
	char overlap_file[STRING_SIZE];
	uint32 slice_mode;
} dcam_fw_slice;

typedef struct _dcam_slice_after_crop0 {
	isp_fw_slice_pos			slice_pos_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap		slice_overlap_array[SLICE_SIZE_MAX];

	isp_fw_slice_pos			yuv_slice_pos_array[SLICE_SIZE_MAX];
	isp_fw_slice_overlap		yuv_slice_overlap_array[SLICE_SIZE_MAX];

	//isp_fw_slice_overlap		overlap_array[SLICE_SIZE_MAX];
	dcam_slice_part_info		dcam_slice_part[SLICE_SIZE_MAX];
	uint32	slice_row_num;
	uint32	slice_col_num;
	uint32	slice_num;
	uint32	slice_height;
	uint32	slice_width;

	uint32	overlap_up;
	uint32	overlap_down;
	uint32	overlap_left;
	uint32	overlap_right;

	uint32	raw_overlap_up;
	uint32	raw_overlap_down;
	uint32	raw_overlap_left;
	uint32	raw_overlap_right;
	uint32	yuv_overlap_up;
	uint32	yuv_overlap_down;
	uint32	yuv_overlap_left;
	uint32	yuv_overlap_right;
	uint32	rgb10_overlap_up;
	uint32	rgb10_overlap_down;
	uint32	rgb10_overlap_left;
	uint32	rgb10_overlap_right;
	char	overlap_file[STRING_SIZE];
    uint32	slice_mode;
} dcam_fw_slice_after_crop0;


typedef struct _ppg_block {
	uint32	bypass;
	uint32	gain;
} isp_fw_pgg_block;

typedef struct _blc_block {
	uint32	bypass;
	uint32	Red_cal;
	uint32	Gr_cal;
	uint32	Gb_cal;
	uint32	Blue_cal;
	uint32	post_bypass;
	uint32	post_Red_cal;
	uint32	post_Gr_cal;
	uint32	post_Gb_cal;
	uint32	post_Blue_cal;
} isp_fw_blc_block;

typedef struct _nlm_slice {
	uint32 center_y_relative_slice_row;
	uint32 center_x_relative_slice_col;
} isp_fw_nlm_slice;

typedef struct _grgb_imbalance {
	uint32 gc2_radial_1d_center_x;
	uint32 gc2_radial_1d_center_y;
	uint32 gc2_global_x_start;
	uint32 gc2_global_y_start;
	uint32 slice_width;
} isp_fw_grgb_imbalance_slice;

typedef struct _nlm_block {
	uint32  NLM_bypass;
	uint32	NLM_dist_mode;
	uint32	impulsive_optimize_en;
	uint32	flat_optimize_en;
	uint32	NLM_simple_bpc_en;
	uint32	NLM_simple_bpc_threshold;
	uint32	NLM_simple_bpc_lum_threshold;
	uint32	NLM_flag_first_lum_en;
	uint32	NLM_lum_threshold0;
	uint32	NLM_lum_threshold1;

	//[lum][flat]
	uint32	NLM_first_lum_flat_thresh[3][3];
	uint32	NLM_first_lum_flat_match_count[3][3];
	uint32	NLM_first_lum_flat_increse_strenth[3][3];
	uint32	NLM_first_lum_texture_decrease_strenth[3];

	uint32	NLM_first_lum_second_lum_addback[3][4][2];
	uint32	NLM_first_lum_second_lum_addback_noise_clip_max[3][4];
	uint32	NLM_first_lum_second_lum_addback_noise_clip_min[3][4];

	uint32	NLM_cal_radius_en;
	uint32	NLM_update_flat_thr_en;
	uint32	NLM_direction_mode_en;
	uint32	NLM_direct_diff_th;
	uint32	NLM_direct_cnt_th;
	uint32	NLM_tdist_min_th;
	uint32	NLM_w_shift0;
	uint32	NLM_w_shift1;
	uint32	NLM_w_shift2;
	uint32	NLM_w[2][72];

	//add in sharkl3
	uint32	NLM_first_lum_flat_thresh_coef[3][3];
	uint32	NLM_first_lum_flat_thresh_max[3][3];

	uint32	NLM_radial_1D_center_x;
	uint32	NLM_radial_1D_center_y;
	uint32	NLM_radial_1D_radius_threshold;
	uint32	NLM_radial_1D_en;
	uint32	NLM_radial_1D_protect_gain_max;
	uint32	NLM_radial_1D_radius_threshold_filter_ratio[3][4];
	uint32	NLM_radial_1D_coef2[3][4];
	uint32	NLM_radial_1D_protect_gain_min[3][4];

	uint32	NLM_direction_addback_mode_en;
	uint32	NLM_first_lum_direction_addback[3][4];
	uint32	NLM_first_lum_direction_addback_noise_clip[3][4];

	char    nlm_weight_filename[2][STRING_SIZE];
	uint32  nlm_argument_number;
	// vst/ivst
	uint32  vst_en;
	uint32  vst_buf_sel;
	char    vst_filename[2][STRING_SIZE];
	char    vst_inv_filename[2][STRING_SIZE];
	uint32	vst_table[2][1027];
	uint32	vst_inv_table[2][1027];
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_nlm_slice *slice_param;
#endif
} isp_fw_nlm_block;

typedef struct _grgb_imblance {
	uint32 imblance_bypass ;
	uint32 imblance_flag12_frezthr ;
	uint32 imblance_flag3_grid ;
	uint32 imblance_flag3_lum ;
	uint32 imblance_flag3_frez ;
	uint32 imblance_lumth1 ;
	uint32 imblance_lumth2 ;
	//lum0
	uint32 imblance_hv_edge_thr0 ;
	uint32 imblance_slash_edge_thr0 ;
	uint32 imblance_hv_flat_thr0 ;
	uint32 imblance_slash_flat_thr0 ;
	uint32 imblance_S_baohedu01 ;
	uint32 imblance_S_baohedu02 ;
	uint32 imblance_lum0_flag2_r ;
	uint32 imblance_lum0_flag0_rs ;
	uint32 imblance_lum0_flag0_r ;
	uint32 imblance_lum0_flag1_r ;
	uint32 imblance_lum0_flag4_r ;
	uint32 imblance_lum0_flag3_r ;
	uint32 imblance_diff0 ;
	//lum1
	uint32 imblance_hv_edge_thr1 ;
	uint32 imblance_slash_edge_thr1 ;
	uint32 imblance_hv_flat_thr1 ;
	uint32 imblance_slash_flat_thr1 ;
	uint32 imblance_S_baohedu11 ;
	uint32 imblance_S_baohedu12 ;
	uint32 imblance_lum1_flag2_r ;
	uint32 imblance_lum1_flag0_rs ;
	uint32 imblance_lum1_flag0_r ;
	uint32 imblance_lum1_flag1_r ;
	uint32 imblance_lum1_flag4_r ;
	uint32 imblance_lum1_flag3_r ;
	uint32 imblance_diff1 ;
	//lum2
	uint32 imblance_hv_edge_thr2 ;
	uint32 imblance_slash_edge_thr2 ;
	uint32 imblance_hv_flat_thr2 ;
	uint32 imblance_slash_flat_thr2 ;
	uint32 imblance_S_baohedu21 ;
	uint32 imblance_S_baohedu22 ;
	uint32 imblance_lum2_flag2_r ;
	uint32 imblance_lum2_flag0_rs ;
	uint32 imblance_lum2_flag0_r ;
	uint32 imblance_lum2_flag1_r ;
	uint32 imblance_lum2_flag4_r ;
	uint32 imblance_lum2_flag3_r ;
	uint32 imblance_diff2  ;
	uint32 imblance_curve_wt0 ;
	uint32 imblance_curve_wt1 ;
	uint32 imblance_curve_wt2 ;
	uint32 imblance_curve_wt3 ;
	uint32 imblance_curve_wr0 ;
	uint32 imblance_curve_wr1 ;
	uint32 imblance_curve_wr2 ;
	uint32 imblance_curve_wr3 ;
	uint32 imblance_curve_wr4 ;
	uint32 imblance_sat_lumth ;
	uint32 imblance_radial_1D_en ;
	uint32 imblance_radial_1D_center_x ;
	uint32 imblance_radial_1D_center_y ;
	uint32 imblance_radial_1D_coef_r0  ;
	uint32 imblance_radial_1D_coef_r1  ;
	uint32 imblance_radial_1D_coef_r2  ;
	uint32 imblance_radial_1D_coef_r3  ;
	uint32 imblance_radial_1D_coef_r4  ;
	uint32 imblance_radial_1D_protect_ratio_max ;
	uint32 imblance_radial_1D_radius_threshold ;
	uint32 imblance_faceRmin ;
	uint32 imblance_faceRmax ;
	uint32 imblance_faceBmin ;
	uint32 imblance_faceBmax ;
	uint32 imblance_faceGmin ;
	uint32 imblance_faceGmax ;
	uint32 imblance_dump_map_en ;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_grgb_imbalance_slice *slice_param;
#endif

} isp_fw_grgb_imblance;

typedef struct _rgbg_slice {
	uint32 seed_init;
} isp_fw_rgbg_slice;

typedef struct _rgbg_block {
	uint32  cali_bypass;
	uint32  global_gain_cali;
	uint32  r_gain_cali;
	uint32  g_gain_cali;
	uint32  b_gain_cali;
	uint32	Pseudo_random_raw_enable;
	uint32	Pseudo_random_raw_range;
	uint32	Pseudo_random_reset_seed_slice;
	uint32     Pseudo_random_raw_seed_init_mode;
	uint32	seed;
	uint32	r_offset;
	uint32	r_shift;
	uint32	takebit[8];
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_rgbg_slice *slice_param;
#endif
} isp_fw_rgbg_block;

typedef struct _nlc_block {
	uint32	bypass;
	uint32	nlc_input[29];
	uint32	nlc_output_red[29];
	uint32	nlc_output_green[29];
	uint32	nlc_output_blue[29];
	char	nlc_file_name[STRING_SIZE];
} isp_fw_nlc_block;

typedef struct _lsc_2d_slice {
	uint32 start_col;
	uint32 start_row;
	uint32 relative_x;
	uint32 relative_y;
	uint32 slice_width;
	uint32 slice_height;
	uint32 grid_x;
	uint32 grid_y;
	uint32 grid_num;
	uint32 param_addr[2];
	uint16 q_val[2][5][2];
	uint32 grid_buf[2];
} isp_fw_lsc_2d_slice;

typedef struct _lsc_2d_block {
	uint32	bypass;
	uint32	LNC_GRID;
	uint32  buf_sel;
	uint32	grid_pitch;
	uint32	endian;
	int16	simple_table[129][3];
	uint32	weight_num;
	uint32	weight_table[258];
	char	full_r0c0_file[2][STRING_SIZE];
	char	full_r1c1_file[2][STRING_SIZE];
	char	full_r0c1_file[2][STRING_SIZE];
	char	full_r1c0_file[2][STRING_SIZE];
	uint32	lens_addr[2];
	uint32  buffer_size_bytes;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_lsc_2d_slice *slice_param;
#endif
} isp_fw_lsc_2d_block;

typedef struct _lsc_1d_slice {
	uint32 start_col;
	uint32 start_row;
	uint32 r[4];
	uint32 r2[4];
	uint32 dr2[4];
} isp_fw_lsc_1d_slice;

typedef struct _lsc_1d_block {
	uint32	bypass;
	uint32	center_r0c0_x;
	uint32	center_r0c1_x;
	uint32	center_r1c0_x;
	uint32	center_r1c1_x;
	uint32	center_r0c0_y;
	uint32	center_r0c1_y;
	uint32	center_r1c0_y;
	uint32	center_r1c1_y;
	char	radial_1D_lens_gain_file[2][STRING_SIZE];
	uint32	gain_r0c0[2][256];
	uint32	gain_r0c1[2][256];
	uint32	gain_r1c0[2][256];
	uint32	gain_r1c1[2][256];
	uint32	raidal_1D_radius_order;
	uint32	buf_sel;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_lsc_1d_slice *slice_param;
#endif
} isp_fw_lsc_1d_block;

typedef struct _awbc_block {
	uint32	bypass;
	uint32	awbc_rscale	;
	uint32	awbc_gbscale;
	uint32	awbc_grscale;
	uint32	awbc_bscale;
	uint32	awbc_roffset;
	uint32	awbc_gboffset;
	uint32	awbc_groffset;
	uint32	awbc_boffset;
	uint32	red_clipvalue;
	uint32	green_clipvalue;
	uint32	blue_clipvalue;
	uint32	awb_gain_cal_bypass;
	uint32	awbc_gain_r[IMAGE_LIST_SIZE];
	uint32	awbc_gain_gr[IMAGE_LIST_SIZE];
	uint32	awbc_gain_gb[IMAGE_LIST_SIZE];
	uint32	awbc_gain_b[IMAGE_LIST_SIZE];
} isp_fw_awbc_block;

typedef struct _binning_block {
	uint32	bypass;
	uint32	hx;
	uint32	vx;
#ifdef RUN_DUMP_DATA
	uint32	out_addr[IMAGE_LIST_SIZE];
#else
	uint32  out_addr[2];
#endif
	uint32  binning_data_long_bytes;
	uint32	ddr_wr_num;// 8bytes write once
	uint32	pixel_num;
	uint32	binning_entrance;
	uint32	skip_num;
	uint32	skip_mode;
} isp_fw_binning_block;

typedef struct _dcam_fw_aem_slice{
    uint32 aem_blk_sx;
    uint32 aem_blk_sy;
    uint32 aem_blk_num_hor;
    uint32 aem_blk_num_ver;
	uint32	ddr_base_addr[4][2];
}dcam_fw_aem_slice;

typedef struct _aem_block {
	uint32 aem_bypass;
	uint32 aem_skip_num;
	uint32 aem_continuous_mode;

    uint32 aem_blk_sx;
    uint32 aem_blk_sy;
    uint32 aem_blk_width;
    uint32 aem_blk_height;
    uint32 aem_avgshf;
    uint32 aem_avgshf_ue;
    uint32 aem_avgshf_oe;
    uint32 aem_blk_num_hor;
    uint32 aem_blk_num_ver;
    uint32 aem_r_low_thr;
    uint32 aem_r_high_thr;
    uint32 aem_g_low_thr;
    uint32 aem_g_high_thr;
    uint32 aem_b_low_thr;
    uint32 aem_b_high_thr;
    uint32 aem_short_r_low_thr;
    uint32 aem_short_r_high_thr;
    uint32 aem_short_g_low_thr;
    uint32 aem_short_g_high_thr;
    uint32 aem_short_b_low_thr;
    uint32 aem_short_b_high_thr;

	//g/r/b channel
	struct aem_data_info g_info[16384];
	struct aem_data_info r_info[16384];
	struct aem_data_info b_info[16384];
	//zzHDR short pattern g/r/b channel
	struct aem_data_info g_short_info[16384];
	struct aem_data_info r_short_info[16384];
	struct aem_data_info b_short_info[16384];

	uint32 aem_hdr_en;
	uint32 aem_hdr_zigzag_init_r;
	uint32 aem_hdr_zigzag_init_gr;
	uint32 aem_hdr_zigzag_init_b;

	uint32	ddr_base_addr[4][2];
	uint32	ddr_short_addr[4][2];
	uint32	ddr_wr_num;// 8bytes write once
	uint32  aem_pitch;

#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_aem_slice *slice_param;
#endif
} dcam_fw_aem_block;

typedef struct _dcam_fw_afm_slice{
	uint32	af_multi_win_x_st;
	uint32	af_multi_win_y_st;
	uint32	af_hor_win_num;
	uint32	af_ver_win_num;
	uint32	ddr_addr[2];
}dcam_fw_afm_slice;

typedef struct _raw_af_block {
	uint32	bypass;
	uint32	skip_num;
	uint32	afm_continuous_mode;
	uint32	af_multi_win_x_st;
	uint32	af_multi_win_x_en;
	uint32	af_multi_win_y_st;
	uint32	af_multi_win_y_en;
	uint32	af_multi_win_width;
	uint32	af_multi_win_height;
	uint32	af_hor_win_num;
	uint32	af_ver_win_num;
	uint32  IIR_denoise_en;
	uint32  IIR_g0;
	uint32  IIR_c1;
	uint32  IIR_c2;
	uint32  IIR_c3;
	uint32  IIR_c4;
	uint32  IIR_c5;
	uint32  IIR_g1;
	uint32  IIR_c6;
	uint32  IIR_c7;
	uint32  IIR_c8;
	uint32  IIR_c9;
	uint32  IIR_c10;
	uint32	af_enhanced_channel_sel;
	uint32	af_enhanced_denoise_mode;
	uint32	af_enhanced_center_weight;
	uint32  af_enhanced_clip_en_fv0;
	uint32  af_enhanced_clip_en_fv1;
	uint32  af_enhanced_max_th_fv0;
	uint32  af_enhanced_min_th_fv0;
	uint32  af_enhanced_max_th_fv1;
	uint32  af_enhanced_min_th_fv1;
	uint32  af_enhanced_shift_fv0;
	uint32  af_enhanced_shift_fv1;
	uint32  lum_stat_channel_sel;
	char	fv1_coeff[STRING_SIZE];
	int		mask_fv11[3][3];
	int		mask_fv12[3][3];
	int		mask_fv13[3][3];
	int		mask_fv14[3][3];
	//statis
	struct afm_data_info afm_data[VER_SUB_WIN_MAX_NUM][HOR_SUB_WIN_MAX_NUM];

	uint32	ddr_addr[2];
	uint32	ddr_wr_num;// 8bytes write once
	uint32  afm_pitch;
	uint32  afm_lbuf_share_mode;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_afm_slice *slice_param;
#endif
} dcam_fw_afm_block;

typedef struct _cmc_block {
	uint32	bypass;
	uint32	cmc_matrix[9];
} isp_fw_cmc_block;

typedef struct _bpc_rawhdr_subblock{
	uint32	bpc_hdr_en;
	uint32	bpc_hdr_2badPixel_en;
	uint32	bpc_hdr_expo_ratio;
	uint32	bpc_hdr_expo_ratio_inv;

	uint32	bpc_hdr_long_overExpo_th;
	uint32	bpc_hdr_short_overExpo_th;
	uint32	bpc_hdr_overExpo_num;

	uint32	bpc_hdr_long_underExpo_th;
	uint32	bpc_hdr_short_underExpo_th;
	uint32	bpc_hdr_underExpo_num;

	uint32	bpc_hdr_flat_th;
	uint32	bpc_hdr_edgeRatio_hv;
	uint32	bpc_hdr_edgeRatio_rd;

	uint32	bpc_hdr_kmax_overExpo;
	uint32	bpc_hdr_kmin_overExpo;
	uint32	bpc_hdr_kmax_underExpo;
	uint32	bpc_hdr_kmin_underExpo;

	uint32	bpc_hdr_zigzag_init_r;
	uint32	bpc_hdr_zigzag_init_gr;
	uint32	bpc_hdr_zigzag_init_b;
	uint32	bpc_hdr_zigzag_init_gb;
	uint32	bpc_hdr_overExpo_th;
 	uint32	bpc_hdr_underExpo_th;
	uint32	bpc_hdr_expo_pattern;
}dcam_bpc_rawhdr_subblock;

typedef struct _bpc_ppe_subblock{
	uint32	bpc_ppi_en;
	uint32	bpc_ppi_glb_row_start;
	uint32	bpc_ppi_glb_col_start;
	uint32	bpc_ppi_block_start_col;
	uint32	bpc_ppi_block_start_row;
	uint32	bpc_ppi_block_end_col;
	uint32	bpc_ppi_block_end_row;
	//
	uint32	bpc_ppi_block_width;
	uint32	bpc_ppi_block_height;
	//
	uint32	bpc_ppi_pattern_count;
	uint32	bpc_ppi_pattern_col[64];
	uint32	bpc_ppi_pattern_row[64];
	uint32	bpc_ppi_pattern_pos[64];
	//
	char	bpc_ppi_pattern_map_file[STRING_SIZE];
}dcam_bpc_ppe_subblock;

typedef struct _bpc_slice {
	uint32 bad_pixel_num;
	uint32 bpc_map_addr;
    uint32 bpc_bad_pixel_pos_out_addr;
	uint32 bpc_output_length;
	uint32 bpc_ppi_glb_row_start;
	uint32 bpc_ppi_glb_col_start;
	uint32 bpc_slice_roi_v_start;
	uint32 bpc_slice_roi_h_start;
	uint32 bpc_slice_roi_height;
	uint32 bpc_slice_roi_width;
}dcam_fw_bpc_slice;

typedef struct _bpc_block {
	uint32	bpc_bypass;
	uint32	bpc_double_bypass;
	uint32	bpc_three_bypass;
	uint32	bpc_four_bypass;
	uint32	bpc_mode;
	uint32	bpc_isMonoSensor;
	uint32	bpc_edge_hv_mode;
	uint32	bpc_edge_rd_mode;
    uint32	bpc_bpcResult_en;
    uint32  bpc_map_clr_en;
    uint32  bpc_rd_max_len_sel;
    uint32  bpc_wr_max_len_sel;
    uint32  bpc_blk_mode;
    uint32  bpc_mod_en;
    uint32  bpc_cg_dis;
	//
	uint32	bpc_four_badPixel_th[4];
	uint32	bpc_three_badPixel_th[4];
	uint32	bpc_double_badPixel_th[4];
	uint32	bpc_texture_th;
	uint32	bpc_flat_th;
	uint32	bpc_shift[3];
//
	uint32	bpc_edgeRatio_hv;
	uint32	bpc_edgeRatio_rd;
	//
	uint32	bpc_highOffset;
	uint32	bpc_lowOffset;
	uint32	bpc_highCoeff;
	uint32	bpc_lowCoeff;
	//
	uint32	bpc_kmincoeff;
	uint32	bpc_kmaxcoeff;
	//
	uint32	bpc_noise_i[8];
	uint32	bpc_noise_k[8];
	uint32	bpc_noise_b[8];
	//
	uint32	bpc_output_addr;
	uint32	bpc_map_addr;
	uint32  bpc_bad_pixel_num;
	char bpc_map_file[STRING_SIZE];
	char bpc_noise_curve_file[STRING_SIZE];
	//zzhdr
	dcam_bpc_rawhdr_subblock bpc_rawhdr_info;
	//ppe
	dcam_bpc_ppe_subblock bpc_ppe_info;
	//slice
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_bpc_slice *slice_param;
#endif
} dcam_fw_bpc_block;

typedef struct _grgb_slice {
	uint32 chk_sum_clr;
} isp_fw_grgb_slice;

typedef struct _grgb_block {
	uint32	bypass;
	uint32	grgb_checksum_en;
	uint32	grgb_diff_th;
	uint32	grgb_hv_edge_thr;
	uint32  grgb_slash_edge_thr;
	uint32  grgb_hv_flat_thr;
	uint32	grgb_slash_flat_thr;
	uint32	grgb_gr_ratio;
	uint32	grgb_gb_ratio;
	uint32	grgb_lum_curve_flat_t1;
	uint32	grgb_lum_curve_flat_t2;
	uint32	grgb_lum_curve_flat_t3;
	uint32	grgb_lum_curve_flat_t4;
	uint32	grgb_lum_curve_flat_r1;
	uint32	grgb_lum_curve_flat_r2;
	uint32	grgb_lum_curve_flat_r3;
	uint32	grgb_lum_curve_edge_t1;
	uint32	grgb_lum_curve_edge_t2;
	uint32	grgb_lum_curve_edge_t3;
	uint32	grgb_lum_curve_edge_t4;
	uint32	grgb_lum_curve_edge_r1;
	uint32	grgb_lum_curve_edge_r2;
	uint32	grgb_lum_curve_edge_r3;
	uint32	grgb_lum_curve_texture_t1;
	uint32	grgb_lum_curve_texture_t2;
	uint32	grgb_lum_curve_texture_t3;
	uint32	grgb_lum_curve_texture_t4;
	uint32	grgb_lum_curve_texture_r1;
	uint32	grgb_lum_curve_texture_r2;
	uint32	grgb_lum_curve_texture_r3;
	uint32	grgb_frez_curve_flat_t1;
	uint32	grgb_frez_curve_flat_t2;
	uint32	grgb_frez_curve_flat_t3;
	uint32	grgb_frez_curve_flat_t4;
	uint32	grgb_frez_curve_flat_r1;
	uint32	grgb_frez_curve_flat_r2;
	uint32	grgb_frez_curve_flat_r3;
	uint32	grgb_frez_curve_edge_t1;
	uint32	grgb_frez_curve_edge_t2;
	uint32	grgb_frez_curve_edge_t3;
	uint32	grgb_frez_curve_edge_t4;
	uint32	grgb_frez_curve_edge_r1;
	uint32	grgb_frez_curve_edge_r2;
	uint32	grgb_frez_curve_edge_r3;
	uint32	grgb_frez_curve_texture_t1;
	uint32	grgb_frez_curve_texture_t2;
	uint32	grgb_frez_curve_texture_t3;
	uint32	grgb_frez_curve_texture_t4;
	uint32	grgb_frez_curve_texture_r1;
	uint32	grgb_frez_curve_texture_r2;
	uint32	grgb_frez_curve_texture_r3;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_grgb_slice *slice_param;
#endif
} isp_fw_grgb_block;

typedef struct _cfa_slice {
	uint32 gbuf_addr_max;
} isp_fw_cfa_slice;

typedef struct _cfa_block {
	uint32	bypass;
	uint32  cfa_min_grid_new;
	uint32	cfa_grid_gain_new;
	uint32	cfa_uni_dir_intplt_tr_new;
	uint32	cfai_str_edge_thr;
	uint32	cfai_grid_thr;
	uint32	cfai_smooth_area_thr;
	uint32	cfai_redblue_high_sat_thr;
	uint32	cfai_cdcr_adj_factor;
	uint32	cfai_weight_control_enable;
	uint32	cfai_gird_dir_weight1;
	uint32	cfai_gird_dir_weight2;
	uint32	cfai_round_diff_03_thr;
	uint32	cfai_lowlux_03_thr;
	uint32	cfai_round_diff_12_thr;
	uint32	cfai_lowlux_12_thr;
	uint32	cfai_css_enable;
	uint32	cfai_css_edge_thr;
	uint32	cfai_css_weak_edge_thr;
	uint32	cfai_css_texture1_thr;
	uint32	cfai_css_texture2_thr;
	uint32	cfai_css_uv_val_thr;
	uint32	cfai_css_uv_diff_thr;
	uint32	cfai_css_gray_thr;
	uint32	cfai_css_pix_similar_thr;
	uint32	cfai_css_skin_u_top1;
	uint32	cfai_css_skin_u_down1;
	uint32	cfai_css_skin_v_top1;
	uint32	cfai_css_skin_v_down1;
	uint32	cfai_css_skin_u_top2;
	uint32	cfai_css_skin_u_down2;
	uint32	cfai_css_skin_v_top2;
	uint32	cfai_css_skin_v_down2;
	uint32	cfai_css_green_edge_thr;
	uint32	cfai_css_green_weak_edge_thr;
	uint32	cfai_css_green_tex1_thr;
	uint32	cfai_css_green_tex2_thr;
	uint32	cfai_css_green_flat_thr;
	uint32	cfai_css_edge_correction_ratio_r;
	uint32	cfai_css_weak_edge_correction_ratio_r;
	uint32	cfai_css_texture1_correction_ratio_r;
	uint32	cfai_css_texture2_correction_ratio_r;
	uint32	cfai_css_flat_correction_ratio_r;
	uint32	cfai_css_edge_correction_ratio_b;
	uint32	cfai_css_weak_edge_correction_ratio_b;
	uint32	cfai_css_texture1_correction_ratio_b;
	uint32	cfai_css_texture2_correction_ratio_b;
	uint32	cfai_css_flat_correction_ratio_b;
	uint32	cfai_css_alpha_for_tex2;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_cfa_slice *slice_param;
#endif
} isp_fw_cfa_block;

typedef struct _gamma_block {
	uint32 bypass;
	uint32 gamma_buf_sel;
	char   gmc_curve_file[2][STRING_SIZE];
	uint32 gamma_r[2][256];
	uint32 gamma_g[2][256];
	uint32 gamma_b[2][256];
} isp_fw_gamma_block;

typedef struct _hsv_block {
	uint32	bypass;
	uint32	hsv_buf_sel;
	uint32	hsv_table[2][360];
	char	hsv_color_tuning_filename[2][STRING_SIZE];
	char	hsv_sub_region_filename[2][STRING_SIZE];
	uint32	h[2][5][2];
	uint32	s[2][5][4];
	uint32	v[2][5][4];
	uint32	r_s[2][5][2];
	uint32	r_v[2][5][2];
} isp_fw_hsv_block;

typedef struct _pstrz_slice {
	uint32 chk_sum_clr;
} isp_fw_pstrz_slice;

typedef struct _pstrz_block {
	uint32	bypass;
	char	posterize_filename[2][STRING_SIZE];
	uint32	posterize_r_data[2][POSTERIZE_NUM];
	uint32	posterize_g_data[2][POSTERIZE_NUM];
	uint32	posterize_b_data[2][POSTERIZE_NUM];
	int		sample;
	int		pingpong;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_pstrz_slice *slice_param;
#endif
} isp_fw_pstrz_block;

typedef struct _cce_block {
	uint32 bypass;
	uint32 cce_matrix[9];
	uint32 y_shift;
	uint32 u_shift;
	uint32 v_shift;
} isp_fw_cce_block;

typedef struct _uvd_slice {
	uint32 chk_sum_clr;
} isp_fw_uvd_slice;

typedef struct _uvd_block {
	uint32 uvdiv_bypass;
	uint32 lum_th_l;
    uint32 lum_th_l_len;
    uint32 lum_th_h;
    uint32 lum_th_h_len;
    uint32 chroma_max_l;
    uint32 chroma_max_h;
    uint32 chroma_ratio;
	int32  u_th_0_l;
	int32  u_th_0_h;
	int32  v_th_0_l;
	int32  v_th_0_h;
	int32  u_th_1_l;
	int32  u_th_1_h;
	int32  v_th_1_l;
	int32  v_th_1_h;
	uint32 ratio_y_min1;
	uint32 ratio_y_min0;
	uint32 ratio_uv_min;
	uint32 luma_ratio;
	uint32 ratio_0;
	uint32 ratio_1;
	uint32 y_th_h_len;
	uint32 y_th_l_len;
	uint32 uv_abs_th_len;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_uvd_slice *slice_param;
#endif
} isp_fw_uvd_block;


typedef struct _dcam_afl_slice {
	int  bypass;//ISP_AFL_FRM_CTRL0
	uint64  init_phase_x;    //add for slice by lyc
       uint64  init_phase_x_region;     //add for slice by lyc
       int     init_region_counter;    //add for slice by lyc
       int     init_region_no;    //add for slice by lyc
       int     init_pre_luma_value_x_cor;    //add for slice by lyc
       int     init_start_col;    //add for slice by lyc
       uint32  afl_start_col;
       uint32  afl_end_col;
	uint32  afl_h_start_region;
	uint32  afl_h_end_region;
	uint32  afl_out_addr[2];
	uint32  region_out_addr[2];
} dcam_fw_afl_slice;


typedef struct _afl_block {
	uint32	bypass;
	uint32  afl_skip_num;
	uint32  afl_frame_num;
	uint32  afl_start_col;
	uint32  afl_end_col;
#ifdef RUN_DUMP_DATA
	uint32	afl_out_addr[IMAGE_LIST_SIZE];
	uint32	region_out_addr[IMAGE_LIST_SIZE];
#else
	uint32  afl_out_addr[2][2];
	uint32  region_out_addr[2][2];
#endif
	uint32	scaling_factor_h;
	uint32	scaling_factor_v;
	uint32	afl_scaling_factor_h_region;
	uint32	afl_scaling_factor_v_region;
	uint32	afl_h_start_region;
	uint32  afl_h_end_region;
	uint32	afl_continuous_mode;
	uint32  afl_sgl_start;
	uint32	global_data_write_num; //64bits every time
	uint32	region_data_write_num; //64bits every time
	int global_afl_data_state;
	#if _FPGA_
	uint32  afl_slice_param;
       #else
	dcam_fw_afl_slice *afl_slice_param;
       #endif
} isp_fw_afl_block;

typedef struct _image_adj_block {
	uint32 bchs_bypass;
	uint32 bchs_vector_bin_en;
	uint32 bchs_vector_rtl_en;
	uint32 bchs_vector_fpga_en;
	uint32 bchs_checksum_en;
	uint32 bchs_brightness_adj_eb;
	uint32 bchs_brightness_adj_factor;
	uint32 bchs_contrast_adj_eb;
	uint32 bchs_contrast_adj_factor;
	uint32 bchs_hue_adj_eb;
	uint32 bchs_hue_adj_theta;
	uint32 bchs_saturation_adj_eb;
	uint32 bchs_saturation_adj_factor_u;
	uint32 bchs_saturation_adj_factor_v;

} isp_fw_image_adj_block;

typedef struct _hist_slice {
	uint32	roi_empty;
	uint32  hist_roi_x_s;
	uint32  hist_roi_y_s;
	uint32  hist_roi_x_e;
	uint32  hist_roi_y_e;
	uint32	memory_clear_dis;
	uint32	last_slice;
	uint32	is_work;
} isp_fw_hist_slice;

typedef struct _hist_block {
	uint32  hist_roi_bypass;
	uint32	skip_mode;
	uint32	hist_roi_skip_num;
	uint32  hist_roi_x_s;
	uint32  hist_roi_y_s;
	uint32  hist_roi_x_e;
	uint32  hist_roi_y_e;
	uint32  hist_roi_channel_G_Y;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_hist_slice *slice_param;
#endif

} isp_fw_hist_block;

typedef struct _ee_block {
	uint32	ee_bypass;
	uint32	ee_new_pyramid_en;
	uint32	ee_old_gradient_en;
	uint32	ee_mode;
	uint32  ee_str_d_p;
	uint32  ee_str_d_n;
	uint32  ee_edge_thr_d_p;
	uint32  ee_edge_thr_d_n;
	uint32  ee_incr_d_p;
	uint32  ee_incr_d_n;
	uint32  ee_corner_cor;
	uint32  ee_corner_th_p;
	uint32  ee_corner_th_n;
	uint32  ee_corner_gain_p;
	uint32  ee_corner_gain_n;
	uint32  ee_corner_sm_p;
	uint32  ee_corner_sm_n;
	uint32  ee_edge_smooth_mode;
	uint32  ee_flat_smooth_mode;
	uint32  ee_ipd_en;
	uint32  ee_ipd_mask_mode;
	uint32  ee_ipd_flat_thr_p;
	uint32  ee_ipd_flat_thr_n;
	uint32  ee_ipd_eq_thr_p;
	uint32  ee_ipd_eq_thr_n;
	uint32  ee_ipd_more_thr_p;
	uint32  ee_ipd_more_thr_n;
	uint32  ee_ipd_less_thr_p;
	uint32  ee_ipd_less_thr_n;
	uint32  ee_ipd_smooth_en;
	uint32  ee_ipd_smooth_mode_p;
	uint32  ee_ipd_smooth_mode_n;
	uint32  ee_ipd_smooth_edge_thr_p;
	uint32  ee_ipd_smooth_edge_thr_n;
	uint32  ee_ipd_smooth_edge_diff_p;
	uint32  ee_ipd_smooth_edge_diff_n;
	uint32  ee_cv_t1;
	uint32  ee_cv_t2;
	uint32  ee_cv_t3;
	uint32  ee_cv_t4;
	uint32  ee_cv_r1;
	uint32  ee_cv_r2;
	uint32  ee_cv_r3;
	uint32  ee_cv_clip_p;
	uint32  ee_cv_clip_n;
	uint32  ee_ratio_hv_3;
	uint32  ee_ratio_hv_5;
	uint32  ee_ratio_diag_3;
	uint32  ee_ratio_diag_5;
	uint32  ee_gain_hv_1_t1;
	uint32  ee_gain_hv_1_t2;
	uint32  ee_gain_hv_1_t3;
	uint32  ee_gain_hv_1_t4;
	uint32  ee_gain_hv_1_r1;
	uint32  ee_gain_hv_1_r2;
	uint32  ee_gain_hv_1_r3;
	uint32  ee_gain_hv_2_t1;
	uint32  ee_gain_hv_2_t2;
	uint32  ee_gain_hv_2_t3;
	uint32  ee_gain_hv_2_t4;
	uint32  ee_gain_hv_2_r1;
	uint32  ee_gain_hv_2_r2;
	uint32  ee_gain_hv_2_r3;
	uint32  ee_gain_diag_1_t1;
	uint32  ee_gain_diag_1_t2;
	uint32  ee_gain_diag_1_t3;
	uint32  ee_gain_diag_1_t4;
	uint32  ee_gain_diag_1_r1;
	uint32  ee_gain_diag_1_r2;
	uint32  ee_gain_diag_1_r3;
	uint32  ee_gain_diag_2_t1;
	uint32  ee_gain_diag_2_t2;
	uint32  ee_gain_diag_2_t3;
	uint32  ee_gain_diag_2_t4;
	uint32  ee_gain_diag_2_r1;
	uint32  ee_gain_diag_2_r2;
	uint32  ee_gain_diag_2_r3;
	uint32  ee_weightt_hv2diag;
	uint32	ee_weightt_diag2hv;
	uint32  ee_gradient_computation_type;
	uint32  ee_lum_t1;
	uint32  ee_lum_t2;
	uint32  ee_lum_t3;
	uint32  ee_lum_t4;
	uint32  ee_lum_r1;
	uint32  ee_lum_r2;
	uint32  ee_lum_r3;
	uint32  ee_pos_t1;
	uint32  ee_pos_t2;
	uint32  ee_pos_t3;
	uint32  ee_pos_t4;
	uint32  ee_pos_r1;
	uint32  ee_pos_r2;
	uint32  ee_pos_r3;
	uint32  ee_pos_c1;
	uint32  ee_pos_c2;
	uint32  ee_pos_c3;
	uint32  ee_neg_t1;
	uint32  ee_neg_t2;
	uint32  ee_neg_t3;
	uint32  ee_neg_t4;
	uint32  ee_neg_r1;
	uint32  ee_neg_r2;
	uint32  ee_neg_r3;
	uint32  ee_neg_c1;
	uint32  ee_neg_c2;
	uint32  ee_neg_c3;
	uint32	ee_freq_t1;
	uint32	ee_freq_t2;
	uint32	ee_freq_t3;
	uint32	ee_freq_t4;
	uint32	ee_freq_r1;
	uint32	ee_freq_r2;
	uint32	ee_freq_r3;
//
    uint32  ee_ratio_old_gradient;
    uint32  ee_ratio_new_pyramid;
	uint32  ee_offset_thr_layer_curve_pos[3][4];
	uint32  ee_offset_ratio_layer_curve_pos[3][3];
	uint32  ee_offset_clip_layer_curve_pos[3][3];
	uint32  ee_offset_thr_layer_curve_neg[3][4];
	uint32  ee_offset_ratio_layer_curve_neg[3][3];
	uint32  ee_offset_clip_layer_curve_neg[3][3];
	uint32  ee_offset_ratio_layer_lum_curve[3][3];
	uint32  ee_offset_ratio_layer_freq_curve[3][3];
} isp_fw_ee_block;

typedef struct _cdn_block {
	uint32	bypass;
	uint32	median_mode;
	uint32	adaptive_median_thr;
	uint32	median_writeback_enable;
	uint32	median_thru0;
	uint32	median_thru1;
	uint32	median_thrv0;
	uint32	median_thrv1;
	uint32	filter_en;
	uint32	gaussian_mode;
	char	c_sigma_u[STRING_SIZE];
	char	c_sigma_v[STRING_SIZE];
	uint8	rangewu[32];
	uint8	rangewv[32];
} isp_fw_cdn_block;

typedef struct _pre_cdn_block {
	uint32	bypass;
	uint32	pre_cnr_mode;
	uint32	median_mode;
	uint32	median_thr;
	uint32	median_thru0;
	uint32	median_thru1;
	uint32	median_thrv0;
	uint32	median_thrv1;
    uint32	uvthr;
    uint32	ythr;
	uint32	median_writeback_enable;
	uint32	den_stren;
	uint32	uv_joint;
	char	c_sigma_y[STRING_SIZE];
	char	c_d_sigma[STRING_SIZE];
	char	c_sigma_u[STRING_SIZE];
	char	c_sigma_v[STRING_SIZE];
	uint8	r_segy[2][7];
	uint8	r_segu[2][7];
	uint8	r_segv[2][7];
	uint8	fix_d_w[25];
} isp_fw_pre_cdn_block;

typedef struct _post_cdn_slice {
	uint32 start_row_mod4;
} isp_fw_post_cdn_slice;

typedef struct _post_cdn_block {
	uint32	bypass;
	uint32	median_mode;
	uint32	post_cnr_mode;
	uint32	uvthr0;
	uint32	uvthr1;
	uint32	downsample_en;
	uint32	median_res_writeback_enable;
	uint32	adaptive_median_thresh;
	uint32	uv_joint;
	uint32	thru0;
	uint32	thru1;
	uint32	thrv0;
	uint32	thrv1;
	char	c_d_sigma[STRING_SIZE];
	char    c_sigma_u[STRING_SIZE];
	char	c_sigma_v[STRING_SIZE];
	uint8   r_segu[2][7];
	uint8   r_segv[2][7];
	uint8   r_distw[15][5];
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_post_cdn_slice *slice_param;
#endif
} isp_fw_post_cdn_block;

typedef struct _ygamma_block {
	uint32	bypass;
	uint32  buf_sel;
	char	solarize_curve_file[2][STRING_SIZE];
	uint32	solarize_y[2][129];
} isp_fw_ygamma_block;

typedef struct _iir_block {
	uint32  bypass;
	uint32  iir_mode;
    uint32  uv_th;
    uint32  uv_dist;
	uint32  y_th;
	uint32  uv_pg_th;
	uint32  ymd_u;
	uint32  ymd_v;
	uint32  ymd_min_u;
	uint32  ymd_min_v;
	uint32  uv_s_th;
	uint32  alpha_hl_diff_u;
	uint32  alpha_hl_diff_v;
	uint32  alpha_low_u;
	uint32  alpha_low_v;
	uint32  y_max_th;
	uint32  y_min_th;
	uint32  pre_uv_th;
	uint32  sat_ratio;
	uint32  y_edge_thr_max[8];
	uint32  y_edge_thr_min[8];
	uint32  uv_low_thr1_tbl[8];
	uint32  uv_low_thr2_tbl[8];
	uint32  uv_high_thr2_tbl[8];
	uint32  slope_uv[8];
	uint32  middle_factor_uv[8];
	uint32  middle_factor_y[8];
	uint32  slope_y[8];
	uint32  uv_diff_thr;
	uint32  css_lum_thr;
} isp_fw_iir_block;

typedef struct _yrandom_slice {
	uint32 chk_sum_clr;
	uint32 seed_init;
} isp_fw_yrandom_slice;

typedef struct _yrandom_block {
	uint32  random_bypass;
	uint32  seed;
	uint32	seed_mode;
	uint32  takeBit[8];
	uint32  r_shift;
	uint32  r_offset;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_yrandom_slice *slice_param;
#endif
} isp_fw_yrandom_block;

typedef struct _ynr_slice {
	uint32 start_row;
	uint32 start_col;
	uint32 ynr_slice_width;
	uint32 ynr_slice_height;
} isp_fw_ynr_slice;

typedef struct _ynr_block {
uint32 ynr_bypass                                                       ;
uint32 ynr_vector_bin_en                                                ;
uint32 ynr_vector_rtl_en                                                ;
uint32 ynr_vector_fpga_en                                               ;
uint32 ynr_checksum_en                                                  ;
uint32 ynr_lum_thresh0                                                  ;
uint32 ynr_lum_thresh1                                                  ;
uint32 ynr_Sal_enable                                                   ;
uint32 ynr_Sal_offset_0                                                 ;
uint32 ynr_Sal_offset_1                                                 ;
uint32 ynr_Sal_offset_2                                                 ;
uint32 ynr_Sal_offset_3                                                 ;
uint32 ynr_Sal_offset_4                                                 ;
uint32 ynr_Sal_offset_5                                                 ;
uint32 ynr_Sal_offset_6                                                 ;
uint32 ynr_Sal_offset_7                                                 ;
uint32 ynr_Sal_thresh0                                                  ;
uint32 ynr_Sal_thresh1                                                  ;
uint32 ynr_Sal_thresh2                                                  ;
uint32 ynr_Sal_thresh3                                                  ;
uint32 ynr_Sal_thresh4                                                  ;
uint32 ynr_Sal_thresh5                                                  ;
uint32 ynr_Sal_thresh6                                                  ;
uint32 ynr_l0_Sal_thresh0                                               ;
uint32 ynr_l0_Sal_thresh1                                               ;
uint32 ynr_Sal_nr_str_0                                                 ;
uint32 ynr_Sal_nr_str_1                                                 ;
uint32 ynr_Sal_nr_str_2                                                 ;
uint32 ynr_Sal_nr_str_3                                                 ;
uint32 ynr_Sal_nr_str_4                                                 ;
uint32 ynr_Sal_nr_str_5                                                 ;
uint32 ynr_Sal_nr_str_6                                                 ;
uint32 ynr_Sal_nr_str_7                                                 ;
uint32 ynr_coef_mode                                                    ;
uint32 ynr_l0_addback_enable                                            ;
uint32 ynr_l0_addback_ratio                                             ;
uint32 ynr_l0_addback_clip                                              ;
uint32 ynr_l1_wv_nr_enable                                              ;
uint32 ynr_l1_wv_thresh1_low                                            ;
uint32 ynr_l1_wv_thresh2_n_low                                          ;
uint32 ynr_l1_wv_ratio1_low                                             ;
uint32 ynr_l1_wv_ratio2_low                                             ;
uint32 ynr_l1_wv_thresh_d1_low                                          ;
uint32 ynr_l1_wv_thresh_d2_n_low                                        ;
uint32 ynr_l1_wv_ratio_d1_low                                           ;
uint32 ynr_l1_wv_ratio_d2_low                                           ;
uint32 ynr_l1_soft_offset_low                                           ;
uint32 ynr_l1_soft_offsetd_low                                          ;
uint32 ynr_l1_wv_thresh1_mid                                            ;
uint32 ynr_l1_wv_thresh2_n_mid                                          ;
uint32 ynr_l1_wv_ratio1_mid                                             ;
uint32 ynr_l1_wv_ratio2_mid                                             ;
uint32 ynr_l1_wv_thresh_d1_mid                                          ;
uint32 ynr_l1_wv_thresh_d2_n_mid                                        ;
uint32 ynr_l1_wv_ratio_d1_mid                                           ;
uint32 ynr_l1_wv_ratio_d2_mid                                           ;
uint32 ynr_l1_soft_offset_mid                                           ;
uint32 ynr_l1_soft_offsetd_mid                                          ;
uint32 ynr_l1_wv_thresh1_high                                           ;
uint32 ynr_l1_wv_thresh2_n_high                                         ;
uint32 ynr_l1_wv_ratio1_high                                            ;
uint32 ynr_l1_wv_ratio2_high                                            ;
uint32 ynr_l1_wv_thresh_d1_high                                         ;
uint32 ynr_l1_wv_thresh_d2_n_high                                       ;
uint32 ynr_l1_wv_ratio_d1_high                                          ;
uint32 ynr_l1_wv_ratio_d2_high                                          ;
uint32 ynr_l1_soft_offset_high                                          ;
uint32 ynr_l1_soft_offsetd_high                                         ;
uint32 ynr_l1_addback_enable                                            ;
uint32 ynr_l1_addback_ratio                                             ;
uint32 ynr_l1_addback_clip                                              ;
uint32 ynr_l2_wv_nr_enable                                              ;
uint32 ynr_l2_wv_thresh1_low                                            ;
uint32 ynr_l2_wv_thresh2_n_low                                          ;
uint32 ynr_l2_wv_ratio1_low                                             ;
uint32 ynr_l2_wv_ratio2_low                                             ;
uint32 ynr_l2_wv_thresh_d1_low                                          ;
uint32 ynr_l2_wv_thresh_d2_n_low                                        ;
uint32 ynr_l2_wv_ratio_d1_low                                           ;
uint32 ynr_l2_wv_ratio_d2_low                                           ;
uint32 ynr_l2_soft_offset_low                                           ;
uint32 ynr_l2_soft_offsetd_low                                          ;
uint32 ynr_l2_wv_thresh1_mid                                            ;
uint32 ynr_l2_wv_thresh2_n_mid                                          ;
uint32 ynr_l2_wv_ratio1_mid                                             ;
uint32 ynr_l2_wv_ratio2_mid                                             ;
uint32 ynr_l2_wv_thresh_d1_mid                                          ;
uint32 ynr_l2_wv_thresh_d2_n_mid                                        ;
uint32 ynr_l2_wv_ratio_d1_mid                                           ;
uint32 ynr_l2_wv_ratio_d2_mid                                           ;
uint32 ynr_l2_soft_offset_mid                                           ;
uint32 ynr_l2_soft_offsetd_mid                                          ;
uint32 ynr_l2_wv_thresh1_high                                           ;
uint32 ynr_l2_wv_thresh2_n_high                                         ;
uint32 ynr_l2_wv_ratio1_high                                            ;
uint32 ynr_l2_wv_ratio2_high                                            ;
uint32 ynr_l2_wv_thresh_d1_high                                         ;
uint32 ynr_l2_wv_thresh_d2_n_high                                       ;
uint32 ynr_l2_wv_ratio_d1_high                                          ;
uint32 ynr_l2_wv_ratio_d2_high                                          ;
uint32 ynr_l2_soft_offset_high                                          ;
uint32 ynr_l2_soft_offsetd_high                                         ;
uint32 ynr_l2_addback_enable                                            ;
uint32 ynr_l2_addback_ratio                                             ;
uint32 ynr_l2_addback_clip                                              ;
uint32 ynr_l3_wv_nr_enable                                              ;
uint32 ynr_l3_wv_thresh1_low                                            ;
uint32 ynr_l3_wv_thresh2_n_low                                          ;
uint32 ynr_l3_wv_ratio1_low                                             ;
uint32 ynr_l3_wv_ratio2_low                                             ;
uint32 ynr_l3_wv_thresh_d1_low                                          ;
uint32 ynr_l3_wv_thresh_d2_n_low                                        ;
uint32 ynr_l3_wv_ratio_d1_low                                           ;
uint32 ynr_l3_wv_ratio_d2_low                                           ;
uint32 ynr_l3_soft_offset_low                                           ;
uint32 ynr_l3_soft_offsetd_low                                          ;
uint32 ynr_l3_wv_thresh1_mid                                            ;
uint32 ynr_l3_wv_thresh2_n_mid                                          ;
uint32 ynr_l3_wv_ratio1_mid                                             ;
uint32 ynr_l3_wv_ratio2_mid                                             ;
uint32 ynr_l3_wv_thresh_d1_mid                                          ;
uint32 ynr_l3_wv_thresh_d2_n_mid                                        ;
uint32 ynr_l3_wv_ratio_d1_mid                                           ;
uint32 ynr_l3_wv_ratio_d2_mid                                           ;
uint32 ynr_l3_soft_offset_mid                                           ;
uint32 ynr_l3_soft_offsetd_mid                                          ;
uint32 ynr_l3_wv_thresh1_high                                           ;
uint32 ynr_l3_wv_thresh2_n_high                                         ;
uint32 ynr_l3_wv_ratio1_high                                            ;
uint32 ynr_l3_wv_ratio2_high                                            ;
uint32 ynr_l3_wv_thresh_d1_high                                         ;
uint32 ynr_l3_wv_thresh_d2_n_high                                       ;
uint32 ynr_l3_wv_ratio_d1_high                                          ;
uint32 ynr_l3_wv_ratio_d2_high                                          ;
uint32 ynr_l3_soft_offset_high                                          ;
uint32 ynr_l3_soft_offsetd_high                                         ;
uint32 ynr_l3_addback_enable                                            ;
uint32 ynr_l3_addback_ratio                                             ;
uint32 ynr_l3_addback_clip                                              ;
uint32 ynr_blf_enable                                                   ;
uint32 ynr_blf_maxRadius                                                ;
uint32 ynr_blf_Radius                                                   ;
uint32 ynr_blf_imgCenterX                                               ;
uint32 ynr_blf_imgCenterY                                               ;
uint32 ynr_blf_range_index                                              ;
uint32 ynr_blf_range_s1                                                 ;
uint32 ynr_blf_range_s2                                                 ;
uint32 ynr_blf_range_s3                                                 ;
uint32 ynr_blf_range_s4                                                 ;
uint32 ynr_blf_range_s0_low                                             ;
uint32 ynr_blf_range_s0_mid                                             ;
uint32 ynr_blf_range_s0_high                                            ;
uint32 ynr_blf_dist_weight0                                             ;
uint32 ynr_blf_dist_weight1                                             ;
uint32 ynr_blf_dist_weight2                                             ;

#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_ynr_slice *slice_param;
#endif
} isp_fw_ynr_block;

typedef struct _scaler_slice {
	uint32 bypass;
	uint32 trim0_size_x;
	uint32 trim0_size_y;
	uint32 trim0_start_x;
	uint32 trim0_start_y;
	uint32 trim1_size_x;
	uint32 trim1_size_y;
	uint32 trim1_start_x;
	uint32 trim1_start_y;
	uint32 scaler_ip_int;
	uint32 scaler_ip_rmd;
	uint32 scaler_cip_int;
	uint32 scaler_cip_rmd;
	uint32 scaler_factor_in;
	uint32 scaler_factor_out;
	uint32 scaler_ip_int_ver;
	uint32 scaler_ip_rmd_ver;
	uint32 scaler_cip_int_ver;
	uint32 scaler_cip_rmd_ver;
	uint32 scaler_factor_in_ver;
	uint32 scaler_factor_out_ver;
	uint32 src_size_x;
	uint32 src_size_y;
	uint32 dst_size_x;
	uint32 dst_size_y;
	uint32 chk_sum_clr;
} isp_fw_scaler_slice;

typedef struct _scaler_block {
	uint32 path_en;
	uint32 yuvscaler_bypass;
	uint32 yuvscaler_bufsel;
	uint32 pos_sel;
	uint32 yuvscaler_frame_deci;
	uint32 src_width;
	uint32 src_height;
	uint32 output_width;
	uint32 output_height;
	uint32 scaler_coef0[32];
	uint32 scaler_coef1[16];
	uint32 scaler_coef2[132];
	uint32 scaler_coef3[132];
	isp_fw_store_block store_info;
	uint32 cam_path_trim_eb;
	uint32 cam_path_trim_start_x;
	uint32 cam_path_trim_start_y;
	uint32 cam_path_trim_size_x;
	uint32 cam_path_trim_size_y;
	uint32 cam_deci_x_eb;
	uint32 cam_deci_y_eb;
	uint32 cam_deci_x;
	uint32 cam_deci_y;
	uint32 cam_sc_en;
	uint32 cam_path_des_size_x;
	uint32 cam_path_des_size_y;
	uint32 yuv_output_format;
	uint32 cam_rot_mode;
	uint32 cam_regular_mode;
	uint32 cam_shrink_y_range;
	uint32 cam_shrink_uv_range;
	uint32 cam_shrink_y_offset;
	uint32 cam_shrink_uv_offset;
	uint32 cam_shrink_uv_dn_th;
	uint32 cam_shrink_uv_up_th;
	uint32 cam_shrink_y_dn_th;
	uint32 cam_shrink_y_up_th;
	uint32 cam_effect_v_th;
	uint32 cam_effect_u_th;
	uint32 cam_effect_y_th;
	uint32 scaler_uv_ver_tap;
	uint32 scaler_y_ver_tap;
	uint32 uv_sync_y;
	uint32 cam_hblank_num_uv;
	uint32 cam_hblank_num_y;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_scaler_slice *slice_param;
#endif
} isp_fw_scaler_block;

typedef struct _noisefilter_slice {
	uint32 seed[4];
	uint32 seed_init;
} isp_fw_noisefilter_slice;

typedef struct _noisefilter_block {
	uint32	noisefilter_bypass;
	uint32	noisefilter_shape_mode;
	uint32	noisefilter_seed_mode;
	uint32	noisefilter_seed;
	uint32	noisefilter_takeBit[8];
	uint32	noisefilter_r_shift;
	uint32	noisefilter_r_offset;
	uint32	noisefilter_filter_thr;
	uint32	noisefilter_filter_thr_m;
	uint32	noisefilter_av_t1;
	uint32	noisefilter_av_t2;
	uint32	noisefilter_av_t3;
	uint32	noisefilter_av_t4;
	uint32	noisefilter_av_r1;
	uint32	noisefilter_av_r2;
	uint32	noisefilter_av_r3;
	uint32	noisefilter_clip_p;
	uint32	noisefilter_clip_n;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_noisefilter_slice *slice_param;
#endif
} isp_fw_noisefilter_block;

typedef struct _ydelay_block {
	uint32	ydelay;
} isp_fw_ydelay_block;

typedef struct _dcam_ppe_slice {
	uint32	slice_bypass;
	uint32	slice_ppi_glb_row_start;
	uint32	slice_ppi_glb_col_start;
	uint32	slice_ppe_af_win_sy;
	uint32	slice_ppe_af_win_sx;
	uint32	slice_ppe_af_win_ey;
	uint32	slice_ppe_af_win_ex;
	uint32	slice_phasepixel_total_num;
	uint32	slice_phasepixel_data_long_bytes;
	uint32	slice_ppe_gain_addr_l[2];
	uint32	slice_ppe_gain_addr_r[2];
} dcam_fw_ppe_slice;

typedef struct _ppe_block {
	uint32	bypass;
	uint32	ppe_extractor_continuous_mode;
	uint32	ppe_extractor_skip_num;
	uint32	ppe_extractor_en;
	uint32	ppe_af_win_sy0;
	uint32	ppe_af_win_sx0;
	uint32	ppe_af_win_ey0;
	uint32	ppe_af_win_ex0;
	uint32	ppe_block_start_col;
	uint32	ppe_block_start_row;
	uint32	ppe_block_end_col;
	uint32	ppe_block_end_row;
	uint32	ppe_block_width;
	uint32	ppe_block_height;
	char	ppe_pattern_map_file[STRING_SIZE];
	uint32	ppe_phase_map_corr_en;
	uint32	ppe_gain_map_grid;
	char	ppe_phase_l_gain_map[STRING_SIZE];
	char	ppe_phase_r_gain_map[STRING_SIZE];
	uint32	ppe_upperbound_gr;
	uint32	ppe_upperbound_r;
	uint32	ppe_upperbound_b;
	uint32	ppe_upperbound_gb;
	uint32	ppe_blacklevel_gr;
	uint32	ppe_blacklevel_r;
	uint32	ppe_blacklevel_b;
	uint32	ppe_blacklevel_gb;
	uint32	pattern_count;
	uint32	pattern_col[64];
	uint32	pattern_row[64];
	uint32	pattern_pos[64];
	uint32	l_gain[130];
	uint32	r_gain[130];
	uint32	grid;
	uint32	phasepixel_data_long_bytes;
	uint32	phase_data_write_num;
	uint32	ppe_gain_addr_l[2];
	uint32	ppe_gain_addr_r[2];
	uint32	bayer_mode;
	uint32	phase_pixel_num;
	uint32	phasepixel_total_num;
	uint32	pdaf_mode;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_ppe_slice *slice_param;
#endif
} dcam_fw_ppe_block;

typedef struct _thumbnailscaler_store_slice {
	uint32 store_addr[4][3];
	uint32 slice_out_width;
	uint32 slice_out_height;
	uint32 overlap_up;
	uint32 overlap_down;
	uint32 overlap_left;
	uint32 overlap_right;
	uint32 shadow_clr;
} isp_fw_thumbnailscaler_store_slice;

typedef struct _thumbnailscaler_store_block {
	uint32	store_bypass;
	uint32	store_shadow_clr_sel;
	uint32  YUVOutputFormat;
	uint32	mono_color_mode;
	uint32	store_color_format;
	uint32	store_endian;
	uint32	store_width;
	uint32  store_height;
	uint32  store_addr[4][3];
	uint32  store_pitch[3];
	uint32	store_size[3];
	uint32	mirror_en;
	//uint32	FBC_en;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_thumbnailscaler_store_slice *slice_param;
#endif
	uint32	speed_2x;
	uint32	total_word;
	uint32	path_sel;
	uint32	base_align;
	uint32	burst_len_sel;
	uint32	trim_en;
	uint32	trim_start_x;
	uint32	trim_start_y;
	uint32	trim_size_x;
	uint32	trim_size_y;
} isp_fw_thumbnailscaler_store_block;

typedef struct _thumbnailscaler_slice {
	uint32 bypass;
	uint32 slice_size_before_trim_hor;
	uint32 slice_size_before_trim_ver;
	uint32 y_trim0_start_hor;
	uint32 y_trim0_start_ver;
	uint32 y_trim0_size_hor;
	uint32 y_trim0_size_ver;
	uint32 uv_trim0_start_hor;
	uint32 uv_trim0_start_ver;
	uint32 uv_trim0_size_hor;
	uint32 uv_trim0_size_ver;
	uint32 y_slice_src_size_hor;
	uint32 y_slice_src_size_ver;
	uint32 y_slice_des_size_hor;
	uint32 y_slice_des_size_ver;
	uint32 uv_slice_src_size_hor;
	uint32 uv_slice_src_size_ver;
	uint32 uv_slice_des_size_hor;
	uint32 uv_slice_des_size_ver;
	uint32 y_init_phase_hor;
	uint32 y_init_phase_ver;
	uint32 uv_init_phase_hor;
	uint32 uv_init_phase_ver;
	uint32 chk_sum_clr;
} isp_fw_thumbnailscaler_slice;

typedef struct _thumbnailscaler_block {
	uint32 bypass;
	uint32 pipeline_pos;
	uint32 ow;
	uint32 oh;
	uint32 outformat;
	uint32 phaseX;
	uint32 phaseY;
	uint32 base_align;
	uint32 trim0_en;
	uint32 trim0_start_x;
	uint32 trim0_start_y;
	uint32 trim0_size_x;
	uint32 trim0_size_y;
	uint32 path_frame_skip;
	uint32 uv_sync_y;

	uint32 y_frame_src_size_hor;
	uint32 y_frame_src_size_ver;
	uint32 y_frame_des_size_hor;
	uint32 y_frame_des_size_ver;
	uint32 uv_frame_src_size_hor;
	uint32 uv_frame_src_size_ver;
	uint32 uv_frame_des_size_hor;
	uint32 uv_frame_des_size_ver;
	uint32 y_deci_hor_en;
	uint32 y_deci_hor_par;
	uint32 y_deci_ver_en;
	uint32 y_deci_ver_par;
	uint32 uv_deci_hor_en;
	uint32 uv_deci_hor_par;
	uint32 uv_deci_ver_en;
	uint32 uv_deci_ver_par;
	uint32 regular_bypass;
	uint32 regular_mode;
	uint32 regular_shrink_y_range;
	uint32 regular_shrink_uv_range;
	uint32 regular_shrink_y_offset;
	uint32 regular_shrink_uv_offset;
	uint32 regular_shrink_uv_dn_th;
	uint32 regular_shrink_uv_up_th;
	uint32 regular_shrink_y_dn_th;
	uint32 regular_shrink_y_up_th;
	uint32 regular_effect_v_th;
	uint32 regular_effect_u_th;
	uint32 regular_effect_y_th;
	isp_fw_thumbnailscaler_store_block store_info;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_thumbnailscaler_slice *slice_param;
#endif
}isp_fw_thumbnailscaler_block;

typedef struct _ltm_stat_slice {
	uint32 roi_start_x;
	uint32 roi_start_y;
	uint32 ddr_addr[2];
	uint32 tile_num_x;
	uint32 tile_num_y;
	uint32 tile_num_x_minus1;
	uint32 tile_num_y_minus1;
	uint32 ddr_wr_num;
} isp_fw_ltm_stat_slice;

typedef struct _ltm_stat_block {
	uint32 bypass;
	uint32 strength;
	uint32 binning_en;
	uint32 region_est_en;
	uint32 text_point_thres[2];
	char text_point_thres_str[2][STRING_SIZE];
	uint32 text_proportion;
	uint32 tile_num_auto;
	uint32 tile_num_x;
	uint32 tile_num_y;
	uint32 tile_height;
	uint32 tile_width;
	uint32 frame_width_stat;
	uint32 frame_height_stat;
	uint32 clip_limit_min;
	uint32 clip_limit;
	uint32 ddr_addr[2];
	uint32 ddr_pitch;
	uint32 buf_full_mode;
	uint32 channel_sel;
	float text_point_alpha[2];
	char text_point_alpha_str[2][STRING_SIZE];
	uint32 lut_buf_sel;
	uint32 ltm_stat_table[2][128];
	uint32 output_en;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_ltm_stat_slice *slice_param;
#endif
} isp_fw_ltm_stat_block;

typedef struct _ltm_map_slice {
	uint32 tile_num_x;
	uint32 tile_num_y;
	uint32 tile_right_flag;
	uint32 tile_left_flag;
	int32 tile_start_y;
	int32 tile_start_x;
	uint32 mem_init_addr[MAX_FUSION_FRAME_NUM];
} isp_fw_ltm_map_slice;

typedef struct _ltm_map_block {
	uint32 bypass;
	uint32 video_mode;
	uint32 burst8_en;
	char   tile_file[STRING_SIZE];
	char   stat_file[STRING_SIZE];
	uint32 hist_pitch;
	uint32 tile_height;
	uint32 tile_width;
	uint32 tile_size_pro;
	uint32 tile_num_x_stat;
	uint32 tile_num_y_stat;
	uint32 tile_width_stat;
	uint32 tile_height_stat;
	uint32 frame_width_stat;
	uint32 frame_height_stat;
	uint32 fetch_wait_en;
	uint32 fetch_wait_line;
	uint32 stat_ddr_addr[MAX_FUSION_FRAME_NUM];
	uint32 hist[MAX_FUSION_FRAME_NUM][LTM_MAX_BIN];
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_ltm_map_slice *slice_param;
#endif
}isp_fw_ltm_map_block;

typedef struct _yuv_deci_block {
	uint32	bypass;
	uint32	yuv_deci_x;
	uint32	yuv_deci_y;
} dcam_fw_yuv_deci_block;

//3dnr_me
typedef struct _dcam_me_slice {
	uint32	slice_roi_start_x;
	uint32	slice_roi_start_y;
	uint32	slice_roi_width;
	uint32	slice_roi_height;
} dcam_fw_me_slice;

typedef struct _me_block {
	uint32	bypass;
	uint32	mv_bypass;
	uint32	sub_me_bypass;
	uint32	roi_start_x;
	uint32	roi_start_y;
	uint32	roi_width;
	uint32	roi_height;
	uint32	project_mode;
	uint32	channel_sel;
	uint32	ping_pong_en;
	uint32	out_en;
	uint32	waddr;
	uint32	x_proj_len;
	uint32	y_proj_len;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_me_slice *slice_param;
#endif
} dcam_fw_me_block;

typedef struct _dcam_gtm_stat_slice {
	uint32 gtm_slice_main;
	uint32 gtm_slice_line_startpos;
	uint32 gtm_slice_line_endpos;
	uint32 slice_width;
	uint32 slice_height;
} dcam_fw_gtm_stat_slice;

typedef struct _raw_gtm_stat_block {
	uint32 mod_en;
	uint32	bypass;
	uint32	rgb2y_mode;
	uint32	rgb2y_r_coeff;
	uint32	rgb2y_g_coeff;
	uint32	rgb2y_b_coeff;
	uint32	min_percentile_16bit;
	uint32	max_percentile_16bit;
	uint32	target_norm_set_mode;
	float	target_norm_coeff;
	uint32 target_norm_coeff_int;
	uint32	image_key_set_mode;
	//uint32	video_luma_smooth_thr;
	uint32	video_cur_Ymin_weight;
	uint32	video_pre_Ymin_weight;
	uint32	target_norm_arry[IMAGE_LIST_SIZE];
	uint32	image_key_arry[IMAGE_LIST_SIZE];
	uint32	gtm_ymin_arry[IMAGE_LIST_SIZE];
	uint32	gtm_yavg_arry[IMAGE_LIST_SIZE];
	uint32	gtm_ymax_arry[IMAGE_LIST_SIZE];
	uint32	gtm_log_min_int_arry[IMAGE_LIST_SIZE];
	uint32	gtm_lr_int_arry[IMAGE_LIST_SIZE];
	uint32	gtm_log_diff_int_arry[IMAGE_LIST_SIZE];
	uint32	gtm_log_max_int_arry[IMAGE_LIST_SIZE];
	uint32	gtm_log_diff_arry[IMAGE_LIST_SIZE];
	uint32 target_norm;
	uint32 image_key;
	uint32	gtm_ymin;
	uint32	gtm_yavg;
	uint32	gtm_ymax;
	uint32	gtm_log_min_int;
	uint32	gtm_lr_int;
	uint32	gtm_log_diff_int;
	uint32	gtm_log_max_int;
	uint32	gtm_log_diff;
	uint32	gtm_hist_total;
	uint32	gtm_min_per;
	uint32	gtm_max_per;
	uint32	gtm_pre_ymin_weight;
	uint32	gtm_cur_ymin_weight;
	uint32	gtm_ymax_diff_thr;
	uint32	tm_lumafilter_c[3][3];
	uint32	tm_lumafilter_shift;
	uint32	tm_hist_xpts[GTM_HIST_BIN_NUM];
	uint32	tm_hist_xpts_arry[IMAGE_LIST_SIZE][GTM_HIST_BIN_NUM];
	char   stat_file[STRING_SIZE];
	uint32	video_luma_sm_Ymax_diff_thr;
	uint32	video_luma_sm_Yavg_diff_thr;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_gtm_stat_slice *slice_param;
#endif
} dcam_fw_raw_gtm_stat_block;

typedef struct _raw_gtm_map_block {
	uint32	bypass;
	uint32	video_mode;
} dcam_fw_raw_gtm_map_block;

typedef struct _dcam_lscm_slice {
	uint32 blk_sx;
	uint32 blk_sy;
	uint32 blk_num_ver;
	uint32 blk_num_hor;
	uint32 pitch;
	uint32 base_addr[2];
	uint32 slice_num;
	uint32 lscm_bypass;

} dcam_fw_lscm_slice;

typedef struct _dcam_fw_lscm_block{
	uint32 lscm_bypass;
	uint32 lscm_checksum_en;
	uint32 lscm_skip_num;
	uint32 lscm_continuous_mode;
	uint32 lscm_blk_sx;
	uint32 lscm_blk_sy;
	uint32 lscm_blk_width;
	uint32 lscm_blk_height;
	uint32 lscm_avgshf;
	uint32 lscm_blk_num_hor;
	uint32 lscm_blk_num_ver;
	uint32 lscm_hdr_en;
	uint32 lscm_hdr_zigzag_init_r;
	uint32 lscm_hdr_zigzag_init_gr;
	uint32 lscm_hdr_zigzag_init_b;
	uint32 lscm_base_addr[2];
	uint32 slice_num;
	//g/r/b channel
	uint64 r_info[16384];
	uint64 g_info[16384];
	uint64 b_info[16384];
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_lscm_slice *slice_param;
#endif
} dcam_fw_lscm_block;

typedef struct _dcam_fbc_slice {

	uint32  slice_title_num_width_storecrop1;
	uint32  slice_fbc_left_tile_storecrop1;
	uint32  slice_title_num_width_storecrop2;
	uint32  slice_fbc_left_tile_storecrop2;
	uint32  slice_head_size;
	uint32  slice_buff_size;
}dcam_fw_fbc_slice;
typedef struct _dcam_fbc_block {
	uint32  head_size[2];
	uint32  head_size_org[2];
	uint32  data_size[2];
	uint32  midial_data_size[2];
	uint32  addr[2];
	uint32  mid_addr[2];
	uint32  head_size_ex[2];
	uint32  low4bit_en;
	uint32  fbc_sel;
	uint32  fbc_en;
	uint32  slice_num;
	uint32  prev_pitch;
	uint32  full_pitch;
       #if _FPGA_
       uint32 slice_param;
	#else
	dcam_fw_fbc_slice *slice_param;
	#endif
}dcam_fw_fbc_block;

typedef struct _nr3_slice {
	unsigned int	bypass;
	unsigned int	crop_bypass;
	unsigned int	src_width;//overlap
	unsigned int	src_height;
	unsigned int	dst_width;
	unsigned int	dst_height;
	unsigned int	offset_start_x;
	unsigned int	offset_start_y;
	unsigned int	start_col;
	unsigned int	start_row;
	unsigned int	src_lum_addr;
	unsigned int	src_chr_addr;
	unsigned int	dst_lum_addr;
	unsigned int	dst_chr_addr;
	unsigned int	ft_y_width;
	unsigned int	ft_y_height;
	unsigned int	ft_uv_width;
	unsigned int	ft_uv_height;
	unsigned int	first_line_mode;
	unsigned int	last_line_mode;
	unsigned int	slice_num;
} isp_fw_nr3_slice;

typedef struct _nr3_fbdc_slice {
	unsigned int	bypass;
	unsigned int	dst_width;
	unsigned int	dst_height;
	unsigned int	start_col;
	unsigned int	start_row;
	unsigned int	tile_col;
	unsigned int	tile_row;
	unsigned int	fbc_tile_number;
	unsigned int	fbc_endian;
	unsigned int	fbc_color_format;
	unsigned int	fbc_slice_mode_en;
	unsigned int	fbc_bypass_flag;
	unsigned int	fbc_size_in_ver;
	unsigned int	fbc_size_in_hor;
	unsigned int	fbc_y_tile_addr_init_x256;
	unsigned int	fbc_c_tile_addr_init_x256;
	unsigned int	fbc_y_header_addr_init;
	unsigned int	fbc_c_header_addr_init;
	unsigned int	fbd_fbdc_cr_ch0123_val0;
	unsigned int	fbd_y_tile_addr_init_x256;
	unsigned int	fbd_y_tiles_num_pitch;
	unsigned int	fbd_y_header_Addr_init;
	unsigned int	fbd_c_tile_addr_init_x256;
	unsigned int	fbd_c_tiles_num_pitch;
	unsigned int	fbd_c_header_Addr_init;
	unsigned int	fbd_y_pixel_size_in_hor;
	unsigned int	fbd_y_pixel_size_in_ver;
	unsigned int	fbd_c_pixel_size_in_hor;
	unsigned int	fbd_c_pixel_size_in_ver;
	unsigned int	fbd_y_pixel_start_in_hor;
	unsigned int	fbd_y_pixel_start_in_ver;
	unsigned int	fbd_c_pixel_start_in_hor;
	unsigned int	fbd_c_pixel_start_in_ver;
	unsigned int	fbd_y_tiles_num_in_hor;
	unsigned int	fbd_y_tiles_num_in_ver;
	unsigned int	fbd_c_tiles_num_in_hor;
	unsigned int	fbd_c_tiles_num_in_ver;
	unsigned int	fbd_c_tiles_start_odd;
	unsigned int	fbd_y_tiles_start_odd;
} isp_fw_nr3_fbdc_slice;

typedef struct _nr3_mem_ctrl_slice {
	unsigned int	bypass;
	unsigned int	ft_ref_addr[2];
	unsigned int	ft_y_addr;
	unsigned int	ft_uv_addr;
	unsigned int	img_pitch_y;
	unsigned int	img_width;
	unsigned int	img_height;
	unsigned int	global_img_width;
	unsigned int	global_img_height;
	unsigned int	ft_y_width;
	unsigned int	ft_y_height;
	unsigned int	ft_uv_width;
	unsigned int	ft_uv_height;
	unsigned int	start_row;
	unsigned int	start_col;
	signed int	mv_x;
	signed int	mv_y;
	//unsigned int	ft_addr_mode;
	unsigned int	ft_pitch;
	unsigned int	ft_max_len_sel;
	unsigned int	ref_pic_flag;
	unsigned int	retain_num;
	unsigned int	roi_mode;
	unsigned int	extra_pixel_mode;
	unsigned int	data_toyuv_en;
	unsigned int	chk_sum_clr_en;
	unsigned int	y_sla_bypass;
	unsigned int	uv_sla_bypass;
	unsigned int	back_toddr_en;
	//unsigned int	last_line_mode_en;
	//unsigned int	first_line_mode_en;
	unsigned int	last_line_mode;
	unsigned int	first_line_mode;
	unsigned int	nr3_done_mode;
	unsigned int	nr3_ft_path_sel;
} isp_fw_nr3_mem_ctrl_slice;

typedef struct _nr3_fetch_slice {
	uint32 slice_addr[3];
	uint32 slice_width;
	uint32 slice_height;
} isp_fw_nr3_fetch_slice;

typedef struct _3dnr_block {
	uint32	bypass;
	uint32	FBC_enable;
	uint32	fusion_mode;
	uint32	capture_mode;
	uint32	filter_switch;
	uint32	y_src_weight;
	uint32	u_src_weight;
	uint32	v_src_weight;
	uint32	y_pixel_noise_threshold;
	uint32	u_pixel_noise_threshold;
	uint32	v_pixel_noise_threshold;
	uint32	y_pixel_noise_weight;
	uint32	u_pixel_noise_weight;
	uint32	v_pixel_noise_weight;
	uint32	threshold_radial_variation_u_range_min;
	uint32	threshold_radial_variation_u_range_max;
	uint32	threshold_radial_variation_v_range_min;
	uint32	threshold_radial_variation_v_range_max;
	uint32	y_threshold_polyline[9];
	uint32	u_threshold_polyline[9];
	uint32	v_threshold_polyline[9];
	uint32	y_intensity_gain_polyline[9];
	uint32	u_intensity_gain_polyline[9];
	uint32	v_intensity_gain_polyline[9];
	uint32	gradient_weight_polyline[11];
	uint32	U_threshold_factor[4];
	uint32	V_threshold_factor[4];
	uint32	U_divisor_factor[4];
	uint32	V_divisor_factor[4];
	uint32	r1_circle;
	uint32	r2_circle;
	uint32	r3_circle;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_nr3_slice *slice_param;
#endif
} isp_fw_3dnr_block;


typedef struct _3dnr_mem_ctrl {
	uint32	operation_mode;
	signed int	mv_x;
	signed int	mv_y;
	int	IsSinglePath3DNR;
	uint32	yuv_mode;
	//pre trim
	uint32	trim_bypass;
	uint32	trim_start_x;
	uint32	trim_start_y;
	uint32	trim_size_x;
	uint32	trim_size_y;
	//cur frame
	uint32	cur_frame_addr[MAX_FUSION_FRAME_NUM][2];
	uint32	cur_frame_pitch[2];
	uint32	cur_frame_width;
	uint32	cur_frame_height;
	uint32	fetch_ref_mode;
	//uint32	first_line_mode_en;
	//uint32	last_line_mode_en;
	uint32  ref_frame_addr[2][2];//in capture, according to cur frame size to alloc buffer
	uint32	ref_frame_pitch[2];
	//uint32	ref0_frame_pitch[2];
	uint32	ref_frame_width; //in capture, only means first ref frame size
	uint32	ref_frame_height;//in capture, only means first ref frame size

	signed int	mv_capture[MAX_FUSION_FRAME_NUM][2]; // [x,y]
	char	mv_file_preview[STRING_SIZE];
	uint32	vector_on;
	uint32	padding_frame_width;//for fbc
	uint32	padding_frame_height;
	uint32 slice_mode_en;
	uint32  pad_frame_addr[2][2];
	uint32  pad_frame_tile_addr[2][2];
	uint32  pad_frame_header_addr[2][2];
	uint32  pad_frame_buffer_size[2][2];
} isp_fw_3dnr_mem_ctrl_block;

typedef struct _scaler_store_fbc {
	uint32	padding_frame_width;//for fbc
	uint32	padding_frame_height;
	uint32  slice_mode_en;
	uint32  slice_tile_pitch[12];
	//uint32  last_slice_tile_pitch;
	uint32  tile_num_pitch;
	uint32  pad_frame_addr[2][2];//1:y_head 0:y_tile 3:c_head 2:c_tile
	uint32  header_size[2][2];
	uint32  buf_size[2][2];
	//uint32  frame_io_addr[2][2];
	uint32  pad_frame_tile_addr[2][2];
	uint32  pad_frame_header_addr[2][2];
	uint32	output_width;
	uint32	output_height;
}isp_fw_scl_store_fbc_mem_ctrl_block;

typedef struct _3dnr_store_block {
	uint32	store_width;
	uint32  store_height;
	uint32  store_addr[2][2];
	uint32  store_pitch[2];
} isp_fw_3dnr_store_block;

typedef struct _3dnr_fbc_block {
	uint32	FBC_const_color_en;
	uint32  FBC_const_color_0;
	uint32  FBC_const_color_1;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_nr3_fbdc_slice *slice_param;
#endif
} isp_fw_3dnr_fbc_block;

typedef struct _3dnr_fbd_block {
	uint32	y_tile_base_addr;
	uint32	c_tile_base_addr;
	uint32	y_tile_addr_init_x256;
	uint32	y_tiles_num_pitch;
	uint32	y_header_Addr_init;
	uint32	c_tile_addr_init_x256;
	uint32	c_tiles_num_pitch;
	uint32	c_header_Addr_init;
	uint32	y_pixel_size_in_hor;
	uint32	y_pixel_size_in_ver;
	uint32	c_pixel_size_in_hor;
	uint32	c_pixel_size_in_ver;
	uint32	y_pixel_start_in_hor;
	uint32	y_pixel_start_in_ver;
	uint32	c_pixel_start_in_hor;
	uint32	c_pixel_start_in_ver;
	uint32	y_tiles_num_in_hor;
	uint32	y_tiles_num_in_ver;
	uint32	c_tiles_num_in_hor;
	uint32	c_tiles_num_in_ver;
	uint32	padding_frame_width;
	uint32	padding_frame_height;
	uint32  slice_mode_en;
	uint32  pad_frame_addr[2][2];
	uint32  header_size[2][2];
} isp_fw_3dnr_fbd_block;

typedef struct _dcam_fetch_slice {
	uint32 slice_addr;
	uint32 slice_width;
	uint32 slice_height;
	uint32 slice_pitch;
	uint32 slice_st_x;
} dcam_fw_fetch_slice;

typedef struct _dcam_fetch_block {
	uint32	fetch_bypass;
	uint32	fetch_width;
	uint32	fetch_height;
	uint32  fetch_slice_width;
	uint32  fetch_addr[3];
	uint32  fetch_pitch[3];
	uint32  fetch_endian;
	uint32	fetch_img_format;
	uint32	fetch_st_x;
	uint32	fetch_size[3];
	uint32  YUVInputFormat;
	uint32	fetch_color_format;
	uint32	lbuf_share_mode;
	uint32	h_blank;
	uint32	gap_num;
	uint32 fetch_width_after_crop0;
	uint32 fetch_height_after_crop0;
	uint32 fetch_st_x_after_crop0;
	uint32 fetch_st_y_after_crop0;

	uint32 bwu1_bypass;
	uint32 bwu1_bitshift;


#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_fetch_slice *slice_param;
#endif
} dcam_fw_fetch_block;

typedef struct _dcam_common_block {
	uint32	lbuf_share_mode[2];
} dcam_fw_common_block;

typedef struct _dcam_csi_block {
	uint32 csi_id;
	uint32 lane_number		  ;
	uint32 image_height       ;
	uint32 block_size         ;
	uint32 image_width        ;
	uint32 hsync_en           ;
	uint32 color_bar_mode     ;
	uint32 ipg_mode           ;
	uint32 ipg_enable         ;
	uint32 block0_bayer_b     ;
	uint32 block0_bayer_g     ;
	uint32 block0_bayer_r     ;
	uint32 block1_bayer_b     ;
	uint32 block1_bayer_g     ;
	uint32 block1_bayer_r     ;
	uint32 block2_bayer_b     ;
	uint32 block2_bayer_g     ;
	uint32 block2_bayer_r     ;
	uint32 bayer_mode         ;
	uint32 frameblanking_size ;
	uint32 lineblanking_size  ;
	uint32 phy_sel; // 0 -- dphy,1 --cphy
} dcam_fw_csi_block;
typedef struct _dcam_bayer2l_block {
	uint32 bayer2y_chanel;
	uint32 bayer2y_mode;
	uint32 bayer2y_bypass;
} dcam_fw_bayer2l_block;

typedef struct _dcam_fw_bayerhist_slice {
	uint32 bayer_hist_sty;
	uint32 bayer_hist_stx;
	uint32 bayer_hist_endy;
	uint32 bayer_hist_endx;
	uint32 bayer_hist_bypass;
}dcam_fw_bayerhist_slice;

typedef struct _dcam_bayerhist_block {
	uint32 hist_bypass	        ;
	uint32 hist_dout_base_waddr[4][2];
	uint32 hist_skip_num	    ;
	uint32 bayer_hist_continuous_mode	    ;
	//uint32 hist_mode_sel	    ;
	uint32 bayer_hist_sty	    ;
	uint32 bayer_hist_stx	    ;
	uint32 bayer_hist_endy	    ;
	uint32 bayer_hist_endx	    ;
	uint32 bayer_hist_hdr_en;
	uint32 bayer_hist_hdr_zigzag_init_r;
	uint32 bayer_hist_hdr_zigzag_init_gr;
	uint32 bayer_hist_hdr_zigzag_init_b;
	#if _FPGA_
       uint32 bayerhist_slice_param;
	#else
	dcam_fw_bayerhist_slice *bayerhist_slice_param;
	#endif
} dcam_fw_bayerhist_block;

typedef struct _dcam_fw_crop_slice {
	uint32 slice_bypass;
	uint32 slice_start_x;
	uint32 slice_end_x;
	uint32 slice_start_y;
	uint32 slice_end_y;
	uint32 slice_size_x;
	uint32 slice_size_y;
}dcam_fw_crop_slice;

typedef struct _crop_block {
	uint32 crop_bypass;
	uint32 pipeline_pos;
	uint32 crop_start_x;
	uint32 crop_start_y;
	uint32 crop_end_x;
	uint32 crop_end_y;
	uint32 crop_size_x;
	uint32 crop_size_y;
	uint32 mode;
	#if _FPGA_
	uint32  crop2_slice_param;
	uint32  crop1_slice_param;
	uint32  crop0_slice_param;
	uint32  crop3_slice_param;
       #else
	dcam_fw_crop_slice *crop2_slice_param;
	dcam_fw_crop_slice *crop1_slice_param;
	dcam_fw_crop_slice *crop0_slice_param;
	dcam_fw_crop_slice *crop3_slice_param;
       #endif
} dcam_fw_crop_block;

typedef struct _dcam_lsc_slice {
	uint32 start_col;
	uint32 start_row;
	uint32 RelaticeX_org;
	uint32 RelaticeY_org;
	uint32 WidthGrid;
	uint32 slice_width;
	uint32 slice_height;
	uint32 GridCurrentX_org;
	uint32 GridCurrentY_org;
	uint32 grid_x;
	uint32 grid_y;
	uint32 grid_num;
	uint32 param_addr[2];
	uint16 q_val[2][5][2];
	uint32 grid_buf[2];
} dcam_fw_lsc_slice;

typedef struct _dcam_rds_slice {

	uint32 slice_src_width;
	uint32 slice_src_height;
	uint32 slice_dst_width;
	uint32 slice_dst_height;
	uint32 input_width_global;
	uint32 input_height_global;
	uint32 output_width_global;
	uint32 output_height_global;
	uint32 phase_int0;
	uint32 phase_int1;
	uint32 phase_rdm0;
	uint32 phase_rdm1;
	uint32 rds_coef_table_buf[48];
	uint8 hor_N;
	uint8 ver_N;
	uint8 hor_tap;
	uint8 ver_tap;
}dcam_fw_rds_slice;


typedef struct _dcam_lsc_block {
	uint32	bypass;
	uint32	LNC_GRID;
	uint32  buf_sel;
	uint32	grid_pitch;
	uint32	endian;
	int16	simple_table[129][3];
	uint32	weight_num;
	uint32	weight_table[258];

	uint32	lens_addr[2];
	uint32  buffer_size_bytes;

	uint32 grid_x;
	uint32 grid_y;
	uint32 grid_num;
	uint32 relative_x;
	uint32 relative_y;

	uint32 param_addr[2];
	uint32 slice_num;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_lsc_slice *slice_param;
#endif

}dcam_fw_lsc_block;
typedef struct _dcam_lsc_file_block{

	char	full_r0c0_file[2][STRING_SIZE];
	char	full_r1c1_file[2][STRING_SIZE];
	char	full_r0c1_file[2][STRING_SIZE];
	char	full_r1c0_file[2][STRING_SIZE];
}dcam_lsc_file_block;

typedef struct _dcam_storecrop1_slice {
	uint32 slice_addr;
	uint32 slice_width;
	uint32 slice_height;
	uint32 slice_pitch;
	uint32 slice_st_x;
	uint32 overlap_right;
	uint32 overlap_left;
	uint32 slice_width_dst;
} dcam_fw_storecrop1_slice;

typedef struct _storecrop1_block {
	uint32 store_bypass;
	uint32 store_format;
	uint32 store_endian;
	uint32 store_width;
	uint32 store_height;
	uint32 r_store_width;
	uint32 r_store_height;
	uint32 store_addr[2];
	uint32 store_pitch;
	uint32 store_size;
	uint32 pack_to_mipi_bypss;
	uint32 FBC_enable;
	uint32 FBC_const_color_en;
	uint32 FBC_const_color_0;
	uint32 FBC_const_color_1;
	uint32 FBC_low4bit_en;
	uint32 trim_en;
	uint32 trim_start_x;
	uint32 trim_start_y;
	uint32 trim_size_x;
	uint32 trim_size_y;
	uint32 YUVOutputFormat;
	uint32 store_color_format;
	uint32 domain_sel;
	uint32 base_align;
	uint32 yuv_format;
	uint32 bwd_bypass;
	uint32 bwd_outbits;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_storecrop1_slice *slice_param;
#endif
} dcam_fw_storecrop1_block;

typedef struct _dcam_storecrop2_slice {
	uint32 slice_addr;
	uint32 slice_width;
	uint32 slice_height;
	uint32 slice_pitch;
	uint32 slice_st_x;
	uint32 overlap_right;
	uint32 overlap_left;
	uint32 slice_width_dst;
} dcam_fw_storecrop2_slice;

typedef struct _storecrop2_block {
	uint32 store_bypass;
	uint32 store_mode;
	uint32 store_format;
	uint32 store_endian;
	uint32 store_width;
	uint32 store_height;
	uint32 r_store_width;
	uint32 r_store_height;
	uint32 store_addr[4];
	uint32 store_addr_pong[4];
	uint32 store_pitch;
	uint32 store_size;
	uint32 pack_to_mipi_bypss;
	uint32 FBC_enable;
	uint32 FBC_const_color_en;
	uint32 FBC_const_color_0;
	uint32 FBC_const_color_1;
	uint32 FBC_low4bit_en;
	uint32 trim_en;
	uint32 trim_start_x;
	uint32 trim_start_y;
	uint32 trim_size_x;
	uint32 trim_size_y;
	uint32 YUVOutputFormat;
	uint32 store_color_format;
	uint32 domain_sel;
	uint32 base_align;
	uint32 slow_motion_en;
	uint32 slow_addr_num;
	uint32 yuv_format;
	uint32 bwd_bypass;
	uint32 bwd_outbits;
	uint32 bin_scl_ctl;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_storecrop2_slice *slice_param;
#endif
} dcam_fw_storecrop2_block;

typedef struct _zzhdr_block {
	uint32 raw_hdr_bypass;
	uint32 raw_hdr_bayer_mode;
	uint32 raw_hdr_exp_ratio_int16; // raw_hdr_expo_ratio ,exposure ratio multiplied by 16
	uint32 raw_hdr_zigzag_init_r;
	uint32 raw_hdr_zigzag_init_gr;
	uint32 raw_hdr_zigzag_init_b;
	uint32 raw_hdr_frc_copy;
	uint32 raw_hdr_auto_copy;
	uint32 raw_hdr_Itp_Rec_eb; //0: Bypass 1: Enable
	uint32 raw_hdr_flat_thr;
	uint32 raw_hdr_flat_long_thr;
	uint32 raw_hdr_rb_long_diff_thr;
	uint32 raw_hdr_g_long_diff_thr;
	uint32 raw_hdr_rb_dir_type_key; //gradient ratio*64
	uint32 raw_hdr_g_dir_type_key; //gradient ratio*64
    uint32 raw_hdr_over_exp_thr;
    uint32 raw_hdr_underExpo_low_thr;
    uint32 raw_hdr_underExpo_high_thr;
	uint32 raw_hdr_underexpo_flat_long_thr;
	uint32 raw_hdr_underexpo_flat_thr;
	uint32 raw_hdr_underExpo_rb_long_diff_thr;
	uint32 raw_hdr_underExpo_g_long_diff_thr;
	uint32 raw_hdr_rb_45_135_min_thr;
	uint32 raw_hdr_g_h_v_min_thr;
	uint32 raw_hdr_k_dir; //gradient ratio*64
	uint32 raw_hdr_rb_spec_k_dir; //gradient ratio*64
	uint32 raw_hdr_g_spec_k_dir; //gradient ratio*64
	uint32 raw_hdr_texture_addback_eb;//1: Enable/0: disable
	uint32 raw_hdr_minAct;
	uint32 raw_hdr_maxAct;
 	uint32 raw_hdr_mc_on;//1: Enable/0: disable
 	uint32 raw_hdr_minMot;
	uint32 raw_hdr_maxMot;
 	uint32 raw_hdr_t2_smoothing;//1: Enable/0: disable
	uint32 raw_hdr_in_low;
	uint32 raw_hdr_in_mid;
	uint32 raw_hdr_sig_low;
	uint32 raw_hdr_sig_mid;
	uint32 raw_hdr_slope_mid;
	uint32 raw_hdr_slope_low;
	uint32 raw_hdr_Rec_ThrShort;
	uint32 raw_hdr_Rec_Trans; //128,256, 512
	uint32 raw_hdr_tm_luma_est_mode; //0: YUV mode 1: HSL mode 2: Local filter mode
	uint32 raw_hdr_deghost_mode;//0-simple, 1-short, 2-long
	uint32 raw_hdr_min_percentile_16bit;
	uint32 raw_hdr_max_percentile_16bit;
	uint32 raw_hdr_imgkey_setting_mode;// 0-user setting, 1-auto compute by HW.if hdr_imgkey_setting_mode=1, hdr_tm_param_calc_by_hw should be 1'b1
	uint32 raw_hdr_video_luma_smooth_thr; //Ymax_diff_thr
	uint32 raw_hdr_cur_Ymin_weight;
	uint32 raw_hdr_pre_ymin_weight;
	uint32 raw_hdr_tarNorm_setting_mode;
	char   raw_hdr_c_tarCoeff[20];
	float  raw_hdr_tarCoeff;

//calc
	uint32 raw_hdr_pattern;
	uint32 raw_hdr_pattern_shift_y;
	uint32 raw_hdr_pattern_shift_x;
	uint32 raw_hdr_pattern_type;
//
	uint32 raw_hdr_raw_bit_depth; //10bit
	uint32 raw_hdr_hist_total; // width*height 8M, 13M, 16M, 26M ,calc
	uint32 raw_hdr_min_per; //(min_percentile_16bit*hist_total)>>16,calc
	uint32 raw_hdr_max_per; // (max_percentile_16bit*hist_total)>>16 ,calc
	uint16 raw_hdr_tm_lumafilter_c[3][3];
	uint16 raw_hdr_tm_lumafilter_shift;
	uint32 raw_hdr_ymin;
	uint32 raw_hdr_ymax;
	uint32 raw_hdr_yavg;
	uint32 raw_hdr_log_min_int; //log2_fixed_point_Impl(Ymin + Lr_int), calc
	uint32 raw_hdr_lr_int; //Ymax*imgKey, calc
	uint32 raw_hdr_log_diff_int; //Log_max_int - Log_min_int, calc
	uint32 raw_hdr_log_max_int;// log2_fixed_point_Impl(Ymax + Lr_int) ,calc
	uint32 raw_hdr_log_diff;//log_diff = 1/log_diff_int
	uint32 raw_hdr_imgkey_setting_value;
	uint32 raw_hdr_target_norm;
	uint32 raw_hdr_target_norm_sw;
	uint32 raw_hdr_act_diff; //(1<<15)/(maxAct-minAct), calc
	uint32 raw_hdr_mot_diff; // (1<<15)/(maxMot-minMot),calc
	uint32 raw_hdr_expo_inv; // (1<<15)/expo_ratio ,calc
	uint32 raw_hdr_short_under_exp_high_thr; // under_exp_high_thr *256/ expo_ratio_int16, calc
	uint32 raw_hdr_short_under_exp_low_thr;// under_exp_low_thr*256/ expo_ratio_int16, calc
	uint32 raw_hdr_long_under_exp_high_thr;//(short_under_exp_high_thr*expo_ratio_int16)>>4, calc
	uint32 raw_hdr_long_under_exp_low_thr;//(short_under_exp_low_thr*expo_ratio_int16)>>4
	uint32 raw_hdr_tm_in_bit_depth; // depend on expo_ratio clac  10, 11, 12, 13, 14
	uint32 raw_hdr_tm_out_bit_depth; // 10bit
	uint32 raw_hdr_histBin[ZZHDR_NUM_HIST_BINS];
	uint32 raw_hdr_tm_param_calc_by_hw;
	uint32 raw_hdr_cur_is_first_frame; // valid when hdr_tm_param_calc_by_hw==1
	uint32 raw_hdr_pause_cur_frame_hist_stat;// valid when hdr_tm_param_calc_by_hw==1
	uint32 raw_hdr_ymax_diff_thr;
} dcam_fw_zzhdr_block;

typedef struct _4in1_block {
	uint32 bypass;
	uint32 bin_4in1_clip;
	uint32 bin_mode;
}dcam_fw_4in1_block;

#define TOTAL_PHASE         (2*4)
#define FILTER_TAP_H        8
#define FILTER_TAP_V        4

typedef struct _rds_block {
	uint32 bypass;
	uint32 rds_des_height;
	uint32 rds_des_width;
	uint32 rds_coef_table_buf[48];
	uint8 hor_N;
	uint8 ver_N;
	uint8 hor_tap;
	uint8 ver_tap;
	int16 hor_weight_table[(TOTAL_PHASE)][(FILTER_TAP_H)];
	int16 ver_weight_table[(TOTAL_PHASE)][(FILTER_TAP_V)];
	uint32 phase_int0;
	uint32 phase_int1;
	uint32 phase_rmd0;
	uint32 phase_rmd1;
	uint32 input_width_global;
	uint32 input_height_global;
	uint32 output_width_global;
	uint32 output_height_global;
#if _FPGA_
	uint32  slice_param;
#else
	dcam_fw_rds_slice *slice_param;
#endif
}dcam_fw_rds_block;

typedef struct _fetch_fbd_slice {
	uint32 slice_pixel_start_in_hor;
	uint32 slice_pixel_start_in_ver;
	uint32 slice_height;
	uint32 slice_width;
	uint32 slice_tiles_num_in_ver;
	uint32 slice_tiles_num_in_hor;
	uint32 slice_header_addr_init;
	uint32 slice_tile_addr_init_x256;
	uint32 slice_low_bit_addr;
	uint32 slice_tiles_start_odd;
	uint32 slice_low_bit_addr_4bit;
	uint32 slice_fetch_fbd_4bit_bypass;
} isp_fw_fetch_fbd_slice;

typedef struct _fetch_fbd_header {
	uint8 tile[3]; //fbc ascii
	uint8 tile_type;
	uint32 img_w_org;
	uint32 img_h_org;
	uint32 img_w;
	uint32 img_h;
}isp_fw_fetch_fbd_header;

typedef struct _fetch_fbd_header_colour_en {
	uint8 const_colour_en;
}isp_fw_fetch_fbd_header_colour_en;

typedef struct _fetch_fbd_header_colour {
	uint32 const_colour[2];
}isp_fw_fetch_fbd_header_colour;

typedef struct _fetch_fbd_block {
	uint32 fetch_fbd_bypass;
	uint32 pixel_start_in_hor;
	uint32 pixel_start_in_ver;
	uint32 height;
	uint32 width;
	uint32 padding_frame_height;
	uint32 padding_frame_width;
	uint32 tiles_num_in_ver;
	uint32 tiles_num_in_hor;
	uint32 tiles_start_odd;
	uint32 tiles_num_pitch;
	uint32 header_addr_init;
	uint32 tile_addr_init_x256;
	uint32 fbd_cr_ch0123_val0;
	uint32 fbd_cr_ch0123_val1;
	uint32 fbd_cr_uv_val1;
	uint32 fbd_cr_y_val1;
	uint32 fbd_cr_uv_val0;
	uint32 low_bit_addr_init;
	uint32 low_bit_pitch;
	uint32 fetch_addr;
	uint32 fetch_size;
	isp_fw_fetch_fbd_header header;
	isp_fw_fetch_fbd_header_colour_en header_colour_en;
	isp_fw_fetch_fbd_header_colour header_colour;
	uint32 tile_size;
	uint32 tile_addr;
	uint32 lowbit_size;
	uint32 lowbit_addr;
	uint32 high_bit_buffer;
	uint32 low_bit_buffer;
	uint32 buffer_offset_addr;
	uint32 fbd_header_addr_init;
	uint32 fbd_tile_addr_init_x256;
	uint32 fbd_low_bit_addr_init;
	uint32 hblank_en;
	uint32 hblank_num;
	uint32 low_bit_addr_init_4bit;
	uint32 low_bit_pitch_4bit;
	uint32 lowbit_size_4bit;
	uint32 lowbit_addr_4bit;
	uint32 low_bit_buffer_4bit;
	uint32 fbd_low_bit_addr_init_4bit;
	uint32 fetch_fbd_4bit_bypass;
#if _FPGA_
	uint32  slice_param;
#else
	isp_fw_fetch_fbd_slice *slice_param;
#endif
}isp_fw_fetch_fbd_block;

struct ISP_PARAM_T {
	isp_fw_general_block				general_info;
	isp_fw_pgg_block					pgg_info;
	isp_fw_blc_block					blc_info;
	//dcam_dev_gain_info				gain_info;
	isp_fw_nlm_block					nlm_info;
	//added the GrGb_imblance
	isp_fw_grgb_imblance				grgb_imblance;
	isp_fw_rgbg_block					rgbg_info;
	isp_fw_nlc_block					nlc_info;
	isp_fw_lsc_2d_block					lsc_2d_info;
	isp_fw_lsc_1d_block					lsc_1d_info;
	isp_fw_awbc_block					awbc_info;
	isp_fw_binning_block				binning_info;
	isp_fw_grgb_block					grgb_info;
	isp_fw_cfa_block					cfai_info;
	isp_fw_cmc_block					cmc_info;
	isp_fw_gamma_block					gamma_info;
	isp_fw_hsv_block					hsv_info;
	isp_fw_pstrz_block					pstrz_info;
	isp_fw_cce_block					cce_info;
	isp_fw_uvd_block					uvd_info;
	isp_fw_afl_block					afl_info;
	isp_fw_image_adj_block				image_adj_info;
	isp_fw_hist_block					hist_info;
	isp_fw_ee_block						ee_info;
	isp_fw_cdn_block					cdn_info;
	isp_fw_pre_cdn_block				pre_cdn_info;
	isp_fw_post_cdn_block				post_cdn_info;
	isp_fw_ygamma_block					ygamma_info;
	isp_fw_iir_block					iir_info;
	isp_fw_yrandom_block				yrandom_info;
	isp_fw_ynr_block					ynr_info;
	isp_fw_slice						slice_info;
	isp_fw_scaler_block					yuv_scaler_info[SCALER_NUM];
	isp_fw_scl_store_fbc_mem_ctrl_block		yuv_scl_store_fbc_info[SCALER_NUM];
	isp_fw_arbiter_block				arbiter_info;
	isp_fw_common_block					common_info;
	isp_fw_noisefilter_block			noisefilter_info;
	isp_fw_store_block					store_info;
	isp_fw_fetch_block					fetch_info;
	isp_fw_fetch_fbd_block					fetch_fbd_info;
	isp_fw_dispatch_block				dispatch_info;
	isp_fw_fmcu_block					fmcu_info;
	isp_fw_iommu_block					iommu_info;
	isp_fw_ydelay_block					ydelay_info;
	isp_fw_cfg_block					cfg_info;
	isp_fw_int_block					int_info;
	isp_fw_ltm_stat_block				ltm_stat_info[LTM_NUM];
	isp_fw_ltm_map_block				ltm_map_info[LTM_NUM];
	isp_fw_3dnr_block					yuv_3dnr_info;
	isp_fw_3dnr_mem_ctrl_block					yuv_3dnr_mem_ctrl_info;
	isp_fw_3dnr_store_block					yuv_3dnr_store_info;
	isp_fw_3dnr_fbc_block					yuv_3dnr_fbc_info;
	isp_fw_3dnr_fbd_block					yuv_3dnr_fbd_info;
	isp_fw_thumbnailscaler_block	thumbnailscaler_info;
	dcam_fw_fetch_block dcam_fetch_info;
	dcam_fw_common_block dcam_common_info;
	dcam_fw_crop_block crop0_info;
	dcam_fw_crop_block crop1_info;
	dcam_fw_crop_block crop2_info;
	dcam_fw_crop_block crop3_info;
	dcam_fw_storecrop1_block storecrop1_info;
	dcam_fw_storecrop2_block storecrop2_info;
	dcam_fw_csi_block				dcam_csi_info;
	dcam_fw_bayer2l_block			dcam_bayer2l_info;
	dcam_fw_bayerhist_block			dcam_bayerhist_info;
	dcam_fw_lsc_block				dcam_lsc_info;
	dcam_lsc_file_block				dcam_lsc_file;
	dcam_fw_zzhdr_block				zzhdr_info[PINGPANG];
	dcam_fw_4in1_block				bin_4in1;
	dcam_fw_rds_block				rds_info;
	dcam_fw_aem_block				aem_info;
	dcam_fw_afm_block			    afm_info;
	dcam_fw_bpc_block				bpc_info;
	dcam_fw_me_block				rgb_me_info;
	dcam_fw_yuv_deci_block			dcam_yuv_deci_info;
	dcam_fw_fbc_block				dcam_fbc_info;
	dcam_fw_raw_gtm_stat_block		dcam_raw_gtm_stat_info;
	dcam_fw_raw_gtm_map_block		dcam_raw_gtm_map_info;
	dcam_fw_lscm_block              dcam_lscm_info;
	dcam_fw_slice					dcam_slice_info;
	dcam_fw_slice_after_crop0		dcam_slice_info_after_crop0;
	dcam_fw_ppe_block               ppe_info;
};

struct host_info_t {
	uint32	enable;
	uint32	path_num;
	uint32	reg_base;
	uint32	dcam_reg_base;
	uint32	semphore_base;
	uint32	img_num;
	uint32	current_img;

	char	config_file[STRING_SIZE];
	char	stream_file[STRING_SIZE];
	char	img_list[IMAGE_LIST_SIZE][STRING_SIZE];
	char	output_file[STRING_SIZE];
	char	output_root[STRING_SIZE];
	char	output_vector_fpga[STRING_SIZE];

	ISP_PARAM_T	isp_param;

	std::deque<int>		FMCU_cmd[IMAGE_LIST_SIZE][SLICE_SIZE_MAX];
	std::vector<int>	bad_pixel_pos; //(y << 16 | x)
};

class Debugger
{
public:
	Debugger();
	void open();
	void switchPath(int pathNum);
	void close();
	void writeBufferLog(char *log);
	void writeVectorLog(char *log);
private:
	int _curPath;
	FILE *g_fp_buffer;
	FILE *g_fp_vector;
	FILE *g_fp_vector_separate[ISP_PATH_NUM];
};

class HostControl
{
public:
	HostControl();
	~HostControl();
	void Start(struct host_info_t *info);
	void PathReady();
	void WaitAllPathReady();
	void WaitAllPathDone();
	void FileLock();
	void FileUnLock();
private:
	int _enable[ISP_PATH_NUM];
	pthread_t _tid[ISP_PATH_NUM];
	int _pathNum;
	volatile int _path_ready_num;
	pthread_mutex_t	_path_ready_mutex;
	pthread_cond_t	_path_ready_cond;
	pthread_mutex_t	_fw_mutex_op_file;
};

int isp_path_init(DrvCaseComm *json2);
int isp_read_param_from_file(struct host_info_t *host_info);

#endif
