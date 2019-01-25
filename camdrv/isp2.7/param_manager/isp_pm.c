/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "isp_pm"

#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#include <string.h>
#endif
#include "ae_tuning_type.h"
#include "isp_mw.h"
#include "isp_pm.h"
#include "isp_blocks_cfg.h"
#include "cmr_types.h"

#define ISP_PM_BUF_NUM        10
#define ISP_PM_MAGIC_FLAG        0xFFEE5511

enum {
	ISP_SCENE_PRV = 0,
	ISP_SCENE_CAP,
	ISP_SCENE_LOWLIGHT_CAP,
	ISP_SCENE_MAX
};

struct isp_pm_context {
	cmr_u32 magic_flag;
	cmr_u32 param_source;
	pthread_mutex_t pm_mutex;
	cmr_u32 is_4in1_sensor;
	cmr_u32 cam_4in1_mode;
	cmr_u32 lowlight_flag;
	cmr_u32 mode_id;
	cmr_u32 prv_mode_id;
	cmr_u32 cap_mode_id;
	struct isp_context cxt_array[ISP_TUNE_MODE_MAX];
	struct isp_pm_mode_param *active_mode[ISP_SCENE_MODE_MAX];
	struct isp_pm_param_data *temp_param_data_ptr[2];
	struct isp_pm_param_data temp_param_data[ISP_TUNE_BLOCK_MAX];
	struct isp_pm_mode_param *tune_mode_array[ISP_TUNE_MODE_MAX];
	struct isp_pm_mode_param *merged_mode_array[ISP_TUNE_MODE_MAX];
};

static cmr_s32 isp_pm_check_handle(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid pm_cxt_ptr");
		rtn = ISP_ERROR;
		return rtn;
	}

	if (ISP_PM_MAGIC_FLAG != pm_cxt_ptr->magic_flag) {
		ISP_LOGE("fail to get valid param : magic_flag = 0x%x", pm_cxt_ptr->magic_flag);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static void isp_pm_check_param(cmr_u32 id, cmr_u32 *update_flag)
{
	switch (id) {
	case DCAM_BLK_NLM:
	case ISP_BLK_RGB_DITHER:
	case DCAM_BLK_BPC:
	case ISP_BLK_GRGB:
	case ISP_BLK_CFA:
	case DCAM_BLK_RGB_AFM:
	case ISP_BLK_UVDIV:
	case DCAM_BLK_3DNR_PRE:
	case DCAM_BLK_3DNR_CAP:
	case ISP_BLK_YUV_PRECDN:
	case ISP_BLK_YNR:
	case ISP_BLK_EDGE:
	case ISP_BLK_UV_CDN:
	case ISP_BLK_UV_POSTCDN:
	case ISP_BLK_IIRCNR_IIR:
	case ISP_BLK_YUV_NOISEFILTER:
	case ISP_BLK_CNR2:
	{
		*update_flag = ISP_PARAM_FROM_TOOL;
		break;
	}
	default:
		break;
	}
}

static struct isp_pm_block_header *isp_pm_get_block_header
	(struct isp_pm_mode_param *pm_mode_param_ptr, cmr_u32 id, cmr_s32 *index)
{
	cmr_u32 i = 0, blk_num = 0;
	struct isp_pm_block_header *header_ptr = PNULL;
	struct isp_pm_block_header *blk_header = PNULL;

	if (PNULL == pm_mode_param_ptr) {
		ISP_LOGE("fail to get valid pm_mode_param_ptr");
		return PNULL;
	}

	blk_header = (struct isp_pm_block_header *)pm_mode_param_ptr->header;
	blk_num = pm_mode_param_ptr->block_num;
	*index = ISP_TUNE_BLOCK_MAX;

	if (PNULL != blk_header) {
		for (i = 0; i < blk_num; i++) {
			if (id == blk_header[i].block_id) {
				break;
			}
		}

		if (i < blk_num) {
			header_ptr = (struct isp_pm_block_header *)&blk_header[i];
			*index = i;
		}
	} else {
		ISP_LOGE("fail to get valid param : blk_header = %p", blk_header);
	}

	return header_ptr;
}

static cmr_s32 isp_pm_context_init(cmr_handle handle,
	cmr_u32 mode_id, cmr_u32 scene_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	cmr_u32 update_flag = 0;

	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid pm_cxt_ptr");
		rtn = ISP_ERROR;
		return rtn;
	}

	if (PNULL == pm_cxt_ptr->merged_mode_array[mode_id]) {
		ISP_LOGE("fail to get mode %d param", mode_id);
		rtn = ISP_ERROR;
		return rtn;
	}

	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[scene_id];
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[mode_id];

	if ((PNULL != isp_cxt_ptr) && (PNULL != mode_param_ptr)) {
		blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
		blk_num = mode_param_ptr->block_num;
		if (PNULL != blk_header_array) {
			for (i = 0; i < blk_num; i++) {
				id = blk_header_array[i].block_id;
				blk_cfg_ptr = isp_pm_get_block_cfg(id);
				blk_header_ptr = &blk_header_array[i];
				if (pm_cxt_ptr->param_source) {
					isp_pm_check_param(id, &update_flag);
					if (0 != update_flag) {
						update_flag = 0;
						continue;
					}
				}
				if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
					if (blk_cfg_ptr->ops) {
						ops = blk_cfg_ptr->ops;
						if (ops->init) {
							if (blk_header_ptr->size > 0) {
								isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
								offset = blk_cfg_ptr->offset;
								blk_ptr = (void *)(isp_cxt_start_addr + offset);
								param_data_ptr = (void *)blk_header_ptr->absolute_addr;
								if ((PNULL != blk_ptr) && (PNULL != param_data_ptr)) {
									ops->init(blk_ptr, param_data_ptr, blk_header_ptr,
										&mode_param_ptr->resolution);
								} else {
									ISP_LOGE("fail to get valid param : i = %d, blk_name = %s",
										i, blk_header_ptr->name);
									ISP_LOGE("fail to get valid param : blk_ptr = %p, param_data_ptr = %p, resolution = %p",
										blk_ptr, param_data_ptr, &mode_param_ptr->resolution);
									rtn = ISP_ERROR;
									return rtn;
								}
							}
						}
					}
				} else {
					ISP_LOGV("i = %d, id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p",
						i, id, blk_cfg_ptr, blk_header_ptr);
				}
			}
		} else {
			ISP_LOGE("fail to get valid param : blk_header_array = %p", blk_header_array);
			rtn = ISP_ERROR;
			return rtn;
		}
	} else {
		ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, mode_param_ptr = %p",
			isp_cxt_ptr, mode_param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_s32 isp_pm_context_update(cmr_handle handle,
	struct isp_pm_mode_param *org_mode_param, cmr_u32 mode_id, cmr_u32 scene_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid pm_cxt_ptr");
		rtn = ISP_ERROR;
		return rtn;
	}

	if (PNULL == pm_cxt_ptr->merged_mode_array[mode_id]) {
		ISP_LOGE("fail to get mode %d param", mode_id);
		rtn = ISP_ERROR;
		return rtn;
	}

	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[scene_id];
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[mode_id];

	if ((PNULL != isp_cxt_ptr) && (PNULL != mode_param_ptr)) {
		blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
		blk_num = mode_param_ptr->block_num;
		if (PNULL != blk_header_array) {
			for (i = 0; i < blk_num; i++) {
				id = blk_header_array[i].block_id;
				blk_cfg_ptr = isp_pm_get_block_cfg(id);
				blk_header_ptr = &blk_header_array[i];
				if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
					if (blk_cfg_ptr->ops) {
						ops = blk_cfg_ptr->ops;
						if (ops->init) {
							if (blk_header_ptr->size > 0) {
								isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
								offset = blk_cfg_ptr->offset;
								blk_ptr = (void *)(isp_cxt_start_addr + offset);
								param_data_ptr = (void *)blk_header_ptr->absolute_addr;
								if ((PNULL != blk_ptr) && (PNULL != param_data_ptr)) {
									for (j = 0; j < org_mode_param->block_num; j++) {
										if (org_mode_param->header[j].block_id == blk_header_ptr->block_id) {
											break;
										}
									}
									if (j < org_mode_param->block_num) {
										if (blk_header_ptr->source_flag != org_mode_param->header[j].source_flag) {
											ops->init(blk_ptr, param_data_ptr, blk_header_ptr,
												&mode_param_ptr->resolution);
										}
									} else {
										ops->init(blk_ptr, param_data_ptr, blk_header_ptr,
											&mode_param_ptr->resolution);
									}
								} else {
									ISP_LOGE("fail to get valid param : i = %d, blk_name = %s",
										i, blk_header_ptr->name);
									ISP_LOGE("fail to get valid param : blk_ptr = %p, param_data_ptr = %p, resolution = %p",
										blk_ptr, param_data_ptr, &mode_param_ptr->resolution);
									rtn = ISP_ERROR;
									return rtn;
								}
							}
						}
					}
				} else {
					ISP_LOGV("i = %d, id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p",
						i, id, blk_cfg_ptr, blk_header_ptr);
				}
			}
		} else {
			ISP_LOGE("fail to get valid param : blk_header_array = %p", blk_header_array);
			rtn = ISP_ERROR;
			return rtn;
		}
	} else {
		ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, mode_param_ptr = %p",
			isp_cxt_ptr, mode_param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_s32 isp_pm_change_mode(cmr_handle handle,
	cmr_u32 mode_id, cmr_u32 scene_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_mode_param *org_mode_param = PNULL;
	struct isp_pm_mode_param *next_mode_param = PNULL;

	org_mode_param = pm_cxt_ptr->active_mode[scene_id];
	next_mode_param = (struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[mode_id];
	pm_cxt_ptr->active_mode[scene_id] = next_mode_param;

	pm_cxt_ptr->cxt_array[scene_id].mode_id =
		pm_cxt_ptr->merged_mode_array[mode_id]->mode_id;

	isp_pm_context_update(handle, org_mode_param, mode_id, scene_id);

	return rtn;
}

static cmr_s32 isp_pm_context_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid pm_cxt_ptr");
		rtn = ISP_ERROR;
		return rtn;
	}

	for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
		if (PNULL == pm_cxt_ptr->merged_mode_array[j]) {
			continue;
		}
		isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[j];
		mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[j];

		if ((PNULL != isp_cxt_ptr) && (PNULL != mode_param_ptr)) {
			blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
			blk_num = mode_param_ptr->block_num;
			if (PNULL != blk_header_array) {
				for (i = 0; i < blk_num; i++) {
					id = blk_header_array[i].block_id;
					blk_cfg_ptr = isp_pm_get_block_cfg(id);
					if (PNULL != blk_cfg_ptr) {
						if (blk_cfg_ptr->ops) {
							ops = blk_cfg_ptr->ops;
							if (ops->deinit) {
								isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
								offset = blk_cfg_ptr->offset;
								blk_ptr = (void *)(isp_cxt_start_addr + offset);
								if (PNULL != blk_ptr) {
									ops->deinit(blk_ptr);
								} else {
									ISP_LOGE("fail to get valid param : i = %d, blk_name = %s, blk_ptr = %p",
										i, blk_header_array[i].name, blk_ptr);
									rtn = ISP_ERROR;
									return rtn;
								}
							}
						}
					} else {
						ISP_LOGV("i = %d, id = 0x%x, blk_cfg_ptr = %p", i, id, blk_cfg_ptr);
					}
				}
			} else {
				ISP_LOGE("fail to get valid param : blk_header_array = %p", blk_header_array);
				rtn = ISP_ERROR;
				return rtn;
			}
		} else {
			ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, mode_param_ptr = %p",
				isp_cxt_ptr, mode_param_ptr);
			rtn = ISP_ERROR;
			return rtn;
		}
	}

	return rtn;
}

