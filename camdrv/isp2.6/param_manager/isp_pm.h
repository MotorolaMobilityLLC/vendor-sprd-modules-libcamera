/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_PM_H_
#define _ISP_PM_H_
#include "cmr_types.h"
#include "isp_com.h"
#include "isp_pm_com_type.h"

#define isp_pm_cmd_mask 0xf000

#define ISP_PARAM_FROM_TOOL 1

#define BLOCK_PARAM_CFG(input, param_data, blk_cmd, blk_id, cfg_ptr, cfg_size)\
	do {\
		param_data.cmd = blk_cmd;\
		param_data.id = blk_id;\
		param_data.data_ptr = cfg_ptr;\
		param_data.data_size = cfg_size;\
		input.param_data_ptr = &param_data;\
		input.param_num = 1;} while (0)

#define IS_DCAM_BLOCK(id) ((id == ISP_BLK_BLC) ||\
	(id == DCAM_BLK_RGB_DITHER) || (id == ISP_BLK_RGB_GAIN) || \
	(id == ISP_BLK_2D_LSC) || (id == ISP_BLK_AWB_NEW) || \
	(id == DCAM_BLK_BPC_V1) || (id == DCAM_BLK_RGB_AFM_V1) ||\
	(id == DCAM_BLK_BPC) || (id == DCAM_BLK_RGB_AFM) ||\
	(id == ISP_BLK_GRGB) || (id == ISP_BLK_RGB_DITHER) ||\
	(id == ISP_BLK_RGB_AEM) || (id == DCAM_BLK_PPE))

enum isp_pm_cmd {
	ISP_PM_CMD_LOCK,
	ISP_PM_CMD_UNLOCK,
	ISP_PM_CMD_SET_BASE = 0x1000,
	ISP_PM_CMD_SET_MODE,
	ISP_PM_CMD_SET_FDR_MODE,
	ISP_PM_CMD_SET_FDR_LOCK,
	ISP_PM_CMD_SET_FDR_UNLOCK,
	ISP_PM_CMD_SET_FDR_PARAM,
	ISP_PM_CMD_SET_AWB,
	ISP_PM_CMD_SET_SMART,
	ISP_PM_CMD_SET_OTHERS,
	ISP_PM_CMD_SET_AI_SCENE_PARAM,
	ISP_PM_CMD_SET_GRID0,
	ISP_PM_CMD_SET_GRID1,
	ISP_PM_CMD_SET_GRID2,
	ISP_PM_CMD_SET_SPECIAL_EFFECT,
	ISP_PM_CMD_SET_PARAM_SOURCE,
	ISP_PM_CMD_SET_LOWLIGHT_FLAG,

	ISP_PM_CMD_GET_BASE = 0x2000,
	ISP_PM_CMD_GET_INIT_AE,
	ISP_PM_CMD_GET_INIT_ALSC,
	ISP_PM_CMD_GET_INIT_AWB,
	ISP_PM_CMD_GET_INIT_AF,
	ISP_PM_CMD_GET_INIT_AF_NEW,
	ISP_PM_CMD_GET_INIT_SMART,
	ISP_PM_CMD_GET_INIT_AFT,
	ISP_PM_CMD_GET_INIT_DUAL_FLASH,
	ISP_PM_CMD_GET_INIT_PDAF,
	ISP_PM_CMD_GET_INIT_TOF,
	ISP_PM_CMD_GET_SINGLE_SETTING,
	ISP_PM_CMD_GET_CAP_SINGLE_SETTING,
	ISP_PM_CMD_GET_ISP_SETTING,
	ISP_PM_CMD_GET_ISP_ALL_SETTING,
	ISP_PM_CMD_GET_ISP_FDR_SETTING,
	ISP_PM_CMD_GET_ISP_FDR_ALL_SETTING,
	ISP_PM_CMD_GET_DV_MODEID_BY_FPS,
	ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION,
	ISP_PM_CMD_GET_PRV_MODEID_BY_RESOLUTION,
	ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION,
	ISP_PM_CMD_GET_AE_SYNC,
	ISP_PM_CMD_GET_AE_ADAPT_PARAM,
	ISP_PM_CMD_GET_4IN1_PARAM,
	ISP_PM_CMD_GET_ATM_PARAM,
	ISP_PM_CMD_GET_FDR_PARAM,
	ISP_PM_CMD_GET_MULTI_NRDATA,
	ISP_PM_CMD_GET_HDR_PARAM,

	ISP_PM_CMD_UPDATE_BASE = 0x3000,
	ISP_PM_CMD_UPDATE_ALL_PARAMS,

	ISP_PM_CMD_SET_THIRD_PART_BASE = 0x4000,
	ISP_PM_CMD_GET_THIRD_PART_BASE = 0x5000,
	ISP_PM_CMD_UPDATE_THIRD_PART_BASE = 0x6000,
};

struct pm_workmode_input {
	cmr_u32 cam_4in1_mode;
	cmr_u32 noramosaic_4in1;
	cmr_u32 pm_sets_num;
	enum tuning_mode mode[PARAM_SET_MAX];
	enum tuning_scene_mode scene[PARAM_SET_MAX];
	enum tuning_custom define[PARAM_SET_MAX];
	cmr_u32 img_w[PARAM_SET_MAX];
	cmr_u32 img_h[PARAM_SET_MAX];
	cmr_u32 remosaic_type; /* 1: software, 2: hardware, 0:other(sensor output bin size) */
};

struct pm_workmode_output {
	cmr_u32 mode_id[PARAM_SET_MAX];
};

struct isp_pm_init_input {
	struct sensor_raw_info *sensor_raw_info_ptr;
	struct isp_data_info tuning_data[ISP_TUNE_MODE_MAX];
	struct sensor_raw_fix_info *fix_data[ISP_TUNE_MODE_MAX];
	struct sensor_nr_fix_info *nr_fix_info;
	cmr_u32 is_4in1_sensor;
	cmr_s8 *push_param_path;
};

struct isp_pm_init_output {
	cmr_u32 multi_nr_flag;
};

struct isp_pm_ioctl_input {
	struct isp_pm_param_data *param_data_ptr;
	cmr_u32 param_num;
};

struct isp_pm_ioctl_output {
	struct isp_pm_param_data *param_data;
	cmr_u32 param_num;
};


struct isp_pm_setting_params {
	struct isp_pm_param_data *param_data;
	struct isp_pm_param_data *prv_param_data;
	struct isp_pm_param_data *cap_param_data;
	cmr_u32 param_num;
	cmr_u32 prv_param_num;
	cmr_u32 cap_param_num;
};

cmr_handle isp_pm_init(struct isp_pm_init_input *input, struct isp_pm_init_output *output);
cmr_s32 isp_pm_ioctl(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output);
cmr_s32 isp_pm_update(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output);
cmr_s32 isp_pm_deinit(cmr_handle handle);


#endif
