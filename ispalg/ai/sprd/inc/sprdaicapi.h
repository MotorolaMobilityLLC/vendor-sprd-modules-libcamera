#ifndef _AICAMERA_API_H_
#define _AICAMERA_API_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_FACE_NUM    (20)
#define MAX_THREAD_NUM  (4)
#define MAX_AESTAT_SIZE (128*128)

#define CATEGORY_NUM_TASK0 2
#define CATEGORY_NUM_TASK1 5
#define CATEGORY_NUM_TASK2 9

typedef struct
{
    int rank0[CATEGORY_NUM_TASK0];
    int rank1[CATEGORY_NUM_TASK1];
    int rank2[CATEGORY_NUM_TASK2];
    float score0[CATEGORY_NUM_TASK0];
    float score1[CATEGORY_NUM_TASK1];
    float score2[CATEGORY_NUM_TASK2];
} SD_RESULT;

enum
{
    SD_CSP_NV12,
    SD_CSP_NV21,
    SD_CSP_I420,
    SD_CSP_YV12,
    SD_CSP_BGR,
    SD_CSP_MAX
};

typedef enum
{
    SD_ORNT_0,
    SD_ORNT_90,
    SD_ORNT_180,
    SD_ORNT_270,
    SD_ORNT_MAX
} SD_ORNT;

typedef struct
{
    int csp;
    int width;
    int height;
    int plane;
    int stride[3];
    uint8_t* data[3];
    uint8_t* bufptr;
    uint32_t bufsize;
    int is_continuous;
} SD_IMAGE;

typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t micro;
    uint8_t nano;
    char built_date[0x20];
    char built_time[0x20];
    char built_rev[0x100];
} aic_version_t;

typedef enum
{
    SC_LABEL_NONE,
    SC_LABEL_PEOPLE,
    SC_LABEL_NIGHT,
    SC_LABEL_BACKLIGHT,
    SC_LABEL_SUNRISESET,
    SC_LABEL_FIREWORK,
    SC_LABEL_FOOD,
    SC_LABEL_GREENPLANT,
    SC_LABEL_DOCUMENT,
    SC_LABEL_CATDOG,
    SC_LABEL_FLOWER,
    SC_LABEL_BLUESKY,
    SC_LABEL_BUILDING,
    SC_LABEL_SNOW,
    SC_LABEL_MAX
} aic_label_e;

typedef enum
{
    AIC_WORKMODE_FULL = 0,
    AIC_WORKMODE_PORTRAIT = 1,
    AIC_WORKMODE_MAX
} aic_workmode_e;

typedef struct
{
    int min_frame_interval;
    int max_frame_interval;
    int scene_change_margin;
    int scene_smooth_level;
    int thread_num;
    bool sync_with_worker;
    int scene_task_thr[SC_LABEL_MAX];
	int camera_id;
} aic_option_t;

typedef struct
{
	uint32_t* r_stat;
	uint32_t* g_stat;
	uint32_t* b_stat;
} aic_aestat_t;

typedef struct
{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t width;
	uint16_t height;
} aic_aerect_t;

typedef struct
{
	uint32_t frame_id;
	uint64_t timestamp;
	aic_aestat_t ae_stat;
	aic_aerect_t ae_rect;
	uint16_t blk_width;
	uint16_t blk_height;
	uint16_t blk_num_hor;
	uint16_t blk_num_ver;
	uint16_t zoom_ratio;
	int curr_bv;
	uint32_t flash_enable;
	uint16_t stable;
	bool data_valid;
	uint32_t app_mode;
} aic_aeminfo_t;

typedef struct
{
	int16_t x, y, width, height;
	int16_t yaw_angle;
	int16_t roll_angle;
	int16_t score;
	int16_t human_id;
} aic_facearea_t;

typedef struct
{
	uint16_t width;
	uint16_t height;
	uint32_t frame_id;
	uint64_t timestamp;
	aic_facearea_t face_area[MAX_FACE_NUM];
	uint16_t face_num;
} aic_faceinfo_t;

typedef enum
{
    AIC_IMAGE_DATA_NOT_REQUIRED,
    AIC_IMAGE_DATA_REQUIRED
} aic_imgflag_e;

typedef struct
{
    uint32_t frame_id;
    int32_t frame_state;
    aic_imgflag_e img_flag;    
} aic_status_t;

typedef struct
{
    uint16_t id;
    uint16_t score;
} aic_scene_t;

typedef struct
{
	uint16_t scene_indoor;
	uint16_t scene_outdoor;
} aic_task0_t;

typedef struct
{
	uint16_t scene_night;
	uint16_t scene_backlight;
	uint16_t scene_sunriseset;
	uint16_t scene_firework;
	uint16_t scene_others;
} aic_task1_t;
			
typedef struct
{
	uint16_t scene_food;
	uint16_t scene_greenplant;
	uint16_t scene_document;
	uint16_t scene_catdog;
	uint16_t scene_flower;
	uint16_t scene_bluesky;
	uint16_t scene_building;
	uint16_t scene_snow;
	uint16_t scene_others;
} aic_task2_t;

enum
{
    SCENE_TASK0_INDOOR,
    SCENE_TASK0_OUTDOOR
};

enum
{
    SCENE_TASK1_NIGHT,
    SCENE_TASK1_BACKLIGHT,
    SCENE_TASK1_SUNRISESET,
    SCENE_TASK1_FIREWORK,
    SCENE_TASK1_OTHERS 
};

enum
{
    SCENE_TASK2_FOOD,
    SCENE_TASK2_GREENPLANT,
    SCENE_TASK2_DOCUMENT,
    SCENE_TASK2_CATDOG,
    SCENE_TASK2_FLOWER,
    SCENE_TASK2_BLUESKY,
    SCENE_TASK2_BUILDING,
    SCENE_TASK2_SNOW,
    SCENE_TASK2_OTHERS
};

typedef struct
{
    SD_IMAGE sd_img;
    SD_ORNT orientation;
    uint32_t frame_id;
    uint64_t timestamp;
} aic_image_t;

typedef struct
{
    uint32_t frame_id;
    aic_label_e scene_label;
    aic_scene_t task0[2];
    aic_scene_t task1[5];
    aic_scene_t task2[9];
    SD_RESULT sd_result;
} aic_result_t;

#define AIC_OK                  (0)
#define AIC_ERROR_INTERNAL      (-1)
#define AIC_ERROR_NOMEMORY      (-2)
#define AIC_ERROR_INVALIDARG    (-3)
#define AIC_ERROR_NULLPOINTER   (-4)
#define AIC_ERROR_MAGICNUM      (-5)

typedef void* aic_handle_t;

#define AICAPI(rettype) extern rettype

#ifdef __cplusplus
extern "C" {
#endif

    AICAPI(int) AIC_GetVersion(aic_version_t* o_version);
    AICAPI(void) AIC_InitOption(aic_option_t* o_option);
    AICAPI(int) AIC_CreateHandle(aic_handle_t* handle, const aic_option_t* i_option);
    AICAPI(void) AIC_DeleteHandle(aic_handle_t* handle);
    AICAPI(int) AIC_SetAemInfo(aic_handle_t handle, const aic_aeminfo_t* i_aem, aic_result_t* o_result);
    AICAPI(int) AIC_SetFaceInfo(aic_handle_t handle, const aic_faceinfo_t* i_faces);
    AICAPI(int) AIC_CheckFrameStatus(aic_handle_t handle, aic_status_t* o_status);
    AICAPI(int) AIC_SetImageData(aic_handle_t handle, const aic_image_t* i_image);
    AICAPI(int) AIC_GetSceneResult(aic_handle_t handle, aic_result_t* o_result);  
    AICAPI(void) AIC_StartProcess(aic_handle_t handle, int i_work_mode);
    AICAPI(void) AIC_StopProcess(aic_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
