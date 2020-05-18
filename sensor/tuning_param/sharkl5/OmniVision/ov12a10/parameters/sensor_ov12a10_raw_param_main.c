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
#include "sensor_ov12a10_raw_param_common.c"
#include "sensor_ov12a10_raw_param_prv_0.c"
#include "sensor_ov12a10_raw_param_prv_1.c"
#include "sensor_ov12a10_raw_param_cap_0.c"
#include "sensor_ov12a10_raw_param_cap_1.c"
#include "sensor_ov12a10_raw_param_video_0.c"
#include "sensor_ov12a10_raw_param_video_1.c"
#include "sensor_ov12a10_raw_param_video_2.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=R1.17.0501 */


/* Capture Sizes:
	1280x720,2048x1536,4096x3072
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_ov12a10_trim_info=
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


static struct sensor_raw_ioctrl s_ov12a10_ioctrl=
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


static struct sensor_version_info s_ov12a10_version_info=
{
	0x000A0008,
	{
		{
			0x3231766F,
			0x00303161,
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


static uint32_t s_ov12a10_libuse_info[]=
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


static struct sensor_raw_info s_ov12a10_mipi_raw_info=
{
	&s_ov12a10_version_info,
	{
		{s_ov12a10_tune_info_common, sizeof(s_ov12a10_tune_info_common)},
		{s_ov12a10_tune_info_prv_0, sizeof(s_ov12a10_tune_info_prv_0)},
		{s_ov12a10_tune_info_prv_1, sizeof(s_ov12a10_tune_info_prv_1)},
		{NULL, 0},
		{NULL, 0},
		{s_ov12a10_tune_info_cap_0, sizeof(s_ov12a10_tune_info_cap_0)},
		{s_ov12a10_tune_info_cap_1, sizeof(s_ov12a10_tune_info_cap_1)},
		{NULL, 0},
		{NULL, 0},
		{s_ov12a10_tune_info_video_0, sizeof(s_ov12a10_tune_info_video_0)},
		{s_ov12a10_tune_info_video_1, sizeof(s_ov12a10_tune_info_video_1)},
		{s_ov12a10_tune_info_video_2, sizeof(s_ov12a10_tune_info_video_2)},
		{NULL, 0},
	},
	&s_ov12a10_trim_info,
	&s_ov12a10_ioctrl,
	(struct sensor_libuse_info *)s_ov12a10_libuse_info,
	{
		&s_ov12a10_fix_info_common,
		&s_ov12a10_fix_info_prv_0,
		&s_ov12a10_fix_info_prv_1,
		NULL,
		NULL,
		&s_ov12a10_fix_info_cap_0,
		&s_ov12a10_fix_info_cap_1,
		NULL,
		NULL,
		&s_ov12a10_fix_info_video_0,
		&s_ov12a10_fix_info_video_1,
		&s_ov12a10_fix_info_video_2,
		NULL,
	},
	{
		{s_ov12a10_common_tool_ui_input, sizeof(s_ov12a10_common_tool_ui_input)},
		{s_ov12a10_prv_0_tool_ui_input, sizeof(s_ov12a10_prv_0_tool_ui_input)},
		{s_ov12a10_prv_1_tool_ui_input, sizeof(s_ov12a10_prv_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_ov12a10_cap_0_tool_ui_input, sizeof(s_ov12a10_cap_0_tool_ui_input)},
		{s_ov12a10_cap_1_tool_ui_input, sizeof(s_ov12a10_cap_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_ov12a10_video_0_tool_ui_input, sizeof(s_ov12a10_video_0_tool_ui_input)},
		{s_ov12a10_video_1_tool_ui_input, sizeof(s_ov12a10_video_1_tool_ui_input)},
		{s_ov12a10_video_2_tool_ui_input, sizeof(s_ov12a10_video_2_tool_ui_input)},
		{NULL, 0},
	},
	{
		&s_ov12a10_nr_scene_map_param,
		&s_ov12a10_nr_level_number_map_param,
		&s_ov12a10_default_nr_level_map_param,
	},
};
