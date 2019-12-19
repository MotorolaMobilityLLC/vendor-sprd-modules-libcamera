// ---------------------------------------------------------
// [CONFIDENTIAL]
// Copyright (c) 2018 Spreadtrum Corporation
// 4D_Tracking_algo.h
// ---------------------------------------------------------
#ifndef _TRACK_ALGO_
#define _TRACK_ALGO_

#define TRACK_VERSION "4DTrack_Algo_Ver: v0.97"

typedef unsigned int BOOL;
#define FALSE 0
#define TRUE 1

typedef struct {
	int dImageW;       //Full Image Width
	int dImageH;       //Full Image Height
	int dScalingW;     //Scaling Image Width
	int dScalingH;     //Scaling Image Height
	int dColorFormat;  //0:YCC420(NV21) 1:YCC422 2:YCC444 3:RGB888
	int dOT_Ratio;     //Tolerance Threshold: 1~2048  Default:1280
} OT_GlobalSetting;

typedef struct {
	int dMovingX;       //Center X Coordinate
	int dMovingY;       //Center Y Coordinate
	int dSize_X;        //Object Size Width
	int dSize_Y;        //Object Size Height
	int dAxis1;         //Object Axis Width
	int dAxis2;         //Object Axis Height
	int dOTDiff;
	int dOTStatus;
	int dOTFrameID;
} OT_Result;

typedef struct
{
    short int wLeft;
    short int wTop;
    short int wWidth;
    short int wHeight;
} OT_RECT;

void NV21toRGB(unsigned char *yuv420,  unsigned char *rgb888, int image_width, int image_height);

#if defined __GNUC__
__attribute__ ((visibility ("default")))
#endif 
int OT_Init(OT_GlobalSetting *a_OTSetting);

#if defined __GNUC__
__attribute__ ((visibility ("default")))
#endif 
int OT_Do(void *a_pScalingBuf, int x_point, int y_point, int af_status);

#if defined __GNUC__
__attribute__ ((visibility ("default")))
#endif 
int OT_GetResult(int *a_MovingX, int *a_MovingY, int *aOTStatus,int *a_OTFrameID);

#if defined __GNUC__
__attribute__ ((visibility ("default")))
#endif 
int OT_GetResultN1(OT_Result *a_OTResult);

#if defined __GNUC__
__attribute__ ((visibility ("default")))
#endif 
int OT_Deinit();

#if defined __GNUC__
__attribute__ ((visibility ("default")))
#endif 
int OT_Stop();

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus



#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* _TRACK_ALGO_ */
