#ifndef _LSC_ADV_H_
#define _LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/

#include "sensor_raw.h"
#include <sys/types.h>
#include <pthread.h>
#include <android/log.h>
#include "stdio.h"
#include "isp_pm.h"

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/

#define max(A,B) (((A) > (B)) ? (A) : (B))
#define min(A,B) (((A) < (B)) ? (A) : (B))

/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/

typedef void *lsc_adv_handle_t;

#define ISP_1_0 	1
#define ISP_2_0 	2

#define ISP_ALSC_SUCCESS 0
#define ISP_ALSC_ERROR -1

#define LSCCTRL_EVT_BASE            0x2000
#define LSCCTRL_EVT_INIT            LSCCTRL_EVT_BASE
#define LSCCTRL_EVT_DEINIT          (LSCCTRL_EVT_BASE + 1)
#define LSCCTRL_EVT_IOCTRL          (LSCCTRL_EVT_BASE + 2)
#define LSCCTRL_EVT_PROCESS         (LSCCTRL_EVT_BASE + 3)

/*RAW RGB BAYER*/
#define SENSOR_IMAGE_PATTERN_RAWRGB_GR                0x00
#define SENSOR_IMAGE_PATTERN_RAWRGB_R                 0x01
#define SENSOR_IMAGE_PATTERN_RAWRGB_B                 0x02
#define SENSOR_IMAGE_PATTERN_RAWRGB_GB                0x03

enum alsc_io_ctrl_cmd {
	SMART_LSC_ALG_UNLOCK = 0,
	SMART_LSC_ALG_LOCK = 1,
	ALSC_CMD_GET_DEBUG_INFO = 2,
	LSC_INFO_TO_AWB = 3,
	ALSC_GET_VER = 4,
	ALSC_FLASH_MAIN_BEFORE = 5,
	ALSC_FLASH_MAIN_AFTER = 6,
	ALSC_FW_STOP = 7,
	ALSC_FW_START = 8,
	ALSC_FW_START_END = 9,
	ALSC_FLASH_PRE_BEFORE = 10,
	ALSC_FLASH_PRE_AFTER = 11,
	ALSC_FLASH_MAIN_LIGHTING = 12,
	ALSC_FLASH_PRE_LIGHTING = 13,
	ALSC_GET_TOUCH = 14,
	ALSC_FW_PROC_START = 15,
	ALSC_FW_PROC_START_END = 16,
	ALSC_GET_UPDATE_INFO = 17,
	ALSC_UNLOCK_UPDATE_FLAG = 18,
	ALSC_DO_SIMULATION = 19,
};

struct tg_alsc_debug_info {
	cmr_u8 *log;
	cmr_u32 size;
};

struct alsc_ver_info {
	cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
};

struct alsc_update_info {
	cmr_u32 alsc_update_flag;
	cmr_u16 can_update_dest;
	cmr_u16 *lsc_buffer_addr;
};

enum lsc_gain_pattern {
	LSC_GAIN_PATTERN_GRBG = 0,
	LSC_GAIN_PATTERN_RGGB = 1,
	LSC_GAIN_PATTERN_BGGR = 2,
	LSC_GAIN_PATTERN_GBRG = 3,
};

enum lsc_return_value {
	LSC_SUCCESS = 0x00,
	LSC_ERROR,
	LSC_PARAM_ERROR,
	LSC_PARAM_NULL,
	LSC_FUN_NULL,
	LSC_HANDLER_NULL,
	LSC_HANDLER_ID_ERROR,
	LSC_ALLOC_ERROR,
	LSC_FREE_ERROR,
	LSC_RTN_MAX
};

struct lsc_adv_tune_param {
	cmr_u32 enable;
	cmr_u32 alg_id;

	cmr_u32 debug_level;
	cmr_u32 restore_open;

	/* alg 0 */
	cmr_s32 strength_level;
	float pa;				//threshold for seg
	float pb;
	cmr_u32 fft_core_id;	//fft param ID
	cmr_u32 con_weight;		//convergence rate
	cmr_u32 freq;

	/* alg 1 */
	//global
	cmr_u32 alg_effective_freq;
	double gradient_threshold_rg_coef[5];
	double gradient_threshold_bg_coef[5];
	cmr_u32 thres_bv;
	double ds_sub_pixel_ratio;
	double es_statistic_credibility;
	cmr_u32 thres_s1_mi;
	double es_credibility_s3_ma;
	cmr_s32 WindowSize_rg;
	cmr_s32 WindowSize_bg;
	double dSigma_rg_dx;
	double dSigma_rg_dy;
	double dSigma_bg_dx;
	double dSigma_bg_dy;
	double iir_factor;
};

