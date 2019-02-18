#ifndef AWB_H_
#define AWB_H_

#ifdef WIN32
#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <android/log.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

	struct awb_stat_img {
		cmr_u32 *r;
		cmr_u32 *g;
		cmr_u32 *b;
	};

	struct awb_rgb_gain {
		cmr_u32 r_gain;
		cmr_u32 g_gain;
		cmr_u32 b_gain;

		cmr_u32 ct;
		cmr_s32 pg;				//0 neutral, -: green, +: purple
		cmr_s32 green100;
	};

	enum {
		WB_AUTO,
		WB_CLOUDY_DAYLIGHT,
		WB_DAYLIGHT,
		WB_FLUORESCENT,
		WB_A,
		WB_SHADE,
		WB_TWILIGHT,
		WB_WARM_FLUORESCENT,
		WB_CANDLE,
	};

	struct awbParaGenIn {
		cmr_s32 version;

		// AWB OTP
		cmr_u32 otp_golden_r;
		cmr_u32 otp_golden_g;
		cmr_u32 otp_golden_b;
		cmr_u32 otp_random_r;
		cmr_u32 otp_random_g;
		cmr_u32 otp_random_b;

		// graychart under neutral light
		cmr_s32 grayNum;		/* [3, 10] */
		cmr_s32 grayCt[10];
		double grayRgb[10][3];

		// graychart under CWF
		double cwfRgb[3];

		// colorchecker under D65
		double colorChecker24[24][3];

		// weight table
		cmr_s32 wTabSz;
		double wTabEv[4];
		double wTabData[4][6];	// start, mid, mid2, end, peak, dc

		// preference
		double ctShiftBv[4];

		double null_data1[4][2];

		double null_data2[4];
		double null_data3[4];

		cmr_s32 ctShiftTickNum[4];
		float ctShiftTick[4][64];
		float ctShiftValTick[4][64];
		float pgrShiftTick[4][64];

		//purple
		cmr_s16 purpleBvNum;
		float purpleBv[5];
		cmr_s16 purpleNodeNum[5];
		float purpleUpPgratioOri[5];
		float purpleDnPgratioOri[5];
		cmr_s16 purpleCt[5][10];
		cmr_s16 purpleUpCt[5][10];
		float purpleUpPgRatio[5][10];
		cmr_s16 purpleDnCt[5][10];
		float purpleDnPgRatio[5][10];

		// Green
		double grassYellowLevel;
		double grassGreenLevel;

		cmr_s16 pgBalanceBvNum;
		float pgBalanceBv[5];
		cmr_s16 pgBalanceNodeNum[5];
		cmr_s16 pgBalanceCt[5][10];
		float artificialGreenRatio[5][10];
		float purpleBalanceRatio[5][10];
		float purpleBalanceTransRatio[5][10];

		//boundary
		cmr_s16 limNum;
		cmr_s16 limMode;
		float limBv[4];
		float limCtRt[4];
		float limCtLt[4];
		float limPgRatioUp[4];
		float limPgRatioDn[4];

		float limCtRtTrans[4];
		float limCtLtTrans[4];
		float limPgUpTrans[4];
		float limPgDnTrans[4];

		float limPgRatioUp2[4];
		float limPgRatioUp3[4];
		float limPgRatioDn2[4];
		float limPgRatioDn3[4];
		float limCtMid[4];
		float limDefaultCt[4];
		float limDefaultPgRatio[4];

		//awbMode
		cmr_s32 modeNum;
		cmr_s32 modeId[10];
		double modeCt[10];
		double modePgr[10];

		cmr_s32 ctShiftNum;
		cmr_s16 ctShiftCt[10];
		float ctShiftPgRatio[10];
		cmr_s16 ctShiftNeutral[10];
		cmr_s16 ctShiftDown[10];
		cmr_s32 bFrontCameraFaceOpt;

		//boundary2
		cmr_s16 lm2En;
		cmr_s16 lm2Mode;
		cmr_s16 lm2BvNum;
		float lm2Bv[5];
		cmr_s16 lm2NodeNum[5];
		cmr_s16 lm2Ct[5][10];

		float lm2PgrUp[5][10];
		float lm2PgrDn[5][10];
		float lm2PgrUpTrans[5][10];
		float lm2PgrDnTrans[5][10];

		cmr_s16 lm2CtRt[5];
		cmr_s16 lm2CtLt[5];
		cmr_s16 lm2CtRtTrans[5];
		cmr_s16 lm2CtLtTrans[5];

		cmr_s16 lm2DefaultCt[5];
		float lm2DefaultPgr[5];


		cmr_s32 nodeWTabSz;
		cmr_u8 nodeWTabSzSub[6];
		cmr_s16 nodeWTabBv[6];
		cmr_s16 nodeWTabCt[6][16];
		cmr_s16 nodeWTabData[6][16];

		cmr_s32 defExSz;
		cmr_s16 defExBv[8];
		cmr_s16 defExCt[8];
		float defExPgr[8];
	};

	struct awb_tuning_param {
		cmr_s32 magic;
		cmr_s32 version;
		cmr_s32 date;
		cmr_s32 time;

		/* MWB table */
		cmr_u8 wbModeNum;
		cmr_u8 wbModeId[10];
		struct awb_rgb_gain wbMode_gain[10];
		struct awb_rgb_gain mwb_gain[101];	// mwb_gain[0] is init_gain

		/* AWB parameter */
		cmr_s32 rgb_12bit_sat_value;

		cmr_s32 pg_x_ratio;
		cmr_s32 pg_y_ratio;
		cmr_s32 pg_shift;

		cmr_s32 isotherm_ratio;
		cmr_s32 isotherm_shift;

		cmr_s32 rmeanCali;
		cmr_s32 gmeanCali;
		cmr_s32 bmeanCali;

		cmr_s32 dct_scale_65536;	//65536=1x

		cmr_s16 pgBalanceBvNum;
		cmr_s16 pgBalanceBv[5];	//1024
		cmr_s16 pg_center[512];
		cmr_s16 pg_up[5][512];
		cmr_s16 pg_up_trans[5][512];
		cmr_s16 pg_dn[5][512];

		cmr_s16 cct_tab[512];
		cmr_s16 cct_tab_div;
		cmr_s16 cct_tab_sz;
		cmr_s32 cct_tab_a[40];
		cmr_s32 cct_tab_b[40];

		cmr_s32 dct_start;
		cmr_s32 pg_start;

		cmr_s32 dct_div1;
		cmr_s32 dct_div2;

		cmr_s32 w_lv_len;
		cmr_s16 w_lv[6];
		cmr_s16 w_lv_tab[6][512];

		cmr_s32 dct_sh_bv[4];
		cmr_s16 dct_sh[4][512];
		cmr_s16 dct_pg_sh100[4][512];
		cmr_s16 null_data1[4][2];
		cmr_s16 null_data2[4];
		cmr_s16 null_data3[4];
		cmr_s16 null_data4[4];
		cmr_s16 null_data5[4];

		cmr_s32 red_num;
		cmr_s16 red_x[10];
		cmr_s16 red_y[10];

		cmr_s32 blue_num;
		cmr_s16 blue_x[10];
		cmr_s16 blue_comp[10];
		cmr_s16 blue_sh[10];

		cmr_s16 ctCompX1;
		cmr_s16 ctCompX2;
		cmr_s16 ctCompY1;
		cmr_s16 ctCompY2;

		cmr_s16 defnum;
		cmr_s16 defev1024[4];
		cmr_s16 defx[9][4];
		cmr_s16 defxDev[4][4];
		cmr_s16 defrange[4][4];
		cmr_s16 defxy[2][4];

		cmr_s16 grassGreenLevel;	//1024;

		//purple
		cmr_s16 purpleBvNum;
		cmr_s16 purpleBv[5];
		cmr_s16 purpleNodeNum[5];
		cmr_s16 purpleUpPgOri[5];
		cmr_s16 purpleDnPgOri[5];
		cmr_s16 purpleCt[5][10];
		cmr_s16 purpleUpCt[5][10];
		cmr_s16 purpleUpPg[5][10];
		cmr_s16 purpleDnCt[5][10];
		cmr_s16 purpleDnPg[5][10];

		cmr_s32 cwfPgAbs100;

		cmr_u8 ui_data[sizeof(struct awbParaGenIn)];

		cmr_s32 reserved[(8200 - sizeof(struct awbParaGenIn)) / 4];
		cmr_s32 stat_type;		// 0 - binning4awb, 1 - aem, default is 0

		// awb control param
		cmr_u32 skip_frame_num;
		cmr_u32 calc_interval_num;
		cmr_u32 smooth_buffer_num;

		cmr_s32 check;
	};

	struct awb_init_param {
		cmr_u32 stat_w;			// will be deprecated
		cmr_u32 stat_h;			// will be deprecated

		cmr_u32 otp_random_r;
		cmr_u32 otp_random_g;
		cmr_u32 otp_random_b;
		cmr_u32 otp_golden_r;
		cmr_u32 otp_golden_g;
		cmr_u32 otp_golden_b;

		struct awb_tuning_param tuning_param;
	};

	struct awb_calc_param {
		struct awb_stat_img stat_img;

		cmr_u32 stat_img_w;
		cmr_u32 stat_img_h;
		cmr_u32 r_pix_cnt;
		cmr_u32 g_pix_cnt;
		cmr_u32 b_pix_cnt;

		cmr_s32 bv;
		cmr_s32 iso;
		cmr_s32 date;			//ex: 20160331
		cmr_s32 time;			//ex: 221059

		// just for simulation
		cmr_s32 matrix[9];
		cmr_u8 gamma[256];


		// AI info
		void* ai_info;

		// XYZ info
		void* xyz_info;
	};

	struct awb_calc_result {
		struct awb_rgb_gain awb_gain[8];

		int r_offset;
		int g_offset;
		int b_offset;

		int reserved[9];

		cmr_u8 *log_buffer;
		cmr_u32 log_size;
	};