static cmr_s32 isp_pm_set_block_param(struct isp_pm_context *pm_cxt_ptr,
	struct isp_pm_param_data *param_data_ptr, cmr_u32 mode_id, cmr_u32 scene_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 cmd = 0, id = 0, offset = 0;
	cmr_s32 index = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	if ((PNULL == pm_cxt_ptr) || (PNULL == param_data_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, param_data_ptr = %p", pm_cxt_ptr, param_data_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[scene_id];
	mode_param_ptr =
		(struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[mode_id];

	if ((PNULL != isp_cxt_ptr) && (PNULL != mode_param_ptr)) {
		cmd = param_data_ptr->cmd;
		id = param_data_ptr->id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = isp_pm_get_block_header(mode_param_ptr, id, &index);

		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->set) {
					isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					if (PNULL != blk_ptr) {
						ops->set(blk_ptr, cmd, param_data_ptr->data_ptr, blk_header_ptr);
					} else {
						ISP_LOGE("fail to get valid param : blk_name = %s, blk_ptr = %p", blk_header_ptr->name, blk_ptr);
						rtn = ISP_ERROR;
						return rtn;
					}
				}
			}
		} else {
			ISP_LOGV("id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p", id, blk_cfg_ptr, blk_header_ptr);
		}
	} else {
		ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, mode_param_ptr = %p",
			isp_cxt_ptr, mode_param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_s32 isp_pm_get_mode_block_param(struct isp_pm_context *pm_cxt_ptr,
	enum isp_pm_cmd cmd, struct isp_pm_param_data *param_data_ptr,
	cmr_u32 *param_counts, cmr_u32 block_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0, counts = 0;
	cmr_u32 id = 0, blk_num = 0;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	if ((PNULL == pm_cxt_ptr) || (PNULL == param_data_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, param_data_ptr = %p", pm_cxt_ptr, param_data_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
		mode_param_ptr =
			(struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
		if (PNULL != mode_param_ptr) {
			blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
			blk_num = mode_param_ptr->block_num;
			if (PNULL != blk_header_array) {
				for (i = 0; i < blk_num; i++) {
					id = blk_header_array[i].block_id;
					if (block_id == id) {
						break;
					}
				}

				if (i < blk_num) {
					param_data_ptr->data_ptr = blk_header_array[i].absolute_addr;
					param_data_ptr->data_size = blk_header_array[i].size;
					param_data_ptr->user_data[0] = blk_header_array[i].bypass;
				} else {
					param_data_ptr->data_ptr = PNULL;
					param_data_ptr->data_size = 0;
				}
			} else {
				param_data_ptr->data_ptr = PNULL;
				param_data_ptr->data_size = 0;
			}
		} else {
			param_data_ptr->data_ptr = PNULL;
			param_data_ptr->data_size = 0;
		}

		param_data_ptr->cmd = cmd;
		param_data_ptr->mode_id = j;
		param_data_ptr->id = (j << 16) | block_id;
		param_data_ptr++;
		counts++;
	}
	*param_counts = counts;

	return rtn;
}

static cmr_s32 isp_pm_get_single_block_param(struct isp_pm_context *pm_cxt_ptr,
	struct isp_pm_ioctl_input *blk_info_in, struct isp_pm_param_data *pm_param_data_ptr,
	cmr_u32 *param_counts, cmr_u32 *rtn_idx, cmr_u32 mode_id, cmr_u32 scene_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_s32 index = 0;
	cmr_u32 counts = 0;
	cmr_u32 cmd = 0, id = 0, offset = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_param_data *block_param_data_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	if ((PNULL == pm_cxt_ptr) || (PNULL == blk_info_in) || (PNULL == pm_param_data_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, blk_info_in = %p, pm_param_data_ptr = %p",
			pm_cxt_ptr, blk_info_in, pm_param_data_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	//just support get only one block info
	if (blk_info_in->param_num > 1) {
		ISP_LOGE("fail to get valid param : param_num = %d", blk_info_in->param_num);
		rtn = ISP_ERROR;
		return rtn;
	}

	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[scene_id];
	mode_param_ptr =
		(struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[mode_id];

	if ((PNULL != isp_cxt_ptr) && (PNULL != mode_param_ptr)) {
		block_param_data_ptr = blk_info_in->param_data_ptr;
		cmd = block_param_data_ptr->cmd;
		id = block_param_data_ptr->id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = isp_pm_get_block_header(mode_param_ptr, id, &index);

		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->get) {
					isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					if (PNULL != blk_ptr) {
						ops->get(blk_ptr, cmd, &pm_param_data_ptr[index], &blk_header_ptr->is_update);
						counts++;
					} else {
						ISP_LOGE("fail to get valid param : blk_name = %s, blk_ptr = %p", blk_header_ptr->name, blk_ptr);
						rtn = ISP_ERROR;
						return rtn;
					}
				}
			}
		} else {
			ISP_LOGV("id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p",
				id, blk_cfg_ptr, blk_header_ptr);
		}
		*rtn_idx = index;
		*param_counts = counts;
	} else {
		ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, mode_param_ptr = %p",
			isp_cxt_ptr, mode_param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_s32 isp_pm_get_setting_param(struct isp_pm_context *pm_cxt_ptr,
	struct isp_pm_param_data *param_data_ptr, cmr_u32 *param_counts,
	cmr_u32 all_setting_flag, cmr_u32 mode_id, cmr_u32 scene_id)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, counts = 0;
	cmr_u32 is_update = 0;
	cmr_u32 id = 0, offset = 0, blk_num = 0;
	void *blk_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;

	if ((PNULL == pm_cxt_ptr) || (PNULL == param_data_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, param_data_ptr = %p", pm_cxt_ptr, param_data_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[scene_id];
	mode_param_ptr =
		(struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[mode_id];

	if ((PNULL != isp_cxt_ptr) && (PNULL != mode_param_ptr)) {
		blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
		blk_num = mode_param_ptr->block_num;
		if (PNULL != blk_header_array) {
			for (i = 0; i < blk_num; i++) {
				id = blk_header_array[i].block_id;
				blk_cfg_ptr = isp_pm_get_block_cfg(id);
				blk_header_ptr = &blk_header_array[i];
				if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
					is_update = 0;
					if ((ISP_ZERO != all_setting_flag) || (ISP_ZERO != blk_header_ptr->is_update)) {
						is_update = 1;
					}
					if (is_update) {
						if (blk_cfg_ptr->ops) {
							ops = blk_cfg_ptr->ops;
							if (ops->get) {
								isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
								offset = blk_cfg_ptr->offset;
								blk_ptr = (void *)(isp_cxt_start_addr + offset);
								if (PNULL != blk_ptr) {
									ops->get(blk_ptr, ISP_PM_BLK_ISP_SETTING,
										param_data_ptr, &blk_header_ptr->is_update);
								} else {
									ISP_LOGE("fail to get valid param : i = %d, blk_name = %s, blk_ptr = %p",
										i, blk_header_ptr->name, blk_ptr);
									rtn = ISP_ERROR;
									return rtn;
								}
							}
						}
						if (param_data_ptr)
							param_data_ptr++;
						counts++;
					}
				} else {
					ISP_LOGV("i = %d, id = 0x%x, blk_cfg_ptr = %p, blk_header_ptr = %p",
						i, id, blk_cfg_ptr, blk_header_ptr);
				}
			}
			*param_counts = counts;
		} else {
			ISP_LOGE("fail to get valid param : blk_header_array = %p", blk_header_array);
			rtn = ISP_ERROR;
			return rtn;
		}
	} else {
		ISP_LOGE("fail to get valid param : isp_cxt_ptr = %p, mode_param_ptr = %p",
			isp_cxt_ptr, mode_param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static cmr_s32 isp_pm_set_param(cmr_handle handle, enum isp_pm_cmd cmd, void *param_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if ((PNULL == pm_cxt_ptr) || (PNULL == param_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, param_ptr = %p", pm_cxt_ptr, param_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	switch (cmd) {
	case ISP_PM_CMD_SET_MODE:
	{
		struct work_mode_info *mode_info = (struct work_mode_info *)param_ptr;

		pm_cxt_ptr->is_4in1_sensor = mode_info->is_4in1_sensor;
		pm_cxt_ptr->cam_4in1_mode = mode_info->cam_4in1_mode;

		pm_cxt_ptr->mode_id = mode_info->mode_id;
		if (pm_cxt_ptr->prv_mode_id != mode_info->prv_mode_id) {
			pm_cxt_ptr->prv_mode_id = mode_info->prv_mode_id;
			rtn = isp_pm_change_mode(handle, pm_cxt_ptr->prv_mode_id, ISP_SCENE_PRV);
		}
		if (pm_cxt_ptr->cap_mode_id != mode_info->cap_mode_id) {
			pm_cxt_ptr->cap_mode_id = mode_info->cap_mode_id;
			rtn = isp_pm_change_mode(handle, pm_cxt_ptr->cap_mode_id, ISP_SCENE_CAP);
		}

		ISP_LOGV("is_4in1_sensor = %d, cam_4in1_mode = %d",
			pm_cxt_ptr->is_4in1_sensor, pm_cxt_ptr->cam_4in1_mode);

		ISP_LOGV("mode_id = %d, prv_mode_id = %d, cap_mode_id = %d",
			pm_cxt_ptr->mode_id, pm_cxt_ptr->prv_mode_id, pm_cxt_ptr->cap_mode_id);

		break;
	}
	case ISP_PM_CMD_SET_LOWLIGHT_FLAG:
	{
		if (pm_cxt_ptr->is_4in1_sensor)
			pm_cxt_ptr->lowlight_flag = *(cmr_u32 *)param_ptr;
		else
			pm_cxt_ptr->lowlight_flag = 0;
		ISP_LOGV("lowlight_flag = %d", pm_cxt_ptr->lowlight_flag);
		break;
	}
	case ISP_PM_CMD_SET_AWB:
	case ISP_PM_CMD_SET_AE:
	case ISP_PM_CMD_SET_AF:
	case ISP_PM_CMD_SET_SMART:
	case ISP_PM_CMD_SET_OTHERS:
	case ISP_PM_CMD_SET_SCENE_MODE:
	case ISP_PM_CMD_SET_SPECIAL_EFFECT:
	{
		cmr_u32 i = 0;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr =
			(struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			ISP_LOGE("fail to get valid param_data_ptr");
			rtn = ISP_ERROR;
			return rtn;
		}

		for (i = 0; i < ioctrl_input_ptr->param_num; i++, param_data_ptr++) {
			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, pm_cxt_ptr->prv_mode_id, ISP_SCENE_PRV);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_set_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}
			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, pm_cxt_ptr->cap_mode_id, ISP_SCENE_CAP);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_set_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}
			if (pm_cxt_ptr->is_4in1_sensor) {
				rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, ISP_MODE_ID_CAP_1, ISP_SCENE_LOWLIGHT_CAP);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGV("fail to do isp_pm_set_block_param");
					rtn = ISP_ERROR;
					return rtn;
				}
			}
		}
		break;
	}
	case ISP_PM_CMD_SET_PRV_PARAM:
	{
		cmr_u32 i = 0;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr =
			(struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			ISP_LOGE("fail to get valid param_data_ptr");
			rtn = ISP_ERROR;
			return rtn;
		}

		for (i = 0; i < ioctrl_input_ptr->param_num; i++, param_data_ptr++) {
			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, pm_cxt_ptr->prv_mode_id, ISP_SCENE_PRV);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_set_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}
		}
		break;
	}
	case ISP_PM_CMD_SET_CAP_PARAM:
	{
		cmr_u32 i = 0;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr =
			(struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			ISP_LOGE("fail to get valid param_data_ptr");
			rtn = ISP_ERROR;
			return rtn;
		}

		for (i = 0; i < ioctrl_input_ptr->param_num; i++, param_data_ptr++) {
			rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, pm_cxt_ptr->cap_mode_id, ISP_SCENE_CAP);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_set_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}
		}
		if (pm_cxt_ptr->is_4in1_sensor) {
			for (i = 0; i < ioctrl_input_ptr->param_num; i++, param_data_ptr++) {
				rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr, ISP_MODE_ID_CAP_1, ISP_SCENE_LOWLIGHT_CAP);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGV("fail to do isp_pm_set_block_param");
					rtn = ISP_ERROR;
					return rtn;
				}
			}
		}
		break;
	}
	case ISP_PM_CMD_SET_PARAM_SOURCE:
	{
		cmr_u32 *param_source = (cmr_u32 *)param_ptr;
		pm_cxt_ptr->param_source = *param_source;
		break;
	}
	default:
		break;
	}

	return rtn;
}

