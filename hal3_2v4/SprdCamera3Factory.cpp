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
    camera_info info;
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
    if (isSingleIdExposeOnMultiCameraMode(camera_id))
        return gSprdCamera3Wrapper->getCameraInfo(camera_id, info);
    else
        return gSprdCamera3Factory.getCameraInfo(
            multiCameraModeIdToPhyId(camera_id), info);
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
    if(mNumOfCameras == 0) {
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
    struct camera_device_manager *devPtr = NULL;
    Mutex::Autolock l(mLock);

    HAL_LOGD("E, camera_id = %d", camera_id);
    if (!mNumOfCameras || camera_id >= mNumOfCameras || !info ||
        (camera_id < 0)) {
        return -ENODEV;
    }

    devPtr =
        (struct camera_device_manager *)SprdCamera3Setting::getCameraIdentifyState();
    for (int m = 0; m < mNumOfCameras; m++) {
        HAL_LOGV("factory identify_state[%d]=%d", m, devPtr->identify_state[m]);
        if (gSprdCamera3Factory.mCameraCallbacks != NULL) {
            gSprdCamera3Factory.mCameraCallbacks->camera_device_status_change(
                gSprdCamera3Factory.mCameraCallbacks,
                m, devPtr->identify_state[m]);
        }
    }

    SprdCamera3Setting::getSensorStaticInfo(camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->static_camera_characteristics = mStaticMetadata;
    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_2;
    info->conflicting_devices_length = 0;

    HAL_LOGD("X");
    return rc;
}
/*====================================================================
*FUNCTION     :setTorchMode
*DESCRIPTION  :Attempt to turn on or off the torch of the flash unint.
*
*PARAMETERS   :
*      @camera_id : camera ID
*      @enabled   : Indicate whether to turn the flash on or off
*
*RETURN       : 0         --success
*               non zero  --failure
*===================================================================*/
int SprdCamera3Factory::setTorchMode(const char *camera_id, bool enabled) {
    int retval = 0;
    ALOGV("%s: In,camera_id:%s,enable:%d", __func__, camera_id, enabled);

    retval = SprdCamera3Flash::setTorchMode(camera_id, enabled);
    ALOGV("retval = %d", retval);

    return retval;
}

int SprdCamera3Factory::set_callbacks(
    const camera_module_callbacks_t *callbacks) {
    ALOGV("%s :In", __func__);
    int retval = 0;
    gSprdCamera3Factory.mCameraCallbacks = callbacks;
    retval = SprdCamera3Flash::registerCallbacks(callbacks);

    return retval;
}

void SprdCamera3Factory::get_vendor_tag_ops(vendor_tag_ops_t *ops) {
    ALOGV("%s : ops=%p", __func__, ops);
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
    struct phySensorInfo *phyPtr = NULL;

    if (camera_id < 0 || multiCameraModeIdToPhyId(camera_id) >= mNumOfCameras)
        return -ENODEV;

    phyPtr = sensorGetPhysicalSnsInfo(multiCameraModeIdToPhyId(camera_id));
    if (phyPtr->phyId != phyPtr->slotId)
        return -ENODEV;

    SprdCamera3HWI *hw =
        new SprdCamera3HWI(multiCameraModeIdToPhyId(camera_id));

    if (!hw) {
        ALOGE("Allocation of hardware interface failed");
        return NO_MEMORY;
    }
    HAL_LOGD("SPRD Camera Hal");
    HAL_LOGD("camera_id=%d", camera_id);
    rc = hw->openCamera(hw_device);
    if (rc != 0) {
        delete hw;
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
    if (module != &HAL_MODULE_INFO_SYM.common) {
        ALOGE("Invalid module. Trying to open %p, expect %p", module,
              &HAL_MODULE_INFO_SYM.common);
        return INVALID_OPERATION;
    }
    if (!id) {
        ALOGE("Invalid camera id");
        return BAD_VALUE;
    }

    if (isSingleIdExposeOnMultiCameraMode(atoi(id))) {
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
bool SprdCamera3Factory::isSingleIdExposeOnMultiCameraMode(int cameraId) {
    /*Camera ID Expose To Camera Apk On MultiCameraMode*/
    if ((SPRD_MULTI_CAMERA_BASE_ID > cameraId) ||
        (cameraId > SPRD_MULTI_CAMERA_MAX_ID))
        return false;

    if ((SPRD_3D_VIDEO_ID == cameraId) || (SPRD_RANGE_FINDER_ID == cameraId) ||
        (SPRD_3D_CAPTURE_ID == cameraId) || (SPRD_3D_PREVIEW_ID == cameraId) ||
        (SPRD_BLUR_ID == cameraId) || (SPRD_SELF_SHOT_ID == cameraId) ||
        (SPRD_PAGE_TURN_ID == cameraId) || (SPRD_BLUR_FRONT_ID == cameraId) ||
        (SPRD_SINGLE_FACEID_REGISTER_ID == cameraId) ||
        (SPRD_SINGLE_FACEID_UNLOCK_ID == cameraId) ||
        (SPRD_PORTRAIT_ID == cameraId) ||
        (SPRD_PORTRAIT_SINGLE_ID == cameraId)) {
        return true;
    } else if (SPRD_REFOCUS_ID == cameraId ||
               (SPRD_3D_CALIBRATION_ID == cameraId) ||
               (SPRD_BACK_HIGH_RESOLUTION_ID == cameraId)) {
        return false;
    }
    /* This camera id should return false
     * if (cameraId == SPRD_FRONT_HIGH_RES)
     *     return false;
     */

    return false;
}

int SprdCamera3Factory::multiCameraModeIdToPhyId(int cameraId) {
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (MIN_MULTI_CAMERA_FAKE_ID > cameraId) {
        return cameraId;
    } else if (SPRD_REFOCUS_ID == cameraId) { // Camera2 apk open  camera id is
        // SPRD_REFOCUS_ID and 2 ,camera hal
        // transform to open physics Camera
        // id is 0 and 2
        return 0;
    } else if (SPRD_3D_CALIBRATION_ID ==
               cameraId) { // ValidationTools apk open  camera id is
        // SPRD_3D_CALIBRATION_ID and 3 ,camera hal transform to
        // open physics Camera id is 1 and 3
        property_get("persist.vendor.cam.ba.blur.version", prop, "0");
        if (6 == atoi(prop)) {
            return 0;
        }
        return 1;
    }

    return 0xff;
}

}; // namespace sprdcamera
