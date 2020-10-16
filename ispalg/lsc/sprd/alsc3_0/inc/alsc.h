#ifndef _ALSC_H_
#define _ALSC_H_

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_WIDTH  32
#define MAX_HEIGHT 32


/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/

enum {
	LSC_CMD_DO_POSTPROCESS = 0,
	LSC_CMD_GET_DEBUG_INFO = 1,
	LSC_CMD_GET_OTP_STD_TABLE = 2,
	LSC_CMD_OTP_CONVERT_TABLE = 3,
	LSC_CMD_GENERATE_FLASH_Y_GAIN = 4,
	LSC_CMD_GET_BG_RG_GRADIENT = 5,
	LSC_CMD_GET_ALSC_LIB_VERSION = 6,   //7~10 for both alsc3.0 and alsc3.2
										//11~15 for alsc3.0 only
	LSC_CMD_GET_LSC_VERSION = 16,       //16~ for alsc3.2 only
	LSC_CMD_GET_CALCULATION_FREQ = 17,
	LSC_CMD_MAX
};

struct lsc_otp_convert_param{
	int gain_width;
	int gain_height;
	int grid;
	unsigned short *lsc_table[9];
	int gridy;
	int gridx;
};

struct lsc_flash_y_gain_param{
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned int percent;
	unsigned int shift_x;
	unsigned int shift_y;
};

struct lsc_sprd_init_in {
	unsigned int gain_width;
	unsigned int gain_height;
	unsigned int gain_pattern;
	unsigned int output_gain_pattern;
	unsigned int grid;
	unsigned int camera_id;
	unsigned int lsc_id;
	unsigned int is_planar;

	unsigned short *lsc_tab_address[9];	// the address of table parameter
	void * tune_param_ptr;

	//otp data
	unsigned int lsc_otp_table_en;
	unsigned int lsc_otp_table_width;
	unsigned int lsc_otp_table_height;
	unsigned int lsc_otp_grid;
	unsigned int lsc_otp_raw_width;
	unsigned int lsc_otp_raw_height;
	unsigned short *lsc_otp_table_addr;

	unsigned int gridx;
	unsigned int gridy;

	unsigned int rg_gradient[9];
	unsigned int bg_gradient[9];

	int stat_inverse;

	unsigned int reserved[32];
};

struct lsc_sprd_init_out{
	unsigned short *lsc_otp_std_tab[9];  //lsc tables after otp convert
	unsigned short *lsc_buffer;
};

struct statistic_raw {
	unsigned int *r;
	unsigned int *gr;
	unsigned int *gb;
	unsigned int *b;
	unsigned int w;    // size of statistic value matrix  ###alg lib fixed value
	unsigned int h;
};

struct lsc_sprd_calc_in {
	struct statistic_raw stat_img;  // statistic value of 4 channels
	int gain_width;		              // width  of shading table
	int gain_height;	              // height of shading table
	int grid;			              // grid size
	unsigned int img_width;           // raw size
	unsigned int img_height;
	unsigned short *lsc_tab[8];

	unsigned short last_lsc_table[MAX_WIDTH*MAX_HEIGHT*4];
	unsigned int main_flash_mode;
	float captureFlashEnvRatio;
	float captureFlash1ofAllRatio;
	unsigned short *preflash_guessing_mainflash_output_table;
	int ct;
	int gridx;
	int gridy;
	unsigned int reserved[8];
};

struct lsc_sprd_calc_out {
	unsigned short *dst_gain;
	void *debug_info_ptr;
	unsigned int debug_info_size;
};

struct lsc_post_shading_param {
	unsigned short *org_gain;
	int gain_width;
	int gain_height;
	int gain_pattern;
	int frame_count;
	int bv;
	int bv_gain;
	int flash_mode;
	int pre_flash_mode;
};

struct addr_info {
	unsigned char *addr;
	int size;
};

struct lsc_rg_bg_calc_param{
	unsigned int *stat_r;
	unsigned int *stat_gr;
	unsigned int *stat_gb;
	unsigned int *stat_b;
	unsigned int w;
	unsigned int h;
	unsigned int rg_gradient;
	unsigned int bg_gradient;
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
// API
#ifdef WIN32
void * alsc_init(struct lsc_sprd_init_in *in_param, struct lsc_sprd_init_out *out_param);
int alsc_calculation(void * handle, struct lsc_sprd_calc_in *param, struct lsc_sprd_calc_out *result);
int alsc_ioctrl(void * handle, int cmd, void *in_param, void *out_param);
int alsc_deinit(void * handle);
#endif
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif