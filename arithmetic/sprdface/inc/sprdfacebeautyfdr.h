/*-------------------------------------------------------------------*/
/*  Copyright(C) 2017 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/* 
    Face beautify library API
*/
#include <stdio.h>
#include <stdlib.h>

#ifndef __SPRD_FACEBEAUTYFDR_API_H__
#define __SPRD_FACEBEAUTYFDR_API_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FBFDRAPI_EXPORTS)
#define FBFDR_EXPORTS __declspec(dllexport)
#elif (defined(__ANDROID__))
#define FBFDR_EXPORTS __attribute__((visibility("default")))
#else
#define FBFDR_EXPORTS
#endif

#ifndef FBFDRAPI
#define FBFDRAPI(rettype) extern FBFDR_EXPORTS rettype
#endif

typedef enum{
    FBFDR_PIKE2 = 0,
    FBFDR_SHARKLE = 1,
    FBFDR_SHARKL3 = 2,
    FBFDR_SHARKL5PRO = 3,
    FBFDR_SHARKL5 = 4
}fbfdr_chipinfo;

/* The error codes */
#define FBFDR_OK                   0     /* Ok!                                      */
#define FBFDR_ERROR_INTERNAL       -1    /* Error: Unknown internal error            */
#define FBFDR_ERROR_NOMEMORY       -2    /* Error: Memory allocation error           */
#define FBFDR_ERROR_INVALIDARG     -3    /* Error: Invalid argument                  */

/* The work modes */
#define FBFDR_WORKMODE_STILL       0x00  /* Still mode: for capture                  */
#define FBFDR_WORKMODE_MOVIE       0x01  /* Movie mode: for preview and video        */

/* The work camera */
#define FBFDR_CAMERA_FRONT       0x00    /* Front: use front camera                  */
#define FBFDR_CAMERA_REAR        0x01    /* Rear: use rear camera                    */

/* Skin color defintions */
#define FBFDR_SKINCOLOR_WHITE      0     /* White color                              */
#define FBFDR_SKINCOLOR_ROSY       1     /* Rosy color                               */
#define FBFDR_SKINCOLOR_WHEAT      2     /* The healthy wheat color                  */

#define FBFDR_LIPCOLOR_CRIMSON     0     /* Crimson color                            */
#define FBFDR_LIPCOLOR_PINK        1     /* Pink color                               */
#define FBFDR_LIPCOLOR_FUCHSIA     2     /* Fuchsia colr                             */

/* Light portrait type defintions */
#define FBFDR_LPT_LIGHT_SATGE         5     /* Stage lighting                        */
#define FBFDR_LPT_LIGHT_CLASSIC       6     /* Classic lighting                      */
#define FBFDR_LPT_LIGHT_WINDOW        7     /* Window lighting                       */
#define FBFDR_LPT_LIGHT_WAVEDOT       8     /* Wavedot lighting                      */

/* Face beautify options */
typedef struct {
    unsigned char skinSmoothLevel;         /* Smooth skin level. Value range [0, 20]            */
    unsigned char skinTextureHiFreqLevel;  /* Skin Texture high freq level. Value range [0, 10] */
    unsigned char skinTextureLoFreqLevel;  /* Skin Texture low freq level. Value range [0, 10]  */
    unsigned char skinSmoothRadiusCoeff;   /* Smooth skin radius coeff. Value range [1, 255]    */
    unsigned char skinBrightLevel;         /* Skin brightness level. Value range [0, 20]        */
    unsigned char largeEyeLevel;           /* Enlarge eye level. Value range [0, 20]            */
    unsigned char slimFaceLevel;           /* Slim face level. Value range [0, 20]              */
    unsigned char lipColorLevel;           /* Red lips level. Value range [0, 20]               */
    unsigned char skinColorLevel;          /* The level to tune skin color. Value range [0, 20] */

    unsigned char removeBlemishFlag;       /* Flag for removing blemish; 0 --> OFF; 1 --> ON    */
    unsigned char blemishSizeThrCoeff;     /* Blemish diameter coeff. Value range [13, 20]      */
    unsigned char skinColorType;           /* The target skin color: white, rosy, or wheat      */
    unsigned char lipColorType;            /* The target lip color: crimson, pink or fuchsia    */ 

    int cameraWork;                        /* The work camera; front or rear                    */
    int cameraBV;                          /* The value of bv for judjing ambient brightness    */ 
    int cameraISO;                         /* The value of iso for judjing light sensitivity    */
    int cameraCT;                          /* The value of ct for judjing color temperature     */

    unsigned char debugMode;               /* Debug mode. 0 --> OFF; 1 --> ON                   */
    unsigned char fbVersion;               /* facebeauty version control. 0 --> old; 1 --> new  */
} FBFDR_BEAUTY_OPTION;


