#ifndef AWBLIB_H_
#define AWBLIB_H_


#ifndef WIN32
typedef long long __int64;
#include <linux/types.h>
#include <sys/types.h>
#include <android/log.h>
#else
#include <windows.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct awb_rgb_gain_3_0
{
	unsigned int r_gain;
	unsigned int g_gain;
	unsigned int b_gain;

	int ct;
	int tint;
	int ct_mean;
	int tint_mean;
};





struct awb_stat_img_3_0
{
	//r, g, b channel statistic
	unsigned int* r_stat;
	unsigned int* g_stat;
	unsigned int* b_stat;

	//statistic count of width, height
	unsigned int width_stat;
	unsigned int height_stat;

	//rgb pixel count of each statistic
	unsigned int r_pixel_cnt;
	unsigned int g_pixel_cnt;
	unsigned int b_pixel_cnt;

	unsigned int isp_pixel_bitcount_version; // 10bits or 14bits, 0 - 10bits, 1 - 14bits
};


// Face coordinate info
struct awb_face_3_0
{
	unsigned int start_x;
	unsigned int start_y;
	unsigned int end_x;
	unsigned int end_y;
	unsigned int pose; /* face pose: frontal, half-profile, full-profile */
	unsigned int score;
};
//Face info
struct awb_face_info_3_0
{
	unsigned int face_num;
	struct awb_face_3_0 face[20];
	unsigned short img_width;
	unsigned short img_height;
};


// AI scene
enum awb_aiscene_type_3_0
{
	AI_SCENE_DEFAULT_3_0,
	AI_SCENE_FOOD_3_0,
	AI_SCENE_PORTRAIT_3_0,
	AI_SCENE_FOLIAGE_3_0,
	AI_SCENE_SKY_3_0,
	AI_SCENE_NIGHT_3_0,
	AI_SCENE_BACKLIGHT_3_0,
	AI_SCENE_TEXT_3_0,
	AI_SCENE_SUNRISE_3_0,
	AI_SCENE_BUILDING_3_0,
	AI_SCENE_LANDSCAPE_3_0,
	AI_SCENE_SNOW_3_0,
	AI_SCENE_FIREWORK_3_0,
	AI_SCENE_BEACH_3_0,
	AI_SCENE_PET_3_0,
	AI_SCENE_FLOWER_3_0,
	AI_SCENE_MAX_3_0
};

enum awb_ai_task_0_3_0
{
	AI_SCENE_TASK0_INDOOR_3_0,
	AI_SCENE_TASK0_OUTDOOR_3_0,
	AI_SCENE_TASK0_MAX_3_0
};

enum awb_ai_task_1_3_0
{
	AI_SCENE_TASK1_NIGHT_3_0,
	AI_SCENE_TASK1_BACKLIGHT_3_0,
	AI_SCENE_TASK1_SUNRISESET_3_0,
	AI_SCENE_TASK1_FIREWORK_3_0,
	AI_SCENE_TASK1_OTHERS_3_0,
	AI_SCENE_TASK1_MAX_3_0
};

enum awb_ai_task_2_3_0
{
	AI_SCENE_TASK2_FOOD_3_0,
	AI_SCENE_TASK2_GREENPLANT_3_0,
	AI_SCENE_TASK2_DOCUMENT_3_0,
	AI_SCENE_TASK2_CATDOG_3_0,
	AI_SCENE_TASK2_FLOWER_3_0,
	AI_SCENE_TASK2_BLUESKY_3_0,
	AI_SCENE_TASK2_BUILDING_3_0,
	AI_SCENE_TASK2_SNOW_3_0,
	AI_SCENE_TASK2_OTHERS_3_0,
	AI_SCENE_TASK2_MAX_3_0
};

struct awb_ai_task0_result_3_0
{
	enum awb_ai_task_0_3_0 id;
	unsigned short score;
};

struct awb_ai_task1_result_3_0
{
	enum awb_ai_task_1_3_0 id;
	unsigned short score;
};

struct awb_ai_task2_result_3_0
{
	enum awb_ai_task_2_3_0 id;
	unsigned short score;
};

