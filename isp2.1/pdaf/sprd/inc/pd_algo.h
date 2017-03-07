// ---------------------------------------------------------
// [CONFIDENTIAL]
// Copyright (c) 2016 Spreadtrum Corporation
// pd_algo.h
// ---------------------------------------------------------
#ifndef _PD_ALGO_
#define _PD_ALGO_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PD_VERSION "PDAF_Algo_Ver: v1.02"
#define PD_AREA_NUMBER (4)
#define PD_PIXEL_ALIGN_X (16)
#define PD_PIXEL_ALIGN_Y (32)
#define PD_PIXEL_ALIGN_HALF_X (7)
#define PD_PIXEL_ALIGN_HALF_Y (15)
#define PD_LINE_W_PIX (16)
#define PD_LINE_H_PIX (16)
#define PD_UNIT_W_SHIFT (4)
#define PD_UNIT_H_SHIFT (4)
#define PD_SLIDE_RANGE (33)   /* number of 33 means -16 to +16 */
#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

typedef struct {
	int dX;
	int dY;
	float fRatio;
	unsigned char ucType;
}PD_PixelInfo;

typedef struct {
	int dBeginX;
	int dBeginY;
  int dImageW;
  int dImageH;
	int dAreaW;
	int dAreaH;
	int dDTCTEdgeTh;
	int dUnitLine;
	//PD Sensor Mode
	//0: Sony IMX258, 1:OV13855
	int dSensorMode;
} PD_GlobalSetting;

typedef void (*PDCALLBACK) (unsigned char *);

int PD_Init(PD_GlobalSetting *a_pdGSetting);
int PD_Do(unsigned char *raw,  unsigned char  *y,
          int a_dRectX, int a_dRectY, int a_dRectW, int a_dRectH, int a_dArea);
int PD_DoType2(void *a_pInPhaseBuf_left, void *a_pInPhaseBuf_right,
          int a_dRectX, int a_dRectY, int a_dRectW, int a_dRectH, int a_dArea);
int PD_SetCurVCM(int CurVCM);
int PD_GetResult(int *a_pdConf, double *a_pdPhaseDiff, int *a_pdFrameID, int a_dArea);
int PD_Uninit();

void FreeBuffer(unsigned char *raw);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* _PD_ALGO_ */