/* Copyright (c) 2020, The Linux Foundataion. All rights reserved.
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
#define LOG_TAG "Cam3PortraitScene"
#include "SprdCamera3PortraitScene.h"
using namespace android;
namespace sprdcamera {

#define BACK_MASTER_ID 0
#define FRONT_MASTER_ID 1
#define DV_BG_W 1280
#define DV_BG_H 720

SprdCamera3PortraitScene *mPbrp = NULL;

// Error Check Macros
#define CHECK_PBRP()                                                           \
    if (!mPbrp) {                                                              \
        HAL_LOGE("Error getting portraitscene ");                              \
        return;                                                                \
    }
#define CHECK_PBRP_ERROR()                                                     \
    if (!mPbrp) {                                                              \
        HAL_LOGE("Error getting portraitscene ");                              \
        return -ENODEV;                                                        \
    }
#define CHECK_HWI_ERROR(hwi)                                                   \
    if (!hwi) {                                                                \
        HAL_LOGE("Error !! HWI not found!!");                                  \
        return -ENODEV;                                                        \
    }
#define MAP_AND_CHECK(x, y)                                                    \
    do {                                                                       \
        if (mPbrp->map(x, y) != NO_ERROR) {                                    \
            HAL_LOGE("faided to map buffer(0x%p)", x);                         \
            return -1;                                                         \
        }                                                                      \
    } while (0)

#define UNMAP_AND_SET_NULL(x, y)                                               \
    do {                                                                       \
        if (*y) {                                                              \
            mPbrp->unmap(x);                                                   \
            *((void **)y) = NULL;                                              \
        }                                                                      \
    } while (0)

#ifndef ABS
#define ABS(x) (((x) > 0) ? (x) : -(x))
#endif
#ifndef FACE_INFO_SCALER
#define FACE_SCREEN_EFFECTS_FACTOR 5
#define FACE_EFFECTS_FACTOR 5
#define FACE_INFO_SCALER(initwidth, initheight, x1, y1, x2, y2)                \
    initwidth *initheight *FACE_SCREEN_EFFECTS_FACTOR /                        \
        ((ABS(x2 - x1) * ABS(x2 - x1) + ABS(y2 - y1) * ABS(y2 - y1)) *         \
         FACE_EFFECTS_FACTOR)
#endif
#define MAX_UPDATE_TIME 3000
#define Mask_MattingW 800
#define Mask_MattingH 600
#define SAVE_MASK_W 512
#define SAVE_MASK_H 512
#define SAVE_MASK_UNIT 2
#define BG_CHANGE_TIME 3000

camera3_device_ops_t SprdCamera3PortraitScene::mCameraCaptureOps = {
    .initialize = SprdCamera3PortraitScene::initialize,
    .configure_streams = SprdCamera3PortraitScene::configure_streams,
    .register_stream_buffers = NULL,
    .construct_default_request_settings =
        SprdCamera3PortraitScene::construct_default_request_settings,
    .process_capture_request =
        SprdCamera3PortraitScene::process_capture_request,
    .get_metadata_vendor_tag_ops = NULL,
    .dump = SprdCamera3PortraitScene::dump,
    .flush = SprdCamera3PortraitScene::flush,
    .reserved = {0},
};

camera3_callback_ops SprdCamera3PortraitScene::callback_ops_main = {
    .process_capture_result =
        SprdCamera3PortraitScene::process_capture_result_main,
    .notify = SprdCamera3PortraitScene::notifyMain};

camera3_callback_ops SprdCamera3PortraitScene::callback_ops_aux = {
    .process_capture_result =
        SprdCamera3PortraitScene::process_capture_result_aux,
    .notify = SprdCamera3PortraitScene::notifyAux};

/*===========================================================================
 * FUNCTION   : SprdCamera3PortraitScene
 *
 * DESCRIPTION: SprdCamera3PortraitScene Constructor
 *
 * PARAMETERS:
 *
 *
 *==========================================================================*/
SprdCamera3PortraitScene::SprdCamera3PortraitScene() {
    HAL_LOGI(" E");
    /*common */
    memset(&m_VirtCamera, 0, sizeof(sprd_virtual_camera_t));
#ifdef CONFIG_FACE_BEAUTY
    memset(&fbLevels, 0, sizeof(faceBeautyLevels));
    memset(&fb_prev, 0, sizeof(fb_beauty_param_t));
    memset(&fb_cap, 0, sizeof(fb_beauty_param_t));
    mFaceBeautyFlag = false;
#endif
    m_VirtCamera.id = CAM_PBRP_MAIN_ID;
    mStaticMetadata = NULL;
    mCameraId = CAM_PBRP_MAIN_ID;
    mFlushing = false;
    mInitThread = false;
    m_pPhyCamera = NULL;
    m_nPhyCameras = 0;
    mReqState = PREVIEW_REQUEST_STATE;
    mCircleSizeScale = 50;
    mFaceNum = 0;
    mLastTouchX = 0;
    mLastTouchY = 0;
    mBlurBody = true;
    mUpdataTouch = false;
    mstartUpdate = 0;
    mUpdataxy = 0;
    mFirstUpdateFrame = 1;
    mFaceInfoX = 0;
    mFaceInfoY = 0;
    mOrientationAngle = 0;
    mIsRunAlgo = true;
    mBGWidth = 0;
    mBGHeight = 0;
    mIsCapBuffAllocated = false;
    mSaveTime = 0;
    mChangeBGMode = false;
    mVideoWidth = 0;
    mVideoHeight = 0;
    mIsRecordMode = false;
    memset(&mCachePrevWeightParams, 0, sizeof(sprd_portrait_scene_proc_t));
    memset(&mCacheCapWeightParams, 0, sizeof(sprd_portrait_scene_proc_t));
    memset(&mMainStreams, 0, sizeof(camera3_stream_t) * PBRP_MAX_NUM_STREAMS);
    /*capture */
    memset(&mSavedReqStreams, 0,
           sizeof(camera3_stream_t *) * PBRP_MAX_NUM_STREAMS);
    memset(mFaceInfo, 0, sizeof(int32_t) * 4);
    memset(&mLocalCapBuffer, 0, sizeof(new_mem_t) * PBRP_LOCAL_BUFF_NUM);
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mjpegSize = 0;
    mOrgJpegSize = 0;
    mMaskSaveSize = SAVE_MASK_W * SAVE_MASK_H * SAVE_MASK_UNIT;
    m_pOrgJpegBuffer = NULL;
    mCapSavedReqList.clear();
    mCapT = new CaptureThread();
    mCapPostT = new CapturePostThread();
    /*preview */
    mPrevSavedReqList.clear();
    mPrevT = new PreviewThread();
    mPrevPostT = new PreviewPostThread();
    mPreviewStreamsNum = 0;
    /*video */
    mVidSavedReqList.clear();

    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   : ~SprdCamera3PortraitScene
 *
 * DESCRIPTION: SprdCamera3PortraitScene Desctructor
 *
 *==========================================================================*/
SprdCamera3PortraitScene::~SprdCamera3PortraitScene() {
    HAL_LOGI("E");
    mCapT = NULL;
    mPrevT = NULL;
    mPrevPostT = NULL;
    if (mStaticMetadata) {
        free_camera_metadata(mStaticMetadata);
        mStaticMetadata = NULL;
    }

    if (m_pPhyCamera) {
        delete[] m_pPhyCamera;
        m_pPhyCamera = NULL;
    }
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   : getCameraPortraitScene
 *
 * DESCRIPTION: Creates Camera portraitscene if not created
 *
 * PARAMETERS:
 *   @pCapture               : Pointer to retrieve Camera portraitscene
 *
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3PortraitScene::getCameraPortraitScene(
    SprdCamera3PortraitScene **pCapture) {
    *pCapture = NULL;

    if (!mPbrp) {
        mPbrp = new SprdCamera3PortraitScene();
    }
    CHECK_PBRP();
    *pCapture = mPbrp;
    HAL_LOGV("mPbrp: %p ", mPbrp);

    return;
}

/*===========================================================================
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: get logical camera info
 *
 * PARAMETERS:
 *   @camera_id     : Logical Camera ID
 *   @info              : Logical Main Camera Info
 *
 * RETURN    :
 *              NO_ERROR  : success
 *              ENODEV : Camera not found
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3PortraitScene::get_camera_info(__unused int camera_id,
                                              struct camera_info *info) {
    int rc = NO_ERROR;

    HAL_LOGV("E camera_id %d", camera_id);
    if (info) {
        rc = mPbrp->getCameraInfo(camera_id, info);
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
int SprdCamera3PortraitScene::camera_device_open(
    __unused const struct hw_module_t *module, const char *id,
    struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    if (!id) {
        HAL_LOGE("Invalid camera id");
        return BAD_VALUE;
    }
    HAL_LOGV("id= %d", atoi(id));
    rc = mPbrp->cameraDeviceOpen(atoi(id), hw_device);
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
int SprdCamera3PortraitScene::close_camera_device(
    __unused hw_device_t *hw_dev) {
    if (hw_dev == NULL) {
        HAL_LOGE("failed.hw_dev null");
        return -1;
    }

    return mPbrp->closeCameraDevice();
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
int SprdCamera3PortraitScene::initialize(
    __unused const struct camera3_device *device,
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;

    HAL_LOGV("E");
    CHECK_PBRP_ERROR();
    rc = mPbrp->initialize(callback_ops);

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
const camera_metadata_t *
SprdCamera3PortraitScene::construct_default_request_settings(
    const struct camera3_device *device, int type) {
    const camera_metadata_t *rc;

    HAL_LOGV("E");
    if (!mPbrp) {
        HAL_LOGE("Error getting capture ");
        return NULL;
    }
    rc = mPbrp->constructDefaultRequestSettings(device, type);

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
int SprdCamera3PortraitScene::configure_streams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_PBRP_ERROR();

    rc = mPbrp->configureStreams(device, stream_list);

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
int SprdCamera3PortraitScene::process_capture_request(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;

    HAL_LOGV("idx:%d", request->frame_number);
    CHECK_PBRP_ERROR();
    rc = mPbrp->processCaptureRequest(device, request);
    HAL_LOGI("X");
    return rc;
}
/*===========================================================================
 * FUNCTION   :process_capture_result_main
 *
 * DESCRIPTION: deconstructor of SprdCamera3PortraitScene
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3PortraitScene::process_capture_result_main(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {

    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_PBRP();
    mPbrp->processCaptureResultMain((camera3_capture_result_t *)result);
}

/*===========================================================================
 * FUNCTION   :process_capture_result_aux
 *
 * DESCRIPTION: deconstructor of SprdCamera3PortraitScene
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void SprdCamera3PortraitScene::process_capture_result_aux(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {

    HAL_LOGV("idx:%d", result->frame_number);
    CHECK_PBRP();
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
void SprdCamera3PortraitScene::notifyMain(
    const struct camera3_callback_ops *ops, const camera3_notify_msg_t *msg) {

    HAL_LOGV("idx:%d", msg->message.shutter.frame_number);
    CHECK_PBRP();
    mPbrp->notifyMain(msg);
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
void SprdCamera3PortraitScene::notifyAux(const struct camera3_callback_ops *ops,
                                         const camera3_notify_msg_t *msg) {

    HAL_LOGD("idx:%d", msg->message.shutter.frame_number);
    CHECK_PBRP();
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
void SprdCamera3PortraitScene::dump(const struct camera3_device *device,
                                    int fd) {
    HAL_LOGV("E");
    CHECK_PBRP();

    mPbrp->_dump(device, fd);

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
int SprdCamera3PortraitScene::flush(const struct camera3_device *device) {
    int rc = 0;

    HAL_LOGV(" E");
    CHECK_PBRP_ERROR();

    rc = mPbrp->_flush(device);

    HAL_LOGV(" X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :CaptureThread
 *
 * DESCRIPTION: constructor of CaptureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::CaptureThread::CaptureThread()
    : mCapApihandle(NULL), mFirstCapture(true), mWeightWidth(0),
      mWeightHeight(0), mUpdateCaptureWeightParams(false) {
    HAL_LOGI(" E");
    memset(&mCapInitParams, 0, sizeof(sprd_portrait_scene_init_t));
    memset(&mCapWeightParams, 0, sizeof(sprd_portrait_scene_proc_t));
    memset(&mCapMaskParams, 0, sizeof(sprd_portrait_scene_adapter_mask_t));

    mCapMsgList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :~~CaptureThread
 *
 * DESCRIPTION: deconstructor of CaptureThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::CaptureThread::~CaptureThread() {
    HAL_LOGI(" E");
    mCapMsgList.clear();
    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :capMattingHandle
 *
 * DESCRIPTION: capblurHandle
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/

int SprdCamera3PortraitScene::CaptureThread::capMattingHandle(
    buffer_handle_t *combo_buffer, void *combo_buff_addr,
    buffer_handle_t *maskBuffer, void *mask_data, uint32_t combo_frm_num) {
    int ret = 0;

    SprdCamera3HWI *hwi = mPbrp->m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    uint32_t orientation =
        SprdCamera3Setting::s_setting[mPbrp->mCameraId].jpgInfo.orientation;
    void *orgJpegBufferAddr = NULL;
    dump_portrait_scene_t combo_buff, mask_buff;
    HAL_LOGI(" E");
    memset(&combo_buff, 0, sizeof(dump_portrait_scene_t));
    mPbrp->m_pOrgJpegBuffer = &mPbrp->mLocalCapBuffer[1].native_handle;
    HAL_LOGI(" m_pOrgJpegBuffer %p", mPbrp->m_pOrgJpegBuffer);
    if (mPbrp->map(mPbrp->m_pOrgJpegBuffer, &orgJpegBufferAddr) == NO_ERROR) {
        mPbrp->mOrgJpegSize = mPbrp->jpeg_encode_exif_simplify(
            combo_buffer, combo_buff_addr, mPbrp->m_pOrgJpegBuffer,
            orgJpegBufferAddr, NULL, NULL, hwi, orientation);
        UNMAP_AND_SET_NULL(mPbrp->m_pOrgJpegBuffer, &orgJpegBufferAddr);
    } else {
        HAL_LOGE("map m_pOrgJpegBuffer(%p) failed", mPbrp->m_pOrgJpegBuffer);
    }

    combo_buff.buffer_addr = combo_buff_addr;
    combo_buff.frame_number = combo_frm_num;
    dumpPbrpImg(DUMP_PORTRAIT_SCENE_COMBO, &combo_buff);

    {
        if (combo_buff_addr != NULL) {
            HAL_LOGD("combo_buff_addr is ok");
            mCapMaskParams.src_YUV = (unsigned char *)combo_buff_addr;
        }
        mCapMaskParams.mask = (uint16_t *)mask_data;

        HAL_LOGD("cap matting init mFirstCapture %d", mFirstCapture);
        if (mFirstCapture) {
            mFirstCapture = false;
            int64_t initStart = systemTime();
            mCapInitParams.handle = mCapApihandle;
            mCapApihandle = sprd_portrait_scene_adpt_init(&mCapInitParams);
            if (mCapApihandle != NULL) {
                HAL_LOGD("cap matting init cost %d ms,handle=%p",
                         (int)ns2ms(systemTime() - initStart), mCapApihandle);
            } else {
                HAL_LOGE("cap matting init failed!!!!!!!");
            }
        }
        if (mUpdateCaptureWeightParams) {
            int64_t creatStart = systemTime();
            mUpdateCaptureWeightParams = false;
            HAL_LOGD("mCapWeightParams:%d %d %d %d %d %d, rotate_angle%d, "
                     "camera_angle%d,mobile_angle%d",
                     mCapWeightParams.valid_roi, mCapWeightParams.roi_type,
                     mCapWeightParams.circle_size, mCapWeightParams.f_number,
                     mCapWeightParams.sel_x, mCapWeightParams.sel_y,
                     mCapWeightParams.rotate_angle,
                     mCapWeightParams.camera_angle,
                     mCapWeightParams.mobile_angle);
            if (mPbrp->mIsRunAlgo) {
                ret = sprd_portrait_scene_adpt_ctrl(
                    mCapApihandle, SPRD_PORTRAIT_SCENE_WEIGHT_CMD,
                    &mCapWeightParams);
            }
            if (ret != 0)
                HAL_LOGE("capture WEIGHT_CMD Err:%d", ret);
            else
                HAL_LOGD("capture WEIGHT_CMD cost %d ms",
                         (int)ns2ms(systemTime() - creatStart));
        }

        int64_t maskStart = systemTime();
        HAL_LOGD("mCapMaskParams:src_yuv=%p,mask_data=%p",
                 mCapMaskParams.src_YUV, mCapMaskParams.mask);
        if (mPbrp->mIsRunAlgo) {
            ret = sprd_portrait_scene_adpt_ctrl(mCapApihandle,
                                                SPRD_PORTRAIT_SCENE_PROCESS_CMD,
                                                &mCapMaskParams);
        }
        if (ret != 0)
            LOGE("capture PROCESS_CMD is error");
        else
            HAL_LOGD("capture PROCESS_CMD cost %d ms",
                     (int)ns2ms(systemTime() - maskStart));
    }

    mask_buff.buffer_addr = mCapMaskParams.mask;
    mask_buff.frame_number = combo_frm_num;
    dumpPbrpImg(DUMP_PORTRAIT_SCENE_MASK, &mask_buff);
    HAL_LOGI(" X");
    return ret;
}
void UpdateWeightParam(sprd_portrait_scene_proc_t *dst,
                       sprd_portrait_scene_proc_t *src) {
    Mutex::Autolock l(mPbrp->mWeightLock);
    memcpy(dst, src, sizeof(sprd_portrait_scene_proc_t));
}
/*===========================================================================
 * FUNCTION   :capFuse
 *
 * DESCRIPTION: capFuse
 *
 * PARAMETERS : yuv_data,mask_data
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3PortraitScene::CaptureThread::capFuse(void *combo_buff_addr,
                                                     void *mask_data) {
    int ret = NO_ERROR, isHorizon = 0, ID2index = 0;
    buffer_handle_t *const fuse_buffer =
        &mPbrp->mLocalCapBuffer[3].native_handle;
    buffer_handle_t *const BG_buffer = &mPbrp->mLocalCapBuffer[4].native_handle;
    void *fuse_buff_addr = NULL;
    void *BG_buff_addr = NULL;

    MAP_AND_CHECK(fuse_buffer, &fuse_buff_addr);
    MAP_AND_CHECK(BG_buffer, &BG_buff_addr);

    // isHorizon: whether hold mobile phone horizontally
    if (mPbrp->mCapT->mCapWeightParams.rotate_angle == 0 ||
        mPbrp->mCapT->mCapWeightParams.rotate_angle == 180) {
        isHorizon = 1;
    }
    // mCapBgID(0~4), ID2index(0~9)
    ID2index = mPbrp->mCapBgID * 2 + isHorizon;

    Mutex::Autolock l(mPbrp->mCapBGLock);
    HAL_LOGD("succeed in reading cap BG");
    if (mPbrp->mCapBgID == BG_COLOR_RETENTION) {
        mPbrp->mCapPostT->mCapFuseParams.fuse.isColorRetention = true;
        ID2index = 0;
    } else {
        mPbrp->mCapPostT->mCapFuseParams.fuse.isColorRetention = false;
        mPbrp->mCapPostT->mCapFuseParams.fuse.data = (int8_t *)BG_buff_addr;
    }
    mPbrp->mCapPostT->mCapFuseParams.fuse.ch = SPRD_PORTRAIT_SCENE_CAPTURE;
    mPbrp->mCapPostT->mCapFuseParams.src_YUV = (unsigned char *)combo_buff_addr;
    mPbrp->mCapPostT->mCapFuseParams.dst_YUV = (unsigned char *)fuse_buff_addr;
    mPbrp->mCapPostT->mCapFuseParams.fuse.mask = (uint16_t *)mask_data;
    mPbrp->mCapPostT->mCapFuseParams.fuse.total = AI_BGIMG_BUFF_NUM * 2;
    mPbrp->mCapPostT->mCapFuseParams.fuse.rotate_angle =
        mPbrp->mCapT->mCapWeightParams.rotate_angle;

    mPbrp->mCapPostT->mCapFuseParams.fuse.fileFormat[ID2index] =
        IMG_FILE_FORMAT_YUV;

    if (mPbrp->checkIsVideo()) {
        mPbrp->mCapPostT->mCapFuseParams.fuse.width[ID2index] =
            mPbrp->mVideoWidth;
        mPbrp->mCapPostT->mCapFuseParams.fuse.height[ID2index] =
            mPbrp->mVideoHeight;
    } else {
        mPbrp->mCapPostT->mCapFuseParams.fuse.width[ID2index] =
            mPbrp->mCaptureWidth;
        mPbrp->mCapPostT->mCapFuseParams.fuse.height[ID2index] =
            mPbrp->mCaptureHeight;
    }
    mPbrp->mCapPostT->mCapFuseParams.fuse.index = ID2index;
    HAL_LOGD("data=%p,isColorRetention=%d,ch=%d,src_yuv=%p,"
             "dst_yuv=%p,total=%d,rotate_angle=%d,"
             "fileFormat[%d]=%d,width[%d]=%d,height[%d]=%d,"
             "index=%d",
             mPbrp->mCapPostT->mCapFuseParams.fuse.data,
             mPbrp->mCapPostT->mCapFuseParams.fuse.isColorRetention,
             mPbrp->mCapPostT->mCapFuseParams.fuse.ch,
             mPbrp->mCapPostT->mCapFuseParams.src_YUV,
             mPbrp->mCapPostT->mCapFuseParams.dst_YUV,
             mPbrp->mCapPostT->mCapFuseParams.fuse.total,
             mPbrp->mCapPostT->mCapFuseParams.fuse.rotate_angle, ID2index,
             mPbrp->mCapPostT->mCapFuseParams.fuse.fileFormat[ID2index],
             ID2index, mPbrp->mCapPostT->mCapFuseParams.fuse.width[ID2index],
             ID2index, mPbrp->mCapPostT->mCapFuseParams.fuse.height[ID2index],
             mPbrp->mCapPostT->mCapFuseParams.fuse.index);

    int64_t fuseStart = systemTime();
    if (mPbrp->mIsRunAlgo) {
        ret = sprd_portrait_scene_adpt_ctrl(mCapApihandle,
                                            SPRD_PORTRAIT_SCENE_FUSE_CMD,
                                            &mPbrp->mCapPostT->mCapFuseParams);
    }
    if (ret != 0) {
        HAL_LOGE("capture FUSE_CMD is error");
    } else {
        HAL_LOGD("capture FUSE_CMD cost %d ms",
                 (int)ns2ms(systemTime() - fuseStart));
    }

    UNMAP_AND_SET_NULL(fuse_buffer, &fuse_buff_addr);
    UNMAP_AND_SET_NULL(BG_buffer, &BG_buff_addr);
    return ret;
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
bool SprdCamera3PortraitScene::CaptureThread::threadLoop() {
    portrait_scene_queue_msg_t capture_msg;
    memset(&capture_msg, 0, sizeof(portrait_scene_queue_msg_t));
    void *combo_buff_addr = NULL;
    void *mask_data = NULL;
    bool matched = false;
    int mime_type = (int)SPRD_MIMETPYE_NONE;
    HAL_LOGI("cap thread run");

    while (!mCapMsgList.empty()) {
        HAL_LOGI("cap thread msglist process");
        List<portrait_scene_queue_msg_t>::iterator itor1 = mCapMsgList.begin();
        capture_msg = *itor1;
        mCapMsgList.erase(itor1);
        switch (capture_msg.msg_type) {
        case PORTRAIT_SCENE_MSG_EXIT: {
            HAL_LOGD("CAP_PORTRAIT_SCENE_MSG_EXIT");
            if (mPbrp->mCapT->mCapApihandle) {
                sprd_portrait_scene_adpt_deinit(mPbrp->mCapT->mCapApihandle,
                                                SPRD_PORTRAIT_SCENE_CAPTURE);
                mPbrp->mCapT->mCapApihandle = NULL;
            }
            memset(&mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
            memset(&mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));
            if (NULL != mSavedCapReqsettings) {
                free_camera_metadata(mSavedCapReqsettings);
                mSavedCapReqsettings = NULL;
            }
            return false;
        } break;
        case PORTRAIT_SCENE_MSG_DATA_PROC: {
            HAL_LOGD("CAP_PORTRAIT_SCENE_MSG_DATA_PROC");
            matched = mPbrp->search_reqlist(capture_msg.combo_buff.frame_number,
                                            mPbrp->mCapSavedReqList);
            if (matched == false || mPbrp->mFlushing) {
                HAL_LOGE("Failed to matched frame idx, or Flushing",
                         capture_msg.combo_buff.frame_number);
                goto exit;
            }
            Mutex::Autolock l(mPbrp->mCapLock);
            SprdCamera3HWI *hwi = mPbrp->m_pPhyCamera[CAM_TYPE_MAIN].hwi;
            MAP_AND_CHECK(capture_msg.combo_buff.buffer, &combo_buff_addr);
            mPbrp->yuv_do_face_beauty_simplify(capture_msg.combo_buff.buffer,
                                               combo_buff_addr, hwi);
            mime_type = (int)SPRD_MIMETPYE_NONE;
            hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE, &mime_type, NULL);
            if ((mPbrp->mCapBgID > BG_OFF) && ((mPbrp->mFaceNum > 0))) {
                HAL_LOGV("mCapBgID=%d,mFaceNum=%d", mPbrp->mCapBgID,
                         mPbrp->mFaceNum);
                mPbrp->m_pmaskBuffer = &mPbrp->mLocalCapBuffer[2].native_handle;
                MAP_AND_CHECK(mPbrp->m_pmaskBuffer, &mask_data);
                HAL_LOGD("test handleAddr=%p,mask_data=%p,buffer "
                         "w=%d,h=%d,should be %d Byte",
                         &mPbrp->mLocalCapBuffer[2], mask_data,
                         mPbrp->mLocalCapBuffer[2].width,
                         mPbrp->mLocalCapBuffer[2].height, 512 * 512 * 2);
                HAL_LOGD("mFlushing:%d, frame idx:%d buffer addr 0x%p",
                         mPbrp->mFlushing, capture_msg.combo_buff.frame_number,
                         capture_msg.combo_buff.buffer);

                // get mask
                capMattingHandle(capture_msg.combo_buff.buffer, combo_buff_addr,
                                 mPbrp->m_pmaskBuffer, mask_data,
                                 capture_msg.combo_buff.frame_number);
                // do fusion
                capFuse(combo_buff_addr, mask_data);

                UNMAP_AND_SET_NULL(capture_msg.combo_buff.buffer,
                                   &combo_buff_addr);
                UNMAP_AND_SET_NULL(mPbrp->m_pmaskBuffer, &mask_data);

                // yuv reprocess
                if (mPbrp->mIsRunAlgo && mCapApihandle != NULL) {
                    mime_type = (int)SPRD_MIMETPYE_PORTRAIT_SCENE;
                    hwi->camera_ioctrl(CAMERA_IOCTRL_SET_MIME_TYPE, &mime_type,
                                       NULL);

                    buffer_handle_t *const fuse_buffer =
                        &mPbrp->mLocalCapBuffer[3].native_handle;
                    if (mPbrp->mCapPostT->yuvReprocessCaptureRequest(
                            fuse_buffer, capture_msg.combo_buff.frame_number) ==
                        false) {
                        HAL_LOGE("fuse_yuv reprocess false");
                        return false;
                    }
                } else {
                    if (mPbrp->mCapPostT->yuvReprocessCaptureRequest(
                            capture_msg.combo_buff.buffer,
                            capture_msg.combo_buff.frame_number) == false) {
                        HAL_LOGE("src_yuv reprocess false");
                        return false;
                    }
                }
            } else {
                UNMAP_AND_SET_NULL(capture_msg.combo_buff.buffer,
                                   &combo_buff_addr);
                HAL_LOGD(
                    "bypass algo, because of mCapBgID = %d and faceNum = %d",
                    mPbrp->mCapBgID, mPbrp->mFaceNum);
                if (mPbrp->mCapPostT->yuvReprocessCaptureRequest(
                        capture_msg.combo_buff.buffer,
                        capture_msg.combo_buff.frame_number) == false) {
                    HAL_LOGE("src_yuv reprocess false");
                    return false;
                }
            }
        } break;
        default:
            HAL_LOGD("unknow msg type = %d", capture_msg.msg_type);
            break;
        }
    };
    HAL_LOGD("cap thread finish process");
exit:
    waitMsgAvailable();

    return true;
}

/*===========================================================================
 * FUNCTION   :sprdDepthHandle
 *
 * DESCRIPTION: sprdDepthHandle
 *
 * PARAMETERS :
 *
 * RETURN	  : None
 *==========================================================================*/

bool SprdCamera3PortraitScene::CaptureThread::capReadHandle(
    portrait_scene_queue_msg_t *post_msg) {
    bool ret = false;
    portrait_scene_queue_msg_t send_post_msg = *post_msg;

    HAL_LOGI("E cap api is %p", send_post_msg.apihandle);
    Mutex::Autolock l(mPbrp->mCapPostT->mMergequeueMutex);
    mPbrp->mCapPostT->mCapPostMsgList.push_back(send_post_msg);
    mPbrp->mCapPostT->mMergequeueSignal.signal();
    HAL_LOGI("X ");
    return ret;
}

/*===========================================================================
 * FUNCTION   :dumpPbrpImg
 *
 * DESCRIPTION: dumpPbrpImg
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::CaptureThread::dumpPbrpImg(
    dump_portrait_scene_type type, dump_portrait_scene_t *dump_buffs) {

    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    unsigned char *buffer_base = NULL;

    switch (type) {
    case DUMP_PORTRAIT_SCENE_COMBO:
        property_get("persist.vendor.cam.pbrp.dump", prop, "0");

        if ((!strcmp(prop, "combo") || !strcmp(prop, "all")) &&
            !mPbrp->mFlushing && dump_buffs != NULL) {
            buffer_base = (unsigned char *)dump_buffs->buffer_addr;
            mPbrp->dumpData(buffer_base, 1,
                            mPbrp->mCaptureWidth * mPbrp->mCaptureHeight * 3 /
                                2,
                            mPbrp->mCaptureWidth, mPbrp->mCaptureHeight,
                            dump_buffs->frame_number, "src_yuv");
        }
        break;

    case DUMP_PORTRAIT_SCENE_MASK:
        property_get("persist.vendor.cam.pbrp.dump", prop, "0");

        if ((!strcmp(prop, "mask") || !strcmp(prop, "all")) &&
            dump_buffs != NULL) {
            buffer_base = (unsigned char *)dump_buffs->buffer_addr;
            mPbrp->dumpData(buffer_base, 1,
                            mPbrp->mCaptureWidth * mPbrp->mCaptureHeight * 3 /
                                2,
                            mPbrp->mCaptureWidth, mPbrp->mCaptureHeight,
                            dump_buffs->frame_number, "mask_yuv");
        }
        break;

    case DUMP_PORTRAIT_SCENE_FUSE:
        property_get("persist.vendor.cam.pbrp.dump", prop, "0");

        if ((!strcmp(prop, "fuse") || !strcmp(prop, "all")) &&
            dump_buffs != NULL) {
            buffer_base = (unsigned char *)dump_buffs->buffer_addr;
            mPbrp->dumpData(buffer_base, 1,
                            mPbrp->mCaptureWidth * mPbrp->mCaptureHeight * 3 /
                                2,
                            mPbrp->mCaptureWidth, mPbrp->mCaptureHeight,
                            dump_buffs->frame_number, "fuse_yuv");
        }
        break;

    case DUMP_PORTRAIT_SCENE_RESULT:
        property_get("persist.vendor.cam.pbrp.dump", prop, "0");
        if ((!strcmp(prop, "result") || !strcmp(prop, "all")) &&
            dump_buffs != NULL) {

            uint32_t para_num = 0;
            buffer_base = (unsigned char *)dump_buffs->buffer_addr;
            uint32_t mask_size =
                mCapInitParams.width * mCapInitParams.height * 3 / 2;
            uint32_t para_size = 0;
            uint32_t org_jpeg_size = mPbrp->mOrgJpegSize;
            para_num += PBRP_REFOCUS_COMMON_PARAM_NUM + PBRP_MAX_ROI * 4;
            para_size = para_num * 4;
            // dump jpeg
            mPbrp->dumpData(buffer_base, 2, dump_buffs->jpeg_size,
                            mPbrp->mCaptureWidth, mPbrp->mCaptureHeight, 0,
                            "jpeg");
            // dump para
            buffer_base += (dump_buffs->use_size - para_size);
            mPbrp->dumpData(buffer_base, 3, para_num, 4, 0, 0, "parameter");
            // dump org jpeg
            buffer_base -= org_jpeg_size;
            mPbrp->dumpData(buffer_base, 2, org_jpeg_size, mPbrp->mCaptureWidth,
                            mPbrp->mCaptureHeight, 0, "orgJpeg");

            // dump mask yuv
            buffer_base -= mask_size;
            mPbrp->dumpData(buffer_base, 1, mask_size, mCapInitParams.width,
                            mCapInitParams.height, 0, "mask");
        }
        break;

    default:
        break;
    }
}

/*===========================================================================
 * FUNCTION   :initCapParams
 *
 * DESCRIPTION: init capture Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3PortraitScene::CaptureThread::initCapParams() {
    mFirstCapture = true;
    mUpdateCaptureWeightParams = true;
    mWeightWidth = 0;
    mWeightHeight = 0;
    mCapInitParams.handle = NULL;
    mCapInitParams.ch = SPRD_PORTRAIT_SCENE_CAPTURE;
    mCapInitParams.productID = PLATFORM_ID;
    mCapInitParams.min_slope = (float)(4) / 10000;
    mCapInitParams.max_slope = (float)(19) / 10000;
    mCapInitParams.slope = 128;
    mCapInitParams.Findex2Gamma_AdjustRatio = (float)(150000) / 10000;
    mCapInitParams.Scalingratio = 4;
    mCapInitParams.SmoothWinSize = 5;
    mCapInitParams.box_filter_size = 3;
    mCapInitParams.gau_min_slope = (float)(10) / 10000;
    mCapInitParams.gau_max_slope = (float)(50) / 10000;
    mCapInitParams.gau_Findex2Gamma_AdjustRatio = (float)(20000) / 10000;
    mCapInitParams.gau_box_filter_size = 11;
    if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
        mCapInitParams.isFrontCamera = 0;
    } else if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID_2) {
        mCapInitParams.isFrontCamera = 1;
    }
    mCapInitParams.run_type = SPRD_CAMALG_RUN_TYPE_CPU;
    mPbrp->mCapT->mCapApihandle = NULL;
    mCapMaskParams.ch = SPRD_PORTRAIT_SCENE_CAPTURE;
    mPbrp->mCurrCapFrameNum = -1;
    return 0;
}

/*===========================================================================
 * FUNCTION   :initCapWeightParams
 *
 * DESCRIPTION: init Blur Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3PortraitScene::CaptureThread::initCapWeightParams() {
    mPbrp->mCacheCapWeightParams.camera_angle =
        SprdCamera3Setting::s_setting[mPbrp->mCameraId].sensorInfo.orientation;
    mPbrp->mCacheCapWeightParams.version = 1;
    mPbrp->mCacheCapWeightParams.roi_type = 2;
    if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
        mPbrp->mCacheCapWeightParams.rear_cam_en = true;
        mPbrp->mCacheCapWeightParams.isFrontCamera = 0;
    } else {
        mPbrp->mCacheCapWeightParams.rear_cam_en = false;
        mPbrp->mCacheCapWeightParams.isFrontCamera = 1;
    }
    // capture weight params
    mPbrp->mCacheCapWeightParams.ch = SPRD_PORTRAIT_SCENE_CAPTURE;
    mPbrp->mCacheCapWeightParams.f_number = 2;
    mPbrp->mCacheCapWeightParams.circle_size = 0;
    mPbrp->mCacheCapWeightParams.valid_roi = 0;
    mPbrp->mCacheCapWeightParams.total_roi = 0;
    mPbrp->mCacheCapWeightParams.rotate_angle = 0;
    mPbrp->mCacheCapWeightParams.isCapture = 1;
    memset(mPbrp->mCacheCapWeightParams.x1, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCacheCapWeightParams.y1, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCacheCapWeightParams.x2, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCacheCapWeightParams.y2, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCacheCapWeightParams.flag, 0x00,
           sizeof(mPbrp->mCacheCapWeightParams.flag));
}

/*===========================================================================
 * FUNCTION   :saveCaptureParams
 *
 * DESCRIPTION: save Capture Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3PortraitScene::CaptureThread::saveCaptureParams(
    buffer_handle_t *result_buff, uint32_t jpeg_size) {
    uint32_t i = 0;
    void *buffer_base = NULL;
    unsigned char *buffer = NULL;
    uint32_t buffer_size = ADP_WIDTH(*result_buff);
    uint32_t mask_size = mPbrp->mMaskSaveSize;
    uint32_t para_size = PBRP_REFOCUS_PARAM_NUM * sizeof(int32_t);
    uint32_t org_jpeg_size = mPbrp->mOrgJpegSize;
    uint32_t use_size = 0;

    HAL_LOGD("E");
    MAP_AND_CHECK(result_buff, &buffer_base);

    buffer = (unsigned char *)buffer_base;
    use_size = para_size + org_jpeg_size + mask_size + jpeg_size;
    /* memset space after jpeg*/
    memset(buffer + jpeg_size, 0, use_size - jpeg_size);
    HAL_LOGD("para_size=%d,org_jpeg_size=%d,mask_size=%d,jpeg_size=%d"
             "buff_size=%d,sub=%d",
             para_size, org_jpeg_size, mask_size, jpeg_size, buffer_size,
             buffer_size - use_size);
    // cpoy common param to tail
    int32_t productID = mCapInitParams.productID;
    int32_t main_width = mCapInitParams.width;
    int32_t main_height = mCapInitParams.height;
    int32_t mask_w = SAVE_MASK_W;
    int32_t mask_h = SAVE_MASK_H;
    int32_t total_roi = mCapWeightParams.total_roi;
    int32_t process_jpeg = jpeg_size;
    int32_t orig_jpeg_size = org_jpeg_size;
    int32_t camera_id = mPbrp->mCameraId;
    int32_t rotate_angle = mCapWeightParams.rotate_angle;
    unsigned char PbrpFlag[] = {'P', 'B', 'R', 'P'};
    unsigned char *p1[] = {
        (unsigned char *)&productID,    (unsigned char *)&main_width,
        (unsigned char *)&main_height,  (unsigned char *)&mask_w,
        (unsigned char *)&mask_h,       (unsigned char *)&total_roi,
        (unsigned char *)&process_jpeg, (unsigned char *)&orig_jpeg_size,
        (unsigned char *)&camera_id,    (unsigned char *)&rotate_angle,
        (unsigned char *)&PbrpFlag};
    buffer += (use_size - PBRP_REFOCUS_COMMON_PARAM_NUM * 4);
    HAL_LOGD("common param base=%p", buffer);
    for (i = 0; i < PBRP_REFOCUS_COMMON_PARAM_NUM; i++) {
        memcpy(buffer + i * 4, p1[i], 4);
    }
    // copy face position array
    buffer -= PBRP_MAX_ROI * 4 * 4;
    for (i = 0; i < PBRP_MAX_ROI; i++) {
        memcpy(buffer + i * 4, mCapWeightParams.x1 + i, 4);
    }
    for (i = 0; i < PBRP_MAX_ROI; i++) {
        memcpy((buffer + PBRP_MAX_ROI * 4) + i * 4, mCapWeightParams.y1 + i, 4);
    }
    for (i = 0; i < PBRP_MAX_ROI; i++) {
        memcpy((buffer + PBRP_MAX_ROI * 8) + i * 4, mCapWeightParams.x2 + i, 4);
    }
    for (i = 0; i < PBRP_MAX_ROI; i++) {
        memcpy((buffer + PBRP_MAX_ROI * 12) + i * 4, mCapWeightParams.y2 + i,
               4);
    }
    // copy org jpeg
    mPbrp->m_pOrgJpegBuffer = &mPbrp->mLocalCapBuffer[1].native_handle;
    HAL_LOGD("befor map1,m_pOrgJpegBuffer=%p", mPbrp->m_pOrgJpegBuffer);
    unsigned char *orig_jpeg_data = NULL;
    if (mPbrp->map(mPbrp->m_pOrgJpegBuffer, (void **)(&orig_jpeg_data)) ==
        NO_ERROR) {
        buffer -= orig_jpeg_size;
        memcpy(buffer, orig_jpeg_data, orig_jpeg_size);
        UNMAP_AND_SET_NULL(mPbrp->m_pOrgJpegBuffer, (void **)(&orig_jpeg_data));
    } else {
        HAL_LOGE("map m_pOrgJpegBuffer(%p) failed", mPbrp->m_pOrgJpegBuffer);
    }

    // copy mask
    mPbrp->m_pmaskBuffer = &mPbrp->mLocalCapBuffer[2].native_handle;
    unsigned char *mask_data = NULL;
    if (mPbrp->map(mPbrp->m_pmaskBuffer, (void **)(&mask_data)) == NO_ERROR) {
        buffer -= mask_size;
        memcpy(buffer, mask_data, mask_size);
        UNMAP_AND_SET_NULL(mPbrp->m_pmaskBuffer, (void **)(&mask_data));
    } else {
        HAL_LOGE("map m_pmaskBuffer(%p) failed", mPbrp->m_pmaskBuffer);
    }

    // blob to indicate all image size(use_size)
    buffer = (unsigned char *)buffer_base;
    mPbrp->setJpegSize((char *)buffer, buffer_size, use_size);

    dump_portrait_scene_t result_jpeg_buff;
    memset(&result_jpeg_buff, 0, sizeof(dump_portrait_scene_t));
    result_jpeg_buff.buffer_addr = buffer_base;
    result_jpeg_buff.use_size = use_size;
    result_jpeg_buff.jpeg_size = jpeg_size;
    dumpPbrpImg(DUMP_PORTRAIT_SCENE_RESULT, &result_jpeg_buff);

    UNMAP_AND_SET_NULL(result_buff, &buffer_base);
    return 0;
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
void SprdCamera3PortraitScene::CaptureThread::requestExit() {
    HAL_LOGI("E");
    portrait_scene_queue_msg_t blur_msg;
    memset(&blur_msg, 0, sizeof(portrait_scene_queue_msg_t));
    blur_msg.msg_type = PORTRAIT_SCENE_MSG_EXIT;
    mCapMsgList.push_back(blur_msg);
    mMergequeueSignal.signal();
    HAL_LOGI("X");
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
void SprdCamera3PortraitScene::CaptureThread::waitMsgAvailable() {
    // TODO:what to do for timeout
    while (mCapMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, PBRP_THREAD_TIMEOUT);
    }
}
/*===========================================================================
 * FUNCTION   :CapPostThread
 *
 * DESCRIPTION: constructor of PreviewThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::CapturePostThread::CapturePostThread()
    : mCallbackOps(NULL) {
    HAL_LOGI("E");
    mCapPostMsgList.clear();
    memset(&mCapFuseParams, 0, sizeof(sprd_portrait_scene_adapter_fuse_t));
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~PreviewThread
 *
 * DESCRIPTION: deconstructor of PreviewThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::CapturePostThread::~CapturePostThread() {
    HAL_LOGI(" E");

    mCapPostMsgList.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :initPrevPostInitParams
 *
 * DESCRIPTION: init initPrevPostInitParams Init Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3PortraitScene::CapturePostThread::initCapPostInitParams() {
    if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
        mCapFuseParams.fuse.isFrontCamera = 0;
    } else {
        mCapFuseParams.fuse.isFrontCamera = 1;
    }
    return 0;
}

/*===========================================================================
 * FUNCTION   :yuvReprocessCaptureRequest
 *
 * DESCRIPTION: yuvReprocessCaptureRequest
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
bool SprdCamera3PortraitScene::CapturePostThread::yuvReprocessCaptureRequest(
    buffer_handle_t *output_buffer, uint32_t combo_frm_num) {
    SprdCamera3HWI *hwi = mPbrp->m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    camera3_capture_request_t request;
    camera3_stream_buffer_t output_stream_buff;
    camera3_stream_buffer_t input_stream_buff;

    memset(&request, 0x00, sizeof(camera3_capture_request_t));
    memset(&output_stream_buff, 0x00, sizeof(camera3_stream_buffer_t));
    memset(&input_stream_buff, 0x00, sizeof(camera3_stream_buffer_t));

    memcpy((void *)&request, &mPbrp->mCapT->mSavedCapRequest,
           sizeof(camera3_capture_request_t));
    request.settings = mPbrp->mCapT->mSavedCapReqsettings;

    request.num_output_buffers = 1;
    memcpy((void *)&input_stream_buff, &mPbrp->mCapT->mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));

    input_stream_buff.stream =
        &mPbrp->mMainStreams[mPbrp->mCaptureStreamsNum - 1];
    input_stream_buff.stream->width = mPbrp->mCaptureWidth;
    input_stream_buff.stream->height = mPbrp->mCaptureHeight;
    input_stream_buff.buffer = output_buffer;

    memcpy((void *)&output_stream_buff, &mPbrp->mCapT->mSavedCapReqStreamBuff,
           sizeof(camera3_stream_buffer_t));
    output_stream_buff.stream =
        &mPbrp->mMainStreams[mPbrp->mCaptureStreamsNum - 1];
    output_stream_buff.stream->width = mPbrp->mCaptureWidth;
    output_stream_buff.stream->height = mPbrp->mCaptureHeight;
    request.input_buffer = &input_stream_buff;
    request.output_buffers = &output_stream_buff;
    mPbrp->mReqState = REPROCESS_STATE;
    HAL_LOGD("capInfo:set repocess to hwi,idx=%d", combo_frm_num);
    if (0 > hwi->process_capture_request(mPbrp->m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                         &request))
        HAL_LOGE("failed. process capture request!");
    if (NULL != mPbrp->mCapT->mSavedCapReqsettings &&
        mPbrp->mReqState == REPROCESS_STATE) {
        free_camera_metadata(mPbrp->mCapT->mSavedCapReqsettings);
        mPbrp->mCapT->mSavedCapReqsettings = NULL;
    }
    HAL_LOGD("X");
    return true;
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

bool SprdCamera3PortraitScene::CapturePostThread::threadLoop() {
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    buffer_handle_t *output_buffer = NULL;
    uint32_t frame_number = 0;
    int ret = 0;
    bool matched = false;
    char prop_value[PROPERTY_VALUE_MAX] = {
        0,
    };

    HAL_LOGV("capturePostThread run");
    while (!mCapPostMsgList.empty()) {
        List<portrait_scene_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mCapPostMsgList.begin();
            muxer_msg = *it;
            mCapPostMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case PORTRAIT_SCENE_MSG_INIT: {
        } break;
        case PORTRAIT_SCENE_MSG_EXIT: {
            HAL_LOGD("CAP_POST_PORTRAIT_SCENE_MSG_EXIT");
            return false;
        } break;
        case PORTRAIT_SCENE_MSG_DATA_PROC: {
            HAL_LOGD("CAP_POST_PORTRAIT_SCENE_MSG_DATA_PROC");
            Mutex::Autolock l(mPbrp->mCapBGLock);
            if (mPbrp->mCapBgID > BG_COLOR_RETENTION) {
                int isHorizon = 0;
                if (mPbrp->mCapT->mCapWeightParams.rotate_angle == 0 ||
                    mPbrp->mCapT->mCapWeightParams.rotate_angle == 180) {
                    isHorizon = 1;
                }
                mPbrp->loadBgImage(SPRD_PORTRAIT_SCENE_CAPTURE, isHorizon);
            }
        } break;
        default:
            HAL_LOGD("Unknow msg type = %d", muxer_msg.msg_type);
            break;
        }
    }
    waitMsgAvailable();

    return true;
}

/*===========================================================================
 * FUNCTION   :requestInit
 *
 * DESCRIPTION: request thread init
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::CapturePostThread::requestInit() {
    Mutex::Autolock l(mMergequeueMutex);
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    muxer_msg.msg_type = PORTRAIT_SCENE_MSG_INIT;
    mCapPostMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
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
void SprdCamera3PortraitScene::CapturePostThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    muxer_msg.msg_type = PORTRAIT_SCENE_MSG_EXIT;
    mCapPostMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
    HAL_LOGI("X");
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
void SprdCamera3PortraitScene::CapturePostThread::waitMsgAvailable() {
    while (mCapPostMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, PBRP_THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :PreviewThread
 *
 * DESCRIPTION: constructor of PreviewThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::PreviewThread::PreviewThread()
    : mUpdatePreviewWeightParams(false), mApiSegHandle(NULL),
      mFirstPreview(false) {
    HAL_LOGI("E");
    memset(&mPrevInitParams, 0, sizeof(sprd_portrait_scene_init_t));
    memset(&mPrevWeightParams, 0, sizeof(sprd_portrait_scene_proc_t));
    memset(&mPrevMaskParams, 0, sizeof(sprd_portrait_scene_adapter_mask_t));
    memset(&mPrevFuseParams, 0, sizeof(sprd_portrait_scene_adapter_fuse_t));
    mPrevMsgList.clear();
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~PreviewThread
 *
 * DESCRIPTION: deconstructor of PreviewThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::PreviewThread::~PreviewThread() {
    HAL_LOGI(" E");

    mPrevMsgList.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :initPrevInitParams
 *
 * DESCRIPTION: init Segmentation Init Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3PortraitScene::PreviewThread::initPrevInitParams() {

    mFirstPreview = true;
    mPrevInitParams.handle = NULL;
    mPrevInitParams.ch = SPRD_PORTRAIT_SCENE_PREVIEW;
    mPrevInitParams.gau_box_filter_size = 11;
    mPrevInitParams.gau_min_slope = (float)(10) / 10000;
    mPrevInitParams.gau_max_slope = (float)(50) / 10000;
    mPrevInitParams.gau_Findex2Gamma_AdjustRatio = (float)(20000) / 10000;
    mPrevInitParams.min_slope = (float)(4) / 10000;
    mPrevInitParams.max_slope = (float)(19) / 10000;
    mPrevInitParams.box_filter_size = 3;
    mPrevInitParams.Findex2Gamma_AdjustRatio = (float)(150000) / 10000;
    mPrevInitParams.slope = 128;
    if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
        mPrevInitParams.isFrontCamera = 0;
        mPrevFuseParams.fuse.isFrontCamera = 0;
    } else if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID_2) {
        mPrevFuseParams.fuse.isFrontCamera = 1;
        mPrevInitParams.isFrontCamera = 1;
    }
    if (mPbrp->isWechatClient == true) {
        if (mPrevFuseParams.fuse.isFrontCamera == 0)
            mPrevFuseParams.fuse.rotate_angle = 90;
        else
            mPrevFuseParams.fuse.rotate_angle = 270;
    } else {
        mPrevFuseParams.fuse.rotate_angle = 0;
    }

    mPrevInitParams.productID = PLATFORM_ID;
    mPrevInitParams.run_type = SPRD_CAMALG_RUN_TYPE_VDSP;
    mPrevMaskParams.ch = SPRD_PORTRAIT_SCENE_PREVIEW;
    mPrevFuseParams.fuse.ch = SPRD_PORTRAIT_SCENE_PREVIEW;
    return 0;
}

/*===========================================================================
 * FUNCTION   :initPrevWeightParams
 *
 * DESCRIPTION: init Prev Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3PortraitScene::PreviewThread::initPrevWeightParams() {

    mPbrp->mCachePrevWeightParams.ch = SPRD_PORTRAIT_SCENE_PREVIEW;
    mPbrp->mCachePrevWeightParams.version = 1;
    mPbrp->mCachePrevWeightParams.roi_type = 2;
    if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
        mPbrp->mCachePrevWeightParams.isFrontCamera = 0;
    } else if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID_2) {
        mPbrp->mCachePrevWeightParams.isFrontCamera = 1;
    }
    // preview weight params
    mPbrp->mCachePrevWeightParams.f_number = 2;
    mPbrp->mCachePrevWeightParams.valid_roi = 0;
    mPbrp->mCachePrevWeightParams.total_roi = 0;
    if (mPbrp->isWechatClient == true) {
        if (mPbrp->mCachePrevWeightParams.isFrontCamera == 0)
            mPbrp->mCachePrevWeightParams.rotate_angle = 90;
        else
            mPbrp->mCachePrevWeightParams.rotate_angle = 270;
    } else {
        mPbrp->mCachePrevWeightParams.rotate_angle = 0;
    }

    mPbrp->mCachePrevWeightParams.camera_angle = 0;
    mPbrp->mCachePrevWeightParams.mobile_angle = 0;
    mPbrp->mCachePrevWeightParams.isCapture = 0;
    memset(mPbrp->mCachePrevWeightParams.x1, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCachePrevWeightParams.y1, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCachePrevWeightParams.x2, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCachePrevWeightParams.y2, 0x00, sizeof(int) * PBRP_MAX_ROI);
    memset(mPbrp->mCachePrevWeightParams.flag, 0x00,
           sizeof(mPbrp->mCachePrevWeightParams.flag));
    mUpdatePreviewWeightParams = true;
}

/*===========================================================================
 * FUNCTION   :prevMattingHandle
 *
 * DESCRIPTION: prevMattingHandle
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3PortraitScene::PreviewThread::prevMattingHandle(
    buffer_handle_t *input1, void *input1_addr, buffer_handle_t *output,
    void *output_addr) {
    int ret = 0, ID2index = 0;
    char prop_value[PROPERTY_VALUE_MAX] = {
        0,
    };
    if (input1 != NULL) {
        HAL_LOGD("input1 is %p, input1_addr%p", input1, input1_addr);
        mPrevMaskParams.src_YUV = (unsigned char *)input1_addr;
    }
    if (output != NULL) {
        HAL_LOGD("output is not null");
        mPrevMaskParams.dst_YUV = (unsigned char *)output_addr;
    }
    if (mUpdatePreviewWeightParams) {
        int64_t creatStart = systemTime();
        mUpdatePreviewWeightParams = false;
        HAL_LOGD("mPrevWeightParams:%d %d %d %d %d %d, rotate_angle%d, "
                 "camera_angle%d,mobile_angle%d",
                 mPrevWeightParams.valid_roi, mPrevWeightParams.roi_type,
                 mPrevWeightParams.circle_size, mPrevWeightParams.f_number,
                 mPrevWeightParams.sel_x, mPrevWeightParams.sel_y,
                 mPrevWeightParams.rotate_angle, mPrevWeightParams.camera_angle,
                 mPrevWeightParams.mobile_angle);

        HAL_LOGV(
            "handle:%p,version:%d,roi_type:%d,f_number:%d;sel_x:%d;sel_y:%d;"
            "circle_size:%d;"
            "valid_roi:%d;total_roi:%d;x1[MAX_ROI]:%d, y1[MAX_ROI]:%d;"
            "x2[0]:%d, "
            "y2[0]:%d;flag[0]:%d;rotate_angle:%d;camera_angle:%d;mobile_"
            "angle:%d;isCapture:%d;isFrontCamera:%d;"
            "ptr1:%p;ptr2:%p;",
            mApiSegHandle, mPrevWeightParams.version,
            mPrevWeightParams.roi_type, mPrevWeightParams.f_number,
            mPrevWeightParams.sel_x, mPrevWeightParams.sel_y,
            mPrevWeightParams.circle_size, mPrevWeightParams.valid_roi,
            mPrevWeightParams.total_roi, mPrevWeightParams.x1[0],
            mPrevWeightParams.y1[0], mPrevWeightParams.x2[0],
            mPrevWeightParams.y2[0], mPrevWeightParams.flag[0],
            mPrevWeightParams.rotate_angle, mPrevWeightParams.camera_angle,
            mPrevWeightParams.mobile_angle, mPrevWeightParams.isCapture,
            mPrevWeightParams.isFrontCamera, mPrevWeightParams.ptr1,
            mPrevWeightParams.ptr2);
        if (mPbrp->mIsRunAlgo) {
            ret = sprd_portrait_scene_adpt_ctrl(mApiSegHandle,
                                                SPRD_PORTRAIT_SCENE_WEIGHT_CMD,
                                                &mPrevWeightParams);
        }
        if (ret != 0)
            HAL_LOGE("preview create_weight_map Err:%d", ret);
        else
            HAL_LOGD("preview WEIGHT_CMD cost %d ms",
                     (int)ns2ms(systemTime() - creatStart));
    }
    HAL_LOGD("segHandle=%p,rotate_angle=%d", mPbrp->mPrevT->mApiSegHandle,
             mPbrp->mPrevT->mPrevWeightParams.rotate_angle);
    mPrevMaskParams.mask = *mPbrp->mPrevMaskBuffList.begin();
    mPbrp->mPrevMaskBuffList.erase(mPbrp->mPrevMaskBuffList.begin());
    int64_t maskStart = systemTime();
    if (mPbrp->mIsRunAlgo) {
        ret = sprd_portrait_scene_adpt_ctrl(
            mApiSegHandle, SPRD_PORTRAIT_SCENE_PROCESS_CMD, &mPrevMaskParams);
    }
    if (ret != 0)
        LOGE("preview matting failed ret %d", ret);
    else
        HAL_LOGD("preview matting PROCESS_CMD cost %d ms",
                 (int)ns2ms(systemTime() - maskStart));

    HAL_LOGV("X ret:%d", ret);
    return ret;
}

bool SprdCamera3PortraitScene::clearVideoRequst() {
    HAL_LOGD("E");
    camera3_capture_result_t newResult;
    camera3_stream_buffer_t newOutput_buffers;
    memset(&newResult, 0, sizeof(camera3_capture_result_t));
    memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
    Mutex::Autolock l(mPbrp->mLock);
    List<request_saved_msg_t>::iterator itor = mVidSavedReqList.begin();
    while (itor != mVidSavedReqList.end()) {
        HAL_LOGD("idx=%d,vid_handle=%p", itor->frame_number, itor->buffer);
        newOutput_buffers.stream = itor->stream;
        newOutput_buffers.buffer = itor->buffer;
        newResult.frame_number = itor->frame_number;
        newOutput_buffers.status = CAMERA3_BUFFER_STATUS_ERROR;
        newOutput_buffers.acquire_fence = -1;
        newOutput_buffers.release_fence = -1;
        newResult.result = NULL;
        newResult.num_output_buffers = 1;
        newResult.input_buffer = NULL;
        newResult.output_buffers = &newOutput_buffers;
        newResult.partial_result = 0;
        mPrevPostT->mCallbackOps->process_capture_result(
            mPrevPostT->mCallbackOps, &newResult);
        itor++;
    }
    mVidSavedReqList.clear();
    HAL_LOGD("X");
    return false;
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

bool SprdCamera3PortraitScene::PreviewThread::threadLoop() {
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    int last_frame = 0;
    int rc = 0;
    bool matched = false;

    HAL_LOGV("prev thread run");
    while (!mPrevMsgList.empty()) {
        List<portrait_scene_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mPrevMsgList.begin();
            muxer_msg = *it;
            mPrevMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case PORTRAIT_SCENE_MSG_INIT: {
            HAL_LOGD("PREVIEW_PORTRAIT_SCENE_MSG_INIT");
            Mutex::Autolock l(mPbrp->mPrevBGLock);
            if (!mApiSegHandle) {
                int64_t initStart = systemTime();
                mApiSegHandle = sprd_portrait_scene_adpt_init(&mPrevInitParams);
                if (mApiSegHandle != NULL) {
                    HAL_LOGD("prev matting init cost %d ms",
                             (int)ns2ms(systemTime() - initStart));
                } else {
                    HAL_LOGE("Failed to init prev algo handle!");
                    return -1;
                }
            }
        } break;
        case PORTRAIT_SCENE_MSG_EXIT: {
            HAL_LOGD("PREVIEW_PORTRAIT_SCENE_MSG_EXIT");
            return mPbrp->clearVideoRequst();
        }
        case PORTRAIT_SCENE_MSG_DATA_PROC: {
            HAL_LOGV("PREVIEW_PORTRAIT_SCENE_MSG_DATA_PROC");
            UpdateWeightParam(&mPrevWeightParams,
                              &mPbrp->mCachePrevWeightParams);
            if (muxer_msg.combo_buff.frame_number < last_frame)
                HAL_LOGE("frame process err last_frame%d, new_frame%d",
                         last_frame, muxer_msg.combo_buff.frame_number);
            matched = mPbrp->search_reqlist(muxer_msg.combo_buff.frame_number,
                                            mPbrp->mPrevSavedReqList);

            HAL_LOGD(
                "frame num:%d,matched=%d,status=%d,mFlushing=%d,mCameraId=%d,"
                "mFaceNum=%d",
                muxer_msg.combo_buff.frame_number, matched,
                muxer_msg.combo_buff.status, mPbrp->mFlushing, mPbrp->mCameraId,
                mPbrp->mFaceNum);

            char value[PROPERTY_VALUE_MAX];
            property_get("persist.vendor.cam.manual.choose.wechat.back.replace",
                         value, "0");
            HAL_LOGD("mBGID get %d", atoi(value));
            mPbrp->mCacheBgID = (portraitSceneBgID)atoi(value);

            int64_t nowTime = systemTime();
            if (mPbrp->mChangeBGMode &&
                (ns2ms(nowTime - mPbrp->mSaveTime) > BG_CHANGE_TIME)) {
                mPbrp->mCacheBgID = (portraitSceneBgID)(1 + mPbrp->mCacheBgID);
                HAL_LOGD("mCacheBgID=%d,sub time=%lld", mPbrp->mCacheBgID,
                         nowTime - mPbrp->mSaveTime);
                mPbrp->mSaveTime = nowTime;
                if (mPbrp->mCacheBgID == BG_CHANGE)
                    mPbrp->mCacheBgID = BG_CITY;
                sprintf(value, "%d", mPbrp->mCacheBgID);
                property_set(
                    "persist.vendor.cam.manual.choose.wechat.back.replace",
                    value);
            }
            mPbrp->mBgID = mPbrp->mCacheBgID;

            if (matched == true && mPbrp->mBgID > BG_OFF &&
                muxer_msg.combo_buff.status != CAMERA3_BUFFER_STATUS_ERROR &&
                mPbrp->mFaceNum > 0) {
                void *buffer_addr = NULL;
                void *record_buffer_addr = NULL;
                MAP_AND_CHECK(muxer_msg.combo_buff.buffer, &buffer_addr);
                rc = prevMattingHandle(muxer_msg.combo_buff.buffer, buffer_addr,
                                       NULL, NULL);
                UNMAP_AND_SET_NULL(muxer_msg.combo_buff.buffer, &buffer_addr);
                if (rc != NO_ERROR) {
                    HAL_LOGE("idx=%d, process failed",
                             muxer_msg.combo_buff.frame_number);
                    return false;
                } else {
                    HAL_LOGD("idx=%d, process successfully",
                             muxer_msg.combo_buff.frame_number);
                }
                muxer_msg.mask = mPrevMaskParams.mask;
                muxer_msg.apihandle = mApiSegHandle;
                prevFuseHandle(&muxer_msg);
            } else {
                mPbrp->CallBackResult(&muxer_msg);
            }
        } break;
        default:
            HAL_LOGD("Unknow msg type = %d", muxer_msg.msg_type);
            break;
        }
        last_frame = muxer_msg.combo_buff.frame_number;
    }
    waitMsgAvailable();
    return true;
}
/*===========================================================================
 * FUNCTION   :sprdDepthHandle
 *
 * DESCRIPTION: sprdDepthHandle
 *
 * PARAMETERS :
 *
 * RETURN	  : None
 *==========================================================================*/

bool SprdCamera3PortraitScene::PreviewThread::prevFuseHandle(
    portrait_scene_queue_msg_t *post_msg) {
    bool ret = false;
    portrait_scene_queue_msg_t send_post_msg = *post_msg;

    Mutex::Autolock l(mPbrp->mPrevPostT->mMergequeueMutex);
    HAL_LOGV(
        "prev Enqueue post_msg combo frame:%d for post cpu process!,size=%zu",
        send_post_msg.combo_buff.frame_number,
        mPbrp->mPrevPostT->mPrevPostMsgList.size());
    mPbrp->mPrevPostT->mPrevPostMsgList.push_back(send_post_msg);
    mPbrp->mPrevPostT->mMergequeueSignal.signal();
    return ret;
}
/*===========================================================================
 * FUNCTION   :requestInit
 *
 * DESCRIPTION: request thread init
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::PreviewThread::requestInit() {
    Mutex::Autolock l(mMergequeueMutex);
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    muxer_msg.msg_type = PORTRAIT_SCENE_MSG_INIT;
    mPrevMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
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
void SprdCamera3PortraitScene::PreviewThread::requestExit() {
    HAL_LOGI("PrevT E");
    Mutex::Autolock l(mMergequeueMutex);
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    muxer_msg.msg_type = PORTRAIT_SCENE_MSG_EXIT;
    // mPrevMsgList.clear();
    mPrevMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
    HAL_LOGI("PrevT X");
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
void SprdCamera3PortraitScene::PreviewThread::waitMsgAvailable() {
    while (mPrevMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, PBRP_THREAD_TIMEOUT);
    }
}

/*===========================================================================
 * FUNCTION   :PreviewThread
 *
 * DESCRIPTION: constructor of PreviewThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::PreviewPostThread::PreviewPostThread()
    : mCallbackOps(NULL) {
    HAL_LOGI("E");
    mPrevPostMsgList.clear();
    HAL_LOGI("X");
}
/*===========================================================================
 * FUNCTION   :~PreviewThread
 *
 * DESCRIPTION: deconstructor of PreviewThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
SprdCamera3PortraitScene::PreviewPostThread::~PreviewPostThread() {
    HAL_LOGI(" E");

    mPrevPostMsgList.clear();

    HAL_LOGI("X");
}

/*===========================================================================
 * FUNCTION   :prevFuse
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
int SprdCamera3PortraitScene::PreviewPostThread::prevFuse(
    portrait_scene_queue_msg_t *muxer_msg, void *buffer_addr) {
    int ret = 0;
    int isHorizon = 0;
    int ID2index = 0;
    int64_t fuseStart = systemTime();

    mPbrp->mPrevT->mPrevFuseParams.fuse.data = NULL;
    if (mPbrp->mBgID != BG_COLOR_RETENTION) {
        if (mPbrp->mPrevT->mPrevWeightParams.rotate_angle == 0 ||
            mPbrp->mPrevT->mPrevWeightParams.rotate_angle == 180) {
            isHorizon = 1;
        }
        if (mPbrp->mPrevBgIon[isHorizon][mPbrp->mBgID]) {
            mPbrp->mPrevT->mPrevFuseParams.fuse.data =
                (int8_t *)mPbrp->mPrevBgIon[isHorizon][mPbrp->mBgID]->data;
        }
        ID2index = mPbrp->mBgID * 2 + isHorizon;
        mPbrp->mPrevT->mPrevFuseParams.fuse.isColorRetention = 0;
        /* prev debug */
        if (mPbrp->mdebugPrevSwitch) {
            mPbrp->mPrevT->mPrevFuseParams.fuse.data =
                (int8_t *)mPbrp->mdebugPrev->data;
        }
    } else {
        ID2index = 0;
        mPbrp->mPrevT->mPrevFuseParams.fuse.isColorRetention = 1;
    }

    mPbrp->mPrevT->mPrevFuseParams.src_YUV = (unsigned char *)buffer_addr;
    mPbrp->mPrevT->mPrevFuseParams.dst_YUV = NULL;
    mPbrp->mPrevT->mPrevFuseParams.fuse.mask = muxer_msg->mask;

    if (mPbrp->isWechatClient == true) {
        if (mPbrp->mPrevT->mPrevFuseParams.fuse.isFrontCamera == 0) {
            mPbrp->mPrevT->mPrevFuseParams.fuse.rotate_angle = 90;
        } else {
            mPbrp->mPrevT->mPrevFuseParams.fuse.rotate_angle = 270;
        }
    } else {
        mPbrp->mPrevT->mPrevFuseParams.fuse.rotate_angle =
            mPbrp->mPrevT->mPrevWeightParams.rotate_angle;
    }

    mPbrp->mPrevT->mPrevFuseParams.fuse.total = AI_BGIMG_BUFF_NUM * 2;

    mPbrp->mPrevT->mPrevFuseParams.fuse.fileFormat[ID2index] =
        IMG_FILE_FORMAT_BMP;
    mPbrp->mPrevT->mPrevFuseParams.fuse.width[ID2index] = mPbrp->mBGWidth;
    mPbrp->mPrevT->mPrevFuseParams.fuse.height[ID2index] = mPbrp->mBGHeight;
    if (mPbrp->isWechatClient) {
        mPbrp->mPrevT->mPrevFuseParams.fuse.width[ID2index] = DV_BG_W;
        mPbrp->mPrevT->mPrevFuseParams.fuse.height[ID2index] = DV_BG_H;
    }

    mPbrp->mPrevT->mPrevFuseParams.fuse.index = ID2index;
    HAL_LOGD("isColorRetention:%d;index:%d;total:%d;data:%p;width[0]"
             ":%d;"
             "height[0]:%d;"
             "fileFormat[0]:%d;yuv420[0]:%p;rotate_angle:%d;",
             mPbrp->mPrevT->mPrevFuseParams.fuse.isColorRetention,
             mPbrp->mPrevT->mPrevFuseParams.fuse.index,
             mPbrp->mPrevT->mPrevFuseParams.fuse.total,
             mPbrp->mPrevT->mPrevFuseParams.fuse.data,
             mPbrp->mPrevT->mPrevFuseParams.fuse.width[ID2index],
             mPbrp->mPrevT->mPrevFuseParams.fuse.height[ID2index],
             mPbrp->mPrevT->mPrevFuseParams.fuse.fileFormat[ID2index],
             mPbrp->mPrevT->mPrevFuseParams.fuse.yuv420[ID2index],
             mPbrp->mPrevT->mPrevFuseParams.fuse.rotate_angle);
    if (mPbrp->mIsRunAlgo) {
        ret = sprd_portrait_scene_adpt_ctrl(muxer_msg->apihandle,
                                            SPRD_PORTRAIT_SCENE_FUSE_CMD,
                                            &mPbrp->mPrevT->mPrevFuseParams);
    }
    if (ret != 0) {
        LOGE("preview fusing failed ret %d", ret);
    } else {
        HAL_LOGD("preview fusing FUSE_CMD cost %d ms",
                 (int)ns2ms(systemTime() - fuseStart));
    }
    mPbrp->mPrevMaskBuffList.push_back(muxer_msg->mask);
    return ret;
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

bool SprdCamera3PortraitScene::PreviewPostThread::threadLoop() {
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    portrait_scene_queue_msg_t video_msg;
    memset(&video_msg, 0, sizeof(portrait_scene_queue_msg_t));
    buffer_handle_t *output_buffer = NULL;
    uint32_t frame_number = 0;
    int rc = 0, ID2index = 0, ret = 0, isHorizon = 0;
    bool matched = false;
    char prop_value[PROPERTY_VALUE_MAX] = {
        0,
    };

    HAL_LOGV("PreviewPostThread run");
    while (!mPrevPostMsgList.empty()) {
        List<portrait_scene_queue_msg_t>::iterator it;
        {
            Mutex::Autolock l(mMergequeueMutex);
            it = mPrevPostMsgList.begin();
            muxer_msg = *it;
            mPrevPostMsgList.erase(it);
        }
        switch (muxer_msg.msg_type) {
        case PORTRAIT_SCENE_MSG_INIT: {
            HAL_LOGD("PREV_POST_PORTRAIT_SCENE_MSG_INIT");
        } break;
        case PORTRAIT_SCENE_MSG_EXIT: {
            HAL_LOGD("PREV_POST_PORTRAIT_SCENE_MSG_EXIT");
            return false;
        } break;
        case PORTRAIT_SCENE_MSG_DATA_PROC: {
            HAL_LOGV("PREV_POST_PORTRAIT_SCENE_MSG_DATA_PROC");

            matched = mPbrp->search_reqlist(muxer_msg.combo_buff.frame_number,
                                            mPbrp->mPrevSavedReqList);
            if (matched) {
                void *buffer_addr = NULL;
                MAP_AND_CHECK(muxer_msg.combo_buff.buffer, &buffer_addr);
                prevFuse(&muxer_msg, buffer_addr);
                UNMAP_AND_SET_NULL(muxer_msg.combo_buff.buffer, &buffer_addr);
            }
            HAL_LOGV("idx=%d, fuse result: %d",
                     muxer_msg.combo_buff.frame_number, rc);
            mPbrp->CallBackResult(&muxer_msg);
        } break;
        default:
            HAL_LOGE("Unknow msg type = %d", muxer_msg.msg_type);
            break;
        }
    }
    waitMsgAvailable();

    return true;
}

/*===========================================================================
 * FUNCTION   :requestInit
 *
 * DESCRIPTION: request thread init
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::PreviewPostThread::requestInit() {
    Mutex::Autolock l(mMergequeueMutex);
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    muxer_msg.msg_type = PORTRAIT_SCENE_MSG_INIT;
    mPrevPostMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
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
void SprdCamera3PortraitScene::PreviewPostThread::requestExit() {
    HAL_LOGI("E");
    Mutex::Autolock l(mMergequeueMutex);
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    muxer_msg.msg_type = PORTRAIT_SCENE_MSG_EXIT;
    mPrevPostMsgList.push_back(muxer_msg);
    mMergequeueSignal.signal();
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
void SprdCamera3PortraitScene::PreviewPostThread::waitMsgAvailable() {
    while (mPrevPostMsgList.empty()) {
        Mutex::Autolock l(mMergequeueMutex);
        mMergequeueSignal.waitRelative(mMergequeueMutex, PBRP_THREAD_TIMEOUT);
    }
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
int SprdCamera3PortraitScene::getCameraInfo(int blur_camera_id,
                                            struct camera_info *info) {
    int rc = NO_ERROR;
    int camera_id = 0;
    int32_t img_size = 0;
    struct logicalSensorInfo *logicalPtr = NULL;
    char mPropSize[PROPERTY_VALUE_MAX] = {
        0,
    };
    int32_t jpeg_stream_size = 0;

    if (mStaticMetadata) {
        free_camera_metadata(mStaticMetadata);
        mStaticMetadata = NULL;
    }

    // TBD gaojun
    if (blur_camera_id == SPRD_PORTRAIT_SCENE_FRONT_ID) {
        m_VirtCamera.id = CAM_PBRP_MAIN_ID_2;
    } else {
        m_VirtCamera.id = CAM_PBRP_MAIN_ID;
    }

    camera_id = m_VirtCamera.id;
    HAL_LOGD("camera_id = %d", camera_id);

    SprdCamera3Setting::initDefaultParameters(camera_id);
    rc = SprdCamera3Setting::getStaticMetadata(camera_id, &mStaticMetadata);
    if (rc < 0) {
        return rc;
    }
    CameraMetadata metadata = clone_camera_metadata(mStaticMetadata);
    if (blur_camera_id == SPRD_PORTRAIT_SCENE_FRONT_ID) {
        property_get("persist.vendor.cam.res.portrait.scene.fr", mPropSize,
                     "RES_PS_5M"); //
        HAL_LOGI("front support cap resolution %s", mPropSize);
    } else {
        property_get("persist.vendor.cam.res.portrait.scene.ba", mPropSize,
                     "RES_PS_8M");
        HAL_LOGI("back support cap resolution %s", mPropSize);
    }
    jpeg_stream_size = getJpegStreamSize(mPropSize);
    img_size = jpeg_stream_size * 2 + mMaskSaveSize +
               (PBRP_REFOCUS_PARAM_NUM * sizeof(int32_t)) +
               sizeof(camera3_jpeg_blob);
    HAL_LOGD("jpeg_size=%d", jpeg_stream_size);
    SprdCamera3Setting::s_setting[camera_id].jpgInfo.max_size = img_size;
    metadata.update(ANDROID_JPEG_MAX_SIZE, &img_size, 1);

    addAvailableStreamSize(metadata, mPropSize);

    mStaticMetadata = metadata.release();

    SprdCamera3Setting::getCameraInfo(camera_id, info);

    info->static_camera_characteristics = mStaticMetadata;
    info->conflicting_devices_length = 0;

    HAL_LOGI("X  total_size=%d", img_size);
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
int SprdCamera3PortraitScene::cameraDeviceOpen(__unused int camera_id,
                                               struct hw_device_t **hw_device) {
    int rc = NO_ERROR;
    uint32_t phyId = 0;
    uint8_t master_id = 0;
    struct logicalSensorInfo *logicalPtr = NULL;
    char prop[PROPERTY_VALUE_MAX] = {
        0,
    };
    // property_set("persist.vendor.cam.fb.run_type", "cpu");
    // HAL_LOGD("set fb run in cpu");
    /*property_set("persist.vendor.cam.preview.fps","20");
    HAL_LOGD("set fps 20");*/

    property_get("persist.vendor.cam.fbrp.id", prop, "1");
    m_nPhyCameras = atoi(prop);
    if (camera_id == SPRD_PORTRAIT_SCENE_FRONT_ID) {
        mCameraId = CAM_PBRP_MAIN_ID_2;
        master_id = FRONT_MASTER_ID;
        m_VirtCamera.id = CAM_PBRP_MAIN_ID_2;

    } else {
        mCameraId = CAM_PBRP_MAIN_ID;
        master_id = BACK_MASTER_ID;
        m_VirtCamera.id = CAM_PBRP_MAIN_ID;
    }
    hw_device_t *hw_dev[m_nPhyCameras];
    HAL_LOGI("mCameraId=%d,master_id=%d,m_VirtCamera.id=%d", mCameraId,
             master_id, m_VirtCamera.id);
    setupPhysicalCameras();

    // Open all physical cameras
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        phyId = m_pPhyCamera[i].id;
        HAL_LOGI("open %d", phyId);
        SprdCamera3HWI *hwi = new SprdCamera3HWI((uint32_t)phyId);
        if (!hwi) {
            HAL_LOGE("Allocation of hardware interface failed");
            return NO_MEMORY;
        }
        hw_dev[i] = NULL;

        hwi->setMultiCameraMode(MODE_PORTRAIT_SCENE);
        hwi->setMasterId(master_id);
        rc = hwi->openCamera(&hw_dev[i]);
        if (rc != NO_ERROR) {
            HAL_LOGE("failed, camera id:%d", phyId);
            delete hwi;
            closeCameraDevice();
            return rc;
        }

        m_pPhyCamera[i].dev = reinterpret_cast<camera3_device_t *>(hw_dev[i]);
        m_pPhyCamera[i].hwi = hwi;
    }

    m_VirtCamera.dev.common.tag = HARDWARE_DEVICE_TAG;
#ifdef CONFIG_ISP_2_4
    m_VirtCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_2;
#else
    m_VirtCamera.dev.common.version = CAMERA_DEVICE_API_VERSION_3_5;
#endif
    m_VirtCamera.dev.common.close = close_camera_device;
    m_VirtCamera.dev.ops = &mCameraCaptureOps;
    m_VirtCamera.dev.priv = (void *)&m_VirtCamera;
    *hw_device = &m_VirtCamera.dev.common;

    HAL_LOGI("X");
    return rc;
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
int SprdCamera3PortraitScene::closeCameraDevice() {
    int rc = NO_ERROR;
    sprdcamera_physical_descriptor_t *sprdCam = NULL;
    HAL_LOGI("E");
    if (!mFlushing)
        mFlushing = true;

    bool flag = false;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;

    // Attempt to close all cameras regardless of unbundle results
    for (uint32_t i = 0; i < m_nPhyCameras; i++) {
        sprdCam = &m_pPhyCamera[i];
        hw_device_t *dev = (hw_device_t *)(sprdCam->dev);
        if (dev == NULL) {
            continue;
        }
        HAL_LOGW("camera id:%d", i);
        rc = SprdCamera3HWI::close_camera_device(dev);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error, camera id:%d", i);
        }
        sprdCam->hwi = NULL;
        sprdCam->dev = NULL;
    }
    if (mPrevPostT != NULL) {
        if (mPrevPostT->isRunning()) {
            HAL_LOGI("ready to exit prev post thread");
            mPrevPostT->requestExit();
        }
        mPrevPostT->join();
    }
    if (mPrevT != NULL) {
        if (mPrevT->isRunning()) {
            HAL_LOGI("ready to exit prev thread");
            mPrevT->requestExit();
        }
        mPrevT->join();
    }
    if (mCapPostT != NULL) {
        if (mCapPostT->isRunning()) {
            HAL_LOGI("ready to exit cap post thread");
            mCapPostT->requestExit();
        }
        mCapPostT->join();
    }
    if (mCapT != NULL) {
        if (mCapT->isRunning()) {
            HAL_LOGI("ready to exit cap thread");
            mCapT->requestExit();
        }
        mCapT->join();
    }
    mPbrp->mPrevSavedReqList.clear();
    mPbrp->mCapSavedReqList.clear();
    mPbrp->mVidSavedReqList.clear();
    mCapT->mCapMsgList.clear();
    mCapPostT->mCapPostMsgList.clear();
    mPrevT->mPrevMsgList.clear();
    mPrevPostT->mPrevPostMsgList.clear();
    mUseTime.clear();
    mInitThread = false;
    freeCapBuffer();
    freePrevBuffer();
    if (mPbrp->mPrevT->mApiSegHandle) {
        sprd_portrait_scene_adpt_deinit(mPbrp->mPrevT->mApiSegHandle,
                                        SPRD_PORTRAIT_SCENE_PREVIEW);
        mPbrp->mPrevT->mApiSegHandle = NULL;
    }
    mIsRecordMode = false;
    HAL_LOGD("mIsRecordMode SET 0");
    HAL_LOGI("X, rc: %d", rc);

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
int SprdCamera3PortraitScene::initialize(
    const camera3_callback_ops_t *callback_ops) {
    int rc = NO_ERROR;
    char prop_value[PROPERTY_VALUE_MAX] = {0};
    sprdcamera_physical_descriptor_t sprdCam = m_pPhyCamera[CAM_TYPE_MAIN];
    SprdCamera3HWI *hwiMain = sprdCam.hwi;

    HAL_LOGI("E");
    CHECK_HWI_ERROR(hwiMain);

    mPbrp->mCapT->mSavedResultBuff = NULL;
    mPbrp->mCapT->mSavedCapReqsettings = NULL;
    mCaptureWidth = 0;
    mCaptureHeight = 0;
    mPreviewStreamsNum = 0;
    mCaptureStreamsNum = 0;
    mjpegSize = 0;
    mBgID = BG_OFF;
    mCacheBgID = BG_OFF;
    mIsRecordMode = false;
    HAL_LOGD("mIsRecordMode SET 0");
    m_pOrgJpegBuffer = NULL;
    mFlushing = false;
    mInitThread = false;
    mOrientationAngle = 0;
    mIsRunAlgo = true;
    mReqState = PREVIEW_REQUEST_STATE;
    memset(mPrevMaskBuffArr, 0,
           sizeof(sprd_camera_memory_t *) * PBRP_PREV_TMP_BUFF_NUM);
    SprdCamera3MultiBase::initialize(MODE_PORTRAIT_SCENE, hwiMain);
    mdebugPrev = NULL;
    mdebugPrevSwitch = false;
    property_get("persist.vendor.cam.portrait.prev.debug", prop_value, "0");
    if (atoi(prop_value) != 0) {
        mdebugPrevSwitch = true;
    }

    rc = hwiMain->initialize(sprdCam.dev, &callback_ops_main);
    if (rc != NO_ERROR) {
        HAL_LOGE("Error main camera while initialize !! ");
        return rc;
    }
    HAL_LOGI("HWI initialize rc %d m_nPhyCameras %d", rc, m_nPhyCameras);
    if (m_nPhyCameras == 2) {
        sprdCam = m_pPhyCamera[CAM_TYPE_AUX];
        SprdCamera3HWI *hwiAux = sprdCam.hwi;
        CHECK_HWI_ERROR(hwiAux);

        rc = hwiAux->initialize(sprdCam.dev, &callback_ops_aux);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error aux camera while initialize !! ");
            return rc;
        }
        int on_off = STREAM_ON;
        rc = hwiAux->camera_ioctrl(CAMERA_IOCTRL_COVERED_SENSOR_STREAM_CTRL,
                                   &on_off, NULL);
        if (rc != NO_ERROR) {
            HAL_LOGE("Error while aux camera streamon !! ");
            return rc;
        }
    }
    memset(mLocalCapBuffer, 0, sizeof(new_mem_t) * PBRP_LOCAL_BUFF_NUM);
    memset(&mCapT->mSavedCapRequest, 0, sizeof(camera3_capture_request_t));
    memset(&mCapT->mSavedCapReqStreamBuff, 0, sizeof(camera3_stream_buffer_t));
    memset(&mPrevBgIon, 0,
           sizeof(sprd_camera_memory_t *) * AI_BGIMG_BUFF_NUM * 2);
    mCapPostT->mCallbackOps = callback_ops;
    mPrevPostT->mCallbackOps = callback_ops;

    property_get("persist.vendor.cam.ip.wechat.back.replace", prop_value, "0");
    HAL_LOGD("wechat.enable=%s", prop_value);
    if (!strcmp(prop_value, "1")) {
        mPbrp->isWechatClient = true;
    } else {
        mPbrp->isWechatClient = false;
    }
    mPbrp->initThread();
    HAL_LOGI("X");

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
const camera_metadata_t *
SprdCamera3PortraitScene::constructDefaultRequestSettings(
    const struct camera3_device *device, int type) {
    HAL_LOGV("E");
    const camera_metadata_t *fwk_metadata = NULL;

    SprdCamera3HWI *hw = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    Mutex::Autolock l(mLock);
    if (!hw) {
        HAL_LOGE("NULL camera device");
        return NULL;
    }

    fwk_metadata = hw->construct_default_request_settings(
        m_pPhyCamera[CAM_TYPE_MAIN].dev, type);
    if (!fwk_metadata) {
        HAL_LOGE("constructDefaultMetadata failed");
        return NULL;
    }
    HAL_LOGV("X");
    return fwk_metadata;
}
bool SprdCamera3PortraitScene::checkIsVideo() {
    if ((mPrevT->mPrevInitParams.width == 1920 &&
         mPrevT->mPrevInitParams.height == 1080) ||
        (mPrevT->mPrevInitParams.width == 720 &&
         mPrevT->mPrevInitParams.height == 480) ||
        (mPrevT->mPrevInitParams.width == 1280 &&
         mPrevT->mPrevInitParams.height == 720))
        return true;
    else
        return false;
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
int SprdCamera3PortraitScene::configureStreams(
    const struct camera3_device *device,
    camera3_stream_configuration_t *stream_list) {
    HAL_LOGD("E");

    int rc = 0;
    camera3_stream_t *pMainStreams[PBRP_MAX_NUM_STREAMS];
    size_t i = 0;
    size_t j = 0;
    int w = 0;
    int h = 0;
    int h16_offset = 0;
    int mask_w = SAVE_MASK_W;
    int mask_h = SAVE_MASK_H * SAVE_MASK_UNIT;
    struct stream_info_s stream_info;
    int addStreamNum = 0;
    Mutex::Autolock l(mLock);
    mPbrp->resetVariablesToDefault();

    HAL_LOGD("configurestreams, stream num:%d", stream_list->num_streams);
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        int requestStreamType = getStreamType(stream_list->streams[i]);
        if (requestStreamType == PREVIEW_STREAM) {
            mPreviewStreamsNum = i;
            stream_list->streams[i]->max_buffers = PBRP_PREV_TMP_BUFF_NUM;
            mPrevT->mPrevInitParams.width = stream_list->streams[i]->width;
            mPrevT->mPrevInitParams.height = stream_list->streams[i]->height;
            mPbrp->mBGWidth = mPrevT->mPrevInitParams.width;
            mPbrp->mBGHeight = mPrevT->mPrevInitParams.height;
            if (mPrevT->mFirstPreview) {
                mPrevT->mPrevWeightParams.sel_x =
                    mPrevT->mPrevInitParams.width / 2;
                mPrevT->mPrevWeightParams.sel_y =
                    mPrevT->mPrevInitParams.height / 2;
                mPrevT->mPrevWeightParams.circle_size =
                    mPrevT->mPrevInitParams.height * mCircleSizeScale / 100 / 2;
            }
            mPrevT->requestInit();

            HAL_LOGD("config preview stream[%zu], size: %dx%d, format %d, "
                     "usage %d",
                     i, stream_list->streams[i]->width,
                     stream_list->streams[i]->height,
                     stream_list->streams[i]->format,
                     stream_list->streams[i]->usage);
            HAL_LOGD("prev debug,w=%d,h=%d", mPrevT->mPrevInitParams.width,
                     mPrevT->mPrevInitParams.height);
        } else if (requestStreamType == SNAPSHOT_STREAM) {
            HAL_LOGD("config capture stream[%zu], size: %dx%d format %d, "
                     "usage %d",
                     i, stream_list->streams[i]->width,
                     stream_list->streams[i]->height,
                     stream_list->streams[i]->format,
                     stream_list->streams[i]->usage);
            w = stream_list->streams[i]->width;
            h = stream_list->streams[i]->height;
            if (stream_list->streams[i]->max_buffers == 0)
                stream_list->streams[i]->max_buffers = 1;
// workaround jpeg cant handle 16-noalign issue, when jpeg fix
// this
// issue, we will remove these code
#ifdef CONFIG_CAMERA_MEET_JPG_ALIGNMENT
            if (h == 1944 && w == 2592) {
                h = 1952;
            }
#endif
            // buffer offset for jpg2yuv algo
            if (h == 1944 && w == 2592) {
                h16_offset = 1952 - 1944;
            }

            if ((mCaptureWidth != w && mCaptureHeight != h) ||
                !mIsCapBuffAllocated) {
                freeCapBuffer();
                // buffer:[0]original yuv [1]original jpg [2]mask [3]fuse yuv
                // [4]BG yuv
                if (0 > allocateOne(w, h, &(mLocalCapBuffer[0]), YUV420)) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateOne(w, h, &(mLocalCapBuffer[1]), YUV420)) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateOne(w, h, &(mLocalCapBuffer[2]), YUV420)) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateOne(w, h, &(mLocalCapBuffer[3]), YUV420)) {
                    HAL_LOGE("request one buf failed.");
                }
                if (0 > allocateOne(w, h + h16_offset, &(mLocalCapBuffer[4]),
                                    YUV420)) {
                    HAL_LOGE("request one buf failed.");
                }
                mIsCapBuffAllocated = true;
            }
            mCaptureWidth = w;
            mCaptureHeight = h;
            mCapT->mCapInitParams.width = w;
            mCapT->mCapInitParams.height = h;

            if (mCapT->mFirstCapture) {
                mCapT->mCapWeightParams.sel_x = mCapT->mCapInitParams.width / 2;
                mCapT->mCapWeightParams.sel_y =
                    mCapT->mCapInitParams.height / 2;
                mCapT->mCapWeightParams.circle_size =
                    mCapT->mCapInitParams.height * mCircleSizeScale / 100 / 2;
            }

            mCaptureStreamsNum = stream_list->num_streams;
            mMainStreams[stream_list->num_streams].max_buffers = 1;
            mMainStreams[stream_list->num_streams].width = w;
            mMainStreams[stream_list->num_streams].height = h;
            mMainStreams[stream_list->num_streams].format =
                HAL_PIXEL_FORMAT_YCbCr_420_888;
            mMainStreams[stream_list->num_streams].usage =
                stream_list->streams[i]->usage;
            mMainStreams[stream_list->num_streams].stream_type =
                CAMERA3_STREAM_OUTPUT;
            mMainStreams[stream_list->num_streams].data_space =
                stream_list->streams[i]->data_space;
            mMainStreams[stream_list->num_streams].rotation =
                stream_list->streams[i]->rotation;
            pMainStreams[stream_list->num_streams] =
                &mMainStreams[stream_list->num_streams];

            /* only in DC*/
            if (stream_list->num_streams == 2) {
                addStreamNum++;
            }
        } else if (requestStreamType == VIDEO_STREAM) {
            HAL_LOGD("config video stream[%zu], size: %dx%d format %d, "
                     "usage %d",
                     i, stream_list->streams[i]->width,
                     stream_list->streams[i]->height,
                     stream_list->streams[i]->format,
                     stream_list->streams[i]->usage);
            stream_list->streams[i]->max_buffers = PBRP_PREV_TMP_BUFF_NUM;
            mVideoWidth = stream_list->streams[i]->width;
            mVideoHeight = stream_list->streams[i]->height;
            mIsRecordMode = true;
        } else {
            HAL_LOGD("stream_list->streams[%d],StreamType=%d", i,
                     requestStreamType);
        }
        mMainStreams[i] = *stream_list->streams[i];
        pMainStreams[i] = &mPbrp->mMainStreams[i];
    }

    camera3_stream_configuration mainconfig;

    if (mPbrp->allocateBuff(mPbrp->mBGWidth, mPbrp->mBGHeight) < 0) {
        HAL_LOGE("failed to allocateBuff.");
        return -1;
    }
    if (mPbrp->checkIsVideo()) {
        rc = mPbrp->loadBgImage(SPRD_PORTRAIT_SCENE_VIDEO, 0);
    } else {
        rc = mPbrp->loadBgImage(SPRD_PORTRAIT_SCENE_PREVIEW, 0);
    }
    if (rc < 0) {
        HAL_LOGE("failed to load prev bgimage.");
        return -1;
    }
    mainconfig = *stream_list;
    mainconfig.num_streams = stream_list->num_streams + addStreamNum;
    mainconfig.streams = pMainStreams;

    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->configure_streams(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                    &mainconfig);
    if (rc < 0) {
        HAL_LOGE("failed to configure main streams!!");
        return rc;
    }

    HAL_LOGD("push back to streamlist, IsRecordMode=%d", mIsRecordMode);

    for (i = 0; i < stream_list->num_streams; i++) {
        memcpy(stream_list->streams[i], &mMainStreams[i],
               sizeof(camera3_stream_t));
        HAL_LOGD("main configurestreams, i%d ->streamtype:%d, format:%d, "
                 "usage:%d width:%d, "
                 "height:%d,max_buffers:%d",
                 i, stream_list->streams[i]->stream_type,
                 stream_list->streams[i]->format,
                 stream_list->streams[i]->usage, stream_list->streams[i]->width,
                 stream_list->streams[i]->height,
                 stream_list->streams[i]->max_buffers);
    }

    HAL_LOGD("X");
    return rc;
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
int SprdCamera3PortraitScene::processCaptureRequest(
    const struct camera3_device *device, camera3_capture_request_t *request) {
    int rc = 0;
    int tmpID = 0;
    uint32_t i = 0;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metaSettings;
    camera3_capture_request_t *req = request;
    camera3_capture_request_t req_main;
    camera3_stream_buffer_t *out_streams_main = NULL;
    uint32_t tagCnt = 0;
    int snap_stream_num = 2;
    int af_bypass = 0;
    int fb_on = 0;
    char prop[PROPERTY_VALUE_MAX] = {0};
    memset(&req_main, 0x00, sizeof(camera3_capture_request_t));
    rc = validateCaptureRequest(req);
    if (rc != NO_ERROR) {
        return rc;
    }

    metaSettings = request->settings;
    tagCnt = metaSettings.entryCount();
    if (tagCnt != 0) {
        uint8_t sprdBurstModeEnabled = 0;
        metaSettings.update(ANDROID_SPRD_BURSTMODE_ENABLED,
                            &sprdBurstModeEnabled, 1);
        uint8_t sprdZslEnabled = 1;
        if (!mIsRecordMode) {
            metaSettings.update(ANDROID_SPRD_ZSL_ENABLED, &sprdZslEnabled, 1);
        }
    }
    if (metaSettings.exists(ANDROID_SPRD_PORTRAIT_SCENE_BG_ID)) {
        tmpID =
            metaSettings.find(ANDROID_SPRD_PORTRAIT_SCENE_BG_ID).data.i32[0] -
            AI_BGIMG_ID_OFFSET;
        if (tmpID < BG_OFF || tmpID >= BG_MAX_CNT) {
            mCacheBgID = BG_OFF;
            mChangeBGMode = false;
            HAL_LOGE("Failed to get correct BgID!!");
        } else {
            if (tmpID == BG_CHANGE) {
                mChangeBGMode = true;
                mCacheBgID = BG_CITY;
                mPbrp->mSaveTime = systemTime();
                HAL_LOGD("start changing,mCacheBgID=%d,time=%lld", mCacheBgID,
                         mPbrp->mSaveTime);
            } else {
                mChangeBGMode = false;
                mCacheBgID = (portraitSceneBgID)tmpID;
            }
        }
        char value[PROPERTY_VALUE_MAX];
        if (mPbrp->isWechatClient && mCacheBgID == BG_CHANGE)
            sprintf(value, "%d", mCacheBgID - 1);
        else
            sprintf(value, "%d", mCacheBgID);

        property_set("persist.vendor.cam.manual.choose.wechat.back.replace",
                     value);
        HAL_LOGD("succeed in updating mCacheBgID = %d,mChangeBGMode=%d",
                 mCacheBgID, mChangeBGMode);
    }

    /* save Perfectskinlevel */
    if (metaSettings.exists(ANDROID_SPRD_UCAM_SKIN_LEVEL)) {
        mPbrp->fbLevels.blemishLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[0];
        mPbrp->fbLevels.smoothLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[1];
        mPbrp->fbLevels.skinColor =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[2];
        mPbrp->fbLevels.skinLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[3];
        mPbrp->fbLevels.brightLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[4];
        mPbrp->fbLevels.lipColor =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[5];
        mPbrp->fbLevels.lipLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[6];
        mPbrp->fbLevels.slimLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[7];
        mPbrp->fbLevels.largeLevel =
            metaSettings.find(ANDROID_SPRD_UCAM_SKIN_LEVEL).data.i32[8];
    }
    /*config main camera*/
    req_main = *req;
    out_streams_main = (camera3_stream_buffer_t *)malloc(
        sizeof(camera3_stream_buffer_t) * (req_main.num_output_buffers));
    if (!out_streams_main) {
        HAL_LOGE("failed");
        return NO_MEMORY;
    }
    memset(out_streams_main, 0x00,
           (sizeof(camera3_stream_buffer_t)) * (req_main.num_output_buffers));
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        int requestStreamType =
            getStreamType(request->output_buffers[i].stream);
        if (requestStreamType == VIDEO_STREAM ||
            requestStreamType == DEFAULT_STREAM) {
            if (i == 0) {
                out_streams_main[0] = req->output_buffers[1];
            }
            req_main.num_output_buffers--;
        } else {
            out_streams_main[i] = req->output_buffers[i];
        }

        HAL_LOGD("idx:%d, buffer num:%d, streamtype:%d", req->frame_number,
                 req->num_output_buffers, requestStreamType);
        if (requestStreamType == SNAPSHOT_STREAM) {
            portrait_scene_queue_msg_t read_BG_msg;
            memset(&read_BG_msg, 0, sizeof(portrait_scene_queue_msg_t));
            read_BG_msg.msg_type = PORTRAIT_SCENE_MSG_DATA_PROC;
            mPbrp->mCapBgID = mCacheBgID;
            UpdateWeightParam(&mCapT->mCapWeightParams,
                              &mPbrp->mCacheCapWeightParams);
            mCapT->capReadHandle(&read_BG_msg);
            mCapT->mSavedResultBuff = request->output_buffers[i].buffer;
            mjpegSize = ADP_WIDTH(*request->output_buffers[i].buffer);
            memcpy(&mCapT->mSavedCapRequest, req,
                   sizeof(camera3_capture_request_t));
            memcpy(&mCapT->mSavedCapReqStreamBuff, &req->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
            req->output_buffers[i].stream->reserved[0] = NULL;
            mSavedReqStreams[mCaptureStreamsNum - 1] =
                req->output_buffers[i].stream;
            HAL_LOGD("capInfo:req,mFlushing:%d,frame_number:%d", mFlushing,
                     request->frame_number);
            mPbrp->mCurrCapFrameNum = request->frame_number;

            snap_stream_num = 2;
            out_streams_main[i].buffer = &mLocalCapBuffer[0].native_handle;
            HAL_LOGD("-----request output buffer addr:%p",
                     out_streams_main[i].buffer);
            hwiMain->camera_ioctrl(CAMERA_IOCTRL_SET_CAPTURE_FACE_BEAUTIFY,
                                   &fb_on, NULL);
            out_streams_main[i].stream =
                &mPbrp->mMainStreams[mPbrp->mCaptureStreamsNum];

            mReqState = WAIT_FIRST_YUV_STATE;
        } else if (requestStreamType == PREVIEW_STREAM ||
                   requestStreamType == CALLBACK_STREAM) {
            out_streams_main[i].stream = &mPbrp->mMainStreams[i];
            updateWeightParams(metaSettings, 0);
        } else {
            out_streams_main[i].stream = &mPbrp->mMainStreams[i];
            updateWeightParams(metaSettings, 0);
        }
        HAL_LOGD("thomas Type=%d", requestStreamType);
    }
    req_main.output_buffers = out_streams_main;
    req_main.settings = metaSettings.release();
    for (size_t i = 0; i < req->num_output_buffers; i++) {
        if (getStreamType(request->output_buffers[i].stream) ==
            SNAPSHOT_STREAM) {
            if (NULL != mCapT->mSavedCapReqsettings) {
                free_camera_metadata(mCapT->mSavedCapReqsettings);
                mCapT->mSavedCapReqsettings = NULL;
            }
            mCapT->mSavedCapReqsettings =
                clone_camera_metadata(req_main.settings);
        }
    }

    saveRequest(request, req_main.settings);

    for (size_t i = 0; i < request->num_output_buffers; i++) {
        HAL_LOGD("request[%d].stream=%d,buff=%p", i,
                 getStreamType(request->output_buffers[i].stream),
                 request->output_buffers[i].buffer);
        HAL_LOGD("req_main[%d].stream=%d,buff=%p", i,
                 getStreamType(req_main.output_buffers[i].stream),
                 req_main.output_buffers[i].buffer);
    }
    HAL_LOGD("send oem buffer[0]=%p", req_main.output_buffers[0].buffer);

    rc = hwiMain->process_capture_request(m_pPhyCamera[CAM_TYPE_MAIN].dev,
                                          &req_main);

    if (rc < 0) {
        HAL_LOGE("failed, mReqState:%d  idx:%d", mReqState,
                 req_main.frame_number);
        goto req_fail;
    }

req_fail:
    if (req_main.settings != NULL) {
        free_camera_metadata(
            const_cast<camera_metadata_t *>(req_main.settings));
        req_main.settings = NULL;
    }
    if (req_main.output_buffers != NULL) {
        free(out_streams_main);
        req_main.output_buffers = NULL;
    }

    return rc;
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
void SprdCamera3PortraitScene::processCaptureResultMain(
    camera3_capture_result_t *result) {
    uint32_t cur_frame_number = result->frame_number;
    uint32_t searchnotifyresult = NOTIFY_NOT_FOUND;
    const camera3_stream_buffer_t *result_buffer = result->output_buffers;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    CameraMetadata metadata;
    char prop_value[PROPERTY_VALUE_MAX] = {
        0,
    };

    metadata = result->result;
    HAL_LOGD("frame_number=%d,mReqState:%d, buff num:%d", result->frame_number,
             mReqState, result->num_output_buffers);
    /* Direclty pass preview buffer and meta result for Main camera */
    if (result_buffer == NULL && result->result != NULL) {
        HAL_LOGD("idx=%d,metadata process", result->frame_number);
        if (search_reqlist(result->frame_number, mCapSavedReqList) == true &&
            0 != result->frame_number) {
            if (mReqState != WAIT_FIRST_YUV_STATE) {
                HAL_LOGD("throw snapshot metadata, framenumber:%d",
                         result->frame_number);
                return;
            } else {
                mCapPostT->mCallbackOps->process_capture_result(
                    mCapPostT->mCallbackOps, result);
                return;
            }
        }

        if (mReqState == PREVIEW_REQUEST_STATE) {
            updateWeightParams(metadata, 1);
        }

        camera3_capture_result_t new_result = *result;
        new_result.result = metadata.release();
        mCapPostT->mCallbackOps->process_capture_result(mCapPostT->mCallbackOps,
                                                        &new_result);
        free_camera_metadata(
            const_cast<camera_metadata_t *>(new_result.result));

        HAL_LOGD("callbackmetadata cur_frame_number:%d mReqState:%d",
                 cur_frame_number, mReqState);
        return;
    }

    if (result_buffer == NULL) {
        HAL_LOGE("result_buffer is NULL");
        return;
    }

    int currStreamType = getStreamType(result_buffer->stream);
    HAL_LOGD("resultmain");
    if (mReqState != PREVIEW_REQUEST_STATE &&
        currStreamType == DEFAULT_STREAM) {
        HAL_LOGD("capInfo:get yuv,framenumber:%d, receive yuv:%d",
                 cur_frame_number, mReqState);
        hwiMain->setMultiCallBackYuvMode(false);
        EnQResultMsg(result);
        HAL_LOGI("mReqState %d", mPbrp->mReqState);
    } else if (mReqState != PREVIEW_REQUEST_STATE &&
               currStreamType == SNAPSHOT_STREAM) {
        uint32_t jpeg_size = 0;
        uint8_t *jpeg_addr = NULL;
        if (mPbrp->map(result->output_buffers->buffer, (void **)(&jpeg_addr)) ==
            NO_ERROR) {
            jpeg_size = getJpegSize(jpeg_addr,
                                    ADP_WIDTH(*result->output_buffers->buffer));
            UNMAP_AND_SET_NULL(result->output_buffers->buffer, &jpeg_addr);
        } else {
            HAL_LOGE("map buffer(%p) failed", result->output_buffers->buffer);
        }

        if (mOrgJpegSize > 0)
            mCapT->saveCaptureParams(result->output_buffers->buffer, jpeg_size);

        CallSnapBackResult(result, CAMERA3_BUFFER_STATUS_OK);
        mReqState = PREVIEW_REQUEST_STATE;
    } else {
        EnQResultMsg(result);
    }
    return;
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
void SprdCamera3PortraitScene::notifyMain(const camera3_notify_msg_t *msg) {
    if (msg->type == CAMERA3_MSG_SHUTTER &&
        (true ==
         search_reqlist(msg->message.shutter.frame_number, mCapSavedReqList))) {
        if (mReqState != WAIT_FIRST_YUV_STATE) {
            HAL_LOGD(" hold yuv cap notify");
            return;
        }
        HAL_LOGD("jpg cap notify");
    }
    mCapPostT->mCallbackOps->notify(mCapPostT->mCallbackOps, msg);
}

/*===========================================================================
 * FUNCTION   :dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::_dump(const struct camera3_device *device,
                                     int fd) {
    HAL_LOGI(" E");

    HAL_LOGI("X");
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
int SprdCamera3PortraitScene::_flush(const struct camera3_device *device) {
    int rc = 0;
    mFlushing = true;

    HAL_LOGI("E m_nPhyCameras=%d, mCapMsgList.size=%zu, "
             "mPrevSavedReqList.size:%zu,mVidSavedReqList.size:%zu",
             m_nPhyCameras, mCapT->mCapMsgList.size(), mPrevSavedReqList.size(),
             mVidSavedReqList.size());

    List<request_saved_msg_t>::iterator i1 = mPrevSavedReqList.begin();
    while (i1 != mPrevSavedReqList.end()) {
        HAL_LOGD("prev list left frame =%d", i1->frame_number);
        i1++;
    }

    // wait for cap process
    Mutex::Autolock l(mPbrp->mCapLock);

    if (mPrevT != NULL) {
        if (mPrevT->isRunning()) {
            HAL_LOGI("prev thread request exit");
            mPrevT->requestExit();
        }
        mPrevT->join();
    }

    if (mPrevPostT != NULL) {
        if (mPrevPostT->isRunning()) {
            HAL_LOGI("prev post thread request exit");
            mPrevPostT->requestExit();
        }
        mPrevPostT->join();
    }
    if (mCapT != NULL) {
        if (mCapT->isRunning()) {
            HAL_LOGI("cap thread request exit");
            mCapT->requestExit();
        }
        mCapT->join();
    }
    if (mCapPostT != NULL) {
        if (mCapPostT->isRunning()) {
            HAL_LOGI("cap post thread request exit");
            mCapPostT->requestExit();
        }
        mCapPostT->join();
    }
    mInitThread = false;
    SprdCamera3HWI *hwiMain = m_pPhyCamera[CAM_TYPE_MAIN].hwi;
    rc = hwiMain->flush(m_pPhyCamera[CAM_TYPE_MAIN].dev);

    if (2 == m_nPhyCameras) {
        SprdCamera3HWI *hwiAux = m_pPhyCamera[CAM_TYPE_AUX].hwi;
        rc = hwiAux->flush(m_pPhyCamera[CAM_TYPE_AUX].dev);
    }
    if (mPbrp->mPrevT->mApiSegHandle) {
        sprd_portrait_scene_adpt_deinit(mPbrp->mPrevT->mApiSegHandle,
                                        SPRD_PORTRAIT_SCENE_PREVIEW);
        mPbrp->mPrevT->mApiSegHandle = NULL;
    }
    HAL_LOGI("X");

    return rc;
}

/*===========================================================================
 * FUNCTION   :initThread
 *
 * DESCRIPTION: initThread
 *
 * PARAMETERS : none
 *
 * RETURN     : None
 *==========================================================================*/
int SprdCamera3PortraitScene::initThread() {
    int rc = NO_ERROR;

    HAL_LOGI("E");
    mCapT->run(String8::format("PbrpCap").string(), -7);
    mCapT->initCapParams();
    mCapT->initCapWeightParams();
    mCapPostT->run(String8::format("PbrpCapPost").string());
    mPrevT->run(String8::format("PbrpPrev").string());
    mPrevT->initPrevInitParams();
    mPrevT->initPrevWeightParams();
    mPrevPostT->run(String8::format("PbrpPrevPost").string());
    mInitThread = true;
    HAL_LOGI("X");
    return rc;
}

int SprdCamera3PortraitScene::resetVariablesToDefault() {
    int rc = NO_ERROR;
    char prop[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.vendor.cam.portrait.runalgo", prop, "1");
    if (atoi(prop) == 0) {
        mPbrp->mIsRunAlgo = false;
    }

    mIsRecordMode = false;
    HAL_LOGD("mIsRecordMode SET 0");
    mFlushing = false;
    if (!mInitThread) {
        mPbrp->initThread();
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : freeLocalCapBuffer
 *
 * DESCRIPTION: free new_mem_t buffer
 *
 * PARAMETERS:
 *
 * RETURN    :  NONE
 *==========================================================================*/
void SprdCamera3PortraitScene::freeCapBuffer() {
    for (size_t i = 0; i < PBRP_LOCAL_BUFF_NUM; i++) {
        new_mem_t *localBuffer = &mLocalCapBuffer[i];
        freeOneBuffer(localBuffer);
    }
    mIsCapBuffAllocated = false;
}

void SprdCamera3PortraitScene::freePrevBuffer() {
    HAL_LOGI("E");
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < AI_BGIMG_BUFF_NUM; i++) {
            if (mPrevBgIon[j][i]) {
                mPbrp->freeIonMem(mPrevBgIon[j][i]);
                mPrevBgIon[j][i] = NULL;
            }
        }
    }

    for (int k = 0; k < PBRP_PREV_TMP_BUFF_NUM; k++) {
        mPbrp->freeIonMem(mPrevMaskBuffArr[k]);
    }
    mPrevMaskBuffList.clear();
    if (mdebugPrev)
        mPbrp->freeIonMem(mdebugPrev);

    HAL_LOGI("X");
}

int SprdCamera3PortraitScene::allocateBuff(int w, int h) {
    int rc = 0;
    /* fixed head 14+40 */
    int bitmapheader = 0; // 54
    int bitdepth = 24;
    size_t bgimgsize = 0;
    sprd_camera_memory_t *memory = NULL;
    HAL_LOGI(":E");
    freePrevBuffer();
    if ((w == 1920 && h == 1080) || (w == 720 && h == 480)) {
        w = 1280;
        h = 720;
    }

    /* lock for pre-init mask_size */
    Mutex::Autolock l(mPrevBGLock);
    bgimgsize = w * h * bitdepth / 8 + (bitmapheader);
    for (int j = 0; j < 2; j++) {
        for (size_t i = 0; i < AI_BGIMG_BUFF_NUM; i++) {
            memory = allocateIonMem(bgimgsize, 1, true);
            if (NULL == memory) {
                HAL_LOGE("error memory is null.");
                goto mem_fail;
            }
            HAL_LOGD("bgion=%p", memory);
            mPrevBgIon[j][i] = memory;
        }
    }

    for (int k = 0; k < PBRP_PREV_TMP_BUFF_NUM; k++) {
        mPrevMaskBuffArr[k] =
            mPbrp->allocateIonMem(mPrevT->mPrevInitParams.mask_size, 1, true);
        if (!mPrevMaskBuffArr[k]) {
            HAL_LOGE("Failed to malloc ion buffer");
            goto mem_fail;
        }
        mPrevMaskBuffList.push_back((uint16_t *)mPrevMaskBuffArr[k]->data);
        HAL_LOGD("mPrevMaskBuffArr[%d].data=%p", k, mPrevMaskBuffArr[k]->data);
    }

    HAL_LOGI(":X");
    return rc;

mem_fail:
    freePrevBuffer();

    return -1;
}

int SprdCamera3PortraitScene::loadBgImageInternal(sprd_camera_memory_t *ion,
                                                  char *path) {
    int ret = 0;
    int file_len = 0;
    FILE *fp = NULL;
    int fileHeader = 54;

    HAL_LOGD("loading path=%s", path);
    fp = fopen(path, "rb");
    if (!fp) {
        HAL_LOGE("Failed to open file");
        ret = -1;
        goto exit;
    }
    if (NO_ERROR != (ret = fseek(fp, 0, SEEK_END))) {
        HAL_LOGE("error code =%d", ret);
        goto exit;
    }

    file_len = ftell(fp);
    if (NO_ERROR != (ret = fseek(fp, fileHeader, SEEK_SET))) {
        HAL_LOGE("error code =%d", ret);
        goto exit;
    }
    if (ion->phys_size >= file_len - fileHeader && file_len > 0) {
        file_len = fread(ion->data, 1, file_len - fileHeader, fp);
    } else {
        HAL_LOGE("Failed to read file, buffer is too small! buffer "
                 "size=%d,file size=%d",
                 ion->phys_size, file_len - fileHeader);
        ret = -1;
    }
exit:
    if (fp)
        fclose(fp);
    return ret;
}

int SprdCamera3PortraitScene::filesize(FILE *fp) {
    int pos = 0;
    int ret = 0;
    unsigned int pos_cur = 0;
    pos_cur = ftell(fp);
    ret = fseek(fp, 0, SEEK_END);
    if (NO_ERROR != ret) {
        HAL_LOGE("error code =%d", ret);
        return ret;
    }
    pos = ftell(fp);
    HAL_LOGD("loadbg pos=%d", pos);
    ret = fseek(fp, pos_cur, SEEK_SET);
    if (NO_ERROR != ret) {
        HAL_LOGE("error code =%d", ret);
        return ret;
    }
    return pos;
}

int SprdCamera3PortraitScene::loadBgImage(sprd_portrait_scene_channel_t ch,
                                          int isHorizon) {
    int ret = 0;
    char temStr[4];
    int bitMapHeader = 54;
    int bitDepth = 24;
    size_t imgSize = 0;
    buffer_handle_t *const BG_buffer = &mPbrp->mLocalCapBuffer[4].native_handle;
    void *BG_buff_addr = NULL;
    FILE *fp = NULL;
    FILE *fp_yuv = NULL;
    HAL_LOGD(": E, channel=%d", ch);
    int tmpW = mCaptureWidth, tmpH = mCaptureHeight;
    int videoW = mVideoHeight, videoH = mVideoHeight;
    char path[255] = {0};
    char prop[255] = {0};
    new_mem_t jpg_buff;
    memset(&jpg_buff, 0, sizeof(jpg_buff));
    switch (ch) {
    case SPRD_PORTRAIT_SCENE_VIDEO:
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < AI_BGIMG_BUFF_NUM; i++) {
                strcpy(path, AI_BG_VID_IMG_PATH);
                sprintf(temStr, "%d", j);
                strcat(path, temStr);
                strcat(path, "_");
                sprintf(temStr, "%d", i);
                strcat(path, temStr);
                strcat(path, ".bmp");
                loadBgImageInternal(mPrevBgIon[j][i], path);
            }
        }
        break;
    case SPRD_PORTRAIT_SCENE_PREVIEW:
        for (int j = 0; j < 2; j++) {
            for (int i = 0; i < AI_BGIMG_BUFF_NUM; i++) {
                strcpy(path, AI_BG_PREV_IMG_PATH);
                sprintf(temStr, "%d", j);
                strcat(path, temStr);
                strcat(path, "_");
                sprintf(temStr, "%d", i);
                strcat(path, temStr);
                strcat(path, ".bmp");
                loadBgImageInternal(mPrevBgIon[j][i], path);
            }
        }

        if (mdebugPrevSwitch) {
            if (mCameraId == CAM_PBRP_MAIN_ID) {
                strcpy(path, AI_BG_REAR_CAP_IMG_PATH);
            } else {
                strcpy(path, AI_BG_FRONT_CAP_IMG_PATH);
            }
            strcat(path, "0_0.bmp");
            if (!mdebugPrev)
                mdebugPrev = mPbrp->allocateIonMem(
                    mCaptureHeight * mCaptureWidth * 3, 1, true);
            loadBgImageInternal(mdebugPrev, path);
        }
        break;
    case SPRD_PORTRAIT_SCENE_CAPTURE: {
        if (mCapBgID < BG_CITY) {
            goto exit;
        }

        if (mCameraId == CAM_PBRP_MAIN_ID) {
            strcpy(path, AI_BG_REAR_CAP_IMG_PATH);
        } else {
            strcpy(path, AI_BG_FRONT_CAP_IMG_PATH);
        }
        MAP_AND_CHECK(BG_buffer, &BG_buff_addr);
        sprintf(temStr, "%d", isHorizon);
        strcat(path, temStr);
        strcat(path, "_");
        sprintf(temStr, "%d", mCapBgID);
        strcat(path, temStr);
        strcat(path, ".jpg");
        int file_len = 0;

        fp = fopen(path, "rb");
        if (!fp) {
            HAL_LOGE("Failed to open file");
            ret = BAD_VALUE;
            goto exit;
        }
        file_len = filesize(fp);
        if (0 > allocateOne(mPbrp->mCaptureWidth, mPbrp->mCaptureHeight,
                            &(jpg_buff), YUV420)) {
            HAL_LOGE("request one buffer failed.");
            ret = BAD_VALUE;
            goto exit;
        }
        MAP_AND_CHECK(&jpg_buff.native_handle, &jpg_buff.vir_addr);
        if (file_len > 0) {
            fread(jpg_buff.vir_addr, file_len, 1, fp);
        } else {
            ret = BAD_VALUE;
            HAL_LOGE("Failed to get file_len = %d", file_len);
            goto exit;
        }

        mPbrp->jpeg_decode_to_yuv(&jpg_buff.native_handle, jpg_buff.vir_addr,
                                  BG_buffer, BG_buff_addr,
                                  mPbrp->m_pPhyCamera[CAM_TYPE_MAIN].hwi);
    } break;
    default:
        HAL_LOGE("Failed to get correct channel");
        ret = BAD_VALUE;
        break;
    }
exit:
    if (jpg_buff.vir_addr) {
        UNMAP_AND_SET_NULL(&jpg_buff.native_handle, &jpg_buff.vir_addr);
        mPbrp->freeOneBuffer(&jpg_buff);
    }
    if (BG_buff_addr) {
        UNMAP_AND_SET_NULL(BG_buffer, &BG_buff_addr);
    }
    if (fp) {
        fclose(fp);
    }
    HAL_LOGD(": E, ret=%d", ret);
    return ret;
}

bool SprdCamera3PortraitScene::search_reqlist(int frame_num,
                                              List<request_saved_msg_t> list) {
    int ret = false;

    Mutex::Autolock l(mPbrp->mRequestLock);
    List<request_saved_msg_t>::iterator i = list.begin();
    while (i != list.end()) {
        if (i->frame_number == frame_num) {
            ret = true;
            HAL_LOGV(
                "search_from_reqlist find matched frame number%d from reqlist",
                frame_num);
            break;
        }
        i++;
    }
    return ret;
}

bool SprdCamera3PortraitScene::search_restlist(
    int frame_num, List<portrait_scene_queue_msg_t> list) {
    int ret = false;

    Mutex::Autolock l(mPbrp->mRequestLock);
    List<portrait_scene_queue_msg_t>::iterator i = list.begin();
    while (i != list.end()) {
        if (i->combo_buff.frame_number == frame_num) {
            ret = true;
            HAL_LOGV("search_from_reqlist find frame number%d from list",
                     frame_num);
            break;
        }
        i++;
    }

    return ret;
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
void SprdCamera3PortraitScene::saveRequest(camera3_capture_request_t *request,
                                           const camera_metadata_t *settings) {
    size_t i = 0;
    request_saved_msg_t currRequest;
    camera3_stream_t *newStream = NULL;
    bzero(&currRequest, sizeof(request_saved_msg_t));

    for (i = 0; i < request->num_output_buffers; i++) {
        newStream = (request->output_buffers[i]).stream;
        newStream->reserved[0] = NULL;
        if (getStreamType(newStream) == CALLBACK_STREAM ||
            getStreamType(newStream) == PREVIEW_STREAM) {
            HAL_LOGD("save prev request %d, buffer %p", request->frame_number,
                     request->output_buffers[i].buffer);
            Mutex::Autolock l(mRequestLock);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            mPrevSavedReqList.push_back(currRequest);

            mPbrp->printUseTime(request->frame_number, (char *)"request");

        } else if (getStreamType(newStream) == SNAPSHOT_STREAM) {
            currRequest.output_buffer = (camera3_stream_buffer_t *)malloc(
                sizeof(camera3_stream_buffer_t));
            memset(currRequest.output_buffer, 0,
                   sizeof(camera3_stream_buffer_t));
            memcpy(currRequest.output_buffer, &request->output_buffers[i],
                   sizeof(camera3_stream_buffer_t));
            currRequest.savedCapRequest = (camera3_capture_request_t *)malloc(
                sizeof(camera3_capture_request_t));
            memset(currRequest.savedCapRequest, 0,
                   sizeof(camera3_capture_request_t));
            memcpy(currRequest.savedCapRequest, request,
                   sizeof(camera3_capture_request_t));
            HAL_LOGD("save snapshot request %d", request->frame_number);
            HAL_LOGV("output_buffer status %d, acquire_fence %d",
                     currRequest.output_buffer->status,
                     currRequest.output_buffer->acquire_fence);
            Mutex::Autolock l(mRequestLock);
            currRequest.settings = clone_camera_metadata(settings);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            HAL_LOGD("enqreq combo_buff.buffer 0x%p", currRequest.buffer);
            currRequest.stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            mCapSavedReqList.push_back(currRequest);
        } else if (getStreamType(newStream) == VIDEO_STREAM) {
            HAL_LOGD("save video request %d, buffer %p", request->frame_number,
                     request->output_buffers[i].buffer);
            Mutex::Autolock l(mPbrp->mLock);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.stream = request->output_buffers[i].stream;
            currRequest.input_buffer = request->input_buffer;
            mVidSavedReqList.push_back(currRequest);
        } else if (getStreamType(newStream) == DEFAULT_STREAM) {
            HAL_LOGD("save default request %d, buffer %p(wechat setup)",
                     request->frame_number, request->output_buffers[i].buffer);
            Mutex::Autolock l(mRequestLock);
            currRequest.frame_number = request->frame_number;
            currRequest.buffer = request->output_buffers[i].buffer;
            currRequest.stream = request->output_buffers[i].stream;
            HAL_LOGD("DEFAULT_STREAM request->output_buffers[%d].stream=%p", i,
                     currRequest.stream);
            currRequest.input_buffer = request->input_buffer;
            mDefaultSavedReqList.push_back(currRequest);
        } else {
            HAL_LOGE("failed stream type=!!!");
        }
    }
}
void SprdCamera3PortraitScene::printUseTime(uint32_t frame_num, char *tag) {
    static uint32_t frame_cnt = 0;
    static int64_t basic_time = systemTime();
    int ms = 0;
    portrait_time_t time;

    Mutex::Autolock l(mTimeLock);
    auto itor = mUseTime.begin();
    int64_t now = systemTime();
    /* 1.record time between request and result
     * 2.record time between result and callback
     * 3.record fps in 3 second
     */
    if (0 == strcmp(tag, "request")) {
        time.idx = frame_num;
        time.time = now;
        mUseTime.push_back(time);
    } else {
        while (itor != mUseTime.end()) {
            if (itor->idx != frame_num) {
                itor++;
                continue;
            }

            if (0 == strcmp(tag, "result")) {
                HAL_LOGD("idx=%d req2res,cost %d ms", frame_num,
                         (int)ns2ms(now - itor->time));
                itor->time = now;
            } else if (0 == strcmp(tag, "callback")) {
                HAL_LOGD("idx=%d res2callback,cost %d ms", frame_num,
                         (int)ns2ms(now - itor->time));
                ms = (int)ns2ms(now - basic_time);
                if (ms > 3000) {
                    HAL_LOGD("FPS=%.1f", frame_cnt / (ms / 1000.0));
                    frame_cnt = 0;
                    basic_time = now;
                }
                frame_cnt++;
                mUseTime.erase(itor);
            }
            return;
        }
    }
}
/*===========================================================================
 * FUNCTION   :EnQResultMsg
 *
 * DESCRIPTION: save buffer in request
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::EnQResultMsg(camera3_capture_result_t *result) {
    size_t i = 0;
    camera3_stream_t *newStream = NULL;
    portrait_scene_queue_msg_t msg;
    memset(&msg, 0, sizeof(portrait_scene_queue_msg_t));
    portrait_scene_queue_msg_t muxer_msg;

    if (mFlushing ||
        result->output_buffers[0].status == CAMERA3_BUFFER_STATUS_ERROR) {
        HAL_LOGD("flushing callbackprev idx=%d,buff status=%d",
                 result->frame_number, result->output_buffers[0].status);
        muxer_msg.combo_buff.frame_number = result->frame_number;
        muxer_msg.combo_buff.buffer = result->output_buffers[0].buffer;
        muxer_msg.combo_buff.stream = result->output_buffers[0].stream;
        muxer_msg.combo_buff.status = CAMERA3_BUFFER_STATUS_ERROR;
        CallBackResult(&muxer_msg);
        return;
    }
    for (i = 0; i < result->num_output_buffers; i++) {
        newStream = (result->output_buffers[i]).stream;
        msg.msg_type = PORTRAIT_SCENE_MSG_DATA_PROC;
        msg.combo_buff.frame_number = result->frame_number;
        msg.combo_buff.input_buffer = result->input_buffer;
        msg.combo_buff.buffer = result->output_buffers[i].buffer;
        msg.combo_buff.stream = result->output_buffers[i].stream;
        msg.combo_buff.status = result->output_buffers[i].status;
        HAL_LOGD("save result %d, buffer %p, streamType=%d",
                 result->frame_number, msg.combo_buff.buffer,
                 getStreamType(newStream));
        if (getStreamType(newStream) == CALLBACK_STREAM) {
            printUseTime(result->frame_number, (char *)"result");
            Mutex::Autolock l(mPrevT->mMergequeueMutex);
            mPrevT->mPrevMsgList.push_back(msg);
            mPrevT->mMergequeueSignal.signal();
        } else if (getStreamType(newStream) == DEFAULT_STREAM) {
            Mutex::Autolock l(mCapT->mMergequeueMutex);
            mCapT->mCapMsgList.push_back(msg);
            mCapT->mMergequeueSignal.signal();
        } else if (getStreamType(newStream) == SNAPSHOT_STREAM) {
            int matched = mPbrp->search_reqlist(msg.combo_buff.frame_number,
                                                mPbrp->mDefaultSavedReqList);
            if (matched) {
                CallBackPrevResultInternal(&msg);
            }
        } else {
            HAL_LOGE("failed to get stream type!!!");
        }
    }
}

/*===========================================================================
 * FUNCTION   : setupPhysicalCameras
 *
 * DESCRIPTION: Creates Camera Capture if not created
 *
 * RETURN     :
 *              NO_ERROR  : success
 *              other: non-zero failure code
 *==========================================================================*/
int SprdCamera3PortraitScene::setupPhysicalCameras() {
    m_pPhyCamera = new sprdcamera_physical_descriptor_t[m_nPhyCameras];
    if (!m_pPhyCamera) {
        HAL_LOGE("Error allocating camera info buffer!!");
        return NO_MEMORY;
    }
    memset(m_pPhyCamera, 0x00,
           (m_nPhyCameras * sizeof(sprdcamera_physical_descriptor_t)));
    if (mCameraId == 0) {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_PBRP_MAIN_ID;
        if (2 == m_nPhyCameras) {
            m_pPhyCamera[CAM_TYPE_AUX].id = CAM_PBRP_AUX_ID;
        }
    } else {
        m_pPhyCamera[CAM_TYPE_MAIN].id = CAM_PBRP_MAIN_ID_2;
        if (2 == m_nPhyCameras) {
            m_pPhyCamera[CAM_TYPE_AUX].id = CAM_PBRP_AUX_ID_2;
        }
    }

    return NO_ERROR;
}
/*===========================================================================
 * FUNCTION   :CallBackPrevResultInternal
 *
 * DESCRIPTION: process preview Request
 *
 * PARAMETERS :
 *    @device: camera3 device
 *    @request:camera3 request
 * RETURN     :
 *==========================================================================*/
int SprdCamera3PortraitScene::CallBackPrevResultInternal(
    portrait_scene_queue_msg_t *muxer_msg) {
    camera3_capture_result_t newResult;
    camera3_stream_buffer_t newOutput_buffers;
    memset(&newResult, 0, sizeof(camera3_capture_result_t));
    memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));

    if (mPbrp->isWechatClient == true) {
        int rc = 0;
        bool callbackflag = false;
        void *output_buf1_addr = NULL;
        void *output_callback_addr = NULL;
        camera3_capture_result_t callbackResult;
        camera3_stream_buffer_t newOutputCallback_buffers;
        memset(&callbackResult, 0, sizeof(camera3_capture_result_t));
        memset(&newOutputCallback_buffers, 0, sizeof(camera3_stream_buffer_t));

        List<request_saved_msg_t> *mDefaultReqList = NULL;
        mDefaultReqList = &mDefaultSavedReqList;
        List<request_saved_msg_t>::iterator itor1;

        itor1 = mDefaultReqList->begin();
        while (itor1 != mDefaultReqList->end()) {
            if (itor1->frame_number == muxer_msg->combo_buff.frame_number) {
                newOutputCallback_buffers.stream = itor1->stream;
                newOutputCallback_buffers.buffer = itor1->buffer;
                mDefaultReqList->erase(itor1);
                callbackflag = true;
                break;
            }
            itor1++;
        }

        if (callbackflag) {
            MAP_AND_CHECK(newOutputCallback_buffers.buffer,
                          &output_callback_addr);
            MAP_AND_CHECK(muxer_msg->combo_buff.buffer, &output_buf1_addr);
            memcpy(output_callback_addr, output_buf1_addr,
                   getBufferSize(*newOutputCallback_buffers.buffer));
            UNMAP_AND_SET_NULL(newOutputCallback_buffers.buffer,
                               &output_callback_addr);
            UNMAP_AND_SET_NULL(muxer_msg->combo_buff.buffer, &output_buf1_addr);

            newOutputCallback_buffers.status = muxer_msg->combo_buff.status;
            newOutputCallback_buffers.acquire_fence = -1;
            newOutputCallback_buffers.release_fence = -1;
            callbackResult.result = NULL;
            callbackResult.frame_number = muxer_msg->combo_buff.frame_number;
            callbackResult.num_output_buffers = 1;
            callbackResult.output_buffers = &newOutputCallback_buffers;
            callbackResult.input_buffer = NULL;
            callbackResult.partial_result = 0;
            HAL_LOGD("id:%d buffer_status %u,buff=%p",
                     callbackResult.frame_number,
                     newOutputCallback_buffers.status,
                     newOutputCallback_buffers.buffer);
            mPrevPostT->mCallbackOps->process_capture_result(
                mPrevPostT->mCallbackOps, &callbackResult);
        }
    }

    newOutput_buffers.stream = muxer_msg->combo_buff.stream;
    newOutput_buffers.buffer = muxer_msg->combo_buff.buffer;
    newOutput_buffers.status = muxer_msg->combo_buff.status;
    newOutput_buffers.acquire_fence = -1;
    newOutput_buffers.release_fence = -1;
    newResult.result = NULL;
    newResult.frame_number = muxer_msg->combo_buff.frame_number;
    newResult.num_output_buffers = 1;
    newResult.input_buffer = NULL;
    newResult.output_buffers = &newOutput_buffers;
    newResult.partial_result = 0;
    HAL_LOGD("id:%d buffer_status %u,buff=%p", newResult.frame_number,
             newOutput_buffers.status, newOutput_buffers.buffer);
    mPrevPostT->mCallbackOps->process_capture_result(mPrevPostT->mCallbackOps,
                                                     &newResult);
    return 0;
}
/*===========================================================================
 * FUNCTION   :Copy2Video
 *
 * DESCRIPTION: Copy2Video
 *
 * PARAMETERS :

 * RETURN     :
 *==========================================================================*/
bool SprdCamera3PortraitScene::Copy2Video(
    portrait_scene_queue_msg_t *prev_msg) {
    HAL_LOGV("E");
    void *prev_addr = NULL;
    void *vid_addr = NULL;
    portrait_scene_queue_msg_t muxer_msg;
    memset(&muxer_msg, 0, sizeof(portrait_scene_queue_msg_t));
    int ret = 0;
    int size = mPbrp->mVideoWidth * mPbrp->mVideoHeight * 3 / 2;
    camera3_capture_result_t newResult;
    camera3_stream_buffer_t newOutput_buffers;
    memset(&newResult, 0, sizeof(camera3_capture_result_t));
    memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
    buffer_handle_t *prev_buff = prev_msg->combo_buff.buffer;
    int idx = prev_msg->combo_buff.frame_number;
    HAL_LOGV("prev copy to video and callback vid");
    int64_t copyStart = systemTime();
    Mutex::Autolock l(mPbrp->mLock);
    List<request_saved_msg_t>::iterator itor = mVidSavedReqList.begin();
    while (itor != mVidSavedReqList.end()) {
        if (itor->frame_number == idx) {
            MAP_AND_CHECK(prev_buff, &prev_addr);
            MAP_AND_CHECK(itor->buffer, &vid_addr);
            HAL_LOGD("idx=%d,vid_addr=%p,prev_addr=%p,vid_handle=%p,prev_"
                     "handle=%p,size=%d",
                     idx, vid_addr, prev_addr, itor->buffer, prev_buff, size);

            memcpy(vid_addr, prev_addr, size);
            HAL_LOGD("memcpy copy my to vid cost %d ms",
                     (int)ns2ms(systemTime() - copyStart));
            /*mPbrp->dumpData((unsigned char *)vid_addr, 1, size,
                mPbrp->mVideoWidth, mPbrp->mVideoHeight,
                idx, "video_yuv");*/
            UNMAP_AND_SET_NULL(prev_buff, &prev_addr);
            UNMAP_AND_SET_NULL(itor->buffer, &vid_addr);
            newOutput_buffers.stream = itor->stream;
            newOutput_buffers.buffer = itor->buffer;
            newResult.frame_number = itor->frame_number;
            newOutput_buffers.status = prev_msg->combo_buff.status;
            newOutput_buffers.acquire_fence = -1;
            newOutput_buffers.release_fence = -1;
            newResult.result = NULL;
            newResult.num_output_buffers = 1;
            newResult.input_buffer = NULL;
            newResult.output_buffers = &newOutput_buffers;
            newResult.partial_result = 0;

            mPrevPostT->mCallbackOps->process_capture_result(
                mPrevPostT->mCallbackOps, &newResult);
            mVidSavedReqList.erase(itor);
            break;
        } else {
            itor++;
        }
    }
    HAL_LOGV("X");
    return true;
}
/*===========================================================================
 * FUNCTION   :CallBackPrevResult
 *
 * DESCRIPTION: process preview Request
 *
 * PARAMETERS :
 *    @device: camera3 device
 *    @request:camera3 request
 * RETURN     :
 *==========================================================================*/
