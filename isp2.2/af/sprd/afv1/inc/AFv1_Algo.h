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
 * AF_Algo.h
 *
 * \brief
 * Algorithm method
 *
 * \date
 * 2016/01/20
 *
 * \author
 * Galen Tsai
 *
 *
 *******************************************************************************
 */
#ifndef __AFV1_ALGO_H__
#define  __AFV1_ALGO_H__

#include "AFv1_Common.h"
#include "AFv1_Tune.h"

ERRCODE AF_Cal_IncDecCount(AF_FV_DATA * pAF_FV_Data, cmr_u8 FrameNum);
ERRCODE AF_Find_MaxMinIdx(AF_FV_DATA * pAF_FV_Data, cmr_u16 * SearchTable, cmr_u8 FrameNum);
ERRCODE AF_CheckCurve(AF_Data * pAF_Data, AF_FV_DATA * pAF_FV_Data, cmr_u16 * SearchTable, cmr_u8 FrameNum, cmr_u8 MaxTableNum, cmr_u8 DIR);
ERRCODE AF_Quad_Fit(double *pPos, double *pFV, cmr_u8 length, cmr_u16 * pPeak, cmr_u64 * pPredictFV);
ERRCODE AF_Cal_FV_Confidence(AF_Data * pAF_Data, AF_FV_DATA * pAF_FV_DATA, cmr_u8 FrameNum, AF_FILTER_TH * pAF_FILTER_TH, cmr_u8 Search_Process);
ERRCODE AF_RvereseArray16(cmr_u16 * arr, cmr_u8 start, cmr_u8 end);
ERRCODE AF_RvereseArray32(cmr_u32 * arr, cmr_u8 start, cmr_u8 end);
ERRCODE AF_RvereseArray64(cmr_u64 * arr, cmr_u8 start, cmr_u8 end);
ERRCODE AF_RvereseAE_Rpt(AE_Report * arr, cmr_u8 start, cmr_u8 end);
ERRCODE AF_AnalyzeFV(AF_Data * pAF_Data, AF_FV * pFV, cmr_u16 * SearchTable, cmr_u8 FrameNum, cmr_u8 MaxTableNum, cmr_u8 DIR);
ERRCODE AF_Check_FV_Confidence(AF_Data * pAF_Data, AF_FV * pFV, cmr_u8 FrameNum, cmr_u8 * SAF_Result, AF_TH * pAF_TH, cmr_u16 * pPeak_POS, cmr_u8 Search_Process);

#endif
