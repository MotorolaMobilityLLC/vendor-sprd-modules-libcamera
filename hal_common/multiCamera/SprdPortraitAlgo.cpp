#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL) 
#include "SprdPortraitAlgo.h"
#include <cutils/trace.h>
typedef enum { PREVIEW = 0, CAPTURE = 1 } portrait_mode;

using namespace android;
namespace sprdcamera {

SprdPortraitAlgo::SprdPortraitAlgo() {
    mFirstSprdBokeh = false;
    mFirstSprdLightPortrait = false;
    mFirstSprdDfa = false;
    mReadOtp = false;
    mDepthPrevHandle = NULL;
    mDepthCapHandle = NULL;
    mBokehCapHandle = NULL;
    mBokehDepthPrevHandle = NULL;
    mPortraitHandle = NULL;
    lpt_return_val = 0;

    memset(&mSize, 0, sizeof(BokehSize));
    memset(&mCalData, 0, sizeof(OtpData));
    memset(&mPreviewbokehParam, 0, sizeof(bokeh_prev_params_t));
    memset(&mCapbokehParam, 0, sizeof(bokeh_cap_params_t));
    memset(&mBokehParams, 0, sizeof(SPRD_BOKEH_PARAM));
    memset(&mPortraitCapParam, 0, sizeof(bokeh_params));
    memset(&lpt_prev, 0, sizeof(class_lpt));
    memset(&lpt_cap, 0, sizeof(class_lpt));
    memset(&dfa_prev, 0, sizeof(class_dfa));
    memset(&dfa_cap, 0, sizeof(class_dfa));
    memset(&lptOptions, 0, sizeof(lpt_options));
    memset(&lptOptions_cap, 0, sizeof(lpt_options));
#ifdef CONFIG_FACE_BEAUTY
    mFirstSprdFB = false;
    memset(&fb_prev, 0, sizeof(fb_beauty_param_t));
    memset(&fb_cap, 0, sizeof(fb_beauty_param_t));
    memset(&face_info, 0, sizeof(FACE_Tag));
    memset(&beautyLevels, 0, sizeof(faceBeautyLevelsT));
    memset(&beauty_face, 0, sizeof(fbBeautyFacetT));
    memset(&beauty_image, 0, sizeof(fb_beauty_image_t));
#endif
}

SprdPortraitAlgo::~SprdPortraitAlgo() {
    mFirstSprdBokeh = false;
    mFirstSprdLightPortrait = false;
    mFirstSprdDfa = false;
    mReadOtp = false;
#ifdef CONFIG_FACE_BEAUTY
    mFirstSprdFB = false;
#endif
}

int SprdPortraitAlgo::initParam(BokehSize *size, OtpData *data,
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
        // sprd_bokeh_Close(mBokehCapHandle);
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
    /*
    rc = sprd_bokeh_Init(&mBokehCapHandle, mSize.capture_w, mSize.capture_h,
                         mCapbokehParam.config_param);
    */
    if (rc != NO_ERROR) {
        HAL_LOGE("sprd_bokeh_Init failed!");
        goto exit;
    }
    if (mReadOtp == false) {
        loadDebugOtp();
        data->otp_exist = mCalData.otp_exist;
        mReadOtp = true;
    }

    HAL_LOGD("msize preview %d x %d,  depth_snap_out %d x %d, capture %d x %d, "
             "otp exist %d",
             mSize.preview_w, mSize.preview_h, mSize.depth_snap_out_w,
             mSize.depth_snap_out_h, mSize.capture_w, mSize.capture_h,
             mCalData.otp_exist);
exit:
    return rc;
}

void SprdPortraitAlgo::getVersionInfo() {}

void SprdPortraitAlgo::getBokenParam(void *param) {
    if (!param) {
        HAL_LOGE("para is illegal");
        return;
    }

    memcpy(&mBokehParams.cap, &mCapbokehParam, sizeof(bokeh_cap_params_t));
    memcpy(&((BOKEH_PARAM *)param)->sprd, &mBokehParams,
           sizeof(SPRD_BOKEH_PARAM));
}

void SprdPortraitAlgo::setCapFaceParam(void *param){
    memcpy(&mPortraitCapParam, (bokeh_params *)param, sizeof(bokeh_params));
    HAL_LOGD("face_info %d %d %d %d",mPortraitCapParam.portrait_param.x1[0] * mSize.capture_w / mSize.depth_snap_out_w,
            mPortraitCapParam.portrait_param.y1[0] * mSize.capture_w / mSize.depth_snap_out_w,
            mPortraitCapParam.portrait_param.x2[0] * mSize.capture_w / mSize.depth_snap_out_w,
            mPortraitCapParam.portrait_param.y2[0] * mSize.capture_w / mSize.depth_snap_out_w);
}

void SprdPortraitAlgo::setBokenParam(void *param) {
    if (!param) {
        HAL_LOGE("para is illegal");
        return;
    }

    Mutex::Autolock l(mSetParaLock);
    int fnum = 0;
    bokeh_params bokeh_param;
    memset(&bokeh_param, 0, sizeof(bokeh_params));
    memcpy(&bokeh_param, (bokeh_params *)param, sizeof(bokeh_params));

    mPreviewbokehParam.weight_params.sel_x = bokeh_param.sel_x;
    mPreviewbokehParam.weight_params.sel_y = bokeh_param.sel_y;
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

int SprdPortraitAlgo::prevDepthRun(void *para1, void *para2, void *para3,
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

int SprdPortraitAlgo::initAlgo() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mDepthCapHandle) {
            // sprd_depth_Close(mDepthCapHandle);
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
        /*
        mDepthCapHandle = sprd_depth_Init(
            &(cap_input_param), &(cap_output_info), cap_mode, cap_outformat);
        if (mDepthCapHandle == NULL) {
            HAL_LOGE("sprd_depth_Init failed!");
            rc = UNKNOWN_ERROR;
            goto exit;
        }
        */
        HAL_LOGD("depth init cost %lld ms", ns2ms(systemTime() - depthInit));
    }

exit:
    return rc;
}

int SprdPortraitAlgo::deinitAlgo() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mBokehCapHandle) {
            // sprd_bokeh_Close(mBokehCapHandle);
        }
    }
    mBokehCapHandle = NULL;

    return rc;
}

int SprdPortraitAlgo::initPrevDepth() {
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

int SprdPortraitAlgo::deinitPrevDepth() {
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

int SprdPortraitAlgo::prevBluImage(sp<GraphicBuffer> &srcBuffer,
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

int SprdPortraitAlgo::setFlag() {
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

int SprdPortraitAlgo::initCapDepth() {
    int rc = NO_ERROR;
    return rc;
}

int SprdPortraitAlgo::deinitCapDepth() {
    int rc = NO_ERROR;
    if (mFirstSprdBokeh) {
        if (mDepthCapHandle) {
            // rc = sprd_depth_Close(mDepthCapHandle);
        }
        if (rc != NO_ERROR) {
            HAL_LOGE("cap sprd_depth_Close failed! %d", rc);
            return rc;
        }
    }
    mDepthCapHandle = NULL;

    return rc;
}

int SprdPortraitAlgo::capDepthRun(void *para1, void *para2, void *para3,
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
    mCapbokehParam.sel_x = mPreviewbokehParam.weight_params.sel_x *
                           mSize.capture_w / mSize.preview_w;
    mCapbokehParam.sel_y = mPreviewbokehParam.weight_params.sel_y *
                           mSize.capture_h / mSize.preview_h;
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

int SprdPortraitAlgo::capBlurImage(void *para1, void *para2, void *para3,
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

int SprdPortraitAlgo::onLine(void *para1, void *para2, void *para3,
                             void *para4) {
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

int SprdPortraitAlgo::getGDepthInfo(void *para1, gdepth_outparam *para2) {
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

int SprdPortraitAlgo::setUserset(char *ptr, int size) {
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

int SprdPortraitAlgo::checkDepthPara(
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

void SprdPortraitAlgo::loadDebugOtp() {
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

int SprdPortraitAlgo::initPortraitParams(BokehSize *size, OtpData *data,
                                         bool galleryBokeh) {
    int rc = NO_ERROR;
    if (mPortraitHandle) {
        rc = sprd_portrait_capture_deinit(mPortraitHandle);
        HAL_LOGE(" mPortraitHandle is not null");
    }

    PortraitCap_Init_Params initParams;
    BokehSize m_Size;
    OtpData m_CalData;
    if (!size || !data) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        return rc;
    }
    memset(&initParams, 0, sizeof(PortraitCap_Init_Params));
    memcpy(&m_Size, size, sizeof(BokehSize));
    memcpy(&m_CalData, data, sizeof(OtpData));
    initParams.libLevel = 1;
    initParams.productInfo = PLATFORM_ID;
    initParams.calcDepth = 1;

    // single capture
    initParams.width = m_Size.capture_w;         // 3264
    initParams.height = m_Size.capture_h;        // 2448
    initParams.depthW = m_Size.depth_snap_out_w; // 800
    initParams.depthH = m_Size.depth_snap_out_h; // 600

    // capture 5M v1.0 v1.1 v1.2
    int mLastMinScope = 4;        // min_slope*10000
    int mLastMaxScope = 19;       // max_slope*10000
    int mLastAdjustRati = 150000; // findex2gamma_adjust_ratio*10000
    initParams.min_slope =
        (float)(mLastMinScope) / 10000; // 0.001~0.01, default is 0.005 ->0.0004
    initParams.max_slope =
        (float)(mLastMaxScope) / 10000; // 0.01~0.1, default is 0.05 ->0.0019
    initParams.Findex2Gamma_AdjustRatio =
        (float)(mLastAdjustRati) / 10000; // 2~11, default is 6.0 ->15.0f
    initParams.Scalingratio = 8;  // only support 2,4,6,8 ->8 for input 5M(1952)
    initParams.SmoothWinSize = 5; // odd number ->5
    initParams.box_filter_size = 0; // odd number ->0

    /*double capture*/
    initParams.input_width_main = m_Size.depth_snap_main_w;
    initParams.input_height_main = m_Size.depth_snap_main_h;
    initParams.input_width_sub = m_Size.depth_snap_sub_w;
    initParams.input_height_sub = m_Size.depth_snap_sub_h;
    initParams.potpbuf = m_CalData.otp_data;
    initParams.otpsize = m_CalData.otp_size;
    initParams.config_param = NULL;
    initParams.imageFormat_main = ImageFormat(YUV420_NV12);
    initParams.imageFormat_sub = ImageFormat(YUV420_NV12);
    rc = sprd_portrait_capture_init(&mPortraitHandle, &initParams);
    if (rc) {
        HAL_LOGE("sprd_portrait_capture_init failed");
        goto exit;
    }
    if (mPortraitHandle) {
        unsigned int maskW = mSize.depth_snap_out_w,
                     maskH = mSize.depth_snap_out_h;
        unsigned int maskSize = maskW * maskH * 2;
        rc = sprd_portrait_capture_get_mask_info(mPortraitHandle, &maskW,
                                                 &maskH, &maskSize);
    }
exit:
    return rc;
}

int SprdPortraitAlgo::capPortraitDepthRun(void *para1, void *para2, void *para3,
                                          void *para4, void *input_buf1_addr,
                                          void *output_buf, int vcmCurValue,
                                          int vcmUp, int vcmDown) {
    HAL_LOGI(" SprdPortraitAlgo ==>E");
    int rc = NO_ERROR;
    int f_number = 0;
    weightmap_param weightParams;
    ProcDepthInputMap depthData;
    PortaitCapProcParams wParams;
    InoutYUV yuvData;

    if (!para1 || !para3 || !para4) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        return rc;
    }
    memset(&depthData, 0, sizeof(ProcDepthInputMap));
    memset(&wParams, 0, sizeof(PortaitCapProcParams));
    memset(&yuvData, 0, sizeof(InoutYUV));

    depthData.mainMap = para4;
    depthData.subMap = para3;
    wParams.DisparityImage = NULL;
    wParams.VCM_cur_value = vcmCurValue;
    memcpy(&wParams.golden_vcm_data, &mPortraitCapParam.relbokeh_oem_data,
           sizeof(struct af_golden_vcm_data));

    wParams.version = 1;
    wParams.roi_type = 2;

    f_number = mPortraitCapParam.f_number;
    wParams.F_number = (MAX_F_FUMBER + 1 - f_number) * 255 / MAX_F_FUMBER;
    wParams.sel_x = mPortraitCapParam.sel_x * mSize.capture_w / mSize.preview_w;
    wParams.sel_y = mPortraitCapParam.sel_y * mSize.capture_h / mSize.preview_h;
    wParams.CircleSize = 50;
    wParams.valid_roi = mPortraitCapParam.portrait_param.valid_roi;
    wParams.total_roi = mPortraitCapParam.portrait_param.face_num;
    memcpy(&wParams.x1, &mPortraitCapParam.portrait_param.x1,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    memcpy(&wParams.x2, &mPortraitCapParam.portrait_param.x2,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    memcpy(&wParams.y1, &mPortraitCapParam.portrait_param.y1,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    memcpy(&wParams.y2, &mPortraitCapParam.portrait_param.y2,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    wParams.rear_cam_en = mPortraitCapParam.portrait_param.rear_cam_en; // true
    wParams.rotate_angle = mPortraitCapParam.portrait_param.mRotation;  //--
    wParams.camera_angle = mPortraitCapParam.portrait_param.camera_angle;
    wParams.mobile_angle = mPortraitCapParam.portrait_param.mobile_angle;

    yuvData.Src_YUV = (unsigned char *)input_buf1_addr;
    yuvData.Dst_YUV = (unsigned char *)output_buf;
    rc = sprd_portrait_capture_process(mPortraitHandle, &depthData, &wParams,
                                       &yuvData, para1, 1);

exit:
    HAL_LOGI(" X");
    return rc;
}

int SprdPortraitAlgo::deinitPortrait() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    if (mFirstSprdBokeh) {
        if (mPortraitHandle) {
            rc = sprd_portrait_capture_deinit(mPortraitHandle);
        }
        if (rc != NO_ERROR) {
            HAL_LOGE("cap sprd_portrait_capture_deinit failed! %d", rc);
            return rc;
        }
    }
    mPortraitHandle = NULL;
    HAL_LOGD("X");
    return rc;
}

int SprdPortraitAlgo::initPortraitLightParams() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    /*dfa init*/
    const char *dfaVersion = NULL;
    dfaVersion = DFA_GetVersion();
    char *dfav = new char[strlen(dfaVersion) + 1];
    strcpy(dfav, dfaVersion);
    HAL_LOGD("dfa version :%s", dfav);
    create_dfa_handle(&dfa_prev, 1);
    if (!dfa_prev.hSprdDfa) {
        HAL_LOGE(" create_dfa_handle failed");
    } else {
        mFirstSprdDfa = true;
    }

    /*lpt init*/
    const char *lptVerison = NULL;
    lptVerison = LPT_GetVersion();
    char *lptv = new char[strlen(lptVerison) + 1];
    strcpy(lptv, lptVerison);
    HAL_LOGD("lpt version :%s", lptv);
    init_lpt_options(&lpt_prev);
    int workMode =
        LPT_WORKMODE_MOVIE; /*1：LPT_WORKMODE_MOVIE(preview)，0：LPT_WORKMODE_STILL(capture)
                               */
    int threadNum = 4;
    create_lpt_handle(&lpt_prev, workMode, threadNum);
    if (!lpt_prev.hSprdLPT) {
        HAL_LOGE("create_lpt_handle failed!");
    } else {
        mFirstSprdLightPortrait = true;
    }
    /*construct lightportrait options */
    lptOptions.lightCursor =
        5; /* Control fill light and shade ratio. Value range [0, 35]*/
    lptOptions.lightWeight =
        12; /* Control fill light intensity. Value range [0, 20]*/
    lptOptions.lightSplitMode = LPT_LIGHTSPLIT_RIGHT;
    lptOptions.debugMode = 0;
    lptOptions.cameraWork = LPT_CAMERA_REAR;

    delete [] dfav;
    delete [] lptv;
    HAL_LOGD("X");
    return rc;
}
int SprdPortraitAlgo::initFaceBeautyParams() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
#ifdef CONFIG_FACE_BEAUTY
    face_beauty_set_devicetype(&fb_prev, SPRD_CAMALG_RUN_TYPE_CPU);
    face_beauty_init(&fb_prev, 1, 2, SHARKL5PRO);
    if (!fb_prev.hSprdFB) {
        HAL_LOGE(" create_fb_handle failed");
    } else {
        mFirstSprdFB = true;
    }
    beautyLevels.blemishLevel = 0;
    beautyLevels.smoothLevel = 6;
    beautyLevels.skinColor = FB_SKINCOLOR_WHITE;
    beautyLevels.skinLevel = 0;
    beautyLevels.brightLevel = 6;
    beautyLevels.lipColor = FB_LIPCOLOR_CRIMSON;
    beautyLevels.lipLevel = 0;
    beautyLevels.slimLevel = 2;
    beautyLevels.largeLevel = 2;
    beautyLevels.cameraWork = FB_CAMERA_REAR;
#endif
    HAL_LOGD("X");
    return rc;
}

int SprdPortraitAlgo::deinitFaceBeauty() {
    int rc = NO_ERROR;
    HAL_LOGD("E %d", fb_prev.runType);
#ifdef CONFIG_FACE_BEAUTY
    if (mFirstSprdFB) {
        if (fb_prev.hSprdFB) {
            rc = face_beauty_ctrl(&fb_prev, FB_BEAUTY_FAST_STOP_CMD, NULL);
            face_beauty_deinit(&fb_prev);
        }
        if (fb_prev.hSprdFB) {
            HAL_LOGE("deinitFB failed");
        }
        mFirstSprdFB = false;
    }
#endif
    HAL_LOGD("X");
    return rc;
}

int SprdPortraitAlgo::deinitLightPortrait() {
    int rc = NO_ERROR;
    HAL_LOGD("E");
    if (mFirstSprdLightPortrait) {
        if (lpt_prev.hSprdLPT) {
            deinit_lpt_handle(&lpt_prev);
        }
        if (lpt_prev.hSprdLPT) {
            HAL_LOGE("deinitLightPortrait failed");
        }
        mFirstSprdLightPortrait = false;
    }
    if (mFirstSprdDfa) {
        if (dfa_prev.hSprdDfa) {
            deinit_dfa_handle(&dfa_prev);
        }
        if (dfa_prev.hSprdDfa) {
            HAL_LOGE("deinit_dfa_handle failed");
        }
        mFirstSprdDfa = false;
    }
    HAL_LOGD("X");
    return rc;
}

void SprdPortraitAlgo::setLightPortraitParam(int param1, int param2, int param3,
                                             int param4) {
    if (!param1 || !param2 || !param3) {
        HAL_LOGE("setLightPortraitParam failed!");
        return;
    }
    lptOptions.cameraBV = param1;
    lptOptions.cameraISO = param2;
    lptOptions.cameraCT = param3;
    lptOptions.lightPortraitType = param4;
    lptOptions_cap.cameraBV = param1;
    lptOptions_cap.cameraISO = param2;
    lptOptions_cap.cameraCT = param3;
}
void SprdPortraitAlgo::getLightPortraitParam(int *param) {
    if (lpt_return_val != 0)
        *param = lpt_return_val;
    else
        *param = 0;
}

int SprdPortraitAlgo::prevLPT(void *input_buff, int picWidth, int picHeight) {
    int rc = NO_ERROR;
    HAL_LOGI("prevLPT E");
    /*run dfa */
    HAL_LOGI("runDFA E");
    ATRACE_BEGIN("dfa");
    rc = runDFA(input_buff, picWidth, picHeight, PREVIEW);
    ATRACE_END();
    HAL_LOGI("runDFA X");
    if (rc != NO_ERROR) {
        HAL_LOGE("prevLPT runDFA failed");
    }
    HAL_LOGV("lightPortraitType %d", lptOptions.lightPortraitType);
    construct_lpt_options(&lpt_prev, lptOptions);
    int index = mPreviewbokehParam.depth_param.portrait_param.face_num;
    for (int j = 0; j < index; j++) {
        int sx = 0, sy = 0, ex = 0, ey = 0;
        if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 0) {
            sx = mPreviewbokehParam.depth_param.portrait_param.x2[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            sy = mPreviewbokehParam.depth_param.portrait_param.y1[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
            ex = mPreviewbokehParam.depth_param.portrait_param.x1[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            ey = mPreviewbokehParam.depth_param.portrait_param.y2[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
        } else if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 90) {
            sx = mPreviewbokehParam.depth_param.portrait_param.x2[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            sy = mPreviewbokehParam.depth_param.portrait_param.y2[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
            ex = mPreviewbokehParam.depth_param.portrait_param.x1[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            ey = mPreviewbokehParam.depth_param.portrait_param.y1[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
        } else if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 180) {
            sx = mPreviewbokehParam.depth_param.portrait_param.x1[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            sy = mPreviewbokehParam.depth_param.portrait_param.y2[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
            ex = mPreviewbokehParam.depth_param.portrait_param.x2[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            ey = mPreviewbokehParam.depth_param.portrait_param.y1[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
        } else {
            sx = mPreviewbokehParam.depth_param.portrait_param.x1[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            sy = mPreviewbokehParam.depth_param.portrait_param.y1[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
            ex = mPreviewbokehParam.depth_param.portrait_param.x2[j] * mSize.preview_w /
                 mSize.depth_snap_out_w;
            ey = mPreviewbokehParam.depth_param.portrait_param.y2[j] * mSize.preview_h /
                 mSize.depth_snap_out_h;
        }
        int yaw = face_info.pose[j];
        int roll = face_info.angle[j];
        int fScore = Fd_score[j];
        unsigned char attriRace = 0;
        unsigned char attriGender = 0;
        unsigned char attriAge = 0;
        construct_lpt_face(&lpt_prev, j, sx, sy, ex, ey, yaw, roll, fScore,
                           attriRace, attriGender, attriAge);
        HAL_LOGD("prev_ yaw %d roll %d fScore %d", yaw, roll, fScore);
    }
    unsigned char *addrY = (unsigned char *)input_buff;
    unsigned char *addrUV = (unsigned char *)input_buff + picWidth * picHeight;
    int format = 1; /*NV21*/
    construct_lpt_image(&lpt_prev, picWidth, picHeight, addrY, addrUV, format);
    int facecount = mPreviewbokehParam.depth_param.portrait_param.face_num;

    /*do lpt */
    HAL_LOGI("do_image_lpt E");
    ATRACE_BEGIN("lpt");
    rc = do_image_lpt(&lpt_prev, facecount);
    ATRACE_END();
    HAL_LOGI("do_image_lpt X");
    lpt_return_val = rc;
    if (rc != NO_ERROR) {
        HAL_LOGD("do_image_lpt error %d", rc);
        rc = NO_ERROR;
    }
    HAL_LOGI("prevLPT X");
    return rc;
}

int SprdPortraitAlgo::capLPT(void *output_buff, int picWidth, int picHeight,
                             unsigned char *outPortraitMask, int lightPortraitType) {
    HAL_LOGD(" capLPT E");
    int rc = NO_ERROR;
    /*init handle*/
    lptOptions_cap.lightCursor =
        5; /* Control fill light and shade ratio. Value range [0, 35]*/
    lptOptions_cap.lightWeight =
        12; /* Control fill light intensity. Value range [0, 20]*/
    lptOptions_cap.lightSplitMode = LPT_LIGHTSPLIT_RIGHT;
    lptOptions_cap.debugMode = 0;
    lptOptions_cap.cameraWork = LPT_CAMERA_REAR;
    lptOptions_cap.lightPortraitType = lightPortraitType;
    int64_t lpt_cost = systemTime();
    init_lpt_options(&lpt_cap);
    create_lpt_handle(&lpt_cap, LPT_WORKMODE_STILL, 4);
    HAL_LOGD("init+create handle cost %lld ms", ns2ms(systemTime() - lpt_cost));
    if (!lpt_cap.hSprdLPT) {
        HAL_LOGE("create_lpt_handle failed!");
    }

    /*portrait mask*/
    construct_lpt_mask(&lpt_cap, 512, 384, outPortraitMask);

    /*run dfa */
    rc = runDFA(output_buff, picWidth, picHeight, CAPTURE);
    if (rc != NO_ERROR) {
        HAL_LOGE("capLPT runDFA failed");
    }
    construct_lpt_options(&lpt_cap, lptOptions_cap);
    unsigned char *addrY = (unsigned char *)output_buff;
    unsigned char *addrUV = (unsigned char *)output_buff + picWidth * picHeight;
    int format = 1; /*NV21*/
    int index = mPortraitCapParam.portrait_param.face_num;
    int facecount = mPortraitCapParam.portrait_param.face_num;
    for (int j = 0; j < index; j++) {
        int sx = 0, sy = 0, ex = 0, ey = 0;
        if (mPortraitCapParam.portrait_param.mobile_angle == 0) {
            sx = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
            sy = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
            ex = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
            ey = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
        } else if (mPortraitCapParam.portrait_param.mobile_angle == 90) {
            sx = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
            sy = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
            ex = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
            ey = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
        } else if (mPortraitCapParam.portrait_param.mobile_angle == 180) {
            sx = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
            sy = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
            ex = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
            ey = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
        } else {
            sx = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
            sy = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
            ex = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
            ey = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
        }
        int yaw = face_info.pose[j];
        int roll = face_info.angle[j];
        int fScore = Fd_score[j];
        unsigned char attriRace = 0;
        unsigned char attriGender = 0;
        unsigned char attriAge = 0;
        construct_lpt_face(&lpt_cap, j, sx, sy, ex, ey, yaw, roll, fScore,
                           attriRace, attriGender, attriAge);
        HAL_LOGD("cap yaw %d roll %d fScore %d", yaw, roll, fScore);
    }
    construct_lpt_image(&lpt_cap, picWidth, picHeight, addrY, addrUV, format);
    rc = do_image_lpt(&lpt_cap, facecount);
    if (rc != NO_ERROR) {
        HAL_LOGD("do_image_lpt error %d", rc);
        rc = NO_ERROR;
    }

    if (lpt_cap.hSprdLPT) {
        deinit_lpt_handle(&lpt_cap);
    }
    HAL_LOGD(" capLPT X");
    return rc;
}

int SprdPortraitAlgo::runDFA(void *input_buff, int picWidth, int picHeight,
                             int mode) {
    int rc = NO_ERROR;
    /*construct dfa options */
    unsigned char *addrY = (unsigned char *)input_buff;
    unsigned char *addrUV = (unsigned char *)input_buff + picWidth * picHeight;
    int format = 1; /*NV21*/
    if (mode == CAPTURE) {
        int64_t dfa_cost = systemTime();
        create_dfa_handle(&dfa_cap, 0);
        HAL_LOGD("runDFA create handle cost %lld ms",
                 ns2ms(systemTime() - dfa_cost));
        construct_dfa_yuv420sp(&dfa_cap, picWidth, picHeight, addrY, addrUV, format);
        unsigned char rType = 0;
        int index = mPortraitCapParam.portrait_param.face_num;

        for (int j = 0; j < index; j++) {
            int rX = 0, rY = 0, rWidth = 0, rHeight = 0;
            if (mPortraitCapParam.portrait_param.mobile_angle == 0) {
                rX = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
                rY = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
                rWidth = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w - rX;
                rHeight = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h - rY;
            } else if (mPortraitCapParam.portrait_param.mobile_angle == 90) {
                rX = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
                rY = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
                rWidth = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w - rX;
                rHeight = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h - rY;
            } else if (mPortraitCapParam.portrait_param.mobile_angle == 180) {
                rX = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
                rY = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
                rWidth = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w - rX;
                rHeight = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h - rY;
            } else {
                rX = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
                rY = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
                rWidth = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w - rX;
                rHeight = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h - rY;
            }
            int rRollAngle = face_info.angle[j];
            construct_dfa_face(&dfa_cap, j, rX, rY, rWidth, rHeight, rRollAngle,
                               rType);
        }
        HAL_LOGV("face_info %d %d %d %d",mPortraitCapParam.portrait_param.x1[0] * mSize.capture_w / mSize.depth_snap_out_w,
            mPortraitCapParam.portrait_param.y1[0] * mSize.capture_w / mSize.depth_snap_out_w,
            mPortraitCapParam.portrait_param.x2[0] * mSize.capture_w / mSize.depth_snap_out_w,
            mPortraitCapParam.portrait_param.y2[0] * mSize.capture_w / mSize.depth_snap_out_w);
        /*do dfa */
        DFA_RESULT dfa_result;
        do_dfa_image_yuv420sp(&dfa_cap, mPortraitCapParam.portrait_param.face_num, &dfa_result);
        construct_lpt_dfaInfo(
            &lpt_cap, dfa_result.pitch, dfa_result.yaw, dfa_result.roll,
            dfa_result.t3d, 3, dfa_result.scale, dfa_result.R, 3,
            dfa_result.alpha_shp, 40, dfa_result.alpha_exp, 20);
        if (dfa_cap.hSprdDfa) {
            deinit_dfa_handle(&dfa_cap);
        }
    } else {
        construct_dfa_yuv420sp(&dfa_prev, picWidth, picHeight, addrY, addrUV, format);
        unsigned char rType = 0;
        int index = mPreviewbokehParam.depth_param.portrait_param.face_num;
        for (int i = 0; i < index; i++) {
            int rX = 0, rY = 0, rWidth = 0, rHeight = 0;
            if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 0) {
                rX = mPreviewbokehParam.depth_param.portrait_param.x2[i] * mSize.preview_w /
                     mSize.depth_snap_out_w;
                rY = mPreviewbokehParam.depth_param.portrait_param.y1[i] * mSize.preview_h /
                     mSize.depth_snap_out_h;
                rWidth = mPreviewbokehParam.depth_param.portrait_param.x1[i] *
                             mSize.preview_w / mSize.depth_snap_out_w -
                         mPreviewbokehParam.depth_param.portrait_param.x2[i] *
                             mSize.preview_w / mSize.depth_snap_out_w;
                rHeight = mPreviewbokehParam.depth_param.portrait_param.y2[i] *
                              mSize.preview_h / mSize.depth_snap_out_h -
                          mPreviewbokehParam.depth_param.portrait_param.y1[i] *
                              mSize.preview_h / mSize.depth_snap_out_h;
            } else if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 90) {
                rX = mPreviewbokehParam.depth_param.portrait_param.x2[i] * mSize.preview_w /
                     mSize.depth_snap_out_w;
                rY = mPreviewbokehParam.depth_param.portrait_param.y2[i] * mSize.preview_h /
                     mSize.depth_snap_out_h;
                rWidth = mPreviewbokehParam.depth_param.portrait_param.x1[i] *
                             mSize.preview_w / mSize.depth_snap_out_w -
                         mPreviewbokehParam.depth_param.portrait_param.x2[i] *
                             mSize.preview_w / mSize.depth_snap_out_w;
                rHeight = mPreviewbokehParam.depth_param.portrait_param.y1[i] *
                              mSize.preview_h / mSize.depth_snap_out_h -
                          mPreviewbokehParam.depth_param.portrait_param.y2[i] *
                              mSize.preview_h / mSize.depth_snap_out_h;
            } else if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 180) {
                rX = mPreviewbokehParam.depth_param.portrait_param.x1[i] * mSize.preview_w /
                     mSize.depth_snap_out_w;
                rY = mPreviewbokehParam.depth_param.portrait_param.y2[i] * mSize.preview_h /
                     mSize.depth_snap_out_h;
                rWidth = mPreviewbokehParam.depth_param.portrait_param.x2[i] *
                             mSize.preview_w / mSize.depth_snap_out_w -
                         mPreviewbokehParam.depth_param.portrait_param.x1[i] *
                             mSize.preview_w / mSize.depth_snap_out_w;
                rHeight = mPreviewbokehParam.depth_param.portrait_param.y1[i] *
                              mSize.preview_h / mSize.depth_snap_out_h -
                          mPreviewbokehParam.depth_param.portrait_param.y2[i] *
                              mSize.preview_h / mSize.depth_snap_out_h;
            } else {
                rX = mPreviewbokehParam.depth_param.portrait_param.x1[i] * mSize.preview_w /
                     mSize.depth_snap_out_w;
                rY = mPreviewbokehParam.depth_param.portrait_param.y1[i] * mSize.preview_h /
                     mSize.depth_snap_out_h;
                rWidth = mPreviewbokehParam.depth_param.portrait_param.x2[i] *
                             mSize.preview_w / mSize.depth_snap_out_w -
                         mPreviewbokehParam.depth_param.portrait_param.x1[i] *
                             mSize.preview_w / mSize.depth_snap_out_w;
                rHeight = mPreviewbokehParam.depth_param.portrait_param.y2[i] *
                              mSize.preview_h / mSize.depth_snap_out_h -
                          mPreviewbokehParam.depth_param.portrait_param.y1[i] *
                              mSize.preview_h / mSize.depth_snap_out_h;
            }
            int rRollAngle = face_info.angle[i];
            construct_dfa_face(&dfa_prev, i, rX, rY, rWidth, rHeight, rRollAngle,
                               rType);
        }
        /*do dfa */
        DFA_RESULT dfa_result;
        do_dfa_image_yuv420sp(&dfa_prev, mPreviewbokehParam.depth_param.portrait_param.face_num, &dfa_result);
        construct_lpt_dfaInfo(
            &lpt_prev, dfa_result.pitch, dfa_result.yaw, dfa_result.roll,
            dfa_result.t3d, 3, dfa_result.scale, dfa_result.R, 3,
            dfa_result.alpha_shp, 40, dfa_result.alpha_exp, 20);
    }

    return rc;
}

int SprdPortraitAlgo::doFaceBeauty(unsigned char *mask, void *input_buff,
                                   int picWidth, int picHeight, int mode,
                                   faceBeautyLevels *facebeautylevel) {
    HAL_LOGV("E");
    int rc = NO_ERROR;
    if (mode == CAPTURE) {
#ifdef CONFIG_FACE_BEAUTY
        face_beauty_set_devicetype(&fb_cap, SPRD_CAMALG_RUN_TYPE_CPU);
        face_beauty_init(&fb_cap, 0, 2, SHARKL5PRO);
        int index = mPortraitCapParam.portrait_param.face_num;
        for (int j = 0; j < index; j++) {
            int sx = 0, sy = 0, ex = 0, ey = 0, angle = 0, pose = 0;
            beauty_face.idx = j;
            if (mPortraitCapParam.portrait_param.mobile_angle == 0) {
                beauty_face.startX = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
                beauty_face.endX = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
            } else if (mPortraitCapParam.portrait_param.mobile_angle == 90) {
                beauty_face.startX = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
                beauty_face.endX = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
            } else if (mPortraitCapParam.portrait_param.mobile_angle == 180) {
                beauty_face.startX = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
                beauty_face.endX = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
            } else {
                beauty_face.startX = mPortraitCapParam.portrait_param.x1[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPortraitCapParam.portrait_param.y1[j] * mSize.capture_h / mSize.depth_snap_out_h;
                beauty_face.endX = mPortraitCapParam.portrait_param.x2[j] * mSize.capture_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPortraitCapParam.portrait_param.y2[j] * mSize.capture_h / mSize.depth_snap_out_h;
            }
            beauty_face.angle = face_info.angle[j];
            beauty_face.pose = face_info.pose[j];
            rc = face_beauty_ctrl(&fb_cap, FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                  &beauty_face);
        }
        beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
        beauty_image.inputImage.addr[0] = (unsigned char *)input_buff;
        beauty_image.inputImage.addr[1] = 0x0;
        beauty_image.inputImage.addr[2] = 0x0;
        beauty_image.inputImage.ion_fd = 22;
        beauty_image.inputImage.offset[0] = 0;
        beauty_image.inputImage.offset[1] = picWidth * picHeight;
        beauty_image.inputImage.width = picWidth;
        beauty_image.inputImage.height = picHeight;
        beauty_image.inputImage.stride = picWidth;
        beauty_image.inputImage.size = picWidth * picHeight * 3 / 2;
        // update
        beautyLevels.blemishLevel = facebeautylevel->blemishLevel;
        beautyLevels.smoothLevel = facebeautylevel->smoothLevel;
        beautyLevels.skinColor = facebeautylevel->skinColor;
        beautyLevels.skinLevel = facebeautylevel->skinLevel;
        beautyLevels.brightLevel = facebeautylevel->brightLevel;
        beautyLevels.lipColor = facebeautylevel->lipColor;
        beautyLevels.lipLevel = facebeautylevel->lipLevel;
        beautyLevels.slimLevel = facebeautylevel->slimLevel;
        beautyLevels.largeLevel = facebeautylevel->largeLevel;
        beautyLevels.cameraBV = lptOptions_cap.cameraBV;
        beautyLevels.cameraISO = lptOptions_cap.cameraISO;
        beautyLevels.cameraCT = lptOptions_cap.cameraCT;
        rc = face_beauty_ctrl(&fb_cap, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                              &beauty_image);
        rc = face_beauty_ctrl(&fb_cap, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                              &beautyLevels);
        fb_beauty_mask_t fbMask;
        fbMask.fb_mask.width = 512;
        fbMask.fb_mask.height = 384;
        fbMask.fb_mask.data = mask;
        rc = face_beauty_ctrl(&fb_cap, FB_BEAUTY_CONSTRUCT_MASK_CMD, &(fbMask));
        fb_beauty_lptparam_t lpt_param;
        lpt_param.faceCount = mPortraitCapParam.portrait_param.face_num;
        lpt_param.lightPortraitType = lptOptions_cap.lightPortraitType;
        rc = face_beauty_ctrl(&fb_cap, FB_BEAUTY_PROCESS_CMD, &(lpt_param));

        HAL_LOGD("capture face beauty done!");
        rc = face_beauty_ctrl(&fb_cap, FB_BEAUTY_FAST_STOP_CMD, NULL);
        face_beauty_deinit(&fb_cap);
#endif
    } else {
        int index = mPreviewbokehParam.depth_param.portrait_param.face_num;
        int faceCount = mPreviewbokehParam.depth_param.portrait_param.face_num;
        for (int j = 0; j < index; j++) {
            int sx = 0, sy = 0, ex = 0, ey = 0, angle = 0, pose = 0;
            beauty_face.idx = j;
            if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 0) {
                beauty_face.startX = mPreviewbokehParam.depth_param.portrait_param.x2[j] *
                                     mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPreviewbokehParam.depth_param.portrait_param.y1[j] *
                                     mSize.preview_h /
                                     mSize.depth_snap_out_h;
                beauty_face.endX = mPreviewbokehParam.depth_param.portrait_param.x1[j] *
                                   mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPreviewbokehParam.depth_param.portrait_param.y2[j] *
                                   mSize.preview_h / mSize.depth_snap_out_h;
            } else if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 90) {
                beauty_face.startX = mPreviewbokehParam.depth_param.portrait_param.x2[j] *
                                     mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPreviewbokehParam.depth_param.portrait_param.y2[j] *
                                     mSize.preview_h /
                                     mSize.depth_snap_out_h;
                beauty_face.endX = mPreviewbokehParam.depth_param.portrait_param.x1[j] *
                                   mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPreviewbokehParam.depth_param.portrait_param.y1[j] *
                                   mSize.preview_h / mSize.depth_snap_out_h;
            } else if (mPreviewbokehParam.depth_param.portrait_param.mobile_angle == 180) {
                beauty_face.startX = mPreviewbokehParam.depth_param.portrait_param.x1[j] *
                                     mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPreviewbokehParam.depth_param.portrait_param.y2[j] *
                                     mSize.preview_h /
                                     mSize.depth_snap_out_h;
                beauty_face.endX = mPreviewbokehParam.depth_param.portrait_param.x2[j] *
                                   mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPreviewbokehParam.depth_param.portrait_param.y1[j] *
                                   mSize.preview_h / mSize.depth_snap_out_h;
            } else {
                beauty_face.startX = mPreviewbokehParam.depth_param.portrait_param.x1[j] *
                                     mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.startY = mPreviewbokehParam.depth_param.portrait_param.y1[j] *
                                     mSize.preview_h /
                                     mSize.depth_snap_out_h;
                beauty_face.endX = mPreviewbokehParam.depth_param.portrait_param.x2[j] *
                                   mSize.preview_w / mSize.depth_snap_out_w;
                beauty_face.endY = mPreviewbokehParam.depth_param.portrait_param.y2[j] *
                                   mSize.preview_h / mSize.depth_snap_out_h;
            }
            beauty_face.angle = face_info.angle[j];
            beauty_face.pose = face_info.pose[j];
            rc = face_beauty_ctrl(&fb_prev, FB_BEAUTY_CONSTRUCT_FACE_CMD,
                                  &beauty_face);
        }
        beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;

        beauty_image.inputImage.addr[0] = (unsigned char *)input_buff;
        beauty_image.inputImage.addr[1] = 0X0;
        beauty_image.inputImage.addr[2] = 0x0;
        beauty_image.inputImage.ion_fd = 22; // ADP_BUFFD(*(buffer_handle_t *)input_buff);
        beauty_image.inputImage.offset[0] = 0;
        beauty_image.inputImage.offset[1] = picWidth * picHeight;
        beauty_image.inputImage.width = picWidth;
        beauty_image.inputImage.height = picHeight;
        beauty_image.inputImage.stride = picWidth;
        beauty_image.inputImage.size = picWidth * picHeight * 3 / 2;
        // update
        beautyLevels.blemishLevel = facebeautylevel->blemishLevel;
        beautyLevels.smoothLevel = facebeautylevel->smoothLevel;
        beautyLevels.skinColor = facebeautylevel->skinColor;
        beautyLevels.skinLevel = facebeautylevel->skinLevel;
        beautyLevels.brightLevel = facebeautylevel->brightLevel;
        beautyLevels.lipColor = facebeautylevel->lipColor;
        beautyLevels.lipLevel = facebeautylevel->lipLevel;
        beautyLevels.slimLevel = facebeautylevel->slimLevel;
        beautyLevels.largeLevel = facebeautylevel->largeLevel;
        beautyLevels.cameraBV = lptOptions.cameraBV;
        beautyLevels.cameraISO = lptOptions.cameraISO;
        beautyLevels.cameraCT = lptOptions.cameraCT;
        rc = face_beauty_ctrl(&fb_prev, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
                              &beauty_image);
        rc = face_beauty_ctrl(&fb_prev, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
                              &beautyLevels);
        fb_beauty_lptparam_t lpt_param;
        lpt_param.faceCount = mPreviewbokehParam.depth_param.portrait_param.face_num;
        lpt_param.lightPortraitType = lptOptions.lightPortraitType;
        rc = face_beauty_ctrl(&fb_prev, FB_BEAUTY_PROCESS_CMD, &(lpt_param));
    }
    HAL_LOGV("X");
    return rc;
}

int SprdPortraitAlgo::getPortraitMask(void *output_buff, void *input_buf1_addr,
                                      int vcmCurValue, unsigned char *result) {
    /*get portrait_mask*/
    HAL_LOGD("E");
    int rc = NO_ERROR;
    int f_number = 0;
    weightmap_param weightParams;
    PortaitCapProcParams wParams;
    InoutYUV yuvData;

    if (!output_buff || !input_buf1_addr) {
        HAL_LOGE(" buff is null");
        rc = BAD_VALUE;
        return rc;
    }
    memset(&wParams, 0, sizeof(PortaitCapProcParams));
    memset(&yuvData, 0, sizeof(InoutYUV));

    wParams.DisparityImage = NULL;
    wParams.VCM_cur_value = vcmCurValue;
    memcpy(&wParams.golden_vcm_data, &mPortraitCapParam.relbokeh_oem_data,
           sizeof(struct af_golden_vcm_data));
    wParams.version = 1;
    wParams.roi_type = 2;
    f_number = mPortraitCapParam.f_number;
    wParams.F_number = (MAX_F_FUMBER + 1 - f_number) * 255 / MAX_F_FUMBER;
    wParams.sel_x = mPortraitCapParam.sel_x * mSize.capture_w / mSize.preview_w;
    wParams.sel_y = mPortraitCapParam.sel_y * mSize.capture_h / mSize.preview_h;
    wParams.CircleSize = 50;
    wParams.valid_roi = mPortraitCapParam.portrait_param.valid_roi;
    wParams.total_roi = mPortraitCapParam.portrait_param.face_num;
    memcpy(&wParams.x1, &mPortraitCapParam.portrait_param.x1,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    memcpy(&wParams.x2, &mPortraitCapParam.portrait_param.x2,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    memcpy(&wParams.y1, &mPortraitCapParam.portrait_param.y1,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    memcpy(&wParams.y2, &mPortraitCapParam.portrait_param.y2,
           mPortraitCapParam.portrait_param.face_num * sizeof(int));
    wParams.rear_cam_en = mPortraitCapParam.portrait_param.rear_cam_en; // true
    wParams.rotate_angle = mPortraitCapParam.portrait_param.mRotation;  //--
    wParams.camera_angle = mPortraitCapParam.portrait_param.camera_angle;
    wParams.mobile_angle = mPortraitCapParam.portrait_param.mobile_angle;

    yuvData.Src_YUV = (unsigned char *)input_buf1_addr;
    yuvData.Dst_YUV = (unsigned char *)output_buff;

    unsigned int maskW = 0, maskH = 0, maskSize = 0;
    rc = sprd_portrait_capture_get_mask_info(mPortraitHandle, &maskW, &maskH,
                                             &maskSize);

    rc = sprd_portrait_capture_process_lpt(mPortraitHandle, NULL, &wParams,
                                           &yuvData, NULL, 1, result);

    HAL_LOGD("x");
    return rc;
}

void SprdPortraitAlgo::setFaceInfo(int *angle, int *pose, int *fd_score) {
    for (int i = 0; i < 10; i++) {
        face_info.angle[i] = *(angle + i);
        face_info.pose[i] = *(pose + i);
        Fd_score[i]= *(fd_score+i);
    }
}
}