int SprdCamera3PortraitScene::CallBackResult(
    portrait_scene_queue_msg_t *muxer_msg) {
    void *addr1 = NULL;
    void *addr2 = NULL;

    Mutex::Autolock l(mPbrp->mRequestLock);
    List<request_saved_msg_t>::iterator i = mPrevSavedReqList.begin();
    while (i != mPrevSavedReqList.end()) {
        if (i->frame_number == muxer_msg->combo_buff.frame_number) {
            muxer_msg->combo_buff.stream = i->stream;
            int64_t time = systemTime();
            if (mPbrp->mIsRecordMode)
                Copy2Video(muxer_msg);

            HAL_LOGD("prev idx=%d,status=%d", i->frame_number,
                     muxer_msg->combo_buff.status);
            printUseTime(i->frame_number, (char *)"callback");
            CallBackPrevResultInternal(muxer_msg);
            mPrevSavedReqList.erase(i);

            break;
        }
        i++;
    }
    return 0;
}

/*===========================================================================
 * FUNCTION   :CallSnapBackResult
 *
 * DESCRIPTION: CallSnapBackResult
 *
 * PARAMETERS :
 *
 * RETURN     : None
 *==========================================================================*/
void SprdCamera3PortraitScene::CallSnapBackResult(
    camera3_capture_result_t *result, camera3_buffer_status_t buffer_status) {
    camera3_capture_result_t newResult;
    camera3_stream_buffer_t newOutput_buffers;

    HAL_LOGV("CallSnapBackResult enter");
    memset(&newOutput_buffers, 0, sizeof(camera3_stream_buffer_t));
    memset(&newResult, 0, sizeof(camera3_capture_result_t));
    if (mPbrp->mFlushing) {
        buffer_status = CAMERA3_BUFFER_STATUS_ERROR;
    }
    Mutex::Autolock l(mPbrp->mRequestLock);
    List<request_saved_msg_t>::iterator i = mCapSavedReqList.begin();
    while (i != mCapSavedReqList.end()) {
        if (i->frame_number == result->frame_number) {
            HAL_LOGV("CallSnapBackResult matched frame: %d", i->frame_number);
            newOutput_buffers.stream = mSavedReqStreams[mCaptureStreamsNum - 1];
            ;
            newOutput_buffers.buffer = mCapT->mSavedResultBuff;
            // newOutput_buffers.buffer = i->buffer;
            HAL_LOGV("newoutput buffer: %p", newOutput_buffers.buffer);
            newOutput_buffers.status = buffer_status;
            newOutput_buffers.acquire_fence = -1;
            newOutput_buffers.release_fence = -1;
            newResult.frame_number = i->frame_number;
            newResult.output_buffers = &newOutput_buffers;
            newResult.input_buffer = NULL;
            newResult.result = NULL;
            newResult.partial_result = 0;
            newResult.num_output_buffers = 1;
            HAL_LOGD("capInfo:callback,buffer_status%d frame_number:%d",
                     buffer_status, newResult.frame_number);
            mCapPostT->mCallbackOps->process_capture_result(
                mCapPostT->mCallbackOps, &newResult);
            if (i->settings != NULL) {
                free_camera_metadata(i->settings);
                i->settings = NULL;
            }
            if (i->output_buffer != NULL) {
                free(i->output_buffer);
                i->output_buffer = NULL;
            }
            if (i->savedCapRequest != NULL) {
                free(i->savedCapRequest);
                i->savedCapRequest = NULL;
            }
            mCapSavedReqList.erase(i);
            HAL_LOGV("find capture frame %d", newResult.frame_number);
        }
        i++;
    }
}
/*===========================================================================
 * FUNCTION   :updateWeightParams
 *
 * DESCRIPTION: update Blur Weight Params
 *
 * PARAMETERS :
 *
 *
 * RETURN     : none
 *==========================================================================*/
