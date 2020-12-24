/*-------------------------------------------------------------------*/
/*  Copyright(C) 2017 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/* 
    Scene Detection Library API
*/

#ifndef __SPRD_SCENE_DETECTION_H__
#define __SPRD_SCENE_DETECTION_H__

#include <stdint.h>

#if ((defined(WIN32) || defined (WIN64)) && defined(_USRDLL))
#   if defined(SD_EXPORTS)
#       define SDAPI_EXPORTS __declspec(dllexport)
#   else
#       define SDAPI_EXPORTS __declspec(dllimport)
#   endif
#elif (defined(__GNUC__) && (__GNUC__ >= 4))
#   define SDAPI_EXPORTS __attribute__((visibility("default")))
#else
#   define SDAPI_EXPORTS
#endif

#ifndef SDAPI
#define SDAPI(rettype) extern SDAPI_EXPORTS rettype
#endif

/* The error codes */
#define SD_OK                    0  /* Ok!                                      */
#define SD_ERROR_INTERNAL       -1  /* Error: Unknown internal error            */
#define SD_ERROR_NOMEMORY       -2  /* Error: Memory allocation error           */
#define SD_ERROR_INVALIDARG     -3  /* Error: Invalid argument                  */

#define CATEGORY_NUM_TASK0 3        /* Category number of task 0 */
#define CATEGORY_NUM_TASK1 4        /* Category number of task 1 */
#define CATEGORY_NUM_TASK2 2        /* Category number of task 2 */
#define CATEGORY_NUM_TASK3 2
#define CATEGORY_NUM_TASK4 2
#define CATEGORY_NUM_TASK5 3
#define CATEGORY_NUM_TASK6 2
#define CATEGORY_NUM_TASK7 2
#define CATEGORY_NUM_TASK8 3
#define CATEGORY_NUM_TASK9 2
#define CATEGORY_NUM_TASK10 2
#define CATEGORY_NUM_TASK11 2
#define CATEGORY_NUM_TASK12 2
#define CATEGORY_NUM_TASK13 3
#define CATEGORY_NUM_TASK14 2

enum
{
    SCENE_TASK0,
    SCENE_TASK1,
    SCENE_TASK2,
    SCENE_TASK3,
    SCENE_TASK4,
    SCENE_TASK5,
    SCENE_TASK6,
    SCENE_TASK7,
    SCENE_TASK8,
    SCENE_TASK9,
    SCENE_TASK10,
    SCENE_TASK11,
    SCENE_TASK12,
    SCENE_TASK13,
    SCENE_TASK14,
    SCENE_TASK_MAX
};

enum
{
    SCENE_TASK0_BLUESKY,
    SCENE_TASK0_SUNRISESET,
    SCENE_TASK0_OTHERS,
};

enum
{
    SCENE_TASK1_SNOW,
    SCENE_TASK1_NIGHT,
    SCENE_TASK1_FIREWORK,
    SCENE_TASK1_OTHERS,
};

enum
{
    SCENE_TASK2_GREENPLANT,
    SCENE_TASK2_OTHERS,
};

enum
{
    SCENE_TASK3_FLOWER,
    SCENE_TASK3_OTHERS,
};

enum
{
    SCENE_TASK4_FOOD,
    SCENE_TASK4_OTHERS,
};
enum
{
    SCENE_TASK5_CHINESE_BUILDING,
    SCENE_TASK5_OTHER_BUILDING,
    SCENE_TASK5_OTHERS,
};
enum
{
    SCENE_TASK6_CATDOG,
    SCENE_TASK6_OTHERS,
};

enum
{
    SCENE_TASK7_TEXT,
    SCENE_TASK7_OTHERS,
};

enum
{
    SCENE_TASK8_BACKLIGHT_POTRAIT,
    SCENE_TASK8_DARKLIGHT_POTRAIT,
    SCENE_TASK8_OTHERS,
};

enum
{
    SCENE_TASK9_CAR,
    SCENE_TASK9_OTHERS,
};

enum
{
    SCENE_TASK10_BICYCLE,
    SCENE_TASK10_OTHERS,
};

enum
{
    SCENE_TASK11_AUTUMNLEAF,
    SCENE_TASK11_OTHERS,
};

enum
{
    SCENE_TASK12_OVERCAST,
    SCENE_TASK12_OTHERS,
};

enum
{
    SCENE_TASK13_BEACH,
    SCENE_TASK13_WATERFALL,
    SCENE_TASK13_OTHERS,
};

enum
{
    SCENE_TASK14_LAKE,
    SCENE_TASK14_OTHERS,
};

