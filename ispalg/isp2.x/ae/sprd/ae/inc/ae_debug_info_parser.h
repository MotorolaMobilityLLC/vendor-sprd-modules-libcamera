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
#ifndef _AE_DEBUG_INFO_PARSER_H_
#define _AE_DEBUG_INFO_PARSER_H_

#include "cmr_types.h"
#include "ae_common.h"


#ifdef __cplusplus
extern "C" {
#endif

	struct ae_point {
		cmr_u32 x;
		cmr_u32 y;
	};

	struct ae_debug_draw_fun_tab {
		cmr_s32(*draw_rect) (cmr_handle, struct ae_rect *rect, struct ae_rgb_l *color, cmr_s32 *angle);
		cmr_s32(*write_text) (cmr_handle, char *text_str, struct ae_rgb_l *color, cmr_s32 *st_x, cmr_s32 *st_y);
		cmr_s32(*fill_rect) (cmr_handle, struct ae_point * point, struct ae_rgb_l * color, cmr_u32 *num);
	};

	struct ae_debug_info_packet_in {
		char alg_version[32];
		char flash_version[32];
		cmr_u16 major_id;
		cmr_u16 minor_id;
		union {
			/*for debug info v2*/
			struct {
				cmr_handle aem_stats;
				cmr_handle base_aem_stats;
				cmr_handle alg_status;
				cmr_handle alg_results;
				cmr_handle history_param;
			};
			/*for debug info v3*/
			cmr_handle alg_handle;
		} data;
		cmr_handle packet_buf;
		cmr_u32 packet_size_max;
	};

	struct ae_debug_info_packet_out {
		cmr_u32 size;/*ae debug info real length*/
	};

	struct ae_debug_info_unpacket_in {
		char alg_version[32];
		cmr_u32 alg_id;
		char flash_version[32];
		cmr_handle packet_buf;		/*input*/
		cmr_u32 packet_len;		/*input*/
		cmr_handle temp_buf;		/*input*/
		cmr_u32 temp_buf_size;	/*input*/
		union {
			struct {
				cmr_handle alg_status;
				cmr_handle history_param;
				cmr_handle pointer_param;
				cmr_handle fuction_param;
			};
			cmr_handle debug_info;
		} debug_data;			/*output*/
	};

	struct ae_debug_info_unpacket_out {
		cmr_u32 reserved;
	};

	struct ae_debug_info_print_in {
		char alg_version[32];
		char flash_version[32];
		cmr_u16 major_id;
		cmr_u16 minor_id;
		cmr_handle packet_buf;
		cmr_u32 packet_len;
		cmr_handle unpacket_buf;
		cmr_u32 unpacket_len;
	};

	struct ae_debug_info_print_out {
		cmr_u32 reserved;
	};

	struct ae_debug_info_draw_in{
		char alg_version[32];
		cmr_handle private_data;
		cmr_handle img;
		cmr_handle packet_buf;
		cmr_u32 packet_len;
		cmr_u32 cmd;
		struct ae_debug_draw_fun_tab draw_func;
	};

	struct ae_debug_info_draw_out {
		cmr_u32 reserved;
	};


	AE_PUBLIC cmr_s32 ae_debug_info_packet(cmr_handle input, cmr_handle output);/*according to the debug structure, save the debug infomation*/
	AE_PUBLIC cmr_s32 ae_debug_info_unpacket(cmr_handle input, cmr_handle output);/*parser the debug information to debug structure*/
	AE_PUBLIC cmr_s32 ae_debug_info_print(cmr_handle input, cmr_handle output);
	AE_PUBLIC cmr_s32 ae_debug_info_draw(cmr_handle input, cmr_handle output);
	AE_PUBLIC cmr_handle ae_debug_info_get_lib_version(void);
	AE_PUBLIC cmr_s32 ae_debug_info_get_alg_version(cmr_handle debug_info, cmr_handle major_id, cmr_handle minor_id);

#ifdef __cplusplus
}
#endif
#endif
