/*
 * Copyright (C) 2017-2018 Spreadtrum Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _DCAM_PATH_H_
#define _DCAM_PATH_H_

#include "dcam_core.h"

#define DCAM_PATH_WMAX			5000
#define DCAM_PATH_HMAX			4000

const char *to_path_name(enum dcam_path_id path_id);

int dcam_cfg_path_base(void *dcam_handle,
				struct dcam_path_desc *path,
				void *param);

int dcam_cfg_path_size(void *dcam_handle,
				struct dcam_path_desc *path,
				void *param);

int dcam_start_path(void *dcam_handle, struct dcam_path_desc *path);

int dcam_stop_path(void *dcam_handle, struct dcam_path_desc *path);

int dcam_path_set_slowmotion_frame(struct dcam_pipe_dev *dev);
int dcam_path_init_slowmotion_frame(struct dcam_pipe_dev *dev);

/* / TODO: refine this*/
int dcam_path_set_store_frm(
			void *dcam_handle,
			struct dcam_path_desc *path,
			struct dcam_sync_helper *helper);

int dcam_set_fetch(void *dcam_handle, struct dcam_fetch_info *fetch);
void dcam_start_fetch(void);
#endif