#ifdef WIN32
	// for AI
	enum ai_scene_type {
		AI_SCENE_DEFAULT,
		AI_SCENE_FOOD,
		AI_SCENE_PORTRAIT,
		AI_SCENE_FOLIAGE,
		AI_SCENE_SKY,
		AI_SCENE_NIGHT,
		AI_SCENE_BACKLIGHT,
		AI_SCENE_TEXT,
		AI_SCENE_SUNRISE,
		AI_SCENE_BUILDING,
		AI_SCENE_LANDSCAPE,
		AI_SCENE_SNOW,
		AI_SCENE_FIREWORK,
		AI_SCENE_BEACH,
		AI_SCENE_PET,
		AI_SCENE_FLOWER,
		AI_SCENE_MAX
	};

	enum ai_task_0 {
		AI_SCENE_TASK0_INDOOR,
		AI_SCENE_TASK0_OUTDOOR,
		AI_SCENE_TASK0_MAX
	};

	enum ai_task_1 {
		AI_SCENE_TASK1_NIGHT,
		AI_SCENE_TASK1_BACKLIGHT,
		AI_SCENE_TASK1_SUNRISESET,
		AI_SCENE_TASK1_FIREWORK,
		AI_SCENE_TASK1_OTHERS,
		AI_SCENE_TASK1_MAX
	};

	enum ai_task_2 {
		AI_SCENE_TASK2_FOOD,
		AI_SCENE_TASK2_GREENPLANT,
		AI_SCENE_TASK2_DOCUMENT,
		AI_SCENE_TASK2_CATDOG,
		AI_SCENE_TASK2_FLOWER,
		AI_SCENE_TASK2_BLUESKY,
		AI_SCENE_TASK2_BUILDING,
		AI_SCENE_TASK2_SNOW,
		AI_SCENE_TASK2_OTHERS,
		AI_SCENE_TASK2_MAX
	};

	struct ai_task0_result {
		enum ai_task_0 id;
		unsigned short score;
	};

	struct ai_task1_result {
		enum ai_task_1 id;
		unsigned short score;
	};

	struct ai_task2_result {
		enum ai_task_2 id;
		unsigned short score;
	};

	struct ai_scene_detect_info {
		unsigned int frame_id;
		enum ai_scene_type cur_scene_id;
		struct ai_task0_result task0[AI_SCENE_TASK0_MAX];
		struct ai_task1_result task1[AI_SCENE_TASK1_MAX];
		struct ai_task2_result task2[AI_SCENE_TASK2_MAX];
	};
#endif

	// for XYZ color sensor
	struct xyz_color_info
	{
		unsigned int x_data;
		unsigned int y_data;
		unsigned int z_data;
		unsigned int ir_data;

		unsigned int x_raw;
		unsigned int y_raw;
		unsigned int z_raw;
		unsigned int ir_raw;

		unsigned int again;
		unsigned int atime;
		unsigned int lux;
		unsigned int cct;
	};


	// for flash
	enum {
		AWB_IOCTRL_GET_CTTABLE20 = 2,
		AWB_IOCTRL_CMD_MAX,
	};

	struct awb_ct_table {
		float ct[20];
		float rg[20];
	};





	// for dualcam sync
	struct awb_stat_sync {
		cmr_u32 *r_info;
		cmr_u32 *g_info;
		cmr_u32 *b_info;
		cmr_u32 width;
		cmr_u32 height;
	};

	struct awb_ctrl_rgb_otp {
		cmr_u32 r;
		cmr_u32 g;
		cmr_u32 b;
	};

	struct awb_sync_info {
		struct awb_stat_sync stat_master_info;
		struct awb_stat_sync stat_slave_info;
		float master_fov;
		float slave_fov;
		cmr_u32 master_pix_cnt;
		cmr_u32 slave_pix_cnt;
		struct awb_ctrl_rgb_otp master_gldn_stat_info;
		struct awb_ctrl_rgb_otp master_rdm_stat_info;
		struct awb_ctrl_rgb_otp slave_gldn_stat_info;
		struct awb_ctrl_rgb_otp slave_rdm_stat_info;
	};

/*------------------------------------------------------------------------------*
*				Function Prototype					*
*-------------------------------------------------------------------------------*/
#ifdef WIN32
	void *awb_init_v1(struct awb_init_param *init_param, struct awb_rgb_gain *gain);
	cmr_s32 awb_calc_v1(void *awb_handle, struct awb_calc_param *calc_param, struct awb_calc_result *calc_result);
	cmr_s32 awb_ioctrl_v1(void *awb_handle, cmr_s32 cmd, void *param);
	cmr_s32 awb_deinit_v1(void *awb_handle);
	cmr_s32 awb_sync_gain(struct awb_sync_info *sync_info, cmr_u32 gain_r_master, cmr_u32 gain_g_master, cmr_u32 gain_b_master, cmr_u32 * gain_r_slave, cmr_u32 * gain_g_slave, cmr_u32 * gain_b_slave);
#endif

#ifdef __cplusplus
}
#endif
#endif
