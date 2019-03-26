#ifndef _LSC_LIB_H_
#define _LSC_LIB_H_

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/

typedef void *lsc_adv_handle_t;

/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/

struct lsc2_tune_param {	// if modified, please contact to TOOL team
	// system setting
	cmr_u32 LSC_SPD_VERSION;	// LSC version of Spreadtrum
	cmr_u32 number_table;	    // no used

	// control_param
	cmr_u32 alg_mode;
	cmr_u32 table_base_index;   // no used
	cmr_u32 user_mode;
	cmr_u32 freq;
	cmr_u32 IIR_weight;

	// slsc2_param
	cmr_u32 num_seg_queue;      // no used
	cmr_u32 num_seg_vote_th;    // no used
	cmr_u32 IIR_smart2;         // no used

	// alsc1_param
	cmr_s32 strength;           // no used

	// alsc2_param
	cmr_u32 lambda_r;
	cmr_u32 lambda_b;
	cmr_u32 weight_r;
	cmr_u32 weight_b;

	// post_gain
	cmr_u32 bv2gainw_en;
	cmr_u32 bv2gainw_p_bv[6];
	cmr_u32 bv2gainw_b_gainw[6];
	cmr_u32 bv2gainw_adjust_threshold;    // no used

	// flash_gain
	cmr_u32 flash_enhance_en;               // no used
	cmr_u32 flash_enhance_max_strength;     // no used
	cmr_u32 flash_enahnce_gain;             // no used
};

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
	float captureFlashEnvRatio;	    //0-1, flash/ (flash+environment)
	float captureFlash1ofALLRatio;	//0-1,  flash1 / (flash1+flash2)

	cmr_handle handle_pm;
	cmr_s32 reserved[50];
};

struct lsc_adv_calc_result {
	cmr_u16 *dst_gain;
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

////////////////////////////// alsc_structure //////////////////////////////

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

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/

//lsc.so API
lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param);
int lsc_adv_calculation(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
int lsc_adv_ioctrl(lsc_adv_handle_t handle, int cmd, void *in_param, void *out_param);
int lsc_adv_deinit(lsc_adv_handle_t handle);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/

#endif