/*
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _CAM_TYPES_H_
#define _CAM_TYPES_H_

#include <linux/workqueue.h>
#include "sprd_img.h"

#ifndef MAX
#define MAX(a, b) ((a > b) ? a : b)
#endif
#ifndef MIN
#define MIN(a, b) ((a < b) ? a : b)
#endif

#define CAM_BUF_ALIGN_SIZE		4


/*************** for global debug starts********************/

/* for iommu
 * 0: auto - dts configured
 * 1: iommu off and reserved memory
 * 2: iommu on and reserved memory
 * 3: iommu on and system memory
 */
enum cam_iommu_mode {
	IOMMU_AUTO = 0,
	IOMMU_OFF,
	IOMMU_ON_RESERVED,
	IOMMU_ON,
};
extern int g_dbg_iommu_mode;
extern int g_dbg_set_iommu_mode;


/* for zoom debug during bringup */
extern uint32_t g_dbg_zoom_mode;
extern uint32_t g_dbg_rds_limit;


/* for memory leak debug */
struct cam_mem_dbg_info {
	atomic_t ion_alloc_cnt;
	atomic_t ion_kmap_cnt;
	atomic_t ion_dma_cnt;
	atomic_t iommu_map_cnt[6];
	atomic_t empty_frm_cnt;
};
extern struct cam_mem_dbg_info *g_mem_dbg;


/* for raw picture dump */
struct cam_dbg_dump {
	uint32_t dump_en;
	uint32_t dump_count;
	uint32_t dump_ongoing;
	struct mutex dump_lock;
	struct completion *dump_start[3];
};
extern struct cam_dbg_dump g_dbg_dump;

/*************** for global debug ends ********************/



enum camera_cap_type {
	CAM_CAP_NORMAL = 0,
	CAM_CAP_RAW_FULL,
	CAM_CAP_RAW_BIN
};

enum camera_id {
	CAM_ID_0 = 0,
	CAM_ID_1,
	CAM_ID_2,
	CAM_ID_3,
	CAM_ID_MAX,
};

enum camera_cap_status {
	CAM_CAPTURE_STOP = 0,
	CAM_CAPTURE_START,
	CAM_CAPTURE_RAWPROC,
};

enum cam_ch_id {
	CAM_CH_RAW = 0,
	CAM_CH_PRE,
	CAM_CH_CAP,
	CAM_CH_VID,
	CAM_CH_PRE_THM,
	CAM_CH_CAP_THM,
	CAM_CH_MAX,
};

enum cam_3dnr_type {
	CAM_3DNR_OFF = 0,
	CAM_3DNR_HW,
	CAM_3DNR_SW,
};

enum cam_data_endian {
	ENDIAN_LITTLE = 0,
	ENDIAN_BIG,
	ENDIAN_HALFBIG,
	ENDIAN_HALFLITTLE,
	ENDIAN_MAX
};

struct camera_format {
	char *name;
	uint32_t fourcc;
	int depth;
};

struct img_endian {
	uint32_t y_endian;
	uint32_t uv_endian;
};

struct img_addr {
	uint32_t addr_ch0;
	uint32_t addr_ch1;
	uint32_t addr_ch2;
};

struct img_pitch {
	uint32_t pitch_ch0;
	uint32_t pitch_ch1;
	uint32_t pitch_ch2;
};

struct img_deci_info {
	uint32_t deci_y_eb;
	uint32_t deci_y;
	uint32_t deci_x_eb;
	uint32_t deci_x;
};

struct img_size {
	uint32_t w;
	uint32_t h;
};

struct img_trim {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t size_x;
	uint32_t size_y;
};

struct img_scaler_info {
	struct img_size src_size;
	struct img_trim src_trim;
	struct img_size dst_size;
};


struct cam_thread_info {
	atomic_t thread_stop;
	void *ctx_handle;
	int (*proc_func)(void *param);
	uint8_t thread_name[32];
	struct completion thread_com;
	struct completion thread_stop_com;
	struct task_struct *thread_task;
};

enum cam_evt_type {
	CAM_EVT_ERROR,
};

enum isp_cb_type {
	ISP_CB_RET_SRC_BUF,
	ISP_CB_RET_DST_BUF,
	ISP_CB_DEV_ERR,
	ISP_CB_MMU_ERR,
};

enum dcam_cb_type {
	DCAM_CB_DATA_DONE,
	DCAM_CB_IRQ_EVENT,
	DCAM_CB_STATIS_DONE,

	DCAM_CB_RET_SRC_BUF,
	DCAM_CB_RET_DST_BUF,

	DCAM_CB_DEV_ERR,
	DCAM_CB_MMU_ERR,

};

extern struct camera_queue *g_empty_frm_q;

typedef int(*isp_dev_callback)(enum isp_cb_type type, void *param,
				void *priv_data);
typedef int(*dcam_dev_callback)(enum dcam_cb_type type, void *param,
				void *priv_data);


#define CAM_WORK_DONE			0
#define CAM_WORK_PENDING		1
#define CAM_WORK_RUNNING		2

struct sprd_cam_work {
	atomic_t status;
	void *priv_data;
	struct timeval time;
	ktime_t boot_time;
	struct work_struct work;
};

static inline uint32_t cal_sprd_raw_pitch(uint32_t w)
{
	uint32_t mod16_len[16] =
		{ 0, 8, 8, 8, 8, 12, 12, 12, 12, 16, 16, 16, 16, 20, 20, 20 };
	return ((w >> 4) * 20 + (mod16_len[w & 0xf]));
}
#endif /* _CAM_TYPES_H_ */
