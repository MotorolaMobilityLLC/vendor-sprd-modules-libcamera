// ---------------------------------------------------------
// [CONFIDENTIAL]
// Copyright (c) 2016 Spreadtrum Corporation
// pd_algo.h
// ---------------------------------------------------------
#ifndef _PD_ALGO_
#define _PD_ALGO_

#if defined __GNUC__
#define PDAF_DECL __attribute__((visibility("default")))
#else
#define PDAF_DECL
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmr_types.h"
//#include <isp_type.h>

#define PD_PIXEL_NUM (24576)
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
#define PD_PIXEL_PAIRS_NUM (32)
#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

typedef struct {
	cmr_s32 dImageW;
	cmr_s32 dImageH;
	cmr_s32 dBeginX;		//pd area begin x
	cmr_s32 dBeginY;
	cmr_s32 dAreaW;		//pd area width
	cmr_s32 dAreaH;
	cmr_s32 pd_unit_w;	//pd block width
	cmr_s32 pd_unit_h;
	cmr_s32 pd_pair_w;
	cmr_s32 pd_pair_h;
	cmr_s32 pd_pairs_num_unit;
	cmr_u16 pd_is_right[PD_PIXEL_PAIRS_NUM];		// 1:right pd pixel  0:left pd pixel
	cmr_u16 pd_pos_row[PD_PIXEL_PAIRS_NUM];		//pd pixel coordinate
	cmr_u16 pd_pos_col[PD_PIXEL_PAIRS_NUM];
	cmr_s32 dDTCTEdgeTh;
	//PD Sensor Mode (0: Sony IMX258, 1:OV13855)
	cmr_s32 dSensorMode;
	//0:No need calibration data, 1:Calibrated by module house (OTP), 2: Calibrated by SPRD
	cmr_s32 dCalibration;
	//Only dCalibration = 1 to be effective
	void *OTPBuffer;
	cmr_s32 dOVSpeedup;
	//0: Normal, 1:Mirror+Flip
	cmr_s32 dSensorSetting;
} PD_GlobalSetting;

typedef void (*PDCALLBACK) (unsigned char *);

PDAF_DECL cmr_s32 PD_Init(PD_GlobalSetting *a_pdGSetting);
PDAF_DECL cmr_s32 PD_Do(cmr_u8 *raw, cmr_u8 *y, cmr_s32 a_dRectX, cmr_s32 a_dRectY, cmr_s32 a_dRectW, cmr_s32 a_dRectH, cmr_s32 a_dArea);
PDAF_DECL cmr_s32 PD_DoType2(void *a_pInPhaseBuf_left, void *a_pInPhaseBuf_right, cmr_s32 a_dRectX, cmr_s32 a_dRectY, cmr_s32 a_dRectW, cmr_s32 a_dRectH, cmr_s32 a_dArea);
PDAF_DECL cmr_s32 PD_DoPoint2(void *a_pInPhaseBuf, cmr_s32 x_point, cmr_s32 y_point, cmr_s32 area_w, cmr_s32 area_h);
PDAF_DECL cmr_s32 PD_SetCurVCM(cmr_s32 CurVCM);
PDAF_DECL cmr_s32 PD_GetResult(cmr_s32 *a_pdConf, double *a_pdPhaseDiff, cmr_s32 *a_pdFrameID, cmr_s32 *a_pdDCCGain, cmr_s32 a_dArea);
PDAF_DECL cmr_s32 PD_Uninit();
PDAF_DECL cmr_s32 PD_PhaseFormatConverter(cmr_u8 *left_in, cmr_u8 *right_in, cmr_s32 *left_out, cmr_s32 *right_out, cmr_s32 a_dNumL, cmr_s32 a_dNumR);
PDAF_DECL cmr_s32 PD_PhasePixelReorder(cmr_s32 *pBuf_pd_l, cmr_s32 *pBuf_pd_r, cmr_s32 *pPd_left_reorder, cmr_s32 *pPd_right_reorder, cmr_s32 buf_width, cmr_s32 buf_height);

PDAF_DECL void FreeBuffer(cmr_u8 *raw);

#ifdef __cplusplus
}
#endif	//__cplusplus
#endif	/* _PD_ALGO_ */