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

typedef struct awb_rgb_gain
{
	unsigned int r_gain;
	unsigned int g_gain;
	unsigned int b_gain;

	int ct;
	int tint;
	int ct_mean;
	int tint_mean;
}awb_rgb_gain_3_0;





typedef struct awb_stat_img
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
}awb_stat_img_3_0;


// Face coordinate info
typedef struct awb_face
{
	unsigned int start_x;
	unsigned int start_y;
	unsigned int end_x;
	unsigned int end_y;
	unsigned int pose; /* face pose: frontal, half-profile, full-profile */
	unsigned int score;
}awb_face_3_0;
//Face info
typedef struct awb_face_info
{
	unsigned int face_num;
	struct awb_face face[20];
	unsigned short img_width;
	unsigned short img_height;
}awb_face_info_3_0;


// AI scene
typedef enum awb_aiscene_type_3_0
{
	AI_SCENE_DEFAULT_v3,
	AI_SCENE_FOOD_v3,
	AI_SCENE_PORTRAIT_v3,
	AI_SCENE_FOLIAGE_v3,
	AI_SCENE_SKY_v3,
	AI_SCENE_NIGHT_v3,
	AI_SCENE_BACKLIGHT_v3,
	AI_SCENE_TEXT_v3,
	AI_SCENE_SUNRISE_v3,
	AI_SCENE_BUILDING_v3,
	AI_SCENE_LANDSCAPE_v3,
	AI_SCENE_SNOW_v3,
	AI_SCENE_FIREWORK_v3,
	AI_SCENE_BEACH_v3,
	AI_SCENE_PET_v3,
	AI_SCENE_FLOWER_v3,
	AI_SCENE_MAX_v3
}awb_aiscene_type_3_0;

typedef enum awb_ai_task_0
{
	AI_SCENE_TASK0_INDOOR_v3,
	AI_SCENE_TASK0_OUTDOOR_v3,
	AI_SCENE_TASK0_MAX_v3
}awb_ai_task_0_3_0;

typedef enum awb_ai_task_1
{
	AI_SCENE_TASK1_NIGHT_v3,
	AI_SCENE_TASK1_BACKLIGHT_v3,
	AI_SCENE_TASK1_SUNRISESET_v3,
	AI_SCENE_TASK1_FIREWORK_v3,
	AI_SCENE_TASK1_OTHERS_v3,
	AI_SCENE_TASK1_MAX_v3
}awb_ai_task_1_3_0;

typedef enum awb_ai_task_2
{
	AI_SCENE_TASK2_FOOD_v3,
	AI_SCENE_TASK2_GREENPLANT_v3,
	AI_SCENE_TASK2_DOCUMENT_v3,
	AI_SCENE_TASK2_CATDOG_v3,
	AI_SCENE_TASK2_FLOWER_v3,
	AI_SCENE_TASK2_BLUESKY_v3,
	AI_SCENE_TASK2_BUILDING_v3,
	AI_SCENE_TASK2_SNOW_v3,
	AI_SCENE_TASK2_OTHERS_v3,
	AI_SCENE_TASK2_MAX_v3
}awb_ai_task_2_3_0;

typedef struct awb_ai_task0_result
{
	awb_ai_task_0_3_0 id;
	unsigned short score;
}awb_ai_task0_result_3_0;

typedef struct awb_ai_task1_result
{
	awb_ai_task_1_3_0 id;
	unsigned short score;
}awb_ai_task1_result_3_0;

typedef struct awb_ai_task2_result
{
	awb_ai_task_2_3_0 id;
	unsigned short score;
}awb_ai_task2_result_3_0;

typedef struct awb_aiscene_info
{
	awb_aiscene_type_3_0 cur_scene_id;
	struct awb_ai_task0_result task0[AI_SCENE_TASK0_MAX];
	struct awb_ai_task1_result task1[AI_SCENE_TASK1_MAX];
	struct awb_ai_task2_result task2[AI_SCENE_TASK2_MAX];
}awb_aiscene_info_3_0;
struct awb_aiscene_info_old {
	cmr_u32 frame_id;
	enum awb_aiscene_type_3_0 cur_scene_id;
	struct ai_task0_result task0[AI_SCENE_TASK0_MAX];
	struct ai_task1_result task1[AI_SCENE_TASK1_MAX];
	struct ai_task2_result task2[AI_SCENE_TASK2_MAX];
};


// XYZ colorsensor
typedef struct awb_colorsensor_info
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
}awb_colorsensor_info_3_0;

typedef struct awb_ct_table
{
	int ct[20];
	float rg[20];
}awb_ct_table_3_0;





typedef struct awb_init_param
{
	unsigned int camera_id;

	unsigned int otp_unit_r;
	unsigned int otp_unit_g;
	unsigned int otp_unit_b;

	void* tool_param;


	// xyz color sensor info
	void* xyz_info;
}awb_init_param_3_0;

typedef struct awb_calc_param
{
	// common info
	unsigned int frame_index;
	unsigned int timestamp;

	// stat info
	struct awb_stat_img stat_img;

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
}awb_calc_param_3_0;

typedef struct awb_calc_result
{
	struct awb_rgb_gain awb_gain;

	unsigned char* log_buffer;  //debug info log buf
	unsigned int log_size;
}awb_calc_result_3_0;


typedef enum
{
	AWB_IOCTRL_GET_MWB_BY_MODEID = 1,
	AWB_IOCTRL_GET_MWB_BY_CT = 2,

	AWB_IOCTRL_GET_CTTABLE20 = 3,

	AWB_IOCTRL_SET_CMC = 4,

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



	AWB_IOCTRL_CMD_MAX,
}AWB3X_CMD;


#ifdef WIN32
#ifdef AWBDLL_EXPORTS
extern __declspec(dllexport) void *awb_init(struct awb_init_param *init_param, struct awb_rgb_gain *gain);
extern __declspec(dllexport) int awb_deinit(void *awb_handle);
extern __declspec(dllexport) int awb_calc(void *awb_handle, struct awb_calc_param *calc_param, struct awb_calc_result *calc_result);
extern __declspec(dllexport) int awb_ioctrl(void *awb_handle, int cmd, void *param1, void *param2);
#else
void *awb_init(struct awb_init_param *init_param, struct awb_rgb_gain *gain);
int awb_deinit(void *awb_handle);
int awb_calc(void *awb_handle, struct awb_calc_param *calc_param, struct awb_calc_result *calc_result);
int awb_ioctrl(void *awb_handle, int cmd, void *param1, void *param2);
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif
