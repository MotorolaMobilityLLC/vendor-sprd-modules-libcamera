#ifndef _LSC_SPRD_ADAPT_V1_H_
#define _LSC_SPRD_ADAPT_V1_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/

#include "sensor_raw.h"
#include <sys/types.h>
#include <pthread.h>
#include <android/log.h>
#include "stdio.h"
#include "isp_pm.h"
#include "lsc_lib.h"


/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/

#define max(A,B) (((A) > (B)) ? (A) : (B))
#define min(A,B) (((A) < (B)) ? (A) : (B))

/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/

#define ISP_1_0 	1
#define ISP_2_0 	2

#define ISP_ALSC_SUCCESS 0
#define ISP_ALSC_ERROR -1

/*RAW RGB BAYER*/
#define SENSOR_IMAGE_PATTERN_RAWRGB_GR                0x00
#define SENSOR_IMAGE_PATTERN_RAWRGB_R                 0x01
#define SENSOR_IMAGE_PATTERN_RAWRGB_B                 0x02
#define SENSOR_IMAGE_PATTERN_RAWRGB_GB                0x03

#define LSCCTRL_EVT_BASE            0x2000
#define LSCCTRL_EVT_INIT            LSCCTRL_EVT_BASE
#define LSCCTRL_EVT_DEINIT          (LSCCTRL_EVT_BASE + 1)
#define LSCCTRL_EVT_IOCTRL          (LSCCTRL_EVT_BASE + 2)
#define LSCCTRL_EVT_PROCESS         (LSCCTRL_EVT_BASE + 3)

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
	ALSC_DO_SIMULATION = 18,
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

struct LSC_info2AWB {
	cmr_u16 value[2];		//final_index;
	cmr_u16 weight[2];		// final_ratio;
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

// change mode (fw_start, fw_stop)
struct alsc_fwstart_info {
	cmr_u16 *lsc_result_address_new;
	cmr_u16 *lsc_tab_address_new[9];
	cmr_u32 gain_width_new;
	cmr_u32 gain_height_new;
	cmr_u32 image_pattern_new;
	cmr_u32 grid_new;
	cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
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
	cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
};

//update flash info
struct alsc_flash_info {
	float io_captureFlashEnvRatio;
	float io_captureFlash1Ratio;
};

//simulation info
struct alsc_simulation_info {
	cmr_u32 raw_width;
	cmr_u32 raw_height;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 grid;
	cmr_u32 flash_mode;
	cmr_u32 ct;
	cmr_s32 bv;
	cmr_s32 bv_gain;
	cmr_u32 stat_r[32*32];
	cmr_u32 stat_g[32*32];
	cmr_u32 stat_b[32*32];
	cmr_u16 lsc_table[32*32*4];
};

struct alsc_do_simulation{
	cmr_u32* stat_r;
	cmr_u32* stat_g;
	cmr_u32* stat_b;
	cmr_u32 ct;
	cmr_s32 bv;
	cmr_s32 bv_gain;
	cmr_u16* sim_output_table;
};

struct lsc_lib_ops {
	cmr_s32(*alsc_calc) (void *handle, struct lsc_adv_calc_param * param, struct lsc_adv_calc_result * adv_calc_result);
	void *(*alsc_init) (struct lsc_adv_init_param * param);
	cmr_s32(*alsc_deinit) (void *handle);
	cmr_s32(*alsc_io_ctrl) (void *handler, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
};

struct post_shading_gain_param {
	cmr_s32 bv2gainw_en;
	cmr_s32 bv2gainw_p_bv[6];     // tunable param, src + 4 points + dst
	cmr_s32 bv2gainw_b_gainw[6];  // tunable param, src + 4 points + dst
	cmr_s32 pbits_gainw;
	cmr_s32 pbits_trunc;
	cmr_s32 action_bv;
	cmr_s32 action_bv_gain;
};

struct lsc_flash_proc_param {
	float captureFlashEnvRatio;     //0-1,  flash  / (flash+environment)
	float captureFlash1ofALLRatio;  //0-1,  flash1 / (flash1+flash2)

	//for change mode flash
	cmr_s32 main_flash_from_other_parameter;
	cmr_u16 *preflash_current_lnc_table_address;                 // log the current tab[0] when preflash on
	cmr_u16 preflash_current_output_table[32*32*4];              // copy the current table to restore back when flash off (with post gain)
	cmr_u16 preflash_current_lnc_table[32*32*4];                 // copy the current DNP table
	cmr_u16 preflash_guessing_mainflash_output_table[32*32*4];   // lsc table after preflash (without post gain)

	//for touch preflash
	cmr_s32 is_touch_preflash;                                   // 0: normal capture preflash    1: touch preflash     others: not preflash
	cmr_s32 ae_touch_framecount;                                 // log the frame_count when touching the screen
	cmr_s32 pre_flash_before_ae_touch_framecount;
	cmr_s32 flash_close_after_frame_count;
	cmr_s32 pre_flash_before_framecount;
};

struct lsc_last_info {
	cmr_s32 bv;
	cmr_s32 bv_gain;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u16 table_r[32*32];
	cmr_u16 table_g[32*32];
	cmr_u16 table_b[32*32];
};

struct lsc_otp_context {
	cmr_u16 *golden_otp_r;
	cmr_u16 *golden_otp_gr;
	cmr_u16 *golden_otp_gb;
	cmr_u16 *golden_otp_b;
	cmr_u16 *random_otp_r;
	cmr_u16 *random_otp_gr;
	cmr_u16 *random_otp_gb;
	cmr_u16 *random_otp_b;
	cmr_u16 *prm_gain_r;
	cmr_u16 *prm_gain_gr;
	cmr_u16 *prm_gain_gb;
	cmr_u16 *prm_gain_b;
	float* gain_ratio_g;
	float* gain_ratio_random_rgbg;
	float* gain_ratio_golden_rgbg;
	float* ratio_rgbg;
	float* gain_ratio_input_rgbg;
	float* gain_ratio_output_rgbg;
};

struct lsc_ctrl_context {
	pthread_mutex_t status_lock;
	void *alsc_handle;		// alsc handler
	void *lib_handle;
	void* lsc_debug_info_ptr;
	void* post_shading_gain_param;
	void* lsc_flash_proc_param;
	void* lsc_last_info;
	struct lsc_lib_ops lib_ops;
	struct third_lib_info *lib_info;
	cmr_u16 *std_init_lsc_table_param_buffer[8];   // without table no.8, golden OTP table
	cmr_u16 *std_lsc_table_param_buffer[8];       // without table no.8, golden OTP table
	cmr_u16 *lsc_pm0;
	cmr_u16 *dst_gain;
	cmr_u16 *lsc_buffer;
	cmr_u16 *fwstart_new_scaled_table;
	cmr_u16 *fwstop_output_table;
	cmr_u16 *flash_y_gain;
	cmr_u32 *ae_stat;
	cmr_u32 img_width;
	cmr_u32 img_height;
	cmr_u32 grid;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 init_img_width;
	cmr_u32 init_img_height;
	cmr_u32 init_gain_width;
	cmr_u32 init_gain_height;
	cmr_u32 init_grid;
	cmr_u32 gain_pattern;
	cmr_u32 output_gain_pattern;
	cmr_u32 change_pattern_flag;
	cmr_u32 IIR_weight;
	cmr_u32 calc_freq;
	cmr_u32 frame_count;
	cmr_u32 alg_count;
	cmr_u32 alg_locked;
	cmr_u32 alg_bypass;
	cmr_u32 alg_quick_in;
	cmr_u32 alg_quick_in_frame;
	cmr_s32 quik_in_start_frame;
	cmr_u32 init_skip_frame;
	cmr_u32 flash_mode;
	cmr_u32 pre_flash_mode;
	cmr_u32 can_update_dest;
	//for pre_flash and main_flash
	cmr_s32 flash_main_after_count;
	cmr_s32 flash_pre_after_count;
	cmr_s32 main_flash_after_flag;
	cmr_s32 pre_flash_after_flag;
	cmr_s32 main_flash_before_flag;

	cmr_u32 alsc_update_flag;
	cmr_u32 fw_start_end;
	cmr_u32 lsc_id;
	cmr_u32 camera_id;
	cmr_s32 fw_start_bv;
	cmr_s32 fw_start_bv_gain;
	cmr_s32 bv_memory[10];
	cmr_s32 bv_gain_memory[10];
	cmr_s32 bv_skip_frame;
	cmr_s32 bv_before_flash;
	cmr_s32 bv_gain_before_flash;
	cmr_s32 flash_done_frame_count;
	cmr_u32 LSC_SPD_VERSION;
	cmr_u32 is_multi_mode;
	cmr_u32 is_master;
	cmr_u32 cmd_alsc_cmd_enable;
	cmr_u32 cmd_alsc_table_pattern;
	cmr_u32 cmd_alsc_table_index;
	cmr_u32 cmd_alsc_bypass;
	cmr_u32 cmd_alsc_bypass_otp;
	cmr_u32 cmd_alsc_dump_aem;
	cmr_u32 cmd_alsc_dump_table;
	cmr_u32 cmd_alsc_dump_otp;
	cmr_u32 cur_lsc_pm_mode;
	cmr_u32 pre_lsc_pm_mode;
};

struct binning_info {
	float ratio;			// binning = 1/2,  double = 2
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
	unsigned short* input_table_buffer;
};

struct pm_lsc_crop {
	unsigned int img_width;
	unsigned int img_height;
	unsigned int start_x;
	unsigned int start_y;
	unsigned int grid;
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned short* output_table_buffer;
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/

// extern used API
cmr_int lsc_ctrl_init(struct lsc_adv_init_param *input_ptr, cmr_handle * handle_lsc);
cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc);
cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr);

cmr_s32 lsc_table_transform(struct lsc_table_transf_info *src, struct lsc_table_transf_info *dst, enum lsc_transform_action action, void *action_info, cmr_u32 input_pattern, cmr_u32 output_pattern);
float table_bicubic_interpolation(unsigned short *src_tab, unsigned int src_gain_width, unsigned int src_gain_height, int TL_i, int TL_j, float dx, float dy);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/

#endif
// End
