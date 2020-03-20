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
#include "alsc.h"

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/

#define max(A,B) (((A) > (B)) ? (A) : (B))
#define min(A,B) (((A) < (B)) ? (A) : (B))
#define RANGE(value, low, high) (max((min((value), (high))),(low)))

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

#define MAX_STAT_WIDTH 32
#define MAX_STAT_HEIGHT 32

typedef struct {
	int grid_size;
	int lpf_mode;
	int lpf_radius;
	int lpf_border;
	int border_patch;
	int border_expand;
	int shading_mode;
	int shading_pct;
} lsc2d_calib_param_t;

struct lsc_wrapper_ops {
	void (*lsc2d_grid_samples) (int w, int h, int gridx, int gridy, int *nx, int *ny);
	void (*lsc2d_calib_param_default) (lsc2d_calib_param_t * calib_param, int grid_size, int lpf_radius, int shading_pct);
	int (*lsc2d_table_preproc) (uint16_t * otp_chn[4], uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
	int (*lsc2d_table_postproc) (uint16_t * tbl_chn[4], int w, int h, int sx, int sy, lsc2d_calib_param_t * calib_param);
};

struct LSC_info2AWB {
	cmr_u16 value[2];	//final_index;
	cmr_u16 weight[2];	// final_ratio;
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
	cmr_u32 stat_r[32 * 32];
	cmr_u32 stat_g[32 * 32];
	cmr_u32 stat_b[32 * 32];
	cmr_u16 lsc_table[32 * 32 * 4];
};

struct lsc_lib_ops {
	cmr_s32(*alsc_calc) (void *handle, struct lsc_sprd_calc_in * param, struct lsc_sprd_calc_out * adv_calc_result);
	void *(*alsc_init) (struct lsc_sprd_init_in * param);
	 cmr_s32(*alsc_deinit) (void *handle);
	 cmr_s32(*alsc_io_ctrl) (void *handler, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
};

struct post_shading_gain_param {
	cmr_s32 bv2gainw_en;
	cmr_s32 bv2gainw_p_bv[6];	// tunable param, src + 4 points + dst
	cmr_s32 bv2gainw_b_gainw[6];	// tunable param, src + 4 points + dst
	cmr_s32 pbits_gainw;
	cmr_s32 pbits_trunc;
	cmr_s32 action_bv;
	cmr_s32 action_bv_gain;
};

struct lsc_flash_proc_param {
	float captureFlashEnvRatio;	//0-1,  flash  / (flash+environment)
	float captureFlash1ofALLRatio;	//0-1,  flash1 / (flash1+flash2)

	//for change mode flash
	cmr_s32 main_flash_from_other_parameter;
	cmr_u16 *preflash_current_lnc_table_address;	// log the current tab[0] when preflash on
	cmr_u16 preflash_current_output_table[32 * 32 * 4];	// copy the current table to restore back when flash off (with post gain)
	cmr_u16 preflash_current_lnc_table[32 * 32 * 4];	// copy the current DNP table
	cmr_u16 preflash_guessing_mainflash_output_table[32 * 32 * 4];	// lsc table after preflash (without post gain)

	//for touch preflash
	cmr_s32 is_touch_preflash;	// 0: normal capture preflash    1: touch preflash     others: not preflash
	cmr_s32 ae_touch_framecount;	// log the frame_count when touching the screen
	cmr_s32 pre_flash_before_ae_touch_framecount;
	cmr_s32 pre_flash_before_framecount;
};

struct lsc_last_info {
	cmr_s32 bv;
	cmr_s32 bv_gain;
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u16 table[32 * 32 * 4];
};

struct lsc_sprd_ctrl_context {
	pthread_mutex_t status_lock;
	void *alsc_handle;	// alsc handler
	void *lib_handle;
	cmr_handle ctrl_handle;
	struct lsc_monitor_info* lscm_info;
	void *lsc_debug_info_ptr;
	void *post_shading_gain_param;
	void *lsc_flash_proc_param;
	void *lsc_last_info;
	struct lsc_lib_ops lib_ops;
	struct third_lib_info *lib_info;
	cmr_u16 *std_init_lsc_table_param_buffer[9];
	cmr_u16 *std_lsc_table_param_buffer[9];
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
	cmr_u16 *last_lsc_table;
	cmr_u16 *output_lsc_table;
	cmr_u16 *lsc_buffer_interlace;
	cmr_u32 is_planar;
	cmr_u32 flash_enhance_ratio;
	cmr_u32 flash_center_shiftx;
	cmr_u32 flash_center_shifty;
	cmr_u32 stats_inverse;
};

struct lsc_param {
	cmr_u32 gain_width;
	cmr_u32 gain_height;
	cmr_u32 pattern;
	cmr_u32 table;
};

#endif
// End
