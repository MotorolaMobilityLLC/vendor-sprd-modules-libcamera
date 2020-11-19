/*-------------------------------------------------------------------*/
/*  Copyright(C) 2020 by Unisoc                                  */
/*  All Rights Reserved.                                             */
/*-------------------------------------------------------------------*/

#ifndef _SWA_FDR_API_H_
#define _SWA_FDR_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "swa_ipm_api.h"

enum {
	FDR_VERSION_0, // 2 yuv fusion
	FDR_VERSION_1, // single rgb fusion
	FDR_VERSION_MAX
};

enum fdr_cb_type {
	FDR_CB_TYPE0 = 0,
	FDR_CB_TYPE1,
	FDR_CB_TYPE2,
	FDR_CB_TYPE3,
	FDR_CB_TYPE_MAX
};


struct swa_fdr_init_param {
	uint32_t fdr_version;
	uint32_t total_frame_num;
	uint32_t ref_frame_num;

	uint32_t param_size;
	void *param_ptr;

	enum swa_image_fmt sensor_fmt;
	enum swa_raw_pattern sensor_pattern;
	enum swa_image_fmt dst_fmt;
	struct pic_size sensor_size;
	struct pic_size dst_size;
	float zoom_ratio;

	void * caller_hanlde;
	ipmpro_callback cb;
};


struct swa_fdr_proc_param {
	void *ae_common_info;
	void *ae_fdr_info;

	uint32_t r;
	uint32_t b;
	uint32_t gr;
	uint32_t gb;

	uint32_t ynr_radius_base;
	uint32_t cnr_radius_base;
	void * ynr_param;
	void * cnr_param;
	struct swa_nrinfo_t *ee_info;

	void *dbg_info_addr;
	int32_t dbg_info_size;
	int32_t nlm_out_ratio0;
	int32_t nlm_out_ratio1;
	int32_t nlm_out_ratio2;
	int32_t nlm_out_ratio3;
	int32_t nlm_out_ratio4;
};

#ifdef __cplusplus
}
#endif

#endif