/* The scene detection handle */
typedef void *SD_HANDLE;

# define SD_INPUT_IMAGE_SIZE 228
/* Detection results*/
typedef struct 
{
    int rank0[CATEGORY_NUM_TASK0];      /* bluesky 0, sunrise 1 others 2*/
    int rank1[CATEGORY_NUM_TASK1];     
    int rank2[CATEGORY_NUM_TASK2];     
    int rank3[CATEGORY_NUM_TASK3];     
    int rank4[CATEGORY_NUM_TASK4];     
    int rank5[CATEGORY_NUM_TASK5];    
    int rank6[CATEGORY_NUM_TASK6];     
    int rank7[CATEGORY_NUM_TASK7];     
    int rank8[CATEGORY_NUM_TASK8];      
    int rank9[CATEGORY_NUM_TASK9];     
    int rank10[CATEGORY_NUM_TASK10];    
    int rank11[CATEGORY_NUM_TASK11];
    int rank12[CATEGORY_NUM_TASK12];
    int rank13[CATEGORY_NUM_TASK13];
    int rank14[CATEGORY_NUM_TASK14];

    float score0[CATEGORY_NUM_TASK0];   /* Confidence score of each label, value range [0,1]*/
    float score1[CATEGORY_NUM_TASK1];   /* Confidence score of each label, value range [0,1]*/
    float score2[CATEGORY_NUM_TASK2];   /* Confidence score of each label, value range [0,1]*/
    float score3[CATEGORY_NUM_TASK3];
    float score4[CATEGORY_NUM_TASK4];
    float score5[CATEGORY_NUM_TASK5];
    float score6[CATEGORY_NUM_TASK6];
    float score7[CATEGORY_NUM_TASK7];
    float score8[CATEGORY_NUM_TASK8];
    float score9[CATEGORY_NUM_TASK9];
    float score10[CATEGORY_NUM_TASK10];
    float score11[CATEGORY_NUM_TASK11];
    float score12[CATEGORY_NUM_TASK12];
    float score13[CATEGORY_NUM_TASK13];
    float score14[CATEGORY_NUM_TASK14];
} SD_RESULT;

typedef enum
{
    SD_CSP_NV12,        /* YYYYUVUV, YUV420SP */
    SD_CSP_NV21,        /* YYYYVUVU, YVU420SP */
    SD_CSP_I420,        /* YYYYUUVV, YUV420P */
    SD_CSP_YV12,        /* YYYYVVUU, YVU420P */
    SD_CSP_BGR,         /* BBBBGGGGRRRR, BGR24 */
    SD_CSP_MAX
} SD_CSP;

typedef enum
{
    SD_ROT_0,
    SD_ROT_90,
    SD_ROT_180,
    SD_ROT_270,
    SD_ROT_MAX
} SD_ROT;

typedef struct  
{
    int csp;                /* colorspace, see SD_CSP */
    int width;              /* image width */
    int height;             /* image height */
    int plane;              /* num of image planes */
    int stride[3];          /* bytes per row for each image plane */
    uint8_t* data[3];       /* data pointer for each image plane */
    uint8_t* bufptr;        /* buffer pointer */
    uint32_t bufsize;
    int is_continuous;
} SD_IMAGE;

typedef int (*SD_CALLBACK) (void* arg);

typedef struct
{
    int32_t threadNum;
    SD_CALLBACK breakFuncPtr;
    void* breakFuncArg;
    int32_t cameraId;
} sd_option_t;

#ifdef  __cplusplus
extern "C" {
#endif

SDAPI(const char *) SdGetVersion();

/* Create SD image */
SDAPI(int) SdCreateImage(SD_IMAGE* image, int width, int height, int csp);

/* Realloc SD image */
SDAPI(int) SdReallocImage(SD_IMAGE* image, int width, int height, int csp);

/* Copy SD image */
SDAPI(int) SdCopyImage(const SD_IMAGE* src, SD_IMAGE* dst);

/* Destroy SD image */
SDAPI(void) SdFreeImage(SD_IMAGE* image);

/* Create a SD handle. threadNum must be in [1, 4] */
SDAPI(int) SdCreateHandle(SD_HANDLE* hSD, sd_option_t* i_sd_option);

/* Release the SD handle */
SDAPI(void) SdDeleteHandle(SD_HANDLE* hSD);

/* Detect the scene label of the input image */
SDAPI(int) SdSceneDetect(SD_HANDLE hSD,
                         const SD_IMAGE* inImage,
                         SD_ROT rotation,
                         SD_RESULT* outResult);

#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_SCENE_DETECTION_H__ */