static cmr_s32 isp_pm_get_param(cmr_handle handle, enum isp_pm_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0;
	cmr_u32 block_id = 0, blk_num = 0;
	cmr_u32 param_counts = 0;
	struct isp_pm_param_data *param_data_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_video_start *param_ptr = PNULL;
	struct isp_pm_ioctl_output *result_ptr = PNULL;

	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if ((PNULL == pm_cxt_ptr) || (PNULL == out_ptr)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, out_ptr = %p", pm_cxt_ptr, out_ptr);
		rtn = ISP_ERROR;
		return rtn;
	}

	switch (cmd) {
	case ISP_PM_CMD_GET_PRV_MODEID_BY_RESOLUTION:
	{
		param_ptr = (struct isp_video_start *)in_ptr;
		if (!param_ptr->is_4in1_sensor) {
			*((cmr_s32 *)out_ptr) = ISP_MODE_ID_PRV_0;
			for (i = ISP_MODE_ID_PRV_0; i <= ISP_MODE_ID_PRV_3; i++) {
				if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
					if (pm_cxt_ptr->merged_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((cmr_s32 *)out_ptr) = pm_cxt_ptr->merged_mode_array[i]->mode_id;
						break;
					}
				}
			}
		} else {
			*((cmr_s32 *)out_ptr) = ISP_MODE_ID_PRV_0;
			for (i = ISP_MODE_ID_PRV_1; i <= ISP_MODE_ID_PRV_3; i++) {
				if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
					if (pm_cxt_ptr->merged_mode_array[i]->resolution.w == param_ptr->size.w) {
						if (!param_ptr->mode_4in1) {
							*((cmr_s32 *)out_ptr) = pm_cxt_ptr->merged_mode_array[i]->mode_id;
							break;
						}
					}
				}
			}
		}
		break;
	}
	case ISP_PM_CMD_GET_CAP_MODEID_BY_RESOLUTION:
	{
		param_ptr = (struct isp_video_start *)in_ptr;
		if (!param_ptr->is_4in1_sensor) {
			*((cmr_s32 *)out_ptr) = ISP_MODE_ID_CAP_0;
			for (i = ISP_MODE_ID_CAP_0; i <= ISP_MODE_ID_CAP_3; i++) {
				if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
					if (pm_cxt_ptr->merged_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((cmr_s32 *)out_ptr) = pm_cxt_ptr->merged_mode_array[i]->mode_id;
						break;
					}
				}
			}
		} else {
			*((cmr_s32 *)out_ptr) = ISP_MODE_ID_CAP_0;
			for (i = ISP_MODE_ID_CAP_2; i <= ISP_MODE_ID_CAP_3; i++) {
				if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
					if (pm_cxt_ptr->merged_mode_array[i]->resolution.w == param_ptr->size.w) {
						if (!param_ptr->mode_4in1) {
							*((cmr_s32 *)out_ptr) = pm_cxt_ptr->merged_mode_array[i]->mode_id;
							break;
						}
					}
				}
			}
		}
		break;
	}
	case ISP_PM_CMD_GET_DV_MODEID_BY_FPS:
	{
		*((cmr_s32 *)out_ptr) = ISP_MODE_ID_VIDEO_0;
		for (i = ISP_MODE_ID_VIDEO_0; i <= ISP_MODE_ID_VIDEO_3; i++) {
			if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
				if (pm_cxt_ptr->merged_mode_array[i]->fps == (cmr_u32)(*(cmr_s32 *)in_ptr)) {
					*((cmr_s32 *)out_ptr) = pm_cxt_ptr->merged_mode_array[i]->mode_id;
					break;
				}
			}
		}
		break;
	}
	case ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION:
	{
		param_ptr = (struct isp_video_start *)in_ptr;
		if (param_ptr->dv_mode) {
			*((cmr_s32 *)out_ptr) = ISP_MODE_ID_VIDEO_0;
			for (i = ISP_MODE_ID_VIDEO_0; i <= ISP_MODE_ID_VIDEO_3; i++) {
				if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
					if (pm_cxt_ptr->merged_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((cmr_s32 *)out_ptr) = pm_cxt_ptr->merged_mode_array[i]->mode_id;
						break;
					}
				}
			}
		}
		break;
	}
	case ISP_PM_CMD_GET_ISP_SETTING:
	case ISP_PM_CMD_GET_ISP_ALL_SETTING:
	{
		cmr_u32 all_setting_flag = 0;
		struct isp_pm_ioctl_output_param *result_param_ptr = PNULL;
		result_param_ptr = (struct isp_pm_ioctl_output_param *)out_ptr;

		if (ISP_PM_CMD_GET_ISP_SETTING == cmd) {
			all_setting_flag = 0;
		} else if (ISP_PM_CMD_GET_ISP_ALL_SETTING == cmd) {
			all_setting_flag = 1;
		}

		param_data_ptr = pm_cxt_ptr->temp_param_data_ptr[0];
		rtn = isp_pm_get_setting_param(pm_cxt_ptr, param_data_ptr, &param_counts,
			all_setting_flag, pm_cxt_ptr->prv_mode_id, ISP_SCENE_PRV);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGV("fail to do isp_pm_get_setting_param");
			rtn = ISP_ERROR;
			return rtn;
		}
		result_param_ptr->prv_param_data = pm_cxt_ptr->temp_param_data_ptr[0];
		result_param_ptr->prv_param_num = param_counts;
		result_param_ptr->prv_param_data->mode_id = pm_cxt_ptr->prv_mode_id;

		param_data_ptr = pm_cxt_ptr->temp_param_data_ptr[1];
		if (!pm_cxt_ptr->lowlight_flag || !pm_cxt_ptr->cam_4in1_mode) {
			rtn = isp_pm_get_setting_param(pm_cxt_ptr, param_data_ptr, &param_counts,
				all_setting_flag, pm_cxt_ptr->cap_mode_id, ISP_SCENE_CAP);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_get_setting_param");
				rtn = ISP_ERROR;
				return rtn;
			}
		} else {
			rtn = isp_pm_get_setting_param(pm_cxt_ptr, param_data_ptr, &param_counts,
				all_setting_flag, ISP_MODE_ID_CAP_1, ISP_SCENE_LOWLIGHT_CAP);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGV("fail to do isp_pm_get_setting_param");
				rtn = ISP_ERROR;
				return rtn;
			}
		}
		result_param_ptr->cap_param_data = pm_cxt_ptr->temp_param_data_ptr[1];
		result_param_ptr->cap_param_num = param_counts;
		result_param_ptr->cap_param_data->mode_id = pm_cxt_ptr->cap_mode_id;
		break;
	}
	case ISP_PM_CMD_GET_SINGLE_SETTING:
	{
		cmr_u32 blk_idx = 0;

		param_data_ptr = pm_cxt_ptr->temp_param_data;
		rtn = isp_pm_get_single_block_param(pm_cxt_ptr, (struct isp_pm_ioctl_input *)in_ptr,
			param_data_ptr, &param_counts, &blk_idx, pm_cxt_ptr->mode_id, ISP_SCENE_PRV);
		if (ISP_SUCCESS != rtn || blk_idx == ISP_TUNE_BLOCK_MAX) {
			ISP_LOGV("fail to do isp_pm_get_single_block_param");
			rtn = ISP_ERROR;
			return rtn;
		}

		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_data = &pm_cxt_ptr->temp_param_data[blk_idx];
		result_ptr->param_num = param_counts;
		result_ptr->param_data->mode_id = pm_cxt_ptr->mode_id;
		break;
	}
	case ISP_PM_CMD_GET_CAP_SINGLE_SETTING:
	{
		cmr_u32 blk_idx = 0;

		param_data_ptr = pm_cxt_ptr->temp_param_data;
		if (!pm_cxt_ptr->lowlight_flag || !pm_cxt_ptr->cam_4in1_mode) {
			ISP_LOGV("ISP_SCENE_CAP");
			rtn = isp_pm_get_single_block_param(pm_cxt_ptr, (struct isp_pm_ioctl_input *)in_ptr,
				param_data_ptr, &param_counts, &blk_idx, pm_cxt_ptr->cap_mode_id, ISP_SCENE_CAP);
			if (ISP_SUCCESS != rtn || blk_idx == ISP_TUNE_BLOCK_MAX) {
				ISP_LOGV("fail to do isp_pm_get_single_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}
		} else {
			ISP_LOGV("ISP_SCENE_LOWLIGHT_CAP");
			rtn = isp_pm_get_single_block_param(pm_cxt_ptr, (struct isp_pm_ioctl_input *)in_ptr,
				param_data_ptr, &param_counts, &blk_idx, ISP_MODE_ID_CAP_1, ISP_SCENE_LOWLIGHT_CAP);
			if (ISP_SUCCESS != rtn || blk_idx == ISP_TUNE_BLOCK_MAX) {
				ISP_LOGV("fail to do isp_pm_get_single_block_param");
				rtn = ISP_ERROR;
				return rtn;
			}
		}

		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_data = &pm_cxt_ptr->temp_param_data[blk_idx];
		result_ptr->param_num = param_counts;
		result_ptr->param_data->mode_id = pm_cxt_ptr->cap_mode_id;
		break;
	}
	case ISP_PM_CMD_GET_AE_VERSION_ID:
	{
		cmr_s32 *version_id = (cmr_s32 *)out_ptr;

		for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
			mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[j];
			if (PNULL != mode_param_ptr) {
				blk_header_array = (struct isp_pm_block_header *)mode_param_ptr->header;
				blk_num = mode_param_ptr->block_num;
				for (i = 0; i < blk_num; i++) {
					if (ISP_BLK_AE_NEW == blk_header_array[i].block_id) {
						*version_id = blk_header_array[i].version_id;
					}
					if (-1 != *version_id) {
						break;
					}
				}
			} else {
				continue;
			}
			if (-1 != *version_id) {
				break;
			}
		}
		break;
	}
	case ISP_PM_CMD_GET_INIT_AE:
		block_id = ISP_BLK_AE_NEW;
		break;
	case ISP_PM_CMD_GET_INIT_ALSC:
		block_id = ISP_BLK_ALSC;
		break;
	case ISP_PM_CMD_GET_INIT_AWB:
		block_id = ISP_BLK_AWB_NEW;
		break;
	case ISP_PM_CMD_GET_INIT_AF:
	case ISP_PM_CMD_GET_INIT_AF_NEW:
		block_id = ISP_BLK_AF_NEW;
		break;
	case ISP_PM_CMD_GET_INIT_SMART:
		block_id = ISP_BLK_SMART;
		break;
	case ISP_PM_CMD_GET_INIT_AFT:
		block_id = ISP_BLK_AFT;
		break;
	case ISP_PM_CMD_GET_THIRD_PART_INIT_SFT_AF:
		block_id = ISP_BLK_SFT_AF;
		break;
	case ISP_PM_CMD_GET_INIT_DUAL_FLASH:
		block_id = ISP_BLK_DUAL_FLASH;
		break;
	case ISP_PM_CMD_GET_AE_SYNC:
		block_id = ISP_BLK_AE_SYNC;
		break;
	case ISP_PM_CMD_GET_4IN1_PARAM:
		block_id = ISP_BLK_4IN1_PARAM;
		break;
	case ISP_PM_CMD_GET_INIT_PDAF:
		block_id = ISP_BLK_PDAF_TUNE;
		break;
	case ISP_PM_CMD_GET_INIT_TOF:
		block_id = ISP_BLK_TOF_TUNE;
		break;
	case ISP_PM_CMD_GET_ATM_PARAM:
		block_id = ISP_BLK_ATM_TUNE;
		break;
	default:
		break;
	}

	if (block_id != 0) {

		param_data_ptr = pm_cxt_ptr->temp_param_data_ptr[0];
		rtn = isp_pm_get_mode_block_param(pm_cxt_ptr, cmd, param_data_ptr,
			&param_counts, block_id);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGV("fail to do isp_pm_get_mode_block_param");
			rtn = ISP_ERROR;
			return rtn;
		}

		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_data = pm_cxt_ptr->temp_param_data_ptr[0];
		result_ptr->param_num = param_counts;
	}

	return rtn;
}

static cmr_s32 isp_pm_mode_common_to_other
	(struct isp_pm_mode_param *mode_common_in, struct isp_pm_mode_param *mode_other_list_out)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0, j = 0, temp_block_num = 0;
	cmr_u32 common_block_num = 0, other_block_num = 0;

	if (PNULL == mode_common_in || PNULL == mode_other_list_out) {
		ISP_LOGE("fail to get valid param : mode_common_in = %p, mode_other_list_out = %p",
			mode_common_in, mode_other_list_out);
		rtn = ISP_ERROR;
		return rtn;
	}

	common_block_num = mode_common_in->block_num;
	other_block_num = mode_other_list_out->block_num;

	if (common_block_num < other_block_num) {
		ISP_LOGE("fail to get valid param : common_block_num = %d, other_block_num = %d",
			common_block_num, other_block_num);
		rtn = ISP_ERROR;
		return rtn;
	}

	temp_block_num = other_block_num;
	for (i = 0; i < common_block_num; i++) {
		for (j = 0; j < other_block_num; j++) {
			if (mode_common_in->header[i].block_id == mode_other_list_out->header[j].block_id) {
				mode_other_list_out->header[j].source_flag = mode_other_list_out->mode_id;
				mode_other_list_out->header[j].mode_id = mode_other_list_out->mode_id;
				break;
			}
		}
		if (j == other_block_num) {
			memcpy((cmr_u8 *)&mode_other_list_out->header[temp_block_num],
				(cmr_u8 *)&mode_common_in->header[i], sizeof(struct isp_pm_block_header));
			mode_other_list_out->header[temp_block_num].source_flag = mode_common_in->mode_id;
			mode_other_list_out->header[temp_block_num].mode_id = mode_other_list_out->mode_id;
			temp_block_num++;
		}
	}

	mode_other_list_out->block_num = temp_block_num;

	return rtn;
}

static cmr_s32 isp_pm_param_list_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	cmr_u32 i = 0;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to get valid pm_cxt_ptr");
		rtn = ISP_ERROR;
		return rtn;
	}

	for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
		if (pm_cxt_ptr->tune_mode_array[i]) {
			free(pm_cxt_ptr->tune_mode_array[i]);
			pm_cxt_ptr->tune_mode_array[i] = PNULL;
		}
		if (pm_cxt_ptr->merged_mode_array[i]) {
			free(pm_cxt_ptr->merged_mode_array[i]);
			pm_cxt_ptr->merged_mode_array[i] = PNULL;
		}
	}

	if (PNULL != pm_cxt_ptr->temp_param_data_ptr[0]) {
		free(pm_cxt_ptr->temp_param_data_ptr[0]);
		pm_cxt_ptr->temp_param_data_ptr[0] = PNULL;
	}

	if (PNULL != pm_cxt_ptr->temp_param_data_ptr[1]) {
		free(pm_cxt_ptr->temp_param_data_ptr[1]);
		pm_cxt_ptr->temp_param_data_ptr[1] = PNULL;
	}

	ISP_LOGV("isp_pm_param_list_deinit : done");

	return rtn;
}

static cmr_s32 isp_pm_param_list_init(cmr_handle handle,
	struct isp_pm_init_input *input, struct isp_pm_init_output *output)
{
	cmr_s32 rtn = ISP_SUCCESS;

