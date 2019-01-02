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

#define LOG_TAG "isp_simulation"

#include "cmr_log.h"
#include "cmr_common.h"
#include "isp_com.h"

cmr_int isp_sim_set_mipi_raw_file_name(char *file_name)
{
	UNUSED(file_name);
	return -1;
}

cmr_int isp_sim_get_mipi_raw_file_name(char *file_name)
{
	UNUSED(file_name);
	return -1;
}

cmr_int isp_sim_set_scene_parm(struct isptool_scene_param *scene_param)
{
	cmr_int ret = ISP_SUCCESS;
	UNUSED(scene_param);

	return ret;
}
