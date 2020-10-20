/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#ifndef __SPRDCAMERA3HWI_H__
#define __SPRDCAMERA3HWI_H__

#include <utils/Mutex.h>
#include <utils/List.h>
#include <utils/KeyedVector.h>
#include <hardware/camera3.h>
#include <CameraMetadata.h>
#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Channel.h"
#include "SprdCamera3OEMIf.h"
#include "SprdCamera3Setting.h"

using namespace android;

namespace sprdcamera {

class SprdCamera3MetadataChannel;
class SprdCamera3HeapMemory;
class SprdCamera3OEMIf;
class SprdCamera3Setting;

#define MIN_MULTI_CAMERA_FAKE_ID 6
#define MAX_MULTI_CAMERA_FAKE_ID 50

class SprdCamera3HWI {
  public:
    /* static variable and functions accessed by camera service */
    static camera3_device_ops_t mCameraOps;
    static int initialize(const struct camera3_device *,
                          const camera3_callback_ops_t *callback_ops);
    static int configure_streams(const struct camera3_device *,
                                 camera3_stream_configuration_t *stream_list);
    static int
    register_stream_buffers(const struct camera3_device *,
                            const camera3_stream_buffer_set_t *buffer_set);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *,
                                       camera3_capture_request_t *request);
    static void get_metadata_vendor_tag_ops(const struct camera3_device *,
                                            vendor_tag_query_ops_t *ops);
    static void dump(const struct camera3_device *, int fd);
    static int flush(const struct camera3_device *);
    static int close_camera_device(struct hw_device_t *device);
    SprdCamera3HWI(int cameraId);
    virtual ~SprdCamera3HWI();
    int openCamera(struct hw_device_t **hw_device);
    static int getNumberOfCameras();
    camera_metadata_t *constructDefaultMetadata(int type);
    static void captureResultCb(cam_result_data_info_t *result_info,
                                void *userdata);
    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(camera3_stream_configuration_t *stream_list);
    int registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set);
    int processCaptureRequest(camera3_capture_request_t *request);
    void getMetadataVendorTagOps(vendor_tag_query_ops_t *ops);
    void dump(int fd);
    int flush();
    void captureResultCb(cam_result_data_info_t *result_info);
    uint64_t getZslBufferTimestamp();
    void setVideoBufferTimestamp(uint64_t timestamp);
    uint64_t getVideoBufferTimestamp(void);
    void setMultiCallBackYuvMode(bool mode);
    void GetFocusPoint(cmr_s32 *point_x, cmr_s32 *point_y);
    cmr_s32 ispSwCheckBuf(cmr_uint *param_ptr);
    void getRawFrame(int64_t timestamp, cmr_u8 **y_addr);
    void stopPreview();
    void startPreview();
    SprdCamera3RegularChannel *getRegularChan();
    SprdCamera3PicChannel *getPicChan();
    SprdCamera3OEMIf *getOEMif();
    void setMultiCameraMode(multiCameraMode Mode);
    void setMasterId(uint8_t masterId);
    void setRefCameraId(uint32_t camera_id);
    void setAeLockUnLock();
    void setVisibleRegion(uint32_t serial, int32_t region[4]);
    void setGlobalZoomRatio(float ratio);
    void setCapState(bool flag);
    static bool isMultiCameraMode(int Mode);
    void setSprdCameraLowpower(int flag);
    int camera_ioctrl(int cmd, void *param1, void *param2);
    int setSensorStream(uint32_t on_off);
    int setCameraClearQBuff();
    void getDualOtpData(void **addr, int *size, int *read);
    void getOnlineBuffer(void *cali_info);
    void setUltraWideMode(unsigned int on_off);
    void setFovFusionMode(unsigned int on_off);
    void setMultiCameraId(uint32_t multi_camera_id);
    void setMultiCaptureTimeStamp(uint64_t time_stamp);
    void pushDualVideoBuffer(hal_mem_info_t *mem_info);
    void setRealMultiMode(bool mode);
    void setMultiAppRatio(float app_ratio);
    static void dumpMemoryAddresses(size_t limit);

  private:
    int openCamera();
    int closeCamera();
    int validateCaptureRequest(camera3_capture_request_t *request);
    void handleCbDataWithLock(cam_result_data_info_t *result_info);
    int checkStreamList(camera3_stream_configuration_t *streamList);
    int32_t checkStreamSizeAndFormat(camera3_stream_t *new_stream);
    int32_t getStreamType(camera3_stream_t *stream);
    void flushRequest(uint32_t frame_num);
    void getLogLevel();
    int resetVariablesToDefault();
    void getHighResZslSetting(void);
    void checkHighResZslSetting(uint32_t *ambient_highlight);
    uint8_t getReqCapureIntent(uint8_t capIntent);
  public:
    SprdCamera3Setting *mSetting;
    uint32_t mFrameNum;
    uint32_t pre_frame_num;

  private:
    typedef struct {
        uint8_t af_trigger;
        uint8_t af_state;
        uint8_t ae_precap_trigger;
        uint8_t ae_state;
        uint8_t ae_manual_trigger;
    } threeA_info_t;

    /* Data structure to store pending request */
    typedef struct {
        uint32_t frame_number;
        uint32_t num_buffers;
        int32_t request_id;
        List<RequestedBufferInfo> buffers;
        nsecs_t timestamp;
        uint8_t bNotified;
        // int input_buffer_present;
        meta_info_t meta_info;
        camera3_stream_buffer_t *input_buffer;
        int32_t receive_req_max;
        uint32_t pipeline_depth;
        threeA_info_t threeA_info;
    } PendingRequestInfo;

    typedef struct {
        stream_status_t status;
        uint32_t width;
        uint32_t height;
        int format;
        camera_stream_type_t type;
        camera3_stream_t *stream;
    } stream_info_t;

    typedef struct {
        uint32_t num_streams;
        stream_info_t preview;
        stream_info_t video;
        stream_info_t yuvcallback;
        stream_info_t yuv2;
        stream_info_t snapshot;
        stream_info_t raw;
    } cam3_stream_configuration_t;

    //for malloc_debug
    typedef struct {
      // Pointer to the buffer allocated by a call to M_GET_MALLOC_LEAK_INFO.
      uint8_t* buffer;
      // The size of the "info" buffer.
      size_t overall_size;
      // The size of a single entry.
      size_t info_size;
      // The sum of all allocations that have been tracked. Does not include
      // any heap overhead.
      size_t total_memory;
      // The maximum number of backtrace entries.
      size_t backtrace_size;
    } android_mallopt_leak_info_t;

    // Opcodes for android_mallopt.

    enum {
      // Marks the calling process as a profileable zygote child, possibly
      // initializing profiling infrastructure.
      M_INIT_ZYGOTE_CHILD_PROFILING = 1,
#define M_INIT_ZYGOTE_CHILD_PROFILING M_INIT_ZYGOTE_CHILD_PROFILING
      M_RESET_HOOKS = 2,
#define M_RESET_HOOKS M_RESET_HOOKS
      // Set an upper bound on the total size in bytes of all allocations made
      // using the memory allocation APIs.
      //   arg = size_t*
      //   arg_size = sizeof(size_t)
      M_SET_ALLOCATION_LIMIT_BYTES = 3,
#define M_SET_ALLOCATION_LIMIT_BYTES M_SET_ALLOCATION_LIMIT_BYTES
      // Called after the zygote forks to indicate this is a child.
      M_SET_ZYGOTE_CHILD = 4,
#define M_SET_ZYGOTE_CHILD M_SET_ZYGOTE_CHILD

      // Options to dump backtraces of allocations. These options only
      // work when malloc debug has been enabled.

      // Writes the backtrace information of all current allocations to a file.
      // NOTE: arg_size has to be sizeof(FILE*) because FILE is an opaque type.
      //   arg = FILE*
      //   arg_size = sizeof(FILE*)
      M_WRITE_MALLOC_LEAK_INFO_TO_FILE = 5,
#define M_WRITE_MALLOC_LEAK_INFO_TO_FILE M_WRITE_MALLOC_LEAK_INFO_TO_FILE
      // Get information about the backtraces of all
      //   arg = android_mallopt_leak_info_t*
      //   arg_size = sizeof(android_mallopt_leak_info_t)
      M_GET_MALLOC_LEAK_INFO = 6,
#define M_GET_MALLOC_LEAK_INFO M_GET_MALLOC_LEAK_INFO
      // Free the memory allocated and returned by M_GET_MALLOC_LEAK_INFO.
      //   arg = android_mallopt_leak_info_t*
      //   arg_size = sizeof(android_mallopt_leak_info_t)
      M_FREE_MALLOC_LEAK_INFO = 7,
#define M_FREE_MALLOC_LEAK_INFO M_FREE_MALLOC_LEAK_INFO
      // Change the heap tagging state. The program must be single threaded at the point when the
      // android_mallopt function is called.
      //   arg = HeapTaggingLevel*
      //   arg_size = sizeof(HeapTaggingLevel)
      M_SET_HEAP_TAGGING_LEVEL = 8,
#define M_SET_HEAP_TAGGING_LEVEL M_SET_HEAP_TAGGING_LEVEL
      // Query whether the current process is considered to be profileable by the
      // Android platform. Result is assigned to the arg pointer's destination.
      //   arg = bool*
      //   arg_size = sizeof(bool)
      M_GET_PROCESS_PROFILEABLE = 9,
#define M_GET_PROCESS_PROFILEABLE M_GET_PROCESS_PROFILEABLE
      // Maybe enable GWP-ASan. Set *arg to force GWP-ASan to be turned on,
      // otherwise this mallopt() will internally decide whether to sample the
      // process. The program must be single threaded at the point when the
      // android_mallopt function is called.
      //   arg = bool*
      //   arg_size = sizeof(bool)
      M_INITIALIZE_GWP_ASAN = 10,
#define M_INITIALIZE_GWP_ASAN M_INITIALIZE_GWP_ASAN
    };

    int timer_stop();

    camera3_device_t mCameraDevice;
    uint8_t mCameraId;
    static multiCameraMode mMultiCameraMode;
    uint8_t mMasterId;
    SprdCamera3OEMIf *mOEMIf;
    bool mCameraOpened;
    bool mCameraInitialized;
    uint32_t mLastFrmNum;
    const camera3_callback_ops_t *mCallbackOps;
    camera3_stream_t *mInputStream;
    SprdCamera3MetadataChannel *mMetadataChannel;
    SprdCamera3PicChannel *mPictureChannel;
    // First request yet to be processed after configureStreams

    List<PendingRequestInfo> mPendingRequestsList;
    List<PendingRequestInfo> mSkipFrmRequestsBakList;
    int mPendingRequest;
    int mDeqBufNum;
    int mRecSkipNum; /*coming bigger frame number*/
    int32_t mCurrentRequestId;
    uint8_t mCurrentCapIntent;

    // mutex for serialized access to camera3_device_ops_t functions
    Mutex mLock;
    Mutex mRequestLock;
    Mutex mResultLock;
    Condition mRequestSignal;
    bool mIsSkipFrm;

    static unsigned int mCameraSessionActive;
    static const int64_t kPendingTime = 1000000;       // 1ms
    static const int64_t kPendingTimeOut = 5000000000; // 5s
    bool mFlush;
    bool mBufferStatusError; // change buffer status to
                             // CAMERA3_BUFFER_STATUS_ERROR when in flush

    SprdCamera3RegularChannel *mRegularChan;
    bool mFirstRegularRequest;
    // int32_t		mRegularWaitBuffNum;

    SprdCamera3PicChannel *mPicChan;
    bool mPictureRequest;
    uint8_t mBurstCapCnt;

    uint8_t mOldCapIntent;
    int32_t mOldRequesId;

    Condition mFlushSignal;

    uint8_t mReciveQeqMax;
    uint64_t mCurFrameTimeStamp;
    int mSprdCameraLowpower;
    bool mFirstRequestGet;
    bool mHighResNonzsl; // high res,1:non-zsl,0:zsl
    //1:always zsl,2:non-zsl,other:detect
    uint8_t mHighResFixZsl;

    cam3_stream_configuration_t mStreamConfiguration;
};

}; // namespace sprdcamera

#endif
