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
#include "sensor_ov02b1b_syp_2_raw_param_common.c"
#include "sensor_ov02b1b_syp_2_raw_param_prv_0.c"
#include "sensor_ov02b1b_syp_2_raw_param_cap_0.c"
#include "sensor_ov02b1b_syp_2_raw_param_video_0.c"

/* End Include */

//#endif


/************************************************************************/


/* IspToolVersion=R1.17.0501 */


/* Capture Sizes:
	1600x1200
*/


/************************************************************************/


static struct sensor_raw_resolution_info_tab s_ov02b1b_syp_2_trim_info=
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


static struct sensor_raw_ioctrl s_ov02b1b_syp_2_ioctrl=
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


/********************************************************************************
 * static struct sensor_version_info s_****_version_info, **** is the sensor name .
 * Param[2]/ Param[3] are ASCII values of the sensor name string ****.
 * Please modify the sensor name by using rename sensor function of the ISP TOOL, 
 * then the Param[2]/ Param[3] are changed accordingly.
 * NO modifying manually.
********************************************************************************/

static struct sensor_version_info s_ov02b1b_syp_2_version_info=
{
	0x000B0009,
	{
		{
			0x3230766F,
			0x5F623162,
			0x5F707973,
			0x00000032,
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


static uint32_t s_ov02b1b_syp_2_libuse_info[]=
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


static struct sensor_raw_info s_ov02b1b_syp_2_mipi_raw_info=
{
	&s_ov02b1b_syp_2_version_info,
	{
		{s_ov02b1b_syp_2_tune_info_common, sizeof(s_ov02b1b_syp_2_tune_info_common)},
		{s_ov02b1b_syp_2_tune_info_prv_0, sizeof(s_ov02b1b_syp_2_tune_info_prv_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_ov02b1b_syp_2_tune_info_cap_0, sizeof(s_ov02b1b_syp_2_tune_info_cap_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_ov02b1b_syp_2_tune_info_video_0, sizeof(s_ov02b1b_syp_2_tune_info_video_0)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},
	&s_ov02b1b_syp_2_trim_info,
	&s_ov02b1b_syp_2_ioctrl,
	(struct sensor_libuse_info *)s_ov02b1b_syp_2_libuse_info,
	{
		&s_ov02b1b_syp_2_fix_info_common,
		&s_ov02b1b_syp_2_fix_info_prv_0,
		NULL,
		NULL,
		NULL,
		&s_ov02b1b_syp_2_fix_info_cap_0,
		NULL,
		NULL,
		NULL,
		&s_ov02b1b_syp_2_fix_info_video_0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
	},
	{
		{s_ov02b1b_syp_2_common_tool_ui_input, sizeof(s_ov02b1b_syp_2_common_tool_ui_input)},
		{s_ov02b1b_syp_2_prv_0_tool_ui_input, sizeof(s_ov02b1b_syp_2_prv_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_ov02b1b_syp_2_cap_0_tool_ui_input, sizeof(s_ov02b1b_syp_2_cap_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{s_ov02b1b_syp_2_video_0_tool_ui_input, sizeof(s_ov02b1b_syp_2_video_0_tool_ui_input)},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
		{NULL, 0},
	},
	{
		&s_ov02b1b_syp_2_nr_scene_map_param,
		&s_ov02b1b_syp_2_nr_level_number_map_param,
		&s_ov02b1b_syp_2_default_nr_level_map_param,
	},
};
