/*
 *******************************************************************************
 * $Header$
 *
 *  Copyright (c) 2016-2025 Spreadtrum Inc. All rights reserved.
 *
 *  +-----------------------------------------------------------------+
 *  | THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY ONLY BE USED |
 *  | AND COPIED IN ACCORDANCE WITH THE TERMS AND CONDITIONS OF SUCH  |
 *  | A LICENSE AND WITH THE INCLUSION OF THE THIS COPY RIGHT NOTICE. |
 *  | THIS SOFTWARE OR ANY OTHER COPIES OF THIS SOFTWARE MAY NOT BE   |
 *  | PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY OTHER PERSON. THE   |
 *  | OWNERSHIP AND TITLE OF THIS SOFTWARE IS NOT TRANSFERRED.        |
 *  |                                                                 |
 *  | THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT   |
 *  | ANY PRIOR NOTICE AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY |
 *  | SPREADTRUM INC.                                                 |
 *  +-----------------------------------------------------------------+
 *
 * $History$
 * 
 *******************************************************************************
 */

/*!
 *******************************************************************************
 * Copyright 2016-2025 Spreadtrum, Inc. All rights reserved.
 *
 * \file
 * AF_Tune.h
 *
 * \brief
 * Tuning parameters for AF
 *
 * \date
 * 2016/01/03
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */
#ifndef __AFV1_TUNE_H__
#define  __AFV1_TUNE_H__

#include "AFv1_Common.h"

#define MC_MIN  220      //minimal mechenical position 
#define MC_MAX  950      //maximal mechenical position

#define OTP_INF  440      //INF position of OTP 
#define OTP_MACRO  562    //MACRO position of OTP

#define HYPER    410      //Hyper Focus position 

#define Move_TH  50      //skip one frame if move position is bigger than TH
/*
#define MC_MIN  332      //minimal mechenical position 
#define MC_MAX  980      //maximal mechenical position

#define OTP_INF  483      //INF position of OTP 
#define OTP_MACRO  631    //MACRO position of OTP

#define Move_TH  100      //skip one frame if move position is bigger than TH
*/

// the threshold of sobel9 	
#define	SAF_RS_SBL9_UB_Ratio_TH_Total	0
#define	SAF_RS_SBL9_UB_Ratio_TH_Slope_Total 0
#define	SAF_RS_SBL9_UB_Ratio_TH_J1	0
#define	SAF_RS_SBL9_UB_Ratio_TH_J2	0
#define	SAF_RS_SBL9_UB_Ratio_TH_J3	0
#define	SAF_RS_SBL9_LB_Ratio_TH_Total	0
#define	SAF_RS_SBL9_LB_Ratio_TH_Slope_Total 0
#define	SAF_RS_SBL9_LB_Ratio_TH_J1	0
#define	SAF_RS_SBL9_LB_Ratio_TH_J2	0
#define	SAF_RS_SBL9_LB_Ratio_TH_J3	0
			
#define	SAF_FS_SBL9_UB_Ratio_TH_Total	0
#define	SAF_FS_SBL9_UB_Ratio_TH_Slope_Total 0
#define	SAF_FS_SBL9_UB_Ratio_TH_J1	0
#define	SAF_FS_SBL9_UB_Ratio_TH_J2	0
#define	SAF_FS_SBL9_UB_Ratio_TH_J3	0
#define	SAF_FS_SBL9_LB_Ratio_TH_Total	0
#define	SAF_FS_SBL9_LB_Ratio_TH_Slope_Total 0
#define	SAF_FS_SBL9_LB_Ratio_TH_J1	0
#define	SAF_FS_SBL9_LB_Ratio_TH_J2	0
#define	SAF_FS_SBL9_LB_Ratio_TH_J3	0

#define	SAF_SBL9_MIN_FV_TH	    1000
			
// the threshold of sobel5 			
#define	SAF_RS_SBL5_UB_Ratio_TH_Total	0
#define	SAF_RS_SBL5_UB_Ratio_TH_J1	0
#define	SAF_RS_SBL5_UB_Ratio_TH_J2	0
#define	SAF_RS_SBL5_UB_Ratio_TH_J3	0
#define	SAF_RS_SBL5_LB_Ratio_TH_Total	0
#define	SAF_RS_SBL5_LB_Ratio_TH_J1	0
#define	SAF_RS_SBL5_LB_Ratio_TH_J2	0
#define	SAF_RS_SBL5_LB_Ratio_TH_J3	0
			
