/*-------------------------------------------------------------------*/
/*  Copyright(C) 2020 by UNISOC                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/* 
Face Detection Library API
*/

#ifndef __SPRD_FD_API_H__
#define __SPRD_FD_API_H__

#if (defined( WIN32 ) || defined( WIN64 )) && (defined FDAPI_EXPORTS)
#define FD_EXPORTS __declspec(dllexport)
#else
#define FD_EXPORTS
#endif

#ifndef FDAPI
#define FDAPI(rettype) extern FD_EXPORTS rettype
#endif

/* The error codes */
#define FD_OK                   0     /* Ok!                                      */
#define FD_ERROR_INTERNAL       -1    /* Error: Unknown internal error            */
#define FD_ERROR_NOMEMORY       -2    /* Error: Memory allocation error           */
#define FD_ERROR_INVALIDARG     -3    /* Error: Invalid argument                  */

/* Face direction definitions */
#define FD_DIRECTION_0          0x01  /* Face direction: upright direction        */
#define FD_DIRECTION_90         0x02  /* Face direction: right-rotate 90 degrees  */
#define FD_DIRECTION_180        0x04  /* Face direction: right-rotate 180 degrees */
#define FD_DIRECTION_270        0x08  /* Face direction: right-rotate 270 degrees */
#define FD_DIRECTION_ALL        0x0F  /* Face direction: all directions           */

/* Face angle range on each face direction */
#define FD_ANGLE_NONE           0x00  /* Each face direction cover 0 degreess, which means not to do face detection */
#define FD_ANGLE_RANGE_30       0x01  /* Each face direction cover 30 degrees:[-15,+15] */
#define FD_ANGLE_RANGE_90       0x02  /* Each face direction cover 90 degrees:[-45,+45] */

#define FD_WORKMODE_STILL       0x00  /* Still mode: only detection               */
#define FD_WORKMODE_MOVIE       0x01  /* Movie mode: detection + tracking         */
#define FD_WORKMODE_FACEENROLL  0x02  /* Face enroll mode                         */
#define FD_WORKMODE_FACEAUTH    0x03  /* Face authentication mode                 */
#define FD_WORKMODE_DEFAULT     FD_WORKMODE_STILL /* the default work mode        */

#define FD_ENV_SW   0x00
#define FD_ENV_HW   0x01
#define FD_ENV_DMS  0x02
#define FD_ENV_DEFAULT  FD_ENV_SW

/**
\brief version information
*/
typedef struct
{
    unsigned char major;        /*!< API major version */
    unsigned char minor;        /*!< API minor version */
    unsigned char micro;        /*!< API micro version */
    unsigned char nano;         /*!< API nano version */
    char built_date[0x20];      /*!< API built date */
    char built_time[0x20];      /*!< API built time */
    char built_rev[0x100];      /*!< API built version, linked with vcs resivion?> */
} FD_VERSION;

/* The image context */
typedef struct
{
    int orientation;
    int brightValue;
    bool aeStable;
    unsigned int backlightPro;
    unsigned int hist[256];
    int zoomRatio;
    int frameID;
} FD_IMAGE_CONTEXT;

/* The gray-scale image structure */
typedef struct
{
    unsigned char *data;              /* Image data                               */
    int data_handle;                  /* Image data from Ion handle for HW FD     */
    int width;                        /* Image width                              */
    int height;                       /* Image height                             */
    int step;                         /* The byte count per scan line             */
    FD_IMAGE_CONTEXT context;
}FD_IMAGE;

/* The face information structure */
typedef struct 
{
    int x, y, width, height;          /* Face rectangle                           */
#ifdef DMS_API
    int landmark[10];
#endif
    int yawAngle;                     /* Out-of-plane rotation angle (Yaw);In [-90, +90] degrees;   */
    int rollAngle;                    /* In-plane rotation angle (Roll); In (-180, +180] degrees;   */
    int score;                        /* Confidence score; In [0, 1000]           */
    int hid;                          /* Human ID Number                          */
    int fid;                          /* frame id */
} FD_FACEINFO;

/*platform*/
#define PLATFORM_ID_GENERIC    0x0000
#define PLATFORM_ID_PIKE2      0x0100
#define PLATFORM_ID_SHARKLE    0x0200
#define PLATFORM_ID_SHARKLER   0x0201
#define PLATFORM_ID_SHARKL3    0x0300
#define PLATFORM_ID_SHARKL5    0x0400
#define PLATFORM_ID_SHARKL5P   0x0401
#define PLATFORM_ID_SHARKL6    0x0500
#define PLATFORM_ID_SHARKL6P   0x0501
#define PLATFORM_ID_ROC1       0x0600

