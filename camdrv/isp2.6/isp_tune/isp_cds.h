/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ISP_CDS_PM_H_
#define _ISP_CDS_PM_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmr_types.h"
#include "cmr_common.h"
#include "isp_mw.h"
#include "isp_pm.h"
#include "isp_blocks_cfg.h"
#include "isp_pm_com_type.h"


#ifdef __cplusplus
extern "C" {

#endif


#define isp_cds_pm_cmd_mask 0xf000


#ifndef CDS_BLK_ID_MAX
#define CDS_BLK_ID_MAX 256
#endif


enum isp_cds_pm_cmd {
	ISP_CDS_PM_CMD_SET_BASE = 0x1000,
	ISP_CDS_PM_CMD_SET_MODE,
	ISP_CDS_PM_CMD_SET_SMART,
	ISP_CDS_PM_CMD_SET_GAMMA,
	ISP_CDS_PM_CMD_SET_AI_PRO,

	ISP_CDS_PM_CMD_GET_BASE = 0x2000,
	ISP_CDS_PM_CMD_GET_INIT_PARAM,
	ISP_CDS_PM_CMD_GET_ALL_PM_PARAM,
	ISP_CDS_PM_CMD_GET_ALL_ISP_PARAM,
	ISP_CDS_PM_CMD_GET_GAMMA_TAB,
};


struct isp_cds_pm_init_blk {
	cmr_u32 blk_id;
	cmr_u32 cmd;
};


struct dynamic_block_info{
	cmr_u32 blk_num;
	cmr_u32 blk_id[CDS_BLK_ID_MAX];
};


struct dynamic_block_addr{
	cmr_u32 blk_id;
	void * data_addr;
	cmr_u32 data_size;
};


struct cds_pm_init_blk {
	cmr_u32 blk_id;
	cmr_u32 register_id;
	void *param_data_ptr;
	cmr_u32 param_size;
	cmr_u32 param_counts;
};


struct cds_pm_start {
	cmr_u32 dv_mode;
	cmr_u32 zsl_flag;
	struct isp_size size;
	struct isp_size dcam_size;
	cmr_u32 work_mode;
	cmr_u32 is_4in1_sensor; /* bind 4in1 sensor,not care sensor out size */
	/* new 4in1 solution, 20191028 */
	cmr_u32 remosaic_type; /* 1: software, 2: hardware, 0:other(sensor output bin size) */
	cmr_u32 is_high_res_mode; /* 1: high resolution mode */
	cmr_u16 app_mode;
};


cmr_handle isp_cds_pm_init(void *input, char *data_path);
cmr_s32 isp_cds_pm_ioctl(cmr_handle handle, enum isp_cds_pm_cmd cmd, void *input, void *output1, void *output2);
cmr_s32 isp_cds_pm_deinit(cmr_handle handle);


#endif