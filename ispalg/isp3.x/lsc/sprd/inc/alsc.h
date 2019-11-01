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
	LSC_CMD_MAX
};

struct lsc_otp_convert_param{
	int gain_width;
	int gain_height;
	int grid;
	unsigned short *lsc_table[9];
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

	unsigned int lsc_otp_oc_en;
	unsigned int lsc_otp_oc_r_x;
	unsigned int lsc_otp_oc_r_y;
	unsigned int lsc_otp_oc_gr_x;
	unsigned int lsc_otp_oc_gr_y;
	unsigned int lsc_otp_oc_gb_x;
	unsigned int lsc_otp_oc_gb_y;
	unsigned int lsc_otp_oc_b_x;
	unsigned int lsc_otp_oc_b_y;
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
	int LSC_SPD_VERSION;
	void *tuning_param;
};

struct addr_info {
	unsigned char *addr;
	int size;
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
// API
void * alsc_init(struct lsc_sprd_init_in *param);
int alsc_calculation(void * handle, struct lsc_sprd_calc_in *param, struct lsc_sprd_calc_out *result);
int alsc_ioctrl(void * handle, int cmd, void *in_param, void *out_param);
int alsc_deinit(void * handle);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif