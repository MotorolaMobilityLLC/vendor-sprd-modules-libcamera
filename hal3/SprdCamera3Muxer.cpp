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
#define LOG_TAG "SprdCamera3Muxer"
//#define LOG_NDEBUG 0
#include "SprdCamera3Muxer.h"

using namespace android;
namespace sprdcamera {

SprdCamera3Muxer *mMuxer = NULL;

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

camera3_device_ops_t SprdCamera3Muxer::mCameraMuxerOps = {
    initialize:                               SprdCamera3Muxer::initialize,
    configure_streams:                        SprdCamera3Muxer::configure_streams,
    register_stream_buffers:                  NULL,
    construct_default_request_settings:       SprdCamera3Muxer::construct_default_request_settings,
    process_capture_request:                  SprdCamera3Muxer::process_capture_request,
    get_metadata_vendor_tag_ops:              NULL,
    dump:                                     SprdCamera3Muxer::dump,
    flush:                                    SprdCamera3Muxer::flush,
    reserved:{0},
};

camera3_callback_ops SprdCamera3Muxer::callback_ops_main = {
    process_capture_result:                   SprdCamera3Muxer::process_capture_result_main,
    notify:                                   SprdCamera3Muxer::notifyMain
};

camera3_callback_ops SprdCamera3Muxer::callback_ops_aux = {
    process_capture_result:                   SprdCamera3Muxer::process_capture_result_aux,
    notify:                                   SprdCamera3Muxer::notifyAux
};

/*===========================================================================
 * FUNCTION         : SprdCamera3Muxer
 *
 * DESCRIPTION     : SprdCamera3Muxer Constructor
 *
 * PARAMETERS:
 *   @num_of_cameras  : Number of Physical Cameras on device
 *
 *==========================================================================*/
SprdCamera3Muxer::SprdCamera3Muxer()
{
    HAL_LOGV(" E");
    m_nPhyCameras = 2;//m_nPhyCameras should always be 2 with dual camera mode

    mStaticMetadata = NULL;
    mMuxerThread = new MuxerThread();
    mMuxerThread->mMaxLocalBufferNum = MAX_QEQUEST_BUF;
    mMuxerThread->mLocalBuffer = NULL;
    setupPhysicalCameras();
    mLastWidth = 0;
    mLastHeight = 0;
    HAL_LOGV("X");
}

/*===========================================================================
 * FUNCTION         : ~SprdCamera3Muxer
 *
 * DESCRIPTION     : SprdCamera3Muxer Desctructor
 *
 *==========================================================================*/
SprdCamera3Muxer::~SprdCamera3Muxer()
{
    HAL_LOGV("E");
    mMuxerThread = NULL;
    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mLastWidth = 0;
    mLastHeight = 0;
    if (m_pPhyCamera) {
        delete [] m_pPhyCamera;
        m_pPhyCamera = NULL;
     }

    HAL_LOGV("X");
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
void SprdCamera3Muxer::MuxerThread::freeLocalBuffer(new_mem_t* mLocalBuffer)
{

    mLocalBufferList.clear();
    for(size_t i = 0; i < mMaxLocalBufferNum; i++){
        if(mLocalBuffer[i].buffer != NULL){
            delete ((private_handle_t*)*(mLocalBuffer[i].buffer));
            delete mLocalBuffer[i].pHeapIon;
            *(mLocalBuffer[i].buffer) = NULL;
        }
    }
    free(mLocalBuffer);

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
void SprdCamera3Muxer::getCameraMuxer(SprdCamera3Muxer** pMuxer)
{

    *pMuxer = NULL;
        if (!mMuxer) {
             mMuxer = new SprdCamera3Muxer();
    }
    CHECK_MUXER();
    *pMuxer = mMuxer;
    HAL_LOGV("mMuxer: %p ", mMuxer);

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
int SprdCamera3Muxer::get_camera_info(__unused int camera_id, struct camera_info *info)
{

    int rc = NO_ERROR;

    HAL_LOGV("E");
    if(info) {
         rc = mMuxer->getCameraInfo(info);
    }
    HAL_LOGV("X, rc: %d", rc);

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
int SprdCamera3Muxer::camera_device_open(
        __unused const struct hw_module_t *module, const char *id,
        struct hw_device_t **hw_device)
{

    int rc = NO_ERROR;

    HAL_LOGV("id= %d",atoi(id));
    if (!id) {
         HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
     }

    rc =  mMuxer->cameraDeviceOpen(atoi(id), hw_device);
    HAL_LOGV("id= %d, rc: %d", atoi(id), rc);

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
int SprdCamera3Muxer::close_camera_device(__unused hw_device_t *hw_dev)
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
int SprdCamera3Muxer::closeCameraDevice()
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

    HAL_LOGV("X, rc: %d", rc);

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
int SprdCamera3Muxer::initialize(__unused const struct camera3_device *device,
                        const camera3_callback_ops_t *callback_ops)
{
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_MUXER_ERROR();

    rc = mMuxer->initialize(callback_ops);

    HAL_LOGV("X");
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
const camera_metadata_t * SprdCamera3Muxer::construct_default_request_settings(const struct camera3_device *device, int type)
{
    const camera_metadata_t* rc;

    HAL_LOGV("E");
    if (!mMuxer) {
        HAL_LOGE("Error getting muxer ");
        return NULL;
    }
    rc = mMuxer->constructDefaultRequestSettings(device,type);

    HAL_LOGV("X");

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
int SprdCamera3Muxer::configure_streams(const struct camera3_device *device, camera3_stream_configuration_t* stream_list)
{
    int rc=0;

    HAL_LOGV(" E");
    CHECK_MUXER_ERROR();

    rc = mMuxer->configureStreams(device,stream_list);

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
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Muxer::process_capture_request(const struct camera3_device* device, camera3_capture_request_t *request)
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
 * DESCRIPTION: deconstructor of SprdCamera3Muxer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Muxer::process_capture_result_main(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result)
{
    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_MUXER();
    mMuxer->processCaptureResultMain(result);
}
/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3Muxer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Muxer::process_capture_result_aux(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result)
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
void SprdCamera3Muxer::notifyMain(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg)
{
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
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
void SprdCamera3Muxer::notifyAux(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg)
{
    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_MUXER();
    mMuxer->notifyAux(msg);
}

/*===========================================================================
 * FUNCTION   :popRequestList
 *
 * DESCRIPTION: deconstructor of SprdCamera3Muxer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
 buffer_handle_t* SprdCamera3Muxer::popRequestList(List <buffer_handle_t*>& list)
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
 * DESCRIPTION: deconstructor of SprdCamera3Muxer
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int SprdCamera3Muxer::MuxerThread::allocateOne(int w,int h, uint32_t is_cache,new_mem_t *new_mem,const native_handle_t *& nBuf )
{

    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;
    private_handle_t *buffer;

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

    HAL_LOGV("X");

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
int SprdCamera3Muxer::validateCaptureRequest(camera3_capture_request_t *request)
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
int SprdCamera3Muxer::cameraDeviceOpen(__unused int camera_id,
        struct hw_device_t **hw_device)
{
    int rc = NO_ERROR;
    uint32_t phyId = 0;

    HAL_LOGV(" E");
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

    m_VirtualCamera.id = 0;//hardcode main camera id here
    m_VirtualCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
    m_VirtualCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
    m_VirtualCamera.dev.common.close = close_camera_device;
    m_VirtualCamera.dev.ops = &mCameraMuxerOps;
    m_VirtualCamera.dev.priv = (void*)&m_VirtualCamera;
    *hw_device = &m_VirtualCamera.dev.common;

    HAL_LOGV("X");
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
int SprdCamera3Muxer::getCameraInfo(struct camera_info *info)
{
    int rc = NO_ERROR;
    int camera_id = m_VirtualCamera.id;
    HAL_LOGV("E, camera_id = %d", camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);

    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;//CAMERA_DEVICE_API_VERSION_3_0;
    info->static_camera_characteristics = mStaticMetadata;
    HAL_LOGV("X");
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
int SprdCamera3Muxer::setupPhysicalCameras()
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
 * FUNCTION   :MuxerThread
 *
 * DESCRIPTION: constructor of MuxerThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3Muxer::MuxerThread::MuxerThread()
{
    mIommuEnabled = 0;
    mLocalBufferList.clear();
    mOldVideoRequestList.clear();
    mMuxerMsgList.clear();
    mGpuApi = (GPUAPI_t*)malloc(sizeof(GPUAPI_t));
    if(mGpuApi == NULL){
        HAL_LOGE("mGpuApi malloc failed.");
    }
    memset(mGpuApi, 0 ,sizeof(GPUAPI_t));

    if(loadGpuApi() < 0){
        HAL_LOGE("load gpu api failed.");
    }

    HAL_LOGV("X");
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
SprdCamera3Muxer::MuxerThread::~MuxerThread()
{
    HAL_LOGV(" E");

    mLocalBufferList.clear();
    mOldVideoRequestList.clear();
    mMuxerMsgList.clear();
    if(mGpuApi != NULL){
        unLoadGpuApi();
        free(mGpuApi);
        mGpuApi = NULL;
    }
    HAL_LOGV("X");
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
int SprdCamera3Muxer::MuxerThread::loadGpuApi()
{
    HAL_LOGV(" E");
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

    HAL_LOGV("load Gpu Api succuss.");

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
void SprdCamera3Muxer::MuxerThread::unLoadGpuApi()
{
    HAL_LOGV("E");

    if(mGpuApi->handle!=NULL){
     dlclose(mGpuApi->handle);
     mGpuApi->handle = NULL;
    }

    HAL_LOGV("X");
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
void SprdCamera3Muxer::MuxerThread::initGpuData(int w,int h, int rotation)
{
    pt_stream_info.dst_height = h;
    pt_stream_info.dst_width  = w;
    pt_stream_info.src_height = h;
    pt_stream_info.src_width  = w;
    pt_stream_info.rot_angle = rotation;

    pt_line_buf.homography_matrix[0] = 1.009579;
    pt_line_buf.homography_matrix[1] = 0.061508;
    pt_line_buf.homography_matrix[2] = 0.0;
    pt_line_buf.homography_matrix[3] = -0.040547;
    pt_line_buf.homography_matrix[4] = 1.007715;
    pt_line_buf.homography_matrix[5] = 0.0;
    pt_line_buf.homography_matrix[6] = 0.000012;
    pt_line_buf.homography_matrix[7] = 0.000016;
    pt_line_buf.homography_matrix[8] = 1.0;
    pt_line_buf.homography_matrix[9] = 1.009010;
    pt_line_buf.homography_matrix[10] = 0.009180;
    pt_line_buf.homography_matrix[11] = 0.0;
    pt_line_buf.homography_matrix[12] = -0.016966;
    pt_line_buf.homography_matrix[13] = 0.991312;
    pt_line_buf.homography_matrix[14] = 0.0;
    pt_line_buf.homography_matrix[15] = 0.000011;
    pt_line_buf.homography_matrix[16] = -0.000015;
    pt_line_buf.homography_matrix[17] = 1.0;

    if(isInitRenderContest){
    mGpuApi->destroyRenderContext();
    isInitRenderContest = false;
    }

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

bool SprdCamera3Muxer::MuxerThread::threadLoop()
{
    buffer_handle_t* output_buffer = NULL;
    muxer_queue_msg_t muxer_msg;
    while(!mMuxerMsgList.empty()){
        List <muxer_queue_msg_t>::iterator itor1 = mMuxerMsgList.begin();
        muxer_msg = *itor1;
        mMuxerMsgList.erase(itor1);
        switch (muxer_msg.msg_type) {
        case MUXER_MSG_EXIT:
            {
                //flush queue
                HAL_LOGW("MuxerThread Stopping, mMuxerMsgList.size=%d, mOldVideoRequestList.size:%d",
                        mMuxerMsgList.size(), mOldVideoRequestList.size());
                for (List < muxer_queue_msg_t >::iterator i = mMuxerMsgList.begin(); i != mMuxerMsgList.end();)
                {
                    if(i != mMuxerMsgList.end()){
                        if(muxerTwoFrame(output_buffer, &muxer_msg.combo_frame) != NO_ERROR){
                            HAL_LOGE("Error happen when merging for frame idx:%d",muxer_msg.combo_frame.frame_number);
                            videoErrorCallback(muxer_msg.combo_frame.frame_number);
                        } else {
                            videoCallBackResult(output_buffer,&muxer_msg.combo_frame);
                        }
                        i = mMuxerMsgList.erase(i);
                    }
                }

                List < old_video_request >::iterator i = mOldVideoRequestList.begin();
                while(mOldVideoRequestList.begin() != mOldVideoRequestList.end()){
                    i = mOldVideoRequestList.begin();
                    videoErrorCallback(i->frame_number);
                }
                HAL_LOGW("MuxerThread Stopped, mMuxerMsgList.size=%d, mOldVideoRequestList.size:%d",
                    mMuxerMsgList.size(), mOldVideoRequestList.size());

                freeLocalBuffer(mLocalBuffer);
                return false;
            }
                break;
        case MUXER_MSG_DATA_PROC:
            {
                if(muxerTwoFrame(output_buffer, &muxer_msg.combo_frame) != NO_ERROR){
                    HAL_LOGE("Error happen when merging for frame idx:%d",muxer_msg.combo_frame.frame_number);
                    videoErrorCallback(muxer_msg.combo_frame.frame_number);
                } else {
                    videoCallBackResult(output_buffer,&muxer_msg.combo_frame);
                }
            }
                break;
            default:
                break;
        }
    };

    waitMsgAvailable();

    return true;   //rc setted 0,then the thread quits,when set rc 0  //han.xu
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
void SprdCamera3Muxer::MuxerThread::requestExit()
{
    muxer_queue_msg_t muxer_msg;
    muxer_msg.msg_type = MUXER_MSG_EXIT;
    mMuxerMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();

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
int SprdCamera3Muxer::getStreamType(camera3_stream_t *new_stream )
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
 * FUNCTION   :waitMsgAvailable
 *
 * DESCRIPTION: wait util two list has data
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3Muxer::MuxerThread::waitMsgAvailable()
{
    // TODO:what to do for timeout
    while(mMuxerMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, THREAD_TIMEOUT);
    }

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

bool SprdCamera3Muxer::matchTwoFrame(hwi_video_frame_buffer_info_t result1,List <hwi_video_frame_buffer_info_t> &list, hwi_video_frame_buffer_info_t* result2)
{
    List<hwi_video_frame_buffer_info_t>::iterator itor2;
    if(list.empty()) {
        HAL_LOGW("match failed for idx:%d, unmatched queue is empty", result1.frame_number);
        return MATCH_FAILED;
    } else {
        itor2=list.begin();
        while(itor2!=list.end()) {
            uint64_t tmp=result1.timestamp-itor2->timestamp;
            if(tmp<=TIME_DIFF && tmp>=-TIME_DIFF) {
                *result2=*itor2;
                list.erase(itor2);
                return MATCH_SUCCESS;
            }
            itor2++;
        }
        HAL_LOGW("match failed for idx:%d, could not find matched frame", result1.frame_number);
        return MATCH_FAILED;
    }
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
int SprdCamera3Muxer::MuxerThread::muxerTwoFrame(buffer_handle_t* &output_buf,
    matched_video_buffer_combination_t* combVideoResult)
{
    int rc = NO_ERROR;
    buffer_handle_t* input_buf1 = combVideoResult->buffer1;
    buffer_handle_t* input_buf2 = combVideoResult->buffer2;

    if(input_buf1 == NULL || input_buf2 == NULL){
        HAL_LOGE("Error, null buffer detected! input_buf1:%p input_buf2:%p",input_buf1, input_buf2);
        return BAD_VALUE;
    }

    List <old_video_request>::iterator itor=mOldVideoRequestList.begin();
    while(itor!=mOldVideoRequestList.end()) {
        if((itor)->frame_number == combVideoResult->frame_number) {
            break;
        }
        itor++;
    }

    //if no matching request found, return buffers
    if(itor == mOldVideoRequestList.end()){
        mLocalBufferList.push_back(input_buf1);
        mLocalBufferList.push_back(input_buf2);
       return UNKNOWN_ERROR;
    } else {
        output_buf = itor->buffer;
        if(output_buf == NULL)
            return BAD_VALUE;
    }

    if(!isInitRenderContest){
            if(CONTEXT_FAIL == mGpuApi->initRenderContext(&pt_stream_info, pt_line_buf.homography_matrix, 18)) {
            HAL_LOGE("initRenderContext fail");
            return UNKNOWN_ERROR;
        }else{
        isInitRenderContest = true;
        }
    }

    dcam_info_t dcam;

    dcam.left_buf = (struct private_handle_t *)*input_buf1;
    dcam.right_buf = (struct private_handle_t *)*input_buf2;
    dcam.dst_buf = (struct private_handle_t *)*output_buf;
    dcam.rot_angle = ROT_90;

    mGpuApi->imageStitchingWithGPU(&dcam);

return rc;
}

/*===========================================================================
 * FUNCTION   :Video_CallBack_Result
 *
 * DESCRIPTION: Video_CallBack_Result
 *
 * PARAMETERS : buffer: the old request buffer from framework
        muxer_msg.combo_frame:the node of mMuxerMsgList list
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Muxer::MuxerThread::videoCallBackResult(buffer_handle_t* buffer,
    matched_video_buffer_combination_t* combVideoResult)
{

    camera3_buffer_status_t buffer_status = CAMERA3_BUFFER_STATUS_OK;
    camera3_capture_result_t result;

    camera3_stream_buffer_t *result_buffers = new camera3_stream_buffer_t[1];
    void* dest = NULL;
    void* src = NULL;

    List <old_video_request >::iterator itor=mOldVideoRequestList.begin();
    while(itor!=mOldVideoRequestList.end()) {
        if(itor->frame_number==combVideoResult->frame_number) {
            break;
        }
        itor++;
    }

    result_buffers->stream = itor->stream;
    result_buffers->buffer = buffer;

    result_buffers->status = buffer_status;

    result_buffers->acquire_fence = -1;
    result_buffers->release_fence = -1;

    result.result = NULL;
    result.frame_number = combVideoResult->frame_number;
    result.num_output_buffers = 1;
    result.output_buffers = result_buffers;
    result.input_buffer = itor->input_buffer;
    result.partial_result = 0;

    mOldVideoRequestList.erase(itor);
    dumpFps();

    HAL_LOGV(":%d", result.frame_number);
    mCallbackOps->process_capture_result(mCallbackOps,&result);

    if(combVideoResult->buffer1!=NULL) {
        mLocalBufferList.push_back(combVideoResult->buffer1);
    }
    if(combVideoResult->buffer2!=NULL) {
        mLocalBufferList.push_back(combVideoResult->buffer2);
    }

   delete[]result_buffers;

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
void SprdCamera3Muxer::MuxerThread::dumpFps()
{
    mVFrameCount++;
    int64_t now = systemTime();
    int64_t diff = now - mVLastFpsTime;
    if (diff > ms2ns(250)) {
        mVFps = (((double)(mVFrameCount - mVLastFrameCount)) *
                (double)(s2ns(1))) / (double)diff;
        HAL_LOGV("[KPI Perf]: PROFILE_3D_VIDEO_FRAMES_PER_SECOND: %.4f ", mVFps);
        mVLastFpsTime = now;
        mVLastFrameCount = mVFrameCount;
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
void SprdCamera3Muxer::dump(const struct camera3_device *device, int fd)
{
    HAL_LOGV("E");
    CHECK_MUXER();

    mMuxer->_dump(device,fd);

    HAL_LOGV("X");

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
int SprdCamera3Muxer::flush(const struct camera3_device *device)
{
    int rc=0;

    HAL_LOGV(" E");
    CHECK_MUXER_ERROR();

    rc = mMuxer->_flush(device);

    HAL_LOGV(" X");

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
int SprdCamera3Muxer::initialize(const camera3_callback_ops_t *callback_ops)
{
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGV("E");
    CHECK_HWI_ERROR(hwiMain);

    mNotifyListAux.clear();
    mNotifyListMain.clear();
    mUnmatchedFrameListMain.clear();
    mUnmatchedFrameListAux.clear();
    mLastWidth = 0;
    mLastHeight = 0;
    mVideoStreamsNum = 0;

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
    mMuxerThread->mLocalBuffer = (new_mem_t*)malloc(sizeof(new_mem_t)*mMuxerThread->mMaxLocalBufferNum);
    if (NULL == mMuxerThread->mLocalBuffer) {
        HAL_LOGE("fatal error! mLocalBuffer pointer is null.");
    }
    memset(mMuxerThread->mLocalBuffer, 0 , sizeof(new_mem_t)*mMuxerThread->mMaxLocalBufferNum);


    mMuxerThread->mCallbackOps = callback_ops;

    HAL_LOGV("X");
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
int SprdCamera3Muxer::configureStreams(const struct camera3_device *device,camera3_stream_configuration_t* stream_list)
{
    HAL_LOGV("E");

    int rc=0;
    bool is_recording = false;
    camera3_stream_t* newStream = NULL;
    camera3_stream_t* videoStream = NULL;
    camera3_stream_t* pAuxStreams[MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    struct stream_info_s stream_info;

    Mutex::Autolock l(mLock1);

    //allocate mMuxerThread->mMaxLocalBufferNum buf
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        newStream = stream_list->streams[i];
          if(getStreamType(newStream) == VIDEO_STREAM){
            videoStream = stream_list->streams[i];
            is_recording = true;
            w = videoStream->width;
            h = videoStream->height;
            if(mLastWidth!= w && mLastHeight != h){
                if(mLastWidth != 0||mLastHeight != 0){
                    mMuxerThread->freeLocalBuffer(mMuxerThread->mLocalBuffer);
                }
                    for(size_t j = 0; j < mMuxerThread->mMaxLocalBufferNum; ){
                        int tmp = mMuxerThread->allocateOne(w,h,\
                                                1,&(mMuxerThread->mLocalBuffer[j]),mMuxerThread->mNativeBuffer[j]);
                        if(tmp < 0){
                            HAL_LOGE("request one buf failed.");
                            continue;
                        }
                        mMuxerThread->mLocalBufferList.push_back(mMuxerThread->mLocalBuffer[j].buffer);
                        j++;
                    }
                }
            mLastWidth = w;
            mLastHeight = h;
            mVideoStreamsNum = i;
            }
        mAuxStreams[i] = *newStream;
        pAuxStreams[i] = &mAuxStreams[i];

        }
    mIsRecording = is_recording;
    if(!is_recording){
        if(mMuxerThread->isRunning()){
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
        SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
        rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,stream_list);
        if(rc < 0){
            HAL_LOGE("failed. configure streams!!");
        }
        return rc;
    }

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
    videoStream->max_buffers += MAX_UNMATCHED_QUEUE_SIZE;

    rc = hwiAux->configure_streams(m_pPhyCamera[CAM_TYPE_AUX].dev,&config);
    if(rc < 0){
        HAL_LOGE("failed. configure aux streams!!");
        return rc;
    }
   mMuxerThread->initGpuData(w,h,newStream->rotation);
   mMuxerThread->run(NULL);
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
const camera_metadata_t * SprdCamera3Muxer::constructDefaultRequestSettings(const struct camera3_device *device, int type)
{
    HAL_LOGV("E");
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
    HAL_LOGV("X");
    return fwk_metadata;

}
/*===========================================================================
 * FUNCTION   :saveVideoBuffer
 *
 * DESCRIPTION: save video buffer in request
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Muxer::saveVideoRequest(camera3_capture_request_t *request)
{
    HAL_LOGV("idx:%d", request->frame_number);

    size_t i = 0;
    old_video_request oldVideoRequest ;
    camera3_stream_t* newStream = NULL;
    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        if(getStreamType(newStream) == VIDEO_STREAM){
        oldVideoRequest.frame_number = request->frame_number;
        oldVideoRequest.buffer = ((request->output_buffers)[i]).buffer;
        oldVideoRequest.stream = ((request->output_buffers)[i]).stream;
        oldVideoRequest.input_buffer = request->input_buffer;

        mMuxerThread->mOldVideoRequestList.push_back(oldVideoRequest);
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
int SprdCamera3Muxer::processCaptureRequest(const struct camera3_device *device,camera3_capture_request_t *request)
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
    camera3_stream_t *video_stream = NULL;
    camera3_stream_buffer_t* out_streams_main = NULL;
    camera3_stream_buffer_t* out_streams_aux = NULL;
    int save_buffer_size = 0;
    int new_buffer_size = 0;

    for(i = 0; i < request->num_output_buffers; i++) {
        if(getStreamType((request->output_buffers[i]).stream) == VIDEO_STREAM) {
            is_recording = true;
            break;
            }
    }
    mIsRecording = is_recording;
    HAL_LOGE("mIsRecording:%b,id=%d",mIsRecording,request->frame_number);
    if(!is_recording){
        rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,request);
        if(rc < 0){
            HAL_LOGE("failed. process capture request!");
        }

        return rc;
    }

    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR){
        return rc;
    }

/*config main camera*/

    req_main = *req;
    out_streams_main = (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t)*(req_main.num_output_buffers));
    if (!out_streams_main) {
        HAL_LOGE("failed");
        return -1;
    }
    memset(out_streams_main, 0x00, (sizeof(camera3_stream_buffer_t))*(req_main.num_output_buffers));

    for(size_t i = 0; i < req->num_output_buffers; i++) {
        out_streams_main[i] = req->output_buffers[i];
        new_stream = (req->output_buffers[i]).stream;
        if(getStreamType(new_stream) == VIDEO_STREAM) {
            video_stream = new_stream;
            out_streams_main[i].buffer = popRequestList(mMuxerThread->mLocalBufferList);
            if(NULL == out_streams_main[i].buffer){
                HAL_LOGE("failed, LocalBufferList is empty!");
                return NO_MEMORY;
            }
            save_buffer_size = ((struct private_handle_t *)*((req->output_buffers[i]).buffer))->size;
            new_buffer_size = ((struct private_handle_t *)*(out_streams_main[i].buffer))->size;
            if(new_buffer_size != save_buffer_size){
                HAL_LOGV("convert video buf size mismatching");
            }
        }
    }

    req_main.output_buffers = out_streams_main;

/*config aux camera * recreate a config without preview stream */
    req_aux = *req;
    req_aux.num_output_buffers = 1;


    out_streams_aux = (camera3_stream_buffer_t *)malloc(sizeof(camera3_stream_buffer_t));
    if (!out_streams_aux) {
        HAL_LOGE("failed");
        return NO_MEMORY;
    }
    memset(out_streams_aux, 0x00, (sizeof(camera3_stream_buffer_t))*(req_aux.num_output_buffers));

    for(size_t i = 0;i < req->num_output_buffers; i++)
    {
        new_stream = (req->output_buffers[i]).stream;
        if(getStreamType(new_stream) == VIDEO_STREAM) {
            out_streams_aux[0] = req->output_buffers[i];
            out_streams_aux[0].buffer = popRequestList(mMuxerThread->mLocalBufferList);
            out_streams_aux[0].stream = &mAuxStreams[mVideoStreamsNum];
            out_streams_aux[0].acquire_fence = -1;
            if(NULL == out_streams_aux[0].buffer){
                HAL_LOGE("failed, LocalBufferList is empty!");
                return NO_MEMORY;
            }

            save_buffer_size = ((struct private_handle_t *)*((req->output_buffers[i]).buffer))->size;
            new_buffer_size = ((struct private_handle_t *)*(out_streams_aux[0].buffer))->size;
            if(new_buffer_size != save_buffer_size){
            HAL_LOGV("convert video buf size mismatching");
            }
            break;
        }
    }
    req_aux.output_buffers = out_streams_aux;
    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,&req_main);

    if(rc < 0){
        for(i = 0; i < req_main.num_output_buffers; i++) {
            if(getStreamType((req_main.output_buffers)[i].stream) == VIDEO_STREAM) {
                mMuxerThread->mLocalBufferList.push_back((req_main.output_buffers)[i].buffer);
            }
    }
        goto req_fail;
    }

    rc = hwiAux->process_capture_request(m_pPhyCamera[CAM_TYPE_AUX].dev,&req_aux);

    if(rc < 0){
        mMuxerThread->mLocalBufferList.push_back((req_aux.output_buffers)[0].buffer);
        HAL_LOGE("failed, idx:%d", req_aux.frame_number);
        goto req_fail;

    }
	saveVideoRequest(req);
    HAL_LOGE("rc, d%d", rc);

req_fail:

    if (req_aux.output_buffers != NULL) {
        free((void*)req_aux.output_buffers);
        req_aux.output_buffers = NULL;
    }

    if (req_main.output_buffers != NULL) {
        free((void*)req_main.output_buffers);
        req_main.output_buffers = NULL;
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
void SprdCamera3Muxer::notifyMain( const camera3_notify_msg_t* msg)
{
    {
        Mutex::Autolock l(mNotifyLockMain);
        mNotifyListMain.push_back(*msg);
        if(mNotifyListMain.size() == MAX_NOTIFY_QUEUE_SIZE){
            List <camera3_notify_msg_t>::iterator itor = mNotifyListMain.begin();
            for(int i = 0; i < CLEAR_NOTIFY_QUEUE; i++){
                mNotifyListMain.erase(itor++);
            }
        }
    }
    mMuxerThread->mCallbackOps->notify(mMuxerThread->mCallbackOps, msg);

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
void SprdCamera3Muxer::processCaptureResultMain( const camera3_capture_result_t *result)
{
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;

    const camera3_stream_buffer_t *result_buffer  =  result->output_buffers;
    /* Direclty pass preview buffer and meta result for Main camera */
    if((result_buffer == NULL) || (getStreamType(result_buffer->stream) != VIDEO_STREAM )){
        mMuxerThread->mCallbackOps->process_capture_result(mMuxerThread->mCallbackOps, result);
        return;
    }

    cur_frame_number = result->frame_number;
    {
        Mutex::Autolock l(mNotifyLockMain);
        for(List <camera3_notify_msg_t>::iterator i = mNotifyListMain.begin(); i != mNotifyListMain.end();i++){
            if(i->message.shutter.frame_number == cur_frame_number){
                if(i->type == CAMERA3_MSG_SHUTTER){
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                     mNotifyListMain.erase(i);
                }   else if (i->type == CAMERA3_MSG_ERROR){
                    HAL_LOGE("Return error video buffer:%d caused by error Notify status", result->frame_number);
                    mMuxerThread->videoErrorCallback(result->frame_number);
                    mMuxerThread->mLocalBufferList.push_back(result->output_buffers->buffer);
                    searchnotifyresult = NOTIFY_ERROR;
                     mNotifyListMain.erase(i);
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
        HAL_LOGE("Return error video buffer:%d caused by error Buffer status", result->frame_number);
        mMuxerThread->videoErrorCallback(result->frame_number);
        mMuxerThread->mLocalBufferList.push_back(result->output_buffers->buffer);
        return;
    }

    hwi_video_frame_buffer_info_t matched_frame;
    hwi_video_frame_buffer_info_t cur_frame;
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
                HAL_LOGV("Enqueue combo frame:%d for frame merge!", muxer_msg.combo_frame.frame_number);
                mMuxerThread->mMuxerMsgList.push_back(muxer_msg);
                mMuxerThread->mMergequeueSignal.signal();
            }
        } else {
            HAL_LOGE("Enqueue newest unmatched frame:%d for Main camera", cur_frame.frame_number);
            hwi_video_frame_buffer_info_t *discard_frame = pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListMain);

            if(discard_frame != NULL){
                HAL_LOGE("Discard oldest unmatched frame:%d for Main camera", discard_frame->frame_number);
                mMuxerThread->mLocalBufferList.push_back(discard_frame->buffer);
                mMuxerThread->videoErrorCallback(discard_frame->frame_number);

                delete discard_frame;
            }
        }
    }

    return;
}
/*===========================================================================
 * FUNCTION   :~SprdCamera3Muxer
 *
 * DESCRIPTION: deconstructor of SprdCamera3Muxer
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Muxer::notifyAux( const camera3_notify_msg_t* msg)
{
    Mutex::Autolock l(mNotifyLockAux);
    mNotifyListAux.push_back(*msg);

    if(mNotifyListAux.size() == MAX_NOTIFY_QUEUE_SIZE){
        List <camera3_notify_msg_t>::iterator itor = mNotifyListAux.begin();
        for(int i = 0; i < CLEAR_NOTIFY_QUEUE; i++){
            mNotifyListAux.erase(itor++);
        }
    }
}
/*===========================================================================
 * FUNCTION   :processCaptureResultMain
 *
 * DESCRIPTION: process Capture Result from the aux hwi
 *
 * PARAMETERS : capture result structure from hwi2
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Muxer::processCaptureResultAux( const camera3_capture_result_t *result)
{
    uint64_t result_timestamp;
    uint32_t cur_frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;

    /* Direclty pass meta result for Aux camera */
    if(result->num_output_buffers == 0){
        return;
    }
    cur_frame_number = result->frame_number;
    {
        Mutex::Autolock l(mNotifyLockAux);
        for(List <camera3_notify_msg_t>::iterator i = mNotifyListAux.begin(); i != mNotifyListAux.end();i++){
            if(i->message.shutter.frame_number == cur_frame_number){
                if(i->type == CAMERA3_MSG_SHUTTER){
                    searchnotifyresult = NOTIFY_SUCCESS;
                    result_timestamp = i->message.shutter.timestamp;
                    mNotifyListAux.erase(i);
                }   else if (i->type == CAMERA3_MSG_ERROR){
                    HAL_LOGE("Return local buffer:%d caused by error Notify status", result->frame_number);
                    searchnotifyresult = NOTIFY_ERROR;
                    mMuxerThread->mLocalBufferList.push_back(result->output_buffers->buffer);
                    mNotifyListAux.erase(i);
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
        mMuxerThread->mLocalBufferList.push_back(result->output_buffers->buffer);
        return;
    }

    hwi_video_frame_buffer_info_t matched_frame;
    hwi_video_frame_buffer_info_t cur_frame;
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
                HAL_LOGV("Enqueue combo frame:%d for frame merge!", muxer_msg.combo_frame.frame_number);
                mMuxerThread->mMuxerMsgList.push_back(muxer_msg);
                mMuxerThread->mMergequeueSignal.signal();
            }
        } else{
            HAL_LOGV("Enqueue newest unmatched frame:%d for Aux camera", cur_frame.frame_number);
            hwi_video_frame_buffer_info_t *discard_frame = pushToUnmatchedQueue(cur_frame, mUnmatchedFrameListAux);
            if(discard_frame != NULL){
                HAL_LOGE("Discard oldest unmatched frame:%d for Aux camera", discard_frame->frame_number);
                mMuxerThread->mLocalBufferList.push_back(discard_frame->buffer);
                delete discard_frame;
            }
        }
    }

    return;
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
void SprdCamera3Muxer::MuxerThread::videoErrorCallback(uint32_t frame_number)
{
    camera3_capture_result_t result;

    List <old_video_request>::iterator i = mOldVideoRequestList.begin();
    for(; i != mOldVideoRequestList.end();){
        if(i->frame_number == frame_number){
            break;
        }
        i++;
    }

    if(i == mOldVideoRequestList.end()){
        return;
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
    mCallbackOps->process_capture_result(mCallbackOps, &result);

    mOldVideoRequestList.erase(i);
    delete result_buffers;

    return;
}
/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION: deconstructor of SprdCamera3Muxer
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3Muxer::_dump(const struct camera3_device *device, int fd)
{

    HAL_LOGV(" E");


    HAL_LOGV("X");

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
void SprdCamera3Muxer::dumpImg(void* addr,int size,int frameId)
{

    HAL_LOGV(" E");
    char name[128];
    snprintf(name, sizeof(name), "/data/misc/media/%d_%d.yuv",\
                        size, frameId);

    FILE *file_fd = fopen(name, "w");

    if(file_fd == NULL) {
        HAL_LOGE("open yuv file fail!\n");
    }
    int count = fwrite(addr, 1, size, file_fd);
    if(count != size) {
        HAL_LOGE("write dst.yuv fail\n");
    }
    fclose(file_fd);

    HAL_LOGV("X");

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
int SprdCamera3Muxer::_flush(const struct camera3_device *device)
{
    int rc=0;

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);

    SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
    rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);

    if(mMuxerThread != NULL){
        if(mMuxerThread->isRunning()){
            mMuxerThread->requestExit();
        }
    }

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
hwi_video_frame_buffer_info_t* SprdCamera3Muxer::pushToUnmatchedQueue(hwi_video_frame_buffer_info_t new_buffer_info, List <hwi_video_frame_buffer_info_t> &queue)
{
    hwi_video_frame_buffer_info_t *pushout = NULL;
    if(queue.size() == MAX_UNMATCHED_QUEUE_SIZE){
        pushout = new hwi_video_frame_buffer_info_t;
        List <hwi_video_frame_buffer_info_t>::iterator i = queue.begin();
        *pushout = *i;
        queue.erase(i);

    }
    queue.push_back(new_buffer_info);

    return pushout;
}

};

