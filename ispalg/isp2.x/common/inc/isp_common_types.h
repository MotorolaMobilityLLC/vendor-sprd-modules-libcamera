
#ifndef _3A_COMMON_H
#define _3A_COMMON_H

#include "ae_ctrl_types.h"

struct tg_ae_ctrl_alc_log {	
	cmr_u8 *log;
	cmr_u32 size;
};

struct awb_ctrl_gain {
	cmr_u32 r;
	cmr_u32 g;
	cmr_u32 b;
};

struct tg_awb_ctrl_alc_log {
	cmr_u8 *log;
	cmr_u32 size;
} log_awb, log_lsc;

struct awb_ctrl_calc_result {
	struct awb_ctrl_gain gain;
	cmr_u32 ct;
	cmr_u32 use_ccm;
	cmr_u16 ccm[9];
//ALC_S 20150519
	cmr_u32 use_lsc;
	cmr_u16 *lsc;
//ALC_S 20150519
	cmr_u32 lsc_size;
/*ALC_S*/
	struct tg_awb_ctrl_alc_log log_awb;
	struct tg_awb_ctrl_alc_log log_lsc;
/*ALC_E*/
	cmr_s32 pg_flag;
	cmr_s32 green100;
};

struct ae_calc_out {
	cmr_u32 cur_lum;
	cmr_u32 cur_index;
	cmr_u32 cur_ev;
	cmr_u32 cur_exp_line;
	cmr_u32 cur_dummy;
	cmr_u32 cur_again;
	cmr_u32 cur_dgain;
	cmr_u32 cur_iso;
	cmr_u32 is_stab;
	cmr_u32 line_time;
	cmr_u32 frame_line;
	cmr_u32 target_lum;
	cmr_u32 flag;
	float *ae_data;
	cmr_s32 ae_data_size;
	cmr_u32 target_lum_ori;
	cmr_u32 flag4idx;
	cmr_u32 cur_bv;
	float exposure_time;
#ifdef CONFIG_CAMERA_DUAL_SYNC
	cmr_u32 sec;
	cmr_u32 usec;
	cmr_s64 monoboottime;
#endif
	struct tg_ae_ctrl_alc_log log_ae;
};

struct ae_ctrl_flash_param {
	float captureFlashEnvRatio;
	float captureFlash1ofALLRatio;
};

struct ae_ctrl_callback_in {
	cmr_u32 is_skip_cur_frame;
	struct ae_alg_calc_result ae_result;
	struct ae_calc_out ae_output;
	struct ae_get_ev ae_ev;
	struct ae_monitor_info monitor_info;
	struct ae_ctrl_flash_param flash_param;
	struct awb_ctrl_calc_result awb_calc;
};

#endif

