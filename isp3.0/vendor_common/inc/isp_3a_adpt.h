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
#include "alwrapper_common.h"
cmr_int isp_dispatch_stats(void *isp_stats, void *ae_stats_buf, void *awb_stats_buf, void *af_stats_buf, void *yhist_stats_buf, void *antif_stats_buf, void *subsample, cmr_u32 sof_idx);
cmr_int isp_separate_3a_bin(void *bin, void **ae_tuning_buf, void **awb_tuning_buf, void **af_tuning_buf);
cmr_int isp_separate_drv_bin(void *bin, void **shading_buf, void **irp_buf);
cmr_int isp_separate_drv_bin_2(void *bin, cmr_u32 bin_size, struct bin2_sep_info *bin_info);
