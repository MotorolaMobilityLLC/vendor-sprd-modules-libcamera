/*-------------------------------------------------------------------*/
/*  Copyright(C) 2020 by Unisoc                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/

#ifndef _SWA_IPM_API_H_
#define _SWA_IPM_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

enum swa_log_level {
	SWA_LOGLEVEL_E = 1,
	SWA_LOGLEVEL_W,
	SWA_LOGLEVEL_I,
	SWA_LOGLEVEL_D,
	SWA_LOGLEVEL_V,
	SWA_LOGLEVEL_MAX,
};

enum swa_image_fmt {
	SWA_IMG_FMT_MIPI_RAW10 = 0,
	SWA_IMG_FMT_MIPI_RAW14,
	SWA_IMG_FMT_YUV422P,
	SWA_IMG_FMT_NV21,
	SWA_IMG_FMT_NV12,
	SWA_IMG_FMT_MAX,
};

enum swa_raw_pattern {
	SWA_RAW_PATTERN_GRBG,
	SWA_RAW_PATTERN_RGGB,
	SWA_RAW_PATTERN_BGGR,
	SWA_RAW_PATTERN_GBRG,
};

struct pic_rect {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t width;
	uint32_t height;
};

struct pic_size {
	uint32_t width;
	uint32_t height;
};

struct swa_ae_param {
	int32_t cur_bv;
	uint32_t face_stable;
	uint32_t face_num;
	uint32_t total_gain;
	uint32_t sensor_gain;
	uint32_t isp_gain;
	uint32_t exp_line;
	uint32_t exp_time;
	uint32_t ae_stable;
};

struct swa_frame {
	uint32_t base_id;
	uint32_t frm_type;
	uint32_t sec;
	uint32_t usec;
	int64_t monoboottime;
	uint32_t fmt;
	uint32_t buf_size;
	int32_t fd;
	void *gpu_handle;
	uintptr_t addr_vir[3];
	uintptr_t addr_phy[3];
	struct pic_rect rect;
	struct pic_size size;
};

struct swa_frames_inout {
	uint32_t frame_num;
	struct swa_frame frms[10];
};

struct swa_nrinfo_t {
	void *param_ptr;
	uint32_t param_size;
	uint32_t *multi_nr_map;
	int32_t mode_num;
	int32_t scene_num;
	int32_t level_num;
	int32_t mode_id;
	int32_t scene_id;
	int32_t ai_scene_id;
	/* smart result */
	int32_t idx0;
	int32_t idx1;
	int32_t weight0;
	int32_t weight1;
	uint32_t reserved[3];
};

int swa_get_handle_size(void);

int swa_open(void *ipmpro_hanlde,
			void * open_param, enum swa_log_level log_level);

int swa_process(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * data);

int swa_close(void *ipmpro_hanlde,
			void * close_param);


typedef 	int (*ipmpro_get_handle_size)(void);

typedef 	int (*ipmpro_open)(void *ipmpro_hanlde,
			void *open_param,
			enum swa_log_level log_level);

typedef	int (*ipmpro_process)(void * ipmpro_hanlde,
			struct swa_frames_inout *in,
			struct swa_frames_inout *out,
			void * data);

typedef	int (*ipmpro_close)(void *ipmpro_hanlde,
			void * close_param);


typedef int (*ipmpro_callback)(
			void * caller_hanlde,
			uint32_t cb_type,
			void * cb_data);

#ifdef __cplusplus
}
#endif
#endif
