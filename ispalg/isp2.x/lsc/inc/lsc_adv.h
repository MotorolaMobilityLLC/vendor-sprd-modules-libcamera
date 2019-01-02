#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#ifdef WIN32
#include "data_type.h"
#include "win_dummy.h"
#else
#include "sensor_raw.h"
#include <sys/types.h>
#include <pthread.h>
#include <android/log.h>
//#include <utils/Log.h>

//#include "isp_com.h"
#endif

#include "stdio.h"
#include "isp_pm.h"

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/

#define max(A,B) (((A) > (B)) ? (A) : (B))
#define min(A,B) (((A) < (B)) ? (A) : (B))

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
	typedef void *lsc_adv_handle_t;

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

	struct debug_lsc_param {
		char LSC_version[50];
		cmr_u16 TB_DNP[12];
		cmr_u16 TB_A[12];
		cmr_u16 TB_TL84[12];
		cmr_u16 TB_D65[12];
		cmr_u16 TB_CWF[12];
		cmr_u32 STAT_R[5];
		cmr_u32 STAT_GR[5];
		cmr_u32 STAT_GB[5];
		cmr_u32 STAT_B[5];
		cmr_u32 gain_width;
		cmr_u32 gain_height;
		cmr_u32 gain_pattern;
		cmr_u32 grid;

		//SLSC
		cmr_s32 error_x10000[9];
		cmr_s32 eratio_before_smooth_x10000[9];
		cmr_s32 eratio_after_smooth_x10000[9];
		cmr_s32 final_ratio_x10000;
		cmr_s32 final_index;
		cmr_u16 smart_lsc_table[1024 * 4];

		//ALSC
		cmr_u16 alsc_lsc_table[1024 * 4];

		//ALSC_SMOOTH
		cmr_u16 alsc_smooth_lsc_table[1024 * 4];

		//OUTPUT
		cmr_u16 last_lsc_table[1024 * 4];
		cmr_u16 output_lsc_table[1024 * 4];
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

////////////////////////////// alsc_structure //////////////////////////////

	struct lsc2_tune_param {	// if modified, please contact to TOOL team
		// system setting
		cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
		cmr_u32 number_table;	// number of used original shading tables

		// control_param
		cmr_u32 alg_mode;
		cmr_u32 table_base_index;
		cmr_u32 user_mode;
		cmr_u32 freq;
		cmr_u32 IIR_weight;

		// slsc2_param
		cmr_u32 num_seg_queue;
		cmr_u32 num_seg_vote_th;
		cmr_u32 IIR_smart2;

		// alsc1_param
		cmr_s32 strength;

		// alsc2_param
		cmr_u32 lambda_r;
		cmr_u32 lambda_b;
		cmr_u32 weight_r;
		cmr_u32 weight_b;

		// post_gain
		cmr_u32 bv2gainw_en;
		cmr_u32 bv2gainw_p_bv[6];
		cmr_u32 bv2gainw_b_gainw[6];
		cmr_u32 bv2gainw_adjust_threshold;

		// flash_gain
		cmr_u32 flash_enhance_en;
		cmr_u32 flash_enhance_max_strength;
		cmr_u32 flash_enahnce_gain;
	};

struct lsc2_context {
	cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
	cmr_u32 alg_mode;	        // control 1d on/off
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 gain_pattern;
	cmr_u32 output_gain_pattern;
	cmr_u32 camera_id;
	cmr_u32 lsc_id;

	void* lsc_debug_info_ptr;
	void* slsc_param;
	void* alsc_param;
	void* lsc1d_param;
	void* reserved_param1;
	void* reserved_param2;
	void* reserved_param3;

	cmr_s32 reserved[50];
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

////////////////////////////// calculation dependent //////////////////////////////

struct lsc_size {
	cmr_u32 w;
	cmr_u32 h;
};

struct lsc_adv_init_param {
	cmr_u32 img_width;
	cmr_u32 img_height;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 gain_pattern;
	cmr_u32 output_gain_pattern;
	cmr_u32 change_pattern_flag;
	cmr_u32 grid;
	cmr_u32 camera_id;		// 0. back camera_master  ,  1. front camera_master
	cmr_u32 lsc_id;

		// isp2.1 added , need to modify to match old version
		struct third_lib_info lib_param;

	/* added parameters */
	cmr_u16 *lum_gain;              // output from smart lsc, no used now
	cmr_u16 *lsc_tab_address[9];	// the address of table parameter
	void *tune_param_ptr;
	void *lsc_debug_info_ptr;

	struct lsc2_tune_param lsc2_tune_param;	// alsc tuning structure

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
		cmr_s32 reserved[50];
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
	cmr_u16 *std_tab_param[8];
	cmr_u32 lsc_tab_size;

		// not fount in isp_app.c
		cmr_u32 pre_bv;
		cmr_u32 pre_ct;

		//for single and dual flash.
		float captureFlashEnvRatio;	//0-1, flash/ (flash+environment)
		float captureFlash1ofALLRatio;	//0-1,  flash1 / (flash1+flash2)

		cmr_handle handle_pm;
		cmr_s32 reserved[50];
	};

	struct lsc_adv_calc_result {
		cmr_u16 *dst_gain;
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

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
	typedef lsc_adv_handle_t(*fun_lsc_adv_init) (struct lsc_adv_init_param * param);
	typedef const char *(*fun_lsc_adv_get_ver_str) (lsc_adv_handle_t handle);
	typedef cmr_s32(*fun_lsc_adv_calculation) (lsc_adv_handle_t handle, struct lsc_adv_calc_param * param, struct lsc_adv_calc_result * result);
	typedef cmr_s32(*fun_lsc_adv_ioctrl) (lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
	typedef cmr_s32(*fun_lsc_adv_deinit) (lsc_adv_handle_t handle);

//lsc.so API
	lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param);
	const char *lsc_adv_get_ver_str(lsc_adv_handle_t handle);
	cmr_s32 lsc_adv_calculation(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
	cmr_s32 lsc_adv_ioctrl(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
	cmr_s32 lsc_adv_deinit(lsc_adv_handle_t handle);
	cmr_s32 is_print_lsc_log(void);

// extern used API
	cmr_int lsc_ctrl_init(struct lsc_adv_init_param *input_ptr, cmr_handle * handle_lsc);
	cmr_int lsc_ctrl_deinit(cmr_handle * handle_lsc);
	cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
	cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, cmr_s32 cmd, void *in_ptr, void *out_ptr);

	cmr_s32 otp_lsc_mod(cmr_u16 * otpLscTabGolden, cmr_u16 * otpLSCTabRandom,	//T1, T2
						cmr_u16 * otpLscTabGoldenRef,	//Ts1
						cmr_u32 * otpAWBMeanGolden, cmr_u32 * otpAWBMeanRandom, cmr_u16 * otpLscTabGoldenMod,	//output: Td2
						cmr_u32 gainWidth, cmr_u32 gainHeight, cmr_s32 bayerPattern);

	cmr_s32 lsc_table_transform(struct lsc_table_transf_info *src, struct lsc_table_transf_info *dst, enum lsc_transform_action action, void *action_info, cmr_u32 input_pattern, cmr_u32 output_pattern);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/

#endif
// End
