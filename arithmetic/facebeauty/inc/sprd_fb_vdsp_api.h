/*-------------------------------------------------------------------*/
/*  Copyright(C) 2017 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/*
    Face beautify VDSP library API
*/

#ifndef __SPRD_FB_VDSP_API_H__
#define __SPRD_FB_VDSP_API_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FBAPI_EXPORTS)
#define FB_EXPORTS __declspec(dllexport)
#else
#define FB_EXPORTS
#endif

#ifndef FBAPI
#define FBAPI(rettype) extern FB_EXPORTS rettype
#endif

/* The error codes */
#define FB_OK                   0     /* Ok!                                      */
#define FB_ERROR_INTERNAL       -1    /* Error: Unknown internal error            */
#define FB_ERROR_NOMEMORY       -2    /* Error: Memory allocation error           */
#define FB_ERROR_INVALIDARG     -3    /* Error: Invalid argument                  */

/* The work modes */
#define FB_WORKMODE_STILL       0x00  /* Still mode: for capture                  */
#define FB_WORKMODE_MOVIE       0x01  /* Movie mode: for preview and video        */

/* Skin color defintions */
#define FB_SKINCOLOR_WHITE      0     /* White color                              */
#define FB_SKINCOLOR_ROSY       1     /* Rosy color                               */
#define FB_SKINCOLOR_WHEAT      2     /* The healthy wheat color                  */

/* The work camera */
#define FB_CAMERA_FRONT       0x00    /* Front: use front camera                  */
#define FB_CAMERA_REAR        0x01    /* Rear: use rear camera                    */

#define FB_LIPCOLOR_CRIMSON     0     /* Crimson color                            */
#define FB_LIPCOLOR_PINK        1     /* Pink color                               */
#define FB_LIPCOLOR_FUCHSIA     2     /* Fuchsia colr                             */

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
} FB_BEAUTY_OPTION_VDSP;


typedef enum {
    YUV420_FORMAT_CBCR_VDSP = 0,          /* NV12 format; pixel order:  CbCrCbCr     */
    YUV420_FORMAT_CRCB_VDSP = 1           /* NV21 format; pixel order:  CrCbCrCb     */
} FB_YUV420_FORMAT_VDSP;

/* YUV420SP image structure */
typedef struct {
    int width;                       /* Image width                              */
    int height;                      /* Image height                             */
    FB_YUV420_FORMAT_VDSP format;         /* Image format                             */
    unsigned char *yData;            /* Y data pointer                           */
    unsigned char *uvData;           /* UV data pointer                          */
    int imageFd;    /* image data ion fd         */
} FB_IMAGE_YUV420SP_VDSP;

/* The face information structure */
typedef struct
{
    int x, y, width, height;        /* Face rectangle                            */
    int yawAngle;                   /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;                  /* In-plane rotation angle (Roll);   In (-180, +180] degrees; */
    int score;                      /* FD score                                                   */
    unsigned char faceAttriRace;    /* Skin color of race: yellow, white, black, or indian        */
    unsigned char faceAttriGender;  /* Gender from face attribute detection demo */
    unsigned char faceAttriAge;     /* Age from face attribute detection demo    */
} FB_FACEINFO_VDSP;


/* The face beauty handle */
typedef void * FB_BEAUTY_HANDLE_VDSP;

#ifdef __cplusplus
extern "C" {
#endif

/* Get the software version */
FBAPI(const char *) FB_GetVersion_VDSP();

FBAPI(int) FB_FaceBeautyFastStop_VDSP(FB_BEAUTY_HANDLE_VDSP hFB);

FBAPI(int) FB_CreateBeautyHandle_VDSP(FB_BEAUTY_HANDLE_VDSP *hFB, int workMode);
FBAPI(void) FB_DeleteBeautyHandle_VDSP(FB_BEAUTY_HANDLE_VDSP *hFB);
FBAPI(int) FB_FaceBeauty_YUV420SP_VDSP(FB_BEAUTY_HANDLE_VDSP hFB,
                                    FB_IMAGE_YUV420SP_VDSP *imageYUV,
                                    const FB_BEAUTY_OPTION_VDSP *option,
                                    const FB_FACEINFO_VDSP *faceInfo,
                                    int faceCount);
#ifdef __cplusplus
}
#endif

#endif /* __SPRD_FB_VDSP_API_H__ */
