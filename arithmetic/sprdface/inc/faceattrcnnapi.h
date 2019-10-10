/*-------------------------------------------------------------------*/
/*  Copyright(C) 2019 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/*
    Face Attribute (CNN Version) Library API
*/
#ifndef __SPRD_FACE_ATTRIBUTE_CNN_H__
#define __SPRD_FACE_ATTRIBUTE_CNN_H__

#if ((defined(WIN32) || defined (WIN64)) && defined(_USRDLL))
#   if defined(FAR_EXPORTS)
#       define FARAPI_EXPORTS __declspec(dllexport)
#   else
#       define FARAPI_EXPORTS __declspec(dllimport)
#   endif
#elif (defined(__GNUC__) && (__GNUC__ >= 4))
#   define FARAPI_EXPORTS __attribute__((visibility("default")))
#else
#   define FARAPI_EXPORTS
#endif

#ifndef FARAPI
#define FARAPI(rettype) extern FARAPI_EXPORTS rettype
#endif

/* The error codes */
#define FAR_OK                    0  /* Ok!                                      */
#define FAR_ERROR_INTERNAL       -1  /* Error: Unknown internal error            */
#define FAR_ERROR_NOMEMORY       -2  /* Error: Memory allocation error           */
#define FAR_ERROR_INVALIDARG     -3  /* Error: Invalid argument                  */

#define FAR_WORKMODE_STILL       0x00  /* Still mode: using one frame information*/
#define FAR_WORKMODE_MOVIE       0x01  /* Movie mode: using mutil-frame information */
#define FAR_WORKMODE_DEFAULT     FAR_WORKMODE_STILL /* the default work mode        */

#define RATIO_AGE                 5     /* ratio of age                              */
#define RATIO_GENDER           1     /* ratio of gender                           */
#define RATIO_INFANT            5     /* ratio of infant                           */
#define RATIO_RACE               1     /* ratio of race                             */
//#define FAR_POINT_NUM             7

// A YUV 4:2:0 image with a plane of 8bit Y samples followed by an
// interleaved U/V planes.
typedef struct
{
    unsigned char *yData;       /* Y data pointer                    */
    unsigned char *uvData;      /* UV data pointer                   */
    int width;                  /* Image width                       */
    int height;                 /* Image height                      */
    unsigned char format;       /* Image format. 0->NV12; 1->NV21    */
                                /* NV12 format; pixel order:  CbCrCbCr     */
                                /* NV21 format; pixel order:  CrCbCrCb     */
} FAR_IMAGE_YUV420SP;

// A YUV 4:2:0 image with a plane of 8bit Y samples followed by
// seperate U and V planes with arbitrary row and column strides.
typedef struct
{
    unsigned char *yData;       /* Y data pointer                    */
    unsigned char *uData;       /* U (Cb) data pointer               */
    unsigned char *vData;       /* V (Cr) data pointer               */
    int width;                  /* Image width                       */
    int height;                 /* Image height                      */
    int yRowStride;             /* bytes per row for Y channel       */
    int uvRowStride;            /* bytes per row for U(V) channel    */
    int uvPixelStride;          /* U/V pixel stride                  */
} FAR_IMAGE_YUV420;

/* The input touch point*/
typedef struct
{
    int x, y;
}FAR_TOUCH_POINT;

/* The face information structure*/
typedef struct
{
    int x, y, width, height;            /* Face Rect*/
    int yawAngle;                       /* Face yaw angle*/
    int rollAngle;                      /* Face roll angle*/
    int faceIdx;                        /* Face Index*/
}FAR_FACEINFO_V2;

/* The face information vector structure*/
typedef struct
{
    FAR_FACEINFO_V2 *faceInfo;
    int faceNum;
}FAR_FACEINFO_VEC;