#define	SAF_FS_SBL5_UB_Ratio_TH_Total	0
#define	SAF_FS_SBL5_UB_Ratio_TH_J1	0
#define	SAF_FS_SBL5_UB_Ratio_TH_J2	0
#define	SAF_FS_SBL5_UB_Ratio_TH_J3	0
#define	SAF_FS_SBL5_LB_Ratio_TH_Total	0
#define	SAF_FS_SBL5_LB_Ratio_TH_J1	0
#define	SAF_FS_SBL5_LB_Ratio_TH_J2	0
#define	SAF_FS_SBL5_LB_Ratio_TH_J3	0

#define	SAF_SBL5_MIN_FV_TH	    1000
			
// the threshold of spsmd 1x			
#define	SAF_RS_SPSMD_UB_Ratio_TH_Total	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_Slope_Total	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_J1	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_J2	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_J3	10000
#define	SAF_RS_SPSMD_LB_Ratio_TH_Total	1500
#define	SAF_RS_SPSMD_LB_Ratio_TH_Slope_Total	25
#define	SAF_RS_SPSMD_LB_Ratio_TH_J1	400
#define	SAF_RS_SPSMD_LB_Ratio_TH_J2	500
#define	SAF_RS_SPSMD_LB_Ratio_TH_J3	800
			
#define	SAF_FS_SPSMD_UB_Ratio_TH_Total	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_Slope_Total	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_J1	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_J2	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_J3	10000
#define	SAF_FS_SPSMD_LB_Ratio_TH_Total	500
#define	SAF_FS_SPSMD_LB_Ratio_TH_Slope_Total	0
#define	SAF_FS_SPSMD_LB_Ratio_TH_J1	300
#define	SAF_FS_SPSMD_LB_Ratio_TH_J2	350
#define	SAF_FS_SPSMD_LB_Ratio_TH_J3	400

#define	SAF_SPSMD_MIN_FV_TH	    1000

// the threshold of spsmd 16x			
#define	SAF_RS_SPSMD_UB_Ratio_TH_Total_16x	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_Slope_Total_16x	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_J1_16x	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_J2_16x	10000
#define	SAF_RS_SPSMD_UB_Ratio_TH_J3_16x	10000
#define	SAF_RS_SPSMD_LB_Ratio_TH_Total_16x	1500
#define	SAF_RS_SPSMD_LB_Ratio_TH_Slope_Total_16x	12
#define	SAF_RS_SPSMD_LB_Ratio_TH_J1_16x	400
#define	SAF_RS_SPSMD_LB_Ratio_TH_J2_16x	500
#define	SAF_RS_SPSMD_LB_Ratio_TH_J3_16x	800
			
#define	SAF_FS_SPSMD_UB_Ratio_TH_Total_16x	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_Slope_Total_16x	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_J1_16x	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_J2_16x	10000
#define	SAF_FS_SPSMD_UB_Ratio_TH_J3_16x	10000
#define	SAF_FS_SPSMD_LB_Ratio_TH_Total_16x	500
#define	SAF_FS_SPSMD_LB_Ratio_TH_Slope_Total_16x	0
#define	SAF_FS_SPSMD_LB_Ratio_TH_J1_16x	300
#define	SAF_FS_SPSMD_LB_Ratio_TH_J2_16x	350
#define	SAF_FS_SPSMD_LB_Ratio_TH_J3_16x	400


#define	SAF_SPSMD_MIN_FV_TH_16x	    1000
			
// the threshold of FV0			
#define	SAF_RS_FV0_UB_Ratio_TH_Total	10000
#define	SAF_RS_FV0_UB_Ratio_TH_J1	10000
#define	SAF_RS_FV0_UB_Ratio_TH_J2	10000
#define	SAF_RS_FV0_UB_Ratio_TH_J3	10000
#define	SAF_RS_FV0_LB_Ratio_TH_Total	5000
#define	SAF_RS_FV0_LB_Ratio_TH_J1	500
#define	SAF_RS_FV0_LB_Ratio_TH_J2	1000
#define	SAF_RS_FV0_LB_Ratio_TH_J3	2000
			
#define	SAF_FS_FV0_UB_Ratio_TH_Total	10000
#define	SAF_FS_FV0_UB_Ratio_TH_J1	10000
#define	SAF_FS_FV0_UB_Ratio_TH_J2	10000
#define	SAF_FS_FV0_UB_Ratio_TH_J3	10000
#define	SAF_FS_FV0_LB_Ratio_TH_Total	1000
#define	SAF_FS_FV0_LB_Ratio_TH_J1	300
#define	SAF_FS_FV0_LB_Ratio_TH_J2	350
#define	SAF_FS_FV0_LB_Ratio_TH_J3	400

#define	SAF_FV0_MIN_FV_TH	    1000
			
// the threshold of FV1			
#define	SAF_RS_FV1_UB_Ratio_TH_Total	0
#define	SAF_RS_FV1_UB_Ratio_TH_J1	0
#define	SAF_RS_FV1_UB_Ratio_TH_J2	0
#define	SAF_RS_FV1_UB_Ratio_TH_J3	0
#define	SAF_RS_FV1_LB_Ratio_TH_Total	0
#define	SAF_RS_FV1_LB_Ratio_TH_J1	0
#define	SAF_RS_FV1_LB_Ratio_TH_J2	0
#define	SAF_RS_FV1_LB_Ratio_TH_J3	0
			
#define	SAF_FS_FV1_UB_Ratio_TH_Total	0
#define	SAF_FS_FV1_UB_Ratio_TH_J1	0
#define	SAF_FS_FV1_UB_Ratio_TH_J2	0
#define	SAF_FS_FV1_UB_Ratio_TH_J3	0
#define	SAF_FS_FV1_LB_Ratio_TH_Total	0
#define	SAF_FS_FV1_LB_Ratio_TH_J1	0
#define	SAF_FS_FV1_LB_Ratio_TH_J2	0
#define	SAF_FS_FV1_LB_Ratio_TH_J3	0

#define	SAF_FV1_MIN_FV_TH	    1000
			
// the threshold of COV 			
#define	SAF_RS_COV_UB_Ratio_TH_Total	0
#define	SAF_RS_COV_UB_Ratio_TH_J1	0
#define	SAF_RS_COV_UB_Ratio_TH_J2	0
#define	SAF_RS_COV_UB_Ratio_TH_J3	0
#define	SAF_RS_COV_LB_Ratio_TH_Total	0
#define	SAF_RS_COV_LB_Ratio_TH_J1	0
#define	SAF_RS_COV_LB_Ratio_TH_J2	0
#define	SAF_RS_COV_LB_Ratio_TH_J3	0
			
#define	SAF_FS_COV_UB_Ratio_TH_Total	0
#define	SAF_FS_COV_UB_Ratio_TH_J1	0
#define	SAF_FS_COV_UB_Ratio_TH_J2	0
#define	SAF_FS_COV_UB_Ratio_TH_J3	0
#define	SAF_FS_COV_LB_Ratio_TH_Total	0
#define	SAF_FS_COV_LB_Ratio_TH_J1	0
#define	SAF_FS_COV_LB_Ratio_TH_J2	0
#define	SAF_FS_COV_LB_Ratio_TH_J3	0

#define	SAF_COV_MIN_FV_TH	    1000

#define Final_Select_Filter T_SPSMD

//For adapt AF 
#define ADAPT_AF_ENABLE 				(0x55aa)

#define ADAPT_AF_HIGH_GAIN_THR				(8 << 7)
#define ADAPT_AF_STD_CAL_RATIO			(100)
#define ADAPT_AF_FALLING_RATIO			(3)
#define ADAPT_AF_RISING_RATIO			(3)
#define ADAPT_AF_MIN_STAT_THR			((60000) >> FOCUS_STAT_DATA_ALIGMENT_BIT)//(16000 >> FOCUS_STAT_DATA_ALIGMENT_BIT)
#define ADAPT_AF_DIFF_STAT_THR			((8000) >> FOCUS_STAT_DATA_ALIGMENT_BIT)//(8000 >> FOCUS_STAT_DATA_ALIGMENT_BIT)
#define ADAPT_AF_TURNBACK_RATIO			(12)
#define ADAPT_AF_FORCEBREAK_RATIO		(ADAPT_AF_TURNBACK_RATIO-2)
#define ADAPT_AF_BREAK_RATIO			(ADAPT_AF_FORCEBREAK_RATIO-2)

#define ADAPT_AF_INTEGRA_SCR			(5)
#define ADAPT_AF_MAX_SHIFT_IDX			(6)
#define ADAPT_AF_BREAK_COUNT			(2)
#define ADAPT_AF_LL_MA_COUNT			(3)
#define ADAPT_AF_POSTURE_SLOP			(3)
#define ADAPT_AF_TEMPOR_FLAT_SLOP		(5)
#define ADAPT_AF_LIMIT_SEARCH_ITVL		(2)
#define ADAPT_AF_SKY_SCENE_THR			(800)
//sp9832 800bv8	
//J3 111bv LV8

ERRCODE Get_AF_tuning_Data(AF_Data* pAF_Data);
ERRCODE Get_Defocus_RoughScan_Table(AF_Data* pAF_Data);
ERRCODE Get_SAF_RoughScan_Table(AF_Data* pAF_Data);
ERRCODE Get_SAF_FineScan_Table(AF_Data* pAF_Data, uint8 startIdx, uint8 dirrection);
ERRCODE Get_SAF_Search_Threshold(AF_Data* pAF_Data);


#endif
 
