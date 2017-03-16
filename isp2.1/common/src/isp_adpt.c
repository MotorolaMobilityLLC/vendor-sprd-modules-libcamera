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

#include "isp_adpt.h"
#include "isp_com.h"
#include "awb_ctrl.h"
#include "ae_sprd_ctrl.h"
#include "af_ctrl.h"
#include "awb_al_ctrl.h"
#include "awb_sprd_ctrl.h"
#include "sensor_raw.h"
#include "ae_log.h"
#include "af_log.h"
#include "af_sprd_ctrl.h"
#include "sprd_af_ctrl_v1.h"
#include "sp_af_ctrl.h"
#include "sprd_pdaf_adpt.h"
#include <dlfcn.h>
#include "ALC_AF_Ctrl.h"

#ifdef CONFIG_USE_ALC_AE
#include "ae_alc_ctrl.h"
#endif

extern struct adpt_ops_type ae_sprd_adpt_ops_ver1;
static uint32_t *ae_sprd_version_ops[]= {
	(uint32_t*)&ae_sprd_adpt_ops_ver1,
	(uint32_t*)&ae_sprd_adpt_ops_ver1,
};

uint32_t get_sprd_ae_ops(uint32_t ae_lib_version, struct adpt_ops_type **ae_ops)
{
	uint32_t rtn = ISP_SUCCESS;
	struct adpt_ops_type **table = (struct adpt_ops_type **)ae_sprd_version_ops;
	*ae_ops = table[ae_lib_version];
	return rtn;
}

static uint32_t (*ae_product_ops[]) (uint32_t, struct adpt_ops_type **) = {
	[ADPT_SPRD_AE_LIB] = get_sprd_ae_ops,
};

static int32_t adpt_get_ae_ops(struct third_lib_info *lib_info,
			       struct adpt_ops_type **ops)
{
	int32_t rtn = -1;
	uint32_t ae_producer_id = 0;
	uint32_t ae_lib_version = 0;

	ae_producer_id = lib_info->product_id;
	ae_lib_version = lib_info->version_id;
	if (ADPT_MAX_AE_LIB > ae_producer_id) {
		rtn = ae_product_ops[ae_producer_id] (ae_lib_version, ops);
	} else {
		rtn = AE_ERROR;
	}

	return rtn;
}

extern struct adpt_ops_type awb_sprd_adpt_ops_ver1;
static uint32_t *awb_sprd_version_ops[]= {
	(uint32_t*)&awb_sprd_adpt_ops_ver1,
	(uint32_t*)&awb_sprd_adpt_ops_ver1,
};

uint32_t get_sprd_awb_ops(uint32_t awb_lib_version, struct adpt_ops_type **awb_ops)
{
	uint32_t rtn = ISP_SUCCESS;
	struct adpt_ops_type **table = (struct adpt_ops_type **)awb_sprd_version_ops;
	*awb_ops = table[awb_lib_version];
	return rtn;
}

static uint32_t (*awb_product_ops[]) (uint32_t, struct adpt_ops_type **) = {
	[ADPT_SPRD_AWB_LIB] = get_sprd_awb_ops,
};

static int32_t adpt_get_awb_ops(struct third_lib_info *lib_info,
			       struct adpt_ops_type **ops)
{
	int32_t rtn = -1;
	uint32_t awb_producer_id = 0;
	uint32_t awb_lib_version = 0;

	awb_producer_id = lib_info->product_id;
	awb_lib_version = lib_info->version_id;
	if (ADPT_MAX_AWB_LIB > awb_producer_id) {
		rtn = awb_product_ops[awb_producer_id] (awb_lib_version, ops);
	} else {
		rtn = AWB_ERROR;
	}

	return rtn;
}

extern struct adpt_ops_type af_sprd_adpt_ops_ver0;
extern struct adpt_ops_type af_sprd_adpt_ops_ver1;
static uint32_t *af_sprd_version_ops[]= {
	(uint32_t*)&af_sprd_adpt_ops_ver0,
	(uint32_t*)&af_sprd_adpt_ops_ver1,
};

uint32_t get_sprd_af_ops(uint32_t af_lib_version, struct adpt_ops_type **af_ops)
{
	uint32_t rtn = ISP_SUCCESS;
	struct adpt_ops_type **table = (struct adpt_ops_type **)af_sprd_version_ops;
	*af_ops = table[af_lib_version];
	return rtn;
}

static uint32_t (*af_product_ops[]) (uint32_t, struct adpt_ops_type **) = {
	[ADPT_SPRD_AF_LIB] = get_sprd_af_ops,
};

static int32_t adpt_get_af_ops(struct third_lib_info *lib_info,
			       struct adpt_ops_type **ops)
{
	int32_t rtn = -1;
	uint32_t af_producer_id = 0;
	uint32_t af_lib_version = 0;

	af_producer_id = lib_info->product_id;
	af_lib_version = lib_info->version_id;
    ISP_LOGE("af_producer_id %d,af_lib_version %d",af_producer_id,af_lib_version);
	if (ADPT_MAX_AF_LIB > af_producer_id) {
		rtn = af_product_ops[af_producer_id] (af_lib_version, ops);
	} else {
		rtn = AF_ERROR;
	}

	return rtn;
}

extern struct adpt_ops_type lsc_sprd_adpt_ops_ver0;
static uint32_t *lsc_sprd_version_ops[]= {
	(uint32_t*)&lsc_sprd_adpt_ops_ver0,
};

uint32_t get_sprd_lsc_ops(uint32_t lsc_lib_version, struct adpt_ops_type **lsc_ops)
{
	uint32_t rtn = ISP_SUCCESS;
	struct adpt_ops_type **table = (struct adpt_ops_type **)lsc_sprd_version_ops;
	*lsc_ops = table[lsc_lib_version];
	return rtn;
}


static uint32_t (*lsc_product_ops[]) (uint32_t, struct adpt_ops_type **) = {
	[ADPT_SPRD_LSC_LIB] = get_sprd_lsc_ops,
};

static int32_t adpt_get_lsc_ops(struct third_lib_info *lib_info,
			       struct adpt_ops_type **ops)
{
	cmr_int rtn = -1;
	uint32_t lsc_producer_id = 0;
	uint32_t lsc_lib_version = 0;

	lsc_producer_id = lib_info->product_id;
	lsc_lib_version = lib_info->version_id;
	if (ADPT_MAX_LSC_LIB > lsc_producer_id) {
		rtn = lsc_product_ops[lsc_producer_id] (lsc_lib_version, ops);
	} else {
		rtn = ISP_ERROR;
	}

	return rtn;
}

extern struct adpt_ops_type sprd_pdaf_adpt_ops;
static uint32_t *pdaf_sprd_version_ops[]= {
	(uint32_t*)&sprd_pdaf_adpt_ops,
};

uint32_t get_sprd_pdaf_ops(uint32_t pdaf_lib_version, struct adpt_ops_type **pdaf_ops)
{
	uint32_t rtn = ISP_SUCCESS;
	struct adpt_ops_type **table = (struct adpt_ops_type **)pdaf_sprd_version_ops;
	*pdaf_ops = table[pdaf_lib_version];
	return rtn;
}

static uint32_t (*pdaf_product_ops[]) (uint32_t, struct adpt_ops_type **) = {
	[ADPT_SPRD_PDAF_LIB] = get_sprd_pdaf_ops,
};

static int32_t adpt_get_pdaf_ops(struct third_lib_info *lib_info,
			       struct adpt_ops_type **ops)
{
	int32_t rtn = -1;
	uint32_t pdaf_producer_id = 0;
	uint32_t pdaf_lib_version = 0;

	pdaf_producer_id = lib_info->product_id;
	pdaf_lib_version = lib_info->version_id;
	pdaf_producer_id = 0;
	pdaf_lib_version = 0;

	ISP_LOGI("pdaf_producer_id %d,pdaf_lib_version %d",pdaf_producer_id,pdaf_lib_version);
	if (ADPT_MAX_PDAF_LIB > pdaf_producer_id) {
		rtn = pdaf_product_ops[pdaf_producer_id] (pdaf_lib_version, ops);
	} else {
		rtn = AF_ERROR;
	}

	return rtn;
}


static int32_t (*modules_ops[]) (struct third_lib_info *,
				 struct adpt_ops_type **) = {
	[ADPT_LIB_AE] = adpt_get_ae_ops,
	[ADPT_LIB_AWB] = adpt_get_awb_ops,
	[ADPT_LIB_AF] = adpt_get_af_ops,
	[ADPT_LIB_LSC] = adpt_get_lsc_ops,
	[ADPT_LIB_PDAF] = adpt_get_pdaf_ops,
};

int32_t adpt_get_ops(int32_t lib_type, struct third_lib_info *lib_info,
		     struct adpt_ops_type **ops)
{
	int32_t rtn = -1;

	if (ADPT_LIB_MAX > lib_type)
		rtn = modules_ops[lib_type](lib_info, ops);

	return rtn;
}