/* Face Detection option */
typedef struct 
{
    unsigned int platform;           /* Piek2/SharkLE/SharkL3 and so on*/
    unsigned int fdEnv;              /* FD_ENV_SW or FD_ENV_HW*/
    unsigned int workMode;           /* Work mode: FD_WORKMODE_STILL or FD_WORKMODE_MOVIE           */
    unsigned int threadNum;          /* Number of CPU threads. (In [1, 4], default: 1)              */

    /* options for both still mode and movie mode */
    unsigned int maxFaceNum;         /* Maximum face number to detect. (In [1, 1024])               */
    unsigned int minFaceSize;        /* Minimum face size to detect.(In [20, 8192], default:20)     */
    unsigned int maxFaceSize;        /* Maximum face size to detect.(In [20, 8192], default:8192)   */
    unsigned int directions;         /* Face directions to detect.(default: FD_DIRECTION_0)    */
    unsigned int angleFrontal;       /* Frontal face: angle range for each direction. (default: FD_ANGLE_RANGE_90) */
    unsigned int angleHalfProfile;   /* Half profile face: angle range for each direction. (default: FD_ANGLE_RANGE_90) */
    unsigned int angleFullProfile;   /* Full profile face: angle range for each direction. (default: FD_ANGLE_RANGE_90) */
    unsigned int detectDensity;      /* Face detection density.(In [3, 10]);                        */
    unsigned int scoreThreshold;     /* The threshold for confidence score. (In [0, 1000], default:0)  */

    /* options for movie mode only */
    unsigned int detectInterval;     /* The frame intervals between the searches for new faces during tracking; (In [0, 50]) */
    unsigned int trackDensity;       /* Face tracking density.(In [3, 10])                                                   */
    unsigned int lostRetryCount;     /* The number of frames to try to search a face that was lost during tracking. (In [0, 10]) */
    unsigned int lostHoldCount;      /* The number of frames to keep outputting a lost face. (In [0, 10]);                   */
    unsigned int holdPositionRate;   /* If the displacement rate during tracking is below the rate, the face center will be corrected back to the previous one. (In [0, 20]) */
    unsigned int holdSizeRate;       /* If the size change during tracking is below the rate, the face size will be corrected back to the previous one. (In [0, 30]) */
    unsigned int swapFaceRate;       /* When the detected face count is larger than "maxFaceNum", only if the new face is larger than the old face by the rate, the old face is replaced by the new face. */
    unsigned int guessFaceDirection; /* 1-->TRUE; 0 --> FALSE; If set as TRUE, new face search will only be performed on the guessed directions, which can speed up the detection */

} FD_OPTION;

/* Face Detector handle */
typedef void * FD_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

    /* Get the software version_new*/
    FDAPI(int)  FdGetVersion(FD_VERSION* version);

    /* Init the FD_OPTION structure by default values */
    FDAPI(void) FdInitOption(FD_OPTION *option);

    /* Create a Face Detector handle according to the input option */
    FDAPI(int)  FdCreateDetector(FD_HANDLE *hDT, const FD_OPTION *option);

    /* Release the Face Detector handle */
    FDAPI(void) FdDeleteDetector(FD_HANDLE *hDT);

    /* Detect face on the input gray-scale image */
    FDAPI(int)  FdDetectFace(FD_HANDLE hDT, const FD_IMAGE *grayImage);

    /* Clear the faces detected in the previous image */
    FDAPI(int)  FdClearFace(FD_HANDLE hDT);

    /* Get the detected face count */
    FDAPI(int)  FdGetFaceCount(const FD_HANDLE hDT);

    /* Get the face information at the specified index */
    FDAPI(int)  FdGetFaceInfo(const FD_HANDLE hDT, int faceIndex, FD_FACEINFO *faceInfo);

    // This function is provided for speed up face detection. 
    // minFaceSize and refFaceAngle will override the settings in FD_OPTION
    // It can only run the the STILL mode
    // faceDirection must be a subset of FD_OPTION.directions
    FDAPI(int)  FdDetectFaceExt(FD_HANDLE hDT,
        const FD_IMAGE *grayImage,
        unsigned int minFaceSize,
        unsigned int faceDirection);

#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_FDAPI_H__ */
