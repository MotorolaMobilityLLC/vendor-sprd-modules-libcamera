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
#define LOG_TAG "SprdCamera3StereoVideo"
//#define LOG_NDEBUG 0
#include "SprdCamera3StereoVideo.h"

using namespace android;
namespace sprdcamera {
//extern  camera_face_info   g_face_info;

SprdCamera3StereoVideo *mMuxer = NULL;

//Error Check Macros
#define CHECK_MUXER() \
    if (!mMuxer) { \
        HAL_LOGE("Error getting muxer "); \
        return; \
    } \

//Error Check Macros
#define CHECK_MUXER_ERROR() \
    if (!mMuxer) { \
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

camera3_device_ops_t SprdCamera3StereoVideo::mCameraMuxerOps = {
    initialize:                               SprdCamera3StereoVideo::initialize,
    configure_streams:                        SprdCamera3StereoVideo::configure_streams,
    register_stream_buffers:                  NULL,
    construct_default_request_settings:       SprdCamera3StereoVideo::construct_default_request_settings,
    process_capture_request:                  SprdCamera3StereoVideo::process_capture_request,
    get_metadata_vendor_tag_ops:              NULL,
    dump:                                     SprdCamera3StereoVideo::dump,
    flush:                                    SprdCamera3StereoVideo::flush,
    reserved:{0},
};

camera3_callback_ops SprdCamera3StereoVideo::callback_ops_main = {
    process_capture_result:                   SprdCamera3StereoVideo::process_capture_result_main,
    notify:                                   SprdCamera3StereoVideo::notifyMain
};

camera3_callback_ops SprdCamera3StereoVideo::callback_ops_aux = {
    process_capture_result:                   SprdCamera3StereoVideo::process_capture_result_aux,
    notify:                                   SprdCamera3StereoVideo::notifyAux
};

/*===========================================================================
 * FUNCTION         : SprdCamera3StereoVideo
 *
 * DESCRIPTION     : SprdCamera3StereoVideo Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3StereoVideo::SprdCamera3StereoVideo()
{
    HAL_LOGD(" E");
    m_nPhyCameras = 2;//m_nPhyCameras should always be 2 with dual camera mode

    mStaticMetadata = NULL;
    mMuxerThread = new MuxerThread();
    mMaxLocalBufferNum = MAX_VIDEO_QEQUEST_BUF;
    mVideoLocalBuffer = NULL;
    mPreviewLocalBuffer = NULL;
    mVideoLocalBufferList.clear();
    mOldVideoRequestList.clear();
    mPreviewLocalBufferList.clear();
    mOldPreviewRequestList.clear();
    setupPhysicalCameras();
    m_VirtualCamera.id = 1;//hardcode left front camera id here
    HAL_LOGD("X");

}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3StereoVideo
 *
 * DESCRIPTION     : SprdCamera3StereoVideo Desctructor
 *
 *==========================================================================*/
SprdCamera3StereoVideo::~SprdCamera3StereoVideo()
{
    HAL_LOGD("E");
    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mPreviewLastWidth = 0;
    mPreviewLastHeight = 0;
    mVideoLocalBufferList.clear();
    mOldVideoRequestList.clear();
    mPreviewLocalBufferList.clear();
    mOldPreviewRequestList.clear();
    if (m_pPhyCamera) {
        delete [] m_pPhyCamera;
        m_pPhyCamera = NULL;
     }
    HAL_LOGD("X");

}
/*===========================================================================
 * FUNCTION         : freeLocalBuffer
 *
 * DESCRIPTION     : free new_mem_t buffer
 *
 * PARAMETERS:
 *   @new_mem_t      : Pointer to struct new_mem_t buffer
 *
 *
 * RETURN             :  NONE
 *==========================================================================*/
void SprdCamera3StereoVideo::freeLocalBuffer(new_mem_t* LocalBuffer, List<buffer_handle_t*>& bufferList,int bufferNum)
{
    HAL_LOGD("free local buffer,bufferNum=%d",bufferNum);
    bufferList.clear();
    for(size_t i = 0; i < bufferNum; i++){
        if(LocalBuffer[i].buffer != NULL){
            delete ((private_handle_t*)*(LocalBuffer[i].buffer));
            delete LocalBuffer[i].pHeapIon;
            *(LocalBuffer[i].buffer) = NULL;
        }
    }
    free(LocalBuffer);

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
void SprdCamera3StereoVideo::getCameraMuxer(SprdCamera3StereoVideo** pMuxer)
{

    *pMuxer = NULL;
        if (!mMuxer) {
             mMuxer = new SprdCamera3StereoVideo();
    }
    CHECK_MUXER();
    *pMuxer = mMuxer;
    HAL_LOGD("mMuxer: %p ", mMuxer);

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
int SprdCamera3StereoVideo::get_camera_info(__unused int camera_id, struct camera_info *info)
{

    int rc = NO_ERROR;

    HAL_LOGD("E");
    if(info) {
         rc = mMuxer->getCameraInfo(info);
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
int SprdCamera3StereoVideo::camera_device_open(
        __unused const struct hw_module_t *module, const char *id,
        struct hw_device_t **hw_device)
{

    int rc = NO_ERROR;

    HAL_LOGD("id= %d",atoi(id));
    if (!id) {
         HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
     }

    rc =  mMuxer->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdCamera3StereoVideo::close_camera_device(__unused hw_device_t *hw_dev)
{
    if(hw_dev == NULL){
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mMuxer->closeCameraDevice();
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
int SprdCamera3StereoVideo::closeCameraDevice()
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
    mUnmatchedPreviewFrameListMain.clear();
    mUnmatchedPreviewFrameListAux.clear();
    freeLocalBuffer(mPreviewLocalBuffer,mPreviewLocalBufferList,MAX_PREVIEW_QEQUEST_BUF);
    freeLocalBuffer(mVideoLocalBuffer,mVideoLocalBufferList,MAX_VIDEO_QEQUEST_BUF);
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
int SprdCamera3StereoVideo::initialize(__unused const struct camera3_device *device,
                        const camera3_callback_ops_t *callback_ops)
{
    int rc = NO_ERROR;

    HAL_LOGD("E");
    CHECK_MUXER_ERROR();

    rc = mMuxer->initialize(callback_ops);

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
const camera_metadata_t * SprdCamera3StereoVideo::construct_default_request_settings(const struct camera3_device *device, int type)
{
    const camera_metadata_t* rc;

    HAL_LOGD("E");
    if (!mMuxer) {
        HAL_LOGE("Error getting muxer ");
        return NULL;
    }
    rc = mMuxer->constructDefaultRequestSettings(device,type);

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
int SprdCamera3StereoVideo::configure_streams(const struct camera3_device *device, camera3_stream_configuration_t* stream_list)
{
    int rc=0;

    HAL_LOGD(" E");
    CHECK_MUXER_ERROR();
    rc = mMuxer->configureStreams(device,stream_list);

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
int SprdCamera3StereoVideo::process_capture_request(const struct camera3_device* device, camera3_capture_request_t *request)
{
    int rc=0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_MUXER_ERROR();
    rc = mMuxer->processCaptureRequest(device,request);

    return rc;
}
/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoVideo
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3StereoVideo::process_capture_result_main(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result)
{
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
    mMuxer->processCaptureResultMain(result);
}
/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoVideo
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3StereoVideo::process_capture_result_aux(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result)
{
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
    mMuxer->processCaptureResultAux(result);
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
void SprdCamera3StereoVideo::notifyMain(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg)
{
    CHECK_MUXER();
    mMuxer->notifyMain(msg);
}
/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: Aux sensor  notify
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3StereoVideo::notifyAux(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg)
{
    CHECK_MUXER();
    mMuxer->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :popRequestList
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoVideo
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
 buffer_handle_t* SprdCamera3StereoVideo::popRequestList(List <buffer_handle_t*>& list)
{
    buffer_handle_t* ret = NULL;

        if(list.empty()){
        return NULL;
    }
    List<buffer_handle_t*>::iterator j = list.begin();
    ret=*j;
    list.erase(j);

    return ret;
}
/*===========================================================================
 * FUNCTION   :allocateOne
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoVideo
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3StereoVideo::allocateOne(int w,int h, uint32_t is_cache,new_mem_t *new_mem,const native_handle_t *& nBuf )
{

    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

    HAL_LOGD("E");
    mem_size = w*h*3/2 ;
    // to make it page size aligned
    //  mem_size = (mem_size + 4095U) & (~4095U);

    if (!mIommuEnabled) {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0, (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0, (1<<31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    buffer = new private_handle_t(private_handle_t::PRIV_FLAGS_USES_ION,0x130,\
                            mem_size, (unsigned char *)pHeapIon->getBase(), 0);

    buffer->share_fd = pHeapIon->getHeapID();
    buffer->format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    buffer->byte_stride = w;
    buffer->internal_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    buffer->width = w;
    buffer->height = h;
    buffer->stride = w;
    buffer->internalWidth = w;
    buffer->internalHeight = h;

    nBuf = buffer;
    new_mem->buffer = &nBuf;
    new_mem->pHeapIon = pHeapIon;

    HAL_LOGD("X");

    return result;

getpmem_fail:
   delete pHeapIon;

    return -1;
}
/*===========================================================================
 * FUNCTION   :validateCaptureRequest
 *
 * DESCRIPTION: validate request received from framework
 *
 * PARAMETERS :
 *  @request:   request received from framework
 *
 * RETURN     :
 *  NO_ERROR if the request is normal
 *  BAD_VALUE if the request if improper
 *==========================================================================*/
int SprdCamera3StereoVideo::validateCaptureRequest(camera3_capture_request_t *request)
{
    size_t idx = 0;
    const camera3_stream_buffer_t *b = NULL;

    /* Sanity check the request */
    if (request == NULL) {
        HAL_LOGE("NULL capture request");
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (request->num_output_buffers < 1
            || request->output_buffers == NULL) {
        HAL_LOGE("Request %d: No output buffers provided!", frameNumber);
        return BAD_VALUE;
    }

    if (request->input_buffer != NULL) {
        b = request->input_buffer;
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %d: NULL buffer handle!", frameNumber, idx);
            return BAD_VALUE;
        }
    }

    // Validate all output buffers
    b = request->output_buffers;
    do {
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            HAL_LOGE("Request %d: Buffer %d: Status not OK!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            HAL_LOGE("Request %d: Buffer %d: Has a release fence!", frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            HAL_LOGE("Request %d: Buffer %d: NULL buffer handle!", frameNumber, idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (size_t) request->num_output_buffers);

    return NO_ERROR;
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
int SprdCamera3StereoVideo::cameraDeviceOpen(__unused int camera_id,
        struct hw_device_t **hw_device)
{
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGD(" E");
    hw_device_t *hw_dev[m_nPhyCameras];
    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        SprdCamera3HWI *hw = new SprdCamera3HWI((uint32_t)phyId);
            if (!hw) {
                HAL_LOGE("Allocation of hardware interface failed");
                return NO_MEMORY;
            }
        hw_dev[i] = NULL;

        hw->setMultiCameraMode((multiCameraMode)camera_id);
        rc = hw->openCamera(&hw_dev[i]);
            if (rc != NO_ERROR) {
                HAL_LOGE("failed, camera id:%d", phyId);
                delete hw;
                closeCameraDevice();
                return rc;
            }

        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t*>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hw;
    }

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
 *   @camera_id : camera ID
 *   @info      : ptr to camera info struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int SprdCamera3StereoVideo::getCameraInfo(struct camera_info *info)
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
    HAL_LOGD("X");
    return rc;
}
/*===========================================================================
 * FUNCTION         : get3DVideoSize
 *
 * DESCRIPTION     : get3DVideoSize
 *
 *==========================================================================*/
void SprdCamera3StereoVideo::get3DVideoSize(int *pWidth, int *pHeight)
{
    SPRD_DEF_Tag sprddefInfo;
    if ( NULL == pWidth || NULL == pHeight || NULL == m_pPhyCamera )
    {
        HAL_LOGE("parameter error!");
        return;
    }
    m_pPhyCamera[CAM_TYPE_MAIN].hwi->mSetting->getSPRDDEFTag(&sprddefInfo);
    HAL_LOGD("sensor full size, w:%d, h:%d", sprddefInfo.sprd_3dcalibration_cap_size[0], \
        sprddefInfo.sprd_3dcalibration_cap_size[1]);
    *pWidth = sprddefInfo.sprd_3dcalibration_cap_size[0]>>1;
    *pHeight = sprddefInfo.sprd_3dcalibration_cap_size[1]>>1;

    HAL_LOGD("3d video size, w:%d, h:%d", *pWidth, *pHeight);
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
int SprdCamera3StereoVideo::setupPhysicalCameras()
{
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,(m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_MAIN_ID;
    m_pPhyCamera[CAM_TYPE_AUX].id = CAM_AUX_ID;

    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   :ReProcessThread
 *
 * DESCRIPTION: constructor of ReProcessThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoVideo::ReProcessThread::ReProcessThread()
{
    HAL_LOGD(" E");
    mVFrameCount = 0;
    mVLastFpsTime = 0;
    mReprocessMsgList.clear();

    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :~ReProcessThread
 *
 * DESCRIPTION: deconstructor of ReProcessThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoVideo::ReProcessThread::~ReProcessThread()
{
    HAL_LOGD(" E");

    mReprocessMsgList.clear();

    HAL_LOGD("X");
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

bool SprdCamera3StereoVideo::ReProcessThread::threadLoop()
{
    muxer_queue_msg_t muxer_msg;

    while(!mReprocessMsgList.empty()){
        List <muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMutex);
            it= mReprocessMsgList.begin();
            muxer_msg = *it;
            mReprocessMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
         case MUXER_MSG_EXIT:
            return false;
            break;
         case MUXER_MSG_DATA_PROC:
            {
                if(reProcessFrame(muxer_msg.combo_frame.buffer1,muxer_msg.combo_frame.frame_number) !=  NO_ERROR){
                    HAL_LOGD("frameid: reprocess frame failed!",muxer_msg.combo_frame.frame_number);
                }

                videoCallBackResult(&muxer_msg.combo_frame);
            }
                break;
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
void SprdCamera3StereoVideo::ReProcessThread::requestExit()
{

    Mutex::Autolock l(mMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mReprocessMsgList.push_back(muxer_msg);
    mSignal.signal();

}
/*===========================================================================
 * FUNCTION   :video_3d_convert_face_info_from_preview2video
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::ReProcessThread::video_3d_convert_face_info_from_preview2video(int *ptr_cam_face_inf,int width,int height)
{
    float zoomWidth,zoomHeight;
    uint32_t  fdWidth,fdHeight,sensorOrgW,sensorOrgH;

    sensorOrgW = mMuxer->mVideoSize.stereoVideoWidth*2;
    sensorOrgH = mMuxer->mVideoSize.stereoVideoHeight*2;
    fdWidth= ptr_cam_face_inf[2]-ptr_cam_face_inf[0];
    fdHeight= ptr_cam_face_inf[3]-ptr_cam_face_inf[1];
    if (fdWidth == 0 || fdHeight == 0){
        HAL_LOGE("parameters error.");
        return;
    }
    zoomWidth = (float)width / (float)sensorOrgW;
    zoomHeight = (float)height / (float)sensorOrgH;
    ptr_cam_face_inf[0] = ptr_cam_face_inf[0] * zoomWidth/2;
    ptr_cam_face_inf[1] = ptr_cam_face_inf[1] * zoomHeight;
    ptr_cam_face_inf[2] = ptr_cam_face_inf[2] * zoomWidth/2;
    ptr_cam_face_inf[3] = ptr_cam_face_inf[3] * zoomHeight;

    return;
}
/*===========================================================================
 * FUNCTION   :video_3d_doFaceMakeup
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::ReProcessThread::video_3d_doFaceMakeup( private_handle_t *private_handle ,int perfect_level,int *face_info)
{

    // init the parameters table. save the value until the process is restart or the device is restart.
    int tab_skinWhitenLevel[10]={0,15,25,35,45,55,65,75,85,95};
    int tab_skinCleanLevel[10]={0,25,45,50,55,60,70,80,85,95};
    struct camera_frame_type video_3d_frame = {0};
    struct camera_frame_type *frame = &video_3d_frame;
    frame->y_vir_addr =(cmr_uint)private_handle->base;
    frame->width = private_handle->width;
    frame->height = private_handle->height;

    TSRect Tsface;
    YuvFormat yuvFormat = TSFB_FMT_NV21;
    if(face_info[0] !=0 ||face_info[1] !=0 || face_info[2]!= 0 || face_info[3] != 0){
        video_3d_convert_face_info_from_preview2video(face_info, frame->width, frame->height);
        Tsface.left = face_info[0];
        Tsface.top = face_info[1];
        Tsface.right = face_info[2];
        Tsface.bottom = face_info[3];
        HAL_LOGD("FACE_BEAUTY rect:%d-%d-%d-%d",Tsface.left,Tsface.top,Tsface.right,Tsface.bottom);

        int level = perfect_level;
        int skinWhitenLevel = 0;
        int skinCleanLevel = 0;
        int level_num = 0;
        // convert the skin_level set by APP to skinWhitenLevel & skinCleanLevel according to the table saved.
        level = (level<0)?0:((level>90)?90:level);
        level_num = level/10;
        skinWhitenLevel = tab_skinWhitenLevel[level_num];
        skinCleanLevel = tab_skinCleanLevel[level_num];
        HAL_LOGD("UCAM skinWhitenLevel is %d, skinCleanLevel is %d frame->height %d frame->width %d", skinWhitenLevel, skinCleanLevel,frame->height,frame->width);

        TSMakeupData  inMakeupData, outMakeupData;
        unsigned char *yBuf = (unsigned char *)(frame->y_vir_addr);
        unsigned char *uvBuf = (unsigned char *)(frame->y_vir_addr) + frame->width*frame->height ;
        unsigned char * tmpBuf = new unsigned char[frame->width*frame->height* 3 / 2];

        inMakeupData.frameWidth = frame->width;
        inMakeupData.frameHeight = frame->height;
        inMakeupData.yBuf = yBuf;
        inMakeupData.uvBuf = uvBuf;

        outMakeupData.frameWidth = frame->width;
        outMakeupData.frameHeight = frame->height;
        outMakeupData.yBuf = tmpBuf;
        outMakeupData.uvBuf = tmpBuf + frame->width*frame->height ;

        if (frame->width > 0 && frame->height > 0 && NULL != outMakeupData.yBuf) {
            int ret_val = ts_face_beautify(&inMakeupData, &outMakeupData, skinCleanLevel, skinWhitenLevel, &Tsface, 0, yuvFormat);
            if(ret_val !=  TS_OK) {
                HAL_LOGE("UCAM ts_face_beautify ret is %d", ret_val);
            } else {
                HAL_LOGD("UCAM ts_face_beautify return OK");
                memcpy((unsigned char *)(frame->y_vir_addr), tmpBuf, frame->width * frame->height * 3 / 2);
            }
        } else {
            HAL_LOGE("No face beauty! frame size %d, %d. If size is not zero, then outMakeupData.yBuf is null!");
        }
        if(NULL != tmpBuf) {
            delete tmpBuf;
            tmpBuf = NULL;
        }
    } else {
        HAL_LOGD("Not detect face!");
    }

}

/*===========================================================================
 * FUNCTION   :reProcessFrame face makeup
 *
 * DESCRIPTION: reprocess frame
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3StereoVideo::ReProcessThread::reProcessFrame(const buffer_handle_t* frame_buffer,int cur_frameid)
{
    int rc = 0;
    int32_t perfectskinlevel = 0;
    int32_t   face_info[4];
    List <old_request >::iterator itor;

    {
        Mutex::Autolock l(mMuxer->mRequest);
        itor=mMuxer->mOldVideoRequestList.begin();
        while(itor!=mMuxer->mOldVideoRequestList.end()) {
            if(itor->frame_number == cur_frameid) {
               perfectskinlevel = itor->perfectskinlevel;
               face_info[0]= itor->g_face_info[0];
               face_info[1]= itor->g_face_info[1];
               face_info[2]= itor->g_face_info[2];
               face_info[3]= itor->g_face_info[3];
               HAL_LOGD("perfectskinlevel=%d face:sx %d sy %d ex %d ey %d", perfectskinlevel,face_info[0],face_info[1],face_info[2],face_info[3]);
               break;
            }
            itor++;
        }
    }
    private_handle_t *private_handle = (struct private_handle_t*) (*frame_buffer);
    video_3d_doFaceMakeup(private_handle,perfectskinlevel,face_info);

    return rc;
}

/*===========================================================================
 * FUNCTION   :Video_CallBack_Result
 *
 * DESCRIPTION: Video_CallBack_Result
 *
 * PARAMETERS : buffer: the old request buffer from framework
        muxer_msg.combo_frame:the node of list
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::ReProcessThread::videoCallBackResult(frame_matched_info_t* combVideoResult)
{

    camera3_buffer_status_t buffer_status = CAMERA3_BUFFER_STATUS_OK;
    camera3_capture_result_t result;
    List <old_request >::iterator itor;
    camera3_stream_buffer_t *result_buffers = new camera3_stream_buffer_t[1];

    {
        Mutex::Autolock l(mMuxer->mRequest);
        itor=mMuxer->mOldVideoRequestList.begin();
        while(itor!=mMuxer->mOldVideoRequestList.end()) {
            if(itor->frame_number == combVideoResult->frame_number) {
               itor->invalid = true;
                break;
            }
            itor++;
        }
    }
    result_buffers->stream = itor->stream;
    result_buffers->buffer = combVideoResult->buffer1;
    result_buffers->status = buffer_status;
    result_buffers->acquire_fence = -1;
    result_buffers->release_fence = -1;
    result.result = NULL;
    result.frame_number = combVideoResult->frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = result_buffers;
    result.input_buffer = itor->input_buffer;
    result.partial_result = 0;

    dumpFps();
    mMuxer->mCallbackOps->process_capture_result(mMuxer->mCallbackOps, &result);
    HAL_LOGD(":%d", result.frame_number);

   delete []result_buffers;

}
/*===========================================================================
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3StereoVideo::ReProcessThread::waitMsgAvailable()
{
    while(mReprocessMsgList.empty()) {
        Mutex::Autolock l(mMutex);
        mSignal.waitRelative(mMutex, THREAD_TIMEOUT);
    }

}
/*===========================================================================
 * FUNCTION   :dumpFps
 *
 * DESCRIPTION: dump frame rate
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::ReProcessThread::dumpFps()
{
    mVFrameCount++;
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    if (diff > ms2ns(1000)) {
        mVFps = (((double)(mVFrameCount - mVLastFrameCount)) *
                (double)(s2ns(1))) / (double)diff;
        HAL_LOGD("[KPI Perf]: PROFILE_3D_VIDEO_FRAMES_PER_SECOND: %.4f ", mVFps);
        mVLastFpsTime = now;
        mVLastFrameCount = mVFrameCount;
    }

}
/*===========================================================================
 * FUNCTION   :MuxerThread
 *
 * DESCRIPTION: constructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoVideo::MuxerThread::MuxerThread()
{
    mMuxerMsgList.clear();
    mGpuApi = (GPUAPI_t*)malloc(sizeof(GPUAPI_t));
    if(mGpuApi == NULL){
        HAL_LOGE("mGpuApi malloc failed.");
    }
    memset(mGpuApi, 0 ,sizeof(GPUAPI_t));

    if(loadGpuApi() < 0){
        HAL_LOGE("load gpu api failed.");
    }
    mReProcessThread = new ReProcessThread();
    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :~~MuxerThread
 *
 * DESCRIPTION: deconstructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3StereoVideo::MuxerThread::~MuxerThread()
{
    HAL_LOGD(" E");
    mMuxerMsgList.clear();
    if(mGpuApi != NULL){
        unLoadGpuApi();
        free(mGpuApi);
        mGpuApi = NULL;
    }
    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :loadGpuApi
 *
 * DESCRIPTION: loadGpuApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3StereoVideo::MuxerThread::loadGpuApi()
{
    HAL_LOGD(" E");
    const char* error = NULL;
    mGpuApi->handle = dlopen(LIB_GPU_PATH, RTLD_LAZY);
    if (mGpuApi->handle == NULL) {
        error = dlerror();
        HAL_LOGE("open GPU API failed.error = %s",error);
        return -1;
    }

    mGpuApi->initRenderContext = (int(*)(struct stream_info_s *,float *,int ))dlsym(mGpuApi->handle, "initRenderContext");
    if(mGpuApi->initRenderContext == NULL){
        error = dlerror();
        HAL_LOGE("sym initRenderContext failed.error = %s",error);
        return -1;
    }

    mGpuApi->imageStitchingWithGPU =(void (*)(dcam_info_t *dcam)) dlsym(mGpuApi->handle, "imageStitchingWithGPU");
    if(mGpuApi->imageStitchingWithGPU == NULL){
        error = dlerror();
        HAL_LOGE("sym imageStitchingWithGPU failed.error = %s",error);
        return -1;
    }

    mGpuApi->destroyRenderContext = (void (*)(void))dlsym(mGpuApi->handle, "destroyRenderContext");
    if(mGpuApi->destroyRenderContext == NULL){
        error = dlerror();
        HAL_LOGE("sym destroyRenderContext failed.error = %s",error);
        return -1;
    }
    isInitRenderContest = false;

    HAL_LOGD("load Gpu Api succuss.");

    return 0;
}
/*===========================================================================
 * FUNCTION   :unLoadGpuApi
 *
 * DESCRIPTION: unLoadGpuApi
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::MuxerThread::unLoadGpuApi()
{
    HAL_LOGD("E");

    if(mGpuApi->handle!=NULL){
     dlclose(mGpuApi->handle);
     mGpuApi->handle = NULL;
    }

    HAL_LOGD("X");
}
/*===========================================================================
 * FUNCTION   :initGpuData
 *
 * DESCRIPTION: init gpu data
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::MuxerThread::initGpuData()
{
    pt_stream_info.dst_height = mMuxer->mVideoSize.srcHeight;
    pt_stream_info.dst_width  = mMuxer->mVideoSize.srcWidth;
    pt_stream_info.src_height = mMuxer->mVideoSize.stereoVideoHeight;
    pt_stream_info.src_width  = mMuxer->mVideoSize.stereoVideoWidth;
    pt_stream_info.rot_angle = ROT_270;
    HAL_LOGV("src_width = %d dst_height=%d,dst_width=%d,dst_height=%d",\
        pt_stream_info.src_width ,pt_stream_info.src_height,\
        pt_stream_info.dst_width , pt_stream_info.dst_height);
    float buff[768];
    float H_left[9], H_right[9];
    FILE *fid = fopen("/productinfo/sprd_3d_calibration/calibration.data","rb");
    if(fid != NULL){
        HAL_LOGE("open calibration file success");
        fread(buff, sizeof(float),768,fid);
        fclose(fid);
        memcpy(H_left, buff+750, sizeof(float)*9);
        memcpy(H_right, buff+759, sizeof(float)*9);

        memcpy(pt_line_buf.homography_matrix, H_left, sizeof(float) * 9);
        memcpy(pt_line_buf.homography_matrix + 9, H_right, sizeof(float) * 9);
    } else{
        HAL_LOGD("can not open calibration file, using default ");
        pt_line_buf.homography_matrix[0] = 1.0;
        pt_line_buf.homography_matrix[1] = 0.0;
        pt_line_buf.homography_matrix[2] = 0.0;
        pt_line_buf.homography_matrix[3] = 0.0;
        pt_line_buf.homography_matrix[4] = 1.0;
        pt_line_buf.homography_matrix[5] = 0.0;
        pt_line_buf.homography_matrix[6] = 0.0;
        pt_line_buf.homography_matrix[7] = 0.0;
        pt_line_buf.homography_matrix[8] = 1.0;

        pt_line_buf.homography_matrix[9] = 1.0;
        pt_line_buf.homography_matrix[10] = 0.0;
        pt_line_buf.homography_matrix[11] = 0.0;
        pt_line_buf.homography_matrix[12] = 0.0;
        pt_line_buf.homography_matrix[13] = 1.0;
        pt_line_buf.homography_matrix[14] = 0.0;
        pt_line_buf.homography_matrix[15] = 0.0;
        pt_line_buf.homography_matrix[16] = 0.0;
        pt_line_buf.homography_matrix[17] = 1.0;
    }
    if(!isInitRenderContest){
        if(CONTEXT_FAIL == mGpuApi->initRenderContext(&pt_stream_info, pt_line_buf.homography_matrix, 18)) {
            HAL_LOGE("initRenderContext fail");
        }else{
            isInitRenderContest = true;
        }
    }
    HAL_LOGD("using following homography_matrix data:\n");
    HAL_LOGD("left:\t%8f  %8f  %8f    right:\t%8f  %8f  %8f", pt_line_buf.homography_matrix[0],  pt_line_buf.homography_matrix[1], pt_line_buf.homography_matrix[2], pt_line_buf.homography_matrix[9], pt_line_buf.homography_matrix[10], pt_line_buf.homography_matrix[11]);
    HAL_LOGD("\t\t%8f  %8f  %8f    \t%8f  %8f  %8f", pt_line_buf.homography_matrix[3],  pt_line_buf.homography_matrix[4], pt_line_buf.homography_matrix[5], pt_line_buf.homography_matrix[12], pt_line_buf.homography_matrix[13], pt_line_buf.homography_matrix[14]);
    HAL_LOGD("\t\t%8f  %8f  %8f    \t%8f  %8f  %8f", pt_line_buf.homography_matrix[6],  pt_line_buf.homography_matrix[7], pt_line_buf.homography_matrix[8], pt_line_buf.homography_matrix[15], pt_line_buf.homography_matrix[16], pt_line_buf.homography_matrix[17]);

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
bool SprdCamera3StereoVideo::MuxerThread::threadLoop()
{

    buffer_handle_t* output_buffer = NULL;
    muxer_queue_msg_t muxer_msg;

    if(!isInitRenderContest)
        initGpuData();

    while(!mMuxerMsgList.empty()){
        List <muxer_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mMuxerMsgList.begin();
            muxer_msg = *it;
            mMuxerMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT:
            {
                //flush queue
                HAL_LOGW("MuxerThread Stopping bef, mMuxerMsgList.size=%d, mOldVideoRequestList.size:%d",
                        mMuxerMsgList.size(), mMuxer->mOldVideoRequestList.size());

                for (List < muxer_queue_msg_t >::iterator i = mMuxerMsgList.begin(); i != mMuxerMsgList.end();)
                {
                    if(i != mMuxerMsgList.end()){
                        if(muxerTwoFrame(output_buffer, &muxer_msg.combo_frame) != NO_ERROR){
                            HAL_LOGE("Error happen when merging for frame idx:%d",muxer_msg.combo_frame.frame_number);
                            videoErrorCallback(muxer_msg.combo_frame.frame_number);
                        } else {
                                Mutex::Autolock l(mReProcessThread->mMutex);
                                muxer_queue_msg_t reProcessMsg = *i;
                                reProcessMsg.combo_frame.buffer1 = output_buffer;
                                HAL_LOGD("Enqueue combo frame:%d for frame reprocess!", reProcessMsg.combo_frame.frame_number);
                                mReProcessThread->mReprocessMsgList.push_back(reProcessMsg);
                                mReProcessThread->mSignal.signal();
                            }
                        {
                            Mutex::Autolock l(mMergequeueMutex);
                            i = mMuxerMsgList.erase(i);
                        }
                    }
                }

                for(List < old_request >::iterator i = mMuxer->mOldVideoRequestList.begin();i != mMuxer->mOldVideoRequestList.end();){
                    if(!i->invalid){
                        HAL_LOGD("flush frame_number=%d",i->frame_number);
                        videoErrorCallback(i->frame_number);
                    }
                    i++;
                };
                HAL_LOGW("MuxerThread Stopped, mMuxerMsgList.size=%d, mOldVideoRequestList.size:%d",
                    mMuxerMsgList.size(), mMuxer->mOldVideoRequestList.size());
                mGpuApi->destroyRenderContext();
                isInitRenderContest = false;
                if(mReProcessThread != NULL){
                    if(mReProcessThread->isRunning()){
                        mReProcessThread->requestExit();
                    }
                }
                return false;
            }
                break;
        case MUXER_MSG_DATA_PROC:
            {
                if(muxerTwoFrame(output_buffer, &muxer_msg.combo_frame) != NO_ERROR){
                    videoErrorCallback(muxer_msg.combo_frame.frame_number);
                } else {
                    Mutex::Autolock l(mReProcessThread->mMutex);
                    muxer_queue_msg_t reProcessMsg = muxer_msg;
                    reProcessMsg.combo_frame.buffer1 = output_buffer;
                    HAL_LOGD("Enqueue combo frame:%d for frame reprocess!", reProcessMsg.combo_frame.frame_number);
                    mReProcessThread->mReprocessMsgList.push_back(reProcessMsg);
                    mReProcessThread->mSignal.signal();
                }
            }
                break;
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
void SprdCamera3StereoVideo::MuxerThread::requestExit()
{

    Mutex::Autolock l(mMergequeueMutex);
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
}
/*===========================================================================
 * FUNCTION   :muxerTwoFrame
 *
 * DESCRIPTION: muxerTwoFrame
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3StereoVideo::MuxerThread::muxerTwoFrame(buffer_handle_t* &output_buf,
    frame_matched_info_t* combVideoResult)
{
    HAL_LOGV("E");

    int rc = NO_ERROR;
    int32_t rotation = ROT_270;
    static int32_t s_rotation = ROT_270;
    buffer_handle_t* input_buf1 = combVideoResult->buffer1;
    buffer_handle_t* input_buf2 = combVideoResult->buffer2;
    List <old_request>::iterator itor;

    if(input_buf1 == NULL || input_buf2 == NULL || !isInitRenderContest){
        HAL_LOGE("error,input_buf1:%p input_buf2:%p, isInitRenderContest:%d",
            input_buf1, input_buf2, isInitRenderContest);
        return BAD_VALUE;
    }
    {
        Mutex::Autolock l(mMuxer->mRequest);

        itor =mMuxer->mOldVideoRequestList.begin();
        while(itor!=mMuxer->mOldVideoRequestList.end()) {
            if((itor)->frame_number == combVideoResult->frame_number) {
                break;
            }
            itor++;
        }
    }
    //if no matching request found, return buffers
    if(itor == mMuxer->mOldVideoRequestList.end()){
       mMuxer->mVideoLocalBufferList.push_back(input_buf1);
       mMuxer->mVideoLocalBufferList.push_back(input_buf2);
       return UNKNOWN_ERROR;
    } else {
        output_buf = itor->buffer;
        rotation = itor->rotation;
        if(output_buf == NULL)
            return BAD_VALUE;
    }

    dcam_info_t dcam;

    dcam.left_buf = (struct private_handle_t *)*input_buf1;
    dcam.right_buf = (struct private_handle_t *)*input_buf2;
    dcam.dst_buf = (struct private_handle_t *)*output_buf;
    if(rotation>=0)
    {
            switch(rotation)
            {
            case 0:
                rotation = 3;
                break;
            case 90:
                rotation = 2;
                break;
            case 180:
                rotation = 1;
                break;
            case 270:
                rotation = 0;
                break;
            default :
                rotation = 3;
                break;
        }
        s_rotation = rotation;
    }

    dcam.rot_angle = s_rotation;
    mGpuApi->imageStitchingWithGPU(&dcam);
    {
        Mutex::Autolock l(mMuxer->mVideoLocalBufferListLock);
        if(combVideoResult->buffer1!=NULL) {
            mMuxer->mVideoLocalBufferList.push_back(combVideoResult->buffer1);
        }
        if(combVideoResult->buffer2!=NULL) {
            mMuxer->mVideoLocalBufferList.push_back(combVideoResult->buffer2);
        }
    }
    HAL_LOGV("X");

return rc;

}

/*===========================================================================
 * function   : videoErrorCallback
 *
 * DESCRIPTION: callback to fw with an ERROR result
 *
 * PARAMETERS : camera3_capture_result_t
 *                        frame_number
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::MuxerThread::videoErrorCallback(uint32_t frame_number)
{
    camera3_capture_result_t result;
    List <old_request>::iterator i;

     i = mMuxer->mOldVideoRequestList.begin();
    {
        Mutex::Autolock l(mMuxer->mRequest);
        for(; i != mMuxer->mOldVideoRequestList.end();){
            if(i->frame_number == frame_number){
                i->invalid = true;
                break;
            }
            i++;
        }
        if(i == mMuxer->mOldVideoRequestList.end()){
            return;
        }
    }
    camera3_stream_buffer_t *result_buffers = new camera3_stream_buffer_t[1];

    result_buffers->stream = i->stream;
    result_buffers->buffer = i->buffer;
    result_buffers->status = CAMERA3_BUFFER_STATUS_ERROR;
    result_buffers->acquire_fence = -1;
    result_buffers->release_fence = -1;

    result.output_buffers = result_buffers;
    result.result = NULL;
    result.frame_number = frame_number;
    result.num_output_buffers = 1;
    result.input_buffer = i->input_buffer;
    result.partial_result = 0;

    HAL_LOGD(":%d", result.frame_number);
    mMuxer->mCallbackOps->process_capture_result(mMuxer->mCallbackOps, &result);
    delete result_buffers;

    return;
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
void SprdCamera3StereoVideo::MuxerThread::waitMsgAvailable()
{
    while(mMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }

}
/*===========================================================================
 * FUNCTION   :CallBackResult
 *
 * DESCRIPTION:
 *
 * PARAMETERS : callback metadata,notify,preview stream
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::CallBackResult(frame_matched_info_t* combResult)
{
    camera3_capture_result_t result;
    camera3_stream_buffer_t result_buffer;
    List <pending_Metadata_t>::iterator j= mPendingMetadataList.begin() ;
    camera3_notify_msg_t msg ;
    result.frame_number = combResult->frame_number;
    int showPreviewDeviceId = CAM_TYPE_MAIN;
    HAL_LOGD("id%d",combResult->frame_number);
    //get show preview frame device id value
    {
        Mutex::Autolock l(mRequest);
        List <old_request>::iterator i = mOldPreviewRequestList.begin();
        while(i != mOldPreviewRequestList.end()){
            if(i->frame_number == combResult->frame_number){
                showPreviewDeviceId = i->showPreviewDeviceId;
                result_buffer.stream = i->stream;
                mOldPreviewRequestList.erase(i);
                HAL_LOGD("id%d,showpreviewid%d",i->frame_number,showPreviewDeviceId);
                break;
            }
            i++;
        }
    }
    if(combResult->buffer2 == NULL){
        showPreviewDeviceId = CAM_TYPE_MAIN;
    }
    //notify
    {
        if(showPreviewDeviceId == CAM_TYPE_MAIN){
            Mutex::Autolock l(mNotifyLockMain);
            for(List <camera3_notify_msg_t>::iterator i = mNotifyListMain.begin(); i !=  mNotifyListMain.end(); i++){
                if(i->message.shutter.frame_number == combResult->frame_number){
                    msg = *i;
                    break;
                }
            }
            }else if(showPreviewDeviceId == CAM_TYPE_AUX){
            Mutex::Autolock l(mNotifyLockAux);
            for(List <camera3_notify_msg_t>::iterator i = mNotifyListAux.begin(); i !=  mNotifyListAux.end(); i++){
                if(i->message.shutter.frame_number == combResult->frame_number){
                    msg = *i;
                    break;
                }
            }
         }
        mCallbackOps->notify(mCallbackOps, &msg);
    }
  //metadata
    {
        Mutex::Autolock l(mPendingMetadataListLock);
        if(showPreviewDeviceId == CAM_TYPE_MAIN){
        for(; j !=  mPendingMetadataList.end(); j++){
            if(j->frame_number == combResult->frame_number &&  j->CamDevice == CAM_TYPE_MAIN){
                result.result = j->metadata;
                result.input_buffer = NULL;
                result.num_output_buffers = 0;
                result.partial_result = 1;
                break;
            }
        }
         if(j ==  mPendingMetadataList.end()){
            HAL_LOGD("can't find metadata for   frame id=%d",result.frame_number);
            result.result = NULL;
            result.partial_result = 0;
         }
         }else  if(showPreviewDeviceId == CAM_TYPE_AUX){
             for(; j !=  mPendingMetadataList.end(); j++){
                 if(j->frame_number == combResult->frame_number &&  j->CamDevice == CAM_TYPE_AUX){
                  result.result = j->metadata;
                  result.partial_result = 1;
                  result.num_output_buffers = 0;
                  result.input_buffer = NULL;
                  break;
                }
             }
             if(j ==  mPendingMetadataList.end()){
                HAL_LOGD("can't find metadata for  frame id=%d",result.frame_number);
                result.result = NULL;
                result.partial_result = 0;
             }
        }
    }

    mCallbackOps->process_capture_result(mCallbackOps,&result);

    if(result.result!=NULL){
        Mutex::Autolock l(mPendingMetadataListLock);
        free_camera_metadata(j->metadata);
        mPendingMetadataList.erase(j);
        for(List <pending_Metadata_t>::iterator it= mPendingMetadataList.begin() ;
            it !=  mPendingMetadataList.end(); it++){
            if(  it->frame_number == combResult->frame_number ){
                free_camera_metadata(it->metadata);
                mPendingMetadataList.erase(it);
                break;
               }
           }
        }
    {
        if(showPreviewDeviceId == CAM_TYPE_MAIN){
            result_buffer.buffer = combResult->buffer1;
            result_buffer.status= combResult->status1;
            result_buffer.acquire_fence = -1;
            result_buffer.release_fence = -1;
            result.num_output_buffers = 1;
            result.input_buffer = NULL;
            result.result = NULL;
            result.output_buffers=&result_buffer;
            mCallbackOps->process_capture_result(mCallbackOps,&result);

            if(combResult->buffer2 != NULL){
                Mutex::Autolock l(mPreviewLocalBufferListLock);
                mPreviewLocalBufferList.push_back(combResult->buffer2);
            }
        }else if(showPreviewDeviceId == CAM_TYPE_AUX){
            result_buffer.buffer = combResult->buffer2;
            result_buffer.status= combResult->status2;
            result_buffer.acquire_fence = -1;
            result_buffer.release_fence = -1;
            result.num_output_buffers = 1;
            result.result = NULL;
            result.input_buffer = NULL;
            result.output_buffers=&result_buffer;
            mCallbackOps->process_capture_result(mCallbackOps,&result);
            if(combResult->buffer1 != NULL){
                Mutex::Autolock l(mPreviewLocalBufferListLock);
                mPreviewLocalBufferList.push_back(combResult->buffer1);
            }
        }
    }

    HAL_LOGV(" X");
}
/*===========================================================================
 * FUNCTION   :getStreamType
 *
 * DESCRIPTION: return stream type
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3StereoVideo::getStreamType(camera3_stream_t *new_stream )
{
    int stream_type = 0;
    int format = new_stream->format;
    if(new_stream->stream_type == CAMERA3_STREAM_OUTPUT)
    {
        if(format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            format =  HAL_PIXEL_FORMAT_YCrCb_420_SP;

        switch (format) {
            case HAL_PIXEL_FORMAT_YCrCb_420_SP:
                if (new_stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                    stream_type = VIDEO_STREAM;
                } else if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                    stream_type = CALLBACK_STREAM;
                } else {
                    stream_type = PREVIEW_STREAM;
                }
                break;
            case HAL_PIXEL_FORMAT_YV12:
            case HAL_PIXEL_FORMAT_YCbCr_420_888:
                if (new_stream->usage & GRALLOC_USAGE_SW_READ_OFTEN) {
                    stream_type =  DEFAULT_DEFAULT;
                }
                break;
            case HAL_PIXEL_FORMAT_BLOB:
                stream_type =  SNAPSHOT_STREAM;
                break;
            default:
                stream_type =  DEFAULT_DEFAULT;
                break;
        }
    }
    else if(new_stream->stream_type== CAMERA3_STREAM_BIDIRECTIONAL) {
        stream_type =  CALLBACK_STREAM;
    }
    return stream_type;
}
/*===========================================================================
 * FUNCTION   :matchTwoFrame
 *
 * DESCRIPTION: matchTwoFrame
 *
 * PARAMETERS : result1: success match  result.
              list_type: main camera or aux camera list
 *              result2: output data,match  result with result1.
 *
 * RETURN     : MATCH_FAILED
 *              MATCH_SUCCESS
 *==========================================================================*/
bool SprdCamera3StereoVideo::matchTwoFrame(hwi_frame_buffer_info_t result1,List <hwi_frame_buffer_info_t> &list, hwi_frame_buffer_info_t* result2)
{
    List<hwi_frame_buffer_info_t>::iterator itor2;

    if(list.empty()) {
        HAL_LOGV("match failed for idx:%d, unmatched queue is empty", result1.frame_number);
        return MATCH_FAILED;
    } else {
        itor2=list.begin();
        while(itor2!=list.end()) {
            int64_t diff = result1.timestamp-itor2->timestamp;
            if(abs(diff) < TIME_DIFF) {
                *result2 =*itor2;
                list.erase(itor2);
                HAL_LOGV("[%d:match:%d],diff=%llu T1:%llu,T2:%llu",
                    result1.frame_number,itor2->frame_number,diff,result1.timestamp,itor2->timestamp);
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        HAL_LOGV("match failed for idx:%d, could not find matched frame", result1.frame_number);
        return MATCH_FAILED;
    }
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
void SprdCamera3StereoVideo::dump(const struct camera3_device *device, int fd)
{
    HAL_LOGD("E");
    CHECK_MUXER();

    mMuxer->_dump(device,fd);

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
int SprdCamera3StereoVideo::flush(const struct camera3_device *device)
{
    int rc=0;

    HAL_LOGD(" E");
    CHECK_MUXER_ERROR();

    rc = mMuxer->_flush(device);

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
int SprdCamera3StereoVideo::initialize(const camera3_callback_ops_t *callback_ops)
{
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGD("E");
    CHECK_HWI_ERROR(hwiMain);

    mIommuEnabled = 0;
    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mVideoLastWidth = 0;
    mVideoLastHeight = 0;
    mPreviewLastWidth = 0;
    mPreviewLastHeight = 0;
    mVideoStreamsNum = 0;
    mOldPreviewRequestList.clear();
    mOldVideoRequestList.clear();
    mUnmatchedPreviewFrameListMain.clear();
    mUnmatchedPreviewFrameListAux.clear();
    mCallbackOps = callback_ops;
    mShowPreviewDeviceId = CAM_TYPE_MAIN;
    mPerfectskinlevel = 0;
    mVideoSize.srcWidth = 0;
    mVideoSize.srcHeight = 0;
    mVideoSize.stereoVideoWidth = 0;
    mVideoSize.stereoVideoHeight = 0;
    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }

    sprdCam = m_pPhyCamera[CAM_TYPE_AUX];
    SprdCamera3HWI *hwiAux = sprdCam.hwi;
    CHECK_HWI_ERROR(hwiAux);

    rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error aux camera while initialize !! ");
        return rc;
    }

    //init buffer_handle_t
    mMaxLocalBufferNum = MAX_PREVIEW_QEQUEST_BUF;
    mPreviewLocalBuffer = (new_mem_t*)malloc(sizeof(new_mem_t)*mMaxLocalBufferNum);
    if (NULL == mPreviewLocalBuffer) {
        HAL_LOGE("fatal error! mPreviewLocalBuffer pointer is null.");
    }
    memset(mPreviewLocalBuffer, 0 , sizeof(new_mem_t)*mMaxLocalBufferNum);

    mMaxLocalBufferNum = MAX_VIDEO_QEQUEST_BUF;
    mVideoLocalBuffer = (new_mem_t*)malloc(sizeof(new_mem_t)*mMaxLocalBufferNum);
    if (NULL == mVideoLocalBuffer) {
        HAL_LOGE("fatal error! mVideoLocalBuffer pointer is null.");
    }
    memset(mVideoLocalBuffer, 0 , sizeof(new_mem_t)*mMaxLocalBufferNum);


    HAL_LOGD("X");
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
int SprdCamera3StereoVideo::configureStreams(const struct camera3_device *device,camera3_stream_configuration_t* stream_list)
{
    HAL_LOGD("E");
    int rc=0;
    bool is_recording = false;
    camera3_stream_t* newStream = NULL;
    camera3_stream_t* videoStream = NULL;
    camera3_stream_t* previewStream = NULL;
    camera3_stream_t* pAuxStreams[MAX_NUM_STREAMS];
    camera3_stream_t* pMainStreams[MAX_NUM_STREAMS];
    int streamNum = 2;
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    struct stream_info_s stream_info;

    Mutex::Autolock l(mLock1);
    streamNum = stream_list->num_streams;
    if(streamNum == 3){
        (stream_list->streams[1]->max_buffers) = 1;
        for (size_t i = 0; i < stream_list->num_streams; i++) {
            pMainStreams[i] = (stream_list->streams)[i];
        }
        (stream_list->streams)[1] = pMainStreams[2];
        (stream_list->streams)[2] = pMainStreams[1];
        stream_list->num_streams = 2;
    }

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        newStream = stream_list->streams[i];
        if(getStreamType(newStream) == VIDEO_STREAM){
          is_recording = true;
        }
    }

    for (size_t i = 0; i < stream_list->num_streams; i++) {
        newStream = stream_list->streams[i];
       if(getStreamType(newStream) == PREVIEW_STREAM){
            previewStream = stream_list->streams[i];
            w = previewStream->width;
            h = previewStream->height;
            HAL_LOGV("mPreviewLastWidth=%d,w=%d",mPreviewLastWidth,w);
         if(mPreviewLastWidth!= w && mPreviewLastHeight != h){
                mMaxLocalBufferNum = MAX_PREVIEW_QEQUEST_BUF;
                if(mPreviewLastWidth != 0||mPreviewLastHeight != 0){
                    freeLocalBuffer(mPreviewLocalBuffer,mPreviewLocalBufferList,mMaxLocalBufferNum);
                }
                for(size_t j = 0; j < mMaxLocalBufferNum; ){
                    int tmp = allocateOne(w,h,\
                            1,&(mPreviewLocalBuffer[j]),mPreviewNativeBuffer[j]);
                    if(tmp < 0){
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                    mPreviewLocalBufferList.push_back(mPreviewLocalBuffer[j].buffer);
                    j++;
                }
            }
            mPreviewLastWidth = w;
            mPreviewLastHeight = h;
            mPreviewStreamsNum = i;
        }else if(getStreamType(newStream) == VIDEO_STREAM){
            videoStream = stream_list->streams[i];
            w = videoStream->width;
            h = videoStream->height;
            mMaxLocalBufferNum = MAX_VIDEO_QEQUEST_BUF;

            if(mVideoSize.srcWidth!= w && mVideoSize.srcHeight != h){
                if(mVideoSize.srcWidth != 0||mVideoSize.srcHeight != 0){
                    freeLocalBuffer(mVideoLocalBuffer,mVideoLocalBufferList,mMaxLocalBufferNum);
                }
                get3DVideoSize( &mVideoSize.stereoVideoWidth, &mVideoSize.stereoVideoHeight);
                for(size_t j = 0; j < mMaxLocalBufferNum; ){
                    int tmp = allocateOne(mVideoSize.stereoVideoWidth,mVideoSize.stereoVideoHeight,\
                            1,&(mVideoLocalBuffer[j]),mVideoNativeBuffer[j]);
                    if(tmp < 0){
                        HAL_LOGE("request one buf failed.");
                        continue;
                    }
                    mVideoLocalBufferList.push_back(mVideoLocalBuffer[j].buffer);
                    j++;
                }
            }
            mVideoSize.srcWidth = w;
            mVideoSize.srcHeight = h;
            videoStream->width = mVideoSize.stereoVideoWidth;
            videoStream->height = mVideoSize.stereoVideoHeight;
            mVideoStreamsNum = i;
            }
        mAuxStreams[i] = *newStream;
        pAuxStreams[i] = &mAuxStreams[i];
    }
    mIsRecording = is_recording;
    if(!is_recording){
        if(mMuxerThread->isRunning()){
            mOldVideoRequestList.clear();
            mMuxerThread->requestExit();
            mNotifyListAux.clear();
            mNotifyListMain.clear();
            mUnmatchedFrameListMain.clear();
            mUnmatchedFrameListAux.clear();
        }else{
            mNotifyListAux.clear();
            mNotifyListMain.clear();
            mUnmatchedFrameListMain.clear();
            mUnmatchedFrameListAux.clear();
        }
        if(streamNum == 2){
            (stream_list->streams[1]->max_buffers) = 1;
            stream_list->num_streams = 1;
        }
    }
    mPendingMetadataList.clear();
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;

    camera3_stream_configuration config;
    config = *stream_list;
    config.streams = pAuxStreams;
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,stream_list);
    if(rc < 0){
        HAL_LOGE("failed. configure main streams!!");
        return rc;
    }
    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_AUX].dev,&config);
    if(rc < 0){
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }

    previewStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
    if(is_recording){
        HAL_LOGD("in recording.");
        videoStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;
        videoStream->width = mVideoSize.srcWidth;
        videoStream->height = mVideoSize.srcHeight;
        mMuxerThread->mMuxerMsgList.clear();
        mMuxerThread->run(NULL);
        mMuxerThread->mReProcessThread->mReprocessMsgList.clear();
        mMuxerThread->mReProcessThread->run(NULL);
    }
    if(streamNum == 3){
        stream_list->num_streams = 3;
        (stream_list->streams)[1] = pMainStreams[1];
        (stream_list->streams)[2] = pMainStreams[2];
    }

   HAL_LOGD("X");

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
const camera_metadata_t * SprdCamera3StereoVideo::constructDefaultRequestSettings(const struct camera3_device *device, int type)
{
    HAL_LOGD("E");
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    Mutex::Autolock l(mLock1);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(m_pPhyCamera[CAM_TYPE_MAIN].dev,type);
    if(!fwk_metadata)
    {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    HAL_LOGD("X");
    return fwk_metadata;

}
/*===========================================================================
 * FUNCTION   :saveRequest
 *
 * DESCRIPTION: save buffer in request
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::saveRequest(camera3_capture_request_t *request,int showPreviewDeviceId,int32_t perfectskinlevel)
{

    size_t i = 0;
    camera3_stream_t* newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if(getStreamType(newStream) == CALLBACK_STREAM){
            old_request oldRequest ;
            HAL_LOGD("idx:%d save preview stream ", request->frame_number);
            Mutex::Autolock l(mRequest);
            oldRequest.frame_number = request->frame_number;
            oldRequest.showPreviewDeviceId = showPreviewDeviceId;
            oldRequest.buffer = ((request->output_buffers)[i]).buffer;
            oldRequest.stream = ((request->output_buffers)[i]).stream;
            oldRequest.input_buffer = request->input_buffer;
            mOldPreviewRequestList.push_back(oldRequest);
        }else if(getStreamType(newStream) == VIDEO_STREAM){
            Mutex::Autolock l(mRequest);
        old_request oldRequest ;
        HAL_LOGD("idx:%d save video stream ", request->frame_number);
        CameraMetadata metadata;
        metadata = request->settings;
        if (metadata.exists(ANDROID_SPRD_SENSOR_ROTATION)) {
            oldRequest.rotation = metadata.find(ANDROID_SPRD_SENSOR_ROTATION).data.i32[0];
        }
        oldRequest.frame_number = request->frame_number;
        oldRequest.buffer = ((request->output_buffers)[i]).buffer;
        oldRequest.stream = ((request->output_buffers)[i]).stream;
        oldRequest.input_buffer = request->input_buffer;
        oldRequest.perfectskinlevel = perfectskinlevel;
        oldRequest.invalid = false;
        mOldVideoRequestList.push_back(oldRequest);
        if(mOldVideoRequestList.size() == MAX_SAVE_REQUEST_QUEUE_SIZE){
            List <old_request>::iterator itor = mOldVideoRequestList.begin();
                while(itor!= mOldVideoRequestList.end()) {
                    if(itor->invalid) {
                        mOldVideoRequestList.erase(itor);
                        break;
                    }
                    itor++;
                }
            }
        }

    }
}
/*===========================================================================
 * FUNCTION   :convertRequest
 *
 * DESCRIPTION: convert Capture Request
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3StereoVideo::convertRequest(const camera3_capture_request_t *request,camera3_capture_request_t *newRequest,int type,int showPreviewDeviceId)
{
    camera3_stream_buffer_t* out_streams_main = NULL;
    camera3_stream_buffer_t* out_streams_aux = NULL;
    camera3_stream_t *new_stream = NULL;
    int num_output_buffers;

    HAL_LOGV("conver request id=%d type=%d",request->frame_number,type);
    if(type == CAM_TYPE_MAIN){
        out_streams_main = (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t)*(request->num_output_buffers));
        if (!out_streams_main) {
            HAL_LOGE("failed");
            return -1;
        }
        memset(out_streams_main, 0x00, (sizeof(camera3_stream_buffer_t))*(request->num_output_buffers));

        for(size_t i = 0; i < request->num_output_buffers; i++){
            out_streams_main[i] = request->output_buffers[i];
            new_stream = (request->output_buffers[i]).stream;
            if(getStreamType(new_stream) == CALLBACK_STREAM){
                if(showPreviewDeviceId == CAM_TYPE_AUX){
                    Mutex::Autolock l(mPreviewLocalBufferListLock);
                    out_streams_main[i].buffer = popRequestList(mPreviewLocalBufferList);
                    if(NULL == out_streams_main[i].buffer){
                        HAL_LOGE("failed, Preview LocalBufferList is empty!");
                        return NO_MEMORY;
                    }
                }
            }else if(getStreamType(new_stream) == VIDEO_STREAM){
                Mutex::Autolock l(mVideoLocalBufferListLock);
                new_stream->width = mVideoSize.stereoVideoWidth;
                new_stream->height = mVideoSize.stereoVideoHeight;
                out_streams_main[i].buffer = popRequestList(mVideoLocalBufferList);
                if(NULL == out_streams_main[i].buffer){
                    HAL_LOGE("failed, Video LocalBufferList is empty!");
                    return NO_MEMORY;
                }
            }
        }

        newRequest->output_buffers = out_streams_main;
    }else if(type == CAM_TYPE_AUX){
        num_output_buffers = request->num_output_buffers;
        if(mIsRecording){
            num_output_buffers = 1;
            newRequest->num_output_buffers = 1;
        }
        out_streams_aux = (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t)*num_output_buffers);
    if (!out_streams_aux) {
        HAL_LOGE("failed");
        return NO_MEMORY;
    }
    memset(out_streams_aux, 0x00, (sizeof(camera3_stream_buffer_t))*num_output_buffers);
    for(size_t i = 0;i < request->num_output_buffers; i++){
        new_stream = (request->output_buffers[i]).stream;
        if(getStreamType(new_stream) == CALLBACK_STREAM && (!mIsRecording)) {
            out_streams_aux[0] = request->output_buffers[i];
            if(showPreviewDeviceId == CAM_TYPE_MAIN){
                Mutex::Autolock l(mPreviewLocalBufferListLock);
                out_streams_aux[0].buffer = popRequestList(mPreviewLocalBufferList);
                if(NULL == out_streams_aux[0].buffer){
                HAL_LOGE("failed,Preview LocalBufferList is empty!");
                return NO_MEMORY;
                }
            }
            out_streams_aux[0].stream = &mAuxStreams[mPreviewStreamsNum];
            out_streams_aux[0].acquire_fence = -1;
        }else if(getStreamType(new_stream) == VIDEO_STREAM){
            Mutex::Autolock l(mVideoLocalBufferListLock);
            out_streams_aux[num_output_buffers-1] = request->output_buffers[i];
            out_streams_aux[num_output_buffers-1].buffer = popRequestList(mVideoLocalBufferList);
            out_streams_aux[num_output_buffers-1].stream = &mAuxStreams[mVideoStreamsNum];
            out_streams_aux[num_output_buffers-1].acquire_fence = -1;
            if(NULL == out_streams_aux[num_output_buffers-1].buffer){
                HAL_LOGE("failed,Video LocalBufferList is empty!");
                return NO_MEMORY;
            }
        }
    }
    newRequest->output_buffers = out_streams_aux;
    }
    return NO_ERROR;

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
void SprdCamera3StereoVideo::clearFrameNeverMatched(int whichCamera)
{
    List<hwi_frame_buffer_info_t>::iterator itor;

    if(whichCamera == CAM_TYPE_MAIN){
        Mutex::Autolock l(mMuxer->mVideoLocalBufferListLock);
        itor = mUnmatchedFrameListMain.begin();
        while(itor != mUnmatchedFrameListMain.end()) {
            mVideoLocalBufferList.push_back(itor->buffer);
            HAL_LOGD("idx:%d", itor->frame_number);
            mMuxerThread->videoErrorCallback(itor->frame_number);
            itor = mUnmatchedFrameListMain.erase(itor);
        }
    } else {
        Mutex::Autolock l(mMuxer->mVideoLocalBufferListLock);
        itor = mUnmatchedFrameListAux.begin();
        while(itor != mUnmatchedFrameListAux.end()) {
            mVideoLocalBufferList.push_back(itor->buffer);
            itor = mUnmatchedFrameListAux.erase(itor);
        }
    }
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
int SprdCamera3StereoVideo::processCaptureRequest(const struct camera3_device *device,camera3_capture_request_t *request)
{
    int rc = 0;
    uint32_t i = 0;
    bool is_recording = false;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_capture_request_t req_aux;
    camera3_stream_t *new_stream = NULL;
    CameraMetadata metadata;
    int ShowPreviewDeviceId = CAM_TYPE_MAIN;
    camera3_stream_t *video_stream = NULL;

    rc = validateCaptureRequest(req);
    if(rc != NO_ERROR){
        return rc;
    }
    metadata = request->settings;
    if(metadata.exists(ANDROID_SPRD_MULTI_CAM3_PREVIEW_ID)) {
        mShowPreviewDeviceId = metadata.find(ANDROID_SPRD_MULTI_CAM3_PREVIEW_ID).data.i32[0];
        HAL_LOGV("show preview device id:%d", mShowPreviewDeviceId);
        if(mShowPreviewDeviceId != 0 && mShowPreviewDeviceId != 1){
            mShowPreviewDeviceId = 0;
        }
    }
    if(metadata.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
         mPerfectskinlevel = metadata.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
         HAL_LOGV("perfectskinlevel=%d",mPerfectskinlevel);
    }

    for(i = 0; i < request->num_output_buffers; i++) {
        if(getStreamType((request->output_buffers[i]).stream) == VIDEO_STREAM) {
                video_stream = (request->output_buffers[i]).stream;
                is_recording = true;
                break;
            }
    }
    mIsRecording = is_recording;
    if(mIsRecording){
        mShowPreviewDeviceId = CAM_TYPE_MAIN;
    }
    HAL_LOGD("mIsRecording=%d id:%d mShowPreviewDeviceId:%d",mIsRecording,request->frame_number,mShowPreviewDeviceId);
    /* stream config main camera*/
    req_main = *req;
    rc = convertRequest(req,&req_main,CAM_TYPE_MAIN,mShowPreviewDeviceId);
    if (rc != NO_ERROR) {
        HAL_LOGE("main convertRequest failed, rc:%d", rc);
        return rc;
    }
    /*config aux camera*/
    req_aux = *req;
    convertRequest(req,&req_aux,CAM_TYPE_AUX,mShowPreviewDeviceId);
    if (rc != NO_ERROR) {
        HAL_LOGE("aux convertRequest failed, rc:%d", rc);
        return rc;
    }

    saveRequest(req,mShowPreviewDeviceId,mPerfectskinlevel);

    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,&req_main);

    if(rc < 0){
        HAL_LOGD("main failed. rc= %d", rc);
        for(i = 0; i < req_main.num_output_buffers; i++) {
            if(getStreamType((req_main.output_buffers)[i].stream) == PREVIEW_STREAM) {
                mPreviewLocalBufferList.push_back((req_main.output_buffers)[i].buffer);
            }
            if(getStreamType((req_main.output_buffers)[i].stream) == VIDEO_STREAM) {
                mVideoLocalBufferList.push_back((req_main.output_buffers)[i].buffer);
            }
        }
    goto req_fail;
    }

    rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,&req_aux);

    if(rc < 0){
            HAL_LOGD("aux failed. rc=%d", rc);
        for(i = 0; i < req_main.num_output_buffers; i++) {
            if(getStreamType((req_main.output_buffers)[i].stream) == PREVIEW_STREAM) {
                mPreviewLocalBufferList.push_back((req_main.output_buffers)[i].buffer);
            }
            if(getStreamType((req_main.output_buffers)[i].stream) == VIDEO_STREAM) {
                mVideoLocalBufferList.push_back((req_main.output_buffers)[i].buffer);
            }
        }
    }

req_fail:

    if (req_aux.output_buffers != NULL) {
        free((void*)req_aux.output_buffers);
        req_aux.output_buffers = NULL;
    }

    if (req_main.output_buffers != NULL) {
        free((void*)req_main.output_buffers);
        req_main.output_buffers = NULL;
    }

    if(is_recording){
        video_stream->width = mVideoSize.srcWidth;
        video_stream->height = mVideoSize.srcHeight;
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
void SprdCamera3StereoVideo::notifyMain( const camera3_notify_msg_t* msg)
{
    HAL_LOGV("main:id%d time=%llu",msg->message.shutter.frame_number,msg->message.shutter.timestamp);

    Mutex::Autolock l(mNotifyLockMain);
    mNotifyListMain.push_back(*msg);
    if(mNotifyListMain.size() == MAX_NOTIFY_QUEUE_SIZE){
        List <camera3_notify_msg_t>::iterator itor = mNotifyListMain.begin();
        mNotifyListMain.erase(itor);
    }

}

/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the main hwi
 *
 * PARAMETERS : capture result structure from hwi1
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::processCaptureResultMain( const camera3_capture_result_t *result)
{
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    bool isRecording = false;
    cur_frame_number = result->frame_number;
    const camera3_stream_buffer_t *result_buffer  =  result->output_buffers;

    {
        Mutex::Autolock l(mRequest);
        CameraMetadata metadata;
        metadata = result->result;
        List <old_request>::iterator i = mOldVideoRequestList.begin();
        while(i != mOldVideoRequestList.end()){
            if(i->frame_number == result->frame_number){
                if((result->result)
                    && metadata.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0] == ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD\
                    && metadata.exists(ANDROID_STATISTICS_FACE_RECTANGLES)) {
                    i->g_face_info[0] = metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[0];
                    i->g_face_info[1] = metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[1];
                    i->g_face_info[2] = metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[2];
                    i->g_face_info[3] = metadata.find(ANDROID_STATISTICS_FACE_RECTANGLES).data.i32[3];
                    HAL_LOGD("sx %d sy %d ex %d ey %d", i->g_face_info[0], i->g_face_info[1], i->g_face_info[2], i->g_face_info[3]);
                }
                isRecording = true;
                break;
            }
            i++;
        }
   }

    HAL_LOGD("id = %d mIsRecording=%d",result->frame_number,isRecording);
    if(result->output_buffers == NULL){
        //save metadata to pendingMetaList
        Mutex::Autolock l(mPendingMetadataListLock);
        pending_Metadata_t meta;
        meta.frame_number = result->frame_number;
        meta.metadata = clone_camera_metadata(result->result);
        meta.CamDevice = CAM_TYPE_MAIN;
        mPendingMetadataList.push_back(meta);
        return;
    }else if(getStreamType(result->output_buffers->stream) == CALLBACK_STREAM ){
        //save preview stream
        Mutex::Autolock l(mUnmatchedPreviewQueueLock);
        hwi_frame_buffer_info_t cur_frame;
        cur_frame.status = (result->output_buffers)->status;
        cur_frame.frame_number = cur_frame_number;
        cur_frame.buffer = (result->output_buffers)->buffer;
        if(mUnmatchedPreviewFrameListAux.size() > 0){
            List <hwi_frame_buffer_info_t>::iterator it = mUnmatchedPreviewFrameListAux.begin();
            hwi_frame_buffer_info_t matched_frame = *it;
            mUnmatchedPreviewFrameListAux.erase(it);
            frame_matched_info_t combo_frame;
            combo_frame.frame_number = cur_frame.frame_number;
            combo_frame.buffer1 = cur_frame.buffer;
            combo_frame.status1 = cur_frame.status;

            combo_frame.buffer2 = matched_frame.buffer;
            combo_frame.status2 = matched_frame.status;
            CallBackResult(&combo_frame);
        }else {
            mUnmatchedPreviewFrameListMain.push_back(cur_frame);
        }

        return;
    }

    //process video stream
    {
        Mutex::Autolock l(mNotifyLockMain);
        for(List <camera3_notify_msg_t>::iterator i = mNotifyListMain.begin(); i != mNotifyListMain.end();i++){
            if(i->message.shutter.frame_number == cur_frame_number){
                if(i->type == CAMERA3_MSG_SHUTTER){
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                }else if (i->type == CAMERA3_MSG_ERROR){
                    HAL_LOGE("Return error video buffer:%d caused by error Notify status", result->frame_number);
                    mMuxerThread->videoErrorCallback(result->frame_number);
                    {
                        Mutex::Autolock l(mVideoLocalBufferListLock);
                        mVideoLocalBufferList.push_back(result->output_buffers->buffer);
                    }
                    searchnotifyresult = NOTIFY_ERROR;
                    return;
                }
            }
        }
    }

    if(searchnotifyresult == NOTIFY_NOT_FOUND){
        //add new error management which will free local buffer
        HAL_LOGE("found no corresponding notify");
        HAL_LOGE("return when missing notify");
        return;
    }

    /* Process error buffer for Main camera:
     * 1.construct error result and return to FW
     * 2.return local buffer
     */
    if(result_buffer->status == CAMERA3_BUFFER_STATUS_ERROR){
        Mutex::Autolock l(mVideoLocalBufferListLock);
        HAL_LOGE("Return error video buffer:%d caused by error Buffer status", result->frame_number);
        mMuxerThread->videoErrorCallback(result->frame_number);
        mVideoLocalBufferList.push_back(result->output_buffers->buffer);
        return;
    }

    hwi_frame_buffer_info_t matched_frame;
    hwi_frame_buffer_info_t cur_frame;
    cur_frame.frame_number = cur_frame_number;
    cur_frame.timestamp = result_timestamp;
    cur_frame.buffer = result->output_buffers->buffer;

    {
        Mutex::Autolock l(mUnmatchedQueueLock);
        if(MATCH_SUCCESS == matchTwoFrame(cur_frame, mUnmatchedFrameListAux, &matched_frame)){
            muxer_queue_msg_t muxer_msg;
            muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
            muxer_msg.combo_frame.frame_number = cur_frame.frame_number;
            muxer_msg.combo_frame.buffer1 = cur_frame.buffer;
            muxer_msg.combo_frame.buffer2 = matched_frame.buffer;
            muxer_msg.combo_frame.input_buffer=result->input_buffer;
            muxer_msg.combo_frame.stream=result->output_buffers->stream;
            {
                Mutex::Autolock l(mMuxerThread->mMergequeueMutex);
                clearFrameNeverMatched(CAM_TYPE_MAIN);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!", muxer_msg.combo_frame.frame_number);
                mMuxerThread->mMuxerMsgList.push_back(muxer_msg);
                mMuxerThread->mMergequeueSignal.signal();
            }
        } else {
            HAL_LOGE("Enqueue newest unmatched frame:%d for Main camera", cur_frame.frame_number);
            hwi_frame_buffer_info_t *discard_frame = pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);
            if(discard_frame != NULL){
                 Mutex::Autolock l(mVideoLocalBufferListLock);
                HAL_LOGE("Discard oldest unmatched frame:%d for Main camera", discard_frame->frame_number);
                mVideoLocalBufferList.push_back(discard_frame->buffer);
                mMuxerThread->videoErrorCallback(discard_frame->frame_number);
                delete discard_frame;
            }
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :notifyAux
 *
 * DESCRIPTION: notify aux camera
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::notifyAux( const camera3_notify_msg_t* msg)
{
    HAL_LOGV("main:id%d time=%llu",msg->message.shutter.frame_number,msg->message.shutter.timestamp);

    Mutex::Autolock l(mNotifyLockAux);
    mNotifyListAux.push_back(*msg);
    if(mNotifyListAux.size() == MAX_NOTIFY_QUEUE_SIZE){
        List <camera3_notify_msg_t>::iterator itor = mNotifyListAux.begin();
            mNotifyListAux.erase(itor);
    }

}
/*===========================================================================
 * FUNCTION   :processCaptureResultAux
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi2
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::processCaptureResultAux( const camera3_capture_result_t *result)
{
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    bool isRecording = false;
    cur_frame_number = result->frame_number;

    {
        Mutex::Autolock l(mRequest);
        List <old_request>::iterator i = mOldVideoRequestList.begin();
        while(i != mOldVideoRequestList.end()){
            if(i->frame_number == result->frame_number){
                isRecording = true;
                break;
            }
            i++;
        }
   }

    HAL_LOGD("id = %d output_buffers=%p isRecording=%d ",result->frame_number,result->output_buffers,isRecording);

    if(result->output_buffers == NULL){
    //save metadata to mPendingMetadataList
        {
            Mutex::Autolock l(mPendingMetadataListLock);
            pending_Metadata_t meta;
            meta.frame_number = result->frame_number;
            meta.metadata = clone_camera_metadata(result->result);
            meta.CamDevice = CAM_TYPE_AUX;
            mPendingMetadataList.push_back(meta);
        }
        if(isRecording){
        //create invalid preview stream data ,preview stream not exit when is recording
            Mutex::Autolock l(mUnmatchedPreviewQueueLock);
            hwi_frame_buffer_info_t cur_frame;
            cur_frame.frame_number = cur_frame_number;
            cur_frame.buffer = NULL;
            cur_frame.status = CAMERA3_BUFFER_STATUS_ERROR;
            if(mUnmatchedPreviewFrameListMain.size() > 0){
                List <hwi_frame_buffer_info_t>::iterator it = mUnmatchedPreviewFrameListMain.begin();
                hwi_frame_buffer_info_t matched_frame = *it;
                mUnmatchedPreviewFrameListMain.erase(it);

                frame_matched_info_t combo_frame;
                combo_frame.frame_number = matched_frame.frame_number;
                combo_frame.buffer1 = matched_frame.buffer;
                combo_frame.status1 = matched_frame.status;
                combo_frame.buffer2 = cur_frame.buffer;
                combo_frame.status2 = cur_frame.status;
                CallBackResult(&combo_frame);
                return;
            }else{
                mUnmatchedPreviewFrameListAux.push_back(cur_frame);
                return;
            }
        }

        return;
    }else if(getStreamType(result->output_buffers->stream) == CALLBACK_STREAM ){
    //save preview stream data
        Mutex::Autolock l(mUnmatchedPreviewQueueLock);
        hwi_frame_buffer_info_t cur_frame;
        cur_frame.status = (result->output_buffers)->status;
        cur_frame.frame_number = cur_frame_number;
        cur_frame.buffer = (result->output_buffers)->buffer;
        if(mUnmatchedPreviewFrameListMain.size() > 0){
            List <hwi_frame_buffer_info_t>::iterator it = mUnmatchedPreviewFrameListMain.begin();
            hwi_frame_buffer_info_t matched_frame = *it;
            mUnmatchedPreviewFrameListMain.erase(it);

            frame_matched_info_t combo_frame;
            combo_frame.frame_number = matched_frame.frame_number;
            combo_frame.buffer1 = matched_frame.buffer;
            combo_frame.status1 = matched_frame.status;
            combo_frame.buffer2 = cur_frame.buffer;
            combo_frame.status2 = cur_frame.status;
            CallBackResult(&combo_frame);
        }else{
            mUnmatchedPreviewFrameListAux.push_back(cur_frame);
        }
        return;
    }

    //process video stream
    {
        Mutex::Autolock l(mNotifyLockAux);
        for(List <camera3_notify_msg_t>::iterator i = mNotifyListAux.begin(); i != mNotifyListAux.end();i++){
            if(i->message.shutter.frame_number == cur_frame_number){
                if(i->type == CAMERA3_MSG_SHUTTER){
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                }   else if (i->type == CAMERA3_MSG_ERROR){
                    HAL_LOGE("Return local buffer:%d caused by error Notify status", result->frame_number);
                    Mutex::Autolock l(mVideoLocalBufferListLock);
                    searchnotifyresult = NOTIFY_ERROR;
                    mVideoLocalBufferList.push_back(result->output_buffers->buffer);
                    return;
                }
            }
        }
    }

    if(searchnotifyresult == NOTIFY_NOT_FOUND){
        HAL_LOGE("found no corresponding notify");
        //add new error management
        HAL_LOGE("return when missing notify");
        return;
    }

    /* Process error buffer for Aux camera: just return local buffer*/
    if(result->output_buffers->status == CAMERA3_BUFFER_STATUS_ERROR){
        camera3_capture_result_t result_copy;
        result_copy = *result;
        HAL_LOGE("Return local buffer:%d caused by error Buffer status", result->frame_number);
        mVideoLocalBufferList.push_back(result->output_buffers->buffer);
        return;
    }

    hwi_frame_buffer_info_t matched_frame;
    hwi_frame_buffer_info_t cur_frame;
    cur_frame.frame_number = cur_frame_number;
    cur_frame.timestamp = result_timestamp;
    cur_frame.buffer = (result->output_buffers)->buffer;
    {
        Mutex::Autolock l(mUnmatchedQueueLock);
        if(MATCH_SUCCESS == matchTwoFrame(cur_frame, mUnmatchedFrameListMain, &matched_frame)){
            muxer_queue_msg_t muxer_msg;
            muxer_msg.msg_type = MUXER_MSG_DATA_PROC;
            muxer_msg.combo_frame.frame_number = matched_frame.frame_number;
            muxer_msg.combo_frame.buffer1 = matched_frame.buffer;
            muxer_msg.combo_frame.buffer2 = cur_frame.buffer;
            muxer_msg.combo_frame.input_buffer = result->input_buffer;
            {
                Mutex::Autolock l(mMuxerThread->mMergequeueMutex);
                HAL_LOGD("Enqueue combo frame:%d for frame merge!", muxer_msg.combo_frame.frame_number);
                clearFrameNeverMatched(CAM_TYPE_AUX);
                mMuxerThread->mMuxerMsgList.push_back(muxer_msg);
                mMuxerThread->mMergequeueSignal.signal();
            }
        } else{
            HAL_LOGD("Enqueue newest unmatched frame:%d for Aux camera", cur_frame.frame_number);
            hwi_frame_buffer_info_t *discard_frame = pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux);
            if(discard_frame != NULL){
                Mutex::Autolock l(mVideoLocalBufferListLock);
                mVideoLocalBufferList.push_back(discard_frame->buffer);
                delete discard_frame;
            }
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3StereoVideo
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::_dump(const struct camera3_device *device, int fd)
{

    HAL_LOGD(" E");


    HAL_LOGD("X");

}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: dump image data
 *
 * PARAMETERS : void*
 *                        int
 *                        int
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3StereoVideo::dumpImg(void* addr,int size,int frameId,int flag)
{

    HAL_LOGD(" E");

    char name[128];
    snprintf(name, sizeof(name), "/data/misc/media/%d_%d_%d.yuv",\
                        size, frameId, flag);

    FILE *file_fd = fopen(name, "w");

    if(file_fd == NULL) {
        HAL_LOGE("open yuv file failed!\n");
    }
    int count = fwrite(addr, 1, size, file_fd);
    if(count != size) {
        HAL_LOGE("write dst.yuv failed\n");
    }
    fclose(file_fd);

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
int SprdCamera3StereoVideo::_flush(const struct camera3_device *device)
{
    int rc=0;
    hwi_frame_buffer_info_t matched_frame_main;
    hwi_frame_buffer_info_t matched_frame_aux;

    HAL_LOGD("E");
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);

    HAL_LOGD(" list size =%d:%d:%d:%d",mUnmatchedPreviewFrameListMain.size(),\
            mUnmatchedPreviewFrameListAux.size(),\
            mUnmatchedFrameListAux.size(),\
            mUnmatchedFrameListMain.size());

    if(mMuxerThread != NULL){
        if(mMuxerThread->isRunning()){
            mMuxerThread->requestExit();
        }
    }

    HAL_LOGD("X");

    return rc;
}
/*===========================================================================
 * FUNCTION   :pushToUnmatchedQueue
 *
 * DESCRIPTION:
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
hwi_frame_buffer_info_t* SprdCamera3StereoVideo::pushToUnmatchedQueue(hwi_frame_buffer_info_t new_buffer_info, List <hwi_frame_buffer_info_t> &queue)
{
    hwi_frame_buffer_info_t *pushout = NULL;
    if(queue.size() == MAX_UNMATCHED_QUEUE_SIZE){
        pushout = new hwi_frame_buffer_info_t;
        List <hwi_frame_buffer_info_t>::iterator i = queue.begin();
        *pushout = *i;
        queue.erase(i);
    }
    queue.push_back(new_buffer_info);

    return pushout;
}

};