struct lsc2_tune_param {	// if modified, please contact to TOOL team
	// system setting
	unsigned int LSC_SPD_VERSION;	// LSC version of Spreadtrum
	unsigned int number_table;	    // no used

	// control_param
	unsigned int alg_mode;
	unsigned int table_base_index;   // no used
	unsigned int user_mode;
	unsigned int freq;
	unsigned int IIR_weight;

	// slsc2_param
	unsigned int num_seg_queue;      // no used
	unsigned int num_seg_vote_th;    // no used
	unsigned int IIR_smart2;         // no used

	// alsc1_param
	int strength;           // no used

	// alsc2_param
	unsigned int lambda_r;
	unsigned int lambda_b;
	unsigned int weight_r;
	unsigned int weight_b;

	// post_gain
	unsigned int bv2gainw_en;
	unsigned int bv2gainw_p_bv[6];
	unsigned int bv2gainw_b_gainw[6];
	unsigned int bv2gainw_adjust_threshold;    // no used

	// flash_gain
	unsigned int flash_enhance_en;
	unsigned int flash_enhance_max_strength;
	unsigned int flash_enahnce_gain;
};

// change mode (fw_start, fw_stop)
struct alsc_fwstart_info {
	cmr_u16 *lsc_result_address_new;
	cmr_u16 *lsc_tab_address_new[9];
	cmr_u32 gain_width_new;
	cmr_u32 gain_height_new;
	cmr_u32 image_pattern_new;
	cmr_u32 grid_new;
	cmr_u32 camera_id;	// 0. back camera_master  ,  1. front camera_master
	cmr_u32 img_width_new;
	cmr_u32 img_height_new;
};

//for fw proc start
struct alsc_fwprocstart_info {
	cmr_u16 *lsc_result_address_new;
	cmr_u16 *lsc_tab_address_new[9];
	cmr_u32 gain_width_new;
	cmr_u32 gain_height_new;
	cmr_u32 image_pattern_new;
	cmr_u32 grid_new;
	cmr_u32 camera_id;	// 0. back camera_master  ,  1. front camera_master
};

//update flash info
struct alsc_flash_info {
	float io_captureFlashEnvRatio;
	float io_captureFlash1Ratio;
};

struct alsc_do_simulation {
	cmr_u32 *stat_r;
	cmr_u32 *stat_g;
	cmr_u32 *stat_b;
	cmr_u32 ct;
	cmr_s32 bv;
	cmr_s32 bv_gain;
	cmr_u16 *sim_output_table;
};

struct binning_info {
	float ratio;		// binning = 1/2,  double = 2
};

struct crop_info {
	unsigned int start_x;
	unsigned int start_y;
	unsigned int width;
	unsigned int height;
};

enum lsc_transform_action {
	LSC_BINNING = 0,
	LSC_CROP = 1,
	LSC_COPY = 2,
};

struct lsc_table_transf_info {
	unsigned int img_width;
	unsigned int img_height;
	unsigned int grid;
	unsigned int gain_width;
	unsigned int gain_height;

	unsigned short *pm_tab0;
	unsigned short *tab;
};

struct pm_lsc_full {
	unsigned int img_width;
	unsigned int img_height;
	unsigned int grid;
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned short *input_table_buffer;
};

struct pm_lsc_crop {
	unsigned int img_width;
	unsigned int img_height;
	unsigned int start_x;
	unsigned int start_y;
	unsigned int grid;
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned short *output_table_buffer;
};

struct lsc_size {
	cmr_u32 w;
	cmr_u32 h;
};

struct lsc_adv_init_param {
#if !defined (CONFIG_ISP_2_6) && !defined (CONFIG_ISP_2_7)
	cmr_u32 alg_open;		// complie alg0.c or alg2.c
#endif
	cmr_u32 img_width;
	cmr_u32 img_height;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 gain_pattern;
	cmr_u32 output_gain_pattern;
	cmr_u32 change_pattern_flag;
	cmr_u32 grid;
	cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master

	// isp2.1 added , need to modify to match old version
	struct third_lib_info lib_param;

	void *tune_param_ptr;
	cmr_u16 *lsc_tab_address[9];	// the copy of table in parameter file
	struct lsc2_tune_param lsc2_tune_param;	// HLSC_V2.0 tuning structure

