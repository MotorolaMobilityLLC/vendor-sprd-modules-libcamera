#include <stdint.h>
#include "fdr_interface.h"
#include "fdr_adapter.h"
#include "sprd_camalg_adapter.h"
#include "properties.h"
#include <string.h>
#include <utils/Log.h>
#include "cmr_types.h"
#include "ae_ctrl_common.h"
#include "sprd_camalg_assist.h"
#include "sprdfacebeautyfdr.h"
#include "sprdfdapi.h"

#define LOG_TAG "sprd_fdr_adapter"
#define ADAPTER_LOGE(format,...) ALOGE(format, ##__VA_ARGS__)
#define ADAPTER_LOGD(format,...) ALOGD(format, ##__VA_ARGS__)

#ifdef DEFAULT_RUNTYPE_VDSP
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
#else
static enum camalg_run_type g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
#endif

int sprd_fdr_adapter_open(void **ctx, fdr_open_param *param)
{
    char strProp[256];

    param->align_param.input_data_mode = ALIGN_INPUT_MODE;
    param->merge_param.output_mode = MERGE_OUTPUT_MODE;
    param->fusion_param.inputMode = FUSION_INPUT_MODE;
    property_get("persist.vendor.cam.fdr.merge_mode", strProp , "");
    if (!strcmp("1", strProp))
        param->merge_param.merge_mode = 1;

    if (g_run_type == SPRD_CAMALG_RUN_TYPE_VDSP && !sprd_caa_vdsp_check_supported())
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    property_get("persist.vendor.cam.fdr.run_type", strProp , "");
    if (!strcmp("cpu", strProp))
        g_run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    else if (!strcmp("vdsp", strProp))
        g_run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    param->run_type = g_run_type;

    ADAPTER_LOGD("run_type: %d\n", param->run_type);

    return sprd_fdr_open(ctx, param);
}

int sprd_fdr_adapter_align_init(void **ctx, sprd_camalg_image_t *image_array, fdr_align_init_param *init_param)
{
    return sprd_fdr_align_init(*ctx, image_array, init_param);
}

int sprd_fdr_adapter_align(void **ctx, sprd_camalg_image_t *image)
{
    return sprd_fdr_align(*ctx, image);
}

int sprd_fdr_adapter_merge(void **ctx, sprd_camalg_image_t *image_array, fdr_merge_param *merge_param, fdr_merge_out_param *merge_out_param)
{
    return sprd_fdr_merge(*ctx, image_array, merge_param, merge_out_param);
}

int sprd_fdr_fb(sprd_camalg_image_t *image)
{
    FD_IMAGE fdImage;
    memset(&fdImage, 0, sizeof(FD_IMAGE));
    fdImage.data = (unsigned char *)image->addr[0];
    fdImage.width = image->width;
    fdImage.height = image->height;
    fdImage.step = image->width;
    fdImage.context.orientation = 270;

    void *fd_handle = 0;
    FD_OPTION option = {0};
    option.fdEnv = FD_ENV_HW;

    FdInitOption(&option);

    option.platform = PLATFORM_ID_SHARKL5P;
    option.workMode = FD_WORKMODE_MOVIE;
    option.maxFaceNum = 10;
    option.minFaceSize = ((fdImage.height < fdImage.width) ? fdImage.height:fdImage.width) / 12;
    option.directions = FD_DIRECTION_ALL;
    option.angleFrontal = FD_ANGLE_RANGE_90;
    option.angleHalfProfile = FD_ANGLE_RANGE_30;
    option.angleFullProfile = FD_ANGLE_RANGE_30;
    option.detectDensity = 5;
    option.scoreThreshold = 0;
    option.detectInterval = 6;
    option.trackDensity = 5;
    option.lostRetryCount = 1;
    option.lostHoldCount = 1;
    option.holdPositionRate = 5;
    option.holdSizeRate = 4;
    option.swapFaceRate = 200;
    option.guessFaceDirection = 1;

    FdCreateDetector(&fd_handle, &option);

    FdDetectFace(fd_handle, &fdImage);

    FD_FACEINFO face;
    FBFDR_FACEINFO faceInfo[10];
    int faceNum = FdGetFaceCount(fd_handle);
    faceNum = faceNum > 10 ? 10:faceNum;
    for (int i = 0; i < faceNum; i++) {
        FdGetFaceInfo(fd_handle, i, &face);

        faceInfo[i].x = face.x;
        faceInfo[i].y = face.y;
        faceInfo[i].width = face.width;
        faceInfo[i].height = face.height;
        faceInfo[i].yawAngle = face.yawAngle;
        faceInfo[i].rollAngle = face.rollAngle;
    }

    FBFDR_IMAGE_YUV420SP fbImage;
    fbImage.format = FBFDR_YUV420_FORMAT_CRCB;
    fbImage.width = image->width;
    fbImage.height = image->height;
    fbImage.yData = (unsigned char *)image->addr[0];
    fbImage.uvData = fbImage.yData + fbImage.width * fbImage.height;

    FBFDR_FaceBeauty_YUV420SP(&fbImage, faceInfo, faceNum, 4);

    FdDeleteDetector(&fd_handle);

    return 0;
}

