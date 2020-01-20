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
#define LOG_TAG "Cam3Multi"
//#define LOG_NDEBUG 0
#include "SprdCamera3Multi.h"
#if (MINICAMERA != 1)
#include <ui/Fence.h>
#endif
using namespace android;
namespace sprdcamera {
SprdCamera3Multi *mMultiBase = NULL;

// Error Check Macros
#define CHECK_BASE()                                                           \
    if (!mMultiBase) {                                                         \
        HAL_LOGE("Error getting blur ");                                       \
        return;                                                                \
    }

// Error Check Macros
#define CHECK_BASE_ERROR()                                                     \
    if (!mMultiBase) {                                                         \
        HAL_LOGE("Error getting blur ");                                       \
        return -ENODEV;                                                        \
    }

#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }

#define CHECK_STREAM_ERROR(stream)                                             \
    if (!stream) {                                                             \
        HAL_LOGE("Error !! stream not found!! check config file ");            \
        return -ENODEV;                                                        \
    }
#ifdef MAX_SAVE_QUEUE_SIZE
#undef MAX_SAVE_QUEUE_SIZE
#endif
#define MAX_SAVE_QUEUE_SIZE 20

#ifdef MAX_NOTIFY_QUEUE_SIZE
#undef MAX_NOTIFY_QUEUE_SIZE
#endif
#define MAX_NOTIFY_QUEUE_SIZE 50

static config_multi_camera multi_camera_physical_config = {
#include "mulitCamera_config_template.h"
};

camera3_device_ops_t SprdCamera3Multi::mCameraCaptureOps = {
    .initialize = SprdCamera3Multi::initialize,
    .configure_streams = SprdCamera3Multi::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3Multi::construct_default_request_settings,
    .process_capture_request = SprdCamera3Multi::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3Multi::dump,
    .flush = SprdCamera3Multi::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3Multi::callback_ops_multi[] = {
    {.process_capture_result = SprdCamera3Multi::process_capture_result_main,
     .notify = SprdCamera3Multi::notifyMain},
    {.process_capture_result = SprdCamera3Multi::process_capture_result_aux1,
     .notify = SprdCamera3Multi::notify_Aux1},
    {.process_capture_result = SprdCamera3Multi::process_capture_result_aux2,
     .notify = SprdCamera3Multi::notify_Aux2},
    {.process_capture_result = SprdCamera3Multi::process_capture_result_aux3,
     .notify = SprdCamera3Multi::notify_Aux3}

};
SprdCamera3Multi::SprdCamera3Multi() {
    m_nPhyCameras = 2;
    mIsCapturing = false;
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    mMultiMode = MODE_SINGLE_CAMERA;
    mStaticMetadata = NULL;
    mLocalBufferNumber = 0;
    mCapFrameNum = -1;
    mCurFrameNum = 0;
    mRequstState = PREVIEW_REQUEST_STATE;
    mFlushing = false;
    mReqConfigNum = 0;
    mFirstFrameCount = 0;
    mMetaNotifyIndex = -1;
    mWaitFrameNum = -1;
    mSendFrameNum = -1;
    mSavedCapReqsettings = NULL;
    mIsSyncFirstFrame = false;
    mMetadataList.clear();
    mLocalBufferList.clear();
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    mNotifyListAux1.clear();
    mNotifyListAux2.clear();
    mNotifyListAux3.clear();
    mNotifyListMain.clear();
    bzero(&m_pPhyCamera,
          sizeof(sprdcamera_physical_descriptor_t) * MAX_MULTI_NUM_CAMERA);
    bzero(&mHalReqConfigStreamInfo,
          sizeof(hal_req_stream_config_total) * MAX_MULTI_NUM_STREAMS);
    bzero(&m_VirtualCamera, sizeof(sprd_virtual_camera_t));
    bzero(&mBufferInfo, sizeof(hal_buffer_info) * MAX_MULTI_NUM_BUFFER);
    bzero(&mLocalBuffer,
          sizeof(new_mem_t) * MAX_MULTI_NUM_BUFFER * MAX_MULTI_NUM_BUFFER);
    bzero(&mMainSize, sizeof(stream_size_t));
}
SprdCamera3Multi::~SprdCamera3Multi() {
    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);
}

/*===========================================================================
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: get logical camera info
 *
 * PARAMETERS:
 *	 @camera_id 	: Logical Camera ID
 *	 @info				: Logical Main Camera Info
 *   @pMulti        :  SprdCamera3Multi*
 * RETURN	 :
 *				NO_ERROR  : success
 *				ENODEV : Camera not found
 *				other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Multi::get_camera_info(__unused int camera_id,
                                      struct camera_info *info,
                                      SprdCamera3Multi *pMulti) {
    int rc = NO_ERROR;

    HAL_LOGV("E");

    if (pMulti) {
        mMultiBase = pMulti;
    }
    if (info) {
        rc = mMultiBase->getCameraInfo(camera_id, info);
    }
    HAL_LOGV("X, rc: %d", rc);

    return rc;
}
/*===========================================================================
 * FUNCTION   : getCameraInfo
 *
 * DESCRIPTION: get logical camera info
 *
 * PARAMETERS:
 *	 @camera_id 	: Logical Camera ID
 *	 @info				: Logical Main Camera Info
 *
 * RETURN	 :
 *				NO_ERROR  : success
 *				ENODEV : Camera not found
 *				other: non-zero failure code
 *==========================================================================*/

int SprdCamera3Multi::getCameraInfo(int id, struct camera_info *info) {
    int rc = NO_ERROR;
    struct logicalSensorInfo *logicalPtr = NULL;
    int i = 0;

    config_multi_camera *config_info = load_config_file();
    if (!config_info) {
        HAL_LOGE("failed to get config file ");
    }
    parse_configure_info(config_info);

    logicalPtr = sensorGetLogicaInfo4MulitCameraId(id);
    if (logicalPtr) {
        if (m_nPhyCameras == logicalPtr->physicalNum) {
            for (i = 0; i < logicalPtr->physicalNum; i++) {
                m_pPhyCamera[i].id = (uint8_t)logicalPtr->phyIdGroup[i];
                HAL_LOGD("i = %d, phyId = %d", i, logicalPtr->phyIdGroup[i]);
            }
        }
    }

    if (mStaticMetadata)
        free_camera_metadata(mStaticMetadata);

    uint32_t camera_id = m_VirtualCamera.id;
    HAL_LOGI("E, camera_id = %d", camera_id);

    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        camera_id = sprdCam.id;
        if (camera_id != m_VirtualCamera.id) {
            SprdCamera3Setting::getSensorStaticInfo(camera_id);
            SprdCamera3Setting::initDefaultParameters(camera_id);
            rc = SprdCamera3Setting::getStaticMetadata(camera_id,
                                                       &mStaticMetadata);
            if (rc < 0) {
                return rc;
            }
        }
    }
    camera_id = m_VirtualCamera.id;
    SprdCamera3Setting::getSensorStaticInfo(camera_id);
    SprdCamera3Setting::initDefaultParameters(camera_id);
    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    CameraMetadata metadata = clone_camera_metadata(mStaticMetadata);
    uint8_t kavailable_physical_ids[2 * m_nPhyCameras];
    for (uint32_t i = 0, j = 0; i < 2 * m_nPhyCameras; i++) {
        if (i % 2 == 0 && i != 0)
            kavailable_physical_ids[i] = '\0';
        else
            kavailable_physical_ids[i] = m_pPhyCamera[j++].id + '0';
    }
    if (SPRD_MULTI_CAMERA_BASE_ID > id) {
        HAL_LOGI(" logical id %d", id);
        setLogicIdTag(metadata, (uint8_t *)kavailable_physical_ids,
                      2 * m_nPhyCameras);
    }
    // config mStaticMetadata
    reConfigGetCameraInfo(metadata);
    mStaticMetadata = metadata.release();
    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version =
        CAMERA_DEVICE_API_VERSION_3_2; // CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;

    HAL_LOGI("X");
    return rc;
}

/*===========================================================================
 * FUNCTION   : createHalStream
 *
 * DESCRIPTION: create stream
 *
 * PARAMETERS:
                                : Logical Main Camera Info
 *
 * RETURN	 :

 *==========================================================================*/
int SprdCamera3Multi::createHalStream(
    camera3_stream_t *preview_stream, camera3_stream_t *callback_stream,
    camera3_stream_t *snap_stream, camera3_stream_t *video_stream,
    camera3_stream_t *output_list,
    sprdcamera_physical_descriptor_t *camera_phy_info) {
    if (output_list == NULL || camera_phy_info == NULL) {
        HAL_LOGE("param is null");
        return UNKNOWN_ERROR;
    }

    for (int i = 0; i < camera_phy_info->stream_num; i++) {
        camera3_stream_t *output = (camera3_stream_t *)&output_list[i];
        hal_stream_info *hal_stream =
            (hal_stream_info *)&camera_phy_info->hal_stream[i];
        switch (hal_stream->type) {
        case PREVIEW_STREAM:
            if (preview_stream) {
                memcpy(output, preview_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            }
            break;
        case SNAPSHOT_STREAM:
            if (snap_stream) {
                memcpy(output, snap_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_BLOB;
            }

            break;
        case CALLBACK_STREAM:
            if (callback_stream) {
                memcpy(output, callback_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_YCbCr_420_888;
                output->usage = GRALLOC_USAGE_SW_READ_OFTEN;
            }

            break;
        case VIDEO_STREAM:
            if (video_stream) {
                memcpy(output, video_stream, sizeof(camera3_stream_t));
            } else {
                output->format = HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED;
                output->usage = GRALLOC_USAGE_HW_VIDEO_ENCODER;
            }

            break;
        case DEFAULT_STREAM:
            break;

        default:
            HAL_LOGI("unknown type %d", hal_stream->type);
            break;
            output->max_buffers = 1;
        }
        if (hal_stream->width != 0 || hal_stream->height != 0) {
            output->width = hal_stream->width;
            output->height = hal_stream->height;
        }
        if (hal_stream->format) {
            output->format = hal_stream->format;
        }
        output->stream_type = CAMERA3_STREAM_OUTPUT;
        output->rotation = 0;

        HAL_LOGI("output width %d , height %d  stream type is %d, %p",
                 output->width, output->height, hal_stream->type, hal_stream);
    }

    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   : configureStreams
 *
 * DESCRIPTION: configure stream
 *
 * PARAMETERS:

 *
 * RETURN	 :

 *==========================================================================*/
int SprdCamera3Multi::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGI("E");
    int rc = 0;

    int total_stream = 0;
    camera3_stream_t *previewStream = NULL;
    camera3_stream_t *snapStream = NULL;
    camera3_stream_t *callbackStream = NULL;
    camera3_stream_t *videoStream = NULL;
    camera3_stream_t *defaultStream = NULL;
    camera3_stream_t *configstreamList[MAX_MULTI_NUM_STREAMS];
    camera3_stream_t *newStream = NULL;
    mFlushing = false;
    mMetadataList.clear();
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    mNotifyListAux1.clear();
    mNotifyListAux2.clear();
    mNotifyListAux3.clear();
    mNotifyListMain.clear();
    mMetaNotifyIndex = -1;
    mCapFrameNum = -1;
    mWaitFrameNum = -1;
    mSendFrameNum = -1;
    bzero(&mMainSize, sizeof(stream_size_t));

    memset(configstreamList, 0,
           sizeof(camera3_stream_t *) * MAX_MULTI_NUM_STREAMS);

    // first.get stream form fw.
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGI("stream num %d, type %d", stream_list->num_streams,
                 requestStreamType);
        if (requestStreamType == PREVIEW_STREAM) {
            previewStream = stream_list->streams[i];
            previewStream->max_buffers = 4;
            mMainSize.preview_w = previewStream->width;
            mMainSize.preview_h = previewStream->height;
        } else if (requestStreamType == CALLBACK_STREAM) {
            callbackStream = stream_list->streams[i];
            callbackStream->max_buffers = 4;
            mMainSize.callback_w = callbackStream->width;
            mMainSize.callback_h = callbackStream->height;
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            snapStream = stream_list->streams[i];
            snapStream->max_buffers = 1;
            mMainSize.capture_w = snapStream->width;
            mMainSize.capture_h = snapStream->height;
        } else if (requestStreamType == VIDEO_STREAM) {
            videoStream = stream_list->streams[i];
            videoStream->max_buffers = 4;
            mMainSize.video_w = videoStream->width;
            mMainSize.video_h = videoStream->height;
        } else if (requestStreamType == DEFAULT_STREAM) {
            defaultStream = stream_list->streams[i];
            defaultStream->max_buffers = 4;
        }
    }
    // second.config streamsList.
    for (size_t i = 0; i < m_nPhyCameras; i++) {

        sprdcamera_physical_descriptor_t *camera_phy_info =
            (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[i];
        camera3_stream_t *config_stream =
            (camera3_stream_t *)&(m_pPhyCamera[i].streams);

        createHalStream(previewStream, callbackStream, snapStream, videoStream,
                        config_stream, camera_phy_info);

        for (int j = 0; j < m_pPhyCamera[i].stream_num; j++) {
            int follow_type = camera_phy_info->hal_stream[j].follow_type;
            if (follow_type != 0) {
                int camera_index =
                    camera_phy_info->hal_stream[j].follow_camera_index;
                HAL_LOGD("camera=%d,follw_type=%d,index=%d", i, follow_type,
                         camera_index);
                camera3_stream_t *find_stream =
                    findStream(follow_type, (sprdcamera_physical_descriptor_t
                                                 *)&m_pPhyCamera[camera_index]);
                CHECK_STREAM_ERROR(find_stream);
                m_pPhyCamera[i].streams[j].width = find_stream->width;
                m_pPhyCamera[i].streams[j].height = find_stream->height;
            }
            configstreamList[j] = &(m_pPhyCamera[i].streams[j]);
        }

        camera3_stream_configuration config;
        config = *stream_list;
        config.num_streams = m_pPhyCamera[i].stream_num;
        config.streams = configstreamList;
        rc = (m_pPhyCamera[i].hwi)
                 ->configure_streams(m_pPhyCamera[i].dev, &config);
        if (rc < 0) {
            HAL_LOGE("failed. configure %d streams!!", i);
            return rc;
        }
    }
    sprdcamera_physical_descriptor_t *main_camera =
        (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[0];

    for (int i = 0; i < main_camera->stream_num; i++) {
        newStream = (camera3_stream_t *)&main_camera->streams[i];
        int stream_type = main_camera->hal_stream[i].type;
        if (stream_type == PREVIEW_STREAM && (previewStream != NULL)) {
            memcpy(previewStream, newStream, sizeof(camera3_stream_t));
            HAL_LOGV("previewStream  max buffers %d",
                     previewStream->max_buffers);
            previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        } else if (stream_type == SNAPSHOT_STREAM && (snapStream != NULL)) {
            memcpy(snapStream, newStream, sizeof(camera3_stream_t));
        } else if (stream_type == CALLBACK_STREAM && (callbackStream != NULL)) {
            memcpy(callbackStream, newStream, sizeof(camera3_stream_t));
            callbackStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        } else if (stream_type == VIDEO_STREAM && (videoStream != NULL)) {
            memcpy(videoStream, newStream, sizeof(camera3_stream_t));
            videoStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        } else if (stream_type == DEFAULT_STREAM && (defaultStream != NULL)) {
            memcpy(defaultStream, newStream, sizeof(camera3_stream_t));
            defaultStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        }
    }

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        HAL_LOGI("main configurestreams, streamtype:%d, format:%d, width:%d, "
                 "height:%d %p requestStreamType %d",
                 stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->width,
                 stream_list->streams[i]->height, stream_list->streams[i]->priv,
                 requestStreamType);
    }
    rc = allocateBuff();
    if (rc == -1) {
        HAL_LOGE("allocateBuff failed. rc%d.", rc);
    }
    reConfigStream();

    HAL_LOGI("x rc%d.", rc);
    return rc;
}

/*===========================================================================
 * FUNCTION   : camera_device_open
 *
 * DESCRIPTION: static function to open a camera device by its ID
 *
 * PARAMETERS :
 *	 @modue: hw module
 *	 @id : camera ID
 *	 @hw_device : ptr to struct storing camera hardware device info
 *
 * RETURN	  :
 *				NO_ERROR  : success
 *				BAD_VALUE : Invalid Camera ID
 *				other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Multi::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device, SprdCamera3Multi *pMulti) {
    int rc = NO_ERROR;

    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }
    HAL_LOGI("id= %d", atoi(id));

    if (pMulti) {
        mMultiBase = pMulti;
    }
    rc = mMultiBase->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGI("id= %d, rc: %d", atoi(id), rc);

    return rc;
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
int SprdCamera3Multi::cameraDeviceOpen(__unused int camera_id,
                                       struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint8_t phyId = 0;

    HAL_LOGV(" E");

    hw_device_t *hw_dev[m_nPhyCameras];
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((int)phyId);
        if (!hw) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode(mMultiMode);
        hw->setMasterId(m_VirtualCamera.id);
        rc = hw->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hw;
            closeCameraDevice();
            return rc;
        }

        HAL_LOGD("open id=%d success", phyId);
        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hw;
    }

    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraCaptureOps;
    m_VirtualCamera.dev.priv = (void *)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;

    reConfigOpen();
    return rc;
}

/*===========================================================================
 * FUNCTION   : close_camera_device
 *
 * DESCRIPTION: Close the camera
 *
 * PARAMETERS :
 *	 @hw_dev : camera hardware device info
 *
 * RETURN	  :
 *				NO_ERROR  : success
 *				other: non-zero failure code
 *==========================================================================*/
int SprdCamera3Multi::close_camera_device(__unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mMultiBase->closeCameraDevice();
}

/*===========================================================================
 * FUNCTION   : close  amera device
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
int SprdCamera3Multi::closeCameraDevice() {

    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;

    HAL_LOGI("E");

    reConfigClose();
    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = m_nPhyCameras; i > 0; i--) {
        sprdCam = &m_pPhyCamera[i - 1];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL)
            continue;

        HAL_LOGI("camera id:%d", sprdCam->id);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", sprdCam->id);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }
    // clear all list
    freeLocalBuffer();
    mIsSyncFirstFrame = false;
    mFirstFrameCount = 0;
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    mMetadataList.clear();
    mNotifyListAux1.clear();
    mNotifyListAux2.clear();
    mNotifyListAux3.clear();
    mNotifyListMain.clear();

    HAL_LOGI("X, rc: %d", rc);

    return rc;
}

/*===========================================================================
 * FUNCTION   :initialize
 *
 * DESCRIPTION: initialize camera device
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::initialize(__unused const struct camera3_device *device,
                                 const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_BASE_ERROR();
    rc = mMultiBase->initialize(callback_ops);

    HAL_LOGV("X");
    return rc;
}

int SprdCamera3Multi::initialize(const camera3_callback_ops_t *callback_ops) {
    int rc = 0;
    mCallbackOps = callback_ops;
    mMetadataList.clear();
    mMetaNotifyIndex = -1;
    SprdCamera3HWI *hwi = NULL;
    SprdCamera3MultiBase::initialize(mMultiMode, m_pPhyCamera[0].hwi);

    for (uint32_t i = 0; i < m_nPhyCameras; i++) {

        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        SprdCamera3HWI *hwi = sprdCam.hwi;
        CHECK_HWI_ERROR(hwi);

        rc = hwi->initialize(sprdCam.dev, &callback_ops_multi[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error  camera while initialize !! ");
            return rc;
        }
    }

    mLocalBufferNumber = 0;
    mCapFrameNum = -1;
    mCurFrameNum = -1;
    mRequstState = PREVIEW_REQUEST_STATE;
    mFlushing = false;
    mReqConfigNum = 0;
    mMetaNotifyIndex = -1;
    mWaitFrameNum = -1;
    mSendFrameNum = -1;
    mFirstFrameCount = 0;
    mSavedCapReqsettings = NULL;
    mIsSyncFirstFrame = false;
    mMetadataList.clear();
    mLocalBufferList.clear();
    mSavedPrevRequestList.clear();
    mSavedCallbackRequestList.clear();
    mSavedVideoRequestList.clear();
    bzero(&mLocalBuffer,
          sizeof(new_mem_t) * MAX_MULTI_NUM_BUFFER * MAX_MULTI_NUM_BUFFER);
    reConfigInit();

    return rc;
}

/*===========================================================================
 * FUNCTION   :construct_default_request_settings
 *
 * DESCRIPTION: construc default request settings
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
const camera_metadata_t *SprdCamera3Multi::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mMultiBase) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mMultiBase->constructDefaultRequestSettings(device, type);

    HAL_LOGV("X");
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

const camera_metadata_t *SprdCamera3Multi::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *fwk_metadata = NULL;

    const camera_metadata_t *metadata = NULL;
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        SprdCamera3HWI *hwi = sprdCam.hwi;
        if (m_VirtualCamera.id == sprdCam.id) {
            fwk_metadata =
                hwi->construct_default_request_settings(sprdCam.dev, type);
            if (!fwk_metadata) {
                HAL_LOGE("Error  camera while initialize !! ");
                return NULL;
            }
        } else {
            metadata =
                hwi->construct_default_request_settings(sprdCam.dev, type);
            if (!metadata) {
                HAL_LOGE("Error  camera while initialize !! ");
                return NULL;
            }
        }
        HAL_LOGI("X,fwk_metadata=%p", fwk_metadata);
    }

    return fwk_metadata;
}
/*===========================================================================
 * FUNCTION   :configure_streams
 *
 * DESCRIPTION: configure streams
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_BASE_ERROR();

    rc = mMultiBase->configureStreams(device, stream_list);

    HAL_LOGV(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_request
 *
 * DESCRIPTION: process capture request
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_BASE_ERROR();
    rc = mMultiBase->processCaptureRequest(device, request);

    return rc;
}

/*===========================================================================
 * FUNCTION   :saveRequest
 *
 * DESCRIPTION: save Request
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::saveRequest(
    camera3_capture_request_t *request,
    camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1], int index) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    multi_request_saved_t currRequest;
    bzero(&currRequest, sizeof(currRequest));
    Mutex::Autolock l(mRequestLock);
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        newStream->reserved[0] = NULL; // for eis crash
        currRequest.buffer = request->output_buffers[i].buffer;
        currRequest.input_buffer = request->input_buffer;
        currRequest.frame_number = request->frame_number;
        currRequest.metaNotifyIndex = index;
        if (getStreamType(newStream) == CALLBACK_STREAM) { // preview
            currRequest.preview_stream = request->output_buffers[i].stream;
            HAL_LOGV("save prev request:id %d,to list ", request->frame_number);
            mSavedPrevRequestList.push_back(currRequest);
            memcpy(&fw_buffer[PREVIEW_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
        } else if (getStreamType(newStream) == DEFAULT_STREAM) { // callback
            currRequest.callback_stream = request->output_buffers[i].stream;
            HAL_LOGV("save callck request:id %d,to list ",
                     request->frame_number);
            mSavedCallbackRequestList.push_back(currRequest);
            memcpy(&fw_buffer[CALLBACK_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));

        } else if (getStreamType(newStream) == SNAPSHOT_STREAM) { // snapshot
            mCapFrameNum = request->frame_number;
            currRequest.snap_stream = request->output_buffers[i].stream;
            memcpy(&fw_buffer[SNAPSHOT_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
            memcpy(&mSavedSnapRequest, &currRequest,
                   sizeof(multi_request_saved_t));
            // mSavedCapReqsettings = clone_camera_metadata(request->settings);
        } else if (getStreamType(newStream) == VIDEO_STREAM) { // video
            currRequest.callback_stream = request->output_buffers[i].stream;
            HAL_LOGV("save video request:id %d,to list ",
                     request->frame_number);
            mSavedCallbackRequestList.push_back(currRequest);
            memcpy(&fw_buffer[VIDEO_STREAM], &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
        }
    }
}
/*===========================================================================
 * FUNCTION   :createOutputBufferStream
 *
 * DESCRIPTION: createOutputBufferStream
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::createOutputBufferStream(
    camera3_stream_buffer_t *outputBuffer, int req_stream_mask,
    const sprdcamera_physical_descriptor_t *camera_phy_info,
    camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1],
    int *buffer_num) {
    int ret = 0;
    int stream_type = 0;
    int i = 0;
    int stream_num = 0;
    camera3_stream_buffer_t *curOutputBuffer = outputBuffer;
    int cur_stream_mask = req_stream_mask;
    while (cur_stream_mask) {
        stream_type = (req_stream_mask >> i++) & 0x1;
        cur_stream_mask >>= 1;
        if (!stream_type) {
            continue;
        }
        stream_num++;
        stream_type = 0x1 << (i - 1);
        HAL_LOGV("curOutputBuffer %p  ,type=%d", curOutputBuffer, i - 1);
        curOutputBuffer->release_fence = -1;
        curOutputBuffer->acquire_fence = -1;
        curOutputBuffer->status = CAMERA3_BUFFER_STATUS_OK;

        switch (stream_type) {
        case DEFAULT_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(DEFAULT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            break;
        case DEFAULT_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(DEFAULT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[DEFAULT_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[DEFAULT_STREAM].acquire_fence;
            break;
        case PREVIEW_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(PREVIEW_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            break;
        case PREVIEW_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(PREVIEW_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[PREVIEW_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[PREVIEW_STREAM].acquire_fence;
            break;
        case VIDEO_STREAM_HAL_BUFFER:
            curOutputBuffer->stream = findStream(VIDEO_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            break;
        case VIDEO_STREAM_FW_BUFFER:
            curOutputBuffer->stream = findStream(VIDEO_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[VIDEO_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[VIDEO_STREAM].acquire_fence;
            break;
        case CALLBACK_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(CALLBACK_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            break;
        case CALLBACK_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(CALLBACK_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[CALLBACK_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[CALLBACK_STREAM].acquire_fence;
            break;
        case SNAPSHOT_STREAM_HAL_BUFFER:
            curOutputBuffer->stream =
                findStream(SNAPSHOT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer =
                popBufferList(mLocalBufferList, curOutputBuffer->stream->width,
                              curOutputBuffer->stream->height);
            curOutputBuffer->acquire_fence = -1;
            HAL_LOGD("start take_picture");
            break;
        case SNAPSHOT_STREAM_FW_BUFFER:
            curOutputBuffer->stream =
                findStream(SNAPSHOT_STREAM, camera_phy_info);
            CHECK_STREAM_ERROR(curOutputBuffer->stream);
            curOutputBuffer->buffer = fw_buffer[SNAPSHOT_STREAM].buffer;
            curOutputBuffer->acquire_fence =
                fw_buffer[SNAPSHOT_STREAM].acquire_fence;
            HAL_LOGD("start take_picture");
            break;

        default:
            HAL_LOGE("don't kown stream,error");
        }
        if (!curOutputBuffer->buffer) {
            HAL_LOGE("buffer get failed.");
            return -1;
        }
        curOutputBuffer++;
    }
    *buffer_num = stream_num;
    HAL_LOGV("buffer_num %d", *buffer_num);
    return ret;
}

/*===========================================================================
 * FUNCTION   : reprocessReq
 *
 * DESCRIPTION: reprocessReq
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::reprocessReq(buffer_handle_t *input_buffers) {
    int ret = NO_ERROR;
    int64_t align = 0;
    int capture_w = 0;
    int capture_h = 0;
    camera3_stream_t *snap_stream = NULL;
#ifdef BOKEH_YUV_DATA_TRANSFORM
    int transform_w = 0;
    int transform_h = 0;
    buffer_handle_t *transform_buffer = NULL;
    buffer_handle_t *output_handle = NULL;
    void *transform_buffer_addr = NULL;
    void *output_handle_addr = NULL;
#endif
    camera3_capture_request_t request;
    if (!input_buffers) {
        HAL_LOGE("reprocess para is null");
        return UNKNOWN_ERROR;
    }

    camera3_stream_buffer_t output_buffer;
    camera3_stream_buffer_t input_buffer;

    memset(&output_buffer, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&input_buffer, 0x00, sizeof(camera3_stream_buffer_t));

    if (mSavedSnapRequest.buffer != NULL) {
        output_buffer.buffer = mSavedSnapRequest.buffer;
    } else {
        HAL_LOGE("reprocess buffer is null");
        ret = UNKNOWN_ERROR;
        goto exit;
    }
    snap_stream = findStream(
        SNAPSHOT_STREAM, (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[0]);
    capture_w = snap_stream->width;
    capture_h = snap_stream->height;
    HAL_LOGI("capture frame num %lld, size w=%d x h=%d", mCapFrameNum,
             capture_w, capture_h);

    input_buffer.stream = snap_stream;
    input_buffer.status = CAMERA3_BUFFER_STATUS_OK;
    input_buffer.acquire_fence = -1;
    input_buffer.release_fence = -1;

    output_buffer.stream = snap_stream;
    output_buffer.status = CAMERA3_BUFFER_STATUS_OK;
    output_buffer.acquire_fence = -1;
    output_buffer.release_fence = -1;
#ifdef BOKEH_YUV_DATA_TRANSFORM
    // workaround jpeg cant handle 16-noalign issue, when jpeg fix
    // this
    // issue, we will remove these code
    if (capture_h == 1944 && capture_w == 2592) {
        transform_w = 2592;
        transform_h = 1952;
    } else if (capture_h == 1836 && capture_w == 3264) {
        transform_w = 3264;
        transform_h = 1840;
    } else if (capture_h == 360 && capture_w == 640) {
        transform_w = 640;
        transform_h = 368;
    } else if (capture_h == 1080 && capture_w == 1920) {
        transform_w = 1920;
        transform_h = 1088;
    } else {
        transform_w = 0;
        transform_h = 0;
    }

    if (transform_w != 0) {
        transform_buffer =
            popBufferList(mLocalBufferList, transform_w, transform_h);
        output_handle = input_buffers;
        map(output_handle, &output_handle_addr);
        map(transform_buffer, &transform_buffer_addr);
        align = systemTime();
        alignTransform(output_handle_addr, capture_w, capture_h, transform_w,
                       transform_h, transform_buffer_addr);
        HAL_LOGD("alignTransform run cost %lld ms",
                 ns2ms(systemTime() - align));

        input_buffer.stream->width = transform_w;
        input_buffer.stream->height = transform_h;
        input_buffer.buffer = transform_buffer;
        output_buffer.stream->width = transform_w;
        output_buffer.stream->height = transform_h;
        unmap(output_handle);
        unmap(transform_buffer);
    } else {
        input_buffer.stream->width = capture_w;
        input_buffer.stream->height = capture_h;
        input_buffer.buffer = input_buffers;
        output_buffer.stream->width = capture_w;
        output_buffer.stream->height = capture_h;
    }
#else
    input_buffer.stream->width = capture_w;
    input_buffer.stream->height = capture_h;
    input_buffer.buffer = input_buffers;
    output_buffer.stream->width = capture_w;
    output_buffer.stream->height = capture_h;
#endif
    if (mSavedCapReqsettings != NULL) {
        request.settings = mSavedCapReqsettings;
    } else {
        HAL_LOGE("reprocess metadata is null,failed.");
    }
    request.num_output_buffers = 1;
    request.frame_number = mCapFrameNum;
    request.output_buffers = &output_buffer;
    request.input_buffer = &input_buffer;

    if (mFlushing) {
        CallBackResult(mCapFrameNum, CAMERA3_BUFFER_STATUS_ERROR,
                       SNAPSHOT_STREAM, 0);
        goto exit;
    }

    mRequstState = REPROCESS_STATE;

    if (0 > m_pPhyCamera[0].hwi->process_capture_request(m_pPhyCamera[0].dev,
                                                         &request)) {
        HAL_LOGE("failed to  reprocess capture request!");
        ret = UNKNOWN_ERROR;
        goto exit;
    }
#ifdef BOKEH_YUV_DATA_TRANSFORM
    pushBufferList(mLocalBuffer, transform_buffer, mLocalBufferNumber,
                   mLocalBufferList);
#endif

exit:
    if (NULL != mSavedCapReqsettings) {
        free_camera_metadata(mSavedCapReqsettings);
        mSavedCapReqsettings = NULL;
    }

    return ret;
}
/*===========================================================================
 * FUNCTION   :sendWaitFrameSingle
 *
 * DESCRIPTION: send single
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

void SprdCamera3Multi::sendWaitFrameSingle() {
    Mutex::Autolock l(mWaitFrameLock);
    mFirstFrameCount++;
    if (mFirstFrameCount == m_nPhyCameras) {
        mWaitFrameSignal.signal();
        HAL_LOGD("wake up first sync");
    }
}
/*===========================================================================
 * FUNCTION   :processCaptureRequest
 *
 * DESCRIPTION: processCaptureRequest
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {

    int rc = 0;
    int cameraMNIndex = 0;
    Mutex::Autolock l(mMultiLock);
    rc = validateCaptureRequest(request);
    if (rc != NO_ERROR) {
        return rc;
    }
    CameraMetadata metaSettings[MAX_MULTI_NUM_CAMERA];
    mCurFrameNum = request->frame_number;
    int req_stream_mak[MAX_MULTI_NUM_CAMERA];
    camera3_capture_request_t newReq[MAX_MULTI_NUM_CAMERA];
    camera3_stream_buffer_t
        out_buffer[MAX_MULTI_NUM_CAMERA][MAX_MULTI_NUM_STREAMS];

    camera3_stream_buffer_t fw_buffer[MAX_MULTI_NUM_STREAMS + 1];
    int total_stream_mask = 0;

    bzero(&newReq, sizeof(camera3_capture_request_t) * MAX_MULTI_NUM_CAMERA);
    bzero(&req_stream_mak, sizeof(int) * MAX_MULTI_NUM_CAMERA);
    bzero(&out_buffer, sizeof(camera3_stream_buffer_t) * MAX_MULTI_NUM_CAMERA *
                           MAX_MULTI_NUM_STREAMS);
    bzero(&fw_buffer,
          sizeof(camera3_stream_buffer_t) * (MAX_MULTI_NUM_STREAMS + 1));

    // 1. config metadata for all camera

    for (int i = 0; i < MAX_MULTI_NUM_CAMERA; i++) {
        metaSettings[i] = request->settings;
    }
    reReqConfig(request, (CameraMetadata *)&metaSettings);

    // 2. get configure info from mHalReqStreamInfo;
    // map
    for (uint32_t i = 0; i < request->num_output_buffers; i++) {
        int stream_type = 0;
        hal_req_stream_config *req_stream_config = NULL;
        camera3_stream_t *newStream = (request->output_buffers[i]).stream;
        stream_type = getStreamType(newStream);
        // get config stream info
        if (stream_type == CALLBACK_STREAM)
            stream_type = PREVIEW_STREAM;
        else if (stream_type == DEFAULT_STREAM)
            stream_type = CALLBACK_STREAM;

        req_stream_config =
            findHalReq(stream_type, &mHalReqConfigStreamInfo[mReqConfigNum]);
        if (!req_stream_config) {
            continue;
        }
        cameraMNIndex = req_stream_config->mn_index;
        // get stream_mask for everyone
        for (int j = 0; j < req_stream_config->total_camera; j++) {
            int camera_index = req_stream_config->camera_index[j];
            req_stream_mak[camera_index] |=
                req_stream_config->stream_type_mask[j];
            total_stream_mask |= req_stream_config->stream_type_mask[j];
            HAL_LOGV("camera %d ,req_stream_config->stream_type_mask:%d", j,
                     req_stream_config->stream_type_mask[j]);
        }
        if (stream_type == SNAPSHOT_STREAM &&
            !(total_stream_mask &
              (SNAPSHOT_STREAM_FW_BUFFER | SNAPSHOT_STREAM_HAL_BUFFER))) {
            mRequstState = WAIT_FIRST_YUV_STATE;
            HAL_LOGD("jpeg ->callback,id=%lld", mCurFrameNum);
        } else if (stream_type == SNAPSHOT_STREAM) {

            HAL_LOGD("jpeg ->start,id=%lld", mCurFrameNum);
            mRequstState = WAIT_JPEG_STATE;
        }
    }

    // 3.  save request;
    saveRequest(request, fw_buffer, cameraMNIndex);
    HAL_LOGV("frame:id=%lld,capture id=%lld,cameraMNIndex=%d,sendF=%lld",
             mCurFrameNum, mCapFrameNum, cameraMNIndex, mSendFrameNum);

    // 5. wait untill switch finish
    if (cameraMNIndex != mMetaNotifyIndex && mMetaNotifyIndex != -1 &&
        mSendFrameNum < request->frame_number - 1 &&
        request->frame_number > 0) {
        mWaitFrameNum = request->frame_number - 1;
        HAL_LOGV("change cameraMNIndex %d to %d,mWaitFrameNum=%lld",
                 mMetaNotifyIndex, cameraMNIndex, mWaitFrameNum);
        Mutex::Autolock l(mWaitFrameLock);
        mWaitFrameSignal.waitRelative(mWaitFrameLock, WAIT_FRAME_TIMEOUT);
        HAL_LOGV("wait succeed.");
    } else if (request->frame_number == 1 && mIsSyncFirstFrame) {
        HAL_LOGV("wait first frame sync.start.");
        Mutex::Autolock l(mWaitFrameLock);
        mWaitFrameSignal.waitRelative(mWaitFrameLock, 500e6);
        HAL_LOGV("wait first frame sync. succeed.");
    }
    mMetaNotifyIndex = cameraMNIndex;
#ifndef MINICAMERA
    int ret;
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t &output = request->output_buffers[i];
        sp<Fence> acquireFence = new Fence(output.acquire_fence);

        ret = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (ret) {
            HAL_LOGE("fence wait failed %d", ret);
            goto req_fail;
        }

        acquireFence = NULL;
    }
#endif
    // 4. DO request.
    for (int i = 0; i < m_nPhyCameras; i++) {
        SprdCamera3HWI *hwi = m_pPhyCamera[i].hwi;
        int streamConfig = req_stream_mak[i];
        int buffer_num = 0;
        if (!streamConfig) {
            HAL_LOGD("no need to config camera:%d", i);
            continue;
        } else if (streamConfig == 3 || streamConfig == (3 << 2) ||
                   streamConfig == (3 << 4) || streamConfig == (3 << 6) ||
                   streamConfig == (3 << 8)) {
            HAL_LOGE("error config camera:%d,please check stream config", i);
            goto req_fail;
        }

        camera3_capture_request_t *tempReq = &newReq[i];
        camera3_stream_buffer_t *tempOutBuffer =
            (camera3_stream_buffer_t *)out_buffer[i];

        rc = createOutputBufferStream(
            tempOutBuffer, streamConfig,
            (sprdcamera_physical_descriptor_t *)&m_pPhyCamera[i], fw_buffer,
            &buffer_num);
        if (rc) {
            HAL_LOGE("failed to createOutputBufferStream, idx:%d,index=%d",
                     tempReq->frame_number, i);
            goto req_fail;
        }
        tempOutBuffer->acquire_fence = -1;
        tempOutBuffer->release_fence = -1;
        if (!buffer_num) {
            HAL_LOGI("check num_output_buffers is 0");
            continue;
        }
        HAL_LOGV("process request camera[%d] ,buffer_num %d,frame=%u", i,
                 buffer_num, tempReq->frame_number);
        memcpy(tempReq, request, sizeof(camera3_capture_request_t));
        tempReq->num_output_buffers = buffer_num;
        tempReq->settings = metaSettings[i].release();
        tempReq->output_buffers = tempOutBuffer;

        if (getStreamType(tempOutBuffer->stream) == DEFAULT_STREAM && i == 0) {
            HAL_LOGD("save main camera meta setting");
            mSavedCapReqsettings = clone_camera_metadata(tempReq->settings);
        }
        if (mIsCapturing) {
            if (mRequstState == WAIT_FIRST_YUV_STATE) {
                struct timespec t1;
                clock_gettime(CLOCK_BOOTTIME, &t1);
                uint64_t currentmainTimestamp =
                    (t1.tv_sec) * 1000000000LL + t1.tv_nsec;
                hwi->camera_ioctrl(CAMERA_IOCTRL_SET_SNAPSHOT_TIMESTAMP,
                                   &currentmainTimestamp, NULL);
            }
        }
        rc = hwi->process_capture_request(m_pPhyCamera[i].dev, tempReq);
        if (rc < 0) {
            HAL_LOGE("failed to process_capture_request, idx:%d,index=%d",
                     tempReq->frame_number, i);
            goto req_fail;
        }
        if (tempReq->settings)
            free_camera_metadata((camera_metadata_t *)(tempReq->settings));
    }

req_fail:

    return rc;
}

/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BASE();
    mMultiBase->processCaptureResultMain((camera3_capture_result_t *)result);
}
/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3Blur
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::process_capture_result_aux1(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BASE();
    mMultiBase->processCaptureResultAux1(result);
}
void SprdCamera3Multi::process_capture_result_aux2(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BASE();
    mMultiBase->processCaptureResultAux2(result);
}
void SprdCamera3Multi::process_capture_result_aux3(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_BASE();
    mMultiBase->processCaptureResultAux3(result);
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION:  main sernsor notify
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::notifyMain(const struct camera3_callback_ops *ops,
                                  const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%u", msg->message.shutter.frame_number);
    CHECK_BASE();
    mMultiBase->notifyMain(msg);
}

/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: Aux sensor	notify
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::notify_Aux1(const struct camera3_callback_ops *ops,
                                   const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%u", msg->message.shutter.frame_number);
    CHECK_BASE();
    mMultiBase->notifyAux1(msg);
}
/*===========================================================================
 * FUNCTION   :notifyAux1
 *
 * DESCRIPTION: process notify of camera index 1,2,3
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

void SprdCamera3Multi::notifyAux1(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }

    int index = findMNIndex(cur_frame_number);
    if (index == 1) {
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        mNotifyListAux1.push_back(*msg);
        if (mNotifyListAux1.size() == MAX_NOTIFY_QUEUE_SIZE) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux1.begin();
            mNotifyListAux1.erase(itor);
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :notifyAux2
 *
 * DESCRIPTION: Aux sensor	notify
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::notify_Aux2(const struct camera3_callback_ops *ops,
                                   const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_BASE();
    mMultiBase->notifyAux2(msg);
}
/*===========================================================================
 * FUNCTION   :notifyAux2
 *
 * DESCRIPTION: process notify of camera index 2
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

void SprdCamera3Multi::notifyAux2(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }

    int index = findMNIndex(cur_frame_number);
    if (index == 2) {
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        mNotifyListAux2.push_back(*msg);
        if (mNotifyListAux2.size() == MAX_NOTIFY_QUEUE_SIZE) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux2.begin();
            mNotifyListAux2.erase(itor);
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :notifyAux3
 *
 * DESCRIPTION: Aux sensor	notify
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::notify_Aux3(const struct camera3_callback_ops *ops,
                                   const camera3_notify_msg_t *msg) {
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_BASE();
    mMultiBase->notifyAux3(msg);
}
/*===========================================================================
 * FUNCTION   :notifyAux3
 *
 * DESCRIPTION: process notify of camera index 3
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

void SprdCamera3Multi::notifyAux3(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }

    int index = findMNIndex(cur_frame_number);
    if (index == 3) {
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        mNotifyListAux3.push_back(*msg);
        if (mNotifyListAux3.size() == MAX_NOTIFY_QUEUE_SIZE) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListAux3.begin();
            mNotifyListAux3.erase(itor);
        }
    }

    return;
}

/*===========================================================================
 * FUNCTION   :notifyMain
 *
 * DESCRIPTION: process notify of camera index 0
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::notifyMain(const camera3_notify_msg_t *msg) {

    uint32_t cur_frame_number = msg->message.shutter.frame_number;

    CHECK_BASE();
    if (cur_frame_number == 0) {
        sendWaitFrameSingle();
    }
    int index = findMNIndex(cur_frame_number);
    if (index == 0) {
        mCallbackOps->notify(mCallbackOps, msg);
    }
    {
        mNotifyListMain.push_back(*msg);
        if (mNotifyListMain.size() == MAX_NOTIFY_QUEUE_SIZE) {
            List<camera3_notify_msg_t>::iterator itor = mNotifyListMain.begin();
            mNotifyListMain.erase(itor);
        }
    }

    return;
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
void SprdCamera3Multi::processCaptureResultMain(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    meta_save_t metadata_t;
    int index = 0;

    HAL_LOGV("E,id=%u", cur_frame_number);

    if (result_buffer == NULL) {
        // meta process
        metadata = result->result;
        HAL_LOGV("send  meta, framenumber:%u", cur_frame_number);
        metadata_t.frame_number = cur_frame_number;
        metadata_t.metadata = clone_camera_metadata(result->result);
        Mutex::Autolock l(mMetatLock);
        index = findMNIndex(cur_frame_number);
        if (index == CAM_TYPE_MAIN)
            mMetadataList.push_back(metadata_t);
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);

    pushBufferList(mLocalBuffer, result_buffer->buffer, mLocalBufferNumber,
                   mLocalBufferList);
    CallBackResult(cur_frame_number, result_buffer->status, currStreamType,
                   CAM_TYPE_MAIN);
    HAL_LOGV("X");
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
void SprdCamera3Multi::processCaptureResultAux1(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    HAL_LOGV("E,id=%u", cur_frame_number);
    int index = 0;
    if (result_buffer == NULL) {
        meta_save_t metadata_t;
        // meta process
        metadata = result->result;
        HAL_LOGV("send  meta, framenumber:%u", cur_frame_number);
        metadata_t.frame_number = cur_frame_number;
        metadata_t.metadata = clone_camera_metadata(result->result);
        Mutex::Autolock l(mMetatLock);
        index = findMNIndex(cur_frame_number);
        if (index == CAM_TYPE_AUX1)
            mMetadataList.push_back(metadata_t);

        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGV("return buffer=%p", result_buffer->buffer);

    pushBufferList(mLocalBuffer, result_buffer->buffer, mLocalBufferNumber,
                   mLocalBufferList);

    CallBackResult(cur_frame_number, result_buffer->status, currStreamType,
                   CAM_TYPE_AUX1);

    HAL_LOGV("X");
}
/*===========================================================================
 * FUNCTION   :processCaptureResultAux2
 *
 * DESCRIPTION: process frame of camera index 2
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::processCaptureResultAux2(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    CameraMetadata metadata;
    HAL_LOGV("E,id=%u", cur_frame_number);
    int index = 0;
    if (result_buffer == NULL) {
        meta_save_t metadata_t;
        // meta process
        metadata = result->result;
        HAL_LOGV("send  meta, framenumber:%u", cur_frame_number);
        metadata_t.frame_number = cur_frame_number;
        metadata_t.metadata = clone_camera_metadata(result->result);
        Mutex::Autolock l(mMetatLock);
        index = findMNIndex(cur_frame_number);
        if (index == CAM_TYPE_AUX2)
            mMetadataList.push_back(metadata_t);

        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGV("return buffer=%p", result_buffer->buffer);

    pushBufferList(mLocalBuffer, result_buffer->buffer, mLocalBufferNumber,
                   mLocalBufferList);

    CallBackResult(cur_frame_number, result_buffer->status, currStreamType,
                   CAM_TYPE_AUX2);
}
/*===========================================================================
 * FUNCTION   :processCaptureResultAux3
 *
 * DESCRIPTION: process frame of camera index 3
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::processCaptureResultAux3(
    const camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    HAL_LOGV("E,id=%u", cur_frame_number);
    CameraMetadata metadata;
    int index = 0;
    if (result_buffer == NULL) {
        meta_save_t metadata_t;
        // meta process
        metadata = result->result;
        HAL_LOGV("send  meta, framenumber:%u", cur_frame_number);
        metadata_t.frame_number = cur_frame_number;
        metadata_t.metadata = clone_camera_metadata(result->result);
        Mutex::Autolock l(mMetatLock);
        index = findMNIndex(cur_frame_number);
        if (index == CAM_TYPE_AUX3)
            mMetadataList.push_back(metadata_t);

        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGV("return buffer=%p", result_buffer->buffer);

    pushBufferList(mLocalBuffer, result_buffer->buffer, mLocalBufferNumber,
                   mLocalBufferList);

    CallBackResult(cur_frame_number, result_buffer->status, currStreamType,
                   CAM_TYPE_AUX3);
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::dump(const struct camera3_device *device, int fd) {
    HAL_LOGV("E");
    CHECK_BASE();

    mMultiBase->_dump(device, fd);

    HAL_LOGV("X");
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::_dump(const struct camera3_device *device, int fd) {
    HAL_LOGI("E");

    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::flush(const struct camera3_device *device) {
    int rc = 0;
    HAL_LOGV(" E");
    CHECK_BASE_ERROR();
    mMultiBase->mFlushing = true;

    rc = mMultiBase->_flush(device);

    HAL_LOGV(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :flush
 *
 * DESCRIPTION: flush
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/

int SprdCamera3Multi::_flush(const struct camera3_device *device) {
    HAL_LOGI("E");

    int rc = 0;

    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[i];
        SprdCamera3HWI *hwi = sprdCam.hwi;
        rc = hwi->flush(sprdCam.dev);
        if (rc < 0) {
            HAL_LOGE("flush err !! ");
            return -1;
        }
    }
    reConfigFlush();

    HAL_LOGI("X");
    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   :CallBackMetadata
 *
 * DESCRIPTION: CallBackMetadata
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Multi::CallBackMetadata() {

    camera3_capture_result_t result;
    bzero(&result, sizeof(camera3_capture_result_t));
    List<meta_save_t>::iterator itor;
    {
        Mutex::Autolock l(mMetatLock);
        itor = mMetadataList.begin();
        while (itor != mMetadataList.end()) {
            result.frame_number = itor->frame_number;
            result.result = reConfigResultMeta(itor->metadata);
            result.num_output_buffers = 0;
            result.output_buffers = NULL;
            result.input_buffer = NULL;
            result.partial_result = 1;
            HAL_LOGV("send meta id=%d, %p", result.frame_number, result.result);
            mCallbackOps->process_capture_result(mCallbackOps, &result);
            free_camera_metadata(
                const_cast<camera_metadata_t *>(result.result));
            mMetadataList.erase(itor);
            itor++;
            {
                Mutex::Autolock l(mWaitFrameLock);
                if (result.frame_number) {
                    mSendFrameNum = result.frame_number;
                }
                if (result.frame_number == mWaitFrameNum) {
                    mWaitFrameSignal.signal();
                    HAL_LOGD("wake up request");
                }
            }
        }
    }
}
/*===========================================================================
 * FUNCTION   :CallBackResult
 *
 * DESCRIPTION: CallBackResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Multi::CallBackResult(uint32_t frame_number, int buffer_status,
                                      int stream_type, int camera_index) {

    camera3_capture_result_t result;
    List<multi_request_saved_t>::iterator itor;
    camera3_stream_buffer_t result_buffers;
    bzero(&result, sizeof(camera3_capture_result_t));
    bzero(&result_buffers, sizeof(camera3_stream_buffer_t));
    List<multi_request_saved_t> *mSavedRequestList = NULL;
    if (stream_type == CALLBACK_STREAM) {
        mSavedRequestList = &mSavedPrevRequestList;
    } else if (stream_type == DEFAULT_STREAM) {
        mSavedRequestList = &mSavedCallbackRequestList;
    } else if (stream_type == VIDEO_STREAM) {
        mSavedRequestList = &mSavedVideoRequestList;
    }

    if (stream_type != SNAPSHOT_STREAM) {
        Mutex::Autolock l(mRequestLock);
        itor = mSavedRequestList->begin();
        while (itor != mSavedRequestList->end()) {
            if (itor->frame_number == frame_number) {
                if (camera_index != itor->metaNotifyIndex) {
                    return;
                }
                result_buffers.stream = itor->stream;
                result_buffers.buffer = itor->buffer;
                mSavedRequestList->erase(itor);
                break;
            }
            itor++;
        }
        if (itor == mSavedRequestList->end()) {
            HAL_LOGV("can't find frame:%u", frame_number);
            return;
        }
    } else {
        result_buffers.stream = mSavedSnapRequest.snap_stream;
        result_buffers.buffer = mSavedSnapRequest.buffer;
    }

    HAL_LOGV(
        "send frame %u:,status.%d, result_buffers.buffer %p, camera_index %d",
        frame_number, buffer_status, &result_buffers.buffer, camera_index);
    CallBackMetadata();

    result_buffers.status = buffer_status;
    result_buffers.acquire_fence = -1;
    result_buffers.release_fence = -1;
    result.result = NULL;
    result.frame_number = frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = &result_buffers;
    result.input_buffer = NULL;
    result.partial_result = 0;
    mCallbackOps->process_capture_result(mCallbackOps, &result);
}

/*===========================================================================
 * FUNCTION   :parse_configure_info
 *
 * DESCRIPTION:  load configure file
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
void SprdCamera3Multi::parse_configure_info(config_multi_camera *config_info) {

    if (!config_info) {
        HAL_LOGE("failed to get configure info");
        return;
    }

    mMultiMode = config_info->mode;
    m_nPhyCameras = config_info->total_config_camera;
    m_VirtualCamera.id = config_info->virtual_camera_id;
    // mBufferInfo
    memcpy(&mBufferInfo, config_info->buffer_info,
           sizeof(hal_buffer_info) * MAX_MULTI_NUM_BUFFER);

    // init m_nPhyCameras
    for (int i = 0; i < m_nPhyCameras; i++) {
        config_physical_descriptor *phy_cam_info =
            (config_physical_descriptor *)&config_info->multi_phy_info[i];
        m_pPhyCamera[i].id = phy_cam_info->id;
        m_pPhyCamera[i].stream_num = phy_cam_info->config_stream_num;
        memcpy(&m_pPhyCamera[i].hal_stream, &(phy_cam_info->hal_stream),
               sizeof(hal_stream_info) * MAX_MULTI_NUM_STREAMS);
    }
    // init mHalReqConfigStreamInfo

    memcpy(&mHalReqConfigStreamInfo, &(config_info->hal_req_config_stream),
           sizeof(hal_req_stream_config_total) * MAX_MULTI_NUM_STREAMS);
}
/*===========================================================================
 * FUNCTION   :freeLocalBuffer
 *
 * DESCRIPTION:  free buffer
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

void SprdCamera3Multi::freeLocalBuffer() {
    for (int i = 0; i < mLocalBufferNumber; i++) {

        freeOneBuffer(&mLocalBuffer[i]);
    }

    mLocalBufferNumber = 0;
    mLocalBufferList.clear();
}
/*===========================================================================
 * FUNCTION   :allocateBuff
 *
 * DESCRIPTION:  allocate buffer
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

int SprdCamera3Multi::allocateBuff() {
    int ret = 0;
    freeLocalBuffer();

    for (int i = 0; i < MAX_MULTI_NUM_BUFFER; i++) {
        int width = mBufferInfo[i].width;
        int height = mBufferInfo[i].height;
        int follow_type = mBufferInfo[i].follow_type;

        if (!mBufferInfo[i].number) {
            return ret;
        }

        if (follow_type != 0) {
            int camera_index = mBufferInfo[i].follow_camera_index;
            for (int j = 0; j < m_pPhyCamera[i].stream_num; j++) {
                camera3_stream_t *find_stream =
                    findStream(follow_type, (sprdcamera_physical_descriptor_t
                                                 *)&m_pPhyCamera[camera_index]);
                CHECK_STREAM_ERROR(find_stream);
                mBufferInfo[i].width = find_stream->width;
                mBufferInfo[i].height = find_stream->height;
            }
        }

        for (int j = 0; j < mBufferInfo[i].number; j++) {
            HAL_LOGI("buffer. j=%d,num=%d,w%d,h%d", j, mBufferInfo[i].number,
                     mBufferInfo[i].width, mBufferInfo[i].height);
            if (0 > allocateOne(mBufferInfo[i].width, mBufferInfo[i].height,
                                &(mLocalBuffer[mLocalBufferNumber]), YUV420)) {
                HAL_LOGE("request one buf failed.");
                return -1;
            }
            mLocalBufferList.push_back(&(mLocalBuffer[mLocalBufferNumber]));
            mLocalBufferNumber++;
        }
    }

    return ret;
}
/*===========================================================================
 * FUNCTION   :findStream
 *
 * DESCRIPTION:  find a match stream from sprdcamera_physical_descriptor_t
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
camera3_stream_t *SprdCamera3Multi::findStream(
    int type, const sprdcamera_physical_descriptor_t *phy_cam_info) {
    camera3_stream_t *ret_stream = NULL;
    for (int i = 0; i < phy_cam_info->stream_num; i++) {
        if (phy_cam_info->hal_stream[i].type == type) {
            ret_stream = (camera3_stream_t *)&(phy_cam_info->streams[i]);
            break;
        }
    }
    if (!ret_stream) {
        HAL_LOGE("failed.find stream");
    } else {
        HAL_LOGV("width %d height %d", ret_stream->width, ret_stream->height);
    }
    return ret_stream;
}
/*===========================================================================
 * FUNCTION   :findHalReq
 *
 * DESCRIPTION:  find a match hal_req_stream_config from
 *hal_req_stream_config_total
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

hal_req_stream_config *
SprdCamera3Multi::findHalReq(int type,
                             const hal_req_stream_config_total *total_config) {
    hal_req_stream_config *hal_req = NULL;
    int i = 0;
    for (i = 0; i < total_config->total_config_stream; i++) {

        hal_req_stream_config *cur_req_config =
            (hal_req_stream_config *)&(total_config->hal_req_config[i]);

        HAL_LOGV("type %d , cur_req_config->roi_stream_type %d", type,
                 cur_req_config->roi_stream_type);

        if (cur_req_config->roi_stream_type == type) {
            hal_req = cur_req_config;
            break;
        }
    }

    if (!hal_req) {
        HAL_LOGD("Cann't find config request for stream=%d", type);
    }
    return hal_req;
}
/*===========================================================================
 * FUNCTION   :findMNIndex
 *
 * DESCRIPTION:  calculate meatadata and notify index
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
int SprdCamera3Multi::findMNIndex(uint32_t frame_number) {
    int index = -1;

    if (frame_number == mCapFrameNum) {
        if (mRequstState != REPROCESS_STATE) {
            index = mSavedSnapRequest.metaNotifyIndex;
        } else {
            index = -1;
        }
        HAL_LOGV("capture frame");

    } else {
        List<multi_request_saved_t>::iterator itor;
        Mutex::Autolock l(mRequestLock);
        itor = mSavedPrevRequestList.begin();
        while (itor != mSavedPrevRequestList.end()) {
            if (itor->frame_number == frame_number) {
                index = itor->metaNotifyIndex;
                break;
            }
            itor++;
        }
        if (itor == mSavedPrevRequestList.end()) {
            HAL_LOGV("can't find frame:%u", frame_number);
            index = -1;
        }
    }
    HAL_LOGV("MNIndex = %d", index);

    return index;
}

/*===========================================================================
 * FUNCTION   :reconfig
 *
 * DESCRIPTION:  interface for inherited class to set parameter
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

void SprdCamera3Multi::reConfigStream() {}
void SprdCamera3Multi::reConfigInit() {}
void SprdCamera3Multi::reConfigGetCameraInfo(CameraMetadata &metadata) {}
void SprdCamera3Multi::reConfigFlush() {}
void SprdCamera3Multi::reConfigOpen() {}
void SprdCamera3Multi::reConfigClose() {}
camera_metadata_t *
SprdCamera3Multi::reConfigResultMeta(camera_metadata_t *meta) {
    return meta;
}
/*===========================================================================
 * FUNCTION   :load_config_file
 *
 * DESCRIPTION:  configure file.
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/

config_multi_camera *SprdCamera3Multi::load_config_file(void) {
    HAL_LOGD("load_config_file ");
    return &multi_camera_physical_config;
}
/*===========================================================================
 * FUNCTION   :isFWBuffer
 *
 * DESCRIPTION:  verify FW Buffer
 *
 * PARAMETERS :
 *
 * RETURN	  :
 *==========================================================================*/
bool SprdCamera3Multi::isFWBuffer(buffer_handle_t *MatchBuffer) {
    Mutex::Autolock l(mRequestLock);
    List<multi_request_saved_t>::iterator itor;
    if (mSavedPrevRequestList.empty()) {
        HAL_LOGE("mSavedPrevRequestList is empty ");
        return false;
    } else {
        itor = mSavedPrevRequestList.begin();
        while (itor != mSavedPrevRequestList.end()) {
            if (MatchBuffer == (itor->buffer)) {
                return true;
            }
            itor++;
        }
        return false;
    }
}
};
