/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "isp_u_raw_afm"

#include "isp_drv.h"

cmr_s32 isp_u_raw_afm_block(cmr_handle handle, void *block_info)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_u_blocks_info *block_param = (struct isp_u_blocks_info *)block_info;
	struct isp_dev_rgb_afm_info *rafm_info =
		(struct isp_dev_rgb_afm_info *)block_param->block_info;

	if (!handle || !block_info) {
		ISP_LOGE("fail to get handle: handle = %p, block_info = %p.", handle, block_info);
		return -1;
	}

	/*temp param*/
	rafm_info->crop_eb = 1;
	rafm_info->source_sel = 1;
	rafm_info->lum_stat_chn_sel = 1;
	rafm_info->iir_eb = 1;
	rafm_info->clk_gate_dis = 0;
	rafm_info->done_title_num.x = 3;
	rafm_info->done_title_num.y = 2;
	rafm_info->crop_size.x = 0x20;
	rafm_info->crop_size.y = 0x20;
	rafm_info->crop_size.w = 640 - 64;
	rafm_info->crop_size.h = 480 - 64;
	rafm_info->win.x = 4;
	rafm_info->win.y = 4;
	rafm_info->win.w = 32;
	rafm_info->win.h = 32;
	rafm_info->win_num.x = 4;
	rafm_info->win_num.y = 3;

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = block_param->scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_BLOCK;
	param.property_param = rafm_info;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_slice_size(cmr_handle handle, cmr_u32 width, cmr_u32 height, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_FRAME_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_iir_nr_cfg(cmr_handle handle, void *afm_iir_nr, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !afm_iir_nr) {
		ISP_LOGE("fail to get handle: handle = %p, afm_iir_nr = %p.", handle, afm_iir_nr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_IIR_NR_CFG;
	param.property_param = afm_iir_nr;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_modules_cfg(cmr_handle handle, void *afm_modules, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !afm_modules) {
		ISP_LOGE("fail to get handle: handle = %p, afm_modules = %p.", handle, afm_modules);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_MODULE_CFG;
	param.property_param = afm_modules;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_bypass(cmr_handle handle, cmr_u32 bypass, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);
	return ret;
}

cmr_s32 isp_u_raw_afm_mode(cmr_handle handle, cmr_u32 mode, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_skip_num(cmr_handle handle, cmr_u32 skip_num, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_skip_num_clr(cmr_handle handle, cmr_u32 clear, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM_CLR;
	param.property_param = &clear;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_win(cmr_handle handle, void *win_range, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_rect win;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	win.x = 4;
	win.y = 4;
	win.w = 32;
	win.h = 32;

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_WIN;
	param.property_param = win_range;
	param.property_param = &win;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}

cmr_s32 isp_u_raw_afm_win_num(cmr_handle handle, cmr_u32 * win_num, cmr_u32 scene_id)
{
	cmr_s32 ret = 0;

	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct img_offset winnum;

	if (!handle) {
		ISP_LOGE("fail to get handle.");
		return -1;
	}

	winnum.x = 4;
	winnum.y = 3;

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.scene_id = scene_id;
	param.sub_block = ISP_BLOCK_RAW_AFM;
	param.property = ISP_PRO_RGB_AFM_WIN_NUM;
	param.property_param = win_num;
	param.property_param = &winnum;

	ret = ioctl(file->fd, SPRD_ISP_IO_CFG_PARAM, &param);

	return ret;
}
