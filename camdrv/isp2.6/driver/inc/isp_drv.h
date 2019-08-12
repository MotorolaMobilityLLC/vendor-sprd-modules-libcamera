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
#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "sprd_isp_k.h"
#include "sprd_img.h"
#include "cmr_types.h"
#include "sensor_raw.h"
#include "isp_mw.h"

#ifndef LOCAL_INCLUDE_ONLY
#error "Hi, This is only for camdrv."
#endif

struct isp_file {
	cmr_s32 fd;
	cmr_u32 chip_id;
	cmr_u32 isp_id;
	void *reserved;
};

struct isp_u_blocks_info {
	cmr_u32 scene_id;
	void *block_info;
};

struct isp_statis_info {
	cmr_u32 buf_type;
	cmr_u32 buf_size;
	cmr_u32 hw_addr;
	cmr_uint uaddr;
	cmr_u64 kaddr;
	cmr_u32 sec;
	cmr_u32 usec;
	cmr_u32 frame_id;
	cmr_u32 zoom_ratio;
	cmr_u32 width;
	cmr_u32 height;
};

struct isp_mem_info {
	cmr_u32 statis_alloc_flag;
	cmr_u32 statis_mem_size;
	cmr_u32 statis_mem_num;
	cmr_s32 statis_mfd;
	cmr_u64 statis_k_addr;
	cmr_uint statis_phys_addr;
	cmr_uint statis_u_addr;

	void *buffer_client_data;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	cmr_handle oem_handle;
};

cmr_s32 isp_dev_open(cmr_s32 fd, cmr_handle * handle);
cmr_s32 isp_dev_close(cmr_handle handle);
cmr_s32 isp_dev_reset(cmr_handle handle);
cmr_s32 isp_dev_get_video_size(
			cmr_handle handle, cmr_u32 *width, cmr_u32 *height);
cmr_s32 isp_dev_get_ktime(
			cmr_handle handle, cmr_u32 *width, cmr_u32 *height);
cmr_s32 isp_dev_set_statis_buf(cmr_handle handle,
			struct isp_statis_buf_input *param);
cmr_s32 isp_dev_raw_proc(cmr_handle handle, void *param_ptr);
cmr_s32 isp_cfg_block(cmr_handle handle, void *param_ptr, cmr_u32 sub_block);
cmr_s32 isp_dev_cfg_start(cmr_handle handle);


cmr_s32 dcam_u_aem_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 dcam_u_aem_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 dcam_u_aem_skip_num(cmr_handle handle, cmr_u32 skip_num);
cmr_s32 dcam_u_aem_win(cmr_handle handle, void *aem_win);
cmr_s32 dcam_u_aem_rgb_thr(cmr_handle handle, void *rgb_thr);

cmr_s32 dcam_u_afm_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_afm_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 dcam_u_afm_mode(cmr_handle handle, cmr_u32 mode);
cmr_s32 dcam_u_afm_skip_num(cmr_handle handle, cmr_u32 skip_num);
cmr_s32 dcam_u_afm_win(cmr_handle handle, void *win_range);
cmr_s32 dcam_u_afm_win_num(cmr_handle handle, void *win_num);
cmr_s32 dcam_u_afm_crop_eb(cmr_handle handle, cmr_u32 crop_eb);
cmr_s32 dcam_u_afm_crop_size(cmr_handle handle, void *crop_size);
cmr_s32 dcam_u_afm_done_tilenum(cmr_handle handle, void *done_tile_num);

cmr_s32 dcam_u_afl_new_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 dcam_u_afl_new_block(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_bayerhist_block(cmr_handle handle, void *bayerhist);

cmr_s32 dcam_u_awbc_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_awbc_bypass(cmr_handle handle, cmr_u32 bypass, cmr_u32 scene_id);
cmr_s32 dcam_u_awbc_bypass(cmr_handle handle, cmr_u32 bypass, cmr_u32 scene_id);

cmr_s32 dcam_u_blc_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_rgb_gain_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_rgb_dither_block(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_lsc_block(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_awbc_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_awbc_bypass(cmr_handle handle, cmr_u32 bypass, cmr_u32 scene_id);
cmr_s32 dcam_u_awbc_gain(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_bpc_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_bpc_ppe(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_pdaf_bypass(cmr_handle handle, cmr_u32 *bypass);
cmr_s32 dcam_u_pdaf_work_mode(cmr_handle handle, cmr_u32 *work_mode);
cmr_s32 dcam_u_pdaf_skip_num(cmr_handle handle, cmr_u32 *skip_num);
cmr_s32 dcam_u_pdaf_roi(cmr_handle handle, void *roi_info);
cmr_s32 dcam_u_pdaf_ppi_info(cmr_handle handle, void *ppi_info);
cmr_s32 dcam_u_pdaf_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_pdaf_type1_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_pdaf_type2_block(cmr_handle handle, void *block_info);
cmr_s32 dcam_u_pdaf_type3_block(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_dual_pdaf_block(cmr_handle handle, void *block_info);

cmr_s32 dcam_u_grgb_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_bchs_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_brightness_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_contrast_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_hue_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_csa_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_cce_matrix_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_cfa_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_cmc_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_edge_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_gamma_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_grgb_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_hist_block(void *handle, void *block_info);
cmr_s32 isp_u_hist2_block(void *handle, void *block_info);
cmr_s32 isp_u_hsv_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_iircnr_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yrandom_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ltm_block(void *handle, void *block_info);
cmr_s32 isp_u_3dnr_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_nlm_block(void *handle, void *block_info);
cmr_s32 isp_u_nlm_imblance(void *handle, void *block_info);
cmr_s32 isp_u_yuv_precdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_postcdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_cdn_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_posterize_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_uvd_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ygamma_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_ynr_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_noisefilter_block(cmr_handle handle, void *block_info);
#endif
