#include "sprd_hdr_adapter.h"
#include "sprd_hdr_adapter_log.h"
#include "properties.h"
#include <string.h>
#include "sprd_camalg_assist.h"

#ifdef CONFIG_SPRD_HDR_LIB
#include "HDR_SPRD.h"
#endif

#include "sprd_hdr_api.h"
#include "sprd_hdr3_api.h"

#ifdef DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

#ifndef CONFIG_SPRD_HDR_LIB
int g_hdr_version = DEFAULT_HDR_VERSION;
#endif

int hdr_adapt_get_exif_size()
{
   int ret = 0;
#ifndef CONFIG_SPRD_HDR_LIB
    if (g_hdr_version == 2)
       ret = sizeof(hdr_exif_info_t);
#endif
    return ret;
}

int hdr_adapt_get_exif_info(void *handle, sprd_hdr_get_exif_info_param_t *param)
{
    int ret = 0;
#ifndef CONFIG_SPRD_HDR_LIB
    if (g_hdr_version == 2) {
        hdr_exif_info_t *exif_info = (hdr_exif_info_t *)param->exif_info;
        ret = sprd_hdr_get_exif_info(handle, exif_info);
        HDR_LOGI("exif_info %p %d %d %d", exif_info, exif_info->gain_ae[0], exif_info->gain_ae[1], exif_info->gain_ae[2]);
    }
#endif
    return ret;
}

int hdr_adapt_process(void *handle, sprd_hdr_param_t *hdr_param)
{
    int ret = 0;
    int i = 0;

#ifdef CONFIG_SPRD_HDR_LIB
    BYTE *Y0 = (BYTE *)hdr_param->input[0].addr[0];
    BYTE *Y1 = (BYTE *)hdr_param->input[1].addr[0];
    BYTE *Y2 = (BYTE *)hdr_param->input[2].addr[0];
    BYTE *output = (BYTE *)hdr_param->output.addr[0];
    int height = hdr_param->input[0].height;
    int width = hdr_param->input[0].width;
    ret = HDR_Function(Y0, Y1, Y2, output, height, width, "YVU420_SEMIPLANAR");
#else
    if (g_hdr_version == 2) {
        HDR_LOGI("handle: %p, hdr_param: %p, input_frame_num: %d\n", handle, hdr_param, hdr_param->input_frame_num);
        ldr_image_t input[2];
        hdr_callback_t callback;
        callback.tuning_param_index = hdr_param->detect_out->tuning_param_index;
        callback.clock = hdr_param->detect_out->clock;
        callback.exif_info.bv = hdr_param->detect_out->bv;
        callback.exif_info.face_num = hdr_param->detect_out->face_num;
        callback.exif_info.prop_dark = hdr_param->detect_out->prop_dark;
        callback.exif_info.prop_bright = hdr_param->detect_out->prop_bright;
        callback.exif_info.sceneChosen = hdr_param->detect_out->sceneChosen;
        callback.exif_info.scene_flag = hdr_param->detect_out->scene_flag;
        for(i = 0; i < 3; i++){
            callback.exif_info.shutter_ae[i] = hdr_param->detect_out->shutter[i];
            callback.exif_info.gain_ae[i] = hdr_param->detect_out->gain[i];
        }

        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) {
            for (i = 0; i < 2; i++) {
                HDR_LOGI("input[%d]: %p\n", i, hdr_param->input[i].addr[0]);
                input[i].data = (uint8_t *)hdr_param->input[i].addr[0];
                input[i].width = hdr_param->input[i].width;
                input[i].height = hdr_param->input[i].height;
                input[i].stride = hdr_param->input[i].stride;
                input[i].ev = hdr_param->detect_out->ev[i];
                input[i].callback = &callback;
            }

            uint8_t *output = (uint8_t *)hdr_param->output.addr[0];
            HDR_LOGI("[%lld]ev: %f, %f, tuning_param_index: %d\n", callback.clock, hdr_param->detect_out->ev[0], hdr_param->detect_out->ev[1], callback.tuning_param_index);
            ret = sprd_hdr_process(handle, input, output);
        } else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP) {
            ldr_image_vdsp_t input_vdsp[2], output_vdsp;
            for (i = 0; i < 2; i++) {
                HDR_LOGI("input[%d]: %p\n", i, hdr_param->input[i].addr[0]);
                input_vdsp[i].image.data = (uint8_t *)hdr_param->input[i].addr[0];
                input_vdsp[i].image.width = hdr_param->input[i].width;
                input_vdsp[i].image.height = hdr_param->input[i].height;
                input_vdsp[i].image.stride = hdr_param->input[i].stride;
                input_vdsp[i].image.ev = hdr_param->detect_out->ev[i];
                input_vdsp[i].image.callback = &callback;
                input_vdsp[i].fd = hdr_param->input[i].ion_fd;
            }

            output_vdsp.image.data = (uint8_t *)hdr_param->output.addr[0];
            output_vdsp.image.width = hdr_param->output.width;
            output_vdsp.image.height = hdr_param->output.height;
            output_vdsp.image.stride = hdr_param->output.stride;
            output_vdsp.fd = hdr_param->output.ion_fd;
            HDR_LOGI("[%lld]ev: %f, %f, tuning_param_index: %d\n", callback.clock, hdr_param->detect_out->ev[0], hdr_param->detect_out->ev[1], callback.tuning_param_index);
            ret = sprd_hdr_vdsp_process(handle, input_vdsp, &output_vdsp);
        }
    } else if (g_hdr_version == 3) {
        HDR_LOGI("handle: %p, hdr_param: %p, input_frame_num: %d\n", handle, hdr_param, hdr_param->input_frame_num);
        hdr3_Image_t input[HDR_IMG_NUM_MAX];
        hdr3_Image_t output;
        hdr3_param_info_t  hdr3_param;
        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU) {
            for (i = 0; i < hdr_param->input_frame_num; i++) {
                HDR_LOGI("input[%d]: %p\n", i, hdr_param->input[i].addr[0]);
                input[i].data[0] = (uint8_t *)hdr_param->input[i].addr[0];
                input[i].width = hdr_param->input[i].width;
                input[i].height = hdr_param->input[i].height;
                input[i].stride = hdr_param->input[i].stride;
                input[i].type = hdr_param->input[i].format;
                input[i].ev = hdr_param->detect_out->ev[i];
            }
            hdr3_param.tuning_param_index = hdr_param->detect_out->tuning_param_index;
            hdr3_param.input_frame_num = hdr_param->input_frame_num;
            hdr3_param.clock = hdr_param->detect_out->clock;
            hdr3_param.face_num = hdr_param->face_num;
            for (i = 0; i < hdr_param->face_num; i++) {
                hdr3_param.face_info[i].width = hdr_param->face_info[i].width;
                hdr3_param.face_info[i].height = hdr_param->face_info[i].height;
                hdr3_param.face_info[i].humanID = hdr_param->face_info[i].humanId;
                hdr3_param.face_info[i].rollAngle = hdr_param->face_info[i].rollAngle;
                hdr3_param.face_info[i].score = hdr_param->face_info[i].score;
                hdr3_param.face_info[i].startX = hdr_param->face_info[i].startX;
                hdr3_param.face_info[i].startY = hdr_param->face_info[i].starty;
                hdr3_param.face_info[i].yawAngle = hdr_param->face_info[i].yawAngle;
            }

            HDR_LOGI("ev: %f, %f, %f, tuning_param_size: %d, input_frame_num: %d\n", hdr_param->detect_out->ev[0], hdr_param->detect_out->ev[1], hdr_param->detect_out->ev[2], hdr3_param.tuning_param_index, hdr3_param.input_frame_num);

            output.data[0] = (uint8_t *)hdr_param->output.addr[0];
            output.width = hdr_param->output.width;
            output.height = hdr_param->output.height;
            HDR_LOGI("output: %p\n", hdr_param->output.addr[0]);
            ret = sprd_hdr3_run(handle, input, &output, &hdr3_param);
        }
    }
#endif

    return ret;
}

