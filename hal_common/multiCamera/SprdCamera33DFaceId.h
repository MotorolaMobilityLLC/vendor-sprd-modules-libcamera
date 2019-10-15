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
#ifndef SPRDCAMERA_IR_FACEID_H_HEADER
#define SPRDCAMERA_IR_FACEID_H_HEADER

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
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"
#include "SprdCamera3MultiBase.h"
#include <unistd.h>

namespace sprdcamera {

#define THREE_D_FACEID_MAX_STREAMS 2
#define THREE_D_FACEID_BUFFER_SUM 4

#define THREE_D_FACEID_WIDTH 640
#define THREE_D_FACEID_HEIGHT 480

typedef struct
{
    int capture_width;
    int capture_height;
    int depth_width;
    int depth_height;
    int calib_width;
    int calib_height;
    int color_raw_width;
    int color_raw_height;
}FACEID_IR_PARAM_T;

typedef enum { CAM_MAIN_3D_FACE_ID = 0, CAM_AUX_FACE_ID_L = 2, CAM_AUX_FACE_ID_R = 3 } Three_D_FaceId;
typedef enum { MAIN_BUFFER_3D, AUX_BUFFER_L, AUX_BUFFER_R, FACE_ADDR_BUFFER} buffer_type_3D;

class SprdCamera33DFaceId : SprdCamera3MultiBase {
  public:
    static void getCameraFaceId(SprdCamera33DFaceId **pFaceid);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static int configure_streams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *device,
                                       camera3_capture_request_t *request);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_aux_L(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAuxL(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static void
    process_capture_result_aux_R(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAuxR(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux_L;
    static camera3_callback_ops callback_ops_aux_R;

    SprdCamera33DFaceId();
    virtual ~SprdCamera33DFaceId();

  private:
    const camera3_callback_ops_t *mCallbackOps;
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    camera_metadata_t *mStaticMetadata;
    int mPhyCameraNum;
    int mPreviewWidth;
    int mPreviewHeight;
    int mIRWidth;
    int mIRHeight;
    int mMaxPendingCount;
    int mPendingRequest;
    uint64_t mReqTimestamp;
    int mFaceMode;
    bool mFlushing;
    Mutex mLock;
    Mutex mRequestLock;
    Mutex mMainLock;
    Mutex mAuxLockL;
    Mutex mAuxLockR;
    Mutex mToFaceLock;
    Mutex mPendingLock;
    Condition mRequestSignal;
    List<multi_request_saved_t> mSavedRequestList;
    List<new_mem_t *> mLocalBufferListMain;
    List<new_mem_t *> mLocalBufferListAuxL;
    List<new_mem_t *> mLocalBufferListAuxR;
    List<new_mem_t *> mLocalBufferListToFaceAddr;
    List<new_mem_t *> mResultBufferListMain;
    List<new_mem_t *> mResultBufferListAuxL;
    List<new_mem_t *> mResultBufferListAuxR;
    new_mem_t mLocalBufferMain[THREE_D_FACEID_BUFFER_SUM];
    new_mem_t mLocalBufferAuxL[THREE_D_FACEID_BUFFER_SUM];
    new_mem_t mLocalBufferAuxR[THREE_D_FACEID_BUFFER_SUM];
    new_mem_t mOtpLocalBuffer;
    new_mem_t mToFaceIDServiceBuffer[THREE_D_FACEID_BUFFER_SUM]; //this buffer is used to store the phy addrs of RGB\IR\Otp\Faceinfo data.

    camera3_stream_t mMainStreams[THREE_D_FACEID_MAX_STREAMS];
    camera3_stream_t mAuxStreamsL[THREE_D_FACEID_MAX_STREAMS - 1];
    camera3_stream_t mAuxStreamsR[THREE_D_FACEID_MAX_STREAMS - 1];
    OtpData mOtpData;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    void processCaptureResultAuxL(camera3_capture_result_t *result);
    void processCaptureResultAuxR(camera3_capture_result_t *result);
    void CallBackResult(uint32_t frame_number,
                        camera3_buffer_status_t buffer_status);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);
    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(int face_camera_id, struct camera_info *info);
    void freeLocalCapBuffer();
    void saveRequest(camera3_capture_request_t *request);
    void raw10ToRaw8(void *dest, void *src, int size);
    void processRawData(new_mem_t *buffer, buffer_type_3D type, uint32_t frame_number);
    void processYUVData(new_mem_t *buffer, uint32_t frame_number);
    bool NV21Rotate90AndMirror(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                      uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                      uint32_t a_udFileSize);
    bool Raw8Rotate90AndMirror(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                    uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                    uint8_t angle);

};
};

#endif /* SPRDCAMERA_IR_FACEID*/
