/*-------------------------------------------------------------------*/
/*  Copyright(C) 2019 by Spreadtrum                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/
/* 
    Dense Face Alignment Library API
*/
#ifndef __SPRD_DFA_API_H__
#define __SPRD_DFA_API_H__

#if ((defined(WIN32) || defined (WIN64)) && defined(_USRDLL))
#   if defined(DFA_EXPORTS)
#       define DFAAPI_EXPORTS __declspec(dllexport)
#   else
#       define DFAAPI_EXPORTS __declspec(dllimport)
#   endif
#elif (defined(__GNUC__) && (__GNUC__ >= 4))
#   define DFAAPI_EXPORTS __attribute__((visibility("default")))
#else
#   define DFAAPI_EXPORTS
#endif

#ifndef DFAAPI
#define DFAAPI(rettype) extern DFAAPI_EXPORTS rettype
#endif

/* The error codes */
#define DFA_OK                    0  /* Ok!                                      */
#define DFA_ERROR_INTERNAL       -1  /* Error: Unknown internal error            */
#define DFA_ERROR_NOMEMORY       -2  /* Error: Memory allocation error           */
#define DFA_ERROR_INVALIDARG     -3  /* Error: Invalid argument                  */

#define DFA_POINT_NUM            68
/*Dense Face Alignment output*/
typedef struct  
{
    float pitch;         /*	unit:radian*/
    float yaw;           /*	unit:radian */
    float roll;	         /*	unit:radian	*/
    float t3d[3];        /*    3DMM translation parameter*/
    float scale;         /*    3DMM scale parameter */
    float R[3][3];       /*    3DMM rotation parameter */
    float alpha_shp[40]; /*    3DMM shape parameter */
    float alpha_exp[20]; /*    3DMM expression parameter */
}DFA_RESULT;

/*
* The DFA option
*/
typedef struct 
{
    unsigned char num_threads;	       /* (default: 1) */
} DFA_OPTION;

typedef enum
{
    DFA_ORNT_0,
    DFA_ORNT_90,
    DFA_ORNT_180,
    DFA_ORNT_270,
    DFA_ORNT_MAX
} DFA_ORNT;

/* 
* A YUV 4:2:0 image with a plane of 8bit Y samples followed by an interleaved U/V planes.
*/
typedef struct
{
    unsigned char *yData;       /* Y data pointer                    */
    unsigned char *uvData;      /* UV data pointer                   */
    int width;                  /* Image width                       */
    int height;                 /* Image height                      */
    unsigned char format;       /* Image format. 0->NV12; 1->NV21    */
                                /* NV12 format; pixel order:  CbCrCbCr     */
                                /* NV21 format; pixel order:  CrCbCrCb     */
} DFA_IMAGE_YUV420SP;

/* 
 * A YUV 4:2:0 image with a plane of 8bit Y samples followed by
 * seperate U and V planes with arbitrary row and column strides.
 */
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
} DFA_IMAGE_YUV420;

/* BGR format image*/
typedef struct  
{
    unsigned char *bData;
    unsigned char *gData;
    unsigned char *rData;
    int width;
    int height;
}DFA_IMAGE_BGR;

typedef struct
{
    int x, y;
}DFA_POINT;

/* The face information structure */
typedef struct
{
    DFA_POINT landmarks[DFA_POINT_NUM]; /* The facial landmark points. The sequence is: Two left-eye corner points
                                          Two right-eye corner points, nose tip, mouth left corner, mouth right corner
                                          */
    int roi_box[4];                       /*x, y, widht, height*/
    int rollAngle;                        /*FD roll angle*/
    unsigned char roi_type;               /*0:face rect 1: face pts68 2: face pts5, default value: 0*/


}DFA_FACEINFO;



/* The Dense Face Alignment handle */
typedef void * DFA_HANDLE;

#ifdef  __cplusplus
extern "C" {
#endif

DFAAPI(const char *) DFA_GetVersion();

/* Initialize the DFA_OPTION structure by default values */
DFAAPI(void) DFA_InitOption(DFA_OPTION *option);

/* Create a DFA_HANDLE initialized by option */
DFAAPI(int) DFA_CreateHandle(DFA_HANDLE *hDFA, const DFA_OPTION *option);

/* Release the DFA_HANDLE */
DFAAPI(void) DFA_DeleteHandle(DFA_HANDLE *hDFA);

/* 
\brief Run Dense Face Alignment on the YUV420SP image
\input:
\param hDFA         The Dense Face Alignment handle
\param inImage      Pointer to the YUV420SP image. 
\param face         The face information
\ouput:
\param result       The Dense Face Alignment result
\return             Return DFA_OK if successful. Return negative numbers otherwise.
*/
DFAAPI(int) DFA_Run_YUV420SP(DFA_HANDLE hDFA,
                                  const DFA_IMAGE_YUV420SP *inImage,
                                  const DFA_FACEINFO *face,
                                  DFA_RESULT *result);

/* 
\brief Run Dense Face Alignment on the YUV420 image
\input:
\param hDFA         The Dense Face Alignment handle
\param inImage      Pointer to the YUV420 image. 
\param face         The face information
\ouput:
\param result       The Dense Face Alignment result
\return             Return DFA_OK if successful. Return negative numbers otherwise.
*/
DFAAPI(int) DFA_Run_YUV420(DFA_HANDLE hDFA,
                                const DFA_IMAGE_YUV420 *inImage,
                                const DFA_FACEINFO *face,
                                DFA_RESULT *result);


/* 
\brief Run Dense Face Alignment on the BGR image
\input:
\param hDFA         The Dense Face Alignment handle
\param inImage      Pointer to the BGR image. 
\param face         The face information
\ouput:
\param result       The Dense Face Alignment result
\return             Return DFA_OK if successful. Return negative numbers otherwise.
*/
DFAAPI(int) DFA_Run_BGR(DFA_HANDLE hDFA,
                             const DFA_IMAGE_BGR *inImage,
                             const DFA_FACEINFO *face,
                             DFA_RESULT *result);
#ifdef  __cplusplus
}
#endif

#endif /* __SPRD_DFA_API_H__ */
