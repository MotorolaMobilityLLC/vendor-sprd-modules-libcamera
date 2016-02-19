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
#include "al3AWrapper.h"
#include "HW3A_Stats.h"

cmr_int isp_dispatch_stats(void *isp_stats, void *ae_stats_buf, void *awb_stats_buf, void *af_stats_buf, void *yhist_stats_buf, void *antif_stats_buf, void *subsample, cmr_u32 sof_idx)
{
	cmr_int                                     ret = ISP_SUCCESS;

	ret = (cmr_int)al3AWrapper_DispatchHW3AStats(isp_stats,
			(ISP_DRV_META_AE_t*)ae_stats_buf,
			(ISP_DRV_META_AWB_t*)awb_stats_buf,
			(ISP_DRV_META_AF_t *)af_stats_buf,
			(ISP_DRV_META_YHist_t *)yhist_stats_buf,
			(ISP_DRV_META_AntiF_t *)antif_stats_buf,
			(ISP_DRV_META_Subsample_t *)subsample,
			sof_idx);
	if (ret) {
		ISP_LOGE("failed to disptach stats %ld", ret);
	}
	ISP_LOGI("done %ld", ret);
	return ret;
}
