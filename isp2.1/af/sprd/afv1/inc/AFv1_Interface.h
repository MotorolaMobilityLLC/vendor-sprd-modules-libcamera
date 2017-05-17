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
 * AF_Interface.h
 *
 * \brief
 * Interface for AF
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

#ifndef __AFV1_INTERFACE_H__
#define  __AFV1_INTERFACE_H__

cmr_u8 AF_Trigger(void *handle, AF_Trigger_Data * aft_in);
cmr_u8 AF_STOP(void *handle);

void *AF_init(AF_Ctrl_Ops * AF_Ops, af_tuning_block_param * af_tuning_data, cmr_u32 * dump_info_len, char *sys_version);
cmr_u8 AF_deinit(void *handle);

cmr_u8 AF_Process_Frame(void *handle);
cmr_u8 AF_Get_Result(void *handle, cmr_u8 * AF_Result);
cmr_u8 AF_record_vcm_pos(void *handle, cmr_u32 vcm_pos);
cmr_u8 AF_record_wins(void *handle, cmr_u8 index, cmr_u32 start_x, cmr_u32 start_y, cmr_u32 end_x, cmr_u32 end_y);
cmr_u32 AF_Get_alg_mode(void *handle);
cmr_u8 AF_is_finished(void *handle);
cmr_u32 AF_Get_Bokeh_result(void *handle, Bokeh_Result * result);
#endif
