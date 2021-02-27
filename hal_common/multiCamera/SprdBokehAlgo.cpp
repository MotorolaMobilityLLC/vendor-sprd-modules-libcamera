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
    memset(&mPrevBokehInitParams, 0, sizeof(sprd_preview_bokeh_init_param_t));
    memset(&mPrevBokehDeinitParams, 0, sizeof(sprd_preview_bokeh_deinit_param_t));
    memset(&mPrevBokehWeightMapParams, 0, sizeof(sprd_preview_bokeh_weight_param_t));
    memset(&mPrevBokehBlurProcessParams, 0, sizeof(sprd_preview_bokeh_blur_param_t));
    memset(&mCapBokehInitParams, 0, sizeof(sprd_capture_bokeh_init_param_t));
    memset(&mCapBokehDeinitParams, 0, sizeof(sprd_capture_bokeh_deinit_param_t));
    memset(&mCapBokehGdepthParams, 0, sizeof(sprd_capture_bokeh_gdepth_param_t));
    memset(&mCapBokehPreParams, 0, sizeof(sprd_capture_bokeh_pre_param_t));
    memset(&mCapBokehGenParams, 0, sizeof(sprd_capture_bokeh_gen_param_t));
    memset(&mCapBokehInfoParams, 0, sizeof(sprd_capture_bokeh_info_param_t));
    memset(&mPrevDepthInitParams, 0, sizeof(sprd_depth_init_param_t));
    memset(&mCapDepthInitParams, 0, sizeof(sprd_depth_init_param_t));
    memset(&mPrevDepthRunParams, 0, sizeof(sprd_depth_run_param_t));
    memset(&mCapDepthRunParams, 0, sizeof(sprd_depth_run_param_t));
    memset(&mPrevDepthOnlineCaliParams, 0, sizeof(sprd_depth_onlineCali_param_t));
    memset(&mPrevDepthOnlinePostParams, 0, sizeof(sprd_depth_onlinepost_param_t));
    memset(&mCapDepthGdepthParams, 0, sizeof(sprd_depth_gdepth_param_t));
    memset(&mDepthUsersetParams, 0, sizeof(sprd_depth_userset_param_t));
    memset(&mBokehPortraitParams, 0, sizeof(portrait_mode_param));
    memset(&tuning_golden_vcm, 0, sizeof(af_relbokeh_oem_data));
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
        mCapBokehDeinitParams.ctx = &mBokehCapHandle;
        rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_DEINIT_CMD, &mCapBokehDeinitParams);
    }
    // preview bokeh params
    mPrevBokehInitParams.width = mSize.preview_w;
    mPrevBokehInitParams.height = mSize.preview_h;
    mPrevBokehInitParams.depth_width = mSize.depth_prev_out_w;
    mPrevBokehInitParams.depth_height = mSize.depth_prev_out_h;
    mPrevBokehInitParams.SmoothWinSize = 11;
    mPrevBokehInitParams.ClipRatio = 50;
    mPrevBokehInitParams.Scalingratio = 2;
    mPrevBokehInitParams.DisparitySmoothWinSize = 11;

    mPrevBokehWeightMapParams.sel_x = mSize.preview_w / 2;
    mPrevBokehWeightMapParams.sel_y = mSize.preview_h / 2;
    mPrevBokehWeightMapParams.F_number = 20;
    mPrevBokehWeightMapParams.DisparityImage = NULL;

    mPreviewbokehParam.depth_param.sel_x = mSize.preview_w / 2;
    mPreviewbokehParam.depth_param.sel_y = mSize.preview_h / 2;
    mPreviewbokehParam.depth_param.F_number = 20;
    mPreviewbokehParam.depth_param.DisparityImage = NULL;

    // capture bokeh params
    mCapbokehParam.sel_x = mSize.capture_w / 2;
    mCapbokehParam.sel_y = mSize.capture_h / 2;
    mCapbokehParam.bokeh_level = 255;
    mCapbokehParam.config_param = NULL;
    mCapbokehParam.param_state = false;

    mCapBokehInitParams.ctx = &mBokehCapHandle;
    mCapBokehInitParams.imgW = mSize.capture_w;
    mCapBokehInitParams.imgH = mSize.capture_h;
    init_param adpt_init;
    memset(&adpt_init,0,sizeof(init_param));
    adpt_init.mode = MODE_DUAL_CAMERA;
    adpt_init.productID = PLATFORM_ID;
    mCapBokehInitParams.param = &adpt_init;
    rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_INIT_CMD,&mCapBokehInitParams);

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
    mPrevBokehWeightMapParams.sel_x = bokeh_param.sel_x;
    mPrevBokehWeightMapParams.sel_y = bokeh_param.sel_y;
    mCapbokehParam.sel_x = bokeh_param.capture_x *mSize.capture_w / mSize.preview_w;
    mCapbokehParam.sel_y = bokeh_param.capture_y *mSize.capture_h / mSize.preview_h;
    fnum = bokeh_param.f_number * MAX_BLUR_F_FUMBER / MAX_F_FUMBER;
    mPrevBokehWeightMapParams.F_number = fnum;

    mPreviewbokehParam.depth_param.sel_x = bokeh_param.sel_x;
    mPreviewbokehParam.depth_param.sel_y = bokeh_param.sel_y;
    mPreviewbokehParam.depth_param.F_number = fnum;
    memcpy(&tuning_golden_vcm,&bokeh_param.relbokeh_oem_data,
        sizeof(struct af_golden_vcm_data));
    HAL_LOGD("unsigned short %d",sizeof(unsigned short));

    mPrevDepthRunParams.params.tuning_golden_vcm = &tuning_golden_vcm;
    mCapDepthRunParams.params.tuning_golden_vcm = &tuning_golden_vcm;
    memcpy(&mBokehPortraitParams,&bokeh_param.portrait_param,
        sizeof(struct portrait_mode_param));
    mPreviewbokehParam.depth_param.VCM_cur_value = bokeh_param.vcm;
}

int SprdBokehAlgo::prevDepthRun(void *para1, void *para2, void *para3,
                                void *para4) {
    int rc = NO_ERROR;
    int64_t depthRun = 0;
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
        mPrevDepthRunParams.input[0].addr[0] = para2;
        mPrevDepthRunParams.input[1].addr[0] = para3;
        mPrevDepthRunParams.output.addr[0] = para1;
        mPrevDepthRunParams.params.F_number = mPreviewbokehParam.depth_param.F_number;
        mPrevDepthRunParams.params.sel_x = mPreviewbokehParam.depth_param.sel_x;
        mPrevDepthRunParams.params.sel_y = mPreviewbokehParam.depth_param.sel_y;
        mPrevDepthRunParams.params.VCM_cur_value = mPreviewbokehParam.depth_param.VCM_cur_value;

        mPrevDepthRunParams.params.portrait_param = &mBokehPortraitParams;
        HAL_LOGD("prev_depth_mRotation %d",mBokehPortraitParams.mRotation);
        rc = sprd_depth_adpt_ctrl(mDepthPrevHandle,SPRD_DEPTH_RUN_CMD,&mPrevDepthRunParams);
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
            rc = sprd_depth_adpt_deinit(mDepthCapHandle);
        }
        if (mDepthPrevHandle) {
            rc = sprd_depth_adpt_deinit(mDepthPrevHandle);
        }
    }

    struct sprd_depth_configurable_para depth_config_param;
    // preview depth params
    mPrevDepthInitParams.inparam.input_width_main = mSize.preview_w;
    mPrevDepthInitParams.inparam.input_height_main = mSize.preview_h;
    mPrevDepthInitParams.inparam.input_width_sub = mSize.depth_prev_sub_w;
    mPrevDepthInitParams.inparam.input_height_sub = mSize.depth_prev_sub_h;
    mPrevDepthInitParams.inparam.otpbuf = mCalData.otp_data;
    mPrevDepthInitParams.inparam.otpsize = mCalData.otp_size;
    mPrevDepthInitParams.inparam.config_param = NULL;
    mPrevDepthInitParams.format = PREVIEW_MODE;
    mDepthPrevHandle = NULL;
    // capture depth params
    mCapDepthInitParams.inparam.input_width_main = mSize.depth_snap_main_w;
    mCapDepthInitParams.inparam.input_height_main = mSize.depth_snap_main_h;
    mCapDepthInitParams.inparam.input_width_sub = mSize.depth_snap_sub_w;
    mCapDepthInitParams.inparam.input_height_sub = mSize.depth_snap_sub_h;
    mCapDepthInitParams.inparam.otpbuf = mCalData.otp_data;
    mCapDepthInitParams.inparam.otpsize = mCalData.otp_size;
    mCapDepthInitParams.inparam.config_param = NULL;
    mCapDepthInitParams.format = CAPTURE_MODE;
    mDepthCapHandle = NULL;
    rc = checkDepthPara(&depth_config_param);
    if (rc) {
        mPrevDepthInitParams.inparam.config_param = (char *)(&sprd_depth_config_para);
        mCapDepthInitParams.inparam.config_param = (char *)(&sprd_depth_config_para);
    } else {
        mPrevDepthInitParams.inparam.config_param = (char *)&depth_config_param;
        mCapDepthInitParams.inparam.config_param = (char *)&depth_config_param;
        HAL_LOGI("sensor_direction=%d", depth_config_param.SensorDirection);
    }
    rc = sprd_depth_adpt_ctrl(NULL,SPRD_DEPTH_GET_VERSION_CMD,NULL);
    if (mDepthPrevHandle == NULL) {
        int64_t depthInit = systemTime();
        mDepthPrevHandle = sprd_depth_adpt_init(&mPrevDepthInitParams);
        if (mDepthPrevHandle == NULL) {
            HAL_LOGE("sprd_depth_Init failed!");
            rc = UNKNOWN_ERROR;
            goto exit;
        }
        HAL_LOGD("depth init cost %lld ms", ns2ms(systemTime() - depthInit));
    }
    if (mDepthCapHandle == NULL) {
        int64_t depthInit = systemTime();
        mDepthCapHandle = sprd_depth_adpt_init(&mCapDepthInitParams);
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
            mCapBokehDeinitParams.ctx = &mBokehCapHandle;
            rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_DEINIT_CMD, &mCapBokehDeinitParams);
        }
    }
    mBokehCapHandle = NULL;

    return rc;
}

int SprdBokehAlgo::initPrevDepth() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mBokehDepthPrevHandle) {
            mPrevBokehDeinitParams.ctx = &mBokehDepthPrevHandle;
            rc = iBokehCtrl_adpt(SPRD_PREVIEW_BOKEH_DEINIT_CMD,&mPrevBokehDeinitParams);
        }
        if (rc != NO_ERROR) {
            rc = UNKNOWN_ERROR;
            HAL_LOGE("Deinit Err:%d", rc);
            goto exit;
        }
    }
    mPrevBokehInitParams.ctx = &mBokehDepthPrevHandle;
    rc = iBokehCtrl_adpt(SPRD_PREVIEW_BOKEH_INIT_CMD, &mPrevBokehInitParams);
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
    HAL_LOGD("E");
    if (mFirstSprdBokeh) {
        if (mDepthPrevHandle != NULL) {
            int64_t depthClose = systemTime();
             rc = sprd_depth_adpt_deinit(mDepthPrevHandle);
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
            mPrevBokehDeinitParams.ctx = &mBokehDepthPrevHandle;
            rc = iBokehCtrl_adpt(SPRD_PREVIEW_BOKEH_DEINIT_CMD,&mPrevBokehDeinitParams);
            if (rc != NO_ERROR) {
                HAL_LOGE("Deinit Err:%d", rc);
            }
            HAL_LOGD("iBokehDeinit cost %lld ms",
                     ns2ms(systemTime() - deinitStart));
        }
    }
    mBokehDepthPrevHandle = NULL;
    HAL_LOGD("X");
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
    mPrevBokehWeightMapParams.DisparityImage = (unsigned char *)param;
    if (mBokehDepthPrevHandle) {
        mPrevBokehWeightMapParams.ctx = &mBokehDepthPrevHandle;
        rc = iBokehCtrl_adpt(SPRD_PREVIEW_BOKEH_WEIGHTMAP_PROCESS_CMD,&mPrevBokehWeightMapParams);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("iBokehCreateWeightMap failed!");
        goto exit;
    }
    HAL_LOGD("iBokehCreateWeightMap cost %lld ms",
             ns2ms(systemTime() - bokehCreateWeightMap));
    bokehBlurImage = systemTime();
    if (mBokehDepthPrevHandle) {
        mPrevBokehBlurProcessParams.ctx = &mBokehDepthPrevHandle;
        mPrevBokehBlurProcessParams.Src_YUV = &(*srcBuffer);
        mPrevBokehBlurProcessParams.Output_YUV = &(*dstBuffer);
        rc = iBokehCtrl_adpt(SPRD_PREVIEW_BOKEH_BLUR_PROCESS_CMD, &mPrevBokehBlurProcessParams);
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
        rc = sprd_depth_adpt_ctrl(mDepthCapHandle,SPRD_DEPTH_FAST_STOP_CMD,NULL);
    } else {
        HAL_LOGE("mDepthCapHandle is null");
    }
    if (mDepthPrevHandle) {
        rc = sprd_depth_adpt_ctrl(mDepthPrevHandle,SPRD_DEPTH_FAST_STOP_CMD,NULL);
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
    HAL_LOGD("E");
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mDepthCapHandle) {
            rc = sprd_depth_adpt_deinit(mDepthCapHandle);
        }
        if (rc != NO_ERROR) {
            HAL_LOGE("cap sprd_depth_Close failed! %d", rc);
            return rc;
        }
    }
    mDepthCapHandle = NULL;
    HAL_LOGD("X");
    return rc;
}

int SprdBokehAlgo::capDepthRun(cap_depth_params_t *cap_depth_para) {
    int rc = NO_ERROR;
    int f_number = 0;
    if (!cap_depth_para->para1 || !cap_depth_para->para3 || !cap_depth_para->para4) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }
    f_number = mPrevBokehWeightMapParams.F_number * MAX_F_FUMBER /
               MAX_BLUR_F_FUMBER;
    mCapbokehParam.bokeh_level =
        (MAX_F_FUMBER + 1 - f_number) * 255 / MAX_F_FUMBER;

    HAL_LOGD("capture fnum %d coordinate (%d,%d) VCM_INFO:%d",
             mCapbokehParam.bokeh_level, mCapbokehParam.sel_x, mCapbokehParam.sel_y,
             cap_depth_para->vcmCurValue);
    mCapDepthRunParams.input[0].addr[0] = cap_depth_para->para4;
    mCapDepthRunParams.input[1].addr[0] = cap_depth_para->para3;
    mCapDepthRunParams.output.addr[0] = cap_depth_para->para1;
    mCapDepthRunParams.params.F_number = mCapbokehParam.bokeh_level;
    mCapDepthRunParams.params.sel_x = mCapbokehParam.sel_x;
    mCapDepthRunParams.params.sel_y = mCapbokehParam.sel_y;
    mCapDepthRunParams.params.VCM_cur_value = cap_depth_para->vcmCurValue;
    mCapDepthRunParams.input_otpsize = mCalData.otp_size;
    mCapDepthRunParams.input_otpbuf = mCalData.otp_data;
    mCapDepthRunParams.output_otpbuf = cap_depth_para->otp_info->otp_data;
    mCapDepthRunParams.mChangeSensor = cap_depth_para->mChangeSensor;
    mCapDepthRunParams.params.portrait_param = &mBokehPortraitParams;
    portrait_mode_param capParams;
    memset(&capParams,0,sizeof(portrait_mode_param));
    memcpy(&capParams,&mBokehPortraitParams,sizeof(struct portrait_mode_param));
    mCapDepthRunParams.params.portrait_param = &capParams;
    HAL_LOGD("cap_depth_mRotation %d",mBokehPortraitParams.mRotation);
    rc = sprd_depth_adpt_ctrl(mDepthCapHandle,SPRD_DEPTH_RUN_CMD,&mCapDepthRunParams);
    cap_depth_para->otp_info->otp_size = mCapDepthRunParams.output_otpsize;
    cap_depth_para->otp_info->dual_otp_flag = 1;
    cap_depth_para->ret_otp  = mCapDepthRunParams.ret_otp;
exit:
    return rc;
}

int SprdBokehAlgo::capBlurImage(void *para1, void *para2, void *para3,
                                int depthW, int depthH, int mode) {
    int rc = NO_ERROR;
    int64_t bokehReFocusTime = 0;

    if (!para1 || !para2 || !para3) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }

    bokehReFocusTime = systemTime();
    if (mBokehCapHandle) {
        mCapBokehPreParams.ctx = &mBokehCapHandle;
        mCapBokehPreParams.srcImg = para1;
        mCapBokehPreParams.depthIn = para2;
        mCapBokehPreParams.params = NULL;
        mCapBokehPreParams.depthW = depthW;
        mCapBokehPreParams.depthH = depthH;
        rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_REFOCUSPRE_CMD,&mCapBokehPreParams);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_bokeh_ReFocusPreProcess failed!");
        goto exit;
    }
    HAL_LOGD("bokeh ReFocusProcess cost %lld ms",
             ns2ms(systemTime() - bokehReFocusTime));
    bokehReFocusTime = systemTime();
    mCapBokehGenParams.ctx = &mBokehCapHandle;
    mCapBokehGenParams.dstImg = para3;
    mCapBokehGenParams.F_number = mCapbokehParam.bokeh_level;
    mCapBokehGenParams.sel_x = mCapbokehParam.sel_x;
    mCapBokehGenParams.sel_y = mCapbokehParam.sel_y;
    if (mBokehCapHandle && 0 == mode) {
        rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_REFOCUSGEN_CMD,&mCapBokehGenParams);
    } else {
        rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_REFOCUSGEN_PORTRAIT_CMD,&mCapBokehGenParams);
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
        mPrevDepthOnlineCaliParams.input[0].addr[0] = para2;
        mPrevDepthOnlineCaliParams.input[1].addr[0] = para3;
        mPrevDepthOnlineCaliParams.output.addr[0] = para1;
        rc =sprd_depth_adpt_ctrl(mDepthPrevHandle,SPRD_DEPTH_ONLINECALI_CMD,&mPrevDepthOnlineCaliParams);
    }
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_OnlineCalibration failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
    HAL_LOGD("onLine run cost %lld ms", ns2ms(systemTime() - onlineRun));
    onlineScale = systemTime();
    if (mDepthPrevHandle) {
        mPrevDepthOnlinePostParams.input.addr[0] = para1;
        mPrevDepthOnlinePostParams.output.addr[0] = para4;
        rc = sprd_depth_adpt_ctrl(mDepthPrevHandle,SPRD_DEPTH_ONLINECALI_POST_CMD,&mPrevDepthOnlinePostParams);
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
    mCapDepthGdepthParams.input.addr[0] = para1;
    mCapDepthGdepthParams.gdepth_output.near = para2->near;
    mCapDepthGdepthParams.gdepth_output.far = para2->far;
    mCapDepthGdepthParams.gdepth_output.output[0].addr[0] = para2->confidence_map;
    mCapDepthGdepthParams.gdepth_output.output[1].addr[0] = para2->depthnorm_data;
    rc = sprd_depth_adpt_ctrl(mDepthCapHandle,SPRD_DEPTH_GDEPTH_CMD,&mCapDepthGdepthParams);
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_get_gdepthinfo failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
    para2->near = mCapDepthGdepthParams.gdepth_output.near;
    para2->far = mCapDepthGdepthParams.gdepth_output.far;
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
    mDepthUsersetParams.ptr = ptr;
    mDepthUsersetParams.size = size;
    rc = sprd_depth_adpt_ctrl(mDepthCapHandle,SPRD_DEPTH_USERSET_CMD,&mDepthUsersetParams);
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_depth_userset failed! %d", rc);
        rc = UNKNOWN_ERROR;
        goto exit;
    }
    mCapBokehInfoParams.ptr = ptr;
    mCapBokehInfoParams.size = size;
    rc = sprd_bokeh_ctrl_adpt(SPRD_CAPTURE_BOKEH_USERSET_CMD, &mCapBokehInfoParams);
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
            if (0 > fscanf(fid, "%d\n", otp_data)){
                HAL_LOGE("Failed to load otp data");
                goto exit;
            }

            otp_data += 4;
            read_byte += 4;
        }
        mCalData.otp_size = read_byte;
        mCalData.otp_exist = true;
        HAL_LOGD("dualotp read_bytes=%d ", read_byte);

        property_get("persist.vendor.cam.dump.calibration.data", prop, "0");
        if (atoi(prop) == 1) {
            for (int i = 0; i < mCalData.otp_size; i++)
                HAL_LOGD("calibraion data [%d] = %d", i, mCalData.otp_data[i]);
        }
    }
exit:
    if(fid)
        fclose(fid);
}

int SprdBokehAlgo::initPortraitParams(BokehSize *size,OtpData *data,
                            bool galleryBokeh, unsigned int bokehMaskSize) {
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
                    int vcmCurValue, void *bokehMask, unsigned char *lptMask) {
    int rc = NO_ERROR;
    return rc;
}
}