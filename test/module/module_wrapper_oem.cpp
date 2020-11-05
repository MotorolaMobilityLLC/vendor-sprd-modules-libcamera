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
#include <dlfcn.h>
#include "cmr_log.h"
#include "cmr_types.h"
#include "cmr_common.h"
#include "sprd_ion.h"
#include "MemIon.h"
#include <pthread.h>
#include "sensor_drv_u.h"
#include "module_wrapper_oem.h"
#define LOG_TAG "IT-moduleOem"

#include <utils/Mutex.h>
#include <utils/List.h>
#include <json2oem.h>
#include <stdlib.h>
#include <stdio.h>

extern map<string, vector<resultData_t> *> gMap_Result;
using namespace android;
#define THIS_MODULE_NAME "oem"
#define PREVIEW_BUFF_NUM 8
#define ZSL_BUFF_NUM 8
#define MAX_SUB_RAWHEAP_NUM 10
#define STATE_WAIT_TIME 20// ms
#define MAX_WAIT_TIME 2000//ms
#define ISP_STATS_MAX 8
#define CAP_TIMEOUT 5000000000       /*5000ms*/

#define SET_PARM(h, x, y, z)                                                   \
    do {                                                                       \
        if (NULL != h && NULL != h->ops)                                       \
            h->ops->camera_set_param(x, y, z);                                 \
    } while (0)

#define OEM_LIBRARY_PATH "libcamoem.so"
#define CAMERA_MIN_FPS 5
#define CAMERA_MAX_FPS 30


typedef struct Oem_context {
    //oem_module_t *oem_dev;
    //cmr_handle oem_handle;
    unsigned int camera_id;
    unsigned int camera_id_max;
    unsigned int width;
    unsigned int height;
    unsigned int fps;
}Oem_context_t;

typedef struct sprd_camera_memory {
    MemIon *ion_heap;
    cmr_uint phys_addr;
    cmr_uint phys_size;
    cmr_s32 fd;
    cmr_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_memory_t;

struct ZslBufferQueue {
    camera_frame_type frame;
    sprd_camera_memory_t *heap_array;
};

typedef int32_t status_t;

enum preview_frame_type {
    PREVIEW_FRAME = 0,
    PREVIEW_VIDEO_FRAME,
    PREVIEW_ZSL_FRAME,
    PREVIEW_CANCELED_FRAME,
    PREVIEW_VIDEO_CANCELED_FRAME,
    PREVIEW_ZSL_CANCELED_FRAME,
    CHANNEL0_FRAME,
    CHANNEL1_FRAME,
    CHANNEL2_FRAME,
    CHANNEL3_FRAME,
    CHANNEL4_FRAME,
    PREVIEW_FRAME_TYPE_MAX
};

typedef enum {
    CAMERA_STREAM_TYPE_DEFAULT, /* default stream type */
    CAMERA_STREAM_TYPE_PREVIEW, /* 1 preview */
    CAMERA_STREAM_TYPE_VIDEO, /* video */
    CAMERA_STREAM_TYPE_CALLBACK, /* callback */
    CAMERA_STREAM_TYPE_YUV2,
    CAMERA_STREAM_TYPE_ZSL_PREVIEW, /* 5 zsl preview */
    CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT, /* 6 non zsl snapshot*/
    CAMERA_STREAM_TYPE_NUM_MAX,
} camera_stream_type_t;

typedef struct {
    camera_stream_type_t stream_type;
    int width;
    int height;
} stream_config;

class CameraOemTest {
  public:
    CameraOemTest();
    ~CameraOemTest();
    int dlLib();
    int openCamera();
    int startPreview();
    int stopPreview();
    int zslSnapshot();
    int takePicture();
    int closeCamera();

    int stopPreviewInternal();
    static void camera_cb(camera_cb_type cb, const void *client_data,
                          camera_func_type func, void *parm4);
    void free_camera_mem(sprd_camera_memory_t *memory);
    int get_preview_buffer_id_for_fd(cmr_s32 fd);
    uint32_t get_zsl_buffer_id_for_fd(cmr_s32 fd);
    int getZSLQueueFrameNum();
    int getLargestSensorSize(cmr_uint cameraId, cmr_u16 *width, cmr_u16 *height);
    int iommu_is_enabled(void);
    bool isCapturing();
    bool isZslMode();
    bool WaitForCaptureDone();
    sprd_camera_memory_t *alloc_camera_mem(int buf_size, int num_bufs,
                                           uint32_t is_cache);
    static cmr_int callback_malloc(enum camera_mem_cb_type type,
                                   cmr_u32 *size_ptr,
                                   cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                                   cmr_uint *vir_addr, cmr_s32 *fd,
                                   void *private_data);
    int callback_preview_malloc(cmr_u32 size, cmr_u32 sum,
                                cmr_uint *phy_addr, cmr_uint *vir_addr,
                                cmr_s32 *fd);
    int callback_capture_malloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                                cmr_uint *vir_addr, cmr_s32 *fd);
    int callback_zsl_malloc(cmr_u32 size, cmr_u32 sum,
                            cmr_uint *phy_addr, cmr_uint *vir_addr,
                            cmr_s32 *fd);
    int callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
                              cmr_u32 sum, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd);
    static cmr_int callback_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                                 cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                                 void *private_data);
    int callback_preview_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                              cmr_s32 *fd, cmr_u32 sum);
    int callback_capture_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                              cmr_s32 *fd, cmr_u32 sum);
    int callback_zsl_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                          cmr_s32 *fd, cmr_u32 sum);
    int callback_other_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                            cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum);
    void freeAllCameraMemIon();

    int snapshot_zsl();
    int pushZslFrame(struct camera_frame_type *frame);
    struct camera_frame_type popZslFrame();
    void pushZSLQueue(ZslBufferQueue *frame);
    ZslBufferQueue popZSLQueue();
    void releaseZSLQueue();
    void HandleStartPreview(enum camera_cb_type cb, void *frame);
    void HandleStopPreview(enum camera_cb_type cb, void *parm4);
    void HandleEncode(enum camera_cb_type cb, void *frame);
    void HandleStopCamera(enum camera_cb_type cb, void *parm4);
    void receivePreviewFrame(struct camera_frame_type *frame);
    void receiveJpegPicture(struct camera_frame_type *frame);

  public:
    Oem_context_t *mOem_cxt;
//preview
    unsigned int dump_total_count = 0;
    unsigned int dump_cnt_pre = 0;
//Snapshot
    unsigned int capture_total_count = 0;
    unsigned int capture_cnt_save = 0;

//    int stream_count = 0;
//    stream_config *streamConfig; // stream config

    int mPreviewWidth;
    int mPreviewHeight;
    int mCaptureWidth;
    int mCaptureHeight;

    char snapshot_tag_name[30]={0};

  private:
    Mutex mPreviewCbLock;
    Mutex mCaptureCbLock;
    Mutex mPrevBufLock;
    Mutex mCapBufLock;
    Mutex mZslLock;
    Mutex mZslBufLock;

    Condition mStateWait;
    Mutex mStateLock;
    Mutex mLock;

    takepicture_mode mCaptureMode;
    int mPictureFormat;

    cmr_u16 mLargestSensorWidth;
    cmr_u16 mLargestSensorHeight;

    int32_t mPicCaptureCnt;

    enum Sprd_camera_state {
        SPRD_INIT,
        SPRD_IDLE,
        SPRD_ERROR,
        SPRD_PREVIEW_IN_PROGRESS,
        SPRD_FOCUS_IN_PROGRESS,
        SPRD_SET_PARAMS_IN_PROGRESS,
        SPRD_WAITING_RAW,
        SPRD_WAITING_JPEG,
        SPRD_FLASH_IN_PROGRESS,

        // internal states
        SPRD_INTERNAL_PREVIEW_STOPPING,
        SPRD_INTERNAL_CAPTURE_STOPPING,
        SPRD_INTERNAL_PREVIEW_REQUESTED,
        SPRD_INTERNAL_RAW_REQUESTED,
        SPRD_INTERNAL_STOPPING,
    };

    typedef struct _camera_state {
        Sprd_camera_state camera_state;
        Sprd_camera_state preview_state;
        Sprd_camera_state capture_state;
        Sprd_camera_state focus_state;
        Sprd_camera_state setParam_state;
    } camera_state;

  private:
//    Oem_context
//    Oem_context_t *mOem_cxt;
    cmr_handle mCameraHandle;
    oem_module_t *mOem;

    bool mReleaseFLag;
    bool mZslCaptureExitLoop;
    bool mSprdZslEnabled;
    uint32_t mIsStoppingPreview;
    List<ZslBufferQueue> mZSLQueue;
    volatile camera_state mCameraState;

    int32_t mZslMaxFrameNum;
    uint32_t mSubRawHeapNum;
    // pre-alloc capture memory
    uint32_t mIsPreAllocCapMem;

    int previewvalid = 0;
    int is_iommu_enabled = 0;
    unsigned int mPreviewHeapNum = 0;
    uint32_t mSubRawHeapSize;
    uint32_t mZslHeapNum=0;
    uint32_t mIspFirmwareReserved_cnt = 0;
    static const int kISPB4awbCount = 16;
    sprd_camera_memory_t *mPreviewHeapReserved = NULL;
    sprd_camera_memory_t *mZslHeapReserved = NULL;
    sprd_camera_memory_t *mIspLscHeapReserved = NULL;
    sprd_camera_memory_t *mIspAFLHeapReserved = NULL;
    sprd_camera_memory_t *mIspFirmwareReserved = NULL;
    sprd_camera_memory_t *mIspStatisHeapReserved = NULL;
    sprd_camera_memory_t *mIspB4awbHeapReserved[kISPB4awbCount];
    sprd_camera_memory_t *mIspRawAemHeapReserved[kISPB4awbCount];
    sprd_camera_memory_t *mPreviewHeapArray[PREVIEW_BUFF_NUM];
    sprd_camera_memory_t *mZslHeapArray[ZSL_BUFF_NUM];
    sprd_camera_memory_t *mSubRawHeapArray[MAX_SUB_RAWHEAP_NUM];

    sprd_camera_memory_t *mIspAemHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspAfmHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspAflHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspPdafHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspBayerhistHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspLscmHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspYuvhistHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIsp3dnrHeapReserved[ISP_STATS_MAX];
    sprd_camera_memory_t *mIspEbdHeapReserved[ISP_STATS_MAX];

    int32_t thumbnail_width;
    int32_t thumbnail_height;

    enum state_owner {
        STATE_CAMERA,
        STATE_PREVIEW,
        STATE_CAPTURE,
        STATE_FOCUS,
        STATE_SET_PARAMS,
    };

    void setCameraState(Sprd_camera_state state,state_owner owner = STATE_CAMERA);
    Sprd_camera_state transitionState(Sprd_camera_state from,
                                      Sprd_camera_state to, state_owner owner,
                                      bool lock = true);
    const char *getCameraStateStr(Sprd_camera_state s);
    inline bool isPreviewing();
    inline bool isCameraInit();
    status_t cancelPicture();
    void deinitCapture(bool isPreAllocCapMem);
    bool WaitForCameraStop();
    int setSnapshotParams();

    inline Sprd_camera_state getCameraState();
    inline Sprd_camera_state getPreviewState();
    inline Sprd_camera_state getCaptureState();

};

CameraOemTest camera_oem;

CameraOemTest::CameraOemTest() {
    int num = 0;
    mCameraHandle = NULL;
    mOem = NULL;

    mReleaseFLag = false;
    mZslCaptureExitLoop = false;
    mSprdZslEnabled = false;
    mIsPreAllocCapMem = 0;

    memset(mIspB4awbHeapReserved, 0, sizeof(mIspB4awbHeapReserved));
    memset(mIspRawAemHeapReserved, 0, sizeof(mIspRawAemHeapReserved));
    memset(mPreviewHeapArray, 0, sizeof(mPreviewHeapArray));
    memset(mZslHeapArray, 0, sizeof(mZslHeapArray));
    memset(mSubRawHeapArray, 0, sizeof(mSubRawHeapArray));

    memset(mIspAemHeapReserved, 0, sizeof(mIspAemHeapReserved));
    memset(mIspAfmHeapReserved, 0, sizeof(mIspAfmHeapReserved));
    memset(mIspAflHeapReserved, 0, sizeof(mIspAflHeapReserved));
    memset(mIspBayerhistHeapReserved, 0, sizeof(mIspBayerhistHeapReserved));
    memset(mIspYuvhistHeapReserved, 0, sizeof(mIspYuvhistHeapReserved));
    memset(mIsp3dnrHeapReserved, 0, sizeof(mIsp3dnrHeapReserved));
    memset(mIspEbdHeapReserved, 0, sizeof(mIspEbdHeapReserved));
    memset(mIspPdafHeapReserved, 0, sizeof(mIspPdafHeapReserved));
    memset(mIspLscmHeapReserved, 0, sizeof(mIspLscmHeapReserved));

    mOem_cxt = (Oem_context_t *)malloc(sizeof(Oem_context_t));
    num = sensorGetPhysicalSnsNum();
    mOem_cxt->camera_id_max = num;
    CMR_LOGI("number of camera : %d",num);
//    mOem_cxt->camera_id = num - 1;
    mOem_cxt->camera_id = 0;

    mOem_cxt->width = 1280;//1280
    mOem_cxt->height = 720;//720

    mOem_cxt->fps = 0;
    thumbnail_width = 240;
    thumbnail_height = 320;

    setCameraState(SPRD_INIT, STATE_CAMERA);

    mCaptureMode = CAMERA_ZSL_MODE;
    mPictureFormat = CAM_IMG_FMT_YUV420_NV21;
    mPicCaptureCnt = 0;

    mZslMaxFrameNum = 1;
    mSubRawHeapNum = 0;

//    cmr_u16 picW, picH;
    cmr_u16 snsW, snsH;

//    getLargestPictureSize(mOem_cxt->camera_id, &picW, &picH);
    getLargestSensorSize(mOem_cxt->camera_id, &snsW, &snsH);

    mLargestSensorWidth = snsW;
    mLargestSensorHeight = snsH;

}

CameraOemTest::~CameraOemTest() {
    if (mOem) {
        if (NULL != mOem->dso)
            dlclose(mOem->dso);
        free((void *)mOem);
        mOem = NULL;
    }

    if (mOem_cxt) {
        free((void *)mOem_cxt);
        mOem_cxt = NULL;
    }
// clean memory in case memory leak
    freeAllCameraMemIon();
}

int CameraOemTest::dlLib() {
    void *handle = NULL;
    oem_module_t *omi = NULL;
    const char *sym = OEM_MODULE_INFO_SYM_AS_STR;

    if (!mOem_cxt) {
        CMR_LOGE("failed: input camera_oem.mOem_cxt is null");
        return -1;
    }

    if (!mOem) {
        mOem = (oem_module_t *)malloc(sizeof(oem_module_t));
        handle = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        mOem->dso = handle;

        if (handle == NULL) {
            CMR_LOGE("open libcamoem failed");
            goto loaderror;
        }
        /* Get the address of the struct hal_module_info. */
        omi = (oem_module_t *)dlsym(handle, sym);

        if (omi == NULL) {
            CMR_LOGE("symbol failed");
            goto loaderror;
        }
        mOem->ops = omi->ops;

        CMR_LOGV("loaded libcamoem handle=%p", handle);
    }
    return 0;

loaderror:
    if (mOem->dso != NULL) {
        dlclose(mOem->dso);
    }
    free((void *)mOem);
    mOem = NULL;

    return -1;
}

int CameraOemTest::openCamera() {

    int ret = 0;
    mOem_cxt->camera_id = 0;
    unsigned int cameraId = mOem_cxt->camera_id;

    if (mOem_cxt == NULL) {
        CMR_LOGE("failed: input camera_oem.mOem_cxt is null");
        return -1;
    }

    cameraId = mOem_cxt->camera_id;

    mOem->ops->camera_set_largest_picture_size(mOem_cxt->camera_id,
                            mLargestSensorWidth, mLargestSensorHeight);

    ret = mOem->ops->camera_init(
        cameraId, camera_cb, this, 0, &mCameraHandle,
        (void *)callback_malloc, (void *)callback_free);//&client_data

    if (ret) {
        setCameraState(SPRD_INIT, STATE_CAMERA);
        CMR_LOGE("camera_init failed");
        goto _exit;
    }

    setCameraState(SPRD_IDLE);

    is_iommu_enabled = iommu_is_enabled();
    CMR_LOGI("is_iommu_enabled=%d", is_iommu_enabled);

    return ret;

_exit:
    CMR_LOGI(":cameraInit: X");
    return ret;

}

int CameraOemTest::startPreview() {
    int ret = 0;
    Mutex::Autolock l(&mLock);

    struct img_size preview_size = {0, 0};
    struct img_size capture_size = {0, 0};
    struct cmr_zoom_param zoom_param;
    struct cmr_range_fps_param fps_param;
    mZslCaptureExitLoop = false;
    mSprdZslEnabled = true;

    memset(&zoom_param, 0, sizeof(struct cmr_zoom_param));
    if (mOem_cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (mCameraHandle == NULL || mOem == NULL ||
        mOem->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        goto exit;
    }

    preview_size.width = mPreviewWidth;
    preview_size.height = mPreviewHeight;

    capture_size.width = mCaptureWidth;
    capture_size.height = mCaptureHeight;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio = mPreviewWidth / mPreviewHeight;

    fps_param.is_recording = 0;
    if (mOem_cxt->fps > 0) {
        fps_param.min_fps = mOem_cxt->fps;
        fps_param.max_fps = mOem_cxt->fps;
    } else {
        fps_param.min_fps = CAMERA_MIN_FPS;
        fps_param.max_fps = CAMERA_MAX_FPS;
    }
    fps_param.video_mode = 0;

    mOem->ops->camera_fast_ctrl(mCameraHandle, CAMERA_FAST_MODE_FD, 0);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&capture_size);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT,
             (cmr_uint)mPictureFormat);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_PREVIEW_FORMAT,
             CAM_IMG_FMT_YUV420_NV21);//IMG_DATA_TYPE_YUV420
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_ZOOM,
             (cmr_uint)&zoom_param);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);
// zsl enable
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);


    ret = mOem->ops->camera_set_mem_func(
        mCameraHandle, (void *)callback_malloc, (void *)callback_free, NULL);
    if (ret) {
        CMR_LOGE("camera_set_mem_func failed");
        goto exit;
    }

    setCameraState(SPRD_INTERNAL_PREVIEW_REQUESTED, STATE_PREVIEW);

    ret = mOem->ops->camera_start_preview(mCameraHandle,
                                          mCaptureMode);//CAMERA_NORMAL_MODE
    if (ret) {
        CMR_LOGE("camera_start_preview failed");
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        //deinitPreview();
        goto exit;
    }

    return ret;

exit:
    return -1;
}

int CameraOemTest::zslSnapshot() {
    int ret = CMR_CAMERA_SUCCESS;
    mCaptureMode = CAMERA_ZSL_MODE;
    Mutex::Autolock l(&mLock);

    mPicCaptureCnt = 1;

//WAIT CAPTURE DONE
    if (isCapturing()) {
      WaitForCaptureDone();
    }

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM,
             mPicCaptureCnt);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_EXIF_MIME_TYPE,
             MODE_SINGLE_CAMERA);

    struct img_size req_size = {0, 0};
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
             (cmr_uint)&req_size);

    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);
    ret = mOem->ops->camera_take_picture(mCameraHandle, mCaptureMode);
    if (ret) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        CMR_LOGE("fail to camera_take_picture");
        goto exit;
    }

    ret = snapshot_zsl();

exit:
    CMR_LOGD("X");
    return ret;

}

int CameraOemTest::stopPreview() {
    int ret = CMR_CAMERA_SUCCESS;

    ret = stopPreviewInternal();
    if (ret) {
        CMR_LOGE("stopPreview  fail ");
    }

    return ret;
}

int CameraOemTest::stopPreviewInternal() {
    int ret = CMR_CAMERA_SUCCESS;
    mZslCaptureExitLoop = true;
    mIsStoppingPreview = 1;

//WAIT CAPTURE DONE
    if (isCapturing()) {
        WaitForCaptureDone();
//        cancelPicture();
    }

    if (mOem_cxt == NULL) {
        CMR_LOGE("failed: input cxt is null");
        goto exit;
    }

    if (mCameraHandle == NULL || mOem == NULL ||
        mOem->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        goto exit;
    }

    if (CMR_CAMERA_SUCCESS != mOem->ops->camera_stop_preview(mCameraHandle)) {
        ret = CMR_CAMERA_FAIL;
        setCameraState(SPRD_ERROR, STATE_PREVIEW);
        CMR_LOGE("stopPreview X: fail to  stop preview");
        goto exit;
    }

    setCameraState(SPRD_IDLE, STATE_PREVIEW);

exit:
    mIsStoppingPreview = 0;
    return ret;

}

int CameraOemTest::takePicture() {
    CMR_LOGI("E");
    int ret = CMR_CAMERA_SUCCESS;
    Mutex::Autolock l(&mLock);
    mCaptureMode = CAMERA_NORMAL_MODE;

    if (SPRD_ERROR == mCameraState.capture_state) {
        CMR_LOGE("takePicture in error status, deinit capture at first ");
    } else if (SPRD_IDLE != mCameraState.capture_state) {
        if (isCapturing()) {
            if (!WaitForCaptureDone()) {
                CMR_LOGE("take picture: wait timeout, direct return!");
                goto exit;
            }
        } else {
            CMR_LOGE("take picture: action alread exist, direct return!");
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
    }

    if (NULL == mCameraHandle || NULL == mOem || NULL == mOem->ops) {
        CMR_LOGE("takePicture: oem is null or oem ops is null");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    mOem->ops->camera_fast_ctrl(mCameraHandle, CAMERA_FAST_MODE_FD, 0);

    if (!isZslMode()) {
        if (isPreviewing()) {
            CMR_LOGI("call stopPreview in takePicture.");

            if (stopPreviewInternal()) {
                ret = CMR_CAMERA_FAIL;
                CMR_LOGE(" fail to stop preview ");
                goto exit;
            } else {
                CMR_LOGI("preview stop ");
            }
        }

        if (!isCameraInit()) {
            CMR_LOGE("camera initialize error!, camera state = %d",
                      getCameraStateStr(getCameraState()));
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }

    }

    if (isCapturing()) {
        CMR_LOGI("Wait For Capture Done");
        WaitForCaptureDone();
    }

    setCameraState(SPRD_INTERNAL_RAW_REQUESTED, STATE_CAPTURE);

    setSnapshotParams();

    if (CMR_CAMERA_SUCCESS !=
        mOem->ops->camera_take_picture(mCameraHandle, mCaptureMode)) {
        setCameraState(SPRD_ERROR, STATE_CAPTURE);
        CMR_LOGE("takePicture: fail to camera_take_picture.");

        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    CMR_LOGI("X");
exit:
    return ret;
}

status_t CameraOemTest::cancelPicture() {
    bool result = true;
    CMR_LOGI("cancelPictureInternal: E, state = %s",
             getCameraStateStr(getCaptureState()));
    if (NULL == mCameraHandle || NULL == mOem || NULL == mOem->ops) {
        CMR_LOGE("cancelPictureInternal: oem is null or oem ops is null");
        return -1;
    }

    switch (getCaptureState()) {
    case SPRD_INTERNAL_RAW_REQUESTED:
    case SPRD_WAITING_RAW:
    case SPRD_WAITING_JPEG:
        CMR_LOGI("camera state is %s, stopping picture.",
                 getCameraStateStr(getCaptureState()));

        setCameraState(SPRD_INTERNAL_CAPTURE_STOPPING, STATE_CAPTURE);

        if (0 != mOem->ops->camera_cancel_takepicture(mCameraHandle)) {
            CMR_LOGE("cancelPictureInternal: camera_stop_capture failed!");
            setCameraState(SPRD_ERROR, STATE_CAPTURE);
            return -1;
        }

        result = WaitForCaptureDone();
        if (!isZslMode()) {
            deinitCapture(mIsPreAllocCapMem);
        }
    /*		camera_set_capture_trace(0);*/
        break;

    default:
        CMR_LOGW("not taking a picture (state %s)",
                 getCameraStateStr(getCaptureState()));
        break;
    }

    CMR_LOGI("cancelPictureInternal: X");

    return result ? 0 : -1;
}

void CameraOemTest::deinitCapture(bool isPreAllocCapMem) {
    CMR_LOGV("E %d", isPreAllocCapMem);

    if (0 == isPreAllocCapMem) {
        callback_capture_free(0, 0, 0, 0);
    } else {
        CMR_LOGD("pre_allocate mode.");
    }

//    Callback_CapturePathFree(0, 0, 0, 0);
}

int CameraOemTest::closeCamera() {
    int ret = 0;

    CMR_LOGI(" E ");
    Mutex::Autolock l(&mLock);

    mZslCaptureExitLoop = true;
// Either preview was ongoing, or we are in the middle or taking a
// picture.  It's the caller's responsibility to make sure the camera
// is in the idle or init state before destroying this object.
    CMR_LOGD("camera state = %s, preview state = %s, capture state = %s",
              getCameraStateStr(getCameraState()),
              getCameraStateStr(getPreviewState()),
              getCameraStateStr(getCaptureState()));

    if (isCapturing()) {
        WaitForCaptureDone();
//        cancelPicture();
    }

    if (isPreviewing()) {
        mOem->ops->camera_stop_preview(mCameraHandle);;
    }

    if (isCameraInit()) {
        setCameraState(SPRD_INTERNAL_STOPPING, STATE_CAMERA);

        CMR_LOGI("deinit camera");
        if (CMR_CAMERA_SUCCESS != mOem->ops->camera_deinit(mCameraHandle)) {
            setCameraState(SPRD_ERROR, STATE_CAMERA);
            mReleaseFLag = true;
            CMR_LOGE("camera_deinit failed");
            ret = -1;
            return ret;
        }

        WaitForCameraStop();
    }

    CMR_LOGI("X");
    return ret;
}

int CameraOemTest::snapshot_zsl() {
    int ret = CMR_CAMERA_SUCCESS;
    struct camera_frame_type zsl_frame;
    int sleep_rtn = 0;
    int cnt = 1;

    while (1) {
        // for exception exit
        if (mZslCaptureExitLoop == true) {
            CMR_LOGD("zsl loop exit done.");
            break;
        }

        zsl_frame = popZslFrame();
        if (zsl_frame.y_vir_addr == 0) {
            CMR_LOGD("wait for correct zsl frame");
            sleep_rtn = usleep(20* 1000);
            if (sleep_rtn) {
                CMR_LOGE("ERROR: no pause for 20 ms");
            }

            if (cnt * 20 == 2000) {
                CMR_LOGE("ERROR: wait for 2000ms, timeout");
                ret = CMR_CAMERA_FAIL;
                break;
            }
            cnt++;
            continue;
        }

        CMR_LOGD("fd=0x%x, frame type = %d", zsl_frame.fd, zsl_frame.type);

        if (zsl_frame.y_vir_addr != 0) {
            mOem->ops->camera_set_zsl_snapshot_buffer(
                mCameraHandle, zsl_frame.y_phy_addr, zsl_frame.y_vir_addr,
                zsl_frame.fd);

            CMR_LOGI("frame addr_vir = 0x%lx, fd = %x",
                      zsl_frame.y_vir_addr, zsl_frame.fd);

            break;
        } else {
            CMR_LOGD("zsl_frame.y_vir_addr = 0");
        }

        if (ret){
            CMR_LOGD("snapshot_zsl fail");
        }
    }

    return ret;
    CMR_LOGD("X");
}

int CameraOemTest::pushZslFrame(struct camera_frame_type *frame) {
    int ret = 0;
    ZslBufferQueue zsl_buffer_q;

    frame->buf_id = get_zsl_buffer_id_for_fd(frame->fd);
    if (frame->buf_id != 0xFFFFFFFF) {
        memset(&zsl_buffer_q, 0, sizeof(zsl_buffer_q));
        zsl_buffer_q.frame = *frame;
        zsl_buffer_q.heap_array = mZslHeapArray[frame->buf_id];
        pushZSLQueue(&zsl_buffer_q);
    }

    return ret;
}

struct camera_frame_type CameraOemTest::popZslFrame() {
    ZslBufferQueue zslFrame;
    struct camera_frame_type zsl_frame;

    bzero(&zslFrame, sizeof(zslFrame));
    bzero(&zsl_frame, sizeof(struct camera_frame_type));

    zslFrame = popZSLQueue();
    zsl_frame = zslFrame.frame;

    return zsl_frame;
}

void CameraOemTest::pushZSLQueue(ZslBufferQueue *frame) {
    Mutex::Autolock l(&mZslLock);
    mZSLQueue.push_back(*frame);
}

ZslBufferQueue CameraOemTest::popZSLQueue() {
    Mutex::Autolock l(&mZslLock);
    List<ZslBufferQueue>::iterator frame;
    ZslBufferQueue ret;

    bzero(&ret, sizeof(ZslBufferQueue));
    if (mZSLQueue.size() == 0) {
        return ret;
    }
    frame = mZSLQueue.begin()++;
    ret = static_cast<ZslBufferQueue>(*frame);
    mZSLQueue.erase(frame);

    return ret;
}

void CameraOemTest::releaseZSLQueue() {

    Mutex::Autolock l(&mZslLock);
    List<ZslBufferQueue>::iterator round;

    CMR_LOGD("para changed.size : %d", mZSLQueue.size());
    while (mZSLQueue.size() > 0) {
        round = mZSLQueue.begin()++;
        mZSLQueue.erase(round);
    }

}

int CameraOemTest::setSnapshotParams() {
    struct img_size preview_size = {0, 0}, video_size = {0, 0};
    struct img_size callback_size = {0, 0}, capture_size = {0, 0};
    struct img_size req_size = {0, 0};
    struct cmr_zoom_param zoom_param;
    zoom_param.mode = ZOOM_LEVEL;
    zoom_param.zoom_level = 0;
    mSprdZslEnabled = false;
    mPicCaptureCnt = 1;
    int ret = 0;

    if (NULL == mCameraHandle || NULL == mOem || NULL == mOem->ops) {
        CMR_LOGE("oem is null or oem ops is null");
        ret = -1;
        return ret;
    }

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_VIDEO_SIZE,
             (cmr_uint)&video_size);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_YUV_CALLBACK_SIZE,
             (cmr_uint)&callback_size);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_ROTATION_CAPTURE, 0);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SCENE_MODE, 0);

    capture_size.width = mCaptureWidth;
    capture_size.height = mCaptureHeight;
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_CAPTURE_SIZE,
             (cmr_uint)&capture_size);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_CAPTURE_FORMAT,
             (cmr_uint)mPictureFormat);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_THUMB_SIZE,
        (cmr_uint)&req_size);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_JPEG_QUALITY, 95);
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_THUMB_QUALITY, 0);

    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SHOT_NUM, mPicCaptureCnt);
//zsl enable
    SET_PARM(mOem, mCameraHandle, CAMERA_PARAM_SPRD_ZSL_ENABLED,
             (cmr_uint)mSprdZslEnabled);

    return 0;
}

void CameraOemTest::camera_cb(enum camera_cb_type cb, const void *client_data,
                              enum camera_func_type func, void *parm4) {

    CameraOemTest *obj = (CameraOemTest *)client_data;
    CMR_LOGD("cb = %d func = %d parm4 = %p", cb, func, parm4);
    switch (func) {
    case CAMERA_FUNC_START_PREVIEW:
        obj->HandleStartPreview(cb, parm4);
        break;
    case CAMERA_FUNC_STOP_PREVIEW:
        obj->HandleStopPreview(cb, parm4);
        break;
/*
    case CAMERA_FUNC_TAKE_PICTURE:
        obj->HandleTakePicture(cb, parm4);
        break;
*/
    case CAMERA_FUNC_ENCODE_PICTURE:
        obj->HandleEncode(cb, parm4);
        break;

    case CAMERA_FUNC_STOP:
        obj->HandleStopCamera(cb, parm4);
        break;

    default:
        CMR_LOGE("unknown camera-callback status %d", cb);
        break;
    }


}

void CameraOemTest::HandleStartPreview(enum camera_cb_type cb, void *parm4) {

    switch (cb) {
    case CAMERA_RSP_CB_SUCCESS:
        setCameraState(SPRD_PREVIEW_IN_PROGRESS, STATE_PREVIEW);
        break;

    case CAMERA_EVT_CB_FRAME:
        switch (getPreviewState()) {
        case SPRD_PREVIEW_IN_PROGRESS:
            receivePreviewFrame((camera_frame_type *)parm4);
            break;

        case SPRD_INTERNAL_PREVIEW_STOPPING:
            CMR_LOGI("camera cb: discarding preview frame "
                 "while stopping preview");
            break;

        default:
            CMR_LOGW("HandleStartPreview: invalid state");
            break;
        }
        break;

    case CAMERA_EXIT_CB_PREPARE:
        break;

    case CAMERA_EXIT_CB_FAILED:
        CMR_LOGE("SprdCameraHardware::camera_cb: @CAMERA_EXIT_CB_FAILURE(%ld) in "
             "state %s",
             (cmr_uint)parm4, getCameraStateStr(getPreviewState()));
        transitionState(getPreviewState(), SPRD_ERROR, STATE_PREVIEW);
        break;

    default:
        CMR_LOGD("unused camera_cb_type = %d", cb);
        break;
    }
}

void CameraOemTest::HandleStopPreview(enum camera_cb_type cb,
                                      void *parm4) {
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);
    Sprd_camera_state tmpPrevState = SPRD_IDLE;
    tmpPrevState = getPreviewState();
    CMR_LOGI("HandleStopPreview in: cb = %d, parm4 = 0x%lx, state = %s", cb,
             (cmr_uint)parm4, getCameraStateStr(tmpPrevState));

    if ((SPRD_IDLE == tmpPrevState) ||
        (SPRD_INTERNAL_PREVIEW_STOPPING == tmpPrevState)) {
        setCameraState(SPRD_IDLE, STATE_PREVIEW);
    } else {
        CMR_LOGE("HandleStopPreview: error preview status, %s",
        getCameraStateStr(tmpPrevState));
        transitionState(tmpPrevState, SPRD_ERROR, STATE_PREVIEW);
    }

    CMR_LOGI("HandleStopPreview out, state = %s",
             getCameraStateStr(getPreviewState()));
}

void CameraOemTest::HandleEncode(enum camera_cb_type cb, void *parm4) {

    switch (cb) {
    case CAMERA_EXIT_CB_DONE:
        receiveJpegPicture((struct camera_frame_type *)parm4);
        setCameraState(SPRD_IDLE, STATE_CAPTURE);
        break;

    case CAMERA_EVT_CB_RETURN_ZSL_BUF:
        if (isPreviewing() && isZslMode()) {
            cmr_u32 buf_id;
            struct camera_frame_type *zsl_frame;
            zsl_frame = (struct camera_frame_type *)parm4;
            if (zsl_frame->fd <= 0) {
                CMR_LOGD("zsl lost a buffer, this should not happen");
                goto handle_encode_exit;
            }
            CMR_LOGD("zsl_frame->fd=0x%x", zsl_frame->fd);
            buf_id = get_zsl_buffer_id_for_fd(zsl_frame->fd);
            if (buf_id != 0xFFFFFFFF) {
                mOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, mZslHeapArray[buf_id]->phys_addr,
                    (cmr_uint)mZslHeapArray[buf_id]->data,
                    mZslHeapArray[buf_id]->fd);
            }
        }
        break;

    default:
        CMR_LOGD("unused camera_cb_type = %d", cb);
        break;
    }

handle_encode_exit:
    CMR_LOGD("X, state = %s", getCameraStateStr(getCaptureState()));
}

void CameraOemTest::HandleStopCamera(enum camera_cb_type cb, void *parm4) {
    CMR_LOGI("HandleStopCamera in: cb = %d, parm4 = 0x%lx, state = %s", cb,
             (cmr_uint)parm4, getCameraStateStr(getCameraState()));

    transitionState(SPRD_INTERNAL_STOPPING, SPRD_INIT, STATE_CAMERA);

    CMR_LOGI("HandleStopCamera out, state = %s", getCameraStateStr(
             getCameraState()));
}

void CameraOemTest::receivePreviewFrame(struct camera_frame_type *parm4) {
    int ret = 0;
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
    struct img_addr addr_vir;
    char tag_name[30]={0};
    strcpy(tag_name, "dump_preview");

    CMR_LOGI("frame type = %d",frame->type);
    memset((void *)&addr_vir, 0, sizeof(addr_vir));

    if (frame == NULL) {
        CMR_LOGI("parm4 error: null");
        goto exit;
    }

    if (!mPreviewHeapArray[frame->buf_id]) {
        CMR_LOGI("preview heap array empty");
        goto exit;
    }

    if (!previewvalid) {
        CMR_LOGI("preview disabled");
        goto exit;
    }

    if (PREVIEW_FRAME == frame->type) {
        if (dump_cnt_pre < dump_total_count) {

            addr_vir.addr_y = frame->y_vir_addr;
            addr_vir.addr_u = frame->y_vir_addr + frame->width * frame->height;

            CMR_LOGI("yuv addr_vir:0x%x,0x%x,0x%x\n", addr_vir.addr_y,
                 addr_vir.addr_u, addr_vir.addr_v);

            if (!addr_vir.addr_y || !addr_vir.addr_u) {
                CMR_LOGE("frame addr_vir is empty");
                goto exit;
            }

            dump_image(tag_name, CAM_IMG_FMT_YUV420_NV21, frame->width,
                       frame->height, dump_cnt_pre, &addr_vir,
                       frame->width * frame->height * 3 / 2);//IMG_DATA_TYPE_YUV420
            dump_cnt_pre++;
            CMR_LOGI("preview frame %d has been saved", dump_cnt_pre);
        }
    }

    if (mOem == NULL || mOem->ops == NULL) {
        CMR_LOGE("mOem is null");
        goto exit;
    }

    if (PREVIEW_FRAME == frame->type) {
        frame->buf_id = get_preview_buffer_id_for_fd(frame->fd);
        if (frame->buf_id != 0xFFFFFFFF) {
            mOem->ops->camera_set_preview_buffer(
                mCameraHandle, (cmr_uint)mPreviewHeapArray[frame->buf_id]->phys_addr,
                (cmr_uint)mPreviewHeapArray[frame->buf_id]->data,
                (cmr_s32)mPreviewHeapArray[frame->buf_id]->fd);
        }
    }

//set zsl buffer
    if (mSprdZslEnabled) {
        if (PREVIEW_ZSL_FRAME == frame->type) {

            CMR_LOGD("zsl buff fd=0x%x, frame type=%ld", frame->fd,
                     frame->type);
            pushZslFrame(frame);

            if (getZSLQueueFrameNum() > mZslMaxFrameNum) {
                struct camera_frame_type zsl_frame;
                zsl_frame = popZslFrame();
                if (zsl_frame.y_vir_addr != 0) {
                    ret = mOem->ops->camera_set_zsl_buffer(
                        mCameraHandle, zsl_frame.y_phy_addr,
                        zsl_frame.y_vir_addr, zsl_frame.fd);
                }
            }
            if (ret) {
                CMR_LOGI("camera_set_zsl_buffer fail");
            }

        } else if (PREVIEW_ZSL_CANCELED_FRAME == frame->type) {
            if (!isCapturing()) {
                mOem->ops->camera_set_zsl_buffer(
                    mCameraHandle, frame->y_phy_addr, frame->y_vir_addr,
                    frame->fd);
            }
        }
    }

exit:
    return;

}

void CameraOemTest::receiveJpegPicture(struct camera_frame_type *parm4) {

    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
    struct img_addr addr_vir;

    CMR_LOGI("frame type = %d",frame->type);
    memset((void *)&addr_vir, 0, sizeof(addr_vir));

    Mutex::Autolock cbLock(&mCaptureCbLock);
    Mutex::Autolock cbPreviewLock(&mPreviewCbLock);

    if (frame == NULL) {
        CMR_LOGI("parm4 error: null");
        goto exit;
    }

    if (!previewvalid) {
        CMR_LOGI("zsl disabled");
        goto exit;
    }

    if (!mZslHeapArray[frame->buf_id] && !mSubRawHeapArray[frame->buf_id]) {
        CMR_LOGI("heap array empty");
        goto exit;
    }

    addr_vir.addr_y = (cmr_uint)(frame->jpeg_param.outPtr);
    if (!addr_vir.addr_y) {
        CMR_LOGE("frame addr_vir is empty");
        goto exit;
    }

    if (capture_cnt_save < capture_total_count) {
        dump_image(snapshot_tag_name, CAM_IMG_FMT_JPEG, mCaptureWidth,
                   mCaptureHeight, capture_cnt_save, &addr_vir,
                   frame->jpeg_param.size);//IMG_DATA_TYPE_YUV420
        CMR_LOGI("picture %d has been saved", capture_cnt_save);
        capture_cnt_save++;
    }

    if (mOem == NULL || mOem->ops == NULL) {
        CMR_LOGE("mOem is null");
        goto exit;
    }

exit:
    return;

}

void CameraOemTest::setCameraState(Sprd_camera_state state,
                                        state_owner owner) {
    Sprd_camera_state org_state = SPRD_IDLE;
    volatile Sprd_camera_state *state_owner = NULL;
    Mutex::Autolock stateLock(&mStateLock);

    CMR_LOGI("state: %s, owner: %d", getCameraStateStr(state), owner);
    switch (owner) {
    case STATE_CAMERA:
        org_state = mCameraState.camera_state;
        state_owner = &(mCameraState.camera_state);
        break;

    case STATE_PREVIEW:
        org_state = mCameraState.preview_state;
        state_owner = &(mCameraState.preview_state);
        break;

    case STATE_CAPTURE:
        org_state = mCameraState.capture_state;
        state_owner = &(mCameraState.capture_state);
        break;

    case STATE_FOCUS:
        org_state = mCameraState.focus_state;
        state_owner = &(mCameraState.focus_state);
        break;

    case STATE_SET_PARAMS:
        org_state = mCameraState.setParam_state;
        state_owner = &(mCameraState.setParam_state);
        break;
    default:
        CMR_LOGE("owner error!");
        break;
    }

    switch (state) {
  /*camera state*/
    case SPRD_INIT:
        mCameraState.camera_state = SPRD_INIT;
        mCameraState.preview_state = SPRD_IDLE;
        mCameraState.capture_state = SPRD_IDLE;
        mCameraState.focus_state = SPRD_IDLE;
        mCameraState.setParam_state = SPRD_IDLE;
        break;

    case SPRD_IDLE:
        *state_owner = SPRD_IDLE;
        break;

    case SPRD_INTERNAL_STOPPING:
        mCameraState.camera_state = state;
        break;

    case SPRD_ERROR:
        *state_owner = SPRD_ERROR;
        break;

  /*preview state*/
    case SPRD_PREVIEW_IN_PROGRESS:
    case SPRD_INTERNAL_PREVIEW_STOPPING:
    case SPRD_INTERNAL_PREVIEW_REQUESTED:
        mCameraState.preview_state = state;
        break;

  /*capture state*/
    case SPRD_INTERNAL_RAW_REQUESTED:
    case SPRD_WAITING_RAW:
    case SPRD_WAITING_JPEG:
    case SPRD_INTERNAL_CAPTURE_STOPPING:
        mCameraState.capture_state = state;
        break;

  /*focus state*/
    case SPRD_FOCUS_IN_PROGRESS:
        mCameraState.focus_state = state;
        break;

  /*set_param state*/
    case SPRD_SET_PARAMS_IN_PROGRESS:
        mCameraState.setParam_state = state;
        break;

    default:
        CMR_LOGE("setCameraState: unknown owner");
        break;
    }

    if (org_state != state)
//        mStateWait.notify_one();
        mStateWait.broadcast();
    /*if state changed should broadcasting*/

    CMR_LOGI("setCameraState: X camera state = %s, preview state = %s, capture "
             "state = %s focus state = %s set param state = %s",
             getCameraStateStr(mCameraState.camera_state),
             getCameraStateStr(mCameraState.preview_state),
             getCameraStateStr(mCameraState.capture_state),
             getCameraStateStr(mCameraState.focus_state),
             getCameraStateStr(mCameraState.setParam_state));

}

CameraOemTest::Sprd_camera_state
CameraOemTest::transitionState(CameraOemTest::Sprd_camera_state from,
                                 CameraOemTest::Sprd_camera_state to,
                                 CameraOemTest::state_owner owner,
                               bool lock) {
    volatile CameraOemTest::Sprd_camera_state *which_ptr = NULL;
    CMR_LOGI("transitionState E");

    if (lock)
        mStateLock.lock();
        CMR_LOGI("transitionState: owner = %d, lock = %d", owner, lock);

    switch (owner) {
    case STATE_CAMERA:
        which_ptr = &mCameraState.camera_state;
        break;

    case STATE_PREVIEW:
        which_ptr = &mCameraState.preview_state;
        break;

    case STATE_CAPTURE:
      which_ptr = &mCameraState.capture_state;
      break;

    case STATE_FOCUS:
        which_ptr = &mCameraState.focus_state;
        break;

    default:
        CMR_LOGI("changeState: error owner");
        break;
    }

    if (NULL != which_ptr) {
        if (from != *which_ptr) {
            to = SPRD_ERROR;
        }

        CMR_LOGI("changeState: %s --> %s", getCameraStateStr(from),
                 getCameraStateStr(to));

      if (*which_ptr != to) {
          *which_ptr = to;
//          mStateWait.notify_one();
          mStateWait.broadcast();
      }
    }

    if (lock)
        mStateLock.unlock();
    CMR_LOGI("transitionState X");

  return to;
}

CameraOemTest::Sprd_camera_state CameraOemTest::getCameraState() {
    CMR_LOGI("getCameraState: %s", getCameraStateStr(mCameraState.camera_state));
    return mCameraState.camera_state;
}

const char *CameraOemTest::getCameraStateStr(CameraOemTest::Sprd_camera_state s) {
    static const char *states[] = {
#define STATE_STR(x) #x
        STATE_STR(SPRD_INIT),
        STATE_STR(SPRD_IDLE),
        STATE_STR(SPRD_ERROR),
        STATE_STR(SPRD_PREVIEW_IN_PROGRESS),
        STATE_STR(SPRD_FOCUS_IN_PROGRESS),
        STATE_STR(SPRD_SET_PARAMS_IN_PROGRESS),
        STATE_STR(SPRD_WAITING_RAW),
        STATE_STR(SPRD_WAITING_JPEG),
        STATE_STR(SPRD_FLASH_IN_PROGRESS),
        STATE_STR(SPRD_INTERNAL_PREVIEW_STOPPING),
        STATE_STR(SPRD_INTERNAL_CAPTURE_STOPPING),
        STATE_STR(SPRD_INTERNAL_PREVIEW_REQUESTED),
        STATE_STR(SPRD_INTERNAL_RAW_REQUESTED),
        STATE_STR(SPRD_INTERNAL_STOPPING),
#undef STATE_STR

    };
    return states[s];
}

CameraOemTest::Sprd_camera_state CameraOemTest::getPreviewState() {
    CMR_LOGV("getPreviewState: %s", getCameraStateStr(mCameraState.preview_state));
    return mCameraState.preview_state;
}

CameraOemTest::Sprd_camera_state CameraOemTest::getCaptureState() {
    CMR_LOGI("getCaptureState: %s", getCameraStateStr(mCameraState.capture_state));
    return mCameraState.capture_state;
}

int CameraOemTest::get_preview_buffer_id_for_fd(cmr_s32 fd) {
    unsigned int i = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!mPreviewHeapArray[i])
            continue;

        if (!(cmr_uint)mPreviewHeapArray[i]->fd)
            continue;

        if (mPreviewHeapArray[i]->fd == fd)
            return i;
    }

    return -1;
}

uint32_t CameraOemTest::get_zsl_buffer_id_for_fd(cmr_s32 fd) {
    uint32_t id = 0xFFFFFFFF;
    uint32_t i;
    for (i = 0; i < mZslHeapNum; i++) {
        if (mZslHeapArray[i] == NULL) {
            CMR_LOGE("mZslHeapArray[%d] is null", i);
            goto exit;
        }

        if (0 != mZslHeapArray[i]->fd && mZslHeapArray[i]->fd == fd) {
            id = i;
            break;
        }
    }

exit:
    return id;
}

int CameraOemTest::getZSLQueueFrameNum() {
    Mutex::Autolock l(&mZslLock);

    int ret = 0;
    ret = mZSLQueue.size();
    //CMR_LOGV("%d", ret);
    return ret;
}

int CameraOemTest::getLargestSensorSize(cmr_uint cameraId, cmr_u16 *width,
                                        cmr_u16 *height) {
    PHYSICAL_SENSOR_INFO_T *ptr = sensorGetPhysicalSnsInfo(cameraId);
    *width = ptr->source_width_max;
    *height = ptr->source_height_max;
    CMR_LOGD("camera id = %d, max_width =%d, max_height = %d", cameraId, *width,
             *height);
    return 0;
}

int CameraOemTest::iommu_is_enabled(void) {
    int ret;
    int iommuIsEnabled = 0;

    if (mCameraHandle == NULL || mOem == NULL || mOem->ops == NULL) {
        CMR_LOGE("failed: input param is null");
        return 0;
    }

    ret = mOem->ops->camera_get_iommu_status(mCameraHandle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

bool CameraOemTest::isCameraInit() {
    CMR_LOGV("%s", getCameraStateStr(mCameraState.camera_state));
    return (SPRD_IDLE == mCameraState.camera_state);
}

bool CameraOemTest::isPreviewing() {
    CMR_LOGV("isPreviewing: %s", getCameraStateStr(mCameraState.preview_state));
    return (SPRD_PREVIEW_IN_PROGRESS == mCameraState.preview_state);
}

bool CameraOemTest::isCapturing() {
    bool ret = false;
    if (SPRD_FLASH_IN_PROGRESS == mCameraState.capture_state) {
        return false;
    }

    if ((SPRD_INTERNAL_RAW_REQUESTED == mCameraState.capture_state) ||
        (SPRD_WAITING_RAW == mCameraState.capture_state) ||
        (SPRD_WAITING_JPEG == mCameraState.capture_state)) {
        ret = true;
    } else if ((SPRD_INTERNAL_CAPTURE_STOPPING == mCameraState.capture_state) ||
               (SPRD_ERROR == mCameraState.capture_state)) {
        mCameraState.capture_state = SPRD_IDLE;
        CMR_LOGD("mCameraState.capture_state = %d", mCameraState.capture_state);
    } else if (SPRD_IDLE != mCameraState.capture_state) {
        CMR_LOGE("error: unknown capture status");
        ret = true;
    }
    return ret;
}

bool CameraOemTest::isZslMode() {
    bool ret = true;

    if (CAMERA_ZSL_MODE != mCaptureMode)
        ret = false;

    if (mSprdZslEnabled == 0)
        ret = false;

    return ret;
}

bool CameraOemTest::WaitForCaptureDone() {
    Mutex::Autolock stateLock(&mStateLock);
    while (SPRD_IDLE != mCameraState.capture_state &&
           SPRD_ERROR != mCameraState.capture_state) {
        CMR_LOGI("WaitForCaptureDone: waiting for SPRD_IDLE");
        if (mStateWait.waitRelative(mStateLock, CAP_TIMEOUT)) {
            CMR_LOGE("WaitForCaptureDone timeout");
            break;
        }

        CMR_LOGI("WaitForCaptureDone: woke up");
    }

    return SPRD_IDLE == mCameraState.capture_state;
}

bool CameraOemTest::WaitForCameraStop() {
    CMR_LOGI("WaitForCameraStop E.\n");
    Mutex::Autolock stateLock(&mStateLock);

    if (SPRD_INTERNAL_STOPPING == mCameraState.camera_state) {
      while (SPRD_INIT != mCameraState.camera_state &&
             SPRD_ERROR != mCameraState.camera_state) {
        CMR_LOGI("WaitForCameraStop: waiting for SPRD_IDLE");
//        mStateWait.wait(slck);
        mStateWait.wait(mStateLock);
        CMR_LOGI("woke up");
      }
    }
    CMR_LOGI("WaitForCameraStop X.\n");

    return SPRD_INIT == mCameraState.camera_state;
}

sprd_camera_memory_t *CameraOemTest::alloc_camera_mem(int buf_size, int num_bufs,
                                                      uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    CMR_LOGI("buf_size=%d, num_bufs=%d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        CMR_LOGE("failed: fatal error! memory pointer is null");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        CMR_LOGE("failed: mem size err");
        goto getpmem_fail;
    }

    if (0 == is_iommu_enabled) {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        CMR_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || ((void *)-1) == pHeapIon->getBase()) {
        CMR_LOGE("error getBase is null. failed: ion "
                 "get base err");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    CMR_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx"
             "heap = %p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }

    if (pHeapIon) {
        delete pHeapIon;
        pHeapIon = NULL;
    }

    return NULL;
}

cmr_int CameraOemTest::callback_malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                                       cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                                       cmr_uint *vir_addr, cmr_s32 *fd,
                                       void *private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;

    UNUSED(private_data);
    CameraOemTest *camera = (CameraOemTest *)private_data;

    CMR_LOGI("type=%d", type);

    if (phy_addr == NULL || vir_addr == NULL || size_ptr == NULL ||
        sum_ptr == NULL || (0 == *size_ptr) || (0 == *sum_ptr)) {
        CMR_LOGE("alloc error 0x%lx 0x%lx 0x%lx", (cmr_uint)phy_addr,
                 (cmr_uint)vir_addr, (cmr_uint)size_ptr);
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        ret = camera->callback_preview_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT == type) {
        ret = camera->callback_capture_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->callback_zsl_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED ||
               type == CAMERA_SNAPSHOT_ZSL_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_RAWAE || type == CAMERA_ISP_BINGING4AWB ||
               type == CAMERA_ISP_ANTI_FLICKER || type == CAMERA_ISPSTATS_AEM ||
               type == CAMERA_ISPSTATS_AFM || type == CAMERA_ISPSTATS_AFL ||
               type == CAMERA_ISPSTATS_YUVHIST || type == CAMERA_ISPSTATS_3DNR ||
               type == CAMERA_ISPSTATS_EBD || type == CAMERA_ISPSTATS_PDAF ||
               type == CAMERA_ISPSTATS_BAYERHIST || type == CAMERA_ISPSTATS_LSCM ) {
        ret = camera->callback_other_malloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    camera->previewvalid = 1;
    return ret;
}

int CameraOemTest::callback_preview_malloc(cmr_u32 size, cmr_u32 sum,
                                           cmr_uint *phy_addr, cmr_uint *vir_addr,
                                           cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;

    CMR_LOGI("size=%d sum=%d mPreviewHeapNum=%d", size, sum, mPreviewHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            CMR_LOGE("alloc_camera_mem failed");
            goto mem_fail;
        }

        mPreviewHeapArray[i] = memory;
        mPreviewHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_preview_free(0, 0, 0, 0);
    return -1;
}

int CameraOemTest::callback_capture_malloc(cmr_u32 size, cmr_u32 sum,
                                           cmr_uint *phy_addr,
                                           cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    CMR_LOGD("size %d sum %d mSubRawHeapNum %d", size, sum, mZslHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

cap_malloc:
    Mutex::Autolock l(&mCapBufLock);

    if (mSubRawHeapNum >= MAX_SUB_RAWHEAP_NUM) {
        CMR_LOGE("error mSubRawHeapNum %d", mSubRawHeapNum);
        return -EINVAL;
    }

    if ((mSubRawHeapNum + sum) >= MAX_SUB_RAWHEAP_NUM) {
        CMR_LOGE("malloc is too more %d %d", mSubRawHeapNum, sum);
        return -EINVAL;
    }

    if (0 == mSubRawHeapNum) {
        for (i = 0; i < (cmr_int)sum; i++) {
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                CMR_LOGE("error memory is null.");
                goto mem_fail;
            }

            mSubRawHeapArray[mSubRawHeapNum] = memory;
            mSubRawHeapNum++;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
        mSubRawHeapSize = size;
    } else {
        if ((mSubRawHeapNum >= sum) && (mSubRawHeapSize >= size)) {
            CMR_LOGD("use pre-alloc cap mem");
            for (i = 0; i < (cmr_int)sum; i++) {
                *phy_addr++ = (cmr_uint)mSubRawHeapArray[i]->phys_addr;
                *vir_addr++ = (cmr_uint)mSubRawHeapArray[i]->data;
                *fd++ = mSubRawHeapArray[i]->fd;
            }
        } else {
            mCapBufLock.unlock();
            callback_capture_free(0, 0, 0, 0);
            goto cap_malloc;
        }
    }

    mCapBufLock.unlock();

    return 0;

mem_fail:
    mCapBufLock.unlock();
    callback_capture_free(0, 0, 0, 0);
    return -1;
}

int CameraOemTest::callback_zsl_malloc(cmr_u32 size, cmr_u32 sum,
                                       cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_int i = 0;

    CMR_LOGI("size %d sum %d mZslHeapNum %d", size, sum, mZslHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    for (i = 0; i < ZSL_BUFF_NUM; i++) {
        memory = alloc_camera_mem(size, 1, true);

        if (NULL == memory) {
            CMR_LOGE("error memory is null.");
            goto mem_fail;
        }

        mZslHeapArray[i] = memory;
        mZslHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_zsl_free(0, 0, 0, 0);
    return -1;

}

int CameraOemTest::callback_other_malloc(enum camera_mem_cb_type type,
                                         cmr_u32 size, cmr_u32 sum,
                                         cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;

    CMR_LOGD("size=%d sum=%d mPreviewHeapNum=%d, type=%d", size, sum,
             mPreviewHeapNum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == mPreviewHeapReserved) {
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mPreviewHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            CMR_LOGI("type=%d,request num=%d, request size=0x%x", type, sum,
                     size);
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
            *fd++ = mPreviewHeapReserved->fd;
        }
    } else if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        if (mZslHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                CMR_LOGE("memory is null.");
                goto mem_fail;
            }
            mZslHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mZslHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mZslHeapReserved->data;
        *fd++ = mZslHeapReserved->fd;
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
            *fd++ = mIspLscHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (mIspStatisHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspStatisHeapReserved = memory;
        }
#if defined(CONFIG_ISP_2_6)
// sharkl5 dont have get_kaddr interface
// m_isp_statis_heap_reserved->ion_heap->get_kaddr(&kaddr, &ksize);
#else
        mIspStatisHeapReserved->ion_heap->get_kaddr(&kaddr, &ksize);
#endif
        *phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)mIspStatisHeapReserved->data;
        *fd++ = mIspStatisHeapReserved->fd;
        *fd++ = mIspStatisHeapReserved->dev_fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_RAWAE) {
        cmr_u64 kaddr = 0;
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        size_t ksize = 0;
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspRawAemHeapReserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAFLHeapReserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAFLHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            *phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
            *fd++ = mIspAFLHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;

    // CAMERA_ISPSTATS_AEM
    } else if (type == CAMERA_ISPSTATS_AEM) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAemHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_AFM) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAfmHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_AFL) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspAflHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_PDAF) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspPdafHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_BAYERHIST) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspBayerhistHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_YUVHIST) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspYuvhistHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_LSCM) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspLscmHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_3DNR) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIsp3dnrHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISPSTATS_EBD) {
        for (i = 0; i < sum; i++) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                CMR_LOGE("alloc_camera_mem failed");
                goto mem_fail;
            }
            mIspEbdHeapReserved[i] = memory;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
//
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    return 0;

mem_fail:
    callback_other_free(type, 0, 0, 0, 0);
    return -1;
}

cmr_int CameraOemTest::callback_free(enum camera_mem_cb_type type,
                                     cmr_uint *phy_addr, cmr_uint *vir_addr,
                                     cmr_s32 *fd, cmr_u32 sum, void *private_data) {
    CameraOemTest *camera = (CameraOemTest *)private_data;
    cmr_int ret = 0;

    if (private_data == NULL || vir_addr == NULL || fd == NULL) {
        CMR_LOGE("error param 0x%lx 0x%lx %p 0x%lx", (cmr_uint)phy_addr,
                 (cmr_uint)vir_addr, fd, (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        CMR_LOGE("mem type error %d", type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = camera->callback_preview_free(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT == type) {
        ret = camera->callback_capture_free(phy_addr, vir_addr, fd, sum);
    } else if (CAMERA_SNAPSHOT_ZSL == type) {
        ret = camera->callback_zsl_free(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED ||
               type == CAMERA_SNAPSHOT_ZSL_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER || type == CAMERA_ISPSTATS_AEM ||
             type == CAMERA_ISPSTATS_AFM || type == CAMERA_ISPSTATS_AFL ||
             type == CAMERA_ISPSTATS_YUVHIST || type == CAMERA_ISPSTATS_3DNR ||
             type == CAMERA_ISPSTATS_EBD || type == CAMERA_ISPSTATS_PDAF ||
             type == CAMERA_ISPSTATS_BAYERHIST || type == CAMERA_ISPSTATS_LSCM) {
        ret = camera->callback_other_free(type, phy_addr, vir_addr, fd, sum);
    } else {
        CMR_LOGE("type ignore: %d, do nothing", type);
    }

    camera->previewvalid = 0;

    return ret;
}

int CameraOemTest::callback_preview_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                         cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    for (i = 0; i < mPreviewHeapNum; i++) {
        if (!mPreviewHeapArray[i])
            continue;

        free_camera_mem(mPreviewHeapArray[i]);
        mPreviewHeapArray[i] = NULL;
    }

    mPreviewHeapNum = 0;

    return 0;
}

int CameraOemTest::callback_capture_free(cmr_uint *phy_addr,
                                         cmr_uint *vir_addr, cmr_s32 *fd,
                                         cmr_u32 sum) {
    cmr_u32 i;
    CMR_LOGD("mSubRawHeapNum %d sum %d", mSubRawHeapNum, sum);
    Mutex::Autolock l(&mCapBufLock);

    for (i = 0; i < mSubRawHeapNum; i++) {
        if (NULL != mSubRawHeapArray[i]) {
            free_camera_mem(mSubRawHeapArray[i]);
        }
        mSubRawHeapArray[i] = NULL;
    }
    mSubRawHeapNum = 0;
    mSubRawHeapSize = 0;

    return 0;
}

int CameraOemTest::callback_zsl_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                     cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;
    Mutex::Autolock l(&mPrevBufLock);
    Mutex::Autolock zsllock(&mZslBufLock);

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    for (i = 0; i < mZslHeapNum; i++) {
        if (!mZslHeapArray[i])
            continue;

        free_camera_mem(mZslHeapArray[i]);
        mZslHeapArray[i] = NULL;
    }

    mZslHeapNum = 0;
    releaseZSLQueue();

    return 0;

}

int CameraOemTest::callback_other_free(enum camera_mem_cb_type type,
                                       cmr_uint *phy_addr, cmr_uint *vir_addr,
                                       cmr_s32 *fd, cmr_u32 sum) {
    unsigned int i;

    UNUSED(phy_addr);
    UNUSED(vir_addr);
    UNUSED(fd);
    UNUSED(sum);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != mPreviewHeapReserved) {
            free_camera_mem(mPreviewHeapReserved);
            mPreviewHeapReserved = NULL;
        }
    }

     if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        if (NULL != mZslHeapReserved) {
            free_camera_mem(mZslHeapReserved);
            mZslHeapReserved = NULL;
         }
     }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            free_camera_mem(mIspLscHeapReserved);
            mIspLscHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                free_camera_mem(mIspB4awbHeapReserved[i]);
                mIspB4awbHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            free_camera_mem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_STATIS) {
        if (NULL != mIspStatisHeapReserved) {
            mIspStatisHeapReserved->ion_heap->free_kaddr();
            free_camera_mem(mIspStatisHeapReserved);
            mIspStatisHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspRawAemHeapReserved[i]) {
                mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
                free_camera_mem(mIspRawAemHeapReserved[i]);
            }
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAFLHeapReserved) {
            free_camera_mem(mIspAFLHeapReserved);
            mIspAFLHeapReserved = NULL;
        }
    }

//
    if (type == CAMERA_ISPSTATS_AEM) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspAemHeapReserved[i]) {
                free_camera_mem(mIspAemHeapReserved[i]);
            }
            mIspAemHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_AFM) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspAfmHeapReserved[i]) {
                free_camera_mem(mIspAfmHeapReserved[i]);
            }
            mIspAfmHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_AFL) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspAflHeapReserved[i]) {
                free_camera_mem(mIspAflHeapReserved[i]);
            }
            mIspAflHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_PDAF) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspPdafHeapReserved[i]) {
                free_camera_mem(mIspPdafHeapReserved[i]);
            }
            mIspPdafHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_BAYERHIST) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspBayerhistHeapReserved[i]) {
                free_camera_mem(mIspBayerhistHeapReserved[i]);
            }
            mIspBayerhistHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_YUVHIST) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspYuvhistHeapReserved[i]) {
                free_camera_mem(mIspYuvhistHeapReserved[i]);
            }
            mIspYuvhistHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_LSCM) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspLscmHeapReserved[i]) {
                free_camera_mem(mIspLscmHeapReserved[i]);
            }
            mIspLscmHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_3DNR) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIsp3dnrHeapReserved[i]) {
                free_camera_mem(mIsp3dnrHeapReserved[i]);
            }
            mIsp3dnrHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISPSTATS_EBD) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspEbdHeapReserved[i]) {
                free_camera_mem(mIspEbdHeapReserved[i]);
            }
            mIspEbdHeapReserved[i] = NULL;
        }
    }
//

    return 0;
}