int sprd_hdr_adpt_detect(sprd_hdr_detect_in_t *detect_in_param, sprd_hdr_detect_out_t *detect_out_param, sprd_hdr_status_t *status)
{
    int ret = 0;
    char vers[256];

    property_get("persist.vendor.cam.hdr.version", vers , "2");

    if (!(strcmp("2", vers)))
        g_hdr_version = 2;
    else if (!(strcmp("3", vers)))
        g_hdr_version = 3;
    HDR_LOGI("detect current hdr type: %d\n", g_hdr_version);

    if (g_hdr_version == 2) {
        hdr_detect_t detect;
        hdr_stat_t  stat;
        detect.thres_dark = 20;         //[0, 40]
        detect.thres_bright = 250;       //[180, 255]
        detect.bv = detect_in_param->bv;
        detect.face_num = detect_in_param->face_num;
        detect.abl_weight = detect_in_param->abl_weight;
        detect.iso = detect_in_param->iso;
        detect.tuning_param = detect_in_param->tuning_param;
        detect.tuning_param_size = detect_in_param->tuning_size;

        stat.hist256 = detect_in_param->hist;
        stat.img = detect_in_param->img;
        stat.w = detect_in_param->w;
        stat.h = detect_in_param->h;
        stat.s = detect_in_param->s;

        ret = sprd_hdr_scndet_multi_inst(&detect, &stat, detect_out_param->ev, &status->p_smooth_flag, &status->p_frameID);
        hdr_callback_t *callback = &detect.callback;
        detect_out_param->tuning_param_index = callback->tuning_param_index;
        detect_out_param->clock = callback->clock;
        detect_out_param->bv = callback->exif_info.bv;
        detect_out_param->face_num = callback->exif_info.face_num;
        detect_out_param->prop_dark = callback->exif_info.prop_dark;
        detect_out_param->prop_bright = callback->exif_info.prop_bright;
        detect_out_param->sceneChosen = callback->exif_info.sceneChosen;
        detect_out_param->scene_flag = callback->exif_info.scene_flag;
        detect_out_param->frame_num = 2;
    } else if (g_hdr_version == 3) {
        hdr3_detect_t detect;
        hdr3_detect_out_t detect_out;
        hdr3_stat_t  stat;

        detect.hist = detect_in_param->hist;
        detect.w = detect_in_param->w;
        detect.h = detect_in_param->h;
        detect.bv = detect_in_param->bv;
        detect.face_num = detect_in_param->face_num;
        detect.abl_weight = detect_in_param->abl_weight;
        detect.iso = detect_in_param->iso;
        detect.tuning_param = detect_in_param->tuning_param;
        detect.tuning_param_size = detect_in_param->tuning_size;

        for (int i=0; i<HDR_IMG_NUM_MAX; i++)
        {
            detect_out.ev[i] = detect_out_param->ev[i];
        }
        detect_out.tuning_param_index = detect_out_param->tuning_param_index;
        detect_out.clock = detect_out_param->clock;

        stat.p_smooth_flag = status->p_smooth_flag;
        stat.p_frameID = status->p_frameID;

        ret = sprd_hdr3_scndet(&detect, &detect_out, &stat);

        status->p_smooth_flag = stat.p_smooth_flag;
        status->p_frameID = stat.p_frameID;
        detect_out_param->frame_num = 7;
        for (int i=0; i<detect_out_param->frame_num; i++)
        {
            detect_out_param->ev[i] = detect_out.ev[i];
        }
        detect_out_param->tuning_param_index = detect_out.tuning_param_index;
        detect_out_param->clock = detect_out.clock;
    }
    return ret;
}

void *sprd_hdr_adpt_init(int max_width, int max_height, void *param)
{
    void *handle = 0;
    char strRunType[256];
    char vers[256];

#ifdef CONFIG_SPRD_HDR_LIB
     if (sprd_hdr_pool_init())
        return 0;
    handle = (void *)-1;
#else
    property_get("persist.vendor.cam.hdr.version", vers , "2");

    if (!(strcmp("2", vers)))
        g_hdr_version = 2;
    else if (!(strcmp("3", vers)))
        g_hdr_version = 3;
    HDR_LOGI("init current hdr type: %d\n", g_hdr_version);

    if (g_hdr_version == 2) {
        hdr_config_t cfg;
        sprd_hdr_config_default(&cfg);
        HDR_LOGI("param: %p\n", param);
        if(param != 0) {
            sprd_hdr_init_param_t *adapt_init_param = (sprd_hdr_init_param_t *)param;
            cfg.tuning_param_size = adapt_init_param->tuning_param_size;
            cfg.tuning_param = adapt_init_param->tuning_param;
            cfg.malloc = adapt_init_param->malloc;
            cfg.free = adapt_init_param->free;
            HDR_LOGI("tuning_param: %p ,tuning_param_size: %d, malloc: %p, free:%p \n",cfg.tuning_param,cfg.tuning_param_size, cfg.malloc, cfg.free);
        }

        cfg.max_width = max_width;
        cfg.max_height = max_height;
        cfg.img_width = max_width;
        cfg.img_height = max_height;
        cfg.img_stride = max_width;

        if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP && !sprd_caa_vdsp_check_supported())
            g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;

        property_get("persist.vendor.cam.hdr2.run_type", strRunType , "");
        if (!(strcmp("cpu", strRunType)))
            g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
        else if (!(strcmp("vdsp", strRunType)))
            g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;

        HDR_LOGI("current run type: %d\n", g_run_type);

        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
            sprd_hdr_open(&handle, &cfg);
        else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
            sprd_hdr_vdsp_open(&handle, &cfg);
    } else if (g_hdr_version == 3) {
        hdr3_config_t cfg;

        sprd_hdr_init_param_t *adapt_init_param = (sprd_hdr_init_param_t *)param;
        cfg.tuning_size = adapt_init_param->tuning_param_size;
        cfg.tuning_param = adapt_init_param->tuning_param;
        cfg.malloc = adapt_init_param->malloc;
        cfg.free = adapt_init_param->free;
        HDR_LOGI("tuning_param: %p ,tuning_param_size: %d, malloc: %p, free:%p \n",cfg.tuning_param,cfg.tuning_size, cfg.malloc, cfg.free);

        property_get("persist.vendor.cam.hdr3.run_type", strRunType , "");
        if (!(strcmp("cpu", strRunType)))
            g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
        HDR_LOGI("current run type: %d\n", g_run_type);

        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
            sprd_hdr3_init(&handle, max_width, max_height, &cfg);
    }
