#include "ArcSoftBokehAlgo.h"

using namespace android;
namespace sprdcamera {

ArcSoftBokehAlgo::ArcSoftBokehAlgo() {
    mDepthSize = 0;
    mFirstArcBokeh = false;
    mReadOtp = false;
#ifdef CONFIG_ALTEK_ZTE_CALI
    mReadZteOtp = false;
    mVcmSteps = 0;
#endif
    mPrevHandle = NULL;
    mCapHandle = NULL;
    mDepthMap = NULL;

    memset(&mSize, 0, sizeof(BokehSize));
    memset(&mCalData, 0, sizeof(OtpData));
    memset(&mPrevParam, 0, sizeof(ARC_DCVR_PARAM));
    memset(&mCapParam, 0, sizeof(ARC_DCIR_REFOCUS_PARAM));
    memset(&mDcrParam, 0, sizeof(ARC_DCIR_PARAM));
    memset(&mData, 0, sizeof(ARC_DC_CALDATA));
    memset(&mArcSoftInfo, 0, sizeof(ARC_REFOCUSCAMERAIMAGE_PARAM));
}

ArcSoftBokehAlgo::~ArcSoftBokehAlgo() {
    mFirstArcBokeh = false;
#ifdef CONFIG_ALTEK_ZTE_CALI
    mReadZteOtp = false;
#endif
    mReadOtp = false;
    mDepthMap = NULL;
}

int ArcSoftBokehAlgo::initParam(BokehSize *size, OtpData *data, bool galleryBokeh) {
    int rc = NO_ERROR;
    const MPBASE_Version *version = NULL;
    int CAM_BOKEH_MAIN_ID = 0;
    int CAM_DEPTH_ID = 2;

    if (!size || !data) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        return rc;
    }
    HAL_LOGD("arcsoft init Bokeh Api Params");
    memcpy(&mSize, size, sizeof(BokehSize));
    if (data->otp_size) {
        memcpy(&mCalData, data, sizeof(OtpData));
    }
    mData.pCalibData = mCalData.otp_data;
    mData.i32CalibDataSize = data->otp_size;
#ifndef CONFIG_ALTEK_ZTE_CALI
    mArcSoftInfo.i32LeftFullWidth =
        (MInt32)SprdCamera3Setting::s_setting[CAM_BOKEH_MAIN_ID]
            .sensor_InfoInfo.pixer_array_size[0];
    mArcSoftInfo.i32LeftFullHeight =
        (MInt32)SprdCamera3Setting::s_setting[CAM_BOKEH_MAIN_ID]
            .sensor_InfoInfo.pixer_array_size[1];
    mArcSoftInfo.i32RightFullWidth =
        (MInt32)SprdCamera3Setting::s_setting[CAM_DEPTH_ID]
            .sensor_InfoInfo.pixer_array_size[0];
    mArcSoftInfo.i32RightFullHeight =
        (MInt32)SprdCamera3Setting::s_setting[CAM_DEPTH_ID]
            .sensor_InfoInfo.pixer_array_size[1];
#else
    mArcSoftInfo.i32LeftFullWidth = mArcParam.left_image_height;
    mArcSoftInfo.i32LeftFullHeight = mArcParam.left_image_width;
    mArcSoftInfo.i32RightFullWidth = mArcParam.left_image_height;
    mArcSoftInfo.i32RightFullHeight = mArcParam.left_image_width;
#endif
    mDcrParam.faceParam.prtFaces = NULL;
    mDcrParam.faceParam.i32FacesNum = 0;
    HAL_LOGD("i32LeftFullWidth %d , i32LeftFullHeight %d,i32RightFullWidth "
             "%d,i32RightFullHeight %d",
             mArcSoftInfo.i32LeftFullWidth, mArcSoftInfo.i32LeftFullHeight,
             mArcSoftInfo.i32RightFullWidth, mArcSoftInfo.i32RightFullHeight);

    // preview params
    mPrevParam.ptFocus.x = (MInt32)(mSize.preview_w / 2);
    mPrevParam.ptFocus.y = (MInt32)(mSize.preview_h / 2);
    mPrevParam.i32BlurLevel = 100;
    mPrevParam.bRefocusOn = 1;

    // capture params
    mCapParam.ptFocus.x = (MInt32)(mSize.capture_w / 2);
    mCapParam.ptFocus.y = (MInt32)(mSize.capture_h / 2);
    mCapParam.i32BlurIntensity = 100;

    mDcrParam.fMaxFOV = arcsoft_config_para.fMaxFOV;
    mDcrParam.i32ImgDegree = 270;

    version = ARC_DCVR_GetVersion();
    if (version == NULL) {
        HAL_LOGE("ARC_DCVR_GetVersion failed!");
    } else {
        HAL_LOGD("ARC_DCVR_GetVersion version [%s]", version->Version);
    }

    version = ARC_DCIR_GetVersion();
    if (version == NULL) {
        HAL_LOGE("ARC_DCIR_GetVersion failed!");
    } else {
        HAL_LOGD("ARC_DCIR_GetVersion version [%s]", version->Version);
    }

#ifdef CONFIG_ALTEK_ZTE_CALI
    if ((mCalData.otp_type == OTP_CALI_ALTEK) && (mReadZteOtp == false)) {
        createArcSoftCalibrationData((unsigned char *)mData.pCalibData,
                                     mData.i32CalibDataSize);
        mReadZteOtp = true;
    }
#endif

    if (mReadOtp == false) {
        loadDebugOtp();
        data->otp_exist = mCalData.otp_exist;
        mReadOtp = true;
    }
#ifdef CONFIG_ALTEK_ZTE_CALI
    memcpy(data->otp_data, mData.pCalibData, sizeof(THIRD_OTP_SIZE));
#endif
    HAL_LOGD("otp size %d, data %p", mData.i32CalibDataSize, mData.pCalibData);
    return rc;
}

void ArcSoftBokehAlgo::getVersionInfo() {}

void ArcSoftBokehAlgo::getBokenParam(void *param) {
    if ((mDepthSize == 0) || !mDepthMap || !param) {
        HAL_LOGE("para is illegal");
        return;
    }

    memcpy(&mBokehParams.prev, &mPrevParam, sizeof(ARC_DCVR_PARAM));
    memcpy(&mBokehParams.cap, &mCapParam, sizeof(ARC_DCIR_REFOCUS_PARAM));
    mBokehParams.size = mDepthSize;
    mBokehParams.map = mDepthMap;
    memcpy(&((BOKEH_PARAM *)param)->arc, &mBokehParams,
           sizeof(ARC_BOKEH_PARAM));
}

void ArcSoftBokehAlgo::setBokenParam(void *param) {
    if (!param) {
        HAL_LOGE("para is illegal");
        return;
    }

    Mutex::Autolock l(mSetParaLock);
    bokeh_params bokeh_param;
    memset(&bokeh_param, 0, sizeof(bokeh_param));
    memcpy(&bokeh_param, (bokeh_params *)param, sizeof(bokeh_params));

    int fnum = MAX_F_FUMBER + 1 - bokeh_param.f_number;
    mPrevParam.ptFocus.x = bokeh_param.sel_x;
    mPrevParam.ptFocus.y = bokeh_param.sel_y;
    mPrevParam.i32BlurLevel = (MInt32)(fnum * 100 / MAX_F_FUMBER);
    mPrevParam.bRefocusOn = !bokeh_param.isFocus;
#ifdef CONFIG_ALTEK_ZTE_CALI
    mVcmSteps = bokeh_param.vcm;
#endif
}

int ArcSoftBokehAlgo::prevDepthRun(void *para1, void *para2, void *para3, void *para4) {
    int rc = NO_ERROR;
    int64_t arcsoftprevRun = 0;
    ASVLOFFSCREEN leftImg, rightImg, dstImg;
    MLong lWidth = 0, lHeight = 0;
    MByte *pLeftImgDataY = NULL, *pLeftImgDataUV = NULL, *pRightImgDataY = NULL,
          *pRightImgDataUV = NULL, *pDstDataY = NULL, *pDstDataUV = NULL;

    if (!para1 || !para2 || !para3) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        goto exit;
    }

    lWidth = (MLong)(mSize.preview_w);
    lHeight = (MLong)(mSize.preview_h);
    pLeftImgDataY = (MByte *)para1;
    pLeftImgDataUV = pLeftImgDataY + lWidth * lHeight;

    pRightImgDataY = (MByte *)para2;
    pRightImgDataUV = pRightImgDataY + lWidth * lHeight;

    pDstDataY = (MByte *)para3;
    pDstDataUV = pDstDataY + lWidth * lHeight;

    memset(&leftImg, 0, sizeof(ASVLOFFSCREEN));
    leftImg.i32Width = lWidth;
    leftImg.i32Height = lHeight;
    leftImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    leftImg.pi32Pitch[0] = lWidth;
    leftImg.ppu8Plane[0] = pLeftImgDataY;
    leftImg.pi32Pitch[1] = lWidth / 2 * 2;
    leftImg.ppu8Plane[1] = pLeftImgDataUV;

    memset(&rightImg, 0, sizeof(ASVLOFFSCREEN));
    rightImg.i32Width = lWidth;
    rightImg.i32Height = lHeight;
    rightImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    rightImg.pi32Pitch[0] = lWidth;
    rightImg.ppu8Plane[0] = pRightImgDataY;
    rightImg.pi32Pitch[1] = lWidth / 2 * 2;
    rightImg.ppu8Plane[1] = pRightImgDataUV;

    memset(&dstImg, 0, sizeof(ASVLOFFSCREEN));
    dstImg.i32Width = lWidth;
    dstImg.i32Height = lHeight;
    dstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    dstImg.pi32Pitch[0] = lWidth;
    dstImg.ppu8Plane[0] = pDstDataY;
    dstImg.pi32Pitch[1] = lWidth / 2 * 2;
    dstImg.ppu8Plane[1] = pDstDataUV;
    arcsoftprevRun = systemTime();

    HAL_LOGD("preview blur level %d, focus %d, coordinate (%d,%d)",
             mPrevParam.i32BlurLevel, mPrevParam.bRefocusOn,
             mPrevParam.ptFocus.x, mPrevParam.ptFocus.y);
    rc = ARC_DCVR_Process(mPrevHandle, &leftImg, &rightImg, &dstImg,
                          &mPrevParam);
    HAL_LOGD("arcsoftprevRun cost %lld ms, rc %d",
             ns2ms(systemTime() - arcsoftprevRun), rc);
exit:
    return rc;
}

int ArcSoftBokehAlgo::initAlgo() {
    int rc = NO_ERROR;
    MInt32 i32ImgDegree = 270;
    if (mFirstArcBokeh) {
        if (mPrevHandle != NULL) {
            rc = ARC_DCVR_Uninit(&mPrevHandle);
            if (rc != MOK) {
                HAL_LOGE("preview ARC_DCVR_Uninit failed!");
                goto exit;
            }
        }
    }
#ifndef CONFIG_ALTEK_ZTE_CALI
    rc = ARC_DCVR_Init(&mPrevHandle);
    if (rc != MOK) {
        HAL_LOGE("ARC_DCVR_Init failed ,rc =%ld", rc);
        goto exit;
    }
    // preview parameter settings
    rc = ARC_DCVR_SetCameraImageInfo(mPrevHandle, &mArcSoftInfo);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCVR_SetCameraImageInfo failed");
    } else {
        HAL_LOGD("arcsoft ARC_DCVR_SetCameraImageInfo succeed");
    }
    rc = ARC_DCVR_SetImageDegree(mPrevHandle, i32ImgDegree);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCVR_SetImageDegree failed");
    } else {
        HAL_LOGD("arcsoft ARC_DCVR_SetImageDegree succeed");
    }
    // read cal data
    rc = ARC_DCVR_SetCaliData(mPrevHandle, &mData);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCVR_SetCaliData failed");
    } else {
        HAL_LOGD("arcsoft ARC_DCVR_SetCaliData succeed");
    }
#endif
    if (mDepthMap != NULL) {
        free(mDepthMap);
    }
    mDepthMap = malloc(ARCSOFT_DEPTH_DATA_SIZE);
    if (mDepthMap == NULL) {
        HAL_LOGE("mDepthMap malloc failed.");
    }

    HAL_LOGD("arcsoft OK mFirstArcBokeh %d", mFirstArcBokeh);
    mFirstArcBokeh = true;
exit:
    return rc;
}

int ArcSoftBokehAlgo::deinitAlgo() {
    int rc = NO_ERROR;
    return rc;
}

int ArcSoftBokehAlgo::initPrevDepth() {
    int rc = NO_ERROR;
    return rc;
}

int ArcSoftBokehAlgo::deinitPrevDepth() {
    int rc = NO_ERROR;
    if (mFirstArcBokeh) {
        MRESULT rc = MOK;
        if (mPrevHandle != NULL) {
            rc = ARC_DCVR_Uninit(&mPrevHandle);
            if (rc != MOK) {
                HAL_LOGE("preview ARC_DCVR_Uninit failed!");
                return rc;
            }
        }
    }
    mPrevHandle = NULL;
    return rc;
}

int ArcSoftBokehAlgo::prevBluImage(sp<GraphicBuffer> &srcBuffer,
                                   sp<GraphicBuffer> &dstBuffer, void *param) {
    int rc = NO_ERROR;
    return rc;
}

int ArcSoftBokehAlgo::setFlag() {
    int rc = NO_ERROR;
    return rc;
}

int ArcSoftBokehAlgo::initCapDepth() {
    int rc = NO_ERROR;
    return rc;
}

int ArcSoftBokehAlgo::deinitCapDepth() {
    int rc = NO_ERROR;
    mCapHandle = NULL;

    return rc;
}

int ArcSoftBokehAlgo::capDepthRun(void *para1, void *para2, void *para3,
                                  void *para4) {
    int rc = NO_ERROR;
    int64_t depthRun = 0;
    if (!para1 || !para2) {
        HAL_LOGE("para is illegal");
        return UNKNOWN_ERROR;
    }
    ASVLOFFSCREEN leftImg;
    ASVLOFFSCREEN rightImg;
    MInt32 lDMSize = 0;
    MLong LWidth = 0, LHeight = 0;
    MLong RWidth = 0, RHeight = 0;
    MByte *pLeftImgDataY = NULL, *pLeftImgDataUV = NULL, *pRightImgDataY = NULL,
          *pRightImgDataUV = NULL;

    LWidth = (MLong)(mSize.capture_w);
    LHeight = (MLong)(mSize.capture_h);
    RWidth = (MLong)(mSize.depth_snap_sub_w);
    RHeight = (MLong)(mSize.depth_snap_sub_h);
    pLeftImgDataY = (MByte *)para1;
    pLeftImgDataUV = pLeftImgDataY + LWidth * LHeight;

    pRightImgDataY = (MByte *)para2;
    pRightImgDataUV = pRightImgDataY + RWidth * RHeight;

    memset(&leftImg, 0, sizeof(ASVLOFFSCREEN));
    leftImg.i32Width = LWidth;
    leftImg.i32Height = LHeight;
    leftImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    leftImg.pi32Pitch[0] = LWidth;
    leftImg.ppu8Plane[0] = pLeftImgDataY;
    leftImg.pi32Pitch[1] = LWidth / 2 * 2;
    leftImg.ppu8Plane[1] = pLeftImgDataUV;

    memset(&rightImg, 0, sizeof(ASVLOFFSCREEN));
    rightImg.i32Width = RWidth;
    rightImg.i32Height = RHeight;
    rightImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    rightImg.pi32Pitch[0] = RWidth;
    rightImg.ppu8Plane[0] = pRightImgDataY;
    rightImg.pi32Pitch[1] = RWidth / 2 * 2;
    rightImg.ppu8Plane[1] = pRightImgDataUV;

    rc = ARC_DCIR_Init(&mCapHandle, arcsoft_config_para.lFocusMode);
    if (rc != MOK) {
        HAL_LOGE("ARC_DCIR_Init failed!");
        goto exit;
    } else {
        HAL_LOGD("ARC_DCIR_Init succeed");
    }
#ifdef CONFIG_ALTEK_ZTE_CALI
    if (mCalData.otp_type == OTP_CALI_ALTEK) {
        rc = createArcSoftCalibrationData((unsigned char *)mData.pCalibData,
                                          mData.i32CalibDataSize);
        if (rc != MOK) {
            HAL_LOGE("create arcsoft calibration failed");
        }
    }
#endif
    // read cal data
    HAL_LOGI("begin ARC_DCIR_SetCaliData");

    rc = ARC_DCIR_SetCaliData(mCapHandle, &mData);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCIR_SetCaliData failed");
    }

#ifndef CONFIG_ALTEK_ZTE_CALI
    rc = ARC_DCIR_SetDistortionCoef(mCapHandle, arcsoft_config_para.leftDis,
                                    arcsoft_config_para.rightDis);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCIR_SetDistortionCoef failed");
    }
#endif
    rc = ARC_DCIR_SetCameraImageInfo(mCapHandle, &mArcSoftInfo);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCIR_SetCameraImageInfo failed");
    }
    HAL_LOGI("ARC_DCIR_SetCameraImageInfo i32LeftFullWidth:%d "
             "i32LeftFullHeight:%d i32RightFullWidth:%d "
             "i32RightFullHeight:%d",
             mArcSoftInfo.i32LeftFullWidth, mArcSoftInfo.i32LeftFullHeight,
             mArcSoftInfo.i32RightFullWidth, mArcSoftInfo.i32RightFullHeight);

    depthRun = systemTime();

    rc =
        ARC_DCIR_CalcDisparityData(mCapHandle, &leftImg, &rightImg, &mDcrParam);
    if (rc != MOK) {
        HAL_LOGD("arcsoft ARC_DCIR_CalcDisparityData failed,res=%d", rc);
        if (rc == 5) {
            rc = NO_ERROR;
        } else {
            goto exit;
        }
    }

    rc = ARC_DCIR_GetDisparityDataSize(mCapHandle, &lDMSize);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCIR_GetDisparityDataSize failed");
        goto exit;
    }
    if (mDepthSize != lDMSize) {
        if (mDepthMap != NULL && mDepthSize) {
            free(mDepthMap);
        }
        mDepthSize = lDMSize;
        mDepthMap = malloc(mDepthSize);
        if (!mDepthMap) {
            HAL_LOGE("arcsoft pDispMap error!");
            goto exit;
        } else {
            HAL_LOGD("arcsoft pDispMap apply memory succeed, lDMSize %ld",
                     lDMSize);
        }
    }
    rc = ARC_DCIR_GetDisparityData(mCapHandle, mDepthMap);
    if (rc != MOK) {
        HAL_LOGE("arcsoft AARC_DCIR_GetDisparityData failed");
    }

    HAL_LOGD("ARC_DCIR_CalcDisparityData cost %lld ms",
             ns2ms(systemTime() - depthRun));
    HAL_LOGD("ARC_DCIR_CalcDisparityData leftImg.i32Width:%d, "
             "leftImg.i32Height:%d, rightImg.i32Width:%d "
             "rightImg.i32Height:%d",
             ((ASVLOFFSCREEN *)para1)->i32Width,
             ((ASVLOFFSCREEN *)para1)->i32Height,
             ((ASVLOFFSCREEN *)para2)->i32Width,
             ((ASVLOFFSCREEN *)para2)->i32Height);
    HAL_LOGD("ARC_DCIR_CalcDisparityData fMaxFOV:%f, i32ImgDegree:%d "
             "i32FacesNum:%d",
             mDcrParam.fMaxFOV, mDcrParam.i32ImgDegree,
             mDcrParam.faceParam.i32FacesNum);

    return MOK;
exit:
    if (ARC_DCIR_Uninit(&mCapHandle)) {
        HAL_LOGE("arcsoft ARC_DCIR_Uninit failed");
    }
    return rc;
}

int ArcSoftBokehAlgo::capBlurImage(void *para1, void *para2, void *para3, int depthW, int depthH) {
    int rc = NO_ERROR;
    int64_t bokehRun = 0;
    ASVLOFFSCREEN leftImg;
    ASVLOFFSCREEN dstImg;
    MInt32 lDMSize = 0;
    MLong LWidth = 0, LHeight = 0;
    MLong RWidth = 0, RHeight = 0;
    MByte *pLeftImgDataY = NULL, *pLeftImgDataUV = NULL;
    MByte *pDstDataY = NULL, *pDstDataUV = NULL;
    if (!para1 || !para2) {
        HAL_LOGE("para is illegal");
        return UNKNOWN_ERROR;
    }

    LWidth = (MLong)(mSize.capture_w);
    LHeight = (MLong)(mSize.capture_h);
    RWidth = (MLong)(mSize.depth_snap_sub_w);
    RHeight = (MLong)(mSize.depth_snap_sub_h);
    pLeftImgDataY = (MByte *)para1;
    pLeftImgDataUV = pLeftImgDataY + LWidth * LHeight;

    memset(&leftImg, 0, sizeof(ASVLOFFSCREEN));
    leftImg.i32Width = LWidth;
    leftImg.i32Height = LHeight;
    leftImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    leftImg.pi32Pitch[0] = LWidth;
    leftImg.ppu8Plane[0] = pLeftImgDataY;
    leftImg.pi32Pitch[1] = LWidth / 2 * 2;
    leftImg.ppu8Plane[1] = pLeftImgDataUV;

    memset(&dstImg, 0, sizeof(ASVLOFFSCREEN));
    pDstDataY = (MByte *)para1;
    pDstDataUV = pDstDataY + LWidth * LHeight;

    dstImg.i32Width = LWidth;
    dstImg.i32Height = LHeight;
    dstImg.u32PixelArrayFormat = ASVL_PAF_NV21;
    dstImg.pi32Pitch[0] = LWidth;
    dstImg.ppu8Plane[0] = pDstDataY;
    dstImg.pi32Pitch[1] = LWidth / 2 * 2;
    dstImg.ppu8Plane[1] = pDstDataUV;

    mCapParam.ptFocus.x =
        mPrevParam.ptFocus.x * mSize.capture_w / mSize.preview_w;
    mCapParam.ptFocus.y =
        mPrevParam.ptFocus.y * mSize.capture_h / mSize.preview_h;
    mCapParam.i32BlurIntensity =
        mPrevParam.i32BlurLevel * arcsoft_config_para.blurCapPrevRatio;
    HAL_LOGD("capture blur level %d, coordinate (%d,%d)",
             mCapParam.i32BlurIntensity, mCapParam.ptFocus.x,
             mCapParam.ptFocus.y);

    bokehRun = systemTime();
    rc = ARC_DCIR_Process(mCapHandle, mDepthMap, mDepthSize, &leftImg,
                          &mCapParam, &dstImg);
    if (rc != MOK) {
        HAL_LOGE("arcsoft ARC_DCIR_Process failed");
    }
    HAL_LOGD("ARC_DCIR_Process cost %lld ms", ns2ms(systemTime() - bokehRun));

    return rc;
}
int ArcSoftBokehAlgo::onLine(void *para1, void *para2, void *para3, void *para4) {
    int rc = NO_ERROR;
    return rc;
}

#ifdef CONFIG_ALTEK_ZTE_CALI
int ArcSoftBokehAlgo::createArcSoftCalibrationData(unsigned char *pBuffer,
                                                   int nBufSize) {
    MRESULT rc = MOK;
    MHandle handle = 0;
    AltekParam stAltkeParam;

    if (!pBuffer) {
        HAL_LOGE(" para is null");
        rc = BAD_VALUE;
        return rc;
    }

    memset(&mArcParam, 0, sizeof(ArcParam));

    HAL_LOGI("E");

    rc = Arc_CaliData_Init(&handle);
    if (rc != MOK) {
        HAL_LOGE("ArcSoft_C failed to Arc_CaliData_Init, rc:%d", (int)rc);
        return UNKNOWN_ERROR;
    }

    stAltkeParam.a_dInVCMStep = mVcmSteps;
    stAltkeParam.a_dInCamLayout = arcsoft_config_para.a_dInCamLayout; // type3

    rc = Arc_CaliData_ParseDualCamData(handle, pBuffer, nBufSize,
                                       cali_type_altek, &stAltkeParam);
    if (rc != MOK) {
        HAL_LOGE("ArcSoft_C ailed to Arc_CaliData_ParseDualCamData, rc:%d",
                 (int)rc);
        goto exit;
    }

    rc = Arc_CaliData_ParseOTPData(handle, pBuffer, nBufSize, otp_type_zte,
                                   NULL);
    if (rc != MOK) {
        HAL_LOGE("ArcSoft_C failed to Arc_CaliData_ParseOTPData, rc:%d",
                 (int)rc);
        goto exit;
    }

    rc = Arc_CaliData_GetArcParam(handle, &mArcParam, sizeof(ArcParam));
    if (rc != MOK) {
        HAL_LOGE("ArcSoft_C failed to Arc_CaliData_GetArcParam, rc:%d",
                 (int)rc);
        goto exit;
    }

    Arc_CaliData_PrintArcParam(&mArcParam);

    mData.i32CalibDataSize = sizeof(ArcParam);
    mData.pCalibData = (MVoid *)&mArcParam;

    HAL_LOGI("i32CalibDataSize:%d, pCalibData: %p", mData.i32CalibDataSize,
             mData.pCalibData);
exit:
    Arc_CaliData_Uninit(&handle);

    HAL_LOGI("X");

    return rc;
}
#endif

void ArcSoftBokehAlgo::loadDebugOtp() {
    int rc = NO_ERROR;
    FILE *fid = fopen("/productinfo/calibration.bin", "rb");
    if (fid != NULL) {
        HAL_LOGD("open calibration file success");
        rc = fread((char *)mData.pCalibData, sizeof(char),
                   ARCSOFT_CALIB_DATA_SIZE, fid);
        HAL_LOGD("read otp size %d bytes", rc);
        fclose(fid);
        mData.i32CalibDataSize = ARCSOFT_CALIB_DATA_SIZE;
        mCalData.otp_exist = true;
    } else {
        HAL_LOGW("no calibration file ");
    }
}
}
