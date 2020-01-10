/*-------------------------------------------------------------------*/
/*  Copyright(C) 2019 by Unisoc                                      */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/* 
    Light portrait library API
*/

#ifndef __UNISOC_LIGHTPORTRAIT_API_H__
#define __UNISOC_LIGHTPORTRAIT_API_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined LPTAPI_EXPORTS)
#define LPT_EXPORTS __declspec(dllexport)
#elif defined(__linux__)
#define LPT_EXPORTS __attribute__ ((visibility ("default")))
#else
#define LPT_EXPORTS
#endif

#ifndef LPTAPI
#define LPTAPI(rettype) extern LPT_EXPORTS rettype
#endif

/* The error codes */
#define LPT_OK                   0     /* Ok!                                  */
#define LPT_ERROR_INTERNAL       -1    /* Error: Unknown internal error        */
#define LPT_ERROR_NOMEMORY       -2    /* Error: Memory allocation error       */
#define LPT_ERROR_INVALIDARG     -3    /* Error: Invalid argument              */

/* The face information does not meet the LPT conditions */
#define LPT_NOFACE             1      /* There is no face detected             */
#define LPT_FACEPSOE_TOOBIG    2      /* The face pose is too big              */
#define LPT_FACE_TOOFAR        3      /* The face is too far from the camera   */
#define LPT_FACE_TOOCLOSE      4      /* The face is too close from the camera */
#define LPT_FACE_NOTONLY       5      /* The face number is not only           */

/* The work modes */
#define LPT_WORKMODE_STILL     0x00   /* Still mode: for capture               */
#define LPT_WORKMODE_MOVIE     0x01   /* Movie mode: for preview and video     */

/* The work camera */
#define LPT_CAMERA_FRONT       0x00   /* Front: use front camera               */
#define LPT_CAMERA_REAR        0x01   /* Rear: use rear camera                 */

/* The light split direction */
#define LPT_LIGHTSPLIT_RIGHT       0    /* Light split is on face right     */
#define LPT_LIGHTSPLIT_LEFT        1    /* Light split is on face left      */

/* Light portrait type defintions */
#define LPT_LIGHT_NO            0     /* No lighting                           */
#define LPT_LIGHT_STUDIO        1     /* Studio lighting                       */
#define LPT_LIGHT_BUTTERFLY     2     /* Butterfly lighting                    */
#define LPT_LIGHT_SPLIT         3     /* Split lighting                        */
#define LPT_LIGHT_CONTOUR       4     /* Contour lighting                      */
#define LPT_LIGHT_SATGE         5     /* Stage lighting                        */
#define LPT_LIGHT_CLASSIC       6     /* Classic lighting                      */
#define LPT_LIGHT_WINDOW        7     /* Window lighting                       */
#define LPT_LIGHT_WAVEDOT       8     /* Wavedot lighting                      */

/* Light portrait options */
typedef struct {
    unsigned char lightPortraitType;       /* The target light portrait type: no lighting, soft lighting, butterfly lighting, split lighting, contour lighting, stage lighting or classic lighting */
    unsigned char lightCursor;             /* Control fill light and shade ratio. Value range [0, 35]           */
    unsigned char lightWeight;             /* Control fill light intensity. Value range [0, 20]                 */
    unsigned char lightSplitMode;          /* The LightSplit direction. Value: LPT_LIGHTSPLIT_RIGHT or LPT_LIGHTSPLIT_LEFT    */
    unsigned char platformInfo;            /* High-performance platform: 0 ; Low-performance platform: 1                      */
    unsigned char debugMode;               /* Debug mode. 0 --> OFF; 1 --> ON                       */    
    int cameraWork;                        /* The work camera; front or rear                        */
    int cameraBV;                          /* The value of bv for judjing ambient brightness        */ 
    int cameraISO;                         /* The value of iso for judjing light sensitivity        */
    int cameraCT;                          /* The value of ct for judjing color temperature         */
} LPT_PORTRAIT_OPTION;


