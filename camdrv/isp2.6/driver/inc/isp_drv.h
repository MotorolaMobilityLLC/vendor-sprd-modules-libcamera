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


/* alloc more bytes for protecting memory from HW accessing out of range */
#define STATIS_TAIL_RESERVED_SIZE  1024

/* AFM max windows: 20 x 15, 16 Bytes for each window */
#define STATIS_AFM_BUF_SIZE  (20 * 15 * 16)


/* SharkL5/ROC1/SharkL5Pro */
/* AFL: global 80 x 16 bytes for one frame, region 482 x 16 bytes one frame */
//#define STATIS_AFL_GBUF_SIZE   (80 * 16 * 3 + 64)
#define STATIS_AFL_RBUF_SIZE   (482 * 16 * 3 + 64)

/* SharkL3 */
/* AFL: global 240 * 8 bytes for one frame, region 964 x 8 bytes one frame */
//#define STATIS_AFL_GBUF_SIZE3   (240 * 8 * 3 + 64)


/* bayerhist:  154 x 16 bytes */
#define STATIS_HIST_BUF_SIZE   (154 * 16)


/* isp yuv hist (only Y) */
#define STATIS_ISP_HIST2_BUF_SIZE   (256 * 4)


#ifdef CONFIG_ISP_2_7 /* for SharkL5Pro */

#define CAM_RAW_BITS (14)

#define STATIS_AEM_BUF_NUM 4
#define STATIS_AFM_BUF_NUM 4
#define STATIS_AFL_BUF_NUM 3
#define STATIS_3DNR_BUF_NUM 6
#define STATIS_PDAF_BUF_NUM 6
#define STATIS_EBD_BUF_NUM 4
#define STATIS_HIST_BUF_NUM   4
#define STATIS_LSCM_BUF_NUM   4
#define STATIS_ISP_HIST2_BUF_NUM 4

#define STATIS_AFL_SIZE  (STATIS_AFL_RBUF_SIZE + STATIS_AFL_GBUF_SIZE)
#define AFL_RBUF_OFFSET	STATIS_AFL_GBUF_SIZE

#define PM0_SIZE	(sizeof(struct dcam_param_data_l5pro) + sizeof(struct debug_base_info))
#define DCAM_PARAM_SIZE  (cmr_u32)((PM0_SIZE + 15) & ~15)
#define PM1_SIZE 	(sizeof(struct isp_param_data_l5pro) + sizeof(struct debug_base_info))
#define ISP_PARAM_SIZE	(cmr_u32)((PM1_SIZE + 15) & ~15)

#elif defined CONFIG_ISP_2_8 /* for SharkL6 */

#define CAM_RAW_BITS (14)

#define STATIS_AEM_BUF_NUM 4
#define STATIS_AFM_BUF_NUM 4
#define STATIS_AFL_BUF_NUM 3
#define STATIS_3DNR_BUF_NUM 6
#define STATIS_PDAF_BUF_NUM 6
#define STATIS_EBD_BUF_NUM 4
#define STATIS_HIST_BUF_NUM   4
#define STATIS_LSCM_BUF_NUM   4
#define STATIS_ISP_HIST2_BUF_NUM 4

#define STATIS_AFL_SIZE  (STATIS_AFL_RBUF_SIZE + STATIS_AFL_GBUF_SIZE)
#define AFL_RBUF_OFFSET	STATIS_AFL_GBUF_SIZE

#define DCAM_PARAM_SIZE  0
#define ISP_PARAM_SIZE  0

#elif defined CONFIG_ISP_2_5 /* for SharkL3 */

#define CAM_RAW_BITS (10)

#define STATIS_AEM_BUF_NUM 4
#define STATIS_AFM_BUF_NUM 4
#define STATIS_AFL_BUF_NUM 3
#define STATIS_3DNR_BUF_NUM 6
#define STATIS_PDAF_BUF_NUM 6
#define STATIS_EBD_BUF_NUM 4
#define STATIS_HIST_BUF_NUM   0
#define STATIS_LSCM_BUF_NUM   0
#define STATIS_ISP_HIST2_BUF_NUM 4
#define STATIS_AFL_SIZE  (STATIS_AFL_RBUF_SIZE + STATIS_AFL_GBUF_SIZE3)
#define AFL_RBUF_OFFSET	STATIS_AFL_GBUF_SIZE3

#define PM0_SIZE	(sizeof(struct dcam_param_data_l3) + sizeof(struct debug_base_info))
#define DCAM_PARAM_SIZE  (cmr_u32)((PM0_SIZE + 15) & ~15)
#define PM1_SIZE 	(sizeof(struct isp_param_data_l3) + sizeof(struct debug_base_info))
#define ISP_PARAM_SIZE	(cmr_u32)((PM1_SIZE + 15) & ~15)

#elif defined CONFIG_ISP_2_6 /* for SharkL5 & ROC1 */

#define CAM_RAW_BITS (10)

#define STATIS_AEM_BUF_NUM 4
#define STATIS_AFM_BUF_NUM 4
#define STATIS_AFL_BUF_NUM 3
#define STATIS_3DNR_BUF_NUM 6
#define STATIS_PDAF_BUF_NUM 6
#define STATIS_EBD_BUF_NUM 4
#define STATIS_HIST_BUF_NUM   4
#define STATIS_LSCM_BUF_NUM   0
#define STATIS_ISP_HIST2_BUF_NUM 4
#define STATIS_AFL_SIZE  (STATIS_AFL_RBUF_SIZE + STATIS_AFL_GBUF_SIZE)
#define AFL_RBUF_OFFSET	STATIS_AFL_GBUF_SIZE
#define DCAM_PARAM_SIZE  0
#define ISP_PARAM_SIZE  0

#else /* more other projects */

#define CAM_RAW_BITS (14)

#define STATIS_AEM_BUF_NUM 4
#define STATIS_AFM_BUF_NUM 4
#define STATIS_AFL_BUF_NUM 3
#define STATIS_3DNR_BUF_NUM 6
#define STATIS_PDAF_BUF_NUM 6
#define STATIS_EBD_BUF_NUM 4
#define STATIS_HIST_BUF_NUM   4
#define STATIS_LSCM_BUF_NUM   4
#define STATIS_ISP_HIST2_BUF_NUM 4

#define STATIS_AFL_SIZE  (STATIS_AFL_RBUF_SIZE + STATIS_AFL_GBUF_SIZE)
#define AFL_RBUF_OFFSET	STATIS_AFL_GBUF_SIZE

#define DCAM_PARAM_SIZE  0
#define ISP_PARAM_SIZE  0

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
	cmr_s32 mfd;
	cmr_u32 offset;
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

struct isp_stats_alloc_info {
	cmr_u32 alloc_num;
	cmr_u32 alloc_size;
	cmr_u32 align_alloc_size;
	cmr_s32 alloc_mfd[STATIS_BUF_NUM_MAX];
	cmr_uint alloc_uaddr[STATIS_BUF_NUM_MAX];

	cmr_u32 size;
	cmr_u32 align_size;
	cmr_u32 num;
	cmr_s32 mfd[STATIS_BUF_NUM_MAX];
	cmr_u32 offset[STATIS_BUF_NUM_MAX];
	cmr_uint uaddr[STATIS_BUF_NUM_MAX];
};

struct isp_pmdbg_alloc_info {
	cmr_u32 alloc_num;
	cmr_u32 alloc_size;
	cmr_u32 align_alloc_size;
	cmr_s32 alloc_mfd[PARAM_BUF_NUM_MAX];
	cmr_uint alloc_uaddr[PARAM_BUF_NUM_MAX];

	cmr_u32 size;
	cmr_u32 align_size;
	cmr_u32 num;
	cmr_s32 mfd[PARAM_BUF_NUM_MAX];
	cmr_u32 offset[PARAM_BUF_NUM_MAX];
	cmr_uint uaddr[PARAM_BUF_NUM_MAX];
};

struct isp_mem_info {
	struct isp_stats_alloc_info buf_info[STATIS_TYPE_MAX];
	struct isp_pmdbg_alloc_info dbg_buf;

	void *buffer_client_data;
	cmr_malloc alloc_cb;
	cmr_free free_cb;
	cmr_invalidate_buf invalidate_cb;
	cmr_flush_buf flush_cb;
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

cmr_s32 dcam_u_lscm_bypass(cmr_handle handle, cmr_u32 bypass);
cmr_s32 dcam_u_lsc_monitor(cmr_handle handle, void *block_info);

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
cmr_s32 dcam_u_bayerhist_bypass(cmr_handle handle, cmr_u32 bypass);

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
cmr_s32 dcam_u_raw_gtm_block(cmr_handle handle, void *block_info);

cmr_s32 isp_u_rgb_ltm_block(cmr_handle handle, void *block_info);
cmr_s32 isp_u_yuv_ltm_block(cmr_handle handle, void *block_info);


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