int sprd_fdr_adapter_fusion(void **ctx, sprd_camalg_image_t *image_in_array, sprd_camalg_image_t *image_out, expfusion_proc_param_t *param_proc)
{
    int ret = -1;
    ret =  sprd_fdr_fusion(*ctx, image_in_array, image_out, param_proc);
    ret |= sprd_fdr_fb(image_out);
    return ret;
}

int sprd_fdr_adapter_close(void **ctx)
{
    sprd_fdr_fast_stop(*ctx);
    return sprd_fdr_close(ctx);
}

int sprd_fdr_adapter_ctrl(FDR_CMD cmd, void *vparam)
{
    int ret = -1;
    if (!vparam) {
        ADAPTER_LOGE("null vparam pointer\n");
        return ret;
    }
    switch (cmd) {
        case FDR_CMD_OPEN:
        {
            FDR_CMD_OPEN_PARAM_T *param = (FDR_CMD_OPEN_PARAM_T *)vparam;
            ret = sprd_fdr_adapter_open(param->ctx, param->param);
            break;
        }
        case FDR_CMD_CLOSE:
        {
            FDR_CMD_CLOSE_PARAM_T *param = (FDR_CMD_CLOSE_PARAM_T *)vparam;
            ret = sprd_fdr_adapter_close(param->ctx);
            break;
        }
        case FDR_CMD_ALIGN_INIT:
        {
            FDR_CMD_ALIGN_INIT_PARAM_T *param = (FDR_CMD_ALIGN_INIT_PARAM_T *)vparam;
            ret = sprd_fdr_adapter_align_init(param->ctx, param->image_array, param->init_param);
            break;
        }
        case FDR_CMD_ALIGN:
        {
            FDR_CMD_ALIGN_PARAM_T *param = (FDR_CMD_ALIGN_PARAM_T *)vparam;
            ret = sprd_fdr_adapter_align(param->ctx, param->image);
            break;
        }
        case FDR_CMD_MERGE:
        {
            FDR_CMD_MERGE_PARAM_T *param = (FDR_CMD_MERGE_PARAM_T *)vparam;
            if (param->ae_param) {
                struct ae_callback_param *ae_param = (struct ae_callback_param *)param->ae_param;
                param->merge_param->scene_param.sensor_gain_AE = ae_param->sensor_gain;
                param->merge_param->scene_param.cur_bv_AE = ae_param->cur_bv;
                param->merge_param->scene_param.exp_line_AE = ae_param->exp_line;
                param->merge_param->scene_param.total_gain_AE = ae_param->total_gain;
                param->merge_param->scene_param.face_stable = ae_param->face_stable;
                param->merge_param->scene_param.face_num = ae_param->face_num;
                param->merge_param->scene_param.exp_time_AE = ae_param->exp_time;
            }
            ret = sprd_fdr_adapter_merge(param->ctx, param->image_array, param->merge_param, param->merge_out_param);
            break;
        }
        case FDR_CMD_FUSION:
        {
            FDR_CMD_FUSION_PARAM_T *param = (FDR_CMD_FUSION_PARAM_T *)vparam;
            if (param->ae_param) {
                struct ae_callback_param *ae_param = (struct ae_callback_param *)param->ae_param;
                param->param_proc->scene_param.sensor_gain_AE = ae_param->sensor_gain;
                param->param_proc->scene_param.cur_bv_AE = ae_param->cur_bv;
                param->param_proc->scene_param.exp_line_AE = ae_param->exp_line;
                param->param_proc->scene_param.total_gain_AE = ae_param->total_gain;
                param->param_proc->scene_param.face_stable = ae_param->face_stable;
                param->param_proc->scene_param.face_num = ae_param->face_num;
                param->param_proc->scene_param.exp_time_AE = ae_param->exp_time;
            }
            ret = sprd_fdr_adapter_fusion(param->ctx, param->image_in_array, param->image_out, param->param_proc);
            break;
        }
        case FDR_CMD_GET_VERSION:
        {
            FDR_CMD_GET_VERSION_PARAM_T *param = (FDR_CMD_GET_VERSION_PARAM_T *)vparam;
            ret = sprd_fdr_get_version(param->lib_version);
            break;
        }
        case FDR_CMD_GET_EXIF:
        {
            FDR_CMD_GET_EXIF_PARAM_T *param = (FDR_CMD_GET_EXIF_PARAM_T *)vparam;
            ret = sprd_fdr_get_exif(*param->ctx, param->exif_info);
            break;
        }
        case FDR_CMD_GET_FRAMENUM:
        {
            FDR_CMD_GET_FRAMENUM_PARAM_T *param = (FDR_CMD_GET_FRAMENUM_PARAM_T *)vparam;
            ret = sprd_fdr_get_frame_num(param->param_in, param->param_out, param->calc_status);
            break;
        }
        case FDR_CMD_GET_MAX_FRAMENUM:
        {
            FDR_CMD_GET_MAX_FRAMENUM_PARAM_T *param = (FDR_CMD_GET_MAX_FRAMENUM_PARAM_T *)vparam;
            ret = sprd_fdr_get_max_frame_num(param->max_total_frame_num, param->max_ref_frame_num);
            break;
        }
        default:
            ADAPTER_LOGE("unknown cmd: %d\n", cmd);
    }
    return ret;
}
