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
#define LOG_TAG "Cam3Wrapper"
#include "SprdCamera3Wrapper.h"

using namespace android;

namespace sprdcamera {

typedef struct {
    cmr_u8 camera_id;
    multiCameraMode camera_mode;
} muti_camera_mode_map_t;
const multiCameraMode available_mutiCamera_mode[MODE_CAMERA_MAX] = {
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    MODE_3D_VIDEO,
#endif

#ifdef CONFIG_BOKEH_SUPPORT
    MODE_BOKEH,
#endif

#ifdef CONFIG_PORTRAIT_SUPPORT
    MODE_PORTRAIT,
#endif

#ifdef CONFIG_OPTICSZOOM_SUPPORT
    MODE_SOFY_OPTICAL_ZOOM,
#endif

#ifdef CONFIG_3DFACE_SUPPORT
    MODE_3D_FACE,
#endif

#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
    MODE_PORTRAIT_SCENE
#endif
};
const muti_camera_mode_map_t cameraid_map_mode[MODE_CAMERA_MAX] = {
    {SPRD_3D_VIDEO_ID, MODE_3D_VIDEO},
    {SPRD_RANGE_FINDER_ID, MODE_RANGE_FINDER},
    {SPRD_3D_CAPTURE_ID, MODE_3D_CAPTURE},
    {SPRD_BLUR_ID, MODE_BOKEH},
    {SPRD_BLUR_FRONT_ID, MODE_BLUR},
    {SPRD_SELF_SHOT_ID, MODE_SELF_SHOT},
    {SPRD_PAGE_TURN_ID, MODE_PAGE_TURN},
    {SPRD_PAGE_TURN_ID, MODE_PAGE_TURN},
    {SPRD_SINGLE_FACEID_REGISTER_ID, MODE_SINGLE_FACEID_REGISTER},
    {SPRD_DUAL_FACEID_REGISTER_ID, MODE_DUAL_FACEID_REGISTER},
    {SPRD_SINGLE_FACEID_UNLOCK_ID, MODE_SINGLE_FACEID_UNLOCK},
    {SPRD_DUAL_FACEID_UNLOCK_ID, MODE_DUAL_FACEID_UNLOCK},
    {SPRD_SOFY_OPTICAL_ZOOM_ID, MODE_SOFY_OPTICAL_ZOOM},
    {SPRD_3D_FACE_ID, MODE_3D_FACE},
    {SPRD_MULTI_CAMERA_ID, MODE_MULTI_CAMERA},
    {SPRD_PORTRAIT_ID, MODE_PORTRAIT},
    {SPRD_PORTRAIT_SINGLE_ID, MODE_PORTRAIT_SINGLE},
    {SPRD_3D_FACEID_REGISTER_ID, MODE_3D_FACEID_REGISTER},
    {SPRD_3D_FACEID_UNLOCK_ID, MODE_3D_FACEID_UNLOCK},
    {SPRD_FOV_FUSION_ID, MODE_FOV_FUSION},
    {SPRD_PORTRAIT_SCENE_FRONT_ID, MODE_PORTRAIT_SCENE},
    {SPRD_PORTRAIT_SCENE_REAR_ID, MODE_PORTRAIT_SCENE}
    };

int SprdCamera3Wrapper::mLogicalSensorNum = CAMERA_LOGICAL_SENSOR_NUM;
int SprdCamera3Wrapper::mPhysicalSensorNum = CAMERA_SENSOR_NUM;
SprdCamera3Wrapper::SprdCamera3Wrapper() {
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    SprdCamera3StereoVideo::getCameraMuxer(&mStereoVideo);
#endif
#ifdef CONFIG_STEREOPREVIEW_SUPPORT
    SprdCamera3StereoPreview::getCameraMuxer(&mStereoPreview);
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    SprdCamera3Capture::getCameraCapture(&mCapture);
#endif
#ifdef CONFIG_BLUR_SUPPORT
    SprdCamera3Blur::getCameraBlur(&mBlur);
#endif
#ifdef CONFIG_BOKEH_SUPPORT
    SprdCamera3RealBokeh::getCameraBokeh(&mRealBokeh);
#endif
#ifdef CONFIG_COVERED_SENSOR
    SprdCamera3SelfShot::getCameraMuxer(&mSelfShot);
    SprdCamera3PageTurn::getCameraMuxer(&mPageturn);
#endif
    SprdDualCamera3Tuning::getTCamera(&mTCam);
#ifdef CONFIG_SINGLE_FACEID_SUPPORT
    SprdCamera3SingleFaceIdRegister::getCameraFaceId(&mSingleFaceIdRegister);
    SprdCamera3SingleFaceIdUnlock::getCameraFaceId(&mSingleFaceIdUnlock);
#endif

#ifdef CONFIG_DUAL_FACEID_SUPPORT
    SprdCamera3DualFaceId::getCameraFaceId(&mDualFaceId);
#endif

#ifdef CONFIG_3D_FACEID_SUPPORT
    SprdCamera33DFaceId::getCameraFaceId(&m3DFaceId);
#endif

#ifdef CONFIG_OPTICSZOOM_SUPPORT
    SprdCamera3OpticsZoomV1::getCamera3dZoomV1(&mZoomV1);
// SprdCamera3OpticsZoom::getCamera3dZoom(&mZoom);
#endif
#ifdef CONFIG_3DFACE_SUPPORT
    SprdCamera33dFace::getCamera3dFace(&m3dFace);
#endif
#ifdef CONFIG_PORTRAIT_SUPPORT
    SprdCamera3Portrait::getCameraPortrait(&mPortrait);
#endif
#ifdef CONFIG_PORTRAIT_SINGLE_SUPPORT
    SprdCamera3SinglePortrait::getCameraBlur(&mSinglePortrait);
#endif
#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
    SprdCamera3PortraitScene::getCameraPortraitScene(&mPortraitScene);
#endif
}

SprdCamera3Wrapper::~SprdCamera3Wrapper() {}

void SprdCamera3Wrapper::getCameraWrapper(SprdCamera3Wrapper **pFinder) {
    *pFinder = NULL;
    *pFinder = new SprdCamera3Wrapper();
    HAL_LOGV("gWrapper: %p ", *pFinder);
    return;
}

multiCameraMode SprdCamera3Wrapper::getMultiCameraMode(int camera_id) {
    multiCameraMode mode = MODE_SINGLE_CAMERA;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    int i = 0;
    HAL_LOGI("cameraId:%d", camera_id);
    if (camera_id > SPRD_MULTI_CAMERA_BASE_ID) {

        for (i = 0; i < MODE_CAMERA_MAX; i++) {
            if (cameraid_map_mode[i].camera_id == camera_id) {
                mode = cameraid_map_mode[i].camera_mode;
                break;
            }
        }
        if (i == MODE_CAMERA_MAX)
            HAL_LOGE("cameraId:%d not supported yet!", camera_id);
    } else if (camera_id >= CAMERA_SENSOR_NUM) {
        int logicid_shit = camera_id - CAMERA_SENSOR_NUM;
        mode = available_mutiCamera_mode[logicid_shit];
    } else {
        mode = MODE_SINGLE_CAMERA;
    }

    if (mode == MODE_BOKEH) {
        property_get("persist.vendor.cam.raw.mode", prop, "jpeg");
        if (!strcmp(prop, "raw")) {
            mode = MODE_TUNING;
        } else {
            property_get("persist.vendor.cam.ba.blur.version", prop, "0");
            if (6 == atoi(prop)) {
                mode = MODE_BOKEH;
            } else if (3 == atoi(prop) || 1 == atoi(prop)) {
                mode = MODE_PORTRAIT_SINGLE;
            }
        }
    }

    if (mode == MODE_PORTRAIT) {
        property_get("persist.vendor.cam.raw.mode", prop, "jpeg");
        if (!strcmp(prop, "raw")) {
            mode = MODE_TUNING;
        } else {
            property_get("persist.vendor.cam.ba.blur.version", prop, "0");
            if (3 == atoi(prop) || 1 == atoi(prop)) {
                mode = MODE_PORTRAIT_SINGLE;
            }
        }
    }

    HAL_LOGI("mode:%d", mode);
    return mode;
}
int SprdCamera3Wrapper::cameraDeviceOpen(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    switch (getMultiCameraMode(atoi(id))) {
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    case MODE_3D_VIDEO:
        rc = mStereoVideo->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_RANGEFINDER_SUPPORT
    case MODE_RANGE_FINDER:
        rc = mRangeFinder->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    case MODE_3D_CAPTURE:
        rc = mCapture->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_STEREOPREVIEW_SUPPORT
    case MODE_3D_PREVIEW:
        rc = mStereoPreview->camera_device_open(module, id, hw_device);
        break;
#endif
    case MODE_TUNING:

        rc = mTCam->camera_device_open(module, id, hw_device);
        break;
    case MODE_BOKEH:
#ifdef CONFIG_BOKEH_SUPPORT
        rc = mRealBokeh->camera_device_open(module, id, hw_device);
#endif
        break;
    case MODE_BLUR:
#ifdef CONFIG_BLUR_SUPPORT
        rc = mBlur->camera_device_open(module, id, hw_device);
#endif
        break;
#ifdef CONFIG_COVERED_SENSOR
    case MODE_SELF_SHOT:
        rc = mSelfShot->camera_device_open(module, id, hw_device);
        break;
    case MODE_PAGE_TURN:
        rc = mPageturn->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_SINGLE_FACEID_SUPPORT
    case MODE_SINGLE_FACEID_REGISTER:
    case MODE_DUAL_FACEID_REGISTER:
        rc = mSingleFaceIdRegister->camera_device_open(module, id, hw_device);
        break;
    case MODE_SINGLE_FACEID_UNLOCK:
        rc = mSingleFaceIdUnlock->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_3D_FACEID_SUPPORT
    case MODE_3D_FACEID_REGISTER:
    case MODE_3D_FACEID_UNLOCK:
        rc = m3DFaceId->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_DUAL_FACEID_SUPPORT
    case MODE_DUAL_FACEID_UNLOCK:
        rc = mDualFaceId->camera_device_open(module, id, hw_device);
        break;
#endif
#ifdef CONFIG_OPTICSZOOM_SUPPORT
    case MODE_SOFY_OPTICAL_ZOOM:
        rc = mZoomV1->camera_device_open(module, id, hw_device, mZoomV1);
        // rc = mZoom->camera_device_open(module, id, hw_device, mZoom);
        break;
#endif
#ifdef CONFIG_3DFACE_SUPPORT
    case MODE_3D_FACE:
        rc = m3dFace->camera_device_open(module, id, hw_device, m3dFace);
        break;
#endif
#ifdef CONFIG_PORTRAIT_SUPPORT
    case MODE_PORTRAIT:
        rc = mPortrait->camera_device_open(module, id, hw_device);
        break;
#endif

#ifdef CONFIG_PORTRAIT_SINGLE_SUPPORT
    case MODE_PORTRAIT_SINGLE:
        rc = mSinglePortrait->camera_device_open(module, id, hw_device);
        break;
#endif

#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
    case MODE_PORTRAIT_SCENE:
        rc = mPortraitScene->camera_device_open(module, id, hw_device);
        break;
#endif

    default:
        HAL_LOGE("cameraId:%d not supported yet!", atoi(id));
        return -EINVAL;
    }

    return rc;
}

int SprdCamera3Wrapper::getCameraInfo(__unused int camera_id,
                                      struct camera_info *info) {
    int rc = NO_ERROR;
    HAL_LOGI("id= %d", camera_id);
    switch (getMultiCameraMode(camera_id)) {
#ifdef CONFIG_STEREOVIDEO_SUPPORT
    case MODE_3D_VIDEO:
        rc = mStereoVideo->get_camera_info(camera_id, info);
        break;
#endif
#ifdef CONFIG_RANGEFINDER_SUPPORT
    case MODE_RANGE_FINDER:
        rc = mRangeFinder->get_camera_info(camera_id, info);
        break;
#endif
#ifdef CONFIG_STEREOCAPUTRE_SUPPORT
    case MODE_3D_CAPTURE:
        rc = mCapture->get_camera_info(camera_id, info);
        break;
#endif
#ifdef CONFIG_STEREOPREVIEW_SUPPORT
    case MODE_3D_PREVIEW:
        rc = mStereoPreview->get_camera_info(camera_id, info);
        break;
#endif
    case MODE_TUNING:
        rc = mTCam->get_camera_info(camera_id, info);
        break;
#ifdef CONFIG_BOKEH_SUPPORT
    case MODE_BOKEH:
        rc = mRealBokeh->get_camera_info(camera_id, info);
#endif
        break;

#ifdef CONFIG_BLUR_SUPPORT
    case MODE_BLUR:
        rc = mBlur->get_camera_info(camera_id, info);
        break;
#endif

#ifdef CONFIG_COVERED_SENSOR
    case MODE_SELF_SHOT:
        rc = mSelfShot->get_camera_info(camera_id, info);
        break;
    case MODE_PAGE_TURN:
        rc = mPageturn->get_camera_info(camera_id, info);
        break;
#endif
#ifdef CONFIG_SINGLE_FACEID_SUPPORT
    case MODE_SINGLE_FACEID_REGISTER:
    case MODE_DUAL_FACEID_REGISTER:
        rc = mSingleFaceIdRegister->get_camera_info(camera_id, info);
        break;
    case MODE_SINGLE_FACEID_UNLOCK:
        rc = mSingleFaceIdUnlock->get_camera_info(camera_id, info);
        break;
#endif

#ifdef CONFIG_DUAL_FACEID_SUPPORT
    case MODE_DUAL_FACEID_UNLOCK:
        rc = mDualFaceId->get_camera_info(camera_id, info);
        break;
#endif
#ifdef CONFIG_3D_FACEID_SUPPORT
    case MODE_3D_FACEID_REGISTER:
    case MODE_3D_FACEID_UNLOCK:
        rc = m3DFaceId->get_camera_info(camera_id, info);
        break;
#endif
#ifdef CONFIG_OPTICSZOOM_SUPPORT
    case MODE_SOFY_OPTICAL_ZOOM:
        rc = mZoomV1->get_camera_info(camera_id, info, mZoomV1);
        // rc = mZoom->get_camera_info(camera_id, info, mZoom);
        break;
#endif
#ifdef CONFIG_3DFACE_SUPPORT
    case MODE_3D_FACE:
        rc = m3dFace->get_camera_info(camera_id, info, m3dFace);
        break;
#endif
#ifdef CONFIG_PORTRAIT_SUPPORT
    case MODE_PORTRAIT:
        rc = mPortrait->get_camera_info(camera_id, info);
        break;
#endif

#ifdef CONFIG_PORTRAIT_SINGLE_SUPPORT
    case MODE_PORTRAIT_SINGLE:
        rc = mSinglePortrait->get_camera_info(camera_id, info);
        break;
#endif

#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
    case MODE_PORTRAIT_SCENE:
        rc = mPortraitScene->get_camera_info(camera_id, info);
        break;
#endif

    default:
        HAL_LOGE("cameraId:%d not supported yet!", camera_id);
        return -EINVAL;
    }
    return rc;
}
};