	cmr_u32 i = 0, j = 0, k = 0;
	cmr_u32 max_num = 0;

	cmr_u32 extend_offset = 0;
	cmr_u32 data_area_size = 0;
	cmr_u32 size = 0;
	cmr_u32 add_ae_len = 0, add_awb_len = 0, add_lnc_len = 0;

	struct isp_mode_param *src_mod_ptr = PNULL;
	struct isp_pm_mode_param *dst_mod_ptr = PNULL;
	struct isp_block_header *src_header = PNULL;
	struct isp_pm_block_header *dst_header = PNULL;
	cmr_u8 *src_data_ptr = PNULL;
	cmr_u8 *dst_data_ptr = PNULL;

	struct sensor_raw_fix_info *fix_data_ptr = PNULL;
	struct sensor_nr_fix_info *nr_fix_ptr = PNULL;
	struct sensor_nr_scene_map_param *nr_scene_map_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_level_number_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_default_level_ptr = PNULL;
	struct nr_set_group_unit *nr_ptr = PNULL;
	struct isp_pm_nr_header_param *dst_nlm_data = PNULL;
	struct isp_pm_nr_simple_header_param *dst_blk_data = PNULL;
	cmr_u32 multi_nr_flag = 0;
	cmr_u32 isp_blk_nr_type = ISP_BLK_TYPE_MAX;
	intptr_t nr_set_addr = 0;
	cmr_u32 nr_set_size = 0;

	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if ((PNULL == pm_cxt_ptr) || (PNULL == input)) {
		ISP_LOGE("fail to get valid param : pm_cxt_ptr = %p, input = %p",
			pm_cxt_ptr, input);
		rtn = ISP_ERROR;
		return rtn;
	}

	multi_nr_flag = SENSOR_MULTI_MODE_FLAG;//SENSOR_DEFAULT_MODE_FLAG
	if (output)
		output->multi_nr_flag = multi_nr_flag;

	pm_cxt_ptr->is_4in1_sensor = input->is_4in1_sensor;
	ISP_LOGV("is_4in1_sensor = %d", pm_cxt_ptr->is_4in1_sensor);

	nr_fix_ptr = input->nr_fix_info;
	nr_scene_map_ptr = (struct sensor_nr_scene_map_param *)(nr_fix_ptr->nr_scene_ptr);
	nr_level_number_ptr = (struct sensor_nr_level_map_param *)(nr_fix_ptr->nr_level_number_ptr);
	nr_default_level_ptr = (struct sensor_nr_level_map_param *)(nr_fix_ptr->nr_default_level_ptr);

	for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
		extend_offset = 0;

		src_mod_ptr = (struct isp_mode_param *)input->tuning_data[i].data_ptr;
		if (PNULL == src_mod_ptr)
			continue;
		src_header = (struct isp_block_header *)src_mod_ptr->block_header;

		data_area_size = src_mod_ptr->size - sizeof(struct isp_mode_param);
		size = data_area_size + sizeof(struct isp_pm_mode_param);

		fix_data_ptr = input->fix_data[i];
		add_ae_len = fix_data_ptr->ae.ae_param.ae_len;
		add_lnc_len = fix_data_ptr->lnc.lnc_param.lnc_len;
		add_awb_len = fix_data_ptr->awb.awb_param.awb_len;
		size += add_ae_len + add_lnc_len + add_awb_len;

		nr_ptr = (struct nr_set_group_unit *)&(fix_data_ptr->nr.nr_set_group);
		for (k = 0; k < sizeof(struct sensor_nr_set_group_param) / sizeof(struct nr_set_group_unit); k++) {
			if (PNULL != nr_ptr[k].nr_ptr) {
				size += sizeof(struct isp_pm_nr_simple_header_param);
			}
		}

		if (PNULL != pm_cxt_ptr->tune_mode_array[i]) {
			free(pm_cxt_ptr->tune_mode_array[i]);
			pm_cxt_ptr->tune_mode_array[i] = PNULL;
		}
		pm_cxt_ptr->tune_mode_array[i] = (struct isp_pm_mode_param *)malloc(size);
		if (PNULL == pm_cxt_ptr->tune_mode_array[i]) {
			ISP_LOGE("fail to malloc tune_mode_array : i = %d", i);
			rtn = ISP_ERROR;
			goto init_param_list_error_exit;
		}
		memset((void *)pm_cxt_ptr->tune_mode_array[i], 0x00, size);

		dst_mod_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[i];
		dst_header = (struct isp_pm_block_header *)dst_mod_ptr->header;

		for (j = 0; j < src_mod_ptr->block_num; j++) {
			dst_header[j].is_update = 0;
			dst_header[j].source_flag = i;
			dst_header[j].bypass = src_header[j].bypass;
			dst_header[j].block_id = src_header[j].block_id;
			dst_header[j].param_id = src_header[j].param_id;
			dst_header[j].version_id = src_header[j].version_id;
			dst_header[j].size = src_header[j].size;

			size = src_header[j].offset - sizeof(struct isp_mode_param);
			size = size + sizeof(struct isp_pm_mode_param);

			src_data_ptr = (cmr_u8 *)((intptr_t)src_mod_ptr + src_header[j].offset);
			dst_data_ptr = (cmr_u8 *)((intptr_t)dst_mod_ptr + size + extend_offset);

			dst_header[j].absolute_addr = (void *)dst_data_ptr;
			dst_header[j].mode_id = i;

			memcpy((void *)dst_data_ptr, (void *)src_data_ptr, src_header[j].size);
			memcpy((void *)dst_header[j].name, (void *)src_header[j].block_name, sizeof(dst_header[j].name));

			switch (src_header[j].block_id) {
			case ISP_BLK_2D_LSC:
			{
				extend_offset += add_lnc_len;
				dst_header[j].size = src_header[j].size + add_lnc_len;
				memcpy((void *)(dst_data_ptr + src_header[j].size),
					(void *)(fix_data_ptr->lnc.lnc_param.lnc), add_lnc_len);
				break;
			}
			case ISP_BLK_AE_NEW:
			{
				extend_offset += add_ae_len;
				dst_header[j].size = src_header[j].size + add_ae_len;
				memcpy((void *)(dst_data_ptr + sizeof(struct ae_param_tmp_001)),
					(void *)(fix_data_ptr->ae.ae_param.ae), add_ae_len);
				memcpy((void *)(dst_data_ptr + sizeof(struct ae_param_tmp_001) + add_ae_len),
					(void *)(src_data_ptr + sizeof(struct ae_param_tmp_001)),
					(src_header[j].size - sizeof(struct ae_param_tmp_001)));
				break;
			}
			case ISP_BLK_AWB_NEW:
			{
				break;
			}
			case DCAM_BLK_NLM:
			{
				dst_nlm_data = (struct isp_pm_nr_header_param *)dst_data_ptr;
				memset((void *)dst_nlm_data, 0x00, sizeof(struct isp_pm_nr_header_param));

				dst_nlm_data->level_number = nr_level_number_ptr->nr_level_map[ISP_BLK_NLM_T];
				dst_nlm_data->default_strength_level = nr_default_level_ptr->nr_level_map[ISP_BLK_NLM_T];
				dst_nlm_data->nr_mode_setting = multi_nr_flag;
				dst_nlm_data->multi_nr_map_ptr = (cmr_uint *)&(nr_scene_map_ptr->nr_scene_map[0]);
				dst_nlm_data->param_ptr = (cmr_uint *)fix_data_ptr->nr.nr_set_group.nlm;
				dst_nlm_data->param1_ptr = (cmr_uint *)fix_data_ptr->nr.nr_set_group.vst;
				dst_nlm_data->param2_ptr = (cmr_uint *)fix_data_ptr->nr.nr_set_group.ivst;

				extend_offset += 3 * sizeof(struct isp_pm_nr_simple_header_param);
				dst_header[j].size = 3 * sizeof(struct isp_pm_nr_simple_header_param);
				break;
			}
			case ISP_BLK_RGB_DITHER:
			{
				isp_blk_nr_type = ISP_BLK_RGB_DITHER_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.rgb_dither);
				nr_set_size = sizeof(struct sensor_rgb_dither_level);
				break;
			}
			case DCAM_BLK_BPC:
			{
				isp_blk_nr_type = ISP_BLK_BPC_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.bpc);
				nr_set_size = sizeof(struct sensor_bpc_level);
				break;
			}
			case ISP_BLK_GRGB:
			{
				isp_blk_nr_type = ISP_BLK_GRGB_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.grgb);
				nr_set_size = sizeof(struct sensor_grgb_level);
				break;
			}
			case ISP_BLK_CFA:
			{
				isp_blk_nr_type = ISP_BLK_CFA_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.cfa);
				nr_set_size = sizeof(struct sensor_cfa_param_level);
				break;
			}
			case DCAM_BLK_RGB_AFM:
			{
				isp_blk_nr_type = ISP_BLK_RGB_AFM_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.rgb_afm);
				nr_set_size = sizeof(struct sensor_rgb_afm_level);
				break;
			}
			case ISP_BLK_UVDIV:
			{
				isp_blk_nr_type = ISP_BLK_UVDIV_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.uvdiv);
				nr_set_size = sizeof(struct sensor_cce_uvdiv_level);
				break;
			}
			case DCAM_BLK_3DNR_PRE:
			{
				isp_blk_nr_type = ISP_BLK_3DNR_PRE_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.nr3d_pre);
				nr_set_size = sizeof(struct sensor_3dnr_level);
				break;
			}
			case DCAM_BLK_3DNR_CAP:
			{
				isp_blk_nr_type = ISP_BLK_3DNR_CAP_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.nr3d_cap);
				nr_set_size = sizeof(struct sensor_3dnr_level);
				break;
			}
			case ISP_BLK_YUV_PRECDN:
			{
				isp_blk_nr_type = ISP_BLK_YUV_PRECDN_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.yuv_precdn);
				nr_set_size = sizeof(struct sensor_yuv_precdn_level);
				break;
			}
			case ISP_BLK_YNR:
			{
				isp_blk_nr_type = ISP_BLK_YNR_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.ynr);
				nr_set_size = sizeof(struct sensor_ynr_level);
				break;
			}
			case ISP_BLK_EDGE:
			{
				isp_blk_nr_type = ISP_BLK_EDGE_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.edge);
				nr_set_size = sizeof(struct sensor_ee_level);
				break;
			}
			case ISP_BLK_UV_CDN:
			{
				isp_blk_nr_type = ISP_BLK_CDN_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.cdn);
				nr_set_size = sizeof(struct sensor_uv_cdn_level);
				break;
			}
			case ISP_BLK_UV_POSTCDN:
			{
				isp_blk_nr_type = ISP_BLK_POSTCDN_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.postcdn);
				nr_set_size = sizeof(struct sensor_uv_postcdn_level);
				break;
			}
			case ISP_BLK_IIRCNR_IIR:
			{
				isp_blk_nr_type = ISP_BLK_IIRCNR_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.iircnr);
				nr_set_size = sizeof(struct sensor_iircnr_level);
				break;
			}
			case ISP_BLK_YUV_NOISEFILTER:
			{
				isp_blk_nr_type = ISP_BLK_YUV_NOISEFILTER_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.yuv_noisefilter);
				nr_set_size = sizeof(struct sensor_yuv_noisefilter_level);
				break;
			}
			case ISP_BLK_CNR2:
			{
				isp_blk_nr_type = ISP_BLK_CNR2_T;
				nr_set_addr = (intptr_t)(fix_data_ptr->nr.nr_set_group.cnr2);
				nr_set_size = sizeof(struct sensor_cnr_level);
				break;
			}
			default:
				break;
			}

			if (src_header[j].block_id == ISP_BLK_RGB_DITHER
				|| src_header[j].block_id == DCAM_BLK_BPC
				|| src_header[j].block_id == ISP_BLK_GRGB
				|| src_header[j].block_id == ISP_BLK_CFA
				|| src_header[j].block_id == DCAM_BLK_RGB_AFM
				|| src_header[j].block_id == ISP_BLK_UVDIV
				|| src_header[j].block_id == DCAM_BLK_3DNR_PRE
				|| src_header[j].block_id == DCAM_BLK_3DNR_CAP
				|| src_header[j].block_id == ISP_BLK_YUV_PRECDN
				|| src_header[j].block_id == ISP_BLK_YNR
				|| src_header[j].block_id == ISP_BLK_EDGE
				|| src_header[j].block_id == ISP_BLK_UV_CDN
				|| src_header[j].block_id == ISP_BLK_UV_POSTCDN
				|| src_header[j].block_id == ISP_BLK_IIRCNR_IIR
				|| src_header[j].block_id == ISP_BLK_YUV_NOISEFILTER
				|| src_header[j].block_id == ISP_BLK_CNR2) {

				dst_blk_data = (struct isp_pm_nr_simple_header_param *)dst_data_ptr;
				memset((void *)dst_blk_data, 0x00, sizeof(struct isp_pm_nr_simple_header_param));

				dst_blk_data->level_number = nr_level_number_ptr->nr_level_map[isp_blk_nr_type];
				dst_blk_data->default_strength_level = nr_default_level_ptr->nr_level_map[isp_blk_nr_type];
				dst_blk_data->nr_mode_setting = multi_nr_flag;
				dst_blk_data->multi_nr_map_ptr = (cmr_uint *)&(nr_scene_map_ptr->nr_scene_map[0]);
				dst_blk_data->param_ptr = (cmr_uint *)nr_set_addr;

				extend_offset += sizeof(struct isp_pm_nr_simple_header_param);
				dst_header[j].size = sizeof(struct isp_pm_nr_simple_header_param);
			}
		}

		if (max_num < src_mod_ptr->block_num)
			max_num = src_mod_ptr->block_num;

		dst_mod_ptr->block_num = src_mod_ptr->block_num;
		dst_mod_ptr->mode_id = src_mod_ptr->mode_id;
		dst_mod_ptr->resolution.w = src_mod_ptr->width;
		dst_mod_ptr->resolution.h = src_mod_ptr->height;
		dst_mod_ptr->fps = src_mod_ptr->fps;

		memcpy((void *)dst_mod_ptr->mode_name,
			(void *)src_mod_ptr->mode_name, sizeof(src_mod_ptr->mode_name));

		ISP_LOGV("mode = %d, mode_name = %s, param modify_time : %d",
			i, src_mod_ptr->mode_name, src_mod_ptr->reserved[0]);

		if (PNULL != pm_cxt_ptr->merged_mode_array[i]) {
			free(pm_cxt_ptr->merged_mode_array[i]);
			pm_cxt_ptr->merged_mode_array[i] = PNULL;
		}

		pm_cxt_ptr->merged_mode_array[i] =
			(struct isp_pm_mode_param *)malloc(sizeof(struct isp_pm_mode_param));
		if (PNULL == pm_cxt_ptr->merged_mode_array[i]) {
			ISP_LOGE("fail to malloc merged_mode_array : i = %d", i);
			rtn = ISP_ERROR;
			goto init_param_list_error_exit;
		}
		memset((void *)pm_cxt_ptr->merged_mode_array[i], 0x00, sizeof(struct isp_pm_mode_param));
	}

	for (i = ISP_MODE_ID_COMMON; i < ISP_TUNE_MODE_MAX; i++) {
		if (PNULL == (cmr_u8 *)pm_cxt_ptr->tune_mode_array[i]) {
			continue;
		}
		memcpy((cmr_u8 *)pm_cxt_ptr->merged_mode_array[i],
			(cmr_u8 *)pm_cxt_ptr->tune_mode_array[i], sizeof(struct isp_pm_mode_param));

		if (0 == pm_cxt_ptr->param_source) {
			memset((void *)&pm_cxt_ptr->cxt_array[i], 0x00, sizeof(pm_cxt_ptr->cxt_array[i]));
		}

	}

	for (i = ISP_MODE_ID_PRV_0; i < ISP_TUNE_MODE_MAX; i++) {
		if (PNULL == pm_cxt_ptr->merged_mode_array[i]) {
			continue;
		}
		rtn = isp_pm_mode_common_to_other(pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_COMMON],
			pm_cxt_ptr->merged_mode_array[i]);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to do isp_pm_mode_common_to_other");
			rtn = ISP_ERROR;
			goto init_param_list_error_exit;
		}
	}

	size = max_num * ISP_TUNE_MODE_MAX * sizeof(struct isp_pm_param_data);

	pm_cxt_ptr->temp_param_data_ptr[0] = (struct isp_pm_param_data *)malloc(size);
	if (PNULL == pm_cxt_ptr->temp_param_data_ptr[0]) {
		ISP_LOGE("fail to malloc temp_param_data_ptr[0]");
		rtn = ISP_ERROR;
		goto init_param_list_error_exit;
	}
	memset((void *)pm_cxt_ptr->temp_param_data_ptr[0], 0x00, size);

	pm_cxt_ptr->temp_param_data_ptr[1] = (struct isp_pm_param_data *)malloc(size);
	if (PNULL == pm_cxt_ptr->temp_param_data_ptr[1]) {
		ISP_LOGE("fail to malloc temp_param_data_ptr[1]");
		rtn = ISP_ERROR;
		goto init_param_list_error_exit;
	}
	memset((void *)pm_cxt_ptr->temp_param_data_ptr[1], 0x00, size);

	if (NULL != pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_PRV_0])
		pm_cxt_ptr->active_mode[ISP_SCENE_PRV] =
			(struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_PRV_0];
	if (NULL != pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_0])
		pm_cxt_ptr->active_mode[ISP_SCENE_CAP] =
			(struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_0];

	//for 4in1
	if (input->is_4in1_sensor) {
		if (NULL != pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_1])
			pm_cxt_ptr->active_mode[ISP_SCENE_LOWLIGHT_CAP] =
				(struct isp_pm_mode_param *)pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_1];
	}

	if (NULL != pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_PRV_0])
		pm_cxt_ptr->cxt_array[ISP_SCENE_PRV].mode_id =
			pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_PRV_0]->mode_id;
	if (NULL != pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_0])
		pm_cxt_ptr->cxt_array[ISP_SCENE_CAP].mode_id =
			pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_0]->mode_id;

	//for 4in1
	if (input->is_4in1_sensor) {
		if (NULL != pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_1])
			pm_cxt_ptr->cxt_array[ISP_SCENE_LOWLIGHT_CAP].mode_id =
				pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_CAP_1]->mode_id;
	}

	rtn = isp_pm_context_init((cmr_handle)pm_cxt_ptr, ISP_MODE_ID_PRV_0, ISP_SCENE_PRV);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_context_init");
		rtn = ISP_ERROR;
		goto init_pm_context_error_exit;
	}
	rtn = isp_pm_context_init((cmr_handle)pm_cxt_ptr, ISP_MODE_ID_CAP_0, ISP_SCENE_CAP);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_context_init");
		rtn = ISP_ERROR;
		goto init_pm_context_error_exit;
	}

	//for 4in1
	if (input->is_4in1_sensor) {
		rtn = isp_pm_context_init((cmr_handle)pm_cxt_ptr, ISP_MODE_ID_CAP_1, ISP_SCENE_LOWLIGHT_CAP);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to do isp_pm_context_init");
			rtn = ISP_ERROR;
			goto init_pm_context_error_exit;
		}
	}

	ISP_LOGV("isp_pm_param_list_init : done");

	return rtn;