struct awb_aiscene_info_3_0
{
	enum awb_aiscene_type_3_0 cur_scene_id;
	struct awb_ai_task0_result_3_0 task0[AI_SCENE_TASK0_MAX];
	struct awb_ai_task1_result_3_0 task1[AI_SCENE_TASK1_MAX];
	struct awb_ai_task2_result_3_0 task2[AI_SCENE_TASK2_MAX];
};
struct awb_aiscene_info_old {
	cmr_u32 frame_id;
	enum awb_aiscene_type_3_0 cur_scene_id;
	struct ai_task0_result task0[AI_SCENE_TASK0_MAX];
	struct ai_task1_result task1[AI_SCENE_TASK1_MAX];
	struct ai_task2_result task2[AI_SCENE_TASK2_MAX];
};


// XYZ colorsensor
struct awb_colorsensor_info_3_0
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

struct awb_ct_table_3_0
{
	int ct[20];
	float rg[20];
};





struct awb_init_param_3_0
{
	unsigned int camera_id;

	unsigned int otp_unit_r;
	unsigned int otp_unit_g;
	unsigned int otp_unit_b;

	void* tool_param;


	// xyz color sensor info
	void* xyz_info;
};

struct awb_calc_param_3_0
{
	// common info
	unsigned int frame_index;
	unsigned int timestamp;

	// stat info
	struct awb_stat_img_3_0 stat_img_3_0;

	// AE info
	int bv;
	int iso;

	// simulation info
	int matrix[9];
	unsigned char gamma[256];

	// FACE info
	void* face_info;

	// AIscene info
	void* aiscene_info;

	// xyz color sensor info
	void* colorsensor_info;

	// FLASH info
	void* flash_info;

	// other info
	void* gyro_info;
};

struct awb_calc_result_3_0
{
	struct awb_rgb_gain_3_0 awb_gain;

	unsigned char* log_buffer;  //debug info log buf
	unsigned int log_size;
};


enum
{
	AWB_IOCTRL_GET_MWB_BY_MODEID_3_0 = 1,
	AWB_IOCTRL_GET_MWB_BY_CT_3_0 = 2,

	AWB_IOCTRL_GET_CTTABLE20_3_0 = 3,

	AWB_IOCTRL_SET_CMC_3_0 = 4,

	AWB_IOCTRL_COLOR_CALIBRATION = 5,

// for WIN32 debugtool
	AWB_IOCTRL_GET_AWBLIB_VERSION,

	AWB_IOCTRL_GET_CURRENT_BV,
	AWB_IOCTRL_GET_STAT_WIDTH,
	AWB_IOCTRL_GET_STAT_HEIGHT,
	AWB_IOCTRL_GET_STAT_AEM,
	AWB_IOCTRL_GET_RANDOM_OTP,
	AWB_IOCTRL_GET_GOLDEN_OTP,

	AWB_IOCTRL_GET_ZONE_CTTINT,
	AWB_IOCTRL_GET_BASIC_CTTINT,
	AWB_IOCTRL_GET_FINAL_RESULT,

	AWB_IOCTRL_GET_CURRENT_BOUNDARY,
	AWB_IOCTRL_GET_CT_TINT_BUFFER,

	AWB_IOCTRL_GET_ZONE_BUFFER,
// for WIN32 debugtool



	AWB_IOCTRL_CMD_MAX_3_0,
};


#ifdef WIN32
#ifdef AWBDLL_EXPORTS
extern __declspec(dllexport) void *awb_init(struct awb_init_param_3_0 *init_param, struct awb_rgb_gain_3_0 *gain);
extern __declspec(dllexport) int awb_deinit(void *awb_handle);
extern __declspec(dllexport) int awb_calc(void *awb_handle, struct awb_calc_param_3_0 *calc_param, struct awb_calc_result_3_0 *calc_result);
extern __declspec(dllexport) int awb_ioctrl(void *awb_handle, int cmd, void *in, void *out);
#else
void *awb_init(struct awb_init_param_3_0 *init_param, struct awb_rgb_gain_3_0 *gain);
int awb_deinit(void *awb_handle);
int awb_calc(void *awb_handle, struct awb_calc_param_3_0 *calc_param, struct awb_calc_result_3_0 *calc_result);
int awb_ioctrl(void *awb_handle, int cmd, void *param1, void *param2);
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif
