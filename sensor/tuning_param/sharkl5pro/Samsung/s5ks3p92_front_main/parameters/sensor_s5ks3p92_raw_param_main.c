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


/************************************************************************/


//#ifdef WIN32

#include "sensor_raw.h"


#define _NR_MAP_PARAM_
#include "isp_nr.h"
#undef _NR_MAP_PARAM_


/* Begin Include */
#include "sensor_s5ks3p92_raw_param_common.c"
#include "sensor_s5ks3p92_raw_param_prv_0.c"
#include "sensor_s5ks3p92_raw_param_prv_1.c"
#include "sensor_s5ks3p92_raw_param_cap_0.c"
#include "sensor_s5ks3p92_raw_param_cap_1.c"
#include "sensor_s5ks3p92_raw_param_video_0.c"
#include "sensor_s5ks3p92_raw_param_video_1.c"
#include "sensor_s5ks3p92_raw_param_fdr_cap0.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=R1.17.0501 */


/* Capture Sizes:
	4608x3488,2304x1744,1280x720
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_s5ks3p92_trim_info=
{
	0x00,
	{
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
	}
};


/************************************************************************/


static struct sensor_raw_ioctrl s_s5ks3p92_ioctrl=
{
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0
};


/************************************************************************/


static struct sensor_version_info s_s5ks3p92_version_info=
{
	0x000B0009,
	{
		{
			0x736B3573,
			0x32397033,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000,
			0x00000000
		}
	},
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000
};


/************************************************************************/


static uint32_t s_s5ks3p92_libuse_info[]=
{
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
};


/************************************************************************/


static struct sensor_raw_info s_s5ks3p92_mipi_raw_info=
{
	&s_s5ks3p92_version_info,
	{
		{s_s5ks3p92_tune_info_common, sizeof(s_s5ks3p92_tune_info_common)},
		{s_s5ks3p92_tune_info_prv_0, sizeof(s_s5ks3p92_tune_info_prv_0)},
		{s_s5ks3p92_tune_info_prv_1, sizeof(s_s5ks3p92_tune_info_prv_1)},
		{NULL, 0},
		{NULL, 0},
		{s_s5ks3p92_tune_info_cap_0, sizeof(s_s5ks3p92_tune_info_cap_0)},
		{s_s5ks3p92_tune_info_cap_1, sizeof(s_s5ks3p92_tune_info_cap_1)},
		{NULL, 0},
		{NULL, 0},
		{s_s5ks3p92_tune_info_video_0, sizeof(s_s5ks3p92_tune_info_video_0)},
		{s_s5ks3p92_tune_info_video_1, sizeof(s_s5ks3p92_tune_info_video_1)},
		{NULL, 0},
		{NULL, 0},
		{s_s5ks3p92_tune_info_fdr_cap0, sizeof(s_s5ks3p92_tune_info_fdr_cap0)},
		{NULL, 0},
		{NULL, 0},
	},
	&s_s5ks3p92_trim_info,
	&s_s5ks3p92_ioctrl,
	(struct sensor_libuse_info *)s_s5ks3p92_libuse_info,
	{
		&s_s5ks3p92_fix_info_common,
		&s_s5ks3p92_fix_info_prv_0,
		&s_s5ks3p92_fix_info_prv_1,
		NULL,
		NULL,
		&s_s5ks3p92_fix_info_cap_0,
		&s_s5ks3p92_fix_info_cap_1,
		NULL,
		NULL,
		&s_s5ks3p92_fix_info_video_0,
		&s_s5ks3p92_fix_info_video_1,
		NULL,
		NULL,
		&s_s5ks3p92_fix_info_fdr_cap0,
		NULL,
		NULL,
	},
	{
		{s_s5ks3p92_common_tool_ui_input, sizeof(s_s5ks3p92_common_tool_ui_input)},
		{s_s5ks3p92_prv_0_tool_ui_input, sizeof(s_s5ks3p92_prv_0_tool_ui_input)},
		{s_s5ks3p92_prv_1_tool_ui_input, sizeof(s_s5ks3p92_prv_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_s5ks3p92_cap_0_tool_ui_input, sizeof(s_s5ks3p92_cap_0_tool_ui_input)},
		{s_s5ks3p92_cap_1_tool_ui_input, sizeof(s_s5ks3p92_cap_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_s5ks3p92_video_0_tool_ui_input, sizeof(s_s5ks3p92_video_0_tool_ui_input)},
		{s_s5ks3p92_video_1_tool_ui_input, sizeof(s_s5ks3p92_video_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_s5ks3p92_fdr_cap0_tool_ui_input, sizeof(s_s5ks3p92_fdr_cap0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
	},
	{
		&s_s5ks3p92_nr_scene_map_param,
		&s_s5ks3p92_nr_level_number_map_param,
		&s_s5ks3p92_default_nr_level_map_param,
	},
};
