/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
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
#define LOG_TAG "SprdCamera3SelfShot"
//#define LOG_NDEBUG 0
#include "SprdCamera3SelfShot.h"

using namespace android;
namespace sprdcamera {
SprdCamera3SelfShot *mSelfShot = NULL;

//Error Check Macros
#define CHECK_MUXER() \
    if (!mSelfShot) { \
        HAL_LOGE("Error getting muxer "); \
        return; \
    } \

//Error Check Macros
#define CHECK_MUXER_ERROR() \
    if (!mSelfShot) { \
        HAL_LOGE("Error getting muxer "); \
        return -ENODEV; \
    } \

#define CHECK_HWI(hwi) \
    if (!hwi) { \
        HAL_LOGE("Error !! HWI not found!!"); \
        return; \
    } \

#define CHECK_HWI_ERROR(hwi) \
    if (!hwi) { \
        HAL_LOGE("Error !! HWI not found!!"); \
        return -ENODEV; \
    } \

#define CHECK_CAMERA(sprdCam) \
            if (!sprdCam) { \
                HAL_LOGE("Error getting physical camera"); \
                return; \
            } \

#define CHECK_CAMERA_ERROR(sprdCam) \
            if (!sprdCam) { \
                HAL_LOGE("Error getting physical camera"); \
                return -ENODEV; \
            } \

camera3_device_ops_t SprdCamera3SelfShot::mCameraMuxerOps = {
    .initialize =                               SprdCamera3SelfShot::initialize,
    .configure_streams =                        SprdCamera3SelfShot::configure_streams,
    .register_stream_buffers =                  NULL,
    .construct_default_request_settings =       SprdCamera3SelfShot::construct_default_request_settings,
    .process_capture_request =                  SprdCamera3SelfShot::process_capture_request,
    .get_metadata_vendor_tag_ops =              NULL,
    .dump =                                     SprdCamera3SelfShot::dump,
    .flush =                                    SprdCamera3SelfShot::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3SelfShot::callback_ops_main = {
    .process_capture_result =                   SprdCamera3SelfShot::process_capture_result_main,
    .notify =                                   SprdCamera3SelfShot::notifyMain
};
camera3_callback_ops SprdCamera3SelfShot::callback_ops_aux = {
    .process_capture_result =                   SprdCamera3SelfShot::process_capture_result_aux,
    .notify =                                   SprdCamera3SelfShot::notifyAux
};

/*===========================================================================
 * FUNCTION         : SprdCamera3SelfShot
 *
 * DESCRIPTION     : SprdCamera3SelfShot Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3SelfShot::SprdCamera3SelfShot()
{
    HAL_LOGD(" E");
    m_nPhyCameras = 2;

    mStaticMetadata = NULL;
    setupPhysicalCameras();
    m_VirtualCamera.id = SELF_SHOT_CAM_MAIN_ID;
    HAL_LOGD("X");

}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3SelfShot
 *
 * DESCRIPTION     : SprdCamera3SelfShot Desctructor
 *
 *==========================================================================*/
SprdCamera3SelfShot::~SprdCamera3SelfShot()
{
    HAL_LOGD("E");
    if (m_pPhyCamera) {
        delete [] m_pPhyCamera;
        m_pPhyCamera = NULL;
     }
    HAL_LOGD("X");

}

/*===========================================================================
 * FUNCTION         : getCameraMuxer
 *
 * DESCRIPTION     : Creates Camera Muxer if not created
 *
 * PARAMETERS:
 *   @pMuxer               : Pointer to retrieve Camera Muxer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3SelfShot::getCameraMuxer(SprdCamera3SelfShot** pMuxer)
{

    *pMuxer = NULL;
    if (!mSelfShot) {
         mSelfShot = new SprdCamera3SelfShot();
    }
    CHECK_MUXER();
    *pMuxer = mSelfShot;
    HAL_LOGD("mSelfShot: %p ", mSelfShot);

    return;
}

/*===========================================================================
 * FUNCTION         : get_camera_info
 *
 * DESCRIPTION     : get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical Main Camera Info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3SelfShot::get_camera_info(__unused int camera_id, struct camera_info *info)
{

    int rc = NO_ERROR;

    HAL_LOGD("E");
    if (info) {
         rc = mSelfShot->getCameraInfo(info);
    }
    HAL_LOGD("X, rc: %d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   : camera_device_open
 *
 * DESCRIPTION: static function to open a camera device by its ID
 *
 * PARAMETERS :
 *   @modue: hw module
 *   @id : camera ID
 *   @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              BAD_VALUE : Invalid Camera ID
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3SelfShot::camera_device_open(
        __unused const struct hw_module_t *module, const char *id,
        struct hw_device_t **hw_device)
{

    int rc = NO_ERROR;

    HAL_LOGD("id= %d",atoi(id));
    if (!id) {
         HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
     }

    rc =  mSelfShot->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGD("id= %d, rc: %d", atoi(id), rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   : close_camera_device
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *   @hw_dev : camera hardware device info
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3SelfShot::close_camera_device(__unused hw_device_t *hw_dev)
{
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mSelfShot->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : closeCameraDevice
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3SelfShot::closeCameraDevice()
{

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t*)(sprdCam->dev);
        if(dev == NULL)
            continue;

        HAL_LOGW("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }

    HAL_LOGD("X, rc: %d", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize camera device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::initialize(__unused const struct camera3_device *device,
                        const camera3_callback_ops_t *callback_ops)
{
    int rc = NO_ERROR;

    HAL_LOGD("E");
    CHECK_MUXER_ERROR();

    rc = mSelfShot->initialize(callback_ops);

    HAL_LOGD("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   :construct_default_request_settings
 *
 * DESCRIPTION: construc default request settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t * SprdCamera3SelfShot::construct_default_request_settings(const struct camera3_device *device, int type)
{
    const camera_metadata_t* rc;

    HAL_LOGD("E");
    if (!mSelfShot) {
        HAL_LOGE("Error getting muxer ");
        return NULL;
    }
    rc = mSelfShot->constructDefaultRequestSettings(device,type);

    HAL_LOGD("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :configure_streams
 *
 * DESCRIPTION: configure streams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::configure_streams(const struct camera3_device *device, camera3_stream_configuration_t* stream_list)
{
    int rc=0;

    HAL_LOGD(" E");
    CHECK_MUXER_ERROR();
    rc = mSelfShot->configureStreams(device,stream_list);

    HAL_LOGD(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_request
 *
 * DESCRIPTION: process capture request
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::process_capture_request(const struct camera3_device* device, camera3_capture_request_t *request)
{
    int rc=0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_MUXER_ERROR();
    rc = mSelfShot->processCaptureRequest(device,request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3SelfShot
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SelfShot::process_capture_result_main(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result)
{
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
    mSelfShot->processCaptureResultMain((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SelfShot::notifyMain(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg)
{
    CHECK_MUXER();
    mSelfShot->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3SelfShot
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SelfShot::process_capture_result_aux(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result)
{
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();

}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION:  aux sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SelfShot::notifyAux(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg)
{
    CHECK_MUXER();
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
int SprdCamera3SelfShot::cameraDeviceOpen(__unused int camera_id,
        struct hw_device_t **hw_device)
{
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGD(" E");
    hw_device_t *hw_dev = NULL;
    mOpenSubsensor = 0;
    phyId = m_pPhyCamera[CAM_TYPE_MAIN].id;
    SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
    if (!hw) {
        HAL_LOGE("Allocation of hardware interface failed");
        return NO_MEMORY;
    }
    hw->setMultiCameraMode((multiCameraMode)camera_id);
    rc = hw->openCamera(&hw_dev);
    if (rc != NO_ERROR) {
        HAL_LOGE("failed, camera id:%d", phyId);
        delete hw;
        closeCameraDevice();
        return rc;
    }

    m_pPhyCamera[CAM_TYPE_MAIN].dev = reinterpret_cast<camera3_device_t*>(hw_dev);
    m_pPhyCamera[CAM_TYPE_MAIN].hwi = hw;
    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraMuxerOps;
    m_VirtualCamera.dev.priv = (void*)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;

    HAL_LOGD("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : getCameraInfo
 *
 * DESCRIPTION: query camera information with its ID
 *
 * PARAMETERS :
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3SelfShot::getCameraInfo(struct camera_info *info)
{
    int rc = NO_ERROR;
    int camera_id = m_VirtualCamera.id;
    HAL_LOGD("E, camera_id = %d", camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;//CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;

    info->conflicting_devices_length = 0;
    HAL_LOGD("X");
    return rc;
}

/*===========================================================================
 * FUNCTION         : setupPhysicalCameras
 *
 * DESCRIPTION     : Creates Camera Muxer if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3SelfShot::setupPhysicalCameras()
{
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,(m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_MAIN].id = SELF_SHOT_CAM_MAIN_ID;
    m_pPhyCamera[CAM_TYPE_AUX].id = SELF_SHOT_CAM_AUX_ID;

    return NO_ERROR;
}

/*======================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump fd
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3SelfShot::dump(const struct camera3_device *device, int fd)
{
    HAL_LOGD("E");
    CHECK_MUXER();

    mSelfShot->_dump(device,fd);

    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::flush(const struct camera3_device *device)
{
    int rc=0;

    HAL_LOGD(" E");
    CHECK_MUXER_ERROR();

    rc = mSelfShot->_flush(device);

    HAL_LOGD(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::initialize(const camera3_callback_ops_t *callback_ops)
{
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGD("E");
    CHECK_HWI_ERROR(hwiMain);
    mAvailableSensorSelfSot = 1;
    mCallbackOps = callback_ops;

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   :configureStreams
 *
 * DESCRIPTION: configureStreams
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::configureStreams(const struct camera3_device *device,camera3_stream_configuration_t* stream_list)
{
    int rc = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,stream_list);
    if (rc < 0) {
        HAL_LOGE("failed. configure main streams!!");
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   :constructDefaultRequestSettings
 *
 * DESCRIPTION: construct Default Request Settings
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
const camera_metadata_t * SprdCamera3SelfShot::constructDefaultRequestSettings(const struct camera3_device *device, int type)
{
    HAL_LOGD("E");
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(m_pPhyCamera[CAM_TYPE_MAIN].dev,type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    HAL_LOGD("X");

    return fwk_metadata;

}

 /*===========================================================================
 * FUNCTION   :processCaptureRequest
 *
 * DESCRIPTION: process Capture Request
 *
 * PARAMETERS :
 *    @device: camera3 device
 *    @request:camera3 request
 * RETURN     :
 *==========================================================================*/
int SprdCamera3SelfShot::processCaptureRequest(const struct camera3_device *device,camera3_capture_request_t *request)
{
    int rc = 0;
    CameraMetadata metadata;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    char prop[PROPERTY_VALUE_MAX] = {0,};

    metadata = request->settings;
    if (metadata.exists(ANDROID_SPRD_AVAILABLE_SENSOR_SELF_SHOT)) {
        mAvailableSensorSelfSot = metadata.find(ANDROID_SPRD_AVAILABLE_SENSOR_SELF_SHOT).data.i32[0];
    }
    property_get("camera.selfshot.enble", prop, "2");
    if (2 != atoi(prop)) {
        mAvailableSensorSelfSot  = atoi(prop);
    }

    HAL_LOGD("mAvailableSensorSelfSot=:%d mOpenSubsensor=%d", mAvailableSensorSelfSot,mOpenSubsensor);
    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,request);
    if (rc < 0) {
        HAL_LOGD("process request failed.");
        return rc;
    }
    if (mAvailableSensorSelfSot && !mOpenSubsensor) {
        int phyId = m_pPhyCamera[CAM_TYPE_AUX].id;

        HAL_LOGE("open sub camera E");
        hw_device_t *hw_dev = NULL;
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw->setMultiCameraMode((multiCameraMode)MODE_SELF_SHOT);
        rc = hw->openCamera(&hw_dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            closeCameraDevice();
            return rc;
        }
        m_pPhyCamera[CAM_TYPE_AUX].dev = reinterpret_cast<camera3_device_t*>(hw_dev);
        m_pPhyCamera[CAM_TYPE_AUX].hwi = hw;
        CHECK_HWI_ERROR(m_pPhyCamera[CAM_TYPE_AUX].hwi);
        HAL_LOGE("open sub camera ok");
        mOpenSubsensor = true;
    } else if (!mAvailableSensorSelfSot && mOpenSubsensor) {
        HAL_LOGE("close sub camera E");
        sprdcamera_physical_descriptor_t *sprdCam = NULL;
        sprdCam = &m_pPhyCamera[CAM_TYPE_AUX];
        hw_device_t *dev = (hw_device_t*)(sprdCam->dev);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("close sub camera failed");
        }
        HAL_LOGE("close sub camera ok");
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
        mOpenSubsensor = false;
    }

   return rc;
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION: device notify
 *
 * PARAMETERS : notify message
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3SelfShot::notifyMain( const camera3_notify_msg_t* msg)
{
  mCallbackOps->notify(mCallbackOps, msg);
}

/*===========================================================================
 * FUNCTION   :getCoveredValue
 *
 * DESCRIPTION: get sub sensor covered value
 *
 * PARAMETERS :
 *
 * RETURN     : covered value
 *==========================================================================*/
int SprdCamera3SelfShot::getCoveredValue()
{
    int rc = 0;
    uint32_t couvered_value = 0;
    char prop[PROPERTY_VALUE_MAX] = {0,};
    property_get("debug.camera.covered", prop, "0");
#if 0
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiMain->getCoveredValue(&couvered_value);
    if (rc < 0) {
        HAL_LOGD("read sub sensor failed");
    }
#endif
    if (0 != atoi(prop)) {
        couvered_value = atoi(prop);
    }
    if (couvered_value < SELFSHOT_ALL_CONVERED_VALURE && couvered_value)
        couvered_value = 1;

    HAL_LOGD("get cover_value %u", couvered_value);
    return couvered_value;

}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3SelfShot::processCaptureResultMain(camera3_capture_result_t *result)
{
    uint32_t cur_frame_number = result->frame_number;

    if (result->output_buffers == NULL) {
        if (mOpenSubsensor) {
            mCoveredValue = getCoveredValue();
        }
        if (mCoveredValue && mOpenSubsensor && cur_frame_number > 100) {
            CameraMetadata metadata;
            metadata = result->result;
            metadata.update(ANDROID_SPRD_BLUR_COVERED,&mCoveredValue,1);
            result->result = metadata.release();
            HAL_LOGD("mCoveredValue=%d,",mCoveredValue);
        }
        mCallbackOps->process_capture_result(mCallbackOps,result);
        return ;
    } else {
        mCallbackOps->process_capture_result(mCallbackOps,result);
    }

    return;
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3SelfShot
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3SelfShot::_dump(const struct camera3_device *device, int fd)
{
    HAL_LOGD(" E");

    HAL_LOGD("X");
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION: flush
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3SelfShot::_flush(const struct camera3_device *device)
{
    int rc = 0;

    HAL_LOGD("E");
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);
    HAL_LOGD("X");

    return rc;
}

};

