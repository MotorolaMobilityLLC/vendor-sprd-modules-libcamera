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
#define LOG_TAG "Cam33dZoom"
//#define LOG_NDEBUG 0
#include "SprdCamera3OpticsZoom.h"

using namespace android;
namespace sprdcamera {

SprdCamera3OpticsZoom *gZoom = NULL;

#define SWITCH_WIDE_TELE_REQUEST (1.7)
#define MAX_DIGITAL_ZOOM_RATIO_OPTICS 8
// Error Check Macros
#define CHECK_MUXER()                                                          \
    if (!gZoom) {                                                              \
        HAL_LOGE("Error getting muxer ");                                      \
        return;                                                                \
    }

/*===========================================================================
 * FUNCTION         : SprdCamera3OpticsZoom
 *
 * DESCRIPTION     : SprdCamera3OpticsZoom Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3OpticsZoom::SprdCamera3OpticsZoom() {
    HAL_LOGI(" E");
    mTeleMaxWidth = 0;
    mTeleMaxHeight = 0;
    mWideMaxWidth = 0;
    mWideMaxHeight = 0;
    mZoomValue = 1.0f;
    mZoomValueTh = SWITCH_WIDE_TELE_REQUEST;
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3OpticsZoom
 *
 * DESCRIPTION     : SprdCamera3OpticsZoom Desctructor
 *
 *==========================================================================*/
SprdCamera3OpticsZoom::~SprdCamera3OpticsZoom() {
    HAL_LOGI("E");

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION         : getCamera3dZoom
 *
 * DESCRIPTION     : Creates Camera Muxer if not created
 *
 * PARAMETERS:
 *   @pMuxer               : Pointer to retrieve Camera Muxer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3OpticsZoom::getCamera3dZoom(SprdCamera3Multi **pMuxer) {
    *pMuxer = NULL;
    if (!gZoom) {
        gZoom = new SprdCamera3OpticsZoom();
    }
    CHECK_MUXER();
    *pMuxer = gZoom;
    HAL_LOGD("gZoom: %p ", gZoom);

    return;
}

static config_multi_camera optics_zoom_config = {
#include "optics_zoom_config.h"
};
/*===========================================================================
 * FUNCTION         : load_config_file
 *
 * DESCRIPTION     : load zoom configure file
 *
 * RETURN             :  NONE
 *==========================================================================*/
config_multi_camera *SprdCamera3OpticsZoom::load_config_file(void) {

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
void SprdCamera3OpticsZoom::reConfigGetCameraInfo(CameraMetadata &metadata) {

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

    metadata.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                    SprdCamera3Setting::s_setting[sub_camera_id]
                        .sensor_InfoInfo.active_array_size,
                    ARRAY_SIZE(SprdCamera3Setting::s_setting[sub_camera_id]
                                   .sensor_InfoInfo.active_array_size));

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
SprdCamera3OpticsZoom::reConfigResultMeta(camera_metadata_t *meta) {

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
float SprdCamera3OpticsZoom::setZoomInfo(CameraMetadata *WideSettings,
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

    property_get("vendor.cam.opticalzoom", value, "0");
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
                zoomRatio = static_cast<float>(mTeleMaxWidth) / zoomWidth;
            } else {
                zoomRatio = static_cast<float>(mTeleMaxHeight) / zoomHeight;
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

    HAL_LOGV(" zoomRatio=%f,w_ratio=%f,h_ratio=%f", zoomRatio, w_ratio,
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

        HAL_LOGV(" crop=%d,%d,%d,%d", crop_Region[0], crop_Region[1],
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
        crop_Region[2] = mWideMaxWidth / zoomRatio;
        crop_Region[3] = mWideMaxHeight / zoomRatio;
        crop_Region[0] = (mWideMaxWidth - crop_Region[2]) >> 1;
        crop_Region[1] = (mWideMaxHeight - crop_Region[3]) >> 1;
        WideSettings->update(ANDROID_SCALER_CROP_REGION, crop_Region,
                             ARRAY_SIZE(crop_Region));

        HAL_LOGV(" crop=%d,%d,%d,%d", crop_Region[0], crop_Region[1],
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

void SprdCamera3OpticsZoom::coordinateTra(int inputWidth, int inputHeight,
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
void SprdCamera3OpticsZoom::reReqConfig(camera3_capture_request_t *request,
                                        CameraMetadata *meta) {
    // meta config
    if (!meta) {
        return;
    }
    char value[PROPERTY_VALUE_MAX] = {
        0,
    };
    int tagCnt = 0;
    float zoomValue = 0;
    mIsSyncFirstFrame = true;
    CameraMetadata *metaSettingsWide = &meta[0];
    CameraMetadata *metaSettingsTele = &meta[1];
    float w_ratio =
        static_cast<float>(mWideMaxWidth) / static_cast<float>(mTeleMaxWidth);
    float h_ratio =
        static_cast<float>(mWideMaxHeight) / static_cast<float>(mTeleMaxHeight);
    property_get("vendor.cam.opticalzoomth", value, "-1");
    if (atof(value) != -1) {
        mZoomValueTh = atof(value);
    }

    tagCnt = metaSettingsWide->entryCount();
    if (tagCnt != 0) {
        zoomValue = setZoomInfo(metaSettingsWide, metaSettingsTele);
        if (zoomValue == -1) {
        } else if (zoomValue < mZoomValueTh) {
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
    HAL_LOGV("switchRequest = %d", mReqConfigNum);
}
}