/* The face attribute structure*/
typedef struct
{
    int smile;                          /* Smile degree: smile(>0); not smile(<0); unknown(0)   */
    int eyeClose;                       /* Eye open degree: open(<0); close(>0); unknown(0)     */
    int infant;                         /* Infant degree: infant(>0); not infant(<0); unknown(0)*/
    int genderPre;                      /* Gender: Male(>0); Female(<0); unknown(0)             */
    int agePre;                         /* age: [0,80]*/
    int genderCnn;                      /* [-100, 100], male: > 0, female < 0*/
    int race;                           /* race: Yellow(0); White(1); Black(2); India(3)        */
    int raceScore[4];                      /* raceScore: [0, 100]*/
    int faceIdx;                        /* Face Index*/
}FAR_ATTRIBUTE_V2;

/* The face attribute vector structure*/
typedef struct
{
    FAR_ATTRIBUTE_V2 *faceAtt;
    int faceNum;
}FAR_ATTRIBUTE_VEC;

/* The option for face attribute detector*/
typedef struct
{
    unsigned char workMode;         /* Work mode: FAR_WORKMODE_STILL or FAR_WORKMODE_MOVIE. default: FAR_WORKMODE_STILL*/
    unsigned char maxFaceNum;       /* Maximum face number to detect attribute per frame. range: [1, 10], default: 5*/
    unsigned char trackInterval;    /* The frame intervals to detect face attribute in movie mode, range[0, 20], default: 5*/

    unsigned char smileOn;          /* run smile degree estimation: 1-->ON; 0-->OFF, default: ON*/
    unsigned char genderOn;         /* run gender detection: 1-->ON; 0-->OFF, default: ON*/
    unsigned char genderCnnOn;      /* run gender detection: 1-->ON; 0-->OFF, default: ON*/
    unsigned char ageOn;            /* run age detection: 1-->ON; 0-->OFF, default: ON*/
    unsigned char eyeOn;            /* run eye close/open recognition: 1-->ON; 0-->OFF, default: OFF*/
    unsigned char infantOn;         /* run infant detection: 1-->ON; 0-->OFF, default: OFF*/
    unsigned char raceOn;           /* run race detection: 1-->ON; 0-->OFF, default: ON*/
}FAR_OPTION_V2;

/* The face attribute detector handle */
typedef void * FAR_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

FARAPI(const char *) FarGetVersion_V2();

/* Init the option to default value*/
FARAPI(void) FAR_InitOption(FAR_OPTION_V2 *io_opt);

/* Create a FAR handle. threadNum must be in [1, 4] */
FARAPI(int) FarCreateRecognizerHandle_V2(FAR_HANDLE *hFAR, int threadNum, FAR_OPTION_V2 *i_opt);

/* Release the FAR handle */
FARAPI(void) FarDeleteRecognizerHandle_V2(FAR_HANDLE *hFAR);

/* Detect Face Attribute in the YUV420SP image */
FARAPI(int) FarRun_YUV420SP(FAR_HANDLE hFAR,
                            const FAR_IMAGE_YUV420SP *i_yuvImage,
                            const FAR_FACEINFO_VEC *i_faceInfoVec,
                            FAR_ATTRIBUTE_VEC *o_attributeVec);

/* Detect Face Attribute in the YUV420 image */
FARAPI(int) FarRun_YUV420(FAR_HANDLE hFAR,
                          const FAR_IMAGE_YUV420 *i_yuvImage,
                          const FAR_FACEINFO_VEC *i_faceInfoVec,
                          FAR_ATTRIBUTE_VEC *o_attributeVec);

/* When there is touch point input, select face */
FARAPI(int) FarSelectFace(FAR_HANDLE hFAR,
                          const FAR_TOUCH_POINT *i_touchPoint,
                          const FAR_FACEINFO_VEC *i_faceInfoVec);

/* When there is no face input, reset FaceArray */
FARAPI(int) FarResetFaceArray(FAR_HANDLE hFAR);

#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FACE_ATTRIBUTE_CNN_H__ */
