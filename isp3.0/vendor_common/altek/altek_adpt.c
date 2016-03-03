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
#define LOG_TAG "altek_adpt"

#include <stdlib.h>
#include "isp_common_types.h"
#include "alwrapper_3a.h"
#include "hw3a_stats.h"

cmr_int isp_dispatch_stats(void *isp_stats, void *ae_stats_buf, void *awb_stats_buf, void *af_stats_buf, void *yhist_stats_buf, void *antif_stats_buf, void *subsample, cmr_u32 sof_idx)
{
	cmr_int                                     ret = ISP_SUCCESS;

	ret = (cmr_int)al3awrapper_dispatchhw3astats(isp_stats,
			(struct isp_drv_meta_ae_t*)ae_stats_buf,
			(struct isp_drv_meta_awb_t*)awb_stats_buf,
			(struct isp_drv_meta_af_t *)af_stats_buf,
			(struct isp_drv_meta_yhist_t *)yhist_stats_buf,
			(struct isp_drv_meta_antif_t *)antif_stats_buf,
			(struct isp_drv_meta_subsample_t *)subsample,
			sof_idx);
	if (ret) {
		ISP_LOGE("failed to disptach stats %ld", ret);
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}

cmr_int isp_separate_3a_bin(void *bin, void **ae_tuning_buf, void **awb_tuning_buf, void **af_tuning_buf)
{
	cmr_int                                     ret = ISP_SUCCESS;

	Separate3ABin((uint32*)bin, (uint32**)ae_tuning_buf, (uint32**)af_tuning_buf, (uint32**)awb_tuning_buf);

	return ret;
}

cmr_int isp_separate_drv_bin(void *bin, void **shading_buf, void **irp_buf)
{
	cmr_int                                     ret = ISP_SUCCESS;

	SeparateShadingIRPBin((uint32*)bin, (uint32**)shading_buf, (uint32**)irp_buf);
	return ret;
}
