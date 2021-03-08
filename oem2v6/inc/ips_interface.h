/*
 * Copyright (C) 2021 The Android Open Source Project
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
#ifndef _IPS_INTERFACE_H_
#define _IPS_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "cmr_types.h"
#include "cmr_common.h"

/////////////// IPS interface ////////////
#define IPS_STEP_MAX 16

enum {
	IPS_TYPE_HDR,
	IPS_TYPE_MFNR,
	IPS_TYPE_CNR,
	IPS_TYPE_DRE,
	IPS_TYPE_DREPRO,
	IPS_TYPE_FB,
	IPS_TYPE_FILTER,
	IPS_TYPE_WATERMARK,
	IPS_TYPE_JPEG,
	IPS_TYPE_MAX,
};

enum {
	IPS_CB_GET_BUF,
	IPS_CB_RETURN_BUF,
	IPS_CB_INV_BUFCACHE,
	IPS_CB_FLUSH_BUFCACHE,
	IPS_CB_JPEG_DONE,
	IPS_CB_ALL_DONE,
};

struct ips_jpeg_param_t {
	struct img_size thumb_size;
	struct cmr_op_mean mean;
	saved_exif_info_t exif_data;
};

struct ips_handle_t {
	cmr_handle scale_handle;
	cmr_handle jpeg_handle;
};

struct ips_proc_t {
	cmr_u32 type;
	cmr_handle ops;
	cmr_handle handle;
};

struct ips_request_t {
	cmr_u32 request_id;
	cmr_u32 frame_total;
	cmr_handle owner;
	struct ips_proc_t proc_steps[IPS_STEP_MAX];
};


typedef cmr_int (*ips_req_cb)(cmr_handle client_handle,
	struct ips_request_t *req, cmr_u32 cb_type, void *data);

cmr_int cmr_ips_jpeg_done(cmr_handle ips_handle,
	struct jpeg_enc_cb_param *cb_data);

cmr_int cmr_ips_post(cmr_handle ips_handle,
	struct ips_request_t *src, struct img_frm *frame);

cmr_int cmr_ips_create_req(cmr_handle ips_handle,
	struct ips_request_t **dst, cmr_handle client_data);
cmr_int cmr_ips_init_req(cmr_handle ips_handle,
	struct ips_request_t *dst, void *data, ips_req_cb req_cb);
cmr_int cmr_ips_destroy_req(cmr_handle ips_handle,
	struct ips_request_t **dst);
cmr_int cmr_ips_quickstop_req(cmr_handle ips_handle,
	struct ips_request_t *dst);

cmr_int cmr_ips_set_jpeg_param(cmr_handle ips_handle,
	struct ips_request_t *dst, void *param);

cmr_int cmr_ips_set_handles(cmr_handle ips_handle,
	struct ips_handle_t *data);

cmr_int cmr_ips_init(cmr_handle *handle, cmr_handle client_data);
cmr_int cmr_ips_deinit(cmr_handle handle);


#ifdef __cplusplus
}
#endif

#endif // for _IPS_INTERFACE_H_