typedef enum {
    LPT_YUV420_FORMAT_CBCR = 0,          /* NV12 format; pixel order:  CbCrCbCr     */
    LPT_YUV420_FORMAT_CRCB = 1           /* NV21 format; pixel order:  CrCbCrCb     */
} LPT_YUV420_FORMAT;

/* YUV420SP image structure */
typedef struct {
    int width;                       /* Image width                              */
    int height;                      /* Image height                             */
    LPT_YUV420_FORMAT format;        /* Image format                             */
    unsigned char *yData;            /* Y data pointer                           */
    unsigned char *uvData;           /* UV data pointer                          */
} LPT_IMAGE_YUV420SP;

/* The portrait background segmentation template */
typedef struct
{
    int width;                      /* Image width                               */
    int height;                     /* Image height                              */
    unsigned char *data;            /* Data pointer                              */
} LPT_PORTRAITMASK;

/* The face information structure */
typedef struct
{
    int x, y, width, height;        /* Face rectangle                            */
    int yawAngle;                   /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;                  /* In-plane rotation angle (Roll);   In (-180, +180] degrees; */
    int score;                      /* Face detect confidence */

    unsigned char faceAttriRace;    /* Skin color of race: yellow, white, black, or indian        */
    unsigned char faceAttriGender;  /* Gender from face attribute detection demo */
    unsigned char faceAttriAge;     /* Age from face attribute detection demo    */
} LPT_FACEINFO;

typedef struct  
{
    float pitch;      /*	unit:radian */
    float yaw;        /*	unit:radian */
    float roll;		  /*	unit:radian	*/
    float t3d[3];
    float scale;
    float R[3][3];
    float alpha_shp[40];
    float alpha_exp[20];

}LPT_DFAINFO;


/* The light portrait handle */
typedef void * LPT_PORTRAIT_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

/* Get the software version */
LPTAPI(const char *) LPT_GetVersion();

/* Initialize the LPT_PORTRAIT_OPTION structure by default values */
LPTAPI(void) LPT_InitPortraitOption(LPT_PORTRAIT_OPTION *option);

/*
\brief Create a Light Portrait handle
\param hLPT         Pointer to the created Light Portrait handle
\param workMode     Work mode: LPT_WORKMODE_STILL or LPT_WORKMODE_MOVIE
\param threadNum    Number of thread to use. Value range [1, 4]
\return Returns     LPT_OK if successful. Returns negative numbers otherwise.
*/
LPTAPI(int) LPT_CreatePortraitHandle(LPT_PORTRAIT_HANDLE *hLPT, int workMode, int threadNum);

/* Release the Light Portrait handle */
LPTAPI(void) LPT_DeletePortraitHandle(LPT_PORTRAIT_HANDLE *hLPT);

/* 
\brief Do Light Portrait on the YUV420SP image
\param hLPT         The Light Portrait handle
\param imageYUV     Pointer to the YUV420SP image. The image is processed in-place
\param imageMask    Pointer to the portrait mask image. The image is processed in-place
\param option       The light portrait options
\param faceInfo     The head of the face array. 
\param faceCount    The face count in the face array.
\return Returns     LPT_OK if successful. Returns negative numbers otherwise.
*/
LPTAPI(int) LPT_lightPortrait_YUV420SP(LPT_PORTRAIT_HANDLE hLPT, 
                                       LPT_IMAGE_YUV420SP *imageYUV,
                                       LPT_PORTRAITMASK *imageMask,
                                       const LPT_PORTRAIT_OPTION *option,
                                       const LPT_FACEINFO *faceInfo,
                                       int faceCount,
                                       const LPT_DFAINFO *dfaInfo);

#ifdef  __cplusplus
}
#endif

#endif /* __UNISOC_LIGHTPORTRAIT_API_H__ */