#endif

    return handle;
}

int sprd_hdr_adpt_deinit(void *handle)
{
    int ret = 0;

#ifdef CONFIG_SPRD_HDR_LIB
    ret = sprd_hdr_pool_destroy();
#else
    if (g_hdr_version == 2) {
        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
            ret = sprd_hdr_close(handle);
        else if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP)
            ret = sprd_hdr_vdsp_close(handle);
    } else if (g_hdr_version == 3) {
        if (g_run_type == SPRD_CAMALG_RUN_TYPE_CPU)
            ret = sprd_hdr3_deinit(&handle);
    }
#endif

    return ret;
}

int sprd_hdr_adpt_ctrl(void *handle, sprd_hdr_cmd_t cmd, void *param)
{
    int ret = 0;
    switch (cmd) {
    case SPRD_HDR_GET_VERSION_CMD: {
#ifndef  CONFIG_SPRD_HDR_LIB
        if (g_hdr_version == 2) {
            hdr_version_t *version = (hdr_version_t *)param;
            ret = sprd_hdr_version(version);
        }
#endif
        break;
    }
    case SPRD_HDR_GET_EXIF_SIZE_CMD: {
        sprd_hdr_get_exif_size_param_t *hdr_param = (sprd_hdr_get_exif_size_param_t *)param;
        hdr_param->exif_size = hdr_adapt_get_exif_size();
        break;
    }
    case SPRD_HDR_PROCESS_CMD: {
        sprd_hdr_param_t *hdr_param = (sprd_hdr_param_t *)param;
        ret = hdr_adapt_process(handle, hdr_param);
        break;
    }
    case SPRD_HDR_GET_EXIF_INFO_CMD: {
        sprd_hdr_get_exif_info_param_t *hdr_param = (sprd_hdr_get_exif_info_param_t *)param;
        ret = hdr_adapt_get_exif_info(handle, hdr_param);
        HDR_LOGI("exif_info %p", hdr_param->exif_info);
        break;
    }
    case SPRD_HDR_FAST_STOP_CMD: {
#ifdef CONFIG_SPRD_HDR_LIB
        sprd_hdr_set_stop_flag(HDR_STOP);
#else
        if (g_hdr_version == 2) {
            ret = sprd_hdr_fast_stop(handle);
        } else if (g_hdr_version == 3) {
            sprd_hdr3_fast_stop(handle);
        }
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

#ifdef CONFIG_SPRD_HDR_LIB
    *type = SPRD_CAMALG_RUN_TYPE_CPU;
#else
    if(g_hdr_version == 2)
        *type = g_run_type;
    else if (g_hdr_version == 3)
        *type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

    return 0;
}

int sprd_hdr_set_devicetype(enum camalg_run_type type)
{
    if (type < SPRD_CAMALG_RUN_TYPE_CPU || type >= SPRD_CAMALG_RUN_TYPE_MAX)
        return 1;
    g_run_type = type;

    return 0;
}
