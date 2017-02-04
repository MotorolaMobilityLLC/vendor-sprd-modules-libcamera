#ifndef _ISP_LSC_ADV_H_
#define _ISP_LSC_ADV_H_

/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#ifdef WIN32
#include "data_type.h"
#else
#include <sys/types.h>
#endif

#include "stdio.h"
#include "sensor_raw.h"
#include "sensor_drv_u.h"
#include <sys/types.h>
#include <pthread.h>
#include <utils/Log.h>

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/
#define LSC_ADV_DEBUG_STR       "LSC_ADV: L %d, %s: "
#define LSC_ADV_DEBUG_ARGS    __LINE__,__FUNCTION__

#define LSC_ADV_LOGE(format,...) ALOGE(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGW(format,...) ALOGW(LSC_ADV_DEBUG_STR, format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)

#define LSC_ADV_LOGI(format,...) ALOGI(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGD(format,...) ALOGD(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)
#define LSC_ADV_LOGV(format,...) ALOGV(LSC_ADV_DEBUG_STR format, LSC_ADV_DEBUG_ARGS, ##__VA_ARGS__)


/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
typedef void* lsc_adv_handle_t;

#define ISP_1_0 	1
#define ISP_2_0 	2

#define ISP_ALSC_SUCCESS 0
#define ISP_ALSC_ERROR -1

#define NUM_ROWS 24
#define NUM_COLS 32

#define MAX_SEGS 8

#define MAX_TAB_WIDTH  64
#define MAX_TAB_HEIGHT  64
#define MAX_SHD_TABLE MAX_TAB_WIDTH*MAX_TAB_HEIGHT

#define ALSC_STAT_W 18
#define ALSC_STAT_H 14

/*RAW RGB BAYER*/
#define SENSOR_IMAGE_PATTERN_RAWRGB_GR                0x00
#define SENSOR_IMAGE_PATTERN_RAWRGB_R                 0x01
#define SENSOR_IMAGE_PATTERN_RAWRGB_B                 0x02
#define SENSOR_IMAGE_PATTERN_RAWRGB_GB                0x03

#define BLOCK_PARAM_CFG(input, param_data, blk_cmd, blk_id, cfg_ptr, cfg_size)\
	do {\
		param_data.cmd = blk_cmd;\
		param_data.id = blk_id;\
		param_data.data_ptr = cfg_ptr;\
		param_data.data_size = cfg_size;\
		input.param_data_ptr = &param_data;\
		input.param_num = 1;} while (0);


enum alsc_io_ctrl_cmd{
	SMART_LSC_ALG_UNLOCK = 0,
	SMART_LSC_ALG_LOCK = 1,
	ALSC_CMD_GET_DEBUG_INFO = 2,
	LSC_INFO_TO_AWB = 3,
};

enum lsc_gain_pattern {
	LSC_GAIN_PATTERN_GRBG = 0,
	LSC_GAIN_PATTERN_RGGB = 1,
	LSC_GAIN_PATTERN_BGGR = 2,
	LSC_GAIN_PATTERN_GBRG = 3,
};


#define LSCCTRL_EVT_BASE            0x2000
#define LSCCTRL_EVT_INIT            LSCCTRL_EVT_BASE
#define LSCCTRL_EVT_DEINIT          (LSCCTRL_EVT_BASE + 1)
#define LSCCTRL_EVT_IOCTRL          (LSCCTRL_EVT_BASE + 2)
#define LSCCTRL_EVT_PROCESS         (LSCCTRL_EVT_BASE + 3)


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

enum interp_table_index{
	LSC_TAB_A = 0,
	LSC_TAB_TL84,
	LSC_TAB_CWF,
	LSC_TAB_D65,
	LSC_TAB_DNP,
};

struct LSC_Segs{
	int table_pair[2];//The index to the shading table pair.
	int max_CT;//The maximum CT that the pair will be used.
	int min_CT;//The minimum CT that the pair will be used.
};

struct LSC_Setting{
	struct LSC_Segs segs[MAX_SEGS];//The shading table pairs.
	int seg_count;//The number of shading table pairs.

	double smooth_ratio;//the smooth ratio for the output weighting
	int proc_inter;//process interval of LSC(unit=frames)

	int num_seg_queue;//The number of elements for segement voting.
	int num_seg_vote_th;//The voting threshold for segmenet voting.
};

struct statistic_raw_t {
	uint32_t *r;
	uint32_t *gr;
	uint32_t *gb;
	uint32_t *b;
};

struct lsc_size {
	uint32_t w;
	uint32_t h;
};

struct lsc_adv_tune_param {
	uint32_t enable;
	uint32_t alg_id;
	uint32_t debug_level;
	uint32_t restore_open;

	/* alg 0 */
	int strength_level;
	float pa;				//threshold for seg
	float pb;
	uint32_t fft_core_id; 	//fft param ID
	uint32_t con_weight;		//convergence rate
	uint32_t freq;

	/* alg 1 */
	//global
	uint32_t alg_effective_freq;
	double gradient_threshold_rg_coef[5];
	double gradient_threshold_bg_coef[5];	
	uint32_t thres_bv;
	double ds_sub_pixel_ratio;
	double es_statistic_credibility;
	uint32_t thres_s1_mi;
	double es_credibility_s3_ma;
	int WindowSize_rg;
	int WindowSize_bg;
	double dSigma_rg_dx;
	double dSigma_rg_dy;
	double dSigma_bg_dx;
	double dSigma_bg_dy;
	double iir_factor;
};

struct alsc_alg0_turn_para{
	float pa;				//threshold for seg
	float pb;
	uint32_t fft_core_id; 	//fft param ID
	uint32_t con_weight;		//convergence rate
	uint32_t bv;
	uint32_t ct;
	uint32_t pre_ct;
	uint32_t pre_bv;
	uint32_t restore_open;
	uint32_t freq;
	float threshold_grad;
};

struct lsc_adv_init_param {
	uint32_t gain_pattern;
	int gain_width;
	int gain_height;
	uint16_t *lum_gain;
	struct lsc_adv_tune_param tune_param;
	uint32_t alg_open;
	uint32_t param_level;
	uint32_t camera_id;
	struct LSC_Setting SLSC_setting;
	uint32_t grid;
	struct third_lib_info lib_param;
};

struct lsc_adv_calc_param{
	struct statistic_raw_t stat_img;
	struct lsc_size stat_size;
	struct lsc_size img_size;

	int gain_width;
	int gain_height;
	uint16_t *lum_gain;

	struct lsc_size block_size;
	uint32_t bv;
	uint32_t pre_bv;
	uint32_t ct;

	int r_gain;
	int b_gain;

	uint32_t pre_ct;
	uint32_t camera_id;
	uint32_t ae_stable;
	uint32_t isp_mode;
	uint32_t isp_id;

	int awb_pg_flag;
	unsigned short* lsc_tab_address[9];
	uint32_t lsc_tab_size;
};
struct lsc_adv_calc_result {
	uint16_t *dst_gain;
};
struct lsc_weight_value {
	int32_t value[2];
	uint32_t weight[2];
};

struct lsc_adv_info {
	uint32_t flat_num;
	uint32_t flat_status1;
	uint32_t flat_status2;
	uint32_t stat_r_var;
	uint32_t stat_b_var;
	uint32_t cali_status;

	uint32_t alg_calc_cnt;
	struct lsc_weight_value cur_index;
	struct lsc_weight_value calc_index;
	uint32_t cur_ct;

	uint32_t alg2_enable;
	uint32_t alg2_seg1_num;
	uint32_t alg2_seg2_num;
	uint32_t alg2_seg_num;
	uint32_t alg2_seg_valid;
	uint32_t alg2_cnt;
	uint32_t center_same_num[4];
};

struct lsc_adv_context {
    void* lsc_alg;
    int32_t gain_pattern;
    int32_t alg_locked;
    int gain_width;
    int gain_height;

    int image_width;
    int image_height;

    uint16_t *lum_gain;
	uint32_t * stat_ptr;
	uint32_t grid;
	pthread_mutex_t 	mutex_stat_buf;

	uint32_t* stat_for_alsc;

	int stat_buf_lock;
    float  color_gain[NUM_ROWS * NUM_COLS * 4];
    float  color_gain_bak[NUM_ROWS * NUM_COLS * 4];
    uint32_t pbayer_r_sums[NUM_ROWS * NUM_COLS];
    uint32_t pbayer_gr_sums[NUM_ROWS * NUM_COLS];
    uint32_t pbayer_gb_sums[NUM_ROWS * NUM_COLS];
    uint32_t pbayer_b_sums[NUM_ROWS * NUM_COLS];
    uint32_t alg_open;
	float color_gain_r_10f[NUM_ROWS * NUM_COLS];
	float color_gain_b_10f[NUM_ROWS * NUM_COLS];
	float color_gain_rg_10f[NUM_ROWS * NUM_COLS];
	float color_gain_xxx[10][NUM_ROWS * NUM_COLS*4];
    struct alsc_alg0_turn_para alg0_turn_para;
    struct LSC_Setting SLSC_setting;
};

struct lsc_lib_ops{
	int32_t (* alsc_calc)(void* handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *adv_calc_result);
	void* (* alsc_init)(struct lsc_adv_init_param *param);
	int32_t (* alsc_deinit)(void* handle);
	int32_t (*alsc_io_ctrl)(void* handler, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
};

struct lsc_ctrl_context{
	pthread_mutex_t status_lock;
	void* alsc_handle;    // alsc handler
	void *lib_handle;
	struct lsc_lib_ops lib_ops;
	struct third_lib_info *lib_info;
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
typedef lsc_adv_handle_t (* fun_lsc_adv_init)(struct lsc_adv_init_param *param);
typedef const char* (* fun_lsc_adv_get_ver_str)(lsc_adv_handle_t handle);
typedef int32_t (* fun_lsc_adv_calculation)(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
typedef int32_t (* fun_lsc_adv_ioctrl)(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
typedef int32_t (* fun_lsc_adv_deinit)(lsc_adv_handle_t handle);

//lsc.so API
lsc_adv_handle_t lsc_adv_init(struct lsc_adv_init_param *param);
const char* lsc_adv_get_ver_str(lsc_adv_handle_t handle);
int32_t lsc_adv_calculation(lsc_adv_handle_t handle, struct lsc_adv_calc_param *param, struct lsc_adv_calc_result *result);
int32_t lsc_adv_ioctrl(lsc_adv_handle_t handle, enum alsc_io_ctrl_cmd cmd, void *in_param, void *out_param);
int32_t lsc_adv_deinit(lsc_adv_handle_t handle);
int32_t is_print_lsc_log(void);

// extern used API
cmr_int lsc_ctrl_init(struct lsc_adv_init_param *input_ptr, cmr_handle *handle_lsc);
cmr_int lsc_ctrl_deinit(cmr_handle handle_lsc);
cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result);
cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, enum alsc_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr);

void alsc_set_param(struct lsc_adv_init_param *lsc_param);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End