void SprdCamera3PortraitScene::updateWeightParams(CameraMetadata metaSettings,
                                                  int type) {
    uint8_t face_num = 0;
    uint8_t i = 0;
    uint8_t k = 0;
    int32_t x1 = 0;
    int32_t x2 = 0;
    int32_t max = 0;
    int32_t origW = SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[0];
    int32_t origH = SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                        .sensor_InfoInfo.pixer_array_size[1];
    unsigned short savePreviewX = mCachePrevWeightParams.sel_x;
    unsigned short savePreviewY = mCachePrevWeightParams.sel_y;
    uint32_t orientation;
    uint32_t tmpRotateAngle = 0;
    uint32_t tmpCameraAngle = 0;
    uint32_t tmpMobileAngle = 0;

    if (origW == 0 || origH == 0) {
        return;
    }
    HAL_LOGD("origw=%d", origW);
    Mutex::Autolock l(mPbrp->mWeightLock);
    // always get f_num and orientattion in request
    if (type == 0) {
        if (metaSettings.exists(ANDROID_SPRD_BLUR_F_NUMBER)) {
            int fnum =
                metaSettings.find(ANDROID_SPRD_BLUR_F_NUMBER).data.i32[0];
            if (fnum < MIN_F_FUMBER) {
                fnum = MIN_F_FUMBER;
            } else if (fnum > MAX_F_FUMBER) {
                fnum = MAX_F_FUMBER;
            }

            fnum = (fnum)*MAX_BLUR_F_FUMBER / MAX_F_FUMBER;
            if (mCachePrevWeightParams.f_number != fnum) {
                mCachePrevWeightParams.f_number = fnum;
                mCacheCapWeightParams.f_number =
                    (MAX_F_FUMBER + 1 - fnum / 2) * 255 / MAX_F_FUMBER;
                mPrevT->mUpdatePreviewWeightParams = true;
            }
        }

        if (metaSettings.exists(ANDROID_SPRD_DEVICE_ORIENTATION)) {
            mOrientationAngle =
                metaSettings.find(ANDROID_SPRD_DEVICE_ORIENTATION).data.i32[0];
            tmpMobileAngle = mOrientationAngle;
            HAL_LOGD("mobile angle=%d", tmpMobileAngle);
            if (mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
                tmpCameraAngle = 90;
                if (tmpMobileAngle == 180) {
                    tmpRotateAngle = 270;
                } else if (tmpMobileAngle == 90) {
                    tmpRotateAngle = 180;
                } else if (tmpMobileAngle == 0) {
                    tmpRotateAngle = 90;
                } else if (tmpMobileAngle == 270) {
                    tmpRotateAngle = 0;
                }
            } else if (CAM_PBRP_MAIN_ID_2) {
                tmpCameraAngle = 270;
                if (tmpMobileAngle == 0) {
                    tmpRotateAngle = 270;
                } else if (tmpMobileAngle == 90) {
                    tmpRotateAngle = 180;
                } else if (tmpMobileAngle == 180) {
                    tmpRotateAngle = 90;
                } else if (tmpMobileAngle == 270) {
                    tmpRotateAngle = 0;
                }
            }

            mCacheCapWeightParams.camera_angle = tmpCameraAngle;
            mCachePrevWeightParams.camera_angle = tmpCameraAngle;
            mCacheCapWeightParams.mobile_angle = tmpMobileAngle;
            mCachePrevWeightParams.mobile_angle = tmpMobileAngle;
            mCacheCapWeightParams.rotate_angle = tmpRotateAngle;
            mCachePrevWeightParams.rotate_angle = tmpRotateAngle;
            HAL_LOGD("update rotate_angle=%d",
                     mCachePrevWeightParams.rotate_angle);
        }

        if (metaSettings.exists(ANDROID_CONTROL_AF_REGIONS)) {
            uint32_t left =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
            uint32_t top =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
            uint32_t right =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[2];
            uint32_t bottom =
                metaSettings.find(ANDROID_CONTROL_AF_REGIONS).data.i32[3];
            int32_t x = left, y = top;
            HAL_LOGV("ANDROID_CONTROL_AF_REGIONS (%d,%d,%d,%d)", left, top,
                     right, bottom);
            if (!(left == 0 && top == 0 && right == 0 && bottom == 0)) {
                x = left + (right - left) / 2;
                y = top + (bottom - top) / 2;
                if (mPbrp->mCameraId == CAM_BLUR_MAIN_ID) {
                    x = x * mPrevT->mPrevInitParams.width / origW;
                    y = y * mPrevT->mPrevInitParams.height / origH;
                    if (x != mCachePrevWeightParams.sel_x ||
                        y != mCachePrevWeightParams.sel_y) {
                        mCachePrevWeightParams.sel_x = x;
                        mCachePrevWeightParams.sel_y = y;
                        mPrevT->mUpdatePreviewWeightParams = true;
                    }
                } else {
                    if (mFaceNum > 0 &&
                        (x != mLastTouchX || y != mLastTouchY)) {
                        mLastTouchX = x;
                        mLastTouchY = y;
                        mUpdataTouch = true;
                        mPrevT->mUpdatePreviewWeightParams = true;
                        HAL_LOGD("mLastTouch (%d,%d)", mLastTouchX,
                                 mLastTouchY);
                    }
                }
            }
        } else {
            if (mCachePrevWeightParams.sel_x !=
                    mPrevT->mPrevInitParams.width / 2 &&
                mCachePrevWeightParams.sel_y !=
                    mPrevT->mPrevInitParams.height / 2) {
                if (mCachePrevWeightParams.roi_type == 0) {
                    CONTROL_Tag controlInfo;
                    mPbrp->m_pPhyCamera[CAM_TYPE_MAIN]
                        .hwi->mSetting->getCONTROLTag(&controlInfo);
                    if (controlInfo.af_state ==
                            ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN &&
                        controlInfo.af_mode ==
                            ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) {
                        mCachePrevWeightParams.sel_x =
                            mPrevT->mPrevInitParams.width / 2;
                        mCachePrevWeightParams.sel_y =
                            mPrevT->mPrevInitParams.height / 2;
                        mPrevT->mUpdatePreviewWeightParams = true;
                        HAL_LOGD("autofocus and blur center");
                    }
                }
            }
        }
    }

    // back camera get other WeightParams in request
    if (type == 0 && mPbrp->mCameraId == CAM_PBRP_MAIN_ID) {
        if (metaSettings.exists(ANDROID_SPRD_BLUR_CIRCLE_SIZE)) {
            int circle =
                metaSettings.find(ANDROID_SPRD_BLUR_CIRCLE_SIZE).data.i32[0];
            if (circle != 0) {
                int max =
                    mPrevT->mPrevInitParams.height * mCircleSizeScale / 100;
                if (mPrevT->mPrevInitParams.width <
                    mPrevT->mPrevInitParams.height) {
                    max =
                        mPrevT->mPrevInitParams.width * mCircleSizeScale / 100;
                }
                circle = max - (max - PBRP_CIRCLE_VALUE_MIN) * circle / 255;
                if (mCachePrevWeightParams.circle_size != circle) {
                    mCachePrevWeightParams.circle_size = circle;
                    mPrevT->mUpdatePreviewWeightParams = true;
                }
            }
        }
    }

    // get WeightParams in result
    if (type == 1) {

        face_num =
            SprdCamera3Setting::s_setting[mPbrp->mCameraId].faceInfo.face_num;
        mFaceNum = face_num;
        if (face_num <= 0) {
            return;
        } else if (face_num > 0) {
            if (mPbrp->mReqState == WAIT_FIRST_YUV_STATE) {
                mCachePrevWeightParams.roi_type = 2;
                mCacheCapWeightParams.roi_type = 2;
            }
            HAL_LOGD("roi_type:%d,face_num:%d mUpdataTouch:%d",
                     mCachePrevWeightParams.roi_type, face_num, mUpdataTouch);

            for (i = 0; i < face_num; i++) {
                if (mPbrp->isWechatClient == true) {
                    x1 = SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                             .faceInfo.face[i]
                             .rect[0];
                    x2 = SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                             .faceInfo.face[i]
                             .rect[2];
                    mCachePrevWeightParams.total_roi = face_num;
                } else {
                    x1 = metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                             .data.i32[i * 4 + 0];
                    x2 = metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                             .data.i32[i * 4 + 2];
                }

                if (x2 - x1 > max) {
                    k = i;
                    max = x2 - x1;
                }
            }
            if (mPbrp->isWechatClient == false) {
                mFaceInfo[0] =
                    (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[k * 4 + 0]) *
                    mPrevT->mPrevInitParams.width / origW;
                mFaceInfo[1] =
                    (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[k * 4 + 1]) *
                    mPrevT->mPrevInitParams.height / origH;
                mFaceInfo[2] =
                    (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[k * 4 + 2]) *
                    mPrevT->mPrevInitParams.width / origW;
                mFaceInfo[3] =
                    (metaSettings.find(ANDROID_STATISTICS_FACE_RECTANGLES)
                         .data.i32[k * 4 + 3]) *
                    mPrevT->mPrevInitParams.height / origH;
            }
            if (k >= PBRP_MAX_ROI / 2 && mCachePrevWeightParams.roi_type != 0 &&
                mPbrp->isWechatClient == false) {
                metaSettings.update(ANDROID_STATISTICS_FACE_RECTANGLES,
                                    mFaceInfo, 4);
            }
            if (mCachePrevWeightParams.roi_type == 0 &&
                mPbrp->isWechatClient == false) {
                unsigned short sel_x;
                unsigned short sel_y;
                int circle;
                circle = (mFaceInfo[2] - mFaceInfo[0]) * 4 / 5;
                if (mCachePrevWeightParams.circle_size != circle) {
                    mCachePrevWeightParams.circle_size = circle;
                    mPrevT->mUpdatePreviewWeightParams = true;
                }
                sel_x = (mFaceInfo[0] + mFaceInfo[2]) / 2 + circle / 8;
                if (mCachePrevWeightParams.sel_x != sel_x) {
                    mCachePrevWeightParams.sel_x = sel_x;
                    mPrevT->mUpdatePreviewWeightParams = true;
                }

                sel_y = (mFaceInfo[1] + mFaceInfo[3]) / 2;
                if (mCachePrevWeightParams.sel_y != sel_y) {
                    mCachePrevWeightParams.sel_y = sel_y;
                    mPrevT->mUpdatePreviewWeightParams = true;
                }
            } else {
                int32_t bodyInfo[4];
                int32_t faceInfo[4];
                bool touchInBody = false;
                // Don't blur which face acreage less than max acreage width
                // x%
                int32_t max_width = 50;
                // The face width increase by x%
                int32_t width_increase = 30;
                // The face height increase by x% on top
                int32_t height_increase = 70;
                // The width of the body is the width of the face increased
                // by
                // x%
                int32_t body_increase = 130;
                // The upper side of the body is at x% of the face position
                int32_t upper_position = 0;
                // The face height increase by x% on bottom
                int32_t bottom_increase = 20;

                memset(bodyInfo, 0x00, sizeof(bodyInfo));
                memset(faceInfo, 0x00, sizeof(faceInfo));
                memset(mCachePrevWeightParams.flag, 0x00,
                       sizeof(mCachePrevWeightParams.flag));

                if (face_num > PBRP_MAX_ROI / 2) {
                    face_num = PBRP_MAX_ROI / 2;
                }
                k = 0;

                if (mCacheCapWeightParams.roi_type == 2) {
                    memset(mCacheCapWeightParams.x1, 0x00,
                           sizeof(mCacheCapWeightParams.x1));
                    memset(mCacheCapWeightParams.y1, 0x00,
                           sizeof(mCacheCapWeightParams.y1));
                    memset(mCacheCapWeightParams.x2, 0x00,
                           sizeof(mCacheCapWeightParams.x2));
                    memset(mCacheCapWeightParams.y2, 0x00,
                           sizeof(mCacheCapWeightParams.y2));
                    memset(mCacheCapWeightParams.flag, 0x00,
                           sizeof(mCacheCapWeightParams.flag));
                }
                for (i = 0; i < face_num; i++) {
                    if (mPbrp->isWechatClient == true) {
                        faceInfo[0] =
                            SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                                .faceInfo.face[i]
                                .rect[0];
                        faceInfo[1] =
                            SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                                .faceInfo.face[i]
                                .rect[1];
                        faceInfo[2] =
                            SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                                .faceInfo.face[i]
                                .rect[2];
                        faceInfo[3] =
                            SprdCamera3Setting::s_setting[mPbrp->mCameraId]
                                .faceInfo.face[i]
                                .rect[3];
                    } else {
                        faceInfo[0] =
                            metaSettings
                                .find(ANDROID_STATISTICS_FACE_RECTANGLES)
                                .data.i32[i * 4 + 0];
                        faceInfo[1] =
                            metaSettings
                                .find(ANDROID_STATISTICS_FACE_RECTANGLES)
                                .data.i32[i * 4 + 1];
                        faceInfo[2] =
                            metaSettings
                                .find(ANDROID_STATISTICS_FACE_RECTANGLES)
                                .data.i32[i * 4 + 2];
                        faceInfo[3] =
                            metaSettings
                                .find(ANDROID_STATISTICS_FACE_RECTANGLES)
                                .data.i32[i * 4 + 3];
                    }

                    if (mCacheCapWeightParams.roi_type == 2) {
                        if (mOrientationAngle == 0) {
                            mCacheCapWeightParams.x1[i] =
                                faceInfo[2] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y1[i] =
                                faceInfo[1] * mCapT->mCapInitParams.height /
                                origH;
                            mCacheCapWeightParams.x2[i] =
                                faceInfo[0] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y2[i] =
                                faceInfo[3] * mCapT->mCapInitParams.height /
                                origH;
                        } else if (mOrientationAngle == 90) {
                            mCacheCapWeightParams.x1[i] =
                                faceInfo[2] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y1[i] =
                                faceInfo[3] * mCapT->mCapInitParams.height /
                                origH;
                            mCacheCapWeightParams.x2[i] =
                                faceInfo[0] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y2[i] =
                                faceInfo[1] * mCapT->mCapInitParams.height /
                                origH;
                        } else if (mOrientationAngle == 180) {
                            mCacheCapWeightParams.x1[i] =
                                faceInfo[0] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y1[i] =
                                faceInfo[3] * mCapT->mCapInitParams.height /
                                origH;
                            mCacheCapWeightParams.x2[i] =
                                faceInfo[2] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y2[i] =
                                faceInfo[1] * mCapT->mCapInitParams.height /
                                origH;
                        } else {
                            mCacheCapWeightParams.x1[i] =
                                faceInfo[0] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y1[i] =
                                faceInfo[1] * mCapT->mCapInitParams.height /
                                origH;
                            mCacheCapWeightParams.x2[i] =
                                faceInfo[2] * mCapT->mCapInitParams.width /
                                origW;
                            mCacheCapWeightParams.y2[i] =
                                faceInfo[3] * mCapT->mCapInitParams.height /
                                origH;
                        }
                        mCacheCapWeightParams.total_roi = face_num;
                    }
                    /*if ((faceInfo[2] - faceInfo[0]) < max * max_width / 100) {
                        if (mPrevWeightParams.valid_roi == face_num * 2) {
                            mUpdatePreviewWeightParams = true;
                        }
                        k++;
                        continue;
                    }*/
                    if (mOrientationAngle == 270) {
                        int w = faceInfo[2] - faceInfo[0];
                        int h = faceInfo[3] - faceInfo[1];

                        faceInfo[0] -= w * width_increase / 200;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += w * width_increase / 200;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[1] -= h * height_increase / 100;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += h * bottom_increase / 100;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        bodyInfo[0] =
                            faceInfo[0] -
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[0] < 0) {
                            bodyInfo[0] = 0;
                        }

                        bodyInfo[1] =
                            faceInfo[3] -
                            (faceInfo[3] - faceInfo[1]) * upper_position / 100;

                        bodyInfo[2] =
                            faceInfo[2] +
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[2] > origW) {
                            bodyInfo[2] = origW;
                        }

                        bodyInfo[3] = origH;
                    } else if (mOrientationAngle == 90) {
                        int w = faceInfo[2] - faceInfo[0];
                        int h = faceInfo[3] - faceInfo[1];

                        faceInfo[0] -= w * width_increase / 200;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += w * width_increase / 200;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[3] += h * height_increase / 100;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[1] -= h * bottom_increase / 100;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        bodyInfo[0] =
                            faceInfo[0] -
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[0] < 0) {
                            bodyInfo[0] = 0;
                        }

                        bodyInfo[1] = 0;

                        bodyInfo[2] =
                            faceInfo[2] +
                            (faceInfo[2] - faceInfo[0]) * body_increase / 200;
                        if (bodyInfo[2] > origW) {
                            bodyInfo[2] = origW;
                        }

                        bodyInfo[3] =
                            faceInfo[1] +
                            (faceInfo[3] - faceInfo[1]) * upper_position / 100;
                    } else if (mOrientationAngle == 180) {
                        int w = faceInfo[3] - faceInfo[1];
                        int h = faceInfo[2] - faceInfo[0];

                        faceInfo[1] -= w * width_increase / 200;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += w * width_increase / 200;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[0] -= h * height_increase / 100;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        faceInfo[2] += h * bottom_increase / 100;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        bodyInfo[0] =
                            faceInfo[2] -
                            (faceInfo[2] - faceInfo[0]) * upper_position / 100;

                        bodyInfo[1] =
                            faceInfo[1] -
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[1] < 0) {
                            bodyInfo[1] = 0;
                        }

                        bodyInfo[2] = origW;

                        bodyInfo[3] =
                            faceInfo[3] +
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[3] > origH) {
                            bodyInfo[3] = origH;
                        }
                    } else {
                        int w = faceInfo[3] - faceInfo[1];
                        int h = faceInfo[2] - faceInfo[0];

                        faceInfo[1] -= w * width_increase / 200;
                        if (faceInfo[1] < 0) {
                            faceInfo[1] = 0;
                        }

                        faceInfo[3] += w * width_increase / 200;
                        if (faceInfo[3] > origH) {
                            faceInfo[3] = origH;
                        }

                        faceInfo[2] += h * height_increase / 100;
                        if (faceInfo[2] > origW) {
                            faceInfo[2] = origW;
                        }

                        faceInfo[0] -= h * bottom_increase / 100;
                        if (faceInfo[0] < 0) {
                            faceInfo[0] = 0;
                        }

                        bodyInfo[0] = 0;

                        bodyInfo[1] =
                            faceInfo[1] -
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[1] < 0) {
                            bodyInfo[1] = 0;
                        }

                        bodyInfo[2] =
                            faceInfo[0] +
                            (faceInfo[2] - faceInfo[0]) * upper_position / 100;

                        bodyInfo[3] =
                            faceInfo[3] +
                            (faceInfo[3] - faceInfo[1]) * body_increase / 200;
                        if (bodyInfo[3] > origH) {
                            bodyInfo[3] = origH;
                        }
                    }
                    if (mUpdataTouch == true && touchInBody == false) {
                        if ((mLastTouchX > faceInfo[0] &&
                             mLastTouchX < faceInfo[2] &&
                             mLastTouchY > faceInfo[1] &&
                             mLastTouchY < faceInfo[3]) ||
                            (mLastTouchX > bodyInfo[0] &&
                             mLastTouchX < bodyInfo[2] &&
                             mLastTouchY > bodyInfo[1] &&
                             mLastTouchY < bodyInfo[3])) {
                            mBlurBody = true;
                            touchInBody = true;
                            HAL_LOGD("in body");
                        } else {
                            mBlurBody = false;
                            HAL_LOGD("out body");
                        }
                    }
                    if (mCachePrevWeightParams.x1[2 * (i - k)] !=
                        faceInfo[0] * mPrevT->mPrevInitParams.width / origW) {
                        mCachePrevWeightParams.x1[2 * (i - k)] =
                            faceInfo[0] * mPrevT->mPrevInitParams.width / origW;
                        mPrevT->mUpdatePreviewWeightParams = true;
                    }
                    if (mCachePrevWeightParams.y1[2 * (i - k)] !=
                        faceInfo[1] * mPrevT->mPrevInitParams.height / origH) {
                        mCachePrevWeightParams.y1[2 * (i - k)] =
                            faceInfo[1] * mPrevT->mPrevInitParams.height /
                            origH;
                        mPrevT->mUpdatePreviewWeightParams = true;
                    }
                    if (mCachePrevWeightParams.x2[2 * (i - k)] !=
                        faceInfo[2] * mPrevT->mPrevInitParams.width / origW) {
                        mCachePrevWeightParams.x2[2 * (i - k)] =
                            faceInfo[2] * mPrevT->mPrevInitParams.width / origW;
                        mPrevT->mUpdatePreviewWeightParams = true;
                    }
                    if (mCachePrevWeightParams.y2[2 * (i - k)] !=
                        faceInfo[3] * mPrevT->mPrevInitParams.height / origH) {
                        mCachePrevWeightParams.y2[2 * (i - k)] =
                            faceInfo[3] * mPrevT->mPrevInitParams.height /
                            origH;
                        mPrevT->mUpdatePreviewWeightParams = true;
                    }
                    mCachePrevWeightParams.x1[2 * (i - k) + 1] =
                        bodyInfo[0] * mPrevT->mPrevInitParams.width / origW;
                    mCachePrevWeightParams.y1[2 * (i - k) + 1] =
                        bodyInfo[1] * mPrevT->mPrevInitParams.height / origH;
                    mCachePrevWeightParams.x2[2 * (i - k) + 1] =
                        bodyInfo[2] * mPrevT->mPrevInitParams.width / origW;
                    mCachePrevWeightParams.y2[2 * (i - k) + 1] =
                        bodyInfo[3] * mPrevT->mPrevInitParams.height / origH;
                    mCachePrevWeightParams.flag[2 * (i - k)] = 0;
                    mCachePrevWeightParams.flag[2 * (i - k) + 1] = 1;
                }
                mCachePrevWeightParams.valid_roi = (face_num - k) * 2;
                if (mCacheCapWeightParams.roi_type == 2) {
                    mCacheCapWeightParams.valid_roi = face_num - k;
                }

                if (mUpdataTouch == true) {
                    mCachePrevWeightParams.sel_x =
                        mLastTouchX * mPrevT->mPrevInitParams.width / origW;
                    mCachePrevWeightParams.sel_y =
                        mLastTouchY * mPrevT->mPrevInitParams.height / origH;
                    mUpdataTouch = false;
                    mUpdataxy = 1;
                    mFirstUpdateFrame = 1;
                }
                if (mUpdataxy != 0) {
                    if (mFirstUpdateFrame == 1) {
                        mstartUpdate = systemTime();
                        mFaceInfoX = mFaceInfo[0];
                        mFaceInfoY = mFaceInfo[1];
                        mFirstUpdateFrame = 0;
                    }
                    if (ns2ms(systemTime() - mstartUpdate) >= MAX_UPDATE_TIME) {
                        int x = FACE_INFO_SCALER(mPrevT->mPrevInitParams.width,
                                                 mPrevT->mPrevInitParams.height,
                                                 mFaceInfo[0], mFaceInfo[1],
                                                 mFaceInfo[2], mFaceInfo[3]);
                        if (ABS(mFaceInfoX - mFaceInfo[0]) * x >
                                mPrevT->mPrevInitParams.width / 2 ||
                            ABS(mFaceInfoY - mFaceInfo[1]) * x >
                                mPrevT->mPrevInitParams.height / 2)
                            mUpdataxy = 0;
                    }
                }
                if (!mUpdataxy) {
                    mCachePrevWeightParams.sel_x =
                        ABS(mFaceInfo[2] + mFaceInfo[0]) / 2;
                    mCachePrevWeightParams.sel_y =
                        ABS(mFaceInfo[3] + mFaceInfo[1]) / 2;
                }
            }
        }
    }

    if (mPrevT->mUpdatePreviewWeightParams) {
        mCapT->mUpdateCaptureWeightParams = true;
        mCacheCapWeightParams.circle_size = mCachePrevWeightParams.circle_size *
                                            mCapT->mCapInitParams.height /
                                            mPrevT->mPrevInitParams.height;
        mCacheCapWeightParams.sel_x = mCachePrevWeightParams.sel_x *
                                      mCapT->mCapInitParams.width /
                                      mPrevT->mPrevInitParams.width;
        mCacheCapWeightParams.sel_y = mCachePrevWeightParams.sel_y *
                                      mCapT->mCapInitParams.height /
                                      mPrevT->mPrevInitParams.height;
        if (mCacheCapWeightParams.roi_type == 2 &&
            mCachePrevWeightParams.valid_roi > 0) {
            mCacheCapWeightParams.valid_roi = mCachePrevWeightParams.valid_roi;
            memset(mCacheCapWeightParams.x1, 0x00,
                   sizeof(mCacheCapWeightParams.x1));
            memset(mCacheCapWeightParams.y1, 0x00,
                   sizeof(mCacheCapWeightParams.y1));
            memset(mCacheCapWeightParams.x2, 0x00,
                   sizeof(mCacheCapWeightParams.x2));
            memset(mCacheCapWeightParams.y2, 0x00,
                   sizeof(mCacheCapWeightParams.y2));
            memcpy(mCacheCapWeightParams.flag, mCachePrevWeightParams.flag,
                   sizeof(mCacheCapWeightParams.flag));
            HAL_LOGD("sizeof flag=%d, ==40",
                     sizeof(mCacheCapWeightParams.flag));
            for (i = 0; i < mCacheCapWeightParams.valid_roi / 2; i++) {
                mCacheCapWeightParams.x1[2 * i] =
                    mCachePrevWeightParams.x1[2 * i] *
                    mCapT->mCapInitParams.width / mPrevT->mPrevInitParams.width;
                mCacheCapWeightParams.y1[2 * i] =
                    mCachePrevWeightParams.y1[2 * i] *
                    mCapT->mCapInitParams.height /
                    mPrevT->mPrevInitParams.height;
                mCacheCapWeightParams.x2[2 * i] =
                    mCachePrevWeightParams.x2[2 * i] *
                    mCapT->mCapInitParams.width / mPrevT->mPrevInitParams.width;
                mCacheCapWeightParams.y2[2 * i] =
                    mCachePrevWeightParams.y2[2 * i] *
                    mCapT->mCapInitParams.height /
                    mPrevT->mPrevInitParams.height;
                mCacheCapWeightParams.x1[2 * i + 1] =
                    mCachePrevWeightParams.x1[2 * i + 1] *
                    mCapT->mCapInitParams.width / mPrevT->mPrevInitParams.width;
                mCacheCapWeightParams.y1[2 * i + 1] =
                    mCachePrevWeightParams.y1[2 * i + 1] *
                    mCapT->mCapInitParams.height /
                    mPrevT->mPrevInitParams.height;
                mCacheCapWeightParams.x2[2 * i + 1] =
                    mCachePrevWeightParams.x2[2 * i + 1] *
                    mCapT->mCapInitParams.width / mPrevT->mPrevInitParams.width;
                mCacheCapWeightParams.y2[2 * i + 1] =
                    mCachePrevWeightParams.y2[2 * i + 1] *
                    mCapT->mCapInitParams.height /
                    mPrevT->mPrevInitParams.height;
            }
        }
    }
}
}; // namespace sprdcamera
