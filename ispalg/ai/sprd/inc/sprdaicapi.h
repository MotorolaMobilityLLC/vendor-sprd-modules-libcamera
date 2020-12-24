#ifndef __SPRD_AICAMERA_API_H__
#define __SPRD_AICAMERA_API_H__

#include <stdint.h>
#include "scenedetectapi.h"

#if ((defined(WIN32) || defined (WIN64)) && defined(_USRDLL))
#   if defined(AIC_EXPORTS)
#       define AICAPI_EXPORTS __declspec(dllexport)
#   else
#       define AICAPI_EXPORTS __declspec(dllimport)
#   endif
#elif (defined(__GNUC__) && (__GNUC__ >= 4))
#   define AICAPI_EXPORTS __attribute__((visibility("default")))
#else
#   define AICAPI_EXPORTS
#endif

#ifndef AICAPI
#define AICAPI(rettype) extern AICAPI_EXPORTS rettype
#endif

#define HAVE_ORIENTATION    1

#define MAX_FACE_NUM        (20)
#define MAX_THREAD_NUM      (4)     /* Maximum CPU thread number */
#define MAX_AESTAT_SIZE     (128*128)

/**
\brief version information
*/
typedef struct
{
    uint8_t major;              /*!< API major version */
    uint8_t minor;              /*!< API minor version */
    uint8_t micro;              /*!< API micro version */
    uint8_t nano;               /*!< API nano version */
    char built_date[0x20];      /*!< API built date */
    char built_time[0x20];      /*!< API built time */
    char built_rev[0x100];      /*!< API built version, linked with vcs resivion?> */
} aic_version_t;

/**
\brief Scene classification result
*/
typedef enum
{
    SC_LABEL_NONE,
    SC_LABEL_PEOPLE,
    SC_LABEL_BLUESKY,
    SC_LABEL_SUNRISESET,
    SC_LABEL_SNOW,
    SC_LABEL_NIGHT,
    SC_LABEL_FIREWORK,
    SC_LABEL_GREENPLANT,
    SC_LABEL_FLOWER,
    SC_LABEL_FOOD,
    SC_LABEL_CHINESE_BUILDING,
    SC_LABEL_OTHER_BUILDING,
    SC_LABEL_CATDOG,
    SC_LABEL_TEXT,
    SC_LABEL_BACKLIGHT_POTRAIT,
    SC_LABEL_DARKLIGHT_POTRAIT,
    SC_LABEL_CAR,
    SC_LABEL_BICYCLE,
    SC_LABEL_AUTUMNLEAF,
    SC_LABEL_OVERCAST,
    SC_LABEL_BEACH,
    SC_LABEL_WATERFALL,
    SC_LABEL_LAKE,
    SC_LABEL_MAX
} aic_label_e;

typedef enum
{
    AIC_WORKMODE_FULL = 0,
    AIC_WORKMODE_PORTRAIT = 1,
    AIC_WORKMODE_MAX
}aic_workmode_e;                     /* 0:12 scenes,1:portrait mode,(default: false) */

/**
\brief AI Camera option
*/
typedef struct
{
    int min_frame_interval;           /* Minimal frame interval for submit a new SD task. (default: 6) */
    int max_frame_interval;           /* Force to submit a new SD task if the frame interval is equal to max_frame_interval. (default: 150) */
    int scene_change_margin;          /* frame margin before new scene detection. (In [1, ], default: 3) */
    int scene_smooth_level;           /* Smooth level for output scene label. (In [1, 10], default: 7 */
    int thread_num;                   /* Number of CPU threads used for scene detection. (In [1, 4], default: 1)              */
    bool sync_with_worker;            /* if set as true, main thread will wait until worker thread finished its task. (default: false)*/
    int scene_task_thr[SC_LABEL_MAX]; /* the threshold of the scenes*/
    int camera_id;
} aic_option_t;

/**
\brief The AEM (auto-exposure monitor) information structure
*/
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
    int32_t curr_bv;
    uint32_t flash_enable;
    uint16_t stable;
    bool data_valid;
    uint32_t app_mode;
} aic_aeminfo_t;

/**
\brief The face information structure
*/
typedef struct
{
    int16_t x, y, width, height;        /* Face rectangle                           */
    int16_t yaw_angle;                  /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int16_t roll_angle;                 /* In-plane rotation angle (Roll); In (-180, +180] degrees;   */
    int16_t score;                      /* Confidence score; In [0, 1000]           */
    int16_t human_id;                   /* Human ID Number                          */
} aic_facearea_t;

typedef struct 
{
    uint16_t img_width;
    uint16_t img_height;
    uint32_t frame_id;
    uint64_t timestamp;
    aic_facearea_t face_area[MAX_FACE_NUM];
    uint16_t face_num;  
} aic_faceinfo_t;

/**
\brief Frame status
*/
typedef enum
{
    AIC_IMAGE_DATA_NOT_REQUESTED,
    AIC_IMAGE_DATA_REQUESTED
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

typedef enum
{
    SD_ORNT_0,
    SD_ORNT_90,
    SD_ORNT_180,
    SD_ORNT_270,
    SD_ORNT_MAX,
} SD_ORNT;

/**
\brief Image structure
*/
typedef struct  
{
    SD_IMAGE sd_img;
#if HAVE_ORIENTATION
    SD_ORNT orientation;
#endif
    uint32_t frame_id;
    uint64_t timestamp;
} aic_image_t;

/**
\brief AI scene classification result
*/
typedef struct 
{
    uint32_t frame_id;
    aic_label_e scene_label;                     /* The most possible scene label */
    aic_scene_t task0[CATEGORY_NUM_TASK0];
    aic_scene_t task1[CATEGORY_NUM_TASK1];
    aic_scene_t task2[CATEGORY_NUM_TASK2];
    aic_scene_t task3[CATEGORY_NUM_TASK3];
    aic_scene_t task4[CATEGORY_NUM_TASK4];
    aic_scene_t task5[CATEGORY_NUM_TASK5];
    aic_scene_t task6[CATEGORY_NUM_TASK6];
    aic_scene_t task7[CATEGORY_NUM_TASK7];
    aic_scene_t task8[CATEGORY_NUM_TASK8];
    aic_scene_t task9[CATEGORY_NUM_TASK9];
    aic_scene_t task10[CATEGORY_NUM_TASK10];
    aic_scene_t task11[CATEGORY_NUM_TASK11];
    aic_scene_t task12[CATEGORY_NUM_TASK12];
    aic_scene_t task13[CATEGORY_NUM_TASK13];
    aic_scene_t task14[CATEGORY_NUM_TASK14];
    SD_RESULT sd_result;
} aic_result_t;

/* The error codes */
#define AIC_OK                  (0)     /* Ok!                                      */
#define AIC_ERROR_INTERNAL      (-1)    /* Error: Unknown internal error            */
#define AIC_ERROR_NOMEMORY      (-2)    /* Error: Memory allocation error           */
#define AIC_ERROR_INVALIDARG    (-3)    /* Error: Invalid argument                  */
#define AIC_ERROR_NULLPOINTER   (-4)    /* Error: Invalid null ptr                  */
#define AIC_ERROR_MAGICNUM      (-5)    /* Error: Invalid magic num                 */

/**
\brief The AI camera handle
*/
typedef void* aic_handle_t;

#ifdef  __cplusplus
extern "C" {
#endif

    /* Get the software API version */
    AICAPI(int) AIC_GetVersion(aic_version_t* o_version);

    /* Init the option structure by default values */
    AICAPI(void) AIC_InitOption(aic_option_t* o_option);

    /* Create a AIC handle. */
    AICAPI(int) AIC_CreateHandle(aic_handle_t* handle, const aic_option_t* i_option);

    /* Release the AIC handle */
    AICAPI(void) AIC_DeleteHandle(aic_handle_t* handle);

    /* Set AEM information and stats */
    AICAPI(int) AIC_SetAemInfo(aic_handle_t handle, const aic_aeminfo_t* i_aem, aic_result_t* o_result);

    /* Set face information */
    AICAPI(int) AIC_SetFaceInfo(aic_handle_t handle, const aic_faceinfo_t* i_faces);

    /* Check whether user need feed image data or not */
    AICAPI(int) AIC_CheckFrameStatus(aic_handle_t handle, aic_status_t* o_status);

    /* Set image data */
    AICAPI(int) AIC_SetImageData(aic_handle_t handle, const aic_image_t* i_image);

    /* Get the scene label of the input image */
    AICAPI(int) AIC_GetSceneResult(aic_handle_t handle, aic_result_t* o_result);

    /* Start worker thread */
    AICAPI(void) AIC_StartProcess(aic_handle_t handle, int i_work_mode);

    /* Stop worker thread */
    AICAPI(void) AIC_StopProcess(aic_handle_t handle);

#ifdef  __cplusplus
}
#endif




#endif /* __SPRD_AICAMERA_API_H__ */
