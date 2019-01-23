#include "sprd_hdr_adapter.h"
#include "sprd_hdr_adapter_log.h"

#ifdef CONFIG_SPRD_HDR_LIB
#include "HDR_SPRD.h"
#endif

#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
#include "sprd_hdr_api.h"
#endif

#ifdef DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

void *sprd_hdr_adpt_init(int max_width, int max_height, void *param)
{
	void *handle = 0;
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
	hdr_config_t cfg;
	sprd_hdr_config_default(&cfg);
	cfg.max_width = max_width;
	cfg.max_height = max_height;
	cfg.img_width = max_width;
	cfg.img_height = max_height;
	cfg.img_stride = max_width;
	
	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
		sprd_hdr_open(&handle, &cfg);
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
		sprd_hdr_vdsp_open(&handle, &cfg);
#endif

#ifdef CONFIG_SPRD_HDR_LIB
	if (sprd_hdr_pool_init())
		return 0;
	handle = (void *)-1;
#endif
	return handle;
}

int sprd_hdr_adpt_deinit(void *handle)
{
	int ret = 0;
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
	if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
		ret = sprd_hdr_close(handle);
	else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
		ret = sprd_hdr_vdsp_close(handle);
#endif

#ifdef CONFIG_SPRD_HDR_LIB
	ret = sprd_hdr_pool_destroy();
#endif
	return ret;
}

int sprd_hdr_adpt_ctrl(void *handle, sprd_hdr_cmd_t cmd, void *param)
{
	int ret = 0;
	switch (cmd) {
	case SPRD_HDR_GET_VERSION_CMD: {
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
		hdr_version_t *version = (hdr_version_t *)param;
		ret = sprd_hdr_version(version);
#endif
		break;
	}
	case SPRD_HDR_PROCESS_CMD: {
		sprd_hdr_param_t *hdr_param = (sprd_hdr_param_t *)param;
		int i = 0;
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
		ldr_image_t input[2];
		if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) {
			for (i = 0; i < 2; i++) {
				input[i].data = (uint8_t *)hdr_param->input[i].addr[0];
				input[i].width = hdr_param->input[i].width;
				input[i].height = hdr_param->input[i].height;
				input[i].stride = hdr_param->input[i].stride;
				input[i].ev = hdr_param->ev[i];
			}
			
			uint8_t *output = (uint8_t *)hdr_param->output.addr[0];
			
			ret = sprd_hdr_process(handle, input, output);
		} else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP) {
			ldr_image_vdsp_t input_vdsp[2], output_vdsp;
			for (i = 0; i < 2; i++) {
				input_vdsp[i].image.data = (uint8_t *)hdr_param->input[i].addr[0];
				input_vdsp[i].image.width = hdr_param->input[i].width;
				input_vdsp[i].image.height = hdr_param->input[i].height;
				input_vdsp[i].image.stride = hdr_param->input[i].stride;
				input_vdsp[i].image.ev = hdr_param->ev[i];
				input_vdsp[i].fd = hdr_param->input[i].ion_fd;
			}
			
			output_vdsp.image.data = (uint8_t *)hdr_param->output.addr[0];
			output_vdsp.image.width = hdr_param->output.width;
			output_vdsp.image.height = hdr_param->output.height;
			output_vdsp.image.stride = hdr_param->output.stride;
			output_vdsp.fd = hdr_param->output.ion_fd;
			
			ret = sprd_hdr_vdsp_process(handle, input_vdsp, &output_vdsp);
		}
#endif

#ifdef CONFIG_SPRD_HDR_LIB
		BYTE *Y0 = (BYTE *)hdr_param->input[0].addr[0];
		BYTE *Y1 = (BYTE *)hdr_param->input[1].addr[0];
		BYTE *Y2 = (BYTE *)hdr_param->input[2].addr[0];
		BYTE *output = (BYTE *)hdr_param->output.addr[0];
		int height = hdr_param->input[0].height;
		int width = hdr_param->input[0].width;
		ret = HDR_Function(Y0, Y1, Y2, output, height, width, "YVU420_SEMIPLANAR");
#endif
		break;
	}
	case SPRD_HDR_FAST_STOP_CMD: {
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
		ret = sprd_hdr_fast_stop(handle);
#endif

#ifdef CONFIG_SPRD_HDR_LIB
		sprd_hdr_set_stop_flag(HDR_STOP);
#endif
		break;
	}
	default:
		HDR_LOGW("unknown cmd: %d\n", cmd);
		break;
	}
	
	return ret;
}

int sprd_hdr_get_devicetype(enum camalg_run_type *type)
{
	if (!type)
		return 1;
#ifdef CONFIG_SPRD_HDR_LIB_VERSION_2
	*type = g_run_type;
#endif

#ifdef CONFIG_SPRD_HDR_LIB
	*type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif
	return 0;
}

int sprd_hdr_set_devicetype(enum camalg_run_type type)
{
	if (type < SPRD_HDR_GET_VERSION_CMD || type >= SPRD_HDR_MAX_CMD)
		return 1;
	g_run_type = type;
	
	return 0;
}