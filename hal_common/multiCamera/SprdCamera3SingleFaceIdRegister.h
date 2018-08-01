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
#ifndef SPRDCAMERA3SINGLEFACEIDREGISTER_H_HEADER
#define SPRDCAMERA3SINGLEFACEIDREGISTER_H_HEADER

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
#include <cutils/ashmem.h>

#if defined(CONFIG_SPRD_ANDROID_8)
#include <ui/GraphicBufferMapper.h>
#endif

namespace sprdcamera {

#define SINGLE_FACEID_REGISTER_MAX_STREAMS 2
#define SINGLE_FACEID_REGISTER_OUTPUT_BUFFERS 2
#define SINGLE_FACEID_REGISTER_BUFFER_SUM 5

typedef enum {
    SINGLE_FACEID_REGISTER_FILE_NV12 = 1,
    SINGLE_FACEID_REGISTER_FILE_YUV422,
    SINGLE_FACEID_REGISTER_FILE_MAX
} SINGLE_FACEID_REGISTER_File_TYPE;

typedef struct {
    cmr_u32 frame_number;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *input_buffer;
} single_faceid_register_saved_request_t;

typedef struct {
    const native_handle_t *native_handle;
    sp<GraphicBuffer> graphicBuffer;
    cmr_uint phy_addr;
    size_t buf_size;
} single_faceid_register_alloc_mem_t;

class SprdCamera3SingleFaceIdRegister : SprdCamera3MultiBase {
  public:
    static void getCameraFaceId(SprdCamera3SingleFaceIdRegister **pFaceid);
    static cmr_s32 camera_device_open(__unused const struct hw_module_t *module,
                                      const char *id,
                                      struct hw_device_t **hw_device);
    static cmr_s32 close_camera_device(struct hw_device_t *device);
    static cmr_s32 get_camera_info(cmr_s32 camera_id, struct camera_info *info);
    static cmr_s32 initialize(const struct camera3_device *device,
                              const camera3_callback_ops_t *ops);
    static cmr_s32
    configure_streams(const struct camera3_device *device,
                      camera3_stream_configuration_t *stream_list);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *,
                                       cmr_s32 type);
    static cmr_s32 process_capture_request(const struct camera3_device *device,
                                           camera3_capture_request_t *request);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static void dump(const struct camera3_device *device, cmr_s32 fd);
    static cmr_s32 flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static camera3_callback_ops callback_ops_aux;

    SprdCamera3SingleFaceIdRegister();
    virtual ~SprdCamera3SingleFaceIdRegister();

  private:
    const camera3_callback_ops_t *mCallbackOps;
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    camera_metadata_t *mStaticMetadata;
    cmr_s32 mPhyCameraNum;
    cmr_s32 mPreviewWidth;
    cmr_s32 mPreviewHeight;
    cmr_s64 mRegisterPhyaddr;
    bool mFlushing;
    Mutex mLock;
    Mutex mNotifyLockMain;
    Mutex mRequestLock;
    Mutex mResultLock;
    List<single_faceid_register_saved_request_t> mSavedRequestList;
    List<single_faceid_register_alloc_mem_t> mPhyAddrBufferList;
    List<buffer_handle_t *> mCreateBufferList;
    single_faceid_register_alloc_mem_t
        mLocalBuffer[SINGLE_FACEID_REGISTER_BUFFER_SUM];
    camera3_stream_t *mSavedReqStreams[SINGLE_FACEID_REGISTER_MAX_STREAMS];
    camera3_stream_t mRegisterStreams[SINGLE_FACEID_REGISTER_MAX_STREAMS];

    cmr_s32 initialize(const camera3_callback_ops_t *callback_ops);
    cmr_s32 configureStreams(const struct camera3_device *device,
                             camera3_stream_configuration_t *stream_list);
    cmr_s32 processCaptureRequest(const struct camera3_device *device,
                                  camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    void CallBackResult(cmr_u32 frame_number,
                        camera3_buffer_status_t buffer_status);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    cmr_s32 type);
    void _dump(const struct camera3_device *device, cmr_s32 fd);
    cmr_s32 _flush(const struct camera3_device *device);
    cmr_s32 closeCameraDevice();
    cmr_s32 cameraDeviceOpen(cmr_s32 camera_id, struct hw_device_t **hw_device);
    cmr_s32 setupPhysicalCameras();
    cmr_s32 getCameraInfo(cmr_s32 face_camera_id, struct camera_info *info);
    void freeLocalCapBuffer();
    cmr_s32 allocateBuffer(cmr_s32 w, cmr_s32 h, cmr_u32 is_cache,
                           cmr_s32 format,
                           single_faceid_register_alloc_mem_t *new_mem);
    void saveRequest(camera3_capture_request_t *request);
};
};

#endif /* SPRDCAMERAMU*/
