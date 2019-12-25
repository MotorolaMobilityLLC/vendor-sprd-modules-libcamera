/* Copyright (c) 2016, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define LOG_TAG "Cam33dZoomV1"
//#define LOG_NDEBUG 0
#include "SprdCamera3OpticsZoomV1.h"

using namespace android;
namespace sprdcamera {

SprdCamera3OpticsZoomV1 *gZoomV1 = NULL;
#define PROC_3Zoom_TIMEOUT 3
#define SWITCH_WIDE_TELE_REQUEST (1.7)
#define MAX_DIGITAL_ZOOM_RATIO_OPTICS 8
// Error Check Macros
#define CHECK_MUXER()                                                          \
    if (!gZoomV1) {                                                            \
        HAL_LOGE("Error getting muxer ");                                      \
        return;                                                                \
    }

/*===========================================================================
 * FUNCTION         : SprdCamera3OpticsZoomV1
 *
 * DESCRIPTION     : SprdCamera3OpticsZoom Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3OpticsZoomV1::SprdCamera3OpticsZoomV1() {
    HAL_LOGI(" E");
    mCapInputbuffer = NULL;
    mTeleMaxWidth = 0;
    mTeleMaxHeight = 0;
    mWideMaxWidth = 0;
    mWideMaxHeight = 0;
    mVcmSteps = 0;
    mZoomValue = 1.0f;
    mZoomValueTh = SWITCH_WIDE_TELE_REQUEST;
    mAlgoStatus = NO_ALGO;
    mPrevAlgoHandle = NULL;
    mCapAlgoHandle = NULL;
    memset(&mOtpData, 0, sizeof(OtpData));
    mPreviewMuxerThread = new TWPreviewMuxerThread();
    mTWCaptureThread = new TWCaptureThread();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux1.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~~SprdCamera3OpticsZoomV1
 *
 * DESCRIPTION     : SprdCamera3OpticsZoom Desctructor
 *
 *==========================================================================*/
SprdCamera3OpticsZoomV1::~SprdCamera3OpticsZoomV1() {
    HAL_LOGI("E");

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCamera3dZoomV1
 *
 * DESCRIPTION     : Creates Camera Muxer if not created
 *
 * PARAMETERS:
 *   @pMuxer               : Pointer to retrieve Camera Muxer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::getCamera3dZoomV1(SprdCamera3Multi **pMuxer) {
    *pMuxer = NULL;
    if (!gZoomV1) {
        gZoomV1 = new SprdCamera3OpticsZoomV1();
    }
    CHECK_MUXER();
    *pMuxer = gZoomV1;
    HAL_LOGD("gZoomV1: %p ", gZoomV1);

    return;
}

static config_multi_camera optics_zoom_config = {
#include "optics_zoom_config_v1.h"
};
/*===========================================================================
 * FUNCTION         : load_config_file
 *
 * DESCRIPTION     : load zoomV1 configure file
 *
 * RETURN             :  NONE
 *==========================================================================*/
config_multi_camera *SprdCamera3OpticsZoomV1::load_config_file(void) {

    HAL_LOGD("load_config_file ");
    return &optics_zoom_config;
}
/*===========================================================================
 * FUNCTION         : reConfigGetCameraInfo
 *
 * DESCRIPTION     : update metadata info in get_camera_info
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::reConfigGetCameraInfo(CameraMetadata &metadata) {

    int sub_camera_id = m_pPhyCamera[1].id;

    int array_size;
#define FILL_CAM_INFO_ARRAY(Array, Start, Num, Flag)                           \
    for (array_size = Start; array_size < Num; array_size++) {                 \
        if (Array[array_size * 4] == 0)                                        \
            break;                                                             \
    }                                                                          \
    metadata.update(Flag, Array, array_size * 4);

    if (metadata.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
        mWideMaxWidth =
            metadata.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)
                .data.i32[1];
        mWideMaxHeight =
            metadata.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)
                .data.i32[2];
        HAL_LOGD("mWideMaxWidth=%d, mWideMaxHeight=%d", mWideMaxWidth,
                 mWideMaxHeight);
    }

    FILL_CAM_INFO_ARRAY(SprdCamera3Setting::s_setting[sub_camera_id]
                            .scalerInfo.available_stream_configurations,
                        0, CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
                        ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)

    FILL_CAM_INFO_ARRAY(SprdCamera3Setting::s_setting[sub_camera_id]
                            .scalerInfo.min_frame_durations,
                        0, CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
                        ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS)
    FILL_CAM_INFO_ARRAY(
        SprdCamera3Setting::s_setting[sub_camera_id].scalerInfo.stall_durations,
        0, CAMERA_SETTINGS_CONFIG_ARRAYSIZE,
        ANDROID_SCALER_AVAILABLE_STALL_DURATIONS)

    if (metadata.exists(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)) {
        mTeleMaxWidth =
            metadata.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)
                .data.i32[1];
        mTeleMaxHeight =
            metadata.find(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS)
                .data.i32[2];
        HAL_LOGD("mTeleMaxWidth=%d, mTeleMaxHeight=%d", mTeleMaxWidth,
                 mTeleMaxHeight);
    }
    float max_zoom = MAX_DIGITAL_ZOOM_RATIO_OPTICS;
    metadata.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &max_zoom, 1);
    uint8_t available = 0;
    metadata.update(ANDROID_FLASH_INFO_AVAILABLE, &available, 1);

    return;
}
/*===========================================================================
 * FUNCTION         : setZoomRatio
 *
 * DESCRIPTION     : set zoom/crop meta info
 *
 * RETURN             :  zoom ratio
 *==========================================================================*/
#define MAX_ROI 10

camera_metadata_t *
SprdCamera3OpticsZoomV1::reConfigResultMeta(camera_metadata_t *meta) {

    CameraMetadata *camMetadata = new CameraMetadata(meta);
    int32_t g_face_info[4];
    uint8_t face_num = 0;
    int32_t faceRectangles[MAX_ROI * 4];
    float h_ratio = (float)mTeleMaxHeight / mWideMaxHeight;
    float w_Ratio = (float)mTeleMaxWidth / mWideMaxWidth;
    camera_metadata_entry_t entry;
    entry = camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES);
    if (entry.count != 0) {
        face_num = entry.count / 4;
    } else {
        face_num = 0;
    }
    HAL_LOGV("w_Ratio=%f,h_ratio=%f", w_Ratio, h_ratio);
    HAL_LOGV("face_num=%d,camMetadata=%p", face_num, camMetadata);
    if (face_num) {
        for (int i = 0; i < face_num; i++) {
            faceRectangles[i * 4 + 0] =
                camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[i * 4 + 0];
            faceRectangles[i * 4 + 1] =
                camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[i * 4 + 1];
            faceRectangles[i * 4 + 2] =
                camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[i * 4 + 2];
            faceRectangles[i * 4 + 3] =
                camMetadata->find(ANDROID_STATISTICS_FACE_RECTANGLES)
                    .data.i32[i * 4 + 3];
            if (mZoomValue < mZoomValueTh) {
                faceRectangles[i * 4 + 0] = faceRectangles[i * 4 + 0] * w_Ratio;
                faceRectangles[i * 4 + 1] = faceRectangles[i * 4 + 1] * h_ratio;
                faceRectangles[i * 4 + 2] = faceRectangles[i * 4 + 2] * w_Ratio;
                faceRectangles[i * 4 + 3] = faceRectangles[i * 4 + 3] * h_ratio;
            }
            HAL_LOGV("the %d st face, faceRectangles     =  %d, %d, %d, %d ", i,
                     faceRectangles[i * 4], faceRectangles[i * 4 + 1],
                     faceRectangles[i * 4 + 2], faceRectangles[i * 4 + 3]);
        }
        camMetadata->update(ANDROID_STATISTICS_FACE_RECTANGLES, faceRectangles,
                            face_num * 4);
    }
    camMetadata->update(ANDROID_CONTROL_AF_STATE,
                        &(SprdCamera3Setting::s_setting[m_VirtualCamera.id]
                              .controlInfo.af_state),
                        1);
    camMetadata->update(ANDROID_CONTROL_AF_MODE,
                        &(SprdCamera3Setting::s_setting[m_VirtualCamera.id]
                              .controlInfo.af_mode),
                        1);
    camMetadata->update(ANDROID_CONTROL_AF_TRIGGER_ID,
                        &(SprdCamera3Setting::s_setting[m_VirtualCamera.id]
                              .controlInfo.af_trigger_Id),
                        1);
    meta = camMetadata->release();
    delete camMetadata;
    return meta;
}
/*===========================================================================
 * FUNCTION         : setZoomRatio
 *
 * DESCRIPTION     : set zoom/crop meta info
 *
 * RETURN             :  zoom ratio
 *==========================================================================*/
float SprdCamera3OpticsZoomV1::setZoomInfo(CameraMetadata *WideSettings,
                                           CameraMetadata *TeleSettings) {
    float zoomWidth, zoomHeight, zoomRatio = 1.0f;
    float prevAspectRatio, capAspectRatio, videoAspectRatio;
    float sensorAspectRatio, outputAspectRatio;
    struct img_rect cropRegion;
    int32_t crop_Region[4] = {0, 0, 0, 0};
    int ret = 0;
    float zoomThreshold = mZoomValueTh;
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    double adbzoom = 5;

    property_get("sys.camera.opticalzoom", value, "0");
    adbzoom = atof(value);
    if (adbzoom >= 1 && adbzoom <= 4) {
        if (adbzoom != zoomRatio) {
            zoomRatio = adbzoom;
            HAL_LOGV("reset zoom to %f %f", adbzoom, zoomRatio);
        }
    } else if (WideSettings->exists(ANDROID_SCALER_CROP_REGION)) {
        cropRegion.start_x =
            WideSettings->find(ANDROID_SCALER_CROP_REGION).data.i32[0];
        cropRegion.start_y =
            WideSettings->find(ANDROID_SCALER_CROP_REGION).data.i32[1];
        cropRegion.width =
            WideSettings->find(ANDROID_SCALER_CROP_REGION).data.i32[2];
        cropRegion.height =
            WideSettings->find(ANDROID_SCALER_CROP_REGION).data.i32[3];

        HAL_LOGV("crop start_x=%d start_y=%d width=%d height=%d",
                 cropRegion.start_x, cropRegion.start_y, cropRegion.width,
                 cropRegion.height);

        sensorAspectRatio = static_cast<float>(mTeleMaxWidth) / mTeleMaxHeight;

        if (cropRegion.width > 0 && cropRegion.height > 0) {
            zoomWidth = static_cast<float>(cropRegion.width);
            zoomHeight = static_cast<float>(cropRegion.height);
        } else {
            zoomWidth = static_cast<float>(mTeleMaxWidth);
            zoomHeight = static_cast<float>(mTeleMaxHeight);
        }

        if (mMainSize.preview_w != 0 && mMainSize.preview_h != 0) {
            prevAspectRatio =
                static_cast<float>(mMainSize.preview_w) / mMainSize.preview_h;
        } else {
            prevAspectRatio = sensorAspectRatio;
        }

        if (mMainSize.preview_w != 0 && mMainSize.preview_h != 0) {
            if (prevAspectRatio >= sensorAspectRatio) {
                zoomRatio = static_cast<float>(mWideMaxWidth) / zoomWidth;
            } else {
                zoomRatio = static_cast<float>(mWideMaxHeight) / zoomHeight;
            }
        }

        if (zoomRatio < MIN_DIGITAL_ZOOM_RATIO)
            zoomRatio = MIN_DIGITAL_ZOOM_RATIO;
        if (zoomRatio > MAX_DIGITAL_ZOOM_RATIO_OPTICS)
            zoomRatio = MAX_DIGITAL_ZOOM_RATIO_OPTICS;

    } else {
        return -1;
    }

    float w_ratio =
        static_cast<float>(mWideMaxWidth) / static_cast<float>(mTeleMaxWidth);
    float h_ratio =
        static_cast<float>(mWideMaxHeight) / static_cast<float>(mTeleMaxHeight);

    HAL_LOGD(" zoomRatio=%f,w_ratio=%f,h_ratio=%f", zoomRatio, w_ratio,
             h_ratio);
    if (zoomRatio >= zoomThreshold) { // tele
        float inputRatio = zoomRatio / zoomThreshold;
        inputRatio = inputRatio > 4 ? 4 : inputRatio;
        crop_Region[2] = static_cast<int32_t>(
            static_cast<float>(mTeleMaxWidth) / (inputRatio));
        crop_Region[3] = static_cast<int32_t>(
            static_cast<float>(mTeleMaxHeight) / (inputRatio));
        crop_Region[0] = (mTeleMaxWidth - crop_Region[2]) >> 1;
        crop_Region[1] = (mTeleMaxHeight - crop_Region[3]) >> 1;

        TeleSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));

        HAL_LOGD(" crop=%d,%d,%d,%d", crop_Region[0], crop_Region[1],
                 crop_Region[2], crop_Region[3]);
        crop_Region[0] = 0;
        crop_Region[1] = 0;
        crop_Region[2] = mWideMaxWidth;
        crop_Region[3] = mWideMaxHeight;

        WideSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));

        HAL_LOGV(" crop=%d,%d,%d,%d", crop_Region[0], crop_Region[1],
                 crop_Region[2], crop_Region[3]);
    } else if (zoomRatio < zoomThreshold) { // wide
        crop_Region[0] = 0;
        crop_Region[1] = 0;
        crop_Region[2] = mTeleMaxWidth;
        crop_Region[3] = mTeleMaxHeight;

        HAL_LOGV(" crop=%d,%d,%d,%d", crop_Region[0], crop_Region[1],
                 crop_Region[2], crop_Region[3]);
        TeleSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));
        crop_Region[0] = 0;
        crop_Region[1] = 0;
        crop_Region[2] = mWideMaxWidth;
        crop_Region[3] = mWideMaxHeight;
        WideSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));
        HAL_LOGD(" crop=%d,%d,%d,%d", crop_Region[0], crop_Region[1],
                 crop_Region[2], crop_Region[3]);
    }
    return zoomRatio;
}
/*===========================================================================
 * FUNCTION         : coordinateTra
 *
 * DESCRIPTION     : convert coordinate for af/ae crop
 *
 * RETURN             :  zoom ratio
 *==========================================================================*/

void SprdCamera3OpticsZoomV1::coordinateTra(int inputWidth, int inputHeight,
                                            int outputWidth, int outputHeight,
                                            float inputRatio, float outputRatio,
                                            int *area) {
    int i;
    float inputHalfWidth = (float)inputWidth / 2;
    float inputHalfHeight = (float)inputHeight / 2;
    float outputHalfWidth = (float)outputWidth / 2;
    float outputHalfHeight = (float)outputHeight / 2;
    float WidthRatio = (float)outputWidth / inputWidth;
    float HeightRatio = (float)outputHeight / inputHeight;
    HAL_LOGV(
        " input_W= %d,input_H=%d,Oput_W=%d,Oput_H=%d,W_Ratio=%f,H_Ratio=%f",
        inputWidth, inputHeight, outputWidth, outputHeight, WidthRatio,
        HeightRatio);
    HAL_LOGV(" area =%ld,%ld,%ld,%ld", area[0], area[1], area[2], area[3]);
    if ((area[0] + area[1] + area[2] + area[3]) != 0) {
        for (i = 0; i < 4; i++) {
            if (i % 2 == 0) {
                if (area[i] <= inputHalfWidth) {
                    area[i] = inputHalfWidth -
                              (inputHalfWidth - area[i]) * inputRatio;
                } else {
                    area[i] = inputHalfWidth +
                              (area[i] - inputHalfWidth) * inputRatio;
                }
            }
            if (i % 2 != 0) {
                if (area[i] <= inputHalfHeight) {
                    area[i] = inputHalfHeight -
                              (inputHalfHeight - area[i]) * inputRatio;
                } else {
                    area[i] = inputHalfHeight +
                              (area[i] - inputHalfHeight) * inputRatio;
                }
            }
        }
        HAL_LOGV(" Zoom 1x area =%ld,%ld,%ld,%ld", area[0], area[1], area[2],
                 area[3]);
        if (mZoomValue >= mZoomValueTh) {
            for (i = 0; i < 4; i++) {
                if (i % 2 == 0) {
                    area[i] = area[i] * WidthRatio;
                }
                if (i % 2 != 0) {
                    area[i] = area[i] * HeightRatio;
                }
            }
            HAL_LOGV("Magnifying size area =%ld,%ld,%ld,%ld", area[0], area[1],
                     area[2], area[3]);
            for (i = 0; i < 4; i++) {
                if (i % 2 == 0) {
                    if (area[i] <= outputHalfWidth) {
                        area[i] = outputHalfWidth -
                                  (outputHalfWidth - area[i]) / outputRatio;
                    } else {
                        area[i] = outputHalfWidth +
                                  (area[i] - outputHalfWidth) / outputRatio;
                    }
                }
                if (i % 2 != 0) {
                    if (area[i] <= outputHalfHeight) {
                        area[i] = outputHalfHeight -
                                  (outputHalfHeight - area[i]) / outputRatio;
                    } else {
                        area[i] = outputHalfHeight +
                                  (area[i] - outputHalfHeight) / outputRatio;
                    }
                }
            }
        }
    }
    HAL_LOGV(" all area equal zero  =%ld,%ld,%ld,%ld", area[0], area[1],
             area[2], area[3]);
}
/*===========================================================================
 * FUNCTION         : reReqConfig
 *
 * DESCRIPTION     : reconfigure request
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::reReqConfig(camera3_capture_request_t *request,
                                          CameraMetadata *meta) {
    CameraMetadata metadata;
    // meta config
    if (!meta) {
        return;
    }
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    int tagCnt1 = meta[CAM_TYPE_MAIN].entryCount();
    if (tagCnt1 != 0) {
        if (meta[CAM_TYPE_MAIN].exists(ANDROID_SPRD_BURSTMODE_ENABLED)) {
            uint8_t sprdBurstModeEnabled = 0;
            meta[CAM_TYPE_MAIN].update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                       &sprdBurstModeEnabled, 1);
            meta[CAM_TYPE_AUX1].update(ANDROID_SPRD_BURSTMODE_ENABLED,
                                       &sprdBurstModeEnabled, 1);
        }

        uint8_t sprdZslEnabled = 1;
        meta[CAM_TYPE_MAIN].update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                   1);
        meta[CAM_TYPE_AUX1].update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled,
                                   1);
    }
    camera3_stream_t *preview_stream = NULL;
    int tagCnt = 0;
    float zoomValue = 0;
    mIsSyncFirstFrame = true;
    CameraMetadata *metaSettingsWide = &meta[0];
    CameraMetadata *metaSettingsTele = &meta[1];
    float w_ratio =
        static_cast<float>(mWideMaxWidth) / static_cast<float>(mTeleMaxWidth);
    float h_ratio =
        static_cast<float>(mWideMaxHeight) / static_cast<float>(mTeleMaxHeight);
    property_get("sys.camera.opticalzoomth", value, "-1");
    if (atof(value) != -1) {
        mZoomValueTh = atof(value);
    }

    tagCnt = metaSettingsWide->entryCount();
    if (tagCnt != 0) {
        zoomValue = setZoomInfo(metaSettingsWide, metaSettingsTele);
        if (zoomValue == -1) {
        } else if (zoomValue < mZoomValueTh && mAlgoStatus == DO_ALGO) {
            mZoomValue = zoomValue;
            mReqConfigNum = 0;
        } else {
            mReqConfigNum = 1;
            mZoomValue = zoomValue;
        }
        if (metaSettingsWide->exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
            int32_t aeTargetFpsRange[2] = {20, 20};
            metaSettingsWide->update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                     aeTargetFpsRange,
                                     ARRAY_SIZE(aeTargetFpsRange));
            metaSettingsTele->update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE,
                                     aeTargetFpsRange,
                                     ARRAY_SIZE(aeTargetFpsRange));
        }
        if (metaSettingsWide->exists(ANDROID_CONTROL_AF_REGIONS)) {
            int32_t af_w_area[5] = {0};
            int32_t af_t_area[5] = {0};
            size_t i = 0;

            if (metaSettingsWide->find(ANDROID_CONTROL_AF_REGIONS).count == 5) {
                for (i = 0; i < 5; i++) {
                    af_w_area[i] =
                        metaSettingsWide->find(ANDROID_CONTROL_AF_REGIONS)
                            .data.i32[i];
                    af_t_area[i] =
                        metaSettingsWide->find(ANDROID_CONTROL_AF_REGIONS)
                            .data.i32[i];
                }
            }
            if (mZoomValue >= mZoomValueTh) {
                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mWideMaxWidth,
                              mWideMaxHeight, mZoomValue, 1, af_w_area);
                metaSettingsWide->update(ANDROID_CONTROL_AF_REGIONS, af_w_area,
                                         ARRAY_SIZE(af_w_area));
                float inputRatio = mZoomValue / mZoomValueTh;
                inputRatio = inputRatio > 4 ? 4 : inputRatio;

                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mTeleMaxWidth,
                              mTeleMaxHeight, mZoomValue, inputRatio,
                              af_t_area);
                metaSettingsTele->update(ANDROID_CONTROL_AF_REGIONS, af_t_area,
                                         ARRAY_SIZE(af_t_area));

            } else {
                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mWideMaxWidth,
                              mWideMaxHeight, mZoomValue, mZoomValue,
                              af_w_area);
                metaSettingsWide->update(ANDROID_CONTROL_AF_REGIONS, af_w_area,
                                         ARRAY_SIZE(af_w_area));

                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mTeleMaxWidth,
                              mTeleMaxHeight, mZoomValue, 1, af_t_area);
                metaSettingsTele->update(ANDROID_CONTROL_AF_REGIONS, af_t_area,
                                         ARRAY_SIZE(af_t_area));
            }
        }

        if (metaSettingsWide->exists(ANDROID_CONTROL_AE_REGIONS)) {
            int32_t ae_w_area[5] = {0};
            int32_t ae_t_area[5] = {0};
            size_t i = 0;
            if (metaSettingsWide->find(ANDROID_CONTROL_AE_REGIONS).count == 5) {
                for (i = 0; i < 5; i++) {
                    ae_w_area[i] =
                        metaSettingsWide->find(ANDROID_CONTROL_AE_REGIONS)
                            .data.i32[i];
                    ae_t_area[i] =
                        metaSettingsWide->find(ANDROID_CONTROL_AE_REGIONS)
                            .data.i32[i];
                }
            }
            if (mZoomValue >= mZoomValueTh) {
                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mWideMaxWidth,
                              mWideMaxHeight, mZoomValue, 1, ae_w_area);
                metaSettingsWide->update(ANDROID_CONTROL_AE_REGIONS, ae_w_area,
                                         ARRAY_SIZE(ae_w_area));
                float inputRatio = mZoomValue / mZoomValueTh;
                inputRatio = inputRatio > 4 ? 4 : inputRatio;
                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mTeleMaxWidth,
                              mTeleMaxHeight, mZoomValue, inputRatio,
                              ae_t_area);
                metaSettingsTele->update(ANDROID_CONTROL_AE_REGIONS, ae_t_area,
                                         ARRAY_SIZE(ae_t_area));

            } else {
                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mWideMaxWidth,
                              mWideMaxHeight, mZoomValue, mZoomValue,
                              ae_w_area);
                metaSettingsWide->update(ANDROID_CONTROL_AE_REGIONS, ae_w_area,
                                         ARRAY_SIZE(ae_w_area));

                coordinateTra(mTeleMaxWidth, mTeleMaxHeight, mTeleMaxWidth,
                              mTeleMaxHeight, mZoomValue, 1, ae_t_area);
                metaSettingsTele->update(ANDROID_CONTROL_AE_REGIONS, ae_t_area,
                                         ARRAY_SIZE(ae_t_area));
            }
        }
    }

    if (mZoomValue < mZoomValueTh) {
        mReqConfigNum = 0;
    } else {
        mReqConfigNum = 1;
    }
    HAL_LOGD("switchRequest = %d ,exit frame_number %u", mReqConfigNum,
             request->frame_number);
}
/*===========================================================================
 * FUNCTION   :reConfigInit
 *
 * DESCRIPTION: set init
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::reConfigInit() {
    mIsCapturing = true;
    mCapInputbuffer = NULL;
    mPrevAlgoHandle = NULL;
    mCapAlgoHandle = NULL;
    mOtpData.otp_size = 0;
    mOtpData.otp_type = 0;
    mOtpData.otp_exist = false;
    memset(&mTWCaptureThread->mSavedOneResultBuff, 0,
           sizeof(camera3_stream_buffer_t));
}
/*===========================================================================
 * FUNCTION   :reConfigFlush
 *
 * DESCRIPTION: set flush
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::reConfigFlush() {
    HAL_LOGD("E");

    TWThreadExit();

    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux1.clear();

    gZoomV1->deinitAlgo();
    HAL_LOGD("X");
}

void SprdCamera3OpticsZoomV1::TWThreadExit() {
    HAL_LOGI("E");

    if (mPreviewMuxerThread != NULL) {
        if (mPreviewMuxerThread->isRunning()) {
            mPreviewMuxerThread->requestExit();
        }
    }
    if (mTWCaptureThread != NULL) {
        if (mTWCaptureThread->isRunning()) {
            mTWCaptureThread->requestExit();
        }
    }
    // wait threads quit to relese object
    if (mTWCaptureThread != NULL)
        mTWCaptureThread->join();
    if (mPreviewMuxerThread != NULL)
        mPreviewMuxerThread->join();

    HAL_LOGI("X");
}

void SprdCamera3OpticsZoomV1::reConfigStream() {
    int rc = NO_ERROR;
#define READ_OTP_SIZE 700
    FILE *fid = fopen("/data/vendor/cameraserver/wt_otp.bin", "rb");
    if (fid != NULL) {
        HAL_LOGD("open depth_config_parameter.bin file success");
        rc = fread(mOtpData.otp_data, sizeof(uint8_t), READ_OTP_SIZE, fid);
        fclose(fid);
        mOtpData.otp_size = rc;
        mOtpData.otp_exist = true;
        HAL_LOGD("dualotp otp_size=%d", mOtpData.otp_size);
    }
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    property_get("persist.vendor.cam.dump.calibration.data", prop, "0");
    if (atoi(prop) == 1) {
        for (int i = 0; i < mOtpData.otp_size; i++)
            HAL_LOGD("calibraion data [%d] = %d", i, mOtpData.otp_data[i]);
    }
    gZoomV1->mPreviewMuxerThread->run(String8::format("TW_Preview").string());
    gZoomV1->mTWCaptureThread->run(String8::format("TW_Capture").string());
    initAlgo();
}

void SprdCamera3OpticsZoomV1::reConfigClose() {
    HAL_LOGD("E");
    if (!mFlushing) {
        mFlushing = true;
        TWThreadExit();
        mUnmatchedFrameListMain.clear();
        mUnmatchedFrameListAux1.clear();
        gZoomV1->deinitAlgo();
    }
    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :TWPreviewMuxerThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3OpticsZoomV1::TWPreviewMuxerThread::TWPreviewMuxerThread() {
    HAL_LOGI("E");
    mPreviewMuxerMsgList.clear();
    // mMsgList.clear();
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~TWPreviewMuxerThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3OpticsZoomV1::TWPreviewMuxerThread::~TWPreviewMuxerThread() {
    HAL_LOGI(" E");
    mPreviewMuxerMsgList.clear();

    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3OpticsZoomV1::TWPreviewMuxerThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    void *output_buf_addr = NULL;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    muxer_queue_msg_t muxer_msg;
    uint32_t frame_number = 0;
    int rc;
    while (!mPreviewMuxerMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mPreviewMuxerMsgList.begin();
            muxer_msg = *it;
            mPreviewMuxerMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT: {
            List<multi_request_saved_t>::iterator itor =
                gZoomV1->mSavedPrevRequestList.begin();
            HAL_LOGD("exit frame_number %u", itor->frame_number);
            while (itor != gZoomV1->mSavedPrevRequestList.end()) {
                frame_number = itor->frame_number;
                itor++;
                gZoomV1->CallBackResult(frame_number,
                                        CAMERA3_BUFFER_STATUS_ERROR,
                                        CALLBACK_STREAM, CAM_TYPE_MAIN);
            }
            return false;
        } break;
        case MUXER_MSG_DATA_PROC: {
            int IsNeedreProcess;
            camera3_stream_t *preview_stream = NULL;
            {
                Mutex::Autolock l(gZoomV1->mRequestLock);
                List<multi_request_saved_t>::iterator itor =
                    gZoomV1->mSavedPrevRequestList.begin();
                while (itor != gZoomV1->mSavedPrevRequestList.end()) {
                    if (itor->frame_number ==
                        muxer_msg.combo_frame.frame_number) {
                        output_buffer = itor->buffer;
                        preview_stream = itor->preview_stream;
                        frame_number = muxer_msg.combo_frame.frame_number;
                        break;
                    }
                    itor++;
                }
            }

            if (output_buffer != NULL) {

                {
                    if (gZoomV1->mZoomValue < gZoomV1->mZoomValueTh) {
                        rc = gZoomV1->runAlgo(gZoomV1->mPrevAlgoHandle,
                                              muxer_msg.combo_frame.buffer1,
                                              muxer_msg.combo_frame.buffer2,
                                              output_buffer);
                        if (rc) {
                            HAL_LOGE("fail to run Algo");
                        }
                    } else {
                        rc = gZoomV1->map(output_buffer, &output_buf_addr);
                        rc = gZoomV1->map(muxer_msg.combo_frame.buffer1,
                                          &input_buf1_addr);
                        rc = gZoomV1->map(muxer_msg.combo_frame.buffer2,
                                          &input_buf2_addr);
                        memcpy(output_buf_addr, input_buf2_addr,
                               ADP_BUFSIZE(*output_buffer));
                        gZoomV1->unmap(muxer_msg.combo_frame.buffer1);
                        gZoomV1->unmap(muxer_msg.combo_frame.buffer2);
                        gZoomV1->unmap(output_buffer);
                    }
                    // dump prev data
                    {
                        char prop[PROPERTY_VALUE_MAX] = {
                            0,
                        };
                        property_get("persist.vendor.cam.twv1.dump", prop, "0");
                        if (!strcmp(prop, "preyuv") || !strcmp(prop, "all")) {
                            rc = gZoomV1->map(output_buffer, &output_buf_addr);
                            rc = gZoomV1->map(muxer_msg.combo_frame.buffer1,
                                              &input_buf1_addr);
                            rc = gZoomV1->map(muxer_msg.combo_frame.buffer2,
                                              &input_buf2_addr);
                            char tmp_str[64] = {0};
                            sprintf(tmp_str, "_zoom=%f", gZoomV1->mZoomValue);
                            char MainPrev[80] = "MainPrev";
                            char SubPrev[80] = "SubPrev";
                            char TWPrev[80] = "TWPrev";
                            strcat(MainPrev, tmp_str);
                            strcat(SubPrev, tmp_str);
                            strcat(TWPrev, tmp_str);
                            // input_buf1 or left image
                            gZoomV1->dumpData((unsigned char *)input_buf1_addr,
                                              1, ADP_BUFSIZE(*output_buffer),
                                              gZoomV1->mWideMaxWidth,
                                              gZoomV1->mWideMaxHeight,
                                              frame_number, MainPrev);
                            // input_buf2 or right image
                            gZoomV1->dumpData((unsigned char *)input_buf2_addr,
                                              1, ADP_BUFSIZE(*output_buffer),
                                              gZoomV1->mTeleMaxWidth,
                                              gZoomV1->mTeleMaxHeight,
                                              frame_number, SubPrev);
                            // output_buffer
                            gZoomV1->dumpData((unsigned char *)output_buf_addr,
                                              1, ADP_BUFSIZE(*output_buffer),
                                              preview_stream->width,
                                              preview_stream->height,
                                              frame_number, TWPrev);
                            gZoomV1->unmap(muxer_msg.combo_frame.buffer1);
                            gZoomV1->unmap(muxer_msg.combo_frame.buffer2);
                            gZoomV1->unmap(output_buffer);
                        }
                    }
                    gZoomV1->pushBufferList(
                        gZoomV1->mLocalBuffer, muxer_msg.combo_frame.buffer1,
                        gZoomV1->mLocalBufferNumber, gZoomV1->mLocalBufferList);
                    gZoomV1->pushBufferList(
                        gZoomV1->mLocalBuffer, muxer_msg.combo_frame.buffer2,
                        gZoomV1->mLocalBufferNumber, gZoomV1->mLocalBufferList);
                    gZoomV1->CallBackResult(muxer_msg.combo_frame.frame_number,
                                            CAMERA3_BUFFER_STATUS_OK,
                                            CALLBACK_STREAM, CAM_TYPE_MAIN);
                }
            }
        } break;
        default:
            break;
        }
    };

    waitMsgAvailable();

    return true;
}
/*===========================================================================
 * FUNCTION   :requestExit
 *
 * DESCRIPTION: request thread exit
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::TWPreviewMuxerThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mPreviewMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}
/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::TWPreviewMuxerThread::waitMsgAvailable() {
    while (mPreviewMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}
/*===========================================================================
 * FUNCTION   :TWCaptureThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3OpticsZoomV1::TWCaptureThread::TWCaptureThread() {
    HAL_LOGI("E");
    mSavedOneResultBuff = NULL;

    mCaptureMsgList.clear();
    // mMsgList.clear();
    memset(&mSavedOneResultBuff, 0, sizeof(camera3_stream_buffer_t));
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~~TWCaptureThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3OpticsZoomV1::TWCaptureThread::~TWCaptureThread() {
    HAL_LOGI(" E");
    mCaptureMsgList.clear();

    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :threadLoop
 *
 * DESCRIPTION: threadLoop
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3OpticsZoomV1::TWCaptureThread::threadLoop() {
    buffer_handle_t *output_buffer = NULL;
    void *output_buf_addr = NULL;
    void *input_buf1_addr = NULL;
    void *input_buf2_addr = NULL;
    uint32_t frame_number = 0;
    muxer_queue_msg_t capture_msg;
    int rc = 0;
    while (!mCaptureMsgList.empty()) {
        List<muxer_queue_msg_t>::iterator itor1;
        {
            Mutex::Autolock l(mMergequeueMutex);
            itor1 = mCaptureMsgList.begin();
            capture_msg = *itor1;
            mCaptureMsgList.erase(itor1);
        }
        switch (capture_msg.msg_type) {
        case MUXER_MSG_EXIT: {
            // flush queue
            HAL_LOGI("TW_MSG_EXIT,mCapFrameNum=%lld", gZoomV1->mCapFrameNum);
            if ((gZoomV1->mCapFrameNum) > 0) {
                HAL_LOGE("TW_MSG_EXIT.HAL don't send capture frame");
                gZoomV1->CallBackResult(gZoomV1->mCapFrameNum,
                                        CAMERA3_BUFFER_STATUS_ERROR,
                                        SNAPSHOT_STREAM, CAM_TYPE_MAIN);
            }
            memset(&(gZoomV1->mSavedSnapRequest), 0,
                   sizeof(multi_request_saved_t));
            memset(&mSavedOneResultBuff, 0, sizeof(camera3_stream_buffer_t));
            return false;
        }
        case MUXER_MSG_DATA_PROC: {

            if (mSavedOneResultBuff) {
                memset(&mSavedOneResultBuff, 0,
                       sizeof(camera3_stream_buffer_t));
            }
            output_buffer = (gZoomV1->popBufferList(
                gZoomV1->mLocalBufferList,
                gZoomV1->mSavedSnapRequest.snap_stream->width,
                gZoomV1->mSavedSnapRequest.snap_stream->height));
            if (gZoomV1->mZoomValue < gZoomV1->mZoomValueTh) {
                rc = gZoomV1->runAlgo(
                    gZoomV1->mCapAlgoHandle, capture_msg.combo_frame.buffer1,
                    capture_msg.combo_frame.buffer2, output_buffer);
            } else {
                rc = gZoomV1->map(capture_msg.combo_frame.buffer1,
                                  &input_buf1_addr);
                rc = gZoomV1->map(capture_msg.combo_frame.buffer2,
                                  &input_buf2_addr);
                rc = gZoomV1->map(output_buffer, &output_buf_addr);
                memcpy(output_buf_addr, input_buf2_addr,
                       ADP_BUFSIZE(*output_buffer));
                gZoomV1->unmap(capture_msg.combo_frame.buffer1);
                gZoomV1->unmap(capture_msg.combo_frame.buffer2);
                gZoomV1->unmap(output_buffer);
            }
            // dump capture data
            {
                char prop[PROPERTY_VALUE_MAX] = {
                    0,
                };

                property_get("persist.vendor.cam.twv1.dump", prop, "0");
                if (!strcmp(prop, "capyuv") || !strcmp(prop, "all")) {
                    rc = gZoomV1->map(capture_msg.combo_frame.buffer1,
                                      &input_buf1_addr);
                    rc = gZoomV1->map(capture_msg.combo_frame.buffer2,
                                      &input_buf2_addr);
                    rc = gZoomV1->map(output_buffer, &output_buf_addr);
                    // input_buf1 or left image
                    char tmp_str[64] = {0};
                    sprintf(tmp_str, "_zoom=%f", gZoomV1->mZoomValue);
                    char MainCap[80] = "MainCap";
                    char SubCap[80] = "SubCap";
                    char TWCapture[80] = "TWCapture";
                    strcat(MainCap, tmp_str);
                    strcat(SubCap, tmp_str);
                    strcat(TWCapture, tmp_str);
                    gZoomV1->dumpData(
                        (unsigned char *)input_buf1_addr, 1,
                        ADP_BUFSIZE(*output_buffer), gZoomV1->mWideMaxWidth,
                        gZoomV1->mWideMaxHeight, frame_number, MainCap);
                    // input_buf2 or right image
                    gZoomV1->dumpData(
                        (unsigned char *)input_buf2_addr, 1,
                        ADP_BUFSIZE(*output_buffer), gZoomV1->mTeleMaxWidth,
                        gZoomV1->mTeleMaxHeight, frame_number, SubCap);
                    // output_buffer
                    gZoomV1->dumpData(
                        (unsigned char *)output_buf_addr, 1,
                        ADP_BUFSIZE(*output_buffer),
                        gZoomV1->mSavedSnapRequest.snap_stream->width,
                        gZoomV1->mSavedSnapRequest.snap_stream->height,
                        frame_number, TWCapture);
                    gZoomV1->unmap(capture_msg.combo_frame.buffer1);
                    gZoomV1->unmap(capture_msg.combo_frame.buffer2);
                    gZoomV1->unmap(output_buffer);
                }
            }

            gZoomV1->pushBufferList(
                gZoomV1->mLocalBuffer, capture_msg.combo_frame.buffer1,
                gZoomV1->mLocalBufferNumber, gZoomV1->mLocalBufferList);
            gZoomV1->pushBufferList(
                gZoomV1->mLocalBuffer, capture_msg.combo_frame.buffer2,
                gZoomV1->mLocalBufferNumber, gZoomV1->mLocalBufferList);
            gZoomV1->mCapInputbuffer = output_buffer;

            gZoomV1->reprocessReq(output_buffer);
            break;
        }
        default:
            break;
        };
        waitMsgAvailable();
    }
    return true;
}
void SprdCamera3OpticsZoomV1::TWCaptureThread::requestExit() {

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t capture_msg;
    capture_msg.msg_type = MUXER_MSG_EXIT;
    mCaptureMsgList.push_back(capture_msg);
    mMergequeueSignal.signal();
}

/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::TWCaptureThread::waitMsgAvailable() {
    while (mCaptureMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }
}
/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process frame of camera index 0
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::processCaptureResultMain(
    const camera3_capture_result_t *result) {

    uint64_t result_timestamp = 0;
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    buffer_handle_t *output_buffer = NULL;

    int vcmSteps = 0;
    int rc = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    HAL_LOGD("cur_frame_number:  %d", cur_frame_number);
    if (result_buffer == NULL) {
        // meta process
        metadata = result->result;
        if (metadata.exists(ANDROID_SPRD_VCM_STEP) & cur_frame_number) {
            vcmSteps = metadata.find(ANDROID_SPRD_VCM_STEP).data.i32[0];
            setAlgoTrigger(vcmSteps);
        }
        if (cur_frame_number == mCapFrameNum && cur_frame_number != 0) {
            if (gZoomV1->mRequstState == REPROCESS_STATE) {
                HAL_LOGD("hold jpeg picture call bac1k, framenumber:%d",
                         result->frame_number);
            } else {
                {

                    Mutex::Autolock(gZoomV1->mMetatLock);
                    metadata_t.frame_number = cur_frame_number;
                    metadata_t.metadata = clone_camera_metadata(result->result);
                    mMetadataList.push_back(metadata_t);
                }
                CallBackMetadata();
            }
            return;
        } else {
            HAL_LOGD("send  meta, framenumber:%d", cur_frame_number);
            metadata_t.frame_number = cur_frame_number;
            metadata_t.metadata = clone_camera_metadata(result->result);
            Mutex::Autolock l(gZoomV1->mMetatLock);
            mMetadataList.push_back(metadata_t);
            return;
        }
    }

    int currStreamType = getStreamType(result_buffer->stream);
    /* Process error buffer for Main camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR ||
        mFlushing) {
        HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                 result->frame_number);
        if (currStreamType == CALLBACK_STREAM) {
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mCurFrameNum, mLocalBufferList);
        }

        CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                       currStreamType, CAM_TYPE_MAIN);

        return;
    }

    if (currStreamType == DEFAULT_STREAM) {
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {

            CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_ERROR,
                           DEFAULT_STREAM, CAM_TYPE_MAIN);
            return;
        }

        Mutex::Autolock l(mDefaultStreamLock);
        if (NULL == mTWCaptureThread->mSavedOneResultBuff) {
            mTWCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {

            muxer_queue_msg_t capture_msg;
            capture_msg.msg_type = MUXER_MSG_DATA_PROC;
            capture_msg.combo_frame.frame_number = result->frame_number;
            capture_msg.combo_frame.buffer1 = result->output_buffers->buffer;
            capture_msg.combo_frame.buffer2 =
                mTWCaptureThread->mSavedOneResultBuff;
            capture_msg.combo_frame.input_buffer = result->input_buffer;
            {
                //    hwiMain->setMultiCallBackYuvMode(false);
                //   hwiAux->setMultiCallBackYuvMode(false);
                Mutex::Autolock l(mTWCaptureThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!",
                         capture_msg.combo_frame.frame_number);

                mTWCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mTWCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (currStreamType == SNAPSHOT_STREAM) {

        if (mCapInputbuffer) {
            gZoomV1->pushBufferList(gZoomV1->mLocalBuffer, mCapInputbuffer,
                                    gZoomV1->mLocalBufferNumber,
                                    gZoomV1->mLocalBufferList);
            mCapInputbuffer = NULL;
        }
        CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                       currStreamType, CAM_TYPE_MAIN);
    } else if (currStreamType == CALLBACK_STREAM) {
        {
            Mutex::Autolock l(mNotifyLockMain);
            for (List<camera3_notify_msg_t>::iterator i =
                     mNotifyListMain.begin();
                 i != mNotifyListMain.end(); i++) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        mNotifyListMain.erase(i);

                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        CallBackResult(cur_frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR,
                                       currStreamType, CAM_TYPE_MAIN);
                        mNotifyListMain.erase(i);
                        return;
                    }
                }
            }
        }
        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }

        hwi_frame_buffer_info_t matched_frame;
        hwi_frame_buffer_info_t cur_frame;

        memset(&matched_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {
            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListAux1,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = cur_frame.buffer;

                muxer_msg.combo_frame.buffer2 = matched_frame.buffer;

                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(cur_frame.frame_number,
                                               matched_frame.frame_number);

                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("Enqueue newest unmatched frame:%d for Main camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
                if (discard_frame != NULL) {
                    HAL_LOGV("discard frame_number %u", cur_frame_number);
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    if (cur_frame.frame_number > 5)
                        CallBackResult(discard_frame->frame_number,
                                       CAMERA3_BUFFER_STATUS_ERROR,
                                       currStreamType, CAM_TYPE_MAIN);
                    delete discard_frame;
                }
            }
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :processCaptureResultAux
 *
 * DESCRIPTION: process frame of camera index 1
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::processCaptureResultAux1(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    CameraMetadata metadata;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    metadata = result->result;
    uint64_t result_timestamp = 0;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    camera3_stream_t *newStream = NULL;
    HAL_LOGD("aux frame_number %d", cur_frame_number);
    if (result->output_buffers == NULL) {
        return;
    }

    if (mFlushing) {
        pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                       mLocalBufferNumber, mLocalBufferList);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGD("aux buffer frame_number %d,currStreamType=%d", cur_frame_number,
             currStreamType);

    /* Process error buffer for Main camera*/
    if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("Return local buffer:%d caused by error Buffer status",
                 result->frame_number);
        if (currStreamType == CALLBACK_STREAM) {
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
        }
        CallBackResult(cur_frame_number, CAMERA3_BUFFER_STATUS_OK,
                       currStreamType, CAM_TYPE_AUX1);

        return;
    }

    if (currStreamType == DEFAULT_STREAM) {

        Mutex::Autolock l(mDefaultStreamLock);
        result_buffer = result->output_buffers;
        if (NULL == mTWCaptureThread->mSavedOneResultBuff) {
            mTWCaptureThread->mSavedOneResultBuff =
                result->output_buffers->buffer;
        } else {
            muxer_queue_msg_t capture_msg;

            capture_msg.msg_type = MUXER_MSG_DATA_PROC;
            capture_msg.combo_frame.frame_number = result->frame_number;
            capture_msg.combo_frame.buffer1 =
                mTWCaptureThread->mSavedOneResultBuff;
            capture_msg.combo_frame.buffer2 = result->output_buffers->buffer;
            capture_msg.combo_frame.input_buffer = result->input_buffer;
            HAL_LOGV("capture combined begin: framenumber %d",
                     capture_msg.combo_frame.frame_number);
            {
                //    hwiMain->setMultiCallBackYuvMode(false);
                //   hwiAux->setMultiCallBackYuvMode(false);
                Mutex::Autolock l(mTWCaptureThread->mMergequeueMutex);
                HAL_LOGV("Enqueue combo frame:%d for frame merge!",
                         capture_msg.combo_frame.frame_number);
                mTWCaptureThread->mCaptureMsgList.push_back(capture_msg);
                mTWCaptureThread->mMergequeueSignal.signal();
            }
        }
    } else if (currStreamType == SNAPSHOT_STREAM) {
        HAL_LOGD("should not entry here, shutter frame:%d",
                 result->frame_number);
    } else if (currStreamType == CALLBACK_STREAM) {
        // process preview buffer
        {

            Mutex::Autolock l(mNotifyLockAux);
            for (List<camera3_notify_msg_t>::iterator i =
                     gZoomV1->mNotifyListAux1.begin();
                 i != gZoomV1->mNotifyListAux1.end(); i++) {
                if (i->message.shutter.frame_number == cur_frame_number) {
                    if (i->type == CAMERA3_MSG_SHUTTER) {
                        searchnotifyresult = NOTIFY_SUCCESS;
                        result_timestamp = i->message.shutter.timestamp;
                        gZoomV1->mNotifyListAux1.erase(i);
                    } else if (i->type == CAMERA3_MSG_ERROR) {
                        HAL_LOGE("Return local buffer:%d caused by error "
                                 "Notify status",
                                 result->frame_number);
                        searchnotifyresult = NOTIFY_ERROR;
                        pushBufferList(mLocalBuffer,
                                       result->output_buffers->buffer,
                                       mLocalBufferNumber, mLocalBufferList);
                        gZoomV1->mNotifyListAux1.erase(i);
                        return;
                    }
                }
            }
        }
        if (searchnotifyresult == NOTIFY_NOT_FOUND) {
            HAL_LOGE("found no corresponding notify");
            return;
        }
        /* Process error buffer for Aux camera: just return local buffer*/
        if (result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR) {
            HAL_LOGV("Return local buffer:%d caused by error Buffer status",
                     result->frame_number);
            pushBufferList(mLocalBuffer, result->output_buffers->buffer,
                           mLocalBufferNumber, mLocalBufferList);
            return;
        }
        hwi_frame_buffer_info_t matched_frame;
        hwi_frame_buffer_info_t cur_frame;

        memset(&matched_frame, 0, sizeof(hwi_frame_buffer_info_t));
        memset(&cur_frame, 0, sizeof(hwi_frame_buffer_info_t));
        cur_frame.frame_number = cur_frame_number;
        cur_frame.timestamp = result_timestamp;
        cur_frame.buffer = (result->output_buffers)->buffer;
        {

            Mutex::Autolock l(mUnmatchedQueueLock);
            if (MATCH_SUCCESS == matchTwoFrame(cur_frame,
                                               mUnmatchedFrameListMain,
                                               &matched_frame)) {
                muxer_queue_msg_t muxer_msg;
                muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
                muxer_msg.combo_frame.frame_number = matched_frame.frame_number;
                muxer_msg.combo_frame.buffer1 = matched_frame.buffer;
                muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
                muxer_msg.combo_frame.input_buffer = result->input_buffer;
                {
                    Mutex::Autolock l(mPreviewMuxerThread->mMergequeueMutex);
                    HAL_LOGV("Enqueue combo frame:%d for frame merge!",
                             muxer_msg.combo_frame.frame_number);
                    // we don't call clearFrameNeverMatched before five
                    // frame.
                    // for first frame meta and ok status buffer update at
                    // the
                    // same time.
                    // app need the point that the first meta updated to
                    // hide
                    // image cover
                    if (cur_frame.frame_number > 5)
                        clearFrameNeverMatched(matched_frame.frame_number,
                                               cur_frame.frame_number);
                    mPreviewMuxerThread->mPreviewMuxerMsgList.push_back(
                        muxer_msg);
                    mPreviewMuxerThread->mMergequeueSignal.signal();
                }
            } else {
                HAL_LOGV("Enqueue newest unmatched frame:%d for Aux camera",
                         cur_frame.frame_number);
                hwi_frame_buffer_info_t *discard_frame =
                    pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux1);
                if (discard_frame != NULL) {
                    pushBufferList(mLocalBuffer, discard_frame->buffer,
                                   mLocalBufferNumber, mLocalBufferList);
                    delete discard_frame;
                }
            }
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :setAlgoTrigger
 *
 * DESCRIPTION: set algo statues
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::setAlgoTrigger(int vcm) {

    if (mVcmSteps == vcm && mVcmSteps) {

        mAlgoStatus = DO_ALGO;
    } else {
        mAlgoStatus = NO_ALGO;
    }
    HAL_LOGV("mAlgoStatus %d,trigger=%d,vcm=%d", mAlgoStatus, mVcmSteps, vcm);

    mVcmSteps = vcm;
}
/*===========================================================================
 * FUNCTION   :clearFrameNeverMatched
 *
 * DESCRIPTION: clear earlier frame which will never be matched any more
 *
 * PARAMETERS : which camera queue to be clear
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3OpticsZoomV1::clearFrameNeverMatched(
    uint32_t main_frame_number, uint32_t sub_frame_number) {
    List<hwi_frame_buffer_info_t>::iterator itor;
    uint32_t frame_num = 0;
    Mutex::Autolock l(mClearBufferLock);

    itor = mUnmatchedFrameListMain.begin();
    while (itor != mUnmatchedFrameListMain.end()) {
        if (itor->frame_number < main_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame main idx:%d", itor->frame_number);
            frame_num = itor->frame_number;
            mUnmatchedFrameListMain.erase(itor);
            CallBackResult(frame_num, CAMERA3_BUFFER_STATUS_ERROR,
                           CALLBACK_STREAM, CAM_TYPE_MAIN);
        }
        itor++;
    }

    itor = mUnmatchedFrameListAux1.begin();
    while (itor != mUnmatchedFrameListAux1.end()) {
        if (itor->frame_number < sub_frame_number) {
            pushBufferList(mLocalBuffer, itor->buffer, mLocalBufferNumber,
                           mLocalBufferList);
            HAL_LOGD("clear frame aux idx:%d", itor->frame_number);
            mUnmatchedFrameListAux1.erase(itor);
        }
        itor++;
    }
}
void SprdCamera3OpticsZoomV1::initAlgo() {
    int ret;
    WT_inparam algo_info;
    algo_info.yuv_mode = 1;
    algo_info.is_preview = 1;
    algo_info.image_width_wide = mMainSize.preview_w;
    algo_info.image_height_wide = mMainSize.preview_h;
    algo_info.image_width_tele = mMainSize.preview_w;
    algo_info.image_height_tele = mMainSize.preview_h;
    algo_info.otpbuf = &mOtpData.otp_data;
    algo_info.otpsize = mOtpData.otp_size;
    algo_info.VCMup = 0;
    algo_info.VCMdown = 1000;
    HAL_LOGD("otpType %d, otpSize %d, otpbuf =%p ", mOtpData.otp_type,
             mOtpData.otp_size, &mOtpData.otp_data);
    HAL_LOGD(
        "yuv_mode=%d,is_preview=%d,image_height_tele = %d,image_width_tele "
        "=%d,image_height_wide =%d,image_width_wide =%d,VCMup=%d,",
        algo_info.yuv_mode, algo_info.is_preview, algo_info.image_height_tele,
        algo_info.image_width_tele, algo_info.image_height_wide,
        algo_info.image_width_wide, algo_info.VCMup);
    ret = WTprocess_Init(&mPrevAlgoHandle, &algo_info);
    if (ret) {
        HAL_LOGE("WT Algo Prev Init failed");
    }
    algo_info.is_preview = 0;
    algo_info.image_width_wide = mMainSize.capture_w;
    algo_info.image_height_wide = mMainSize.capture_h;
    algo_info.image_width_tele = mMainSize.capture_w;
    algo_info.image_height_tele = mMainSize.capture_h;
    ret = WTprocess_Init(&mCapAlgoHandle, &algo_info);
    if (ret) {
        HAL_LOGE("WT Algo Cap Init failed");
    }
}
int SprdCamera3OpticsZoomV1::runAlgo(void *handle,
                                     buffer_handle_t *input_image_wide,
                                     buffer_handle_t *input_image_tele,
                                     buffer_handle_t *output_image) {
    int ret;
    WT_inparam *algoInfo = (WT_inparam *)handle;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;
    int32_t yuvTextFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    uint32_t inWidth = 0, inHeight = 0, inStride = 0;
#if defined(CONFIG_SPRD_ANDROID_8)
    uint32_t inLayCount = 1;
#endif
    HAL_LOGD("algoInfo->is_preview =%d", algoInfo->is_preview);
    if (algoInfo->is_preview) {
        inWidth = mMainSize.preview_w;
        inHeight = mMainSize.preview_h;
        inStride = mMainSize.preview_w;
    } else if (algoInfo->is_preview == 0) {
        inWidth = mMainSize.capture_w;
        inHeight = mMainSize.capture_h;
        inStride = mMainSize.capture_w;
    }
    HAL_LOGD(
        "yuvTextUsage "
        "=%d,yuvTextFormat=%d,inWidth=%d,inHeight=%d,inStride=%d,inLayCount=%d",
        yuvTextUsage, yuvTextFormat, inWidth, inHeight, inStride, inLayCount);
#if defined(CONFIG_SPRD_ANDROID_8)
    sp<GraphicBuffer> inputWideBuffer = new GraphicBuffer(
        (native_handle_t *)(*input_image_wide),
        GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
        yuvTextFormat, inLayCount, yuvTextUsage, inStride);
    sp<GraphicBuffer> inputTeleBuffer = new GraphicBuffer(
        (native_handle_t *)(*input_image_tele),
        GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
        yuvTextFormat, inLayCount, yuvTextUsage, inStride);
    sp<GraphicBuffer> outputBuffer = new GraphicBuffer(
        (native_handle_t *)(*output_image),
        GraphicBuffer::HandleWrapMethod::CLONE_HANDLE, inWidth, inHeight,
        yuvTextFormat, inLayCount, yuvTextUsage, inStride);
#else

    sp<GraphicBuffer> inputWideBuffer =
        new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                          inStride, (native_handle_t *)(*input_image_wide), 0);
    sp<GraphicBuffer> inputTeleBuffer =
        new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                          inStride, (native_handle_t *)(*input_image_tele), 0);
    sp<GraphicBuffer> outputBuffer =
        new GraphicBuffer(inWidth, inHeight, yuvTextFormat, yuvTextUsage,
                          inStride, (native_handle_t *)(*output_image), 0);
#endif

    ret = WTprocess_function(handle, &(*inputWideBuffer), &(*inputTeleBuffer),
                             &(*outputBuffer), mVcmSteps, mZoomValue);

    if (ret) {
        HAL_LOGE("WT Algo Run failed");
    }
    return ret;
}
void SprdCamera3OpticsZoomV1::deinitAlgo() {
    int ret;
    HAL_LOGV("E");
    if (mPrevAlgoHandle) {
        ret = WTprocess_deinit(mPrevAlgoHandle);
        if (ret) {
            HAL_LOGE("WT Algo Prev deinit failed");
        }
        mPrevAlgoHandle = NULL;
    }
    if (mCapAlgoHandle) {
        ret = WTprocess_deinit(mCapAlgoHandle);
        if (ret) {
            HAL_LOGE("WT Algo Cap deinit failed");
        }
        mCapAlgoHandle = NULL;
    }
    HAL_LOGV("X");
}
}
