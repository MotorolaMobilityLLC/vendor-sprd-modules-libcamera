#include "lsc_adv.h"
#include "isp_adpt.h"
#include "isp_dev_access.h"

#include <dlfcn.h>

#define SMART_LSC_VERSION 1

#define UNUSED(param)  (void)(param)


/************************************* INTERNAL DATA TYPE ***************************************/
struct lsc_ctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct lsc_ctrl_cxt {
	cmr_handle thr_handle;
	cmr_handle caller_handle;
	struct lsc_ctrl_work_lib work_lib;
	struct lsc_adv_calc_result proc_out;
};

char liblsc_path[][20] = {
	"liblsc.so",
	"liblsc_v1.so",
	"liblsc_v2.so",
	"liblsc_v3.so",
	"liblsc_v4.so",
	"liblsc_v5.so",
};

static int32_t _lscctrl_deinit_adpt(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_deinit) {
		rtn = lib_ptr->adpt_ops->adpt_deinit(lib_ptr->lib_handle, NULL, NULL);
	} else {
		ISP_LOGI(":ISP:adpt_deinit fun is NULL");
	}

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}


static int32_t _lscctrl_destroy_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param , param is NULL");
		rtn = LSC_ERROR;
		goto exit;
	}

	if (cxt_ptr->thr_handle) {
		rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
			cxt_ptr->thr_handle = NULL;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static int32_t _lscctrl_process(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param,param is NULL!");
		goto exit;
	}
	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_process) {
		rtn = lib_ptr->adpt_ops->adpt_process(lib_ptr->lib_handle, in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:process fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static int32_t _lscctrl_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *handle = (struct lsc_ctrl_cxt *)p_data;

	if (!message || !p_data) {
		ISP_LOGE("fail to chcek param");
		goto exit;
	}
	ISP_LOGI(":ISP:message.msg_type 0x%x, data %p", message->msg_type,
		 message->data);

	switch (message->msg_type) {
	case LSCCTRL_EVT_INIT:
		break;
	case LSCCTRL_EVT_DEINIT:
		break;
	case LSCCTRL_EVT_IOCTRL:
		break;
	case LSCCTRL_EVT_PROCESS:  // ISP_PROC_LSC_CALC
		rtn = _lscctrl_process(handle, (struct lsc_adv_calc_param*)message->data, &handle->proc_out);
		break;
	default:
		ISP_LOGE("fail to proc,don't support msg");
		break;
	}

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static int32_t _lscctrl_init_lib(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param ,param is NULL!");
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_init) {
		lib_ptr->lib_handle = lib_ptr->adpt_ops->adpt_init(in_ptr, NULL);
	} else {
		ISP_LOGI(":ISP:adpt_init fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static int32_t _lscctrl_init_adpt(struct lsc_ctrl_cxt *cxt_ptr, struct lsc_adv_init_param *in_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		goto exit;
	}

	/* find vendor adpter */
	rtn  = adpt_get_ops(ADPT_LIB_LSC, &in_ptr->lib_param, &cxt_ptr->work_lib.adpt_ops);
	if (rtn) {
		ISP_LOGE("fail to get adapter layer ret = %ld", rtn);
		goto exit;
	}

	rtn = _lscctrl_init_lib(cxt_ptr, in_ptr);
exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

static int32_t _lscctrl_create_thread(struct lsc_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = LSC_SUCCESS;

	rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, _lscctrl_ctrl_thr_proc, (void*)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create ctrl thread");
		rtn = LSC_ERROR;
	}

	ISP_LOGI(":ISP:lsc_ctrl thread rtn %ld", rtn);
	return rtn;
}

static int32_t _lscsprd_unload_lib(struct lsc_ctrl_context *cxt)
{
	int32_t rtn = LSC_SUCCESS;

	if (NULL == cxt) {
		ISP_LOGE("fail to check param, Param is NULL");
		rtn = LSC_PARAM_NULL;
		goto exit;
	}

	if (!cxt->lib_handle) {
		dlclose(cxt->lib_handle);
		cxt->lib_handle = NULL;
	}

exit:
	return rtn;
}

static int32_t _lscsprd_load_lib(struct lsc_ctrl_context *cxt)
{
	int32_t rtn = LSC_SUCCESS;
	int32_t v_count = 0;
	uint32_t version_id = cxt->lib_info->version_id;

	if (NULL == cxt) {
		ISP_LOGE("fail to check param,Param is NULL");
		rtn = LSC_PARAM_NULL;
		goto exit;
	}

	v_count = sizeof(liblsc_path) / sizeof(liblsc_path[0]);
	if (version_id >= v_count) {
		ISP_LOGE("fail to get lsc lib version , version_id :%d", version_id);
		rtn = LSC_ERROR;
		goto exit;
	}

	ISP_LOGI(":ISP:lib lsc v_count : %d, version id: %d, libae path :%s", v_count ,version_id, liblsc_path[version_id]);

	cxt->lib_handle = dlopen(liblsc_path[version_id], RTLD_NOW);
	if (!cxt->lib_handle) {
		ISP_LOGE("fail to dlopen lsc lib");
		rtn = LSC_ERROR;
		goto exit;
	}

	cxt->lib_ops.alsc_init = dlsym(cxt->lib_handle, "lsc_adv_init");
	if (!cxt->lib_ops.alsc_init) {
		ISP_LOGE("fail to dlsym lsc_sprd_init");
		rtn = LSC_ERROR;
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_calc = dlsym(cxt->lib_handle, "lsc_adv_calculation");
	if (!cxt->lib_ops.alsc_calc) {
		ISP_LOGE("fail to dlsym lsc_sprd_calculation");
		rtn = LSC_ERROR;
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_io_ctrl = dlsym(cxt->lib_handle, "lsc_adv_ioctrl");
	if (!cxt->lib_ops.alsc_io_ctrl) {
		ISP_LOGE("fail to dlsym lsc_sprd_io_ctrl");
		rtn = LSC_ERROR;
		goto error_dlsym;
	}

	cxt->lib_ops.alsc_deinit = dlsym(cxt->lib_handle, "lsc_adv_deinit");
	if (!cxt->lib_ops.alsc_deinit) {
		ISP_LOGE("fail to dlsym lsc_sprd_deinit");
		rtn = LSC_ERROR;
		goto error_dlsym;
	}
	ISP_LOGI(":ISP:load lsc lib success");

	return LSC_SUCCESS;

error_dlsym:
	rtn = _lscsprd_unload_lib(cxt);

exit:
	ISP_LOGE("fail to load lsc lib ret = %d", rtn);
	return rtn;
}

static void* lsc_sprd_init(void *in, void *out)
{
	int32_t rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_init_param *init_param = (struct lsc_adv_init_param *)in;
	void *alsc_handle = NULL;

	cxt = (struct lsc_ctrl_context *)malloc(sizeof(struct lsc_ctrl_context));
	if (NULL == cxt) {
		rtn = LSC_ALLOC_ERROR;
		ISP_LOGE("fail to alloc!");
		goto EXIT;
	}

	memset(cxt, 0, sizeof(*cxt));

	cxt->lib_info = &init_param->lib_param;

	rtn = _lscsprd_load_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to load lsc lib");
		goto EXIT;
	}

	alsc_handle = cxt->lib_ops.alsc_init(init_param);
	if (NULL == alsc_handle) {
		ISP_LOGE("fail to do alsc init!");
		rtn = LSC_ALLOC_ERROR;
		goto EXIT;
	}

	cxt->alsc_handle = alsc_handle;

	pthread_mutex_init(&cxt->status_lock, NULL);

	ISP_LOGI(":ISP:isp_3a_ctrl lsc_init rtn = %d", rtn);
	return (void *)cxt;

EXIT:

	if (NULL != alsc_handle) {
		rtn = cxt->lib_ops.alsc_deinit(alsc_handle);
		alsc_handle = NULL;
	}

	if (NULL != cxt) {
		rtn = _lscsprd_unload_lib(cxt);
		free(cxt);
		cxt = NULL;
	}

	ISP_LOGI(":ISP:done rtn = %d", rtn);

	return NULL;
}

static int32_t lsc_sprd_deinit(void *handle, void *in, void *out)
{
	int32_t rtn =LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	UNUSED(in);
	UNUSED(out);

	if (!handle) {
		ISP_LOGE("fail to check param!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context *)handle;

	rtn = cxt->lib_ops.alsc_deinit(cxt->alsc_handle);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to do alsc deinit!");
	}

	rtn = _lscsprd_unload_lib(cxt);
	if (LSC_SUCCESS != rtn) {
		ISP_LOGE("fail to unload lsc lib");
	}

	pthread_mutex_destroy(&cxt->status_lock);

	memset(cxt, 0, sizeof(*cxt));
	free(cxt);
	ISP_LOGI(":ISP:done rtn = %d", rtn);

	return rtn;
}

static int32_t lsc_sprd_calculation(void* handle, void *in, void *out)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;
	struct lsc_adv_calc_param *param = (struct lsc_adv_calc_param *)in;
	struct lsc_adv_calc_result *result = (struct lsc_adv_calc_result *)out;

	if (!handle) {
		ISP_LOGE("fail to check param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context*)handle;

	rtn = cxt->lib_ops.alsc_calc(cxt->alsc_handle, param, result);

	return rtn;
}

static int32_t lsc_sprd_ioctrl(void* handle, enum alsc_io_ctrl_cmd cmd,void *in, void *out)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_context *cxt = NULL;

	if (!handle) {
		ISP_LOGE("fail to check param, param is NULL!");
		return LSC_ERROR;
	}

	cxt = (struct lsc_ctrl_context*)handle;

	rtn = cxt->lib_ops.alsc_io_ctrl(cxt->alsc_handle, cmd, in, out);

	return rtn;
}

/*
void alsc_set_param(struct lsc_adv_init_param *lsc_param)
{
	//	alg_open
	//	0: front_camera close, back_camera close;
	//	1: front_camera open, back_camera open;
	//	2: front_camera close, back_camera open;
	lsc_param->alg_open = 1;
       	lsc_param->tune_param.enable = 1;
#if SMART_LSC_VERSION
	lsc_param->tune_param.alg_id = 2;
#else
	lsc_param->tune_param.alg_id = 0;
#endif
	//common
	lsc_param->tune_param.strength_level = 5;
	lsc_param->tune_param.pa = 1;
	lsc_param->tune_param.pb = 1;
	lsc_param->tune_param.fft_core_id = 0;
	lsc_param->tune_param.con_weight =	5;		//double avg weight:[1~16]; weight =100 for 10 tables avg
	lsc_param->tune_param.restore_open = 0;
	lsc_param->tune_param.freq = 3;

	//Smart LSC Setting
	lsc_param->SLSC_setting.num_seg_queue = 10;
	lsc_param->SLSC_setting.num_seg_vote_th = 8;
	lsc_param->SLSC_setting.proc_inter = 3;
	lsc_param->SLSC_setting.seg_count = 4;
	lsc_param->SLSC_setting.smooth_ratio = 0.9;
	lsc_param->SLSC_setting.segs[0].table_pair[0]=LSC_TAB_A;
	lsc_param->SLSC_setting.segs[0].table_pair[1]=LSC_TAB_TL84;
	lsc_param->SLSC_setting.segs[0].max_CT = 3999;
	lsc_param->SLSC_setting.segs[0].min_CT = 0;
	lsc_param->SLSC_setting.segs[1].table_pair[0]=LSC_TAB_A;
	lsc_param->SLSC_setting.segs[1].table_pair[1]=LSC_TAB_CWF;
	lsc_param->SLSC_setting.segs[1].max_CT = 3999;
	lsc_param->SLSC_setting.segs[1].min_CT = 0;
	lsc_param->SLSC_setting.segs[2].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[2].table_pair[1]=LSC_TAB_TL84;
	lsc_param->SLSC_setting.segs[2].max_CT = 10000;
	lsc_param->SLSC_setting.segs[2].min_CT = 0;
	lsc_param->SLSC_setting.segs[3].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[3].table_pair[1]=LSC_TAB_CWF;
	lsc_param->SLSC_setting.segs[3].max_CT = 10000;
	lsc_param->SLSC_setting.segs[3].min_CT = 0;

	lsc_param->SLSC_setting.segs[4].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[4].table_pair[1]=LSC_TAB_DNP;
	lsc_param->SLSC_setting.segs[4].max_CT = 10000;
	lsc_param->SLSC_setting.segs[4].min_CT = 0;
}
*/


cmr_int lsc_ctrl_init(struct lsc_adv_init_param *input_ptr, cmr_handle *handle_lsc)
{
	cmr_int                         rtn = ISP_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = NULL;

	cxt_ptr = (struct lsc_ctrl_cxt*)malloc(sizeof(struct lsc_ctrl_cxt));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create lsc ctrl context!");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(struct lsc_ctrl_cxt));

	rtn = _lscctrl_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	rtn = _lscctrl_init_adpt(cxt_ptr, input_ptr);
	if (rtn) {
		goto exit;
	}

exit:
	if (rtn) {
		if (cxt_ptr) {
			free(cxt_ptr);
		}
	} else {
		*handle_lsc = (cmr_handle)cxt_ptr;
	}
	ISP_LOGI(":ISP:done %ld", rtn);

	return rtn;
}

cmr_int lsc_ctrl_deinit(cmr_handle *handle_lsc)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = *handle_lsc;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
	}

	rtn = _lscctrl_deinit_adpt(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to deinit lscctrl adpt %ld", rtn );
		rtn = _lscctrl_destroy_thread(cxt_ptr);
		if (rtn)
			ISP_LOGE("fail to destroy lscctrl thread %ld", rtn );
		goto exit;
	}

	rtn = _lscctrl_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy lscctrl thread %ld", rtn );
		goto exit;
	}

exit:
	if (cxt_ptr) {
		free((void*)cxt_ptr);
		*handle_lsc = NULL;
	}

	ISP_LOGI(":ISP:done %d", rtn);
	return rtn;
}

cmr_int lsc_ctrl_process(cmr_handle handle_lsc, struct lsc_adv_calc_param *in_ptr, struct lsc_adv_calc_result *result)
{
	cmr_int                         rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt*)handle_lsc;

	if (!handle_lsc) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	cxt_ptr->proc_out.dst_gain = result->dst_gain;

	CMR_MSG_INIT(message);
	message.data = malloc(sizeof(struct lsc_adv_calc_param));
	if (!message.data) {
		ISP_LOGE("fail to malloc msg");
		rtn = LSC_ALLOC_ERROR;
		goto exit;
	}

	memcpy(message.data, (void *)in_ptr, sizeof(struct lsc_adv_calc_param));
	message.alloc_flag = 1;
	message.msg_type = LSCCTRL_EVT_PROCESS;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	rtn = cmr_thread_msg_send(cxt_ptr->thr_handle, &message);

	if (rtn) {
		ISP_LOGE("fail to send msg to main thr %ld", rtn);
		if (message.data)
			free(message.data);
		goto exit;
	}

exit:
	ISP_LOGI(":ISP:done %ld", rtn);
	return rtn;
}