init_pm_context_error_exit:
	isp_pm_context_deinit((cmr_handle)pm_cxt_ptr);

init_param_list_error_exit:

	isp_pm_param_list_deinit((cmr_handle)pm_cxt_ptr);

	return rtn;
}

cmr_handle isp_pm_init(struct isp_pm_init_input *input, struct isp_pm_init_output *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = PNULL;

	if ((PNULL == input) || (PNULL == output)) {
		ISP_LOGE("fail to get valid param : input = %p, output = %p", input, output);
		goto init_error_exit;
	}

	pm_cxt_ptr = (struct isp_pm_context *)malloc(sizeof(struct isp_pm_context));
	if (PNULL == pm_cxt_ptr) {
		ISP_LOGE("fail to malloc pm_cxt_ptr");
		goto init_error_exit;
	}
	memset((void *)pm_cxt_ptr, 0x00, sizeof(struct isp_pm_context));

	pm_cxt_ptr->magic_flag = ISP_PM_MAGIC_FLAG;

	pthread_mutex_init(&pm_cxt_ptr->pm_mutex, NULL);

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		goto init_error_exit;
	}

	rtn = isp_pm_param_list_init((cmr_handle)pm_cxt_ptr, input, output);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to isp_pm_param_list_init");
		goto init_error_exit;
	}

	ISP_LOGI("isp_pm_init : done");

	return (cmr_handle)pm_cxt_ptr;

init_error_exit:

	if (PNULL != pm_cxt_ptr) {
		pthread_mutex_destroy(&pm_cxt_ptr->pm_mutex);
		free(pm_cxt_ptr);
		pm_cxt_ptr = PNULL;
	}
	return PNULL;
}

cmr_s32 isp_pm_ioctl(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		return rtn;
	}

	switch ((cmd & isp_pm_cmd_mask)) {
	case ISP_PM_CMD_SET_BASE:
		pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
		rtn = isp_pm_set_param((cmr_handle)pm_cxt_ptr, cmd, input);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGV("fail to do isp_pm_set_param : cmd = 0x%x", cmd);
			pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
			return rtn;
		}
		pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
		break;
	case ISP_PM_CMD_GET_BASE:
	case ISP_PM_CMD_GET_THIRD_PART_BASE:
		if (pm_cxt_ptr->param_source) {
			ISP_LOGV("fail to get valid param, isp tool is writing param");
			return ISP_ERROR;
		}
		pthread_mutex_lock(&pm_cxt_ptr->pm_mutex);
		rtn = isp_pm_get_param((cmr_handle)pm_cxt_ptr, cmd, input, output);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGV("fail to do isp_pm_get_param : cmd = 0x%x", cmd);
			pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
			return rtn;
		}
		pthread_mutex_unlock(&pm_cxt_ptr->pm_mutex);
		break;
	default:
		break;
	}

	return rtn;
}

cmr_s32 isp_pm_update(cmr_handle handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == input) {
		ISP_LOGE("fail to get valid input param");
		rtn = ISP_ERROR;
		return rtn;
	}

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		return rtn;
	}

	if (ISP_PM_CMD_UPDATE_ALL_PARAMS == cmd) {
		rtn = isp_pm_param_list_init((cmr_handle)pm_cxt_ptr, input, output);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("fail to do isp_pm_param_list_init");
			return rtn;
		}
	}

	return rtn;
}

cmr_s32 isp_pm_deinit(cmr_handle handle)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	rtn = isp_pm_check_handle((cmr_handle)pm_cxt_ptr);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("fail to do isp_pm_check_handle");
		rtn = ISP_ERROR;
		return rtn;
	}

	if (PNULL != pm_cxt_ptr) {
		pthread_mutex_destroy(&pm_cxt_ptr->pm_mutex);
		isp_pm_context_deinit((cmr_handle)pm_cxt_ptr);
		isp_pm_param_list_deinit((cmr_handle)pm_cxt_ptr);
		free(pm_cxt_ptr);
		pm_cxt_ptr = PNULL;
	}

	ISP_LOGI("isp_pm_deinit : done");

	return rtn;
}
