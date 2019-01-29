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

#ifndef _ISP_SIMULATION_H_
#define _ISP_SIMULATION_H_

#include "isp_com.h"
#include "cmr_common.h"

cmr_int isp_sim_set_mipi_raw_file_name(char *file_name);
cmr_int isp_sim_get_mipi_raw_file_name(char *file_name);
cmr_int isp_sim_get_no_lsc_ae_stats(struct isp_awb_statistic_info *awb_statis, cmr_u32 *stat_w, cmr_u32 *stat_h);
cmr_int isp_sim_save_ae_stats(struct isp_awb_statistic_info *awb_statis, cmr_u32 stat_w, cmr_u32 stat_h);
cmr_int isp_sim_get_ae_stats(struct isp_awb_statistic_info *awb_statis, cmr_u32 *stat_w, cmr_u32 *stat_h);
cmr_int isp_sim_set_scene_parm(struct isptool_scene_param *scene_param);
cmr_int isp_sim_get_scene_parm(struct isptool_scene_param *scene_param);

#endif