typedef enum {
    FBFDR_YUV420_FORMAT_CBCR = 0,          /* NV12 format; pixel order:  CbCrCbCr     */
    FBFDR_YUV420_FORMAT_CRCB = 1           /* NV21 format; pixel order:  CrCbCrCb     */
} FBFDR_YUV420_FORMAT;

/* YUV420SP image structure */
typedef struct {
    int width;                       /* Image width                              */
    int height;                      /* Image height                             */
    FBFDR_YUV420_FORMAT format;         /* Image format                             */
    unsigned char *yData;            /* Y data pointer                           */
    unsigned char *uvData;           /* UV data pointer                          */
} FBFDR_IMAGE_YUV420SP;

/* The portrait background segmentation template */
typedef struct
{
    int width;                      /* Image width                               */
    int height;                     /* Image height                              */
    unsigned char *data;            /* Data pointer                              */
} FBFDR_PORTRAITMASK;

/* The face information structure */
typedef struct
{
    int x, y, width, height;        /* Face rectangle                            */
    int yawAngle;                   /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;                  /* In-plane rotation angle (Roll);   In (-180, +180] degrees; */
    int score;                      /* FD score                                  */

    unsigned char faceAttriRace;    /* Skin color of race: yellow, white, black, or indian        */
    unsigned char faceAttriGender;  /* Gender from face attribute detection demo */
    unsigned char faceAttriAge;     /* Age from face attribute detection demo    */
} FBFDR_FACEINFO;


/* The face beauty handle */
typedef void * FBFDR_BEAUTY_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

/* Get the software version */
const char* FBFDR_GetVersion();

/* Initialize the FBFDR_BEAUTY_OPTION structure by default values */
void FBFDR_InitBeautyOption(FBFDR_BEAUTY_OPTION *option);

/*
\brief Create a Face Beauty handle
\param hFB          Pointer to the created Face Beauty handle
\param workMode     Work mode: FBFDR_WORKMODE_STILL or FBFDR_WORKMODE_MOVIE
\param threadNum    Number of thread to use. Value range [1, 4]
\return Returns FBFDR_OK if successful. Returns negative numbers otherwise.
*/
int FBFDR_CreateBeautyHandle(FBFDR_BEAUTY_HANDLE *hFB, int workMode, int threadNum);

/* Release the Face Beauty handle */
void FBFDR_DeleteBeautyHandle(FBFDR_BEAUTY_HANDLE *hFB);

/* 
\brief Do face beautification on the YUV420SP image
\param hFB          The Face Beauty handle
\param imageYUV     Pointer to the YUV420SP image. The image is processed in-place
\param option       The face beautification options
\param faceInfo     The head of the face array. 
\param faceCount    The face count in the face array.
\return Returns FBFDR_OK if successful. Returns negative numbers otherwise.
*/
int FDRFB_FaceBeauty_YUV420SP(FBFDR_BEAUTY_HANDLE hFB, 
                                  FBFDR_IMAGE_YUV420SP *imageYUV,
                                  const FBFDR_BEAUTY_OPTION *option,
                                  const FBFDR_FACEINFO *faceInfo,
                                  int faceCount,
                                  FBFDR_PORTRAITMASK *imageMask);

FBFDRAPI(int) FBFDR_FaceBeauty_YUV420SP(FBFDR_IMAGE_YUV420SP *imageYUV,
								const FBFDR_FACEINFO *faceInfo,
								int faceCount,
								int threadNum);


#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FACEBEAUTYFDR_API_H__ */
