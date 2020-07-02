#include "SprdBokehAlgo.h"

using namespace android;
namespace sprdcamera {

SprdBokehAlgo::SprdBokehAlgo() {
    mFirstSprdBokeh = false;
    mReadOtp = false;
    mDepthPrevHandle = NULL;
    mDepthCapHandle = NULL;
    mBokehCapHandle = NULL;
    mBokehDepthPrevHandle = NULL;
    mPortraitHandle = NULL;
    memset(&mSize, 0, sizeof(BokehSize));
    memset(&mCalData, 0, sizeof(OtpData));
    memset(&mPreviewbokehParam, 0, sizeof(bokeh_prev_params_t));
    memset(&mCapbokehParam, 0, sizeof(bokeh_cap_params_t));
    memset(&mBokehParams, 0, sizeof(SPRD_BOKEH_PARAM));
    memset(&mPortraitCapParam, 0, sizeof(bokeh_params));
}

SprdBokehAlgo::~SprdBokehAlgo() {
    mFirstSprdBokeh = false;
    mReadOtp = false;
}

int SprdBokehAlgo::initParam(BokehSize *size, OtpData *data,
                             bool galleryBokeh) {
    int rc = NO_ERROR;

    if (!size || !data) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        return rc;
    }

    memcpy(&mSize, size, sizeof(BokehSize));
    memcpy(&mCalData, data, sizeof(OtpData));
    if (mFirstSprdBokeh) {
        sprd_bokeh_Close(mBokehCapHandle);
    }
    // preview bokeh params
    mPreviewbokehParam.init_params.width = mSize.preview_w;
    mPreviewbokehParam.init_params.height = mSize.preview_h;
    mPreviewbokehParam.init_params.depth_width = mSize.depth_prev_out_w;
    mPreviewbokehParam.init_params.depth_height = mSize.depth_prev_out_h;
    mPreviewbokehParam.init_params.SmoothWinSize = 11;
    mPreviewbokehParam.init_params.ClipRatio = 50;
    mPreviewbokehParam.init_params.Scalingratio = 2;
    mPreviewbokehParam.init_params.DisparitySmoothWinSize = 11;
    mPreviewbokehParam.weight_params.sel_x = mSize.preview_w / 2;
    mPreviewbokehParam.weight_params.sel_y = mSize.preview_h / 2;
    mPreviewbokehParam.weight_params.F_number = 20;
    mPreviewbokehParam.weight_params.DisparityImage = NULL;

    mPreviewbokehParam.depth_param.sel_x =
        mPreviewbokehParam.weight_params.sel_x;
    mPreviewbokehParam.depth_param.sel_y =
        mPreviewbokehParam.weight_params.sel_y;
    mPreviewbokehParam.depth_param.F_number =
        mPreviewbokehParam.weight_params.F_number;
    mPreviewbokehParam.depth_param.DisparityImage = NULL;
    memset(&mPreviewbokehParam.depth_param.golden_vcm_data, 0,
           sizeof(af_golden_vcm_data));

    // capture bokeh params
    mCapbokehParam.sel_x = mSize.capture_w / 2;
    mCapbokehParam.sel_y = mSize.capture_h / 2;
    mCapbokehParam.bokeh_level = 255;
    mCapbokehParam.config_param = NULL;
    mCapbokehParam.param_state = false;

    rc = sprd_bokeh_Init(&mBokehCapHandle, mSize.capture_w, mSize.capture_h,
                         mCapbokehParam.config_param);

    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_bokeh_Init failed!");
        goto exit;
    }
    if (mReadOtp == false) {
        loadDebugOtp();
        data->otp_exist = mCalData.otp_exist;
        mReadOtp = true;
    }

    HAL_LOGD("msize preview %d x %d, depth out prev %d x %d, capture %d x %d, "
             "otp exist %d",
             mSize.preview_w, mSize.preview_h, mSize.depth_prev_out_w,
             mSize.depth_prev_out_h, mSize.capture_w, mSize.capture_h,
             mCalData.otp_exist);
exit:
    return rc;
}

void SprdBokehAlgo::getVersionInfo() {}

void SprdBokehAlgo::getBokenParam(void *param) {
    if (!param) {
        HAL_LOGE("para is illegal");
        return;
    }

    memcpy(&mBokehParams.cap, &mCapbokehParam, sizeof(bokeh_cap_params_t));
    memcpy(&((BOKEH_PARAM *)param)->sprd, &mBokehParams,
           sizeof(SPRD_BOKEH_PARAM));
}

void SprdBokehAlgo::setCapFaceParam(void *param) {}

void SprdBokehAlgo::setBokenParam(void *param) {
    if (!param) {
        HAL_LOGE("para is illegal");
        return;
    }

    Mutex::Autolock l(mSetParaLock);
    int fnum = 0;
    bokeh_params bokeh_param;
    memset(&bokeh_param, 0, sizeof(bokeh_params));
    memcpy(&bokeh_param, (bokeh_params *)param, sizeof(bokeh_params));
    memcpy(&mPortraitCapParam, (bokeh_params *)param, sizeof(bokeh_params));
    mPreviewbokehParam.weight_params.sel_x = bokeh_param.sel_x;
    mPreviewbokehParam.weight_params.sel_y = bokeh_param.sel_y;
    mCapbokehParam.sel_x = bokeh_param.capture_x *mSize.capture_w / mSize.preview_w;
    mCapbokehParam.sel_y = bokeh_param.capture_y *mSize.capture_h / mSize.preview_h;
    fnum = bokeh_param.f_number * MAX_BLUR_F_FUMBER / MAX_F_FUMBER;
    mPreviewbokehParam.weight_params.F_number = fnum;
    mPreviewbokehParam.depth_param.sel_x = bokeh_param.sel_x;
    mPreviewbokehParam.depth_param.sel_y = bokeh_param.sel_y;
    mPreviewbokehParam.depth_param.F_number = fnum;
    mPreviewbokehParam.depth_param.golden_vcm_data.golden_count =
        bokeh_param.relbokeh_oem_data.golden_count;
    mPreviewbokehParam.depth_param.golden_vcm_data.golden_macro =
        bokeh_param.relbokeh_oem_data.golden_macro;
    mPreviewbokehParam.depth_param.golden_vcm_data.golden_infinity =
        bokeh_param.relbokeh_oem_data.golden_infinity;
    memcpy(&mPreviewbokehParam.depth_param.portrait_param,
           &bokeh_param.portrait_param, sizeof(struct portrait_mode_param));
    for (int i = 0;
         i < mPreviewbokehParam.depth_param.golden_vcm_data.golden_count; i++) {
        mPreviewbokehParam.depth_param.golden_vcm_data.golden_vcm[i] =
            bokeh_param.relbokeh_oem_data.golden_vcm[i];
        mPreviewbokehParam.depth_param.golden_vcm_data.golden_distance[i] =
            bokeh_param.relbokeh_oem_data.golden_distance[i];
    }
    mPreviewbokehParam.depth_param.VCM_cur_value = bokeh_param.vcm;
}

int SprdBokehAlgo::prevDepthRun(void *para1, void *para2, void *para3,
                                void *para4) {
    int rc = NO_ERROR;
    int64_t depthRun = 0;
    distanceRet distance;
    if (!para1 || !para2 || !para3 || !para4) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }
    HAL_LOGD("preview depth fnum %d, coordinate (%d,%d) vcm %d",
             mPreviewbokehParam.depth_param.F_number,
             mPreviewbokehParam.depth_param.sel_x,
             mPreviewbokehParam.depth_param.sel_y,
             mPreviewbokehParam.depth_param.VCM_cur_value);
    depthRun = systemTime();
    if (mDepthPrevHandle) {
        rc = sprd_depth_Run_distance(mDepthPrevHandle, para1, para4, para3,
                                     para2, &mPreviewbokehParam.depth_param,
                                     &distance);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_Run_distance failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
    HAL_LOGD("depth run cost %lld ms", ns2ms(systemTime() - depthRun));
exit:
    return rc;
}

int SprdBokehAlgo::initAlgo() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mDepthCapHandle) {
            sprd_depth_Close(mDepthCapHandle);
        }
        if (mDepthPrevHandle) {
            sprd_depth_Close(mDepthPrevHandle);
        }
    }

    struct sprd_depth_configurable_para depth_config_param;
    char acVersion[256] = {
        0,
    };
    // preview depth params
    depth_init_inputparam prev_input_param;
    depth_init_outputparam prev_output_info;
    depth_mode prev_mode;
    outFormat prev_outformat;
    prev_input_param.input_width_main = mSize.preview_w;
    prev_input_param.input_height_main = mSize.preview_h;
    prev_input_param.input_width_sub = mSize.depth_prev_sub_w;
    prev_input_param.input_height_sub = mSize.depth_prev_sub_h;
    prev_input_param.output_depthwidth = mSize.depth_prev_out_w;
    prev_input_param.output_depthheight = mSize.depth_prev_out_h;
    prev_input_param.online_depthwidth = mSize.depth_snap_out_w;
    prev_input_param.online_depthheight = mSize.depth_snap_out_h;
    prev_input_param.depth_threadNum = 1;
    prev_input_param.online_threadNum = 2;
    prev_input_param.imageFormat_main = YUV420_NV12;
    prev_input_param.imageFormat_sub = YUV420_NV12;
    prev_input_param.potpbuf = mCalData.otp_data;
    prev_input_param.otpsize = mCalData.otp_size;
    prev_input_param.config_param = NULL;
    prev_mode = MODE_CAPTURE;
    prev_outformat = MODE_WEIGHTMAP;
    mDepthPrevHandle = NULL;
    // capture depth params
    depth_init_inputparam cap_input_param;
    depth_init_outputparam cap_output_info;
    depth_mode cap_mode;
    outFormat cap_outformat;
    cap_input_param.input_width_main = mSize.depth_snap_main_w;
    cap_input_param.input_height_main = mSize.depth_snap_main_h;
    cap_input_param.input_width_sub = mSize.depth_snap_sub_w;
    cap_input_param.input_height_sub = mSize.depth_snap_sub_h;
    cap_input_param.output_depthwidth = mSize.depth_snap_out_w;
    cap_input_param.output_depthheight = mSize.depth_snap_out_h;
    cap_input_param.online_depthwidth = 0;
    cap_input_param.online_depthheight = 0;
    cap_input_param.depth_threadNum = 2;
    cap_input_param.online_threadNum = 0;
    cap_input_param.imageFormat_main = YUV420_NV12;
    cap_input_param.imageFormat_sub = YUV420_NV12;
    cap_input_param.potpbuf = mCalData.otp_data;
    cap_input_param.otpsize = mCalData.otp_size;
    cap_input_param.config_param = NULL;
    cap_mode = MODE_CAPTURE;
    cap_outformat = MODE_DISPARITY;
    mDepthCapHandle = NULL;
    rc = checkDepthPara(&depth_config_param);
    if (rc) {
        prev_input_param.config_param = (char *)(&sprd_depth_config_para);
        cap_input_param.config_param = (char *)(&sprd_depth_config_para);
    } else {
        prev_input_param.config_param = (char *)&depth_config_param;
        cap_input_param.config_param = (char *)&depth_config_param;
        HAL_LOGI("sensor_direction=%d", depth_config_param.SensorDirection);
    }
    rc = sprd_depth_VersionInfo_Get(acVersion, 256);
    HAL_LOGD("depth api version [%s]", acVersion);
    if (mDepthPrevHandle == NULL) {
        int64_t depthInit = systemTime();
        mDepthPrevHandle =
            sprd_depth_Init(&(prev_input_param), &(prev_output_info), prev_mode,
                            prev_outformat);
        if (mDepthPrevHandle == NULL) {
            HAL_LOGE("sprd_depth_Init failed!");
            rc = UNKNOWN_ERROR;
            goto exit;
        }
        HAL_LOGD("depth init cost %lld ms", ns2ms(systemTime() - depthInit));
    }
    if (mDepthCapHandle == NULL) {
        int64_t depthInit = systemTime();

        mDepthCapHandle = sprd_depth_Init(
            &(cap_input_param), &(cap_output_info), cap_mode, cap_outformat);
        if (mDepthCapHandle == NULL) {
            HAL_LOGE("sprd_depth_Init failed!");
            rc = UNKNOWN_ERROR;
            goto exit;
        }

        HAL_LOGD("depth init cost %lld ms", ns2ms(systemTime() - depthInit));
    }

exit:
    return rc;
}

int SprdBokehAlgo::deinitAlgo() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mBokehCapHandle) {
            sprd_bokeh_Close(mBokehCapHandle);
        }
    }
    mBokehCapHandle = NULL;

    return rc;
}

int SprdBokehAlgo::initPrevDepth() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mBokehDepthPrevHandle) {
            rc = iBokehDeinit(mBokehDepthPrevHandle);
        }
        if (rc != NO_ERROR) {
            rc = UNKNOWN_ERROR;
            HAL_LOGE("Deinit Err:%d", rc);
            goto exit;
        }
    }
    rc = iBokehInit(&mBokehDepthPrevHandle, &(mPreviewbokehParam.init_params));
    if (rc != NO_ERROR) {
        rc = UNKNOWN_ERROR;
        HAL_LOGE("iBokehInit failed!");
        goto exit;
    } else {
        HAL_LOGD("iBokehInit success");
    }

    mFirstSprdBokeh = true;
exit:
    return rc;
}

int SprdBokehAlgo::deinitPrevDepth() {
    int rc = NO_ERROR;

    if (mFirstSprdBokeh) {

        if (mDepthPrevHandle != NULL) {
            int64_t depthClose = systemTime();
            rc = sprd_depth_Close(mDepthPrevHandle);
            if (rc != NO_ERROR) {
                HAL_LOGE("prev sprd_depth_Close failed! %d", rc);
                return rc;
            }
            HAL_LOGD("prev depth close cost %lld ms",
                     ns2ms(systemTime() - depthClose));
        }
        mDepthPrevHandle = NULL;

        if (mBokehDepthPrevHandle != NULL) {
            int64_t deinitStart = systemTime();
            rc = iBokehDeinit(mBokehDepthPrevHandle);
            if (rc != NO_ERROR) {
                HAL_LOGE("Deinit Err:%d", rc);
            }
            HAL_LOGD("iBokehDeinit cost %lld ms",
                     ns2ms(systemTime() - deinitStart));
        }
    }
    mBokehDepthPrevHandle = NULL;

    return rc;
}

int SprdBokehAlgo::prevBluImage(sp<GraphicBuffer> &srcBuffer,
                                sp<GraphicBuffer> &dstBuffer, void *param) {
    int rc = NO_ERROR;
    int64_t bokehBlurImage = 0;
    int64_t bokehCreateWeightMap = systemTime();
    if (!param) {
        HAL_LOGE("para is null");
        rc = BAD_VALUE;
        goto exit;
    }

    mPreviewbokehParam.weight_params.DisparityImage = (unsigned char *)param;
    if (mBokehDepthPrevHandle) {
        rc = iBokehCreateWeightMap(mBokehDepthPrevHandle,
                                   &mPreviewbokehParam.weight_params);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("iBokehCreateWeightMap failed!");
        goto exit;
    }
    HAL_LOGD("iBokehCreateWeightMap cost %lld ms",
             ns2ms(systemTime() - bokehCreateWeightMap));
    bokehBlurImage = systemTime();
    if (mBokehDepthPrevHandle) {
        rc = iBokehBlurImage(mBokehDepthPrevHandle, &(*srcBuffer),
                             &(*dstBuffer));
    }

    if (rc != NO_ERROR) {
        HAL_LOGE("iBokehBlurImage failed!");
        goto exit;
    }
    HAL_LOGD("iBokehBlurImage cost %lld ms",
             ns2ms(systemTime() - bokehBlurImage));
exit:
    return rc;
}

int SprdBokehAlgo::setFlag() {
    int rc = NO_ERROR;

    if (mDepthCapHandle) {
        sprd_depth_Set_Stopflag(mDepthCapHandle, DEPTH_STOP);
    } else {
        HAL_LOGE("mDepthCapHandle is null");
    }
    if (mDepthPrevHandle) {
        sprd_depth_Set_Stopflag(mDepthPrevHandle, DEPTH_STOP);
    } else {
        HAL_LOGE("mDepthPrevHandle is null");
    }

    return rc;
}

int SprdBokehAlgo::initCapDepth() {
    int rc = NO_ERROR;
    return rc;
}

int SprdBokehAlgo::deinitCapDepth() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mDepthCapHandle) {
            rc = sprd_depth_Close(mDepthCapHandle);
        }
        if (rc != NO_ERROR) {
            HAL_LOGE("cap sprd_depth_Close failed! %d", rc);
            return rc;
        }
    }
    mDepthCapHandle = NULL;

    return rc;
}

int SprdBokehAlgo::capDepthRun(void *para1, void *para2, void *para3,
                               void *para4, int vcmCurValue, int vcmUp,
                               int vcmDown) {
    int rc = NO_ERROR;
    int f_number = 0;
    weightmap_param weightParams;
    char prop1[PROPERTY_VALUE_MAX] = {
        0,
    };
    char prop2[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (!para1 || !para3 || !para4) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }
    f_number = mPreviewbokehParam.weight_params.F_number * MAX_F_FUMBER /
               MAX_BLUR_F_FUMBER;
    mCapbokehParam.bokeh_level =
        (MAX_F_FUMBER + 1 - f_number) * 255 / MAX_F_FUMBER;

    weightParams.F_number = mCapbokehParam.bokeh_level;
    weightParams.sel_x = mCapbokehParam.sel_x;
    weightParams.sel_y = mCapbokehParam.sel_y;
    weightParams.DisparityImage = NULL;
    weightParams.VCM_cur_value = vcmCurValue;
    memcpy(&weightParams.portrait_param,
           &mPreviewbokehParam.depth_param.portrait_param,
           sizeof(struct portrait_mode_param));
    memcpy(&weightParams.golden_vcm_data,
           &mPreviewbokehParam.depth_param.golden_vcm_data,
           sizeof(struct af_golden_vcm_data));
    HAL_LOGD("capture fnum %d coordinate (%d,%d) VCM_INFO:%d",
             weightParams.F_number, mCapbokehParam.sel_x, mCapbokehParam.sel_y,
             weightParams.VCM_cur_value);

    rc = sprd_depth_Run(mDepthCapHandle, para1, para2, para3, para4,
                        &weightParams);
exit:
    return rc;
}

int SprdBokehAlgo::capBlurImage(void *para1, void *para2, void *para3,
                                int depthW, int depthH, int mode) {
    int rc = NO_ERROR;
    int64_t bokehReFocusTime = 0;
    char acVersion[256] = {
        0,
    };
    if (!para1 || !para2 || !para3) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }

    if (mBokehCapHandle) {

        sprd_bokeh_VersionInfo_Get(acVersion, 256);
        HAL_LOGD("Bokeh Api Version [%s]", acVersion);
    }

    bokehReFocusTime = systemTime();
    if (mBokehCapHandle) {
        rc = sprd_bokeh_ReFocusPreProcess(mBokehCapHandle, para1, para2, depthW,
                                          depthH);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_bokeh_ReFocusPreProcess failed!");
        goto exit;
    }
    HAL_LOGD("bokeh ReFocusProcess cost %lld ms",
             ns2ms(systemTime() - bokehReFocusTime));
    bokehReFocusTime = systemTime();
    if (mBokehCapHandle && 0 == mode) {
        rc = sprd_bokeh_ReFocusGen(mBokehCapHandle, para3,
                                   mCapbokehParam.bokeh_level,
                                   mCapbokehParam.sel_x, mCapbokehParam.sel_y);
    } else {
        rc = sprd_bokeh_ReFocusGen_Portrait(
            mBokehCapHandle, para3, mCapbokehParam.bokeh_level,
            mCapbokehParam.sel_x, mCapbokehParam.sel_y);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_bokeh_ReFocusGen failed!");
        goto exit;
    }
    HAL_LOGD("bokeh ReFocusGen cost %lld ms",
             ns2ms(systemTime() - bokehReFocusTime));
exit:
    return rc;
}

int SprdBokehAlgo::onLine(void *para1, void *para2, void *para3, void *para4) {
    int rc = NO_ERROR;
    int64_t onlineRun = 0;
    int64_t onlineScale = 0;
    if (!para1 || !para2 || !para3) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }

    onlineRun = systemTime();
    if (mDepthPrevHandle) {
        rc =
            sprd_depth_OnlineCalibration(mDepthPrevHandle, para1, para3, para2);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_OnlineCalibration failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
    HAL_LOGD("onLine run cost %lld ms", ns2ms(systemTime() - onlineRun));
    onlineScale = systemTime();
    if (mDepthPrevHandle) {
        rc = sprd_depth_OnlineCalibration_postprocess(mDepthPrevHandle, para1,
                                                      para4);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_OnlineCalibration_postprocess failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
    HAL_LOGD("sprd_depth_OnlineCalibration_postprocess run cost %lld ms",
             ns2ms(systemTime() - onlineScale));
exit:
    return rc;
}

int SprdBokehAlgo::getGDepthInfo(void *para1, gdepth_outparam *para2) {
    int rc = NO_ERROR;
    if (!para1 || !para2) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }

    rc = sprd_depth_get_gdepthinfo(mDepthCapHandle, para1, para2);
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_get_gdepthinfo failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
exit:
    return rc;
}

int SprdBokehAlgo::setUserset(char *ptr, int size) {
    int rc = NO_ERROR;
    if (!ptr) {
        HAL_LOGE(" ptr is null");
        rc = BAD_VALUE;
        goto exit;
    }

    rc = sprd_depth_userset(ptr, size);
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_userset failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }

    rc = sprd_bokeh_userset(ptr, size);
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_userset failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
exit:
    return rc;
}

/*===========================================================================
 * FUNCTION   :checkDepthPara
 *
 * DESCRIPTION: check depth config parameters
 *
 * PARAMETERS : struct sprd_depth_configurable_para *depth_config_param
 *
 * RETURN:
 *                  0  : success
 *                  other: non-zero failure code
 *==========================================================================*/

int SprdBokehAlgo::checkDepthPara(
    struct sprd_depth_configurable_para *depth_config_param) {
    int rc = NO_ERROR;
    char para[50] = {0};
    FILE *fid =
        fopen("/data/vendor/cameraserver/depth_config_parameter.bin", "rb");
    if (fid != NULL) {
        HAL_LOGD("open depth_config_parameter.bin file success");
        rc = fread(para, sizeof(char),
                   sizeof(struct sprd_depth_configurable_para), fid);
        HAL_LOGD("read depth_config_parameter.bin size %d bytes", rc);
        memcpy((void *)depth_config_param, (void *)para,
               sizeof(struct sprd_depth_configurable_para));
        HAL_LOGD(
            "read sprd_depth_configurable_para: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            depth_config_param->SensorDirection,
            depth_config_param->DepthScaleMin,
            depth_config_param->DepthScaleMax,
            depth_config_param->CalibInfiniteZeroPt,
            depth_config_param->SearhRange, depth_config_param->MinDExtendRatio,
            depth_config_param->inDistance, depth_config_param->inRatio,
            depth_config_param->outDistance, depth_config_param->outRatio);
        fclose(fid);
        rc = NO_ERROR;
    } else {
        HAL_LOGW("open depth_config_parameter.bin file error");
        rc = UNKNOWN_ERROR;
    }
    return rc;
}

void SprdBokehAlgo::loadDebugOtp() {
    int rc = NO_ERROR;
    uint32_t read_byte = 0;
    cmr_u8 *otp_data = (cmr_u8 *)mCalData.otp_data;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };

    FILE *fid = fopen("/mnt/vendor/productinfo/calibration.txt", "rb");
    if (NULL == fid) {
        HAL_LOGD("dualotp read failed!");
        rc = -1;
    } else {

        while (!feof(fid)) {
            fscanf(fid, "%d\n", otp_data);
            otp_data += 4;
            read_byte += 4;
        }
        fclose(fid);
        mCalData.otp_size = read_byte;
        mCalData.otp_exist = true;
        HAL_LOGD("dualotp read_bytes=%d ", read_byte);

        property_get("persist.vendor.cam.dump.calibration.data", prop, "0");
        if (atoi(prop) == 1) {
            for (int i = 0; i < mCalData.otp_size; i++)
                HAL_LOGD("calibraion data [%d] = %d", i, mCalData.otp_data[i]);
        }
    }
}

int SprdBokehAlgo::initPortraitParams(BokehSize *size,
                                                  OtpData *data,
                                                  bool galleryBokeh) {
    int rc = NO_ERROR;
    return rc;
}

int SprdBokehAlgo::capPortraitDepthRun(
    void *para1, void *para2, void *para3, void *para4, void *input_buf1_addr,
    void *output_buf, int vcmCurValue, int vcmUp, int vcmDown, void *mask) {
    int rc = NO_ERROR;
    return rc;
}

int SprdBokehAlgo::deinitPortrait() {
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::initPortraitLightParams() {
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::deinitLightPortrait(){
    int rc = NO_ERROR;
    return rc;
}
void SprdBokehAlgo::setLightPortraitParam(int param1, int param2, int param3, int param4){

}
void SprdBokehAlgo::getLightPortraitParam(int *param){

}
int SprdBokehAlgo::prevLPT(void *input_buff, int picWidth, int picHeight){
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::capLPT(void *output_buff, int picWidth, int picHeight, 
                        unsigned char *outPortraitMask, int lightPortraitType) {
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::runDFA(void *input_buff, int picWidth, int picHeight, int mode) {
    int rc = NO_ERROR;
    return rc;
}
void SprdBokehAlgo::setFaceInfo(int *angle, int *pose, int *fd_score) {

}
int SprdBokehAlgo::doFaceBeauty(unsigned char *mask, void *input_buff, 
                    int picWidth, int picHeight, int mode, faceBeautyLevels *facebeautylevel) {
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::initFaceBeautyParams() {
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::deinitFaceBeauty() {
    int rc = NO_ERROR;
    return rc;
}
int SprdBokehAlgo::getPortraitMask(void *para1, void *para2, void *output_buff, void *input_buf1_addr, 
                    int vcmCurValue, unsigned char *result) {
    int rc = NO_ERROR;
    return rc;
}

}