/* Copyright (c) 2012-2016, The Linux Foundataion. All rights reserved.
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

#define LOG_TAG "Cam3Factory"
//#define LOG_NDEBUG 0

#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <hardware/camera3.h>

#include "SprdCamera3Factory.h"
#include "SprdCamera3Flash.h"
#include "hal_common/multiCamera/SprdCamera3Wrapper.h"

#ifdef CONFIG_MULTICAMERA_SUPPORT
#include "SprdCamera3MultiCamera.h"
#endif

using namespace android;

namespace sprdcamera {

SprdCamera3Factory gSprdCamera3Factory;
SprdCamera3Wrapper *gSprdCamera3Wrapper = NULL;
/*===========================================================================
 * FUNCTION   : SprdCamera3Factory
 *
 * DESCRIPTION: default constructor of SprdCamera3Factory
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Factory::SprdCamera3Factory() {
    char boot_mode[PROPERTY_VALUE_MAX] = {'\0'};

    property_get("ro.bootmode", boot_mode, "0");
    LOGI("%s boot_mode = %s", __FUNCTION__, boot_mode);

    if (strcmp(boot_mode, "autotest") && strcmp(boot_mode, "cali")) {
        mNumOfCameras = SprdCamera3Setting::getNumberOfCameras();
    }
    if (!gSprdCamera3Wrapper) {
        SprdCamera3Wrapper::getCameraWrapper(&gSprdCamera3Wrapper);
        if (!gSprdCamera3Wrapper) {
            LOGE("Error !! Failed to get SprdCamera3Wrapper");
        }
    }

    mStaticMetadata = NULL;
}

/*===========================================================================
 * FUNCTION   : ~SprdCamera3Factory
 *
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Factory::~SprdCamera3Factory() {
    if (gSprdCamera3Wrapper) {
        delete gSprdCamera3Wrapper;
        gSprdCamera3Wrapper = NULL;
    }
}

/*===========================================================================
 * FUNCTION   : get_number_of_cameras
 *
 * DESCRIPTION: static function to query number of cameras detected
 *
 * PARAMETERS : none
 *
 * RETURN     : number of cameras detected
 *==========================================================================*/
int SprdCamera3Factory::get_number_of_cameras() {
    return gSprdCamera3Factory.getNumberOfCameras();
}

/*===========================================================================
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: static function to query camera information with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Factory::get_camera_info(int camera_id,
                                        struct camera_info *info) {

#ifdef CONFIG_MULTICAMERA_SUPPORT
    if (camera_id == SPRD_MULTI_CAMERA_ID || camera_id == SPRD_FOV_FUSION_ID) {
        if (NULL == sensorGetLogicaInfo4MulitCameraId(SPRD_MULTI_CAMERA_ID)) {
            camera_id = 0;
        }
    }
#endif

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.id", value, "null");
    if (value[0] >= '0' && value[0] <= '9') {
        camera_id = value[0] - '0';
    }

#ifdef CONFIG_MULTICAMERA_SUPPORT
    if (camera_id == SPRD_MULTI_CAMERA_ID || camera_id == SPRD_FOV_FUSION_ID)
        return SprdCamera3MultiCamera::get_camera_info(camera_id, info);
#endif
    /* high resolution id */
    if (camera_id == SPRD_BACK_HIGH_RESOLUTION_ID ||
        camera_id == SPRD_FRONT_HIGH_RES)
        return gSprdCamera3Factory.getHighResolutionSize(
            multi_id_to_phyid(camera_id), info);

    if (is_single_expose(camera_id))
        return gSprdCamera3Wrapper->getCameraInfo(camera_id, info);
    else
        return gSprdCamera3Factory.getCameraInfo(multi_id_to_phyid(camera_id),
                                                 info);
}

/*===========================================================================
 * FUNCTION   : getNumberOfCameras
 *
 * DESCRIPTION: query number of cameras detected
 *
 * PARAMETERS : none
 *
 * RETURN     : number of cameras detected
 *==========================================================================*/
int SprdCamera3Factory::getNumberOfCameras() {
    if (mNumOfCameras == 0) {
        mNumOfCameras = SprdCamera3Setting::getNumberOfCameras();
    }
    return mNumOfCameras;
}

/*===========================================================================
 * FUNCTION   : getCameraInfo
 *
 * DESCRIPTION: query camera information with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Factory::getCameraInfo(int camera_id, struct camera_info *info) {
    int rc;
    Mutex::Autolock l(mLock);

    HAL_LOGI("E, camera_id = %d", camera_id);

    if (!mNumOfCameras || camera_id >= mNumOfCameras || !info ||
        (camera_id < 0)) {
        return -ENODEV;
    }

    SprdCamera3Setting::getSensorStaticInfo(camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;

    HAL_LOGV("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : getHighResolutionSize
 *
 * DESCRIPTION: query camera information with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Factory::getHighResolutionSize(int camera_id,
                                              struct camera_info *info) {
    int rc;
    Mutex::Autolock l(mLock);

    HAL_LOGI("E, camera_id = %d", camera_id);

    if (!mNumOfCameras || camera_id >= mNumOfCameras || !info ||
        (camera_id < 0)) {
        return -ENODEV;
    }

    SprdCamera3Setting::getSensorStaticInfo(camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    CameraMetadata metadata = clone_camera_metadata(mStaticMetadata);

    const struct img_size *stream_info;
    int32_t scaler_formats[] = {
        HAL_PIXEL_FORMAT_YCbCr_420_888, HAL_PIXEL_FORMAT_BLOB,
        HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, HAL_PIXEL_FORMAT_RAW16};
    size_t scaler_formats_count = sizeof(scaler_formats) / sizeof(int32_t);
    int array_size = 0;
    int32_t avail_stream_config[CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4];

    memset(avail_stream_config, 0, CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4);

    SprdCamera3Setting::getHighResCapSize(camera_id, &stream_info);

    for (size_t j = 0; j < scaler_formats_count; j++) {
        for (size_t i = 0; stream_info; i++) {
            /* last img_size must be {0, 0} */
            if ((stream_info[i].width == 0) || (stream_info[i].height == 0))
                break;
            avail_stream_config[array_size++] = scaler_formats[j];
            avail_stream_config[array_size++] = stream_info[i].width;
            avail_stream_config[array_size++] = stream_info[i].height;
            avail_stream_config[array_size++] =
                ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT;
        }
    }

    metadata.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS,
                    avail_stream_config, array_size);

    mStaticMetadata = metadata.release();
    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = mStaticMetadata;

    HAL_LOGV("X");
    return rc;
}

/*====================================================================
*FUNCTION     :set_torch_mode
*DESCRIPTION  :Attempt to turn on or off the torch of the flash unint.
*
*PARAMETERS   :
*      @camera_id : camera ID
*      @enabled   : Indicate whether to turn the flash on or off
*
*RETURN       : 0         --success
*               non zero  --failure
*===================================================================*/
int SprdCamera3Factory::set_torch_mode(const char *camera_id, bool enabled) {
    int retval = 0;

    retval = SprdCamera3Flash::setTorchMode(camera_id, enabled);
    HAL_LOGV("camera_id %d,enabled %d ret %d", camera_id, enabled, retval);

    return retval;
}

int SprdCamera3Factory::set_callbacks(
    const camera_module_callbacks_t *callbacks) {
    int retval = 0;

    HAL_LOGV("E");
    retval = SprdCamera3Flash::registerCallbacks(callbacks);

    return retval;
}

void SprdCamera3Factory::get_vendor_tag_ops(vendor_tag_ops_t *ops) {
    HAL_LOGV("E,ops=%p", ops);
    ops->get_tag_count = SprdCamera3Setting::get_tag_count;
    ops->get_all_tags = SprdCamera3Setting::get_all_tags;
    ops->get_section_name = SprdCamera3Setting::get_section_name;
    ops->get_tag_name = SprdCamera3Setting::get_tag_name;
    ops->get_tag_type = SprdCamera3Setting::get_tag_type;
}
/*===========================================================================
 * FUNCTION   : cameraDeviceOpen
 *
 * DESCRIPTION: open a camera device with its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Factory::cameraDeviceOpen(int camera_id,
                                         struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    int phyId = 0;
    int is_high_res_mode = 0;
    unsigned int on_off = 0;
    struct phySensorInfo *phyPtr = NULL;
    char prop[PROPERTY_VALUE_MAX];

    phyId = multi_id_to_phyid(camera_id);
    if (camera_id < 0 || phyId >= mNumOfCameras)
        return -ENODEV;

    if (phyId < 0)
        phyId = 0;
    phyPtr = sensorGetPhysicalSnsInfo(phyId);
    if (phyPtr->phyId != phyPtr->slotId)
        return -ENODEV;

    SprdCamera3HWI *hw = new SprdCamera3HWI(phyId);

    if (!hw) {
        HAL_LOGE("Allocation of hardware interface failed");
        return NO_MEMORY;
    }

    // Only 3d calibration use this
    // TBD: move 3d calibration to multi camera
    HAL_LOGD("SPRD Camera Hal, camera_id=%d", camera_id);
    if (SPRD_3D_CALIBRATION_ID == camera_id) {
        hw->setMultiCameraMode(MODE_3D_CALIBRATION);
        property_set("vendor.cam.dualmode", "high_mode");
    }
    if (SPRD_OPTICSZOOM_W_ID == camera_id) {
        hw->setMultiCameraMode(MODE_OPTICSZOOM_CALIBRATION);
    }
    if (SPRD_OPTICSZOOM_T_ID == camera_id) {
        hw->setMultiCameraMode(MODE_OPTICSZOOM_CALIBRATION);
    }
    rc = hw->openCamera(hw_device);
    if (rc != 0) {
        delete hw;
    } else {
        if (SPRD_FRONT_HIGH_RES == camera_id ||
            SPRD_BACK_HIGH_RESOLUTION_ID == camera_id) {
            is_high_res_mode = 1;
            hw->camera_ioctrl(CAMERA_TOCTRL_SET_HIGH_RES_MODE,
                              &is_high_res_mode, NULL);
        }

        property_get("persist.vendor.cam.ultrawide.enable", prop, "1");
        if (SPRD_ULTRA_WIDE_ID == camera_id)
            on_off = atoi(prop);
        hw->camera_ioctrl(CAMERA_IOCTRL_ULTRA_WIDE_MODE, &on_off, NULL);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : camera_device_open
 *
 * DESCRIPTION: static function to open a camera device by its ID
 *
 * PARAMETERS :
 *   @camera_id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3Factory::camera_device_open(const struct hw_module_t *module,
                                           const char *id,
                                           struct hw_device_t **hw_device) {
    char value[PROPERTY_VALUE_MAX];
    if (module != &HAL_MODULE_INFO_SYM.common) {
        HAL_LOGE("Invalid module. Trying to open %p, expect %p", module,
                 &HAL_MODULE_INFO_SYM.common);
        return INVALID_OPERATION;
    }
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }

    HAL_LOGI("SPRD Camera Hal :hal3: cameraId=%d", atoi(id));

#ifdef CONFIG_MULTICAMERA_SUPPORT
    if (atoi(id) == SPRD_MULTI_CAMERA_ID || atoi(id) ==SPRD_FOV_FUSION_ID) {
        if (NULL == sensorGetLogicaInfo4MulitCameraId(SPRD_MULTI_CAMERA_ID)) {
            id = "0";
        }
    }
#endif

#ifdef CONFIG_PORTRAIT_SCENE_SUPPORT
        property_get("persist.vendor.cam.ip.wechat.back.replace", value, "0");
        if(!strcmp(value,"1")) {
            HAL_LOGD("com.tencent.mm call camera, use camera pbrp");
            if (atoi(id) == 0){
                id = "53";
            }else if (atoi(id) == 1){
                id = "52";
            }else{
                HAL_LOGI("unsupport camera id for tencent camera!!");
            }
        }
#endif
    // for camera id 0&2&3 debug
    property_get("persist.vendor.cam.id", value, "null");
    if (value[0] >= '0' && value[0] <= '9') {
        value[1] = '\0';
        id = value;
    }

    HAL_LOGI("multi cameraId=%d", atoi(id));

#ifdef CONFIG_MULTICAMERA_SUPPORT
    if (atoi(id) == SPRD_MULTI_CAMERA_ID || atoi(id) ==SPRD_FOV_FUSION_ID) {
        return SprdCamera3MultiCamera::camera_device_open(module, id,
                                                          hw_device);
    }
#endif

    if (is_single_expose(atoi(id))) {
        return gSprdCamera3Wrapper->cameraDeviceOpen(module, id, hw_device);
    } else {
        return gSprdCamera3Factory.cameraDeviceOpen(atoi(id), hw_device);
    }
}

struct hw_module_methods_t SprdCamera3Factory::mModuleMethods = {
    .open = SprdCamera3Factory::camera_device_open,
};

/*Camera ID Expose To Camera Apk On MultiCameraMode
camera apk use one camera id to open camera on MODE_3D_VIDEO, MODE_RANGE_FINDER,
MODE_3D_CAPTURE
camera apk use two camera id MODE_REFOCUS and 2 to open Camera
ValidationTools apk use two camera id MODE_3D_CALIBRATION and 3 to open Camera
*/
bool SprdCamera3Factory::is_single_expose(int cameraId) {

    if ((SprdCamera3Setting::mPhysicalSensorNum > cameraId) ||
        (cameraId > SPRD_MULTI_CAMERA_MAX_ID)) {
        return false;
    }

    if ((SPRD_REFOCUS_ID == cameraId) || (SPRD_3D_CALIBRATION_ID == cameraId) ||
        (SPRD_ULTRA_WIDE_ID == cameraId) ||
        (SPRD_BACK_HIGH_RESOLUTION_ID == cameraId) ||
        (SPRD_FRONT_HIGH_RES == cameraId) ||
        (SPRD_OPTICSZOOM_W_ID == cameraId) ||
        (SPRD_OPTICSZOOM_T_ID == cameraId)) {
        return false;
    }

    return true;
}

int SprdCamera3Factory::multi_id_to_phyid(int cameraId) {
    char prop[PROPERTY_VALUE_MAX];

    if (MIN_MULTI_CAMERA_FAKE_ID > cameraId) {
        return cameraId;
    } else if (SPRD_REFOCUS_ID == cameraId ||
               SPRD_BACK_HIGH_RESOLUTION_ID == cameraId) {
        return 0;
    } else if (SPRD_3D_CALIBRATION_ID == cameraId) {
        property_get("persist.vendor.cam.ba.blur.version", prop, "0");
        if (6 == atoi(prop)) {
            return 0;
        }
        return 1;
    } else if (SPRD_ULTRA_WIDE_ID == cameraId) {
        return sensorGetRole(MODULE_SPW_NONE_BACK);
    } else if (SPRD_FRONT_HIGH_RES == cameraId) {
        return 1;
    } else if (SPRD_OPTICSZOOM_W_ID == cameraId) {
        return sensorGetRole(MODULE_OPTICSZOOM_WIDE_BACK);
    } else if (SPRD_OPTICSZOOM_T_ID == cameraId) {
        return sensorGetRole(MODULE_OPTICSZOOM_TELE_BACK);
    }

    return 0xff;
}

}; // namespace sprdcamera
