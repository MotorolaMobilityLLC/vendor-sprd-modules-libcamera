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
#include "sensor_hi1336_raw_param_common.c"
#include "sensor_hi1336_raw_param_prv_0.c"
#include "sensor_hi1336_raw_param_prv_1.c"
#include "sensor_hi1336_raw_param_cap_0.c"
#include "sensor_hi1336_raw_param_cap_1.c"
#include "sensor_hi1336_raw_param_video_0.c"
#include "sensor_hi1336_raw_param_video_1.c"
#include "sensor_hi1336_raw_param_video_2.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=R1.18.2001 */


/* Capture Sizes:
	4208x3120,2104x1560,1280x720
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_hi1336_trim_info=
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


static struct sensor_raw_ioctrl s_hi1336_ioctrl=
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


static struct sensor_version_info s_hi1336_version_info=
{
	0x00090007,
	{
		{
			0x33316968,
			0x00003633,
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


static uint32_t s_hi1336_libuse_info[]=
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


static struct sensor_raw_info s_hi1336_mipi_raw_info=
{
	&s_hi1336_version_info,
	{
		{s_hi1336_tune_info_common, sizeof(s_hi1336_tune_info_common)},
		{s_hi1336_tune_info_prv_0, sizeof(s_hi1336_tune_info_prv_0)},
		{s_hi1336_tune_info_prv_1, sizeof(s_hi1336_tune_info_prv_1)},
		{NULL, 0},
		{NULL, 0},
		{s_hi1336_tune_info_cap_0, sizeof(s_hi1336_tune_info_cap_0)},
		{s_hi1336_tune_info_cap_1, sizeof(s_hi1336_tune_info_cap_1)},
		{NULL, 0},
		{NULL, 0},
		{s_hi1336_tune_info_video_0, sizeof(s_hi1336_tune_info_video_0)},
		{s_hi1336_tune_info_video_1, sizeof(s_hi1336_tune_info_video_1)},
		{s_hi1336_tune_info_video_2, sizeof(s_hi1336_tune_info_video_2)},
		{NULL, 0},
	},
	&s_hi1336_trim_info,
	&s_hi1336_ioctrl,
	(struct sensor_libuse_info *)s_hi1336_libuse_info,
	{
		&s_hi1336_fix_info_common,
		&s_hi1336_fix_info_prv_0,
		&s_hi1336_fix_info_prv_1,
		NULL,
		NULL,
		&s_hi1336_fix_info_cap_0,
		&s_hi1336_fix_info_cap_1,
		NULL,
		NULL,
		&s_hi1336_fix_info_video_0,
		&s_hi1336_fix_info_video_1,
		&s_hi1336_fix_info_video_2,
		NULL,
	},
	{
		{s_hi1336_common_tool_ui_input, sizeof(s_hi1336_common_tool_ui_input)},
		{s_hi1336_prv_0_tool_ui_input, sizeof(s_hi1336_prv_0_tool_ui_input)},
		{s_hi1336_prv_1_tool_ui_input, sizeof(s_hi1336_prv_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_hi1336_cap_0_tool_ui_input, sizeof(s_hi1336_cap_0_tool_ui_input)},
		{s_hi1336_cap_1_tool_ui_input, sizeof(s_hi1336_cap_1_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{s_hi1336_video_0_tool_ui_input, sizeof(s_hi1336_video_0_tool_ui_input)},
		{s_hi1336_video_1_tool_ui_input, sizeof(s_hi1336_video_1_tool_ui_input)},
		{s_hi1336_video_2_tool_ui_input, sizeof(s_hi1336_video_2_tool_ui_input)},
		{NULL, 0},
	},
	{
		&s_hi1336_nr_scene_map_param,
		&s_hi1336_nr_level_number_map_param,
		&s_hi1336_default_nr_level_map_param,
	},
};