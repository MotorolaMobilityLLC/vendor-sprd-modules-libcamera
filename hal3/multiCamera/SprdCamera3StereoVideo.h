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
#ifndef SPRDCAMERAMUXER_H_HEADER
#define SPRDCAMERAMUXER_H_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include <sprd_ion.h>
#include <gralloc_priv.h>
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"

namespace sprdcamera {

#define MAX_NUM_CAMERAS    3
#define MAX_NUM_STREAMS    3
#define MAX_QEQUEST_BUF 16
#define MAX_UNMATCHED_QUEUE_SIZE 3
#define TIME_DIFF (100e6)
#define LIB_GPU_PATH "libimagestitcher.so"
#define CONTEXT_SUCCESS 1
#define CONTEXT_FAIL 0
#define THREAD_TIMEOUT    50e6
#define MAX_NOTIFY_QUEUE_SIZE 100
#define CLEAR_NOTIFY_QUEUE 50

typedef struct {
    struct private_handle_t *left_buf;
    struct private_handle_t *right_buf;
    struct private_handle_t *dst_buf;
    int rot_angle;
}dcam_info_t;

enum rot_angle {
    ROT_0 = 0,
    ROT_90,
    ROT_180,
    ROT_270,
    ROT_MAX
};

struct stream_info_s
{
    int src_width;
    int src_height;
    int dst_width;
    int dst_height;
    int rot_angle;
};//stream_info_t;

typedef struct line_buf_s
{
   float homography_matrix[18];
   float warp_matrix[18];
   int   points[32];
}line_buf_t;

typedef enum {
    /* Main camera of the related cam subsystem which controls*/
    CAM_TYPE_MAIN = 0,
    /* Aux camera of the related cam subsystem */
    CAM_TYPE_AUX
} CameraType;

typedef enum {
    /* Main camera device id*/
    CAM_MAIN_ID =1,
    /* Aux camera device id*/
    CAM_AUX_ID =3
} CameraID;

typedef enum {
    MUXER_MSG_DATA_PROC = 1,
    MUXER_MSG_EXIT
} muxerMsgType;

typedef struct {
    int frameId;
    camera3_notify_msg_t* notifyMsg;
    camera3_capture_result_t* result;
}muxer_result_notify;

typedef struct {
    muxerMsgType  msg_type;
    frame_matched_info_t combo_frame;
} muxer_queue_msg_t;

typedef struct{
    void* handle;
    int (*initRenderContext)(struct stream_info_s *resolution,float *homography_matrix,int matrix_size);
    void (*imageStitchingWithGPU)(dcam_info_t *dcam);
    void (*destroyRenderContext)(void);
}GPUAPI_t;

class SprdCamera3StereoVideo
{
public:
    static void getCameraMuxer(SprdCamera3StereoVideo** pMuxer);
    static int camera_device_open(
        __unused const struct hw_module_t *module, const char *id,
        struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int initialize(const struct camera3_device *device, const camera3_callback_ops_t *ops);
    static int configure_streams(const struct camera3_device *device, camera3_stream_configuration_t* stream_list);
    static const camera_metadata_t *construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device* device, camera3_capture_request_t *request);
    static void notifyMain(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg);
    static void process_capture_result_main(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result);
    static void process_capture_result_aux(const struct camera3_callback_ops *ops, const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops, const camera3_notify_msg_t* msg);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);

    static camera3_device_ops_t mCameraMuxerOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock1;
    camera_metadata_t *mStaticMetadata;
    int mLastWidth;
    int mLastHeight;
    camera3_stream_t mAuxStreams[MAX_QEQUEST_BUF];
    uint8_t mVideoStreamsNum;
    bool mIsRecording;
    //when notify callback ,push hw notify into notify_list, with lock
    List <camera3_notify_msg_t> mNotifyListMain;
    Mutex mNotifyLockMain;
    List <camera3_notify_msg_t> mNotifyListAux;
    Mutex mNotifyLockAux;

    //This queue stores unmatched buffer for each hwi, accessed with lock
    List <hwi_frame_buffer_info_t> mUnmatchedFrameListMain;
    List <hwi_frame_buffer_info_t> mUnmatchedFrameListAux;
    Mutex mUnmatchedQueueLock;

    Mutex      mMergequeueMutex;
    Condition  mMergequeueSignal;

    int cameraDeviceOpen(int camera_id,struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(struct camera_info *info);
    int validateCaptureRequest(camera3_capture_request_t *request);
    void saveVideoRequest(camera3_capture_request_t *request);
    int  pushRequestList( buffer_handle_t *request,List <buffer_handle_t*>&);
    buffer_handle_t * popRequestList(List <buffer_handle_t*>& list);
    bool matchTwoFrame(hwi_frame_buffer_info_t result1,List <hwi_frame_buffer_info_t> &list, hwi_frame_buffer_info_t* result2);
    int getStreamType(camera3_stream_t *new_stream );
    hwi_frame_buffer_info_t* pushToUnmatchedQueue(hwi_frame_buffer_info_t new_buffer_info, List <hwi_frame_buffer_info_t> &queue);

public:

    SprdCamera3StereoVideo();
    virtual ~SprdCamera3StereoVideo();

    class MuxerThread: public Thread
    {
    public:
        MuxerThread();
        ~MuxerThread();
        virtual bool  threadLoop();
        virtual void requestExit();
        void videoErrorCallback(uint32_t frame_number);
        int loadGpuApi();
        void unLoadGpuApi();
        void freeLocalBuffer(new_mem_t* mLocalBuffer);
        int allocateOne(int w,int h,uint32_t is_cache,new_mem_t*,const native_handle_t *& nBuf );
    void initGpuData(int w,int h,int );
        GPUAPI_t* mGpuApi;
        //This queue stores matched buffer as frame_matched_info_t
        List <muxer_queue_msg_t> mMuxerMsgList;
        List <old_video_request > mOldVideoRequestList;
        List<buffer_handle_t*> mLocalBufferList;

        Mutex      mMergequeueMutex;
        Condition  mMergequeueSignal;
        const camera3_callback_ops_t* mCallbackOps;
        uint8_t mMaxLocalBufferNum;
        const native_handle_t* mNativeBuffer[MAX_QEQUEST_BUF];
        new_mem_t* mLocalBuffer;
        line_buf_t  pt_line_buf  ;
        struct stream_info_s pt_stream_info;
        bool isInitRenderContest;
    private:
        bool mIommuEnabled;
        int mVFrameCount;
        int mVLastFrameCount;
        nsecs_t mVLastFpsTime;
        double mVFps;
        void dumpFps();
        void waitMsgAvailable();
        int muxerTwoFrame(/*out*/buffer_handle_t* &output_buf, frame_matched_info_t* combVideoResult);
        void videoCallBackResult(buffer_handle_t* buffer, frame_matched_info_t* combVideoResult);
    };
    sp<MuxerThread> mMuxerThread;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,camera3_stream_configuration_t* stream_list);
    int processCaptureRequest(const struct camera3_device *device,camera3_capture_request_t *request);
    void notifyMain( const camera3_notify_msg_t* msg);
    void processCaptureResultMain( const camera3_capture_result_t *result);
    void notifyAux( const camera3_notify_msg_t* msg);
    void processCaptureResultAux( const camera3_capture_result_t *result);
    const camera_metadata_t *constructDefaultRequestSettings( const struct camera3_device *device,int type);
    void _dump(const struct camera3_device *device, int fd);
    void dumpImg(void* addr,int size,int fd);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();

};

};

#endif /* SPRDCAMERAMU*/

