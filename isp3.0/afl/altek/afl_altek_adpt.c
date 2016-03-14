/*
 * Copyright (C) 2015 The Android Open Source Project
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
#define LOG_TAG "alk_adpt_afl"

#include <dlfcn.h>
#include "afl_altek_adpt.h"
#include "allib_flicker.h"
#include "allib_flicker_errcode.h"
#include "alwrapper_flicker.h"
#include "alwrapper_flicker_errcode.h"


#define AFLLIB_PATH "libalFlickerLib.so"

#define AFL_NSECOND_BASE       100 // x10us

struct aflaltek_lib_ops {
	BOOL (*load_func)(struct alflickerruntimeobj_t *flicker_run_obj, unsigned long identityID);
	void (*get_lib_ver)(struct al_flickerlib_version_t* flicker_LibVersion);
};


struct aflaltek_lib_data {
	struct flicker_output_data_t output_data;
	cmr_u16 success_num;
	struct alhw3a_flicker_cfginfo_t  hwisp_cfg;
	struct al3awrapper_stats_flicker_t stats_data;
};

struct aflaltek_statistics_queue {
	cmr_u32 read;
	cmr_u32 write;
	cmr_u32 size;
	struct isp3a_statistics_data* data[10];
};

/*ae altek context*/
struct aflaltek_cxt {
	cmr_u32 is_inited;
	cmr_u32 camera_id;
	cmr_handle caller_handle;
	cmr_handle lib_handle;
	struct afl_ctrl_init_in init_in_param;

	struct alflickerruntimeobj_t afl_obj;
	void *lib_run_data;
	struct aflaltek_lib_ops lib_ops;
	struct aflaltek_lib_data lib_data;
	struct aflaltek_statistics_queue stat_queue;
};

/**function**/
static cmr_int aflaltek_unload_lib(struct aflaltek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL !!!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	if (cxt_ptr->lib_handle) {
		dlclose(cxt_ptr->lib_handle);
		cxt_ptr->lib_handle = NULL;
	}
	return ISP_SUCCESS;
exit:
	return ret;
}

static cmr_int aflaltek_get_lib_ver(struct aflaltek_cxt *cxt_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct al_flickerlib_version_t lib_ver;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}
	cmr_bzero(&lib_ver, sizeof(lib_ver));
	if (cxt_ptr->lib_ops.get_lib_ver)
		cxt_ptr->lib_ops.get_lib_ver(&lib_ver);
	ISP_LOGI("LIB Ver %04d.%04d", lib_ver.major_version, lib_ver.minor_version);

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aflaltek_load(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_init_in *in_ptr, struct afl_ctrl_init_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	cmr_int is_ret = 0;


	if (!cxt_ptr ||!in_ptr || !out_ptr) {
		ISP_LOGE("init param is null, input_ptr is %p, output_ptr is %p", in_ptr, out_ptr);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	cxt_ptr->lib_handle = dlopen(AFLLIB_PATH, RTLD_NOW);
	if (!cxt_ptr->lib_handle) {
		ISP_LOGE("failed to dlopen");
		ret = ISP_ERROR;
		goto exit;
	}

	cxt_ptr->lib_ops.load_func = dlsym(cxt_ptr->lib_handle, "allib_flicker_loadfunc");
	if (!cxt_ptr->lib_ops.load_func) {
		ISP_LOGE("failed to load func");
		ret = ISP_ERROR;
		goto exit;
	}

	cxt_ptr->lib_ops.get_lib_ver = dlsym(cxt_ptr->lib_handle, "allib_flicker_getlibversion");
	if (!cxt_ptr->lib_ops.get_lib_ver) {
		ISP_LOGE("failed to dlsym get lib func");
		ret = ISP_ERROR;
		goto exit;
	}
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aflaltek_set_init_setting(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;
	struct flicker_set_param_t in_param;
	struct flicker_output_data_t *output_param_ptr = NULL;
	enum flicker_set_param_type_t type = 0;
	struct flicker_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->afl_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->flicker_enable = FLICKERINIT_FLICKER_ENABLE;
	param_ct_ptr->totalqueue = FLICKERINIT_TOTAL_QUEUE;
	param_ct_ptr->refqueue = FLICKERINIT_REF_QUEUE;
	param_ct_ptr->referencepreviousdata = REFERENCE_PREVIOUS_DATA_INTERVAL;
	param_ct_ptr->rawsizex = in_ptr->init.resolution.frame_size.w;
	param_ct_ptr->rawsizey = in_ptr->init.resolution.frame_size.h;
	param_ct_ptr->line_time = in_ptr->init.resolution.line_time * AFL_NSECOND_BASE; //unit: nsecond

	type = FLICKER_SET_PARAM_INIT_SETTING;
	in_param.flicker_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, cxt_ptr->lib_run_data);
	if (lib_ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_set_enable(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;
	struct flicker_set_param_t in_param;
	struct flicker_output_data_t *output_param_ptr = NULL;
	enum flicker_set_param_type_t type = 0;
	struct flicker_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->afl_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->flicker_enable = in_ptr->enable.flicker_enable;

	type = FLICKER_SET_PARAM_ENABLE;
	in_param.flicker_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, cxt_ptr->lib_run_data);
	if (lib_ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_set_current_frequency(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;

	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->afl_obj;

	if (obj_ptr && obj_ptr->set_param)
		lib_ret = al3awrapper_antif_set_flickermode(in_ptr->mode.flicker_mode, obj_ptr, cxt_ptr->lib_run_data);
	if (lib_ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_set_reference_data_interval(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;
	struct flicker_set_param_t in_param;
	struct flicker_output_data_t *output_param_ptr = NULL;
	enum flicker_set_param_type_t type = 0;
	struct flicker_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, in_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->afl_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->referencepreviousdata = in_ptr->ref_data.data_interval;

	type = FLICKER_SET_PARAM_REFERENCE_DATA_INTERVAL;
	in_param.flicker_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, cxt_ptr->lib_run_data);
	if (lib_ret)
		goto exit;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_to_afl_hw_cfg(struct alhw3a_flicker_cfginfo_t *from, struct isp3a_afl_hw_cfg *to)
{
	cmr_int ret = ISP_ERROR;

	if (!from || !to) {
		ISP_LOGE("param %p %p is NULL error!", from, to);
		goto exit;
	}
	to->token_id = from->tokenid;
	to->offset_ratiox = from->uwoffsetratiox;
	to->offset_ratioy = from->uwoffsetratioy;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}

static cmr_int aflaltek_stat_queue_release_all(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct afl_ctrl_callback_in callback_in;
	cmr_u32 data_length;

	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	data_length = ARRAY_SIZE(cxt_ptr->stat_queue.data) - 1;
	while (cxt_ptr->stat_queue.read != cxt_ptr->stat_queue.write) {
		callback_in.stat_data = cxt_ptr->stat_queue.data[cxt_ptr->stat_queue.read];
		cxt_ptr->stat_queue.size --;
		if (data_length == cxt_ptr->stat_queue.read) {
			cxt_ptr->stat_queue.read = 0;
		} else {
			cxt_ptr->stat_queue.read ++;
		}
		cxt_ptr->init_in_param.ops_in.afl_callback(cxt_ptr->caller_handle, AFL_CTRL_CB_STAT_DATA, &callback_in);
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;
}


static cmr_int aflaltek_get_hw_config(struct aflaltek_cxt *cxt_ptr, struct isp3a_afl_hw_cfg *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;
	struct flicker_get_param_t in_param;
	enum flicker_get_param_type_t type = 0;
	struct flicker_get_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !out_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, out_ptr);
		goto exit;
	}

	obj_ptr = &cxt_ptr->afl_obj;
	param_ct_ptr = &in_param.get_param;

	type = FLICKER_GET_ALHW3A_CONFIG;
	in_param.flicker_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, cxt_ptr->lib_run_data);
	if (lib_ret)
		goto exit;
	cxt_ptr->lib_data.hwisp_cfg = in_param.alhw3a_flickerconfig;

	ret = aflaltek_to_afl_hw_cfg(&cxt_ptr->lib_data.hwisp_cfg, out_ptr);
	if (ret)
		goto exit;
	ISP_LOGI("token=%d, ratiox=%d, ratioy=%d",
		out_ptr->token_id,
		out_ptr->offset_ratiox,
		out_ptr->offset_ratioy);
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_get_success_num(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;
	struct flicker_get_param_t in_param;
	enum flicker_get_param_type_t type = 0;
	struct flicker_get_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr) {
		ISP_LOGE("param %p is NULL error!", cxt_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->afl_obj;
	param_ct_ptr = &in_param.get_param;

	type = FLICKER_GET_SUCCESS_NUM;
	in_param.flicker_get_param_type = type;
	if (obj_ptr && obj_ptr->get_param)
		lib_ret = obj_ptr->get_param(&in_param, cxt_ptr->lib_run_data);
	if (lib_ret)
		goto exit;

	cxt_ptr->lib_data.success_num = param_ct_ptr->SuccessNum50;

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_set_work_mode(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_param_in *in_ptr, struct afl_ctrl_param_out *out_ptr)
{
	cmr_int ret = ISP_ERROR;

	cmr_int lib_ret = 0;
	struct alflickerruntimeobj_t *obj_ptr = NULL;
	struct flicker_set_param_t in_param;
	struct flicker_output_data_t *output_param_ptr = NULL;
	enum flicker_set_param_type_t type = 0;
	struct flicker_set_param_content_t *param_ct_ptr = NULL;


	if (!cxt_ptr || !in_ptr || !out_ptr) {
		ISP_LOGE("param %p %p %p is NULL error!", cxt_ptr, in_ptr, out_ptr);
		goto exit;
	}
	obj_ptr = &cxt_ptr->afl_obj;
	output_param_ptr = &cxt_ptr->lib_data.output_data;
	param_ct_ptr = &in_param.set_param;

	param_ct_ptr->line_time = in_ptr->work_param.resolution.line_time * AFL_NSECOND_BASE; //unit: nsecond
	param_ct_ptr->rawsizex = in_ptr->work_param.resolution.frame_size.w;
	param_ct_ptr->rawsizey = in_ptr->work_param.resolution.frame_size.h;

	type = FLICKER_SET_PARAM_LINE_TIME;
	in_param.flicker_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, cxt_ptr->lib_run_data);
	if (lib_ret) {
		ISP_LOGE("set line time failed !!!");
		ret = ISP_ERROR;
		goto exit;
	}

	type = FLICKER_SET_PARAM_RAW_SIZE;
	in_param.flicker_set_param_type = type;
	if (obj_ptr && obj_ptr->set_param)
		lib_ret = obj_ptr->set_param(&in_param, output_param_ptr, cxt_ptr->lib_run_data);
	if (lib_ret) {
		ISP_LOGE("set raw size failed !!!");
		ret = ISP_ERROR;
		goto exit;
	}

	ret = aflaltek_get_hw_config(cxt_ptr, &out_ptr->hw_cfg);
	if (ret)
		goto exit;

	cmr_bzero(&cxt_ptr->stat_queue, sizeof(cxt_ptr->stat_queue));
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld, lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int aflaltek_init(struct aflaltek_cxt *cxt_ptr, struct afl_ctrl_init_in *in_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct afl_ctrl_param_in in_param;
	cmr_int lib_ret = 0;
	cmr_int is_ret = 0;
	unsigned long id = 0;


	id = in_ptr->camera_id;
	is_ret = cxt_ptr->lib_ops.load_func(&cxt_ptr->afl_obj, id);
	if (!is_ret) {
		ret = ISP_ERROR;
		ISP_LOGE("load func is failed!");
		goto exit;
	}

	lib_ret = cxt_ptr->afl_obj.initial(&cxt_ptr->lib_run_data);
	if (lib_ret) {
		ret = ISP_ERROR;
		ISP_LOGE("lib init failed !!!");
		goto exit;
	}
	ISP_LOGI("lib run=%p", cxt_ptr->lib_run_data);

	ret = aflaltek_get_lib_ver(cxt_ptr);
	if (ret)
		goto exit;

	in_param.init.resolution.line_time = in_ptr->init_param.resolution.line_time;
	in_param.init.resolution.frame_size.w = in_ptr->init_param.resolution.frame_size.w;
	in_param.init.resolution.frame_size.h = in_ptr->init_param.resolution.frame_size.h;

	ret = aflaltek_set_init_setting(cxt_ptr, &in_param, NULL);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld lib_ret=%ld !!!", ret, lib_ret);
	return ret;
}

static cmr_int afl_altek_adpt_init(void *in, void *out, cmr_handle *handle)
{
	cmr_int ret = ISP_ERROR;
	struct aflaltek_cxt *cxt_ptr = NULL;
	struct afl_ctrl_init_in *in_ptr = (struct afl_ctrl_init_in*)in;
	struct afl_ctrl_init_out *out_ptr = (struct afl_ctrl_init_out*)out;


	if (!in_ptr || !out_ptr || !handle) {
		ISP_LOGE("init param %p %p %p is NULL error!", in_ptr, out_ptr, handle);
		ret = ISP_PARAM_NULL;
		goto exit;
	}

	*handle = NULL;
	cxt_ptr = (struct aflaltek_cxt*)malloc(sizeof(*cxt_ptr));
	if (NULL == cxt_ptr) {
		ISP_LOGE("failed to create ae ctrl context!");
		ret = ISP_ALLOC_ERROR;
		goto exit;
	}
	cmr_bzero(cxt_ptr, sizeof(*cxt_ptr));

	cxt_ptr->camera_id = in_ptr->camera_id;
	cxt_ptr->caller_handle = in_ptr->caller_handle;
	cxt_ptr->init_in_param = *in_ptr;

	ret = aflaltek_load(cxt_ptr, in_ptr, out_ptr);
	if (ret) {
		goto exit;
	}

	ret = aflaltek_init(cxt_ptr, in_ptr);
	if (ret) {
		goto exit;
	}

	ret = aflaltek_get_hw_config(cxt_ptr, &out_ptr->hw_cfg);
	if (ret)
		goto exit;
	*handle = (cmr_handle)cxt_ptr;
	cxt_ptr->is_inited = 1;
	return ISP_SUCCESS;
exit:
	if (cxt_ptr) {
		ret = aflaltek_unload_lib(cxt_ptr);
		if (ret)
			ISP_LOGW("unload lib failed ret=%ld", ret);

		free((void*)cxt_ptr);
	}
	ISP_LOGE("ret=%ld !!!", ret);
	return ret;
}

static cmr_int afl_altek_adpt_deinit(cmr_handle handle)
{
	cmr_int ret = ISP_ERROR;
	struct aflaltek_cxt *cxt_ptr = (struct aflaltek_cxt*)handle;

	ISP_CHECK_HANDLE_VALID(handle);

	if (0 == cxt_ptr->is_inited) {
		ISP_LOGW("has been de-initialized");
		goto exit;
	}
	if (cxt_ptr->afl_obj.deinit)
		cxt_ptr->afl_obj.deinit(cxt_ptr->lib_run_data);

	ret = aflaltek_unload_lib(cxt_ptr);
	if (ret)
		ISP_LOGW("unload lib failed ret=%ld", ret);

	free((void*)handle);
	return ISP_SUCCESS;
exit:
	ISP_LOGI("ret=%ld !!!", ret);
	return ret;
}

static cmr_int afl_altek_adpt_ioctrl(cmr_handle handle, cmr_int cmd, void *in, void *out)
{
	cmr_int ret = ISP_SUCCESS;
	struct aflaltek_cxt *cxt_ptr = (struct aflaltek_cxt*)handle;
	struct afl_ctrl_param_in *in_ptr = (struct afl_ctrl_param_in*)in;
	struct afl_ctrl_param_out *out_ptr = (struct afl_ctrl_param_out*)out;


	if (!handle) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}

	switch (cmd) {
	case AFL_CTRL_SET_WORK_MODE:
		ret = aflaltek_set_work_mode(cxt_ptr, in_ptr, out_ptr);
		break;
	case AFL_CTRL_SET_FLICKER:
		ret = aflaltek_set_current_frequency(cxt_ptr, in_ptr, out_ptr);
		break;
	case AFL_CTRL_SET_ENABLE:
		ret = aflaltek_set_enable(cxt_ptr, in_ptr, out_ptr);
		break;
	case AFL_CTRL_SET_PREVIOUS_DATA_INTERVAL:
		ret = aflaltek_set_reference_data_interval(cxt_ptr, in_ptr, out_ptr);
		break;
	case AFL_CTRL_GET_SUCCESS_NUM:
		ret = aflaltek_get_success_num(cxt_ptr, in_ptr, out_ptr);
		break;
	case AFL_CTRL_SET_STAT_QUEUE_RELEASE:
		ret = aflaltek_stat_queue_release_all(cxt_ptr, in_ptr, out_ptr);
		break;
	default:
		ISP_LOGE("cmd %ld is not defined!", cmd);
		break;
	};

exit:
	return ret;
}

static cmr_int aflaltek_stat_queue_process(struct aflaltek_cxt *cxt_ptr, struct isp3a_statistics_data *stat_data_ptr)
{
	cmr_int ret = ISP_ERROR;
	struct afl_ctrl_callback_in callback_in;
	cmr_u32 data_length;

	if (!cxt_ptr || !stat_data_ptr) {
		ISP_LOGE("param %p %p is NULL error!", cxt_ptr, stat_data_ptr);
		goto exit;
	}

	data_length = ARRAY_SIZE(cxt_ptr->stat_queue.data) - 1;
	ISP_LOGI("add stat_data =%p,size:%d", stat_data_ptr,cxt_ptr->stat_queue.size);
	cxt_ptr->stat_queue.data[cxt_ptr->stat_queue.write] = stat_data_ptr;
	cxt_ptr->stat_queue.size ++;
	if (data_length == cxt_ptr->stat_queue.write) {
		cxt_ptr->stat_queue.write = 0;
	} else {
		cxt_ptr->stat_queue.write ++;
	}

	if (cxt_ptr->stat_queue.size > FLICKERINIT_TOTAL_QUEUE) {
		callback_in.stat_data = cxt_ptr->stat_queue.data[cxt_ptr->stat_queue.read];
		cxt_ptr->stat_queue.size --;
		if (data_length == cxt_ptr->stat_queue.read) {
			cxt_ptr->stat_queue.read = 0;
		} else {
			cxt_ptr->stat_queue.read ++;
		}
		cxt_ptr->init_in_param.ops_in.afl_callback(cxt_ptr->caller_handle, AFL_CTRL_CB_STAT_DATA, &callback_in);
	}

	return ISP_SUCCESS;
exit:
	ISP_LOGE("ret=%ld", ret);
	return ret;

}

static cmr_int afl_altek_adpt_process(cmr_handle handle, void *in, void *out)
{
	cmr_int ret = ISP_ERROR;
	cmr_int lib_ret = 0;
	struct aflaltek_cxt *cxt_ptr = (struct aflaltek_cxt*)handle;
	struct afl_ctrl_proc_in *in_ptr = (struct afl_ctrl_proc_in*)in;
	struct afl_ctrl_proc_out *out_ptr = (struct afl_ctrl_proc_out*)out;
	struct afl_ctrl_callback_in callback_in;
	enum ae_antiflicker_mode_t temp_flicker_mode;

	if (!handle || !in) {
		ISP_LOGE("param is NULL error!");
		goto exit;
	}
	if (NULL == in_ptr->stat_data_ptr->addr) {
		ISP_LOGI("hw stat data is NULL error!!");
		goto exit;
	}
	lib_ret = al3awrapper_dispatchhw3a_flickerstats((struct isp_drv_meta_antif_t*)in_ptr->stat_data_ptr->addr
			, &cxt_ptr->lib_data.stats_data, &cxt_ptr->afl_obj, cxt_ptr->lib_run_data);
	if (ERR_WPR_FLICKER_SUCCESS != lib_ret) {
		ret = ISP_ERROR;
		ISP_LOGE("dispatch lib_ret=%ld", lib_ret);
		goto exit;
	}

	lib_ret = cxt_ptr->afl_obj.process(&cxt_ptr->lib_data.stats_data, cxt_ptr->lib_run_data, &cxt_ptr->lib_data.output_data);
	if (lib_ret)
		goto exit;
	if (cxt_ptr->init_in_param.ops_in.afl_callback) {
		ISP_LOGI("FinalFreq=%d", cxt_ptr->lib_data.output_data.finalfreq);
		lib_ret = al3awrapper_antif_get_flickermode(cxt_ptr->lib_data.output_data.finalfreq, &temp_flicker_mode);
		if (ERR_WPR_FLICKER_SUCCESS == lib_ret) {
			callback_in.flicker_mode = temp_flicker_mode;
			//cxt_ptr->init_in_param.ops_in.afl_callback(cxt_ptr->caller_handle, AFL_CTRL_CB_FLICKER_MODE, &callback_in);
		}
	}

	ret = aflaltek_stat_queue_process(cxt_ptr, in_ptr->stat_data_ptr);
	if (ret)
		goto exit;
	return ISP_SUCCESS;
exit:
	ISP_LOGI("done %ld, lib_ret=%ld", ret, lib_ret);
	return ret;
}

/*************************************ADAPTER CONFIGURATION ***************************************/
static struct isp_lib_config afl_altek_lib_info = {
	.product_id = 0,
	.version_id = 0,
};

static struct adpt_ops_type afl_altek_adpt_ops = {
	.adpt_init = afl_altek_adpt_init,
	.adpt_deinit = afl_altek_adpt_deinit,
	.adpt_process = afl_altek_adpt_process,
	.adpt_ioctrl = afl_altek_adpt_ioctrl,
};

struct adpt_register_type afl_altek_adpt_info = {
	.lib_type = ADPT_LIB_AFL,
	.lib_info = &afl_altek_lib_info,
	.ops = &afl_altek_adpt_ops,
};