	/* no use in lsc_adv2 */
	cmr_u32 param_level;
	cmr_u16 *lum_gain;		// space to save pre_table from smart1.0
#if !defined (CONFIG_ISP_2_6) && !defined (CONFIG_ISP_2_7)
	struct lsc_adv_tune_param tune_param;
#endif
	//otp data
	cmr_u32 lsc_otp_table_en;
	cmr_u32 lsc_otp_table_width;
	cmr_u32 lsc_otp_table_height;
	cmr_u32 lsc_otp_grid;
	cmr_u16 *lsc_otp_table_addr;

	cmr_u32 lsc_otp_oc_en;
	cmr_u32 lsc_otp_oc_r_x;
	cmr_u32 lsc_otp_oc_r_y;
	cmr_u32 lsc_otp_oc_gr_x;
	cmr_u32 lsc_otp_oc_gr_y;
	cmr_u32 lsc_otp_oc_gb_x;
	cmr_u32 lsc_otp_oc_gb_y;
	cmr_u32 lsc_otp_oc_b_x;
	cmr_u32 lsc_otp_oc_b_y;

	//dual cam
	cmr_u8 is_master;
	cmr_u32 is_multi_mode;

	void *otp_info_ptr;

	//add lsc buffer addr
	cmr_u16 *lsc_buffer_addr;

	cmr_u32 lsc_id;
	isp_lsc_cb lsc_set_cb;
	cmr_handle caller_handle;
	cmr_u32 lsc_otp_raw_width;
	cmr_u32 lsc_otp_raw_height;
	cmr_s32 reserved[45];
};

struct statistic_raw_t {
	cmr_u32 *r;
	cmr_u32 *gr;
	cmr_u32 *gb;
	cmr_u32 *b;
};

struct lsc_adv_calc_param {
	struct statistic_raw_t stat_img;	// statistic value of 4 channels
	struct lsc_size stat_size;	// size of statistic value matrix
	cmr_s32 gain_width;		// width  of shading table
	cmr_s32 gain_height;	// height of shading table
	cmr_u32 ct;				// ct from AWB calc
	cmr_s32 r_gain;			// r_gain from AWB calc
	cmr_s32 b_gain;			// b_gain from AWB calc
	cmr_s32 bv;				// bv from AE calc
	cmr_s32 bv_gain;		// AE_gain from AE calc
	cmr_u32 isp_mode;		// about the mode of interperlation of shading table
	cmr_u32 isp_id;			// 0. alg0.c ,  2. alg2.c
	cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
	struct lsc_size img_size;	// raw size
	cmr_s32 grid;			// grid size

	// no use in HLSC_V2.0
	struct lsc_size block_size;
	cmr_u16 *lum_gain;		// pre_table from smart1.0
	cmr_u32 ae_stable;		// ae stable info from AE calc
	cmr_s32 awb_pg_flag;

	cmr_u16 *lsc_tab_address[9];	// lsc_tab_address
	cmr_u32 lsc_tab_size;

	// not fount in isp_app.c
	cmr_u32 pre_bv;
	cmr_u32 pre_ct;

	//for single and dual flash.
	float captureFlashEnvRatio;	//0-1, flash/ (flash+environment)
	float captureFlash1ofALLRatio;	//0-1,  flash1 / (flash1+flash2)

	cmr_handle handle_pm;
	cmr_u16 *std_tab_param[8];
	cmr_s32 reserved[42];
};

struct lsc_adv_calc_result {
	cmr_u16 *dst_gain;
};

struct lsc_trim {
	cmr_u32 x;
	cmr_u32 y;
	cmr_u32 w;
	cmr_u32 h;
};

struct lsc_monitor_info {
	cmr_u32 shift;
	cmr_u32 work_mode;/*single or continue mode*/
	cmr_u32 skip_num;/*skip num: default value is 0*/
	struct lsc_size win_size;
	struct lsc_size win_num;
	struct lsc_trim trim;
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/

// extern used API
cmr_int lsc_ctrl_init(struct lsc_adv_init_param *input_ptr, cmr_handle * handle_lsc);
cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc);
cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr);

cmr_s32 lsc_table_transform(struct lsc_table_transf_info *src, struct lsc_table_transf_info *dst, enum lsc_transform_action action, void *action_info, cmr_u32 input_pattern,
			    cmr_u32 output_pattern);
float table_bicubic_interpolation(unsigned short *src_tab, unsigned int src_gain_width, unsigned int src_gain_height, int TL_i, int TL_j, float dx, float dy);
cmr_s32 lsc_set_monitor(cmr_handle handler, struct lsc_monitor_info *in_param);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/

#endif
// End
