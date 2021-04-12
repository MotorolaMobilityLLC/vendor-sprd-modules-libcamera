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
#ifndef _CMR_MEM_H_
#define _CMR_MEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cutils/list.h>
#include "cmr_common.h"

enum isp_raw_format{
    ISP_RAW_PACK10 = 0x00,
    ISP_RAW_HALF10 = 0x01,
    ISP_RAW_HALF14 = 0x02,
    ISP_RAW_FORMAT_MAX
};

typedef int (*alloc_mem_ptr)(void *handle, unsigned int size,
                             unsigned long *addr_phy, unsigned long *addr_vir,
                             signed int *fd);
typedef int (*free_mem_ptr)(void *handle);

struct cmr_cap_2_frm {
    struct img_frm mem_frm;

    void *handle;
    int type;
    int zoom_post_proc;
    alloc_mem_ptr alloc_mem;
    free_mem_ptr free_mem;
};


struct cmr_buf {
	struct listnode list;
	int fd;
	void *gpu_handle;
	cmr_u32 cache;
	cmr_u32 mem_type;
	cmr_u32 mem_size;
	cmr_u32 used;
	cmr_uint vaddr;
	cmr_uint paddr;
};

struct cmr_queue {
	cmr_u32 type;
	cmr_u32 max;
	cmr_u32 cnt;
	cmr_u32 free_cnt;
	cmr_u32 total_size;
	void *lock;
	struct listnode header;
};

int check_free_buffer(struct cmr_queue *q, uint32_t size, uint32_t *cnt);
int get_free_buffer(struct cmr_queue *q, uint32_t size, struct cmr_buf *dst);
int put_free_buffer(struct cmr_queue *q, struct cmr_buf *dst);
int get_buf_gpuhandle(struct cmr_queue *q, int32_t buf_fd, void **gpu_hanlde);

int inc_buffer_q(struct cmr_queue *q, struct memory_param *mops, uint32_t size, uint32_t *cnt);
int dec_buffer_q(struct cmr_queue *q, struct memory_param *mops, uint32_t size, uint32_t *cnt);
int inc_gbuffer_q(struct cmr_queue *q, struct memory_param *mops,
        uint32_t width, uint32_t height, uint32_t *cnt);

int init_buffer_q(struct cmr_queue *q, uint32_t max, uint32_t type);
int deinit_buffer_q(struct cmr_queue *q, struct memory_param *mops);
int clear_buffer_queue(struct cmr_queue *q, struct memory_param *mops);


int camera_set_largest_pict_size(cmr_u32 camera_id, cmr_u16 width,
                                 cmr_u16 height);

int camera_get_raw_postproc_capture_size(cmr_u32 camera_id, cmr_u32 *pp_cap_size, cmr_u32 is_loose);
int camera_get_4in1_postproc_capture_size(cmr_u32 camera_id, cmr_u32 *pp_cap_size, cmr_u32 is_loose);
int camera_get_postproc_capture_size(cmr_u32 camera_id, cmr_u32 * pp_cap_size, cmr_u32 channel_size);

int camera_arrange_capture_buf(
    struct cmr_cap_2_frm *cap_2_frm, struct img_size *sn_size,
    struct img_rect *sn_trim, struct img_size *image_size, uint32_t orig_fmt,
    struct img_size *cap_size, struct img_size *thum_size,
    struct cmr_cap_mem *capture_mem, struct sensor_exp_info *sn_if, uint32_t need_rot, uint32_t need_scale,
    uint32_t image_cnt, cmr_u32 is_4in1_mode);
uint32_t camera_get_aligned_size(uint32_t type, uint32_t size);

void camera_set_mem_multimode(multiCameraMode camera_mode);

unsigned int camera_get_mipi_raw_dcam_pitch(int w);

#ifdef __cplusplus
}
#endif

#endif
