/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "isp_blocks_cfg.h"




 isp_s32 _pm_nlc_init_v1(void *dst_nlc_param, void *src_nlc_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_nlc_param_v1 *dst_ptr = (struct isp_nlc_param_v1 *)dst_nlc_param;
	struct sensor_nlc_param *src_ptr = (struct sensor_nlc_param *)src_nlc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = header_ptr->bypass;

	for (i = 0; i < ISP_NLC_POINTER_NUM; ++i) {
		dst_ptr->cur.node.r_node[i] = src_ptr->r_node[i];
		dst_ptr->cur.node.g_node[i] = src_ptr->g_node[i];
		dst_ptr->cur.node.b_node[i] = src_ptr->b_node[i];
	}

	for( i = 0; i < ISP_NLC_POINTER_L_NUM; ++i) {
		dst_ptr->cur.node.l_node[i] = src_ptr->l_node[i];
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

 isp_s32 _pm_nlc_set_param_v1(void *nlc_param, isp_u32 cmd, void* param_ptr0, void *param_ptr1)
{
	isp_u32 i = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlc_param_v1 *dst_ptr = (struct isp_nlc_param_v1*)nlc_param;
	struct sensor_nlc_param *src_ptr = (struct sensor_nlc_param *)param_ptr0;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_NLC:
		for (i = 0; i < ISP_NLC_POINTER_NUM; ++i) {
			dst_ptr->cur.node.r_node[i] = src_ptr->r_node[i];
			dst_ptr->cur.node.g_node[i] = src_ptr->g_node[i];
			dst_ptr->cur.node.b_node[i] = src_ptr->b_node[i];
		}
		for( i = 0; i < ISP_NLC_POINTER_L_NUM; ++i) {
			dst_ptr->cur.node.l_node[i] = src_ptr->l_node[i];
		}
	break;

	case ISP_PM_BLK_NLC_BYPASS:
		dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		header_ptr->is_update = ISP_ZERO;
	break;

	}

	return rtn;
}
 isp_s32 _pm_nlc_get_param_v1(void *nlc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	struct isp_nlc_param_v1 *nlc_ptr = (struct isp_nlc_param_v1 *)nlc_param;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_NLC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nlc_ptr->cur;
		param_data_ptr->data_size = sizeof(nlc_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_NLC_BYPASS:
		param_data_ptr->data_ptr = (void*)&nlc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(nlc_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}