void CameraOemTest::free_camera_mem(sprd_camera_memory_t *memory) {
    if (memory) {
        if (memory->ion_heap) {
            CMR_LOGD(
                "fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
                memory->fd, memory->phys_addr, memory->data, memory->phys_size,
                memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            CMR_LOGD("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                     "virt_addr=%p, size=0x%lx",
                     memory->fd, memory->phys_addr, memory->data,
                     memory->phys_size);
        }
        free(memory);
        memory = NULL;
    }
}

void CameraOemTest::freeAllCameraMemIon() {
    int i;
    callback_capture_free(0, 0, 0, 0);

    if (NULL != mPreviewHeapReserved) {
        free_camera_mem(mPreviewHeapReserved);
        mPreviewHeapReserved = NULL;
    }

    if (NULL != mZslHeapReserved) {
        free_camera_mem(mZslHeapReserved);
        mZslHeapReserved = NULL;
    }

    if (NULL != mIspLscHeapReserved) {
        free_camera_mem(mIspLscHeapReserved);
        mIspLscHeapReserved = NULL;
    }

    if (NULL != mIspAFLHeapReserved) {
        free_camera_mem(mIspAFLHeapReserved);
        mIspAFLHeapReserved = NULL;
    }

    if (NULL != mIspFirmwareReserved) {
        free_camera_mem(mIspFirmwareReserved);
        mIspFirmwareReserved = NULL;
    }

    if (NULL != mIspStatisHeapReserved) {
        free_camera_mem(mIspStatisHeapReserved);
        mIspStatisHeapReserved = NULL;
    }

    for (i = 0; i < kISPB4awbCount; i++) {
        if (NULL != mIspB4awbHeapReserved[i]) {
            free_camera_mem(mIspB4awbHeapReserved[i]);
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    for (i = 0; i < kISPB4awbCount; i++) {
        if (NULL != mIspRawAemHeapReserved[i]) {
            free_camera_mem(mIspRawAemHeapReserved[i]);
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (NULL != mPreviewHeapArray[i]) {
            free_camera_mem(mPreviewHeapArray[i]);
            mPreviewHeapArray[i] = NULL;
       }
    }

    for (i = 0; i < ZSL_BUFF_NUM; i++) {
        if (NULL != mZslHeapArray[i]) {
            free_camera_mem(mZslHeapArray[i]);
            mZslHeapArray[i] = NULL;
       }
    }
}

ModuleWrapperOEM::ModuleWrapperOEM() {
    pVec_Result = gMap_Result[THIS_MODULE_NAME];
    IT_LOGD("module_oem succeeds in get Vec_Result");
}

ModuleWrapperOEM::~ModuleWrapperOEM() { IT_LOGD(""); }

int ModuleWrapperOEM::SetUp() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperOEM::TearDown() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperOEM::Run(IParseJson *Json2) {
    int ret = IT_OK;
    OemCaseComm *_json2 = (OemCaseComm *)Json2;
    CMR_LOGD("start oem IT");

    int num = 0;
    num = sensorGetPhysicalSnsNum();
    camera_stream_type_t stream_count;

    if (strcmp(_json2->m_funcName.c_str(),"opencamera")== 0) {

        camera_oem.mOem_cxt->camera_id = _json2->m_cameraID;

        ret = camera_oem.dlLib();
        if (ret) {
            CMR_LOGE("load lib failed");
            goto exit;
        }
        CMR_LOGD("load oem lib success");

        ret = camera_oem.openCamera();
        if (ret) {
            CMR_LOGE(" camera init failed");
            IT_LOGE("camera init failed");
            goto exit;
        }
        IT_LOGD("camera init success");

        resultData_t openResult;
        openResult.available = 1;
        openResult.funcID = _json2->getID();
        openResult.funcName = _json2->m_funcName;
        pVec_Result->push_back(openResult);
    }

    if (strcmp(_json2->m_funcName.c_str(),"startpreview")== 0) {
        CMR_LOGD("startpreview : E");

        camera_oem.dump_total_count = _json2->m_dumpNum;

        for (auto &stream : _json2->m_StreamArr) {
            stream_count = (camera_stream_type_t)stream->s_type;

            switch (stream_count) {
            case CAMERA_STREAM_TYPE_PREVIEW:
                camera_oem.mOem_cxt->fps = stream->s_fps;
                if (stream->s_width > 0 && stream->s_height >0) {
                    camera_oem.mPreviewWidth = stream->s_width;
                    camera_oem.mPreviewHeight = stream->s_height;
                } else {
                    CMR_LOGE("preview stream in startpreview case is null");
                    goto exit;
                }
                break;

            case CAMERA_STREAM_TYPE_ZSL_PREVIEW:
                if (stream->s_width > 0 && stream->s_height >0) {
                    camera_oem.mCaptureWidth = stream->s_width;
                    camera_oem.mCaptureHeight = stream->s_height;
                } else {
                    CMR_LOGE("preview stream in startpreview case is null");
                    goto exit;
                }
                break;

            default:
                CMR_LOGE("camera_stream_type_t = %d: error or wait to be added",
                         (int)stream->s_type);
                goto exit;
                break;
            }
        }

        ret = camera_oem.startPreview();
        if (ret) {
            CMR_LOGE(" camera start preview failed");
            IT_LOGE("camera startpreview failed");
            goto exit;
        }
        IT_LOGD("camera startpreview success");

        resultData previewResult;
        previewResult.available = 1;
        previewResult.ret = 1;
        previewResult.funcID = _json2->getID();
        previewResult.funcName = _json2->m_funcName;
        pVec_Result = gMap_Result[THIS_MODULE_NAME];
        pVec_Result->push_back(previewResult);

    }

    if (strcmp(_json2->m_funcName.c_str(),"stoppreview")== 0) {

        CMR_LOGD("stoppreview : E");
        int time = STATE_WAIT_TIME;

        while (1) {
            if (camera_oem.dump_cnt_pre >= camera_oem.dump_total_count &&
                camera_oem.capture_cnt_save >= camera_oem.capture_total_count) {
                break;
            }

            usleep(STATE_WAIT_TIME*1000);
            CMR_LOGD("wait for preview or zsl frame %d ms",time);
            time += STATE_WAIT_TIME;

            if (time > MAX_WAIT_TIME) {
                CMR_LOGD("wait too long for preview or zsl frame, timeout");
                break;
            }
        }

        ret = camera_oem.stopPreview();

        resultData StopPreviewResult;
        StopPreviewResult.available = 1;
        StopPreviewResult.funcID = _json2->getID();
        StopPreviewResult.funcName = _json2->m_funcName;
        pVec_Result->push_back(StopPreviewResult);

        if (ret) {
            CMR_LOGE("camera stoppreview failed");
            IT_LOGE("camera stoppreview failed");
            goto exit;
        }
        IT_LOGD("camera stoppreview success");

    }

    if (strcmp(_json2->m_funcName.c_str(),"zslsnapshot")== 0) {
        CMR_LOGD("zslSnapshot : E");

        camera_oem.capture_total_count += _json2->m_dumpNum > 0 ? 1 : 0;
        //m_dumpNum=0,don't dump
        strcpy(camera_oem.snapshot_tag_name, "dump_zslsnapshot");
        ret = camera_oem.zslSnapshot();

        resultData zslSnapshotResult;
        zslSnapshotResult.available = 1;
        zslSnapshotResult.ret = 1;
        zslSnapshotResult.funcID = _json2->getID();
        zslSnapshotResult.funcName = _json2->m_funcName;
        pVec_Result = gMap_Result[THIS_MODULE_NAME];
        pVec_Result->push_back(zslSnapshotResult);

        if (ret) {
            CMR_LOGE("camera zslSnapshot failed");
            IT_LOGE("camera zslSnapshot failed");
            goto exit;
        }
        IT_LOGD("camera zslSnapshot success");

    }

    if (strcmp(_json2->m_funcName.c_str(),"nozslsnapshot")== 0) {
        CMR_LOGD("noZslSnapshot : E");

        strcpy(camera_oem.snapshot_tag_name, "dump_nozslsnapshot");
        stream_count =
            (camera_stream_type_t)_json2->m_StreamArr.front()->s_type;
        int _width = _json2->m_StreamArr.front()->s_width;
        int _height = _json2->m_StreamArr.front()->s_height;

        camera_oem.capture_total_count += _json2->m_dumpNum > 0 ? 1 : 0;
        if (stream_count == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT &&
            _width > 0 && _height > 0) {
            camera_oem.mCaptureWidth = _width;
            camera_oem.mCaptureHeight = _height;
        } else {
            CMR_LOGE("stream in noZslSnapshot case error");
            goto exit;
        }

        ret = camera_oem.takePicture();

        resultData noZslResult;
        noZslResult.available = 1;
        noZslResult.ret = 1;
        noZslResult.funcID = _json2->getID();
        noZslResult.funcName = _json2->m_funcName;
        pVec_Result = gMap_Result[THIS_MODULE_NAME];
        pVec_Result->push_back(noZslResult);

        if (ret) {
            CMR_LOGE("camera noZslSnapshot failed");
            IT_LOGE("camera noZslSnapshot failed");
            goto exit;
        }
        IT_LOGD("camera noZslSnapshot success");

    }

    if (strcmp(_json2->m_funcName.c_str(),"closecamera")== 0) {

        CMR_LOGD("closecamera : E");
        int time = STATE_WAIT_TIME;

        while (1) {
            if (camera_oem.capture_cnt_save >= camera_oem.capture_total_count) {
                break;
            }

            usleep(STATE_WAIT_TIME*1000);
            CMR_LOGD("wait for picture %d ms",time);
            time += STATE_WAIT_TIME;

            if (time > MAX_WAIT_TIME) {
                CMR_LOGD("wait too long for picture ");
                break;
            }
        }

        ret = camera_oem.closeCamera();

        resultData closeResult;
        closeResult.available = 1;
        closeResult.ret = 1;
        closeResult.funcID = _json2->getID();
        closeResult.funcName = _json2->m_funcName;
        pVec_Result = gMap_Result[THIS_MODULE_NAME];
        pVec_Result->push_back(closeResult);

        if (ret) {
            CMR_LOGE("camera closeCamera failed");
            IT_LOGE("camera closeCamera failed");
            goto exit;
        }
        IT_LOGD("camera closeCamera success");
    }


    CMR_LOGI("Oem_IT finish and exit");
    return ret;

exit:
    return -IT_ERR;
}

int ModuleWrapperOEM::InitOps() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}