cmr_int lsc_ctrl_ioctrl(cmr_handle handle_lsc, enum alsc_io_ctrl_cmd cmd, void *in_ptr, void *out_ptr)
{
	cmr_int rtn = LSC_SUCCESS;
	struct lsc_ctrl_cxt *cxt_ptr = (struct lsc_ctrl_cxt*)handle_lsc;
	struct lsc_ctrl_work_lib *lib_ptr = NULL;

	if (!handle_lsc) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = LSC_HANDLER_NULL;
		goto exit;
	}

	lib_ptr = &cxt_ptr->work_lib;
	if (lib_ptr->adpt_ops->adpt_ioctrl) {
		rtn = lib_ptr->adpt_ops->adpt_ioctrl(lib_ptr->lib_handle, cmd, in_ptr, out_ptr);
	} else {
		ISP_LOGI(":ISP:ioctrl fun is NULL");
	}
exit:
	ISP_LOGI(":ISP:cmd = %d,done %ld", cmd, rtn);
	return rtn;
}


struct adpt_ops_type lsc_sprd_adpt_ops_ver0 = {
	.adpt_init = lsc_sprd_init,
	.adpt_deinit = lsc_sprd_deinit,
	.adpt_process = lsc_sprd_calculation,
	.adpt_ioctrl = lsc_sprd_ioctrl,
};

