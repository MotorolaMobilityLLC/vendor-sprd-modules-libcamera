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
#include "module_wrapper_hal.h"
#include <algorithm>
#include <chrono>
#include <mutex>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>
#include <inttypes.h>
#include <android/hardware/camera/device/1.0/ICameraDevice.h>
#include <android/hardware/camera/device/3.2/ICameraDevice.h>
#include <android/hardware/camera/device/3.3/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceSession.h>
#include <android/hardware/camera/device/3.4/ICameraDeviceCallback.h>
#include <android/hardware/camera/provider/2.4/ICameraProvider.h>
#include <android/hidl/manager/1.0/IServiceManager.h>
#include <binder/MemoryHeapBase.h>
#include <CameraMetadata.h>
#include <CameraParameters.h>
#include <cutils/properties.h>
#include <fmq/MessageQueue.h>
//#include <gui/Surface.h>
//#include <gui/SurfaceComposerClient.h>
//#include <gui/CpuConsumer.h>
//#include <gui/ISurfaceComposer.h>
#include <hardware/gralloc.h>
#include <hardware/gralloc1.h>
#include "hardware/camera3.h"
#include <system/camera.h>
#include <system/camera_metadata.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/DisplayInfo.h>
#include <android/hardware/graphics/mapper/2.0/IMapper.h>
#include <android/hardware/graphics/mapper/2.0/types.h>
#include <system/camera_metadata.h>
#include "../include/SprdCamera3Tags.h"
#include "../include/SprdCamera3.h"
#include "CameraModule.h"
#include "VendorTagDescriptor.h"
#include "CameraMetadata.h"
#include "system/camera_metadata.h"
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include "cmr_msg.h"
#include <pthread.h>
#include <system/thread_defs.h>
#include <utils/List.h>
#include <json2hal.h>
#include <map>
#include "gralloc_public.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#define THIS_MODULE_NAME "hal"
using namespace ::android::hardware::camera::device;
using ::android::GraphicBuffer;
using ::android::sp;
using ::android::wp;
using ::android::hardware::hidl_handle;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
// using ::android::IGraphicBufferProducer;
// using ::android::IGraphicBufferConsumer;
// using ::android::Surface;
using android::status_t;
using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
using ::android::hardware::camera::common::V1_0::CameraDeviceStatus;
using ::android::hardware::camera::common::V1_0::Status;
using ::android::hardware::camera::common::V1_0::TorchMode;
using ::android::hardware::camera::common::V1_0::TorchModeStatus;
using ::android::hardware::camera::common::V1_0::helper::CameraModule;
using ::android::hardware::camera::common::V1_0::helper::CameraParameters;
using ::android::hardware::camera::common::V1_0::helper::Size;
using ::android::hardware::camera::common::V1_0::helper::VendorTagDescriptor;
using ::android::hardware::camera::common::V1_0::helper::
    VendorTagDescriptorCache;
using ::android::hardware::camera::device::V1_0::DataCallbackMsg;
using ::android::hardware::camera::device::V1_0::NotifyCallbackMsg;
using ::android::hardware::camera::device::V3_2::BufferCache;
using ::android::hardware::camera::device::V3_2::BufferStatus;
using ::android::hardware::camera::device::V3_2::CameraMetadata;
using ::android::hardware::camera::device::V3_2::CaptureRequest;
using ::android::hardware::camera::device::V3_2::CaptureResult;
using ::android::hardware::camera::device::V3_2::ErrorCode;
using ::android::hardware::camera::device::V3_2::HalStreamConfiguration;
using ::android::hardware::camera::device::V3_2::ICameraDevice;
using ::android::hardware::camera::device::V3_2::ICameraDeviceCallback;
using ::android::hardware::camera::device::V3_2::ICameraDeviceSession;
using ::android::hardware::camera::device::V3_2::MsgType;
using ::android::hardware::camera::device::V3_2::NotifyMsg;
using ::android::hardware::camera::device::V3_2::RequestTemplate;
using ::android::hardware::camera::device::V3_2::StreamBuffer;
using ::android::hardware::camera::device::V3_2::StreamConfiguration;
using ::android::hardware::camera::device::V3_2::StreamConfigurationMode;
using ::android::hardware::camera::device::V3_2::StreamRotation;
using ::android::hardware::camera::device::V3_2::StreamType;
using ::android::hardware::camera::provider::V2_4::ICameraProvider;
using ::android::hardware::camera::provider::V2_4::ICameraProviderCallback;
using ::android::hardware::graphics::common::V1_0::PixelFormat;

using ResultMetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;
using ::android::hidl::manager::V1_0::IServiceManager;

using namespace ::android::hardware::camera;
// using namespace sprdcamera;

#define V(fmt, args...)                                                        \
    do {                                                                       \
        ALOGV(fmt, ##args);                                                    \
        printf(fmt "\n", ##args);                                              \
    } while (0)
#define D(fmt, args...)                                                        \
    do {                                                                       \
        ALOGD(fmt, ##args);                                                    \
        printf(fmt "\n", ##args);                                              \
    } while (0)
#define E(fmt, args...)                                                        \
    do {                                                                       \
        ALOGE(fmt, ##args);                                                    \
        printf(fmt "\n", ##args);                                              \
    } while (0)
#define UNUSED(x) (void)(x)
#define JPEG_SIZE 3264 * 2448
#ifndef container_of
#define container_of(ptr, type, member)                                        \
    ({                                                                         \
        const __typeof__(((type *)0)->member) *__mptr = (ptr);                 \
        (type *)((char *)__mptr - (char *)(&((type *)0)->member));             \
    })
#endif
#define DATA_ALIGNMENT ((size_t)8)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#endif
#define LOG_TAG "IT-moduleHal"
#define ALIGN_TO(val, alignment)                                               \
    (((uintptr_t)(val) + ((alignment)-1)) & ~((alignment)-1))
struct AvailableStream {
    int32_t width;
    int32_t height;
    int32_t format;
};

camera_metadata_entry updatemeta[5];
int metacount = 0;
typedef struct {
    int fd;
    size_t size;
    // offset from fd, always set to 0
    void *addr_phy;
    void *addr_vir;
    int width;
    int height;
    int format;
    sp<GraphicBuffer> pbuffer;
    void *bufferPtr;
} hal_mem_info_t;

typedef struct {
    const native_handle_t *native_handle;
    sp<GraphicBuffer> graphicBuffer;
    int width;
    int height;
    void *vir_addr;
    void *phy_addr;
    //    camera_buffer_type_t type;
    void *private_handle;
} new_mem_t;

typedef enum {
    CAMERA_OPEN_CAMEAR,
    CAMERA_START_PREVIEW,
    CAMERA_TAKE_PIC,
    CAMERA_SET_AWB,
    CAMERA_CLOSE_CAMERA,
    CAMERA_TEST_MAX,
} test_item;

typedef struct {
    camera_stream_type_t stream_type;
    int width;
    int height;
    android_pixel_format_t pixel;
} streamconfig;

hal_mem_info_t memory_gloable;    // snapshot memory map
static new_mem_t snapshot_buffer; // snapshot memory
static sp<ICameraDeviceSession> g_session;
static int capture_flag = false; // flag to  start capture
static int capture_frame = -10;  // reocording capture frame
static int max_buffers = 4;
static streamconfig *g_streamConfig; // stream config
int g_stream_count = 0;
static bool volatile g_result[CAMERA_TEST_MAX] = {false}; // result struct

new_mem_t mLocalBuffer[20];
new_mem_t mCallbackBuffer[20];
new_mem_t mVideoBuffer[20];
map<string, uint32_t> mMap_tag;
map<string, uint8_t> mMap_tagType;
android::List<new_mem_t *> mLocalBufferList;
android::List<new_mem_t *> mCallbackBufferList;
android::List<new_mem_t *> mVideoBufferList;
uint32_t previewStreamID = -1;
uint32_t snapshotStreamID = -1;
uint32_t callbackStreamID = -1;
uint32_t videoStreamID = -1;

static long find_pid_by_name(char *pidName) {
    DIR *dir;
    struct dirent *next;
    int i = 0;
    long pid = 0;
    dir = opendir("/proc");
    if (!dir) {
        ALOGE("can not open proc");
    }
    while ((next = readdir(dir)) != NULL) {
        FILE *status;
        char filename[50];
        char buffer[50];
        char name[50];

        if (strcmp(next->d_name, "..") == 0)
            continue;

        if (!isdigit(*next->d_name))
            continue;
        sprintf(filename, "/proc/%s/status", next->d_name);
        if (!(status = fopen(filename, "r"))) {
            continue;
        }
        if (fgets(buffer, 50 - 1, status) == NULL) {
            fclose(status);
            continue;
        }
        fclose(status);
        sscanf(buffer, "%*s %s", name);
        if (strcmp(name, pidName) == 0) {
            ALOGE("find pid ,%s", name);
            pid = atoi(next->d_name);
        }
    }
    closedir(dir);
    return pid;
}

static void pushBufferList(new_mem_t *localbuffer, buffer_handle_t *backbuf,
                           int localbuffer_num,
                           android::List<new_mem_t *> &list) {
    int i;

    if (backbuf == NULL) {
        ALOGE("backbuf is NULL");
        return;
    }
    ALOGE("pushBufferList = %p", *backbuf);
    //    Mutex::Autolock l(mBufferListLock);
    for (i = 0; i < localbuffer_num; i++) {
        if (localbuffer[i].native_handle == NULL)
            continue;
        if ((&(localbuffer[i].native_handle)) == backbuf) {
            list.push_back(&(localbuffer[i]));
            break;
        }
    }
    if (i >= localbuffer_num) {
        ALOGE("find backbuf failed");
    }
    return;
}

namespace {
// "device@<version>/legacy/<id>"
const char *kDeviceNameRE = "device@([0-9]+\\.[0-9]+)/%s/(.+)";
const int CAMERA_DEVICE_API_VERSION_3_4_1 = 0x304;
const int CAMERA_DEVICE_API_VERSION_3_3_1 = 0x303;
const int CAMERA_DEVICE_API_VERSION_3_2_1 = 0x302;
const int CAMERA_DEVICE_API_VERSION_1_0_1 = 0x100;
const char *kHAL3_4 = "3.4";
const char *kHAL3_3 = "3.3";
const char *kHAL3_2 = "3.2";
const char *kHAL1_0 = "1.0";

int g_sensor_width = 1440;
int g_sensor_height = 1080;

int allocateOne(int w, int h, new_mem_t *new_mem, int type) {

    sp<GraphicBuffer> graphicBuffer = NULL;
    native_handle_t *native_handle = NULL;
    unsigned long phy_addr = 0;
    size_t buf_size = 0;
    uint32_t yuvTextUsage = GraphicBuffer::USAGE_HW_TEXTURE |
                            GraphicBuffer::USAGE_SW_READ_OFTEN |
                            GraphicBuffer::USAGE_SW_WRITE_OFTEN;

    graphicBuffer = new GraphicBuffer(w, h, HAL_PIXEL_FORMAT_YCrCb_420_SP, 1,
                                      yuvTextUsage, "dualcamera");

    native_handle = (native_handle_t *)graphicBuffer->handle;

    new_mem->phy_addr = (void *)phy_addr;
    new_mem->native_handle = native_handle;
    new_mem->graphicBuffer = graphicBuffer;
    new_mem->width = w;
    new_mem->height = h;

    return 0;
}

bool matchDeviceName(const hidl_string &deviceName,
                     const hidl_string &providerType,
                     std::string *deviceVersion, std::string *cameraId) {
    ::android::String8 pattern;
    pattern.appendFormat(kDeviceNameRE, providerType.c_str());
    std::regex e(pattern.string());
    std::string deviceNameStd(deviceName.c_str());
    std::smatch sm;
    if (std::regex_match(deviceNameStd, sm, e)) {
        if (deviceVersion != nullptr) {
            *deviceVersion = sm[1];
        }
        if (cameraId != nullptr) {
            *cameraId = sm[2];
        }
        return true;
    }
    return false;
}

int getCameraDeviceVersion(const hidl_string &deviceName,
                           const hidl_string &providerType) {
    std::string version;
    bool match = matchDeviceName(deviceName, providerType, &version, nullptr);
    if (!match) {
        return -1;
    }

    if (version.compare(kHAL3_4) == 0) {
        return CAMERA_DEVICE_API_VERSION_3_4_1;
    } else if (version.compare(kHAL3_3) == 0) {
        return CAMERA_DEVICE_API_VERSION_3_3_1;
    } else if (version.compare(kHAL3_2) == 0) {
        return CAMERA_DEVICE_API_VERSION_3_2_1;
    } else if (version.compare(kHAL1_0) == 0) {
        return CAMERA_DEVICE_API_VERSION_1_0_1;
    }
    return 0;
}

bool parseProviderName(const std::string &name, std::string *type /*out*/,
                       uint32_t *id /*out*/) {
    if (!type || !id) {
        ALOGE("%s\uff0c%d", __FUNCTION__, __LINE__);
        return false;
    }

    std::string::size_type slashIdx = name.find('/');
    if (slashIdx == std::string::npos || slashIdx == name.size() - 1) {
        ALOGE("Provider name does not have separator between type and id "
              "%s\uff0c%d",
              __FUNCTION__, __LINE__);
        return false;
    }

    std::string typeVal = name.substr(0, slashIdx);

    char *endPtr;
    errno = 0;
    long idVal = strtol(name.c_str() + slashIdx + 1, &endPtr, 10);
    if (errno != 0) {
        ALOGE("%s\uff0c%d cannot parse provider id as an integer: %s",
              __FUNCTION__, __LINE__, name.c_str());
        return false;
    }
    if (endPtr != name.c_str() + name.size()) {
        ALOGE("%s\uff0c%d provider id has unexpected length: %s", __FUNCTION__,
              __LINE__, name.c_str());
        return false;
    }
    if (idVal < 0) {
        ALOGE("%s\uff0c%d id is negative:: %s,%d", __FUNCTION__, __LINE__,
              name.c_str(), static_cast<uint32_t>(idVal));
        return false;
    }

    *type = typeVal;
    *id = static_cast<uint32_t>(idVal);

    return true;
}
} // namespace

class NativeCameraHidl {
    hidl_vec<hidl_string> getCameraDeviceNames(int g_camera_id,
                                               sp<ICameraProvider> provider);

    struct EmptyDeviceCb : public ICameraDeviceCallback {
        virtual Return<void> processCaptureResult(
            const hidl_vec<CaptureResult> & /*results*/) override {
            ALOGI("processCaptureResult callback");
            return Void();
        }

        virtual Return<void>
        notify(const hidl_vec<NotifyMsg> & /*msgs*/) override {
            ALOGI("notify callback");
            return Void();
        }
    };

    struct DeviceCb : public V3_4::ICameraDeviceCallback {
        DeviceCb(NativeCameraHidl *parent) : mParent(parent) {}
        Return<void> processCaptureResult_3_4(
            const hidl_vec<V3_4::CaptureResult> &results) override;
        Return<void>
        processCaptureResult(const hidl_vec<CaptureResult> &results) override;
        Return<void> notify(const hidl_vec<NotifyMsg> &msgs) override;

        void
        getInflightBufferKeys(std::vector<std::pair<int32_t, int32_t>> *out);
        android::status_t pushInflightBufferLocked(int32_t frameNumber,
                                                   int32_t streamId,
                                                   buffer_handle_t *buffer,
                                                   int acquireFence);
        android::status_t popInflightBuffer(int32_t frameNumber,
                                            int32_t streamId,
                                            /*out*/ buffer_handle_t **buffer);
        void wrapAsHidlRequest(
            camera3_capture_request_t *request,
            /*out*/ device::V3_2::CaptureRequest *captureRequest,
            /*out*/ std::vector<native_handle_t *> *handlesCreated);

      private:
        bool processCaptureResultLocked(const CaptureResult &results);

        NativeCameraHidl *mParent; // Parent object
        std::mutex mInflightLock;

      public:
        int mBufferId;
    };

    struct TorchProviderCb : public ICameraProviderCallback {
        TorchProviderCb(NativeCameraHidl *parent) : mParent(parent) {}
        virtual Return<void>
        cameraDeviceStatusChange(const hidl_string &,
                                 CameraDeviceStatus) override {
            return Void();
        }

        virtual Return<void>
        torchModeStatusChange(const hidl_string &,
                              TorchModeStatus newStatus) override {
            std::lock_guard<std::mutex> l(mParent->mTorchLock);
            mParent->mTorchStatus = newStatus;
            mParent->mTorchCond.notify_one();
            return Void();
        }

      private:
        NativeCameraHidl *mParent; // Parent object
    };

    void
    castSession(const sp<ICameraDeviceSession> &session, int32_t deviceVersion,
                sp<device::V3_3::ICameraDeviceSession> *session3_3 /*out*/,
                sp<device::V3_4::ICameraDeviceSession> *session3_4 /*out*/);
    void createStreamConfiguration(
        const ::android::hardware::hidl_vec<V3_2::Stream> &streams3_2,
        StreamConfigurationMode configMode,
        ::android::hardware::camera::device::V3_2::StreamConfiguration
            *config3_2,
        ::android::hardware::camera::device::V3_4::StreamConfiguration
            *config3_4);

    void
    configureAvailableStream(const std::string &name, int32_t deviceVersion,
                             sp<ICameraProvider> provider,
                             const AvailableStream *previewThreshold,
                             sp<ICameraDeviceSession> *session /*out*/,
                             V3_2::Stream *previewStream /*out*/,
                             HalStreamConfiguration *halStreamConfig /*out*/,
                             bool *supportsPartialResults /*out*/,
                             uint32_t *partialResultCount /*out*/);

    static Status
    getAvailableOutputStreams(camera_metadata_t *staticMeta,
                              std::vector<AvailableStream> &outputStreams,
                              const AvailableStream *threshold = nullptr);

  public:
    int startnativePreview(int g_camera_id, int g_width, int g_height,
                           int g_sensor_width, int g_sensor_height,
                           int g_rotate);
    int StopnativePreview();
    bool need_exit(void);
    bool exit_camera(void);
    int nativecamerainit(int g_rotate, int mirror);
    int openNativeCamera(int g_camera_id);
    int map2(buffer_handle_t *buffer_handle, hal_mem_info_t *mem_info);
    int startpreview();
    int SetupSprdTags();
    int transferMetaData(HalCaseComm *_json2);
    int freeAllBuffers();
    void freeOneBuffer(new_mem_t *buffer);
    bool previewstop = 0;
    std::vector<camera_stream_type_t> streamlist;

  protected:
    // In-flight queue for tracking completion of capture requests.
    struct InFlightRequest {
        // Set by notify() SHUTTER call.
        nsecs_t shutterTimestamp;

        bool errorCodeValid;
        ErrorCode errorCode;

        // Is partial result supported
        bool usePartialResult;

        // Partial result count expected
        uint32_t numPartialResults;

        // Message queue
        std::shared_ptr<ResultMetadataQueue> resultQueue;

        // Set by process_capture_result call with valid metadata
        bool haveResultMetadata;

        // Decremented by calls to process_capture_result with valid output
        // and input buffers
        ssize_t numBuffersLeft;

        // A 64bit integer to index the frame number associated with this
        // result.
        int64_t frameNumber;

        // The partial result count (index) for this capture result.
        int32_t partialResultCount;

        // For buffer drop errors, the stream ID for the stream that lost a
        // buffer.
        // Otherwise -1.
        int32_t errorStreamId;

        // If this request has any input buffer
        bool hasInputBuffer;

        // Result metadata
        ::android::hardware::camera::common::V1_0::helper::CameraMetadata
            collectedResult;

        // Buffers are added by process_capture_result when output buffers
        // return from HAL but framework.
        ::android::Vector<StreamBuffer> resultOutputBuffers;

        InFlightRequest()
            : shutterTimestamp(0), errorCodeValid(false),
              errorCode(ErrorCode::ERROR_BUFFER), usePartialResult(false),
              numPartialResults(0), resultQueue(nullptr),
              haveResultMetadata(false), numBuffersLeft(0), frameNumber(0),
              partialResultCount(0), errorStreamId(-1), hasInputBuffer(false) {}

        InFlightRequest(ssize_t numBuffers, bool hasInput, bool partialResults,
                        uint32_t partialCount,
                        std::shared_ptr<ResultMetadataQueue> queue = nullptr)
            : shutterTimestamp(0), errorCodeValid(false),
              errorCode(ErrorCode::ERROR_BUFFER),
              usePartialResult(partialResults), numPartialResults(partialCount),
              resultQueue(queue), haveResultMetadata(false),
              numBuffersLeft(numBuffers), frameNumber(0), partialResultCount(0),
              errorStreamId(-1), hasInputBuffer(hasInput) {}
    };

    // Map from frame number to the in-flight request state
    typedef ::android::KeyedVector<uint32_t, InFlightRequest *> InFlightMap;

    std::mutex mLock; // Synchronize access to member variables

    std::condition_variable
        mResultCondition;     // Condition variable for incoming results
    InFlightMap mInflightMap; // Map of all inflight requests

    std::mutex mTorchLock;              // Synchronize access to torch status
    std::condition_variable mTorchCond; // Condition variable for torch status
    TorchModeStatus mTorchStatus;       // Current torch status

    // Camera provider service
    sp<ICameraProvider> mProvider;
    pthread_t preview_thread_handle;
    // Camera provider type.
    std::string mProviderType;

    camera_metadata_entry local_meta[5];
    int local_meta_count = 0;
    std::vector<uint8_t> meta_u8;
    std::vector<int32_t> meta_int32;
    uint8_t mode_test;
    buffer_handle_t *popBufferList(android::List<new_mem_t *> &list, int type);
    // void pushBufferList(new_mem_t *localbuffer,
    //                                     buffer_handle_t *backbuf,
    //                                     int localbuffer_num,
    //                                    android::List<new_mem_t *> &list) ;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata
    updatemetedata(camera_metadata_t *metaBuffer);
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata
    updatemetedata_v2(camera_metadata_t *metaBuffer,
                      camera_metadata_entry *meta_entry, int meta_count);
    int g_surface_f = HAL_PIXEL_FORMAT_YCrCb_420_SP;

    //    sp<ANativeWindow> nativeWindow = NULL;
};

static bool volatile g_need_exit = false;
static NativeCameraHidl native_camera;
bool g_first_open = false;
bool g_first_preview = false;
void IntSignalHandler(int signum, siginfo_t *, void *sigcontext) {
    UNUSED(sigcontext);

    if (signum == SIGINT) {
        ALOGI("\nCTRL^C pressed");
        g_need_exit = true;

    } else if (signum == SIGTERM) {
        ALOGI("\n get Kill command");
        g_need_exit = true;
    }
}
bool NativeCameraHidl::need_exit(void) {
    // todo:  check kernel if need exit camera
    return false;
}
bool NativeCameraHidl::exit_camera(void) { return g_need_exit || need_exit(); }

int NativeCameraHidl::nativecamerainit(int g_rotate, int mirror) {
    /* if (g_rotate == 90) {
         g_rotate  = NATIVE_WINDOW_TRANSFORM_ROT_90;
     } else if (g_rotate == 180) {
         g_rotate  = NATIVE_WINDOW_TRANSFORM_ROT_180;
     } else if (g_rotate == 270) {
         g_rotate  = NATIVE_WINDOW_TRANSFORM_ROT_270;
     } else {
         g_rotate  = 0;
     }*/
    g_rotate = 0;
    if (mirror == 1) {
        //        g_rotate |= NATIVE_WINDOW_TRANSFORM_FLIP_H;
    } else {
        // Nothing to do;
    }
    struct sigaction act, oldact;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = ::IntSignalHandler;
    act.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGINT, &act, &oldact) != 0) {
        ALOGI("install signal error");
        return -1;
    }

    if (sigaction(SIGTERM, &act, &oldact) != 0) {
        ALOGI("install signal SIGTERM error");
        return -1;
    }
    return 0;
}

Return<void> NativeCameraHidl::DeviceCb::processCaptureResult_3_4(
    const hidl_vec<V3_4::CaptureResult> &results) {
    if (nullptr == mParent) {
        return Void();
    }

    bool notify = false;
    std::unique_lock<std::mutex> l(mParent->mLock);
    for (size_t i = 0; i < results.size(); i++) {
        notify = processCaptureResultLocked(results[i].v3_2);
    }

    l.unlock();
    if (notify) {
        mParent->mResultCondition.notify_one();
    }

    return Void();
}

Return<void> NativeCameraHidl::DeviceCb::processCaptureResult(
    const hidl_vec<CaptureResult> &results) {
    // ALOGI("debug_zy begin %s ,%d,frame_number=%d partialResult=%d,%p",
    // __FUNCTION__,__LINE__,results[0].frameNumber,
    // results[0].partialResult,&(results[0].outputBuffers[0].buffer));

    if (nullptr == mParent) {
        return Void();
    }

    bool notify = false;
    std::unique_lock<std::mutex> l(mParent->mLock);

    // ALOGI("debug_zy %s ,%d,results.size()=%zu",
    // __FUNCTION__,__LINE__,results.size());
    for (size_t i = 0; i < results.size(); i++) {
        notify = processCaptureResultLocked(results[i]);
    }

    l.unlock();
    if (notify) {
        mParent->mResultCondition.notify_one();
    }

    return Void();
}
std::unordered_map<uint64_t, std::pair<buffer_handle_t *, int>>
    mInflightBufferMap;

void NativeCameraHidl::DeviceCb::getInflightBufferKeys(
    std::vector<std::pair<int32_t, int32_t>> *out) {
    std::unique_lock<std::mutex> l(mParent->mLock);
    out->clear();
    out->reserve(mInflightBufferMap.size());
    for (auto &pair : mInflightBufferMap) {
        uint64_t key = pair.first;
        int32_t streamId = key & 0xFFFFFFFF;
        int32_t frameNumber = (key >> 32) & 0xFFFFFFFF;
        out->push_back(std::make_pair(frameNumber, streamId));
    }
    l.unlock();
    return;
}

android::status_t NativeCameraHidl::DeviceCb::pushInflightBufferLocked(
    int32_t frameNumber, int32_t streamId, buffer_handle_t *buffer,
    int acquireFence) {
    uint64_t key = static_cast<uint64_t>(frameNumber) << 32 |
                   static_cast<uint64_t>(streamId);
    auto pair = std::make_pair(buffer, acquireFence);
    mInflightBufferMap[key] = pair;
    return android::OK;
}

android::status_t NativeCameraHidl::DeviceCb::popInflightBuffer(
    int32_t frameNumber, int32_t streamId,
    /*out*/ buffer_handle_t **buffer) {
    std::lock_guard<std::mutex> lock(mInflightLock);

    uint64_t key = static_cast<uint64_t>(frameNumber) << 32 |
                   static_cast<uint64_t>(streamId);

    // ALOGI("debug_zy_map:%s\uff0c%d", __FUNCTION__,__LINE__);
    auto it = mInflightBufferMap.find(key);
    if (it == mInflightBufferMap.end()) {
        return android::NAME_NOT_FOUND;
    }
    auto pair = it->second;
    *buffer = pair.first;
    int acquireFence = pair.second;
    if (acquireFence > 0) {
        ::close(acquireFence);
    }
    mInflightBufferMap.erase(it);

    return android::OK;
}
void NativeCameraHidl::DeviceCb::wrapAsHidlRequest(
    camera3_capture_request_t *request,
    /*out*/ device::V3_2::CaptureRequest *captureRequest,
    /*out*/ std::vector<native_handle_t *> *handlesCreated) {

    ALOGV("%s: captureRequest (%p) and handlesCreated (%p) ", __FUNCTION__,
          captureRequest, handlesCreated);

    if (captureRequest == nullptr || handlesCreated == nullptr) {
        ALOGE(
            "%s: captureRequest (%p) and handlesCreated (%p) must not be null",
            __FUNCTION__, captureRequest, handlesCreated);
        return;
    }

    captureRequest->frameNumber = request->frame_number;
    // ALOGI("debug_zy :%s\uff0c%d,request->frame_number:%d",
    // __FUNCTION__,__LINE__,request->frame_number);

    captureRequest->fmqSettingsSize = 0;

    {
        std::lock_guard<std::mutex> lock(mInflightLock);

        captureRequest->inputBuffer.streamId = -1;
        captureRequest->inputBuffer.bufferId = 0;

        // ALOGI("debug_zy :%s\uff0c%d,request->num_output_buffers:%d",
        // __FUNCTION__,__LINE__,request->num_output_buffers);

        captureRequest->outputBuffers.resize(request->num_output_buffers);
        int32_t streamId = 0;
        ALOGE("num_out_buffers =%d", request->num_output_buffers);
        for (size_t i = 0; i < request->num_output_buffers; i++) {
            const camera3_stream_buffer_t *src = request->output_buffers + i;
            StreamBuffer &dst = captureRequest->outputBuffers[i];

            buffer_handle_t buf = *(src->buffer);
            // auto pair = getBufferId(buf, streamId);
            bool isNewBuffer = 1; // pair.first;

            dst.streamId = streamId;

            ALOGE("dst.streamId =%d i =%d", dst.streamId, i);
            if (mBufferId == 6) {
                mBufferId = 1;
            }

            // D("mBufferId:%d",mBufferId); //
            dst.bufferId = mBufferId++; // 1;//pair.second;1 2 3 4 5
            dst.buffer = isNewBuffer ? buf : nullptr;

            // D("
            // buffer_handle:%d",(*request->output_buffers->buffer)->version);
            // //12
            //	ALOGI("buffer_handle:%d",captureRequest->outputBuffers[0].buffer->version);

            dst.status = BufferStatus::OK;
            native_handle_t *acquireFence = nullptr;
            if (src->acquire_fence != -1) {
                acquireFence = native_handle_create(1, 0);
                acquireFence->data[0] = src->acquire_fence;
                handlesCreated->push_back(acquireFence);
            }
            dst.acquireFence = acquireFence;
            dst.releaseFence = nullptr;

            pushInflightBufferLocked(captureRequest->frameNumber, streamId,
                                     src->buffer, src->acquire_fence);
            streamId++;
        }
    }
}

bool NativeCameraHidl::DeviceCb::processCaptureResultLocked(
    const CaptureResult &results) {
    bool notify = false;
    uint32_t frameNumber = results.frameNumber;
    camera3_capture_result r;
    r.frame_number = results.frameNumber;
    android::status_t res;

    if ((results.result.size() == 0) && (results.outputBuffers.size() == 0) &&
        (results.inputBuffer.buffer == nullptr) &&
        (results.fmqResultSize == 0)) {
        ALOGE(
            "%s: No result data provided by HAL for frame %d result count: %d",
            __FUNCTION__, frameNumber, (int)results.fmqResultSize);
        return notify;
    }

    ssize_t idx = mParent->mInflightMap.indexOfKey(frameNumber);

    ALOGI("debug_zy %s ,%d,idx:%zu,frameNumber:%d", __FUNCTION__, __LINE__, idx,
          frameNumber);
    if (::android::NAME_NOT_FOUND == idx) {
        ALOGE("%s: Unexpected frame number! received: %u", __FUNCTION__,
              frameNumber);
        return notify;
    }

    bool isPartialResult = false;
    bool hasInputBufferInRequest = false;
    InFlightRequest *request = mParent->mInflightMap.editValueAt(idx);
    ::android::hardware::camera::device::V3_2::CameraMetadata resultMetadata;
    size_t resultSize = 0;
    if (results.fmqResultSize > 0) {
        resultMetadata.resize(results.fmqResultSize);
        if (request->resultQueue == nullptr) {
            return notify;
        }
        if (!request->resultQueue->read(resultMetadata.data(),
                                        results.fmqResultSize)) {
            ALOGE("%s: Frame %d: Cannot read camera metadata from fmq,"
                  "size = %" PRIu64,
                  __FUNCTION__, frameNumber, results.fmqResultSize);
            return notify;
        }
        resultSize = resultMetadata.size();
    } else if (results.result.size() > 0) {
        resultMetadata.setToExternal(
            const_cast<uint8_t *>(results.result.data()),
            results.result.size());
        resultSize = resultMetadata.size();
    }

    if (!request->usePartialResult && (resultSize > 0) &&
        (results.partialResult != 1)) {
        ALOGE("%s: Result is malformed for frame %d: partial_result %u "
              "must be 1  if partial result is not supported",
              __FUNCTION__, frameNumber, results.partialResult);
        return notify;
    }

    if (results.partialResult != 0) {
        request->partialResultCount = results.partialResult;
    }

    // Check if this result carries only partial metadata
    if (request->usePartialResult && (resultSize > 0)) {
        if ((results.partialResult > request->numPartialResults) ||
            (results.partialResult < 1)) {
            ALOGE("%s: Result is malformed for frame %d: partial_result %u"
                  " must be  in the range of [1, %d] when metadata is "
                  "included in the result",
                  __FUNCTION__, frameNumber, results.partialResult,
                  request->numPartialResults);
            return notify;
        }
        request->collectedResult.append(
            reinterpret_cast<const camera_metadata_t *>(resultMetadata.data()));

        isPartialResult = (results.partialResult < request->numPartialResults);
    } else if (resultSize > 0) {
        request->collectedResult.append(
            reinterpret_cast<const camera_metadata_t *>(resultMetadata.data()));
        isPartialResult = false;
    }

    hasInputBufferInRequest = request->hasInputBuffer;
    // ALOGI("debug_zy  %s
    // ,%d,resultSize:%zu,isPartialResult:%d,haveResultMetadata:%d",
    // __FUNCTION__,__LINE__,resultSize,isPartialResult,request->haveResultMetadata);
    // Did we get the (final) result metadata for this capture?
    if ((resultSize > 0) && !isPartialResult) {
        if (request->haveResultMetadata) {
            ALOGE("%s: Called multiple times with metadata for frame %d",
                  __FUNCTION__, frameNumber);
            // return notify;
        }
        request->haveResultMetadata = true;
        request->collectedResult.sort();
    }

    uint32_t numBuffersReturned = results.outputBuffers.size();

    std::vector<camera3_stream_buffer_t> outputBuffers(
        results.outputBuffers.size());
    std::vector<buffer_handle_t> outputBufferHandles(
        results.outputBuffers.size());
    ALOGI("results.outputBuffers.size() =%d", results.outputBuffers.size());
    for (size_t i = 0; i < results.outputBuffers.size(); i++) {
        auto &bDst = outputBuffers[i];
        const StreamBuffer &bSrc = results.outputBuffers[i];
        buffer_handle_t *buffer;
        res = popInflightBuffer(results.frameNumber, bSrc.streamId, &buffer);
        if (res != android::OK) {
            ALOGE("%s: Frame %d: Buffer %zu: No in-flight buffer for stream %d",
                  __FUNCTION__, results.frameNumber, i, bSrc.streamId);
            // return;
        }
        bDst.buffer = buffer;

        if (results.partialResult == 0 && (results.frameNumber == 1)) {
            g_result[CAMERA_START_PREVIEW] = true;
            ALOGI("receive first preview frame,preview sucess");
        }

// D("queueBuffer");
// ALOGI("debug_zy :%s\uff0c%d,version:%d",
// __FUNCTION__,__LINE__,(results.outputBuffers[0].buffer)->version);
// ALOGI("debug_zy :%s\uff0c%d,version:%d,partialResult:%d",
// __FUNCTION__,__LINE__,(*outputBuffers[0].buffer)->version,results.partialResult);
#if 1 // ok
      // if (results.partialResult == 0 && (results.frameNumber ==
      // capture_frame )) {
        if (results.partialResult == 0 &&
            (results.frameNumber == capture_frame)) {
            android::GraphicBufferMapper &mapper =
                android::GraphicBufferMapper::get();
            void *dst;
            // mapper.lock(*outputBuffers[0].buffer,
            // GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_OFTEN,
            // android::Rect(1440, 1080), &dst);
            mapper.lock(
                *outputBuffers[0].buffer,
                GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_SW_READ_OFTEN,
                android::Rect(g_streamConfig[snapshotStreamID].width *
                                  g_streamConfig[snapshotStreamID].height,
                              1),
                &dst);
            {
                ALOGE("address = %p ", dst);
                if (dst == memory_gloable.addr_vir) // write pic frame
                // if(dst != NULL)
                {
                    ALOGE("address = %p ", dst);
                    char filename[128];
                    sprintf(filename, "/data/vendor/ylog/%d_%dx%d.nv",
                            results.frameNumber, g_sensor_width,
                            g_sensor_height);
                    FILE *fp = fopen(filename, "w+");
                    if (fp) {
                        // int ret =  fwrite(dst, 1, 1440 * 1080 * 3 / 2, fp);
                        int ret =
                            fwrite(dst, 1,
                                   g_streamConfig[snapshotStreamID].width *
                                       g_streamConfig[snapshotStreamID].height,
                                   fp);
                        ALOGE("write ret = %d", ret);
                        fclose(fp);
                    } else {
                        ALOGE("fail to open file");
                    }
                    g_result[CAMERA_TAKE_PIC] = true;
                }
            }

            mapper.unlock(*outputBuffers[0].buffer);
        }
#endif
        //	D("debug_zy_exit:%s\uff0c%d,%p",
        //__FUNCTION__,__LINE__,outputBuffers[0].buffer);
        pushBufferList(mLocalBuffer, outputBuffers[0].buffer, 20,
                       mLocalBufferList);
        pushBufferList(mCallbackBuffer, outputBuffers[0].buffer, 20,
                       mCallbackBufferList);
        pushBufferList(mVideoBuffer, outputBuffers[0].buffer, 20,
                       mVideoBufferList);
        //  android::status_t res =
        //  mParent->nativeWindow->queueBuffer(mParent->nativeWindow.get(),
        //                                                            container_of(outputBuffers[0].buffer,
        //                                                            ANativeWindowBuffer,
        //                                                            handle),
        //                                                            -1);
        if (res != android::OK) {
            E("queueBuffer error=%d", res);
        }
    }
    r.num_output_buffers = outputBuffers.size();
    r.output_buffers = outputBuffers.data();

    if (results.inputBuffer.buffer != nullptr) {
        if (hasInputBufferInRequest) {
            numBuffersReturned += 1;
        } else {
            ALOGW("%s: Input buffer should be NULL if there is no input"
                  " buffer sent in the request",
                  __FUNCTION__);
        }
    }

    // ALOGI("debug_zy  %s ,%d,numBuffersLeft:%zu,numBuffersReturned:%d",
    // __FUNCTION__,__LINE__,request->numBuffersLeft,numBuffersReturned);
    request->numBuffersLeft -= numBuffersReturned;
    if (request->numBuffersLeft < 0) {
        ALOGE("%s: Too many buffers returned for frame %d", __FUNCTION__,
              frameNumber);
        // return notify;
    }
    request->resultOutputBuffers.appendArray(results.outputBuffers.data(),
                                             results.outputBuffers.size());
    // If shutter event is received notify the pending threads.
    if (request->shutterTimestamp != 0) {
        notify = true;
    }

    return notify;
}

Return<void>
NativeCameraHidl::DeviceCb::notify(const hidl_vec<NotifyMsg> &messages) {
    std::lock_guard<std::mutex> l(mParent->mLock);

    for (size_t i = 0; i < messages.size(); i++) {
        ssize_t idx = mParent->mInflightMap.indexOfKey(
            messages[i].msg.shutter.frameNumber);

        // ALOGI("debug_zy %s ,%d,idx:%zu,frameNumber:%d",
        // __FUNCTION__,__LINE__,idx,messages[i].msg.shutter.frameNumber);

        if (::android::NAME_NOT_FOUND == idx) {
            ALOGE("%s: Unexpected frame number! received: %u", __FUNCTION__,
                  messages[i].msg.shutter.frameNumber);
            break;
        }
        InFlightRequest *r = mParent->mInflightMap.editValueAt(idx);

        switch (messages[i].type) {
        case MsgType::ERROR:
            if (ErrorCode::ERROR_DEVICE == messages[i].msg.error.errorCode) {
                ALOGE("%s: Camera reported serious device error", __FUNCTION__);
            } else {
                r->errorCodeValid = true;
                r->errorCode = messages[i].msg.error.errorCode;
                r->errorStreamId = messages[i].msg.error.errorStreamId;
            }
            break;
        case MsgType::SHUTTER:
            r->shutterTimestamp = messages[i].msg.shutter.timestamp;
            break;
        default:
            ALOGE("%s: Unsupported notify message %d", __FUNCTION__,
                  messages[i].type);
            break;
        }
    }

    mParent->mResultCondition.notify_one();
    return Void();
}

hidl_vec<hidl_string>
NativeCameraHidl::getCameraDeviceNames(int g_camera_id,
                                       sp<ICameraProvider> provider) {
    std::vector<std::string> cameraDeviceNames;
    Return<void> ret;
    ret = provider->getCameraIdList([&](auto status, const auto &idList) {
        ALOGI("getCameraIdList returns status:%d", (int)status);
        for (size_t i = 0; i < idList.size(); i++) {
            ALOGI("Camera Id[%zu] is %s", i, idList[i].c_str());
        }
        // for (const auto & id : idList) {
        //  cameraDeviceNames.push_back(id);
        //}
        cameraDeviceNames.push_back(idList[g_camera_id].c_str());
    });
    if (!ret.isOk()) {
        ALOGE("%s\uff0c%d", __FUNCTION__, __LINE__);
    }

    // External camera devices are reported through cameraDeviceStatusChange
    struct ProviderCb : public ICameraProviderCallback {
        virtual Return<void>
        cameraDeviceStatusChange(const hidl_string &devName,
                                 CameraDeviceStatus newStatus) override {
            ALOGI("camera device status callback name %s, status %d",
                  devName.c_str(), (int)newStatus);
            if (newStatus == CameraDeviceStatus::PRESENT) {
                externalCameraDeviceNames.push_back(devName);
            }
            return Void();
        }

        virtual Return<void> torchModeStatusChange(const hidl_string &,
                                                   TorchModeStatus) override {
            return Void();
        }

        std::vector<std::string> externalCameraDeviceNames;
    };
    sp<ProviderCb> cb = new ProviderCb;
    auto status = mProvider->setCallback(cb);

    for (const auto &devName : cb->externalCameraDeviceNames) {
        if (cameraDeviceNames.end() == std::find(cameraDeviceNames.begin(),
                                                 cameraDeviceNames.end(),
                                                 devName)) {
            cameraDeviceNames.push_back(devName);
        }
    }

    hidl_vec<hidl_string> retList(cameraDeviceNames.size());
    for (size_t i = 0; i < cameraDeviceNames.size(); i++) {
        retList[i] = cameraDeviceNames[i];
    }
    return retList;
}

// Retrieve all valid output stream resolutions from the camera
// static characteristics.
Status NativeCameraHidl::getAvailableOutputStreams(
    camera_metadata_t *staticMeta, std::vector<AvailableStream> &outputStreams,
    const AvailableStream *threshold) {
    if (nullptr == staticMeta) {
        return Status::ILLEGAL_ARGUMENT;
    }

    camera_metadata_ro_entry entry;
    int rc = find_camera_metadata_ro_entry(
        staticMeta, ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
    if ((0 != rc) || (0 != (entry.count % 4))) {
        return Status::ILLEGAL_ARGUMENT;
    }

    for (size_t i = 0; i < entry.count; i += 4) {
        if (ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT ==
            entry.data.i32[i + 3]) {
            if (nullptr == threshold) {
                AvailableStream s = {entry.data.i32[i + 1],
                                     entry.data.i32[i + 2], entry.data.i32[i]};
                outputStreams.push_back(s);
            } else {
                if ((threshold->format == entry.data.i32[i]) &&
                    (threshold->width >= entry.data.i32[i + 1]) &&
                    (threshold->height >= entry.data.i32[i + 2])) {
                    AvailableStream s = {entry.data.i32[i + 1],
                                         entry.data.i32[i + 2],
                                         threshold->format};
                    outputStreams.push_back(s);
                }
            }
        }
    }

    return Status::OK;
}

void NativeCameraHidl::createStreamConfiguration(
    const ::android::hardware::hidl_vec<V3_2::Stream> &streams3_2,
    StreamConfigurationMode configMode,
    ::android::hardware::camera::device::V3_2::StreamConfiguration
        *config3_2 /*out*/,
    ::android::hardware::camera::device::V3_4::StreamConfiguration
        *config3_4 /*out*/) {
    ::android::hardware::hidl_vec<V3_4::Stream> streams3_4(streams3_2.size());
    size_t idx = 0;
    for (auto &stream3_2 : streams3_2) {
        V3_4::Stream stream;
        stream.v3_2 = stream3_2;
        streams3_4[idx++] = stream;
    }
    *config3_4 = {streams3_4, configMode, {}};
    *config3_2 = {streams3_2, configMode};
}

// Open a device session and configure a preview stream.
void NativeCameraHidl::configureAvailableStream(
    const std::string &name, int32_t deviceVersion,
    sp<ICameraProvider> provider, const AvailableStream *previewThreshold,
    sp<ICameraDeviceSession> *session /*out*/,
    V3_2::Stream *previewStream /*out*/,
    HalStreamConfiguration *halStreamConfig /*out*/,
    bool *supportsPartialResults /*out*/,
    uint32_t *partialResultCount /*out*/) {

    std::vector<AvailableStream> outputPreviewStreams;
    std::vector<AvailableStream> outputCaptureStreams;
    std::vector<AvailableStream> outputCallbackStreams;
    std::vector<AvailableStream> outputVideoStreams;
    ::android::sp<ICameraDevice> device3_x;
    bool captureStreamNeed = false;
    bool videoStreamNeed = false;
    bool callbackStreamNeed = false;
    ALOGI("configureStreams: Testing camera device %s", name.c_str());
    Return<void> ret;
    ret = provider->getCameraDeviceInterface_V3_x(
        name, [&](auto status, const auto &device) {
            ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                  (int)status);
            device3_x = device;
        });

    /*  sp<DeviceCb> cb = new DeviceCb(this);
      ret = device3_x->open(
                cb,
      [&](auto status, const auto & newSession) {
          ALOGI("device::open returns status:%d", (int)status);
          *session = newSession;
      });*/
    *session = g_session;
    sp<device::V3_3::ICameraDeviceSession> session3_3;
    sp<device::V3_4::ICameraDeviceSession> session3_4;
    castSession(*session, deviceVersion, &session3_3, &session3_4);

    camera_metadata_t *staticMeta;
    ret = device3_x->getCameraCharacteristics(
        [&](Status s,
            android::hardware::camera::device::V3_2::CameraMetadata metadata) {
            ALOGI("%s,%d,s: %d", __FUNCTION__, __LINE__,
                  static_cast<uint32_t>(s));
            staticMeta = clone_camera_metadata(
                reinterpret_cast<const camera_metadata_t *>(metadata.data()));
        });

    camera_metadata_ro_entry entry;
    auto status = find_camera_metadata_ro_entry(
        staticMeta, ANDROID_REQUEST_PARTIAL_RESULT_COUNT, &entry);
    if ((0 == status) && (entry.count > 0)) {
        *partialResultCount = entry.data.i32[0];
        *supportsPartialResults = (*partialResultCount > 1);
    }

    outputPreviewStreams.clear();
    auto rc = getAvailableOutputStreams(staticMeta, outputPreviewStreams,
                                        previewThreshold);
    for (int i = 0; i < g_stream_count; i++) {
        if (g_streamConfig[i].stream_type ==
            CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
            AvailableStream captureThreshold = {
                g_streamConfig[i].width, g_streamConfig[i].height,
                static_cast<int32_t>(g_streamConfig[i].pixel)};
            auto rc1 = getAvailableOutputStreams(
                staticMeta, outputCaptureStreams, &captureThreshold);
            ALOGI("%s,%d,rc: %d", __FUNCTION__, __LINE__,
                  static_cast<uint32_t>(rc));
            captureStreamNeed = true;
        }
    }

    for (int i = 0; i < g_stream_count; i++) {
        if (g_streamConfig[i].stream_type == CAMERA_STREAM_TYPE_CALLBACK) {
            AvailableStream callbackThreshold = {
                g_streamConfig[i].width, g_streamConfig[i].height,
                static_cast<int32_t>(g_streamConfig[i].pixel)};
            auto rc1 = getAvailableOutputStreams(
                staticMeta, outputCallbackStreams, &callbackThreshold);
            ALOGI("%s,%d,rc: %d", __FUNCTION__, __LINE__,
                  static_cast<uint32_t>(rc));
            callbackStreamNeed = true;
        }
    }

    for (int i = 0; i < g_stream_count; i++) {
        if (g_streamConfig[i].stream_type == CAMERA_STREAM_TYPE_VIDEO) {
            AvailableStream videoThreshold = {
                g_streamConfig[i].width, g_streamConfig[i].height,
                static_cast<int32_t>(g_streamConfig[i].pixel)};
            auto rc1 = getAvailableOutputStreams(staticMeta, outputVideoStreams,
                                                 &videoThreshold);
            ALOGI("%s,%d,rc: %d", __FUNCTION__, __LINE__,
                  static_cast<uint32_t>(rc));
            videoStreamNeed = true;
        }
    }

    free_camera_metadata(staticMeta);
    int size = outputCaptureStreams.size();

    V3_2::Stream stream3_2;
    int32_t streamIndex = 0;
    ALOGI("g_stream_count = %d", g_stream_count);
    ::android::hardware::hidl_vec<V3_2::Stream> streams3_2(g_stream_count);
    stream3_2 = {
        streamIndex,
        StreamType::OUTPUT,
        static_cast<uint32_t>(outputPreviewStreams[0].width),
        static_cast<uint32_t>(outputPreviewStreams[0].height),
        static_cast<android::hardware::graphics::common::V1_0::PixelFormat>(
            outputPreviewStreams[0].format),
        GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
        0,
        StreamRotation::ROTATION_0};
    streams3_2[streamIndex] = stream3_2; // preview stream
    streamIndex++;
    streamlist.push_back(CAMERA_STREAM_TYPE_PREVIEW);

    if (callbackStreamNeed) {
        V3_2::Stream stream3_2_2; // capture stream configure
        stream3_2_2 = {
            streamIndex,
            StreamType::OUTPUT,
            static_cast<uint32_t>(outputCallbackStreams[0].width),
            static_cast<uint32_t>(outputCallbackStreams[0].height),
            static_cast<android::hardware::graphics::common::V1_0::PixelFormat>(
                outputCallbackStreams[0].format),
            GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
            0,
            StreamRotation::ROTATION_0};
        //::android::hardware::hidl_vec<V3_2::Stream> streams3_2 =
        //{stream3_2,stream3_2_1};
        streams3_2[streamIndex] = stream3_2_2; // captrue stream
        streamIndex++;
        streamlist.push_back(CAMERA_STREAM_TYPE_CALLBACK);
        ALOGI("callbackstream configured");
    }

    if (videoStreamNeed) {
        V3_2::Stream stream3_2_3; // capture stream configure
        stream3_2_3 = {streamIndex,
                       StreamType::OUTPUT,
                       static_cast<uint32_t>(outputVideoStreams[0].width),
                       static_cast<uint32_t>(outputVideoStreams[0].height),
                       static_cast<android::hardware::graphics::common::V1_0::PixelFormat>(
                           outputVideoStreams[0].format),
                       GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER,
                       0,
                       StreamRotation::ROTATION_0};
        //::android::hardware::hidl_vec<V3_2::Stream> streams3_2 =
        //{stream3_2,stream3_2_1};
        streams3_2[streamIndex] = stream3_2_3; // captrue stream
        streamIndex++;
        streamlist.push_back(CAMERA_STREAM_TYPE_VIDEO);
        ALOGI("video configured");
    }

    if (captureStreamNeed) {
        V3_2::Stream stream3_2_1; // capture stream configure
        stream3_2_1 = {streamIndex,
                       StreamType::OUTPUT,
                       static_cast<uint32_t>(outputCaptureStreams[0].width),
                       static_cast<uint32_t>(outputCaptureStreams[0].height),
                       static_cast<android::hardware::graphics::common::V1_0::PixelFormat>(
                           outputCaptureStreams[0].format),
                       GRALLOC1_CONSUMER_USAGE_HWCOMPOSER,
                       0,
                       StreamRotation::ROTATION_0};
        //::android::hardware::hidl_vec<V3_2::Stream> streams3_2 =
        //{stream3_2,stream3_2_1};
        streams3_2[streamIndex] = stream3_2_1; // captrue stream
        streamIndex++;
        streamlist.push_back(CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT);
    }

    /*add more stream here*/
    ::android::hardware::camera::device::V3_2::StreamConfiguration config3_2;
    ::android::hardware::camera::device::V3_4::StreamConfiguration config3_4;
    createStreamConfiguration(streams3_2, StreamConfigurationMode::NORMAL_MODE,
                              &config3_2, &config3_4);
    if (session3_4 != nullptr) {
        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        ret = session3_4->constructDefaultRequestSettings(
            reqTemplate, [&config3_4](auto status, const auto &req) {
                ALOGD("%s,%d,status: %d", __FUNCTION__, __LINE__,
                      static_cast<uint32_t>(status));
                config3_4.sessionParams = req;
            });
        ret = session3_4->configureStreams_3_4(
            config3_4,
            [&](Status s, device::V3_4::HalStreamConfiguration halConfig) {
                ALOGD("%s,%d,status: %d", __FUNCTION__, __LINE__,
                      static_cast<uint32_t>(s));
                halStreamConfig->streams.resize(halConfig.streams.size());
                for (size_t i = 0; i < halConfig.streams.size(); i++) {
                    halStreamConfig->streams[i] =
                        halConfig.streams[i].v3_3.v3_2;
                }
            });
    } else if (session3_3 != nullptr) {
        ret = session3_3->configureStreams_3_3(
            config3_2,
            [&](Status s, device::V3_3::HalStreamConfiguration halConfig) {
                ALOGD("%s,%d,status: %d", __FUNCTION__, __LINE__,
                      static_cast<uint32_t>(s));
                halStreamConfig->streams.resize(halConfig.streams.size());
                for (size_t i = 0; i < halConfig.streams.size(); i++) {
                    halStreamConfig->streams[i] = halConfig.streams[i].v3_2;
                    ALOGE("stream id = %d", halStreamConfig->streams[i].id);
                }
            });
    } else {
        ret = (*session)->configureStreams(
            config3_2, [&](Status s, HalStreamConfiguration halConfig) {
                ALOGV("%s,%d,status: %d", __FUNCTION__, __LINE__,
                      static_cast<uint32_t>(s));
                *halStreamConfig = halConfig;
            });
    }
    *previewStream = stream3_2;
}

// Cast camera device session to corresponding version
void NativeCameraHidl::castSession(
    const sp<ICameraDeviceSession> &session, int32_t deviceVersion,
    sp<device::V3_3::ICameraDeviceSession> *session3_3 /*out*/,
    sp<device::V3_4::ICameraDeviceSession> *session3_4 /*out*/) {

    switch (deviceVersion) {
    case CAMERA_DEVICE_API_VERSION_3_4_1: {
        auto castResult = device::V3_4::ICameraDeviceSession::castFrom(session);
        *session3_4 = castResult;
        break;
    }
    case CAMERA_DEVICE_API_VERSION_3_3_1: {
        auto castResult = device::V3_3::ICameraDeviceSession::castFrom(session);
        *session3_3 = castResult;
        break;
    }
    default:
        // no-op
        return;
    }
}

int NativeCameraHidl::map2(buffer_handle_t *buffer_handle,
                           hal_mem_info_t *mem_info) {
    int ret = 0;

    if (NULL == mem_info || NULL == buffer_handle) {
        ALOGE("Param invalid handle=%p, info=%p", buffer_handle, mem_info);
        return -EINVAL;
    }

    int width = g_streamConfig[snapshotStreamID].width *
                g_streamConfig[snapshotStreamID].height;
    int height = 1;
    int format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    android_ycbcr ycbcr;
    android::Rect bounds(width, height);
    void *vaddr = NULL;
    int usage;
    android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

    bzero((void *)&ycbcr, sizeof(ycbcr));
    usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
    // ALOGE("nativecamera map2 width=%d,height =%d,format =%d buffer_handle =
    // %p",width,height,format,buffer_handle);
    // ALOGE("nativecamera map2 555 width=%d,height =%d,format =%d buffer_handle
    // = %p",width,height,format,*buffer_handle);
    if (format == HAL_PIXEL_FORMAT_YCbCr_420_888) {
        ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle, usage,
                               bounds, &ycbcr);
        if (ret != 0) {
            ALOGE("lockcbcr.onQueueFilled, mapper.lock failed try "
                  "lockycbcr. %p, ret %d",
                  *buffer_handle, ret);
            ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                              bounds, &vaddr);
            if (ret != 0) {
                ALOGE("locky.onQueueFilled, mapper.lock fail %p, ret %d",
                      *buffer_handle, ret);
            } else {
                mem_info->addr_vir = vaddr;
            }
        } else {
            mem_info->addr_vir = ycbcr.y;
        }
    } else {
        ret = mapper.lock((const native_handle_t *)*buffer_handle, usage,
                          bounds, &vaddr);
        if (ret != 0) {
            ALOGE("lockonQueueFilled, mapper.lock failed try lockycbcr. %p, "
                  "ret %d",
                  *buffer_handle, ret);
            ret = mapper.lockYCbCr((const native_handle_t *)*buffer_handle,
                                   usage, bounds, &ycbcr);
            if (ret != 0) {
                ALOGE("lockycbcr.onQueueFilled, mapper.lock fail %p, ret %d",
                      *buffer_handle, ret);
            } else {
                mem_info->addr_vir = ycbcr.y;
            }
        } else {
            mem_info->addr_vir = vaddr;
        }
    }
    // mem_info->fd = ADP_BUFFD(*buffer_handle);
    // mem_info->addr_phy is offset, always set to 0 for yaddr
    mem_info->addr_phy = (void *)0;
    // mem_info->size = ADP_BUFSIZE(*buffer_handle);
    // mem_info->width = ADP_WIDTH(*buffer_handle);
    // mem_info->height = ADP_HEIGHT(*buffer_handle);
    // mem_info->format= ADP_FORMAT(*buffer_handle);
    ALOGE("fd = 0x%x, addr_phy offset = %p, addr_vir = %p,"
          " buf size = %zu, width = %d, height = %d, fmt = %d",
          mem_info->fd, mem_info->addr_phy, mem_info->addr_vir, mem_info->size,
          mem_info->width, mem_info->height, mem_info->format);

err_out:
    return ret;
}

buffer_handle_t *
NativeCameraHidl::popBufferList(android::List<new_mem_t *> &list, int type) {
    buffer_handle_t *ret = NULL;
    if (list.empty()) {
        ALOGE("list is NULL");
        return NULL;
    }
    // Mutex::Autolock l(mBufferListLock);
    android::List<new_mem_t *>::iterator j = list.begin();
    for (; j != list.end(); j++) {
        ret = &((*j)->native_handle);
        if (ret && 1) {
            break;
        }
    }
    if (ret == NULL || j == list.end()) {
        ALOGE("popBufferList failed!");
        return ret;
    }
    list.erase(j);
    return ret;
}

int NativeCameraHidl::openNativeCamera(int g_camera_id) {
    ALOGI("%s: g_camera_id: %d", __FUNCTION__, g_camera_id);
    uint32_t id;
    std::string service_name = "legacy/0";
    ALOGI("get service with name: %s", service_name.c_str());
    mProvider = ICameraProvider::getService(service_name);
    ALOGI("mProvider address = %p", &mProvider);
    parseProviderName(service_name, &mProviderType, &id);
    hidl_vec<hidl_string> cameraDeviceNames;
    if (g_camera_id < 4)
        cameraDeviceNames = getCameraDeviceNames(g_camera_id, mProvider);
    ::android::sp<ICameraDevice> device3_x;

    if (g_camera_id > 4) // logical camera id
    {
        char cameraid[50];
        sprintf(cameraid, "device@3.3/legacy/%d", g_camera_id);
        hidl_string string_id(cameraid);
        Return<void> ret;
        int deviceVersion = getCameraDeviceVersion(string_id, mProviderType);
        ALOGI("name = %s,deviceVersion=%d", string_id.c_str(), deviceVersion);
        ret = mProvider->getCameraDeviceInterface_V3_x(
            string_id, [&](auto status, const auto &device) {
                ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                      (int)status);
                device3_x = device;
            });
        sp<DeviceCb> cb = new DeviceCb(this);
        ret = device3_x->open(cb, [&](auto status, const auto &newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            g_session = newSession;
        });
        //    ALOGI("open camera id =%d, ret =%d",g_camera_id ,ret);
        return 0;
    }

    for (const auto &name : cameraDeviceNames) {
        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0_1) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __FUNCTION__,
                  deviceVersion);
            return -1;
        }
        ALOGI("configureStreams: Testing camera device %s", name.c_str());
        Return<void> ret;
        // sp<ICameraDeviceSession> session;
        ret = mProvider->getCameraDeviceInterface_V3_x(
            name, [&](auto status, const auto &device) {
                ALOGI("getCameraDeviceInterface_V3_x returns status:%d",
                      (int)status);
                device3_x = device;
            });
        sp<DeviceCb> cb = new DeviceCb(this);
        ret = device3_x->open(cb, [&](auto status, const auto &newSession) {
            ALOGI("device::open returns status:%d", (int)status);
            g_session = newSession;
        });
    }
    ALOGE("open camera end");
    return 0;
}
#define CMR_EVT_UT_IT_BASE (1 << 27)
#define CMR_EVT_PREVIEW_START (CMR_EVT_UT_IT_BASE + 0X100)

static void *ut_process_thread_proc(void *data) {

    NativeCameraHidl *nativecamera = (NativeCameraHidl *)data;
    nativecamera->startnativePreview(0, 1440, 1080, g_streamConfig[0].width,
                                     g_streamConfig[0].height, 0);
    ALOGE("ut_process_thread_proc exit");
    return NULL;
}
int NativeCameraHidl::startpreview() // create preview running thread
{
    int ret = 0;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&preview_thread_handle, &attr, ut_process_thread_proc,
                         (void *)this);
    pthread_setname_np(preview_thread_handle, "grab");
    pthread_attr_destroy(&attr);
    return ret;
}
int NativeCameraHidl::transferMetaData(HalCaseComm *_json2) {
    uint32_t tag = 0;
    uint8_t tag_type = 0;
    string tag_name;
    bool is_sprdtag = false;
    bool is_androidtag = false;
    int i = 0;
    long vendorId;
    int64_t temp = 1;
    uint8_t value;
    uint8_t *value_2;
    int32_t **p;
    meta_int32.clear();
    meta_u8.clear();
    meta_int32.resize(50);
    meta_u8.resize(50);
    for (auto &metadata : _json2->m_MetadataArr) {
        is_sprdtag = false;
        tag = 0;
        tag_name = metadata->m_tagName;
        if (mMap_tag.find(tag_name) != mMap_tag.end()) {
            tag = mMap_tag[tag_name];
            tag_type = mMap_tagType[tag_name];
            ALOGI("sprd tag =%ld,tag_type =%d,tag_name=%s", tag, tag_type,
                  tag_name.c_str());
            is_sprdtag = true;
        }
        // android tag ,continue to find
        if (!is_sprdtag) {
            tag_name = metadata->m_tagName;
            sp<VendorTagDescriptor> vTags =
                VendorTagDescriptor::getGlobalVendorTagDescriptor();
            if (vTags.get() == NULL) {
                sp<VendorTagDescriptorCache> cache =
                    VendorTagDescriptorCache::getGlobalVendorTagCache();
                if (cache.get() != NULL) {
                    cache->getVendorTagDescriptor(vendorId, &vTags);
                }
            }
            status_t res = ::android::hardware::camera::common::V1_0::helper::
                CameraMetadata::getTagFromName(tag_name.c_str(), vTags.get(),
                                               &tag);
            if (tag == 0) {
                ALOGI("not find the metadata , please check the metaname");
                return -1;
            }
            tag_type = get_local_camera_metadata_tag_type(tag, NULL);
            ALOGI("andorid tag =%ld,tag_type =%d,tag_name=%s", tag, tag_type,
                  tag_name.c_str());
        }

        updatemeta[metacount].tag = tag;
        updatemeta[metacount].count = metadata->mVec_tagVal.size();
        updatemeta[metacount].type = tag_type;
        switch (tag_type) {
        case 0:
            for (i = 0; i < updatemeta[metacount].count; i++) {
                meta_u8.push_back((uint8_t)metadata->mVec_tagVal[i].i32);
                if (i == 0) {
                    updatemeta[metacount].data.u8 = &meta_u8.back();
                }
                ALOGI("update success,updatemeta[metacount].data.u8=%d",
                      updatemeta[metacount].data.u8[i]);
            }
            break;
        case 1:
            ALOGI("count =%d", updatemeta[metacount].count);
            for (i = 0; i < updatemeta[metacount].count; i++) {
                meta_int32.push_back((int32_t)metadata->mVec_tagVal[i].i32);
                if (i == 0) {
                    updatemeta[metacount].data.i32 = &meta_int32.back();
                }
                ALOGI("update success,updatemeta[metacount].data.int32=%d",
                      updatemeta[metacount].data.i32[i]);
            }
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        }
        metacount++;
        ALOGI("metacount =%d", metacount);
    }
    return 0;
}

::android::hardware::camera::common::V1_0::helper::CameraMetadata
NativeCameraHidl::updatemetedata(camera_metadata_t *metaBuffer) {
    // camera_metadata_t *metaBuffer;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata
        requestmeta;
    requestmeta = metaBuffer;
    // metaBuffer = requestmeta.release();
    void *data;
    for (int i = 0; i < metacount; i++) {
        metaBuffer = requestmeta.release();

        if (updatemeta[i].tag > VENDOR_SECTION_START) {
            switch (updatemeta[i].type) {
            case 0:
                data = (void *)updatemeta[i].data.u8;
                break;
            case 1:
                data = (void *)updatemeta[i].data.i32;
                break;
            case 2:
                data = (void *)updatemeta[i].data.f;
                break;
            case 3:
                data = (void *)updatemeta[i].data.i64;
                break;
            case 4:
                data = (void *)updatemeta[i].data.d;
                break;
            case 5:
                data = (void *)updatemeta[i].data.r;
                break;
            default:
                break;
            }
            ALOGE("enter > VENDOR_SECTION_START");
            camera_metadata_entry_t entry;
            int res = find_camera_metadata_entry(metaBuffer, updatemeta[i].tag,
                                                 &entry);
            // switch(updatemeta[i].type)
            // case  1:
            if (res == -1) {
                res = add_camera_metadata_entry(metaBuffer, updatemeta[i].tag,
                                                data, updatemeta[i].count);
            } else if (res == android::OK) {
                res = update_camera_metadata_entry(
                    metaBuffer, entry.index, data, updatemeta[i].count, NULL);
            }
            requestmeta = metaBuffer;
        } else {
            ALOGE("enter <=  VENDOR_SECTION_START");
            requestmeta = metaBuffer;
            switch (updatemeta[i].type) {
            case 0:
                ALOGE("type =%d,data=%d,i=%d,tag=%ld", updatemeta[i].type,
                      updatemeta[i].data.u8[0], i, updatemeta[i].tag);
                requestmeta.update(updatemeta[i].tag, updatemeta[i].data.u8,
                                   updatemeta[i].count);
                break;
            case 1:
                requestmeta.update(updatemeta[i].tag, updatemeta[i].data.i32,
                                   updatemeta[i].count);
                break;
            case 2:
                requestmeta.update(updatemeta[i].tag, updatemeta[i].data.f,
                                   updatemeta[i].count);
                break;
            case 3:
                requestmeta.update(updatemeta[i].tag, updatemeta[i].data.i64,
                                   updatemeta[i].count);
                break;
            case 4:
                requestmeta.update(updatemeta[i].tag, updatemeta[i].data.d,
                                   updatemeta[i].count);
                break;
            case 5:
                requestmeta.update(updatemeta[i].tag, updatemeta[i].data.r,
                                   updatemeta[i].count);
                break;
            default:
                break;
            }
        }
    }

    ALOGE("updatemeta end");
    return requestmeta;
}

::android::hardware::camera::common::V1_0::helper::CameraMetadata
NativeCameraHidl::updatemetedata_v2(camera_metadata_t *metaBuffer,
                                    camera_metadata_entry *meta_entry,
                                    int meta_count) {
    // camera_metadata_t *metaBuffer;
    ::android::hardware::camera::common::V1_0::helper::CameraMetadata
        requestmeta;
    requestmeta = metaBuffer;
    // metaBuffer = requestmeta.release();
    void *data;
    for (int i = 0; i < meta_count; i++) {
        metaBuffer = requestmeta.release();

        if (updatemeta[i].tag > VENDOR_SECTION_START) {
            switch (meta_entry[i].type) {
            case 0:
                data = (void *)meta_entry[i].data.u8;
                break;
            case 1:
                data = (void *)meta_entry[i].data.i32;
                break;
            case 2:
                data = (void *)meta_entry[i].data.f;
                break;
            case 3:
                data = (void *)meta_entry[i].data.i64;
                break;
            case 4:
                data = (void *)meta_entry[i].data.d;
                break;
            case 5:
                data = (void *)meta_entry[i].data.r;
                break;
            default:
                break;
            }
            ALOGE("enter > VENDOR_SECTION_START");
            camera_metadata_entry_t entry;
            int res = find_camera_metadata_entry(metaBuffer, meta_entry[i].tag,
                                                 &entry);
            switch (meta_entry[i].type)
            case 1:
                if (res == -1) {
                    res =
                        add_camera_metadata_entry(metaBuffer, meta_entry[i].tag,
                                                  data, meta_entry[i].count);
                } else if (res == android::OK) {
                    res = update_camera_metadata_entry(
                        metaBuffer, entry.index, data, meta_entry[i].count,
                        NULL);
                }
            requestmeta = metaBuffer;
        } else {
            ALOGE("enter <=  VENDOR_SECTION_START");
            requestmeta = metaBuffer;
            switch (meta_entry[i].type) {
            case 0:
                ALOGE("type =%d,data=%d,i=%d,tag=%ld", meta_entry[i].type,
                      meta_entry[i].data.u8[0], i, meta_entry[i].tag);
                requestmeta.update(meta_entry[i].tag, meta_entry[i].data.u8,
                                   meta_entry[i].count);
                break;
            case 1:
                requestmeta.update(meta_entry[i].tag, meta_entry[i].data.i32,
                                   meta_entry[i].count);
                break;
            case 2:
                requestmeta.update(meta_entry[i].tag, meta_entry[i].data.f,
                                   meta_entry[i].count);
                break;
            case 3:
                requestmeta.update(meta_entry[i].tag, meta_entry[i].data.i64,
                                   meta_entry[i].count);
                break;
            case 4:
                requestmeta.update(meta_entry[i].tag, meta_entry[i].data.d,
                                   meta_entry[i].count);
                break;
            case 5:
                requestmeta.update(meta_entry[i].tag, meta_entry[i].data.r,
                                   meta_entry[i].count);
                break;
            default:
                break;
            }
        }
    }

    ALOGE("updatemeta end");
    return requestmeta;
}

int NativeCameraHidl::SetupSprdTags() {
    android::tags_info_t *cam_tag = android::cam_tag_info[0];
    for (int i = 0; i < (android::VENDOR_SECTION_END - VENDOR_SECTION_START);
         i++) {
        string tag_name(cam_tag[i].tag_name,
                        cam_tag[i].tag_name + strlen(cam_tag[i].tag_name));
        mMap_tag[tag_name] = VENDOR_SECTION_START + i;
        mMap_tagType[tag_name] = cam_tag[i].tag_type;
    }
    return 0;
}

int NativeCameraHidl::startnativePreview(int g_camera_id, int g_width,
                                         int g_height, int g_sensor_width,
                                         int g_sensor_height, int g_rotate) {
    ALOGI("%s: g_camera_id: %d", __FUNCTION__, g_camera_id);
    std::string service_name = "legacy/0";
    ALOGI("get service with name: %s", service_name.c_str());
    // mProvider = ICameraProvider::getService(service_name);

    uint32_t id;
    bool previewStreamFound = false;
    uint64_t bufferId = 0;
    uint32_t frameNumber = 0;
    ::android::hardware::hidl_vec<uint8_t> settings;

    // parseProviderName(service_name, &mProviderType, &id);
    ALOGI("mProvider address = %p", &mProvider);
    hidl_vec<hidl_string> cameraDeviceNames =
        getCameraDeviceNames(g_camera_id, mProvider);
    std::vector<AvailableStream> outputPreviewStreams;
    AvailableStream previewThreshold;

    for (int i = 0; i < g_stream_count; i++) {
        if (g_streamConfig[i].stream_type == CAMERA_STREAM_TYPE_PREVIEW) {
            previewThreshold = {g_streamConfig[i].width,
                                g_streamConfig[i].height,
                                static_cast<int32_t>(g_streamConfig[i].pixel)};
            ALOGI("g_streamConfig .stream_type =%d",
                  g_streamConfig[i].stream_type);
            previewStreamID = i;
            previewStreamFound = true;
        }
        if (g_streamConfig[i].stream_type ==
            CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
            snapshotStreamID = i;
        }

        if (g_streamConfig[i].stream_type == CAMERA_STREAM_TYPE_CALLBACK) {
            callbackStreamID = i;
        }

        if (g_streamConfig[i].stream_type == CAMERA_STREAM_TYPE_VIDEO) {
            videoStreamID = i;
        }
        ALOGI("previewStreamID=%d,snapshotStreamID=%d,callbackStreamID=%d,"
              "videoStreamID=%d",
              previewStreamID, snapshotStreamID, callbackStreamID,
              videoStreamID);
    }

    if (!previewStreamFound) {
        ALOGE("please config preview stream first");
        return -1;
    }

    for (const auto &name : cameraDeviceNames) {

        int deviceVersion = getCameraDeviceVersion(name, mProviderType);
        if (deviceVersion == CAMERA_DEVICE_API_VERSION_1_0_1) {
            continue;
        } else if (deviceVersion <= 0) {
            ALOGE("%s: Unsupported device version %d", __FUNCTION__,
                  deviceVersion);
            return -1;
        }

        V3_2::Stream previewStream;
        HalStreamConfiguration halStreamConfig;
        sp<ICameraDeviceSession> session;
        bool supportsPartialResults = false;
        uint32_t partialResultCount = 0;
        PERFORMANCE_MONITOR("configureAvailableStream",
                             configureAvailableStream(
            name, deviceVersion, mProvider, &previewThreshold, &session /*out*/,
            &previewStream /*out*/, &halStreamConfig /*out*/,
            &supportsPartialResults /*out*/, &partialResultCount /*out*/));
        std::shared_ptr<ResultMetadataQueue> resultQueue;
        auto resultQueueRet = session->getCaptureResultMetadataQueue(
            [&resultQueue](const auto &descriptor) {
                resultQueue = std::make_shared<ResultMetadataQueue>(descriptor);
                if (!resultQueue->isValid() ||
                    resultQueue->availableToWrite() <= 0) {
                    ALOGE("%s: HAL returns empty result metadata fmq,"
                          " not use it",
                          __FUNCTION__);
                    resultQueue = nullptr;
                    // Don't use the queue onwards.
                }
            });

        InFlightRequest inflightReq = {1, false, supportsPartialResults,
                                       partialResultCount, resultQueue};
        RequestTemplate reqTemplate = RequestTemplate::PREVIEW;
        Return<void> ret;
        ret = session->constructDefaultRequestSettings(
            reqTemplate, [&](auto status, const auto &req) {
                ALOGV("%s,%d,status: %d", __FUNCTION__, __LINE__,
                      static_cast<uint32_t>(status));
                settings = req;
            });

#if 1

        int32_t _frameNum = 0;
        int32_t result;
        camera3_stream_t preview_stream;
        camera3_stream_t capture_stream;
        camera3_stream_t callback_stream;
        camera3_stream_t video_stream;

        camera3_stream_buffer_t sb[g_stream_count]; // buffer
        camera3_capture_request_t request_buf;

        // create preview surface
        int bufferIdx = 0;
        int maxConsumerBuffers;
        android::DisplayInfo dinfo;

        /*alloc local preview memory begin */
        for (size_t j = 0; j < max_buffers + 1; j++) {
            if (0 > allocateOne(g_streamConfig[previewStreamID].width,
                                g_streamConfig[previewStreamID].height,
                                &(mLocalBuffer[j]), 0)) {
                ALOGE("request one buf failed.");
                return -1;
            }
            mLocalBufferList.push_back(&(mLocalBuffer[j]));
        }
        ALOGE("alloc preview buffer num =%d", max_buffers + 1);
        /*alloc local preview memory end */
        for (int i = 0; i < g_stream_count; i++) {
            if (streamlist[i] == CAMERA_STREAM_TYPE_CALLBACK) {
                for (size_t j = 0; j < max_buffers + 1; j++) {
                    if (0 > allocateOne(g_streamConfig[callbackStreamID].width,
                                        g_streamConfig[callbackStreamID].height,
                                        &(mCallbackBuffer[j]), 0)) {
                        ALOGE("request one buf failed.");
                        return -1;
                    }
                    // mLocalBuffer[j].type = PREVIEW_MAIN_BUFFER;
                    mCallbackBufferList.push_back(&(mCallbackBuffer[j]));
                }
                ALOGE("alloc callback buffer num =%d", max_buffers + 1);
            }
            // alloc other buffer here
            if (streamlist[i] == CAMERA_STREAM_TYPE_VIDEO) {
                for (size_t j = 0; j < max_buffers + 1; j++) {
                    if (0 > allocateOne(g_streamConfig[videoStreamID].width,
                                        g_streamConfig[videoStreamID].height,
                                        &(mVideoBuffer[j]), 0)) {
                        ALOGE("request one buf failed.");
                        return -1;
                    }
                    // mLocalBuffer[j].type = PREVIEW_MAIN_BUFFER;
                    mVideoBufferList.push_back(&(mVideoBuffer[j]));
                }
                ALOGE("video  buffer num =%d", max_buffers + 1);
            }
        }

        /* surface code */
        /*
                ANativeWindowBuffer **anwBuffers = NULL;
                // create a client to surfaceflinger
                int trim_x = 0, trim_y = 0;
                ALOGE("begin get surface 111");
                sp<android::SurfaceComposerClient> composerClient = new
           android::SurfaceComposerClient();
                ALOGE("begin get surface 000");
                if (android::NO_ERROR != composerClient->initCheck()) {
                    E("initCheck error");
                    return -1;
                }
                ALOGE("begin get surface 222");
                //android::SurfaceComposerClient::getDisplayInfo(android::SurfaceComposerClient::getBuiltInDisplay(android::ISurfaceComposer::eDisplayIdMain),
           &dinfo);
                android::SurfaceComposerClient::getDisplayInfo(android::SurfaceComposerClient::getInternalDisplayToken(),
           &dinfo);
                E("w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n", dinfo.w, dinfo.h,
           dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);
               // g_width = dinfo.w;
               // g_height = dinfo.h;
                ALOGE("begin get surface 333");
                sp<android::SurfaceControl> surfaceControl =
           composerClient->createSurface(::android::String8("NativeCameraPreviewSurface"),
           g_width + trim_x, g_height + trim_y, g_surface_f);

                if (NULL == surfaceControl.get() || !surfaceControl->isValid())
           {
                    E("createSurface error");
                    return -1;
                }
                android::SurfaceComposerClient::Transaction t;

                t.setPosition(surfaceControl, -trim_x / 2, 0);
                t.setLayer(surfaceControl, 0x7FFFFFFF).apply();
                t.show(surfaceControl);
                sp<Surface> surface = surfaceControl->getSurface();
                if (surface == NULL || surface.get() == NULL) {
                    E("getSurface error");
                    return -1;
                }
                ALOGE("begin get surface 444");
                nativeWindow = surface;
                android::status_t ui_status =
           native_window_api_connect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                if (ui_status != android::OK) {
                    E("native_window_api_connect error");
                    //return -1;
                }
                //\u628a\u7a97\u53e3\u8bbe\u7f6e\u4e3a\u9690\u85cf\u5e94\u53ef\u4ee5\u51cf\u5c11\u529f\u8017
                ui_status = native_window_set_usage(nativeWindow.get(),
                                                    GRALLOC_USAGE_SW_READ_OFTEN
           |
                                                    GRALLOC_USAGE_SW_WRITE_OFTEN
           |
                                                    GRALLOC_USAGE_HW_TEXTURE |
                                                    GRALLOC_USAGE_EXTERNAL_DISP);

                if (ui_status != android::OK) {
                    E("native_window_set_usage error");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }
                ui_status = native_window_set_scaling_mode(nativeWindow.get(),
           NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW);
                if (ui_status != android::OK) {
                    E("native_window_set_scaling_mode error");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }

                ui_status =
           native_window_set_buffers_dimensions(nativeWindow.get(),
           g_sensor_width, g_sensor_height);
                if (ui_status != android::OK) {
                    E("native_window_set_buffers_dimensions error");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }
                ui_status = native_window_set_buffers_format(nativeWindow.get(),
           g_surface_f);
                if (ui_status != android::OK) {
                    E("native_window_set_buffers_format error");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }

                ui_status = nativeWindow->query(nativeWindow.get(),
           NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &maxConsumerBuffers);
                if (ui_status != android::OK) {
                    E("Unable to query consumer undequeued");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }
                D("wants %d buffers", maxConsumerBuffers);
                int totalBuffers = maxConsumerBuffers + max_buffers;
                D("totalBuffers %d", totalBuffers);
                ui_status = native_window_set_buffer_count(nativeWindow.get(),
           totalBuffers);
                if (ui_status != android::OK) {
                    E("native_window_set_buffer_count error");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }
                D("rotate window");
                ui_status =
           native_window_set_buffers_transform(nativeWindow.get(), g_rotate |
           NATIVE_WINDOW_TRANSFORM_INVERSE_DISPLAY);
                if (ui_status != android::OK) {
                    E("native_window_set_buffers_transform error");
                    native_window_api_disconnect(nativeWindow.get(),
           NATIVE_WINDOW_API_CAMERA);
                    return -1;
                }

                anwBuffers = new ANativeWindowBuffer*[totalBuffers];*/

        _frameNum = 0;
        bufferIdx = 0;

        hidl_handle buffer_handle;
        hidl_handle buffer_handle_new[g_stream_count];
        sp<DeviceCb> cb = new DeviceCb(this);
        cb->mBufferId = 1;

        while (1) {
            // D("dequeueBufferr");
            /* surface code */
            //   ui_status =
            //   native_window_dequeue_buffer_and_wait(nativeWindow.get(),
            //   &anwBuffers[bufferIdx]);
            if (exit_camera()) {
                break;
            }
            request_buf.frame_number = _frameNum++;

            ::android::hardware::camera::common::V1_0::helper::CameraMetadata
                requestMeta;
            ::android::hardware::hidl_vec<uint8_t> requestSettings;
            camera_metadata_entry_t entry;
            int res;
            camera_metadata_t *metaBuffer;
            if (request_buf.frame_number == 0) {
                requestMeta.append(
                    reinterpret_cast<camera_metadata_t *>(settings.data()));
            }
            for (int i = 0; i < g_stream_count; i++) {
                ALOGE("request_buf.frame_number =%d,capture_frame=%d",
                      request_buf.frame_number, capture_frame);
                if (streamlist[i] == CAMERA_STREAM_TYPE_PREVIEW) {
                    sb[i].stream = &preview_stream;
                    sb[i].status = 0;
                    /* surface code */
                    // sb[0].buffer = &anwBuffers[bufferIdx]->handle;
                    sb[i].buffer = popBufferList(mLocalBufferList, 0);
                    sb[i].acquire_fence = -1;
                    sb[i].release_fence = -1;
                }
                if (capture_flag &&
                    streamlist[i] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                    if (snapshot_buffer.native_handle == NULL) {
                        allocateOne(g_streamConfig[snapshotStreamID].width *
                                        g_streamConfig[snapshotStreamID].height,
                                    1, &snapshot_buffer,
                                    0); // capture ion buffer ,size fixed??
                        ALOGE("alloc new memory ,width =%d ,height =%d",
                              g_streamConfig[snapshotStreamID].width,
                              g_streamConfig[snapshotStreamID].height);
                        sb[i].stream = &capture_stream;
                        sb[i].status = 0;
                        sb[i].buffer = &snapshot_buffer.native_handle;
                        sb[i].acquire_fence = -1;
                        sb[i].release_fence = -1;
                        map2(sb[i].buffer, &memory_gloable);
                    }
                }
                if (streamlist[i] == CAMERA_STREAM_TYPE_VIDEO) {
                    // alloc video buffer here
                    sb[i].stream = &video_stream;
                    sb[i].status = 0;
                    sb[i].buffer = popBufferList(mVideoBufferList, 0);
                    sb[i].acquire_fence = -1;
                    sb[i].release_fence = -1;
                }
                if (streamlist[i] == CAMERA_STREAM_TYPE_CALLBACK) {
                    // alloc callback buffer here
                    sb[i].stream = &callback_stream;
                    sb[i].status = 0;
                    sb[i].buffer = popBufferList(mCallbackBufferList, 0);
                    sb[i].acquire_fence = -1;
                    sb[i].release_fence = -1;
                }
            }
            // D("buffer_handle:%d",(*sb.buffer)->version);
            request_buf.output_buffers = (const camera3_stream_buffer_t *)&sb;
            if (capture_flag)
                request_buf.num_output_buffers = g_stream_count;
            else
                request_buf.num_output_buffers = g_stream_count - 1;

            // D("buffer_handle:%d",(*request_buf.output_buffers->buffer)->version);
            // //12
            // D("debug_zy:%s\uff0c%d,%p", __FUNCTION__,__LINE__,sb.buffer);

            // native_handle_t *buffers = (native_handle_t *)*sb[0].buffer ;
            // buffer_handle = buffers;
            StreamBuffer outputBuffer_new[g_stream_count];
            for (int i = 0; i < g_stream_count; i++) {
                if ((streamlist[i] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT &&
                     capture_flag) ||
                    streamlist[i] != CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                    buffer_handle_new[i] = (native_handle_t *)*sb[i].buffer;
                    outputBuffer_new[i] = {halStreamConfig.streams[i].id,
                                           bufferId,
                                           buffer_handle_new[i],
                                           BufferStatus::OK,
                                           nullptr,
                                           nullptr};
                    ALOGI("i=%d,halStreamConfig.streams[i].id =%d ", i,
                          halStreamConfig.streams[i].id);
                }
            }

            ::android::hardware::hidl_vec<StreamBuffer> outputBuffers;
            if (capture_flag) // add capture buffer
                outputBuffers.resize(g_stream_count);
            else
                outputBuffers.resize(g_stream_count - 1);

            uint32_t bufferindex = 0;
            for (int i = 0; i < g_stream_count; i++) {
                if ((streamlist[i] == CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT &&
                     capture_flag) ||
                    streamlist[i] != CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT) {
                    outputBuffers[bufferindex] = outputBuffer_new[i];
                    ALOGI("bufferindex =%d,i =%d,capture_flag=%d", bufferindex,
                          i, capture_flag);
                    bufferindex++;
                }
            }
            // reset capture flag and record capture num
            if (capture_flag) {
                capture_frame = _frameNum - 1;
                capture_flag = false;
            }

            if (true) { // update meta  when preview runing
                ALOGE("update metadata here");
                if (request_buf.frame_number ==
                    capture_frame + 1) // after capture ,set capture intent
                {
                    mode_test = static_cast<uint8_t>(
                        ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW);
                    updatemeta[0].tag = ANDROID_CONTROL_CAPTURE_INTENT;
                    updatemeta[0].data.u8 = &mode_test;
                    ALOGE("capture intent = %d,mode =%d",
                          updatemeta[0].data.u8[0], mode_test);
                    updatemeta[0].count = 1;
                    updatemeta[0].type = 0;
                    metacount = 1;
                }
                ALOGE("metacount =%d,local_meta_count=%d", metacount,
                      local_meta_count);
                if (metacount > 0) {
                    metaBuffer = requestMeta.release();
                    requestMeta = updatemetedata(metaBuffer);
                    //  requestMeta =
                    //  updatemetedata_v2(metaBuffer,local_meta,local_meta_count);
                    metaBuffer = requestMeta.release();
                    metacount = 0;
                    requestSettings.setToExternal(
                        reinterpret_cast<uint8_t *>(metaBuffer),
                        get_camera_metadata_size(metaBuffer), true);
                }
                if (local_meta_count > 0) {
                    ALOGE("update local_meta");
                    metaBuffer = requestMeta.release();
                    memcpy(updatemeta, local_meta,
                           5 * sizeof(camera_metadata_entry));
                    metacount = local_meta_count;
                    requestMeta = updatemetedata(metaBuffer);
                    local_meta_count = 0;
                    metacount = 0;
                    metaBuffer = requestMeta.release();
                    requestSettings.setToExternal(
                        reinterpret_cast<uint8_t *>(metaBuffer),
                        get_camera_metadata_size(metaBuffer), true);
                }
            }

            const StreamBuffer emptyInputBuffer = {
                -1, 0, nullptr, BufferStatus::ERROR, nullptr, nullptr};
            CaptureRequest request = {frameNumber, 0 /* fmqSettingsSize */,
                                      settings, emptyInputBuffer,
                                      outputBuffers};
            {
                std::unique_lock<std::mutex> l(mLock);
                inflightReq = {1, false, supportsPartialResults,
                               partialResultCount, resultQueue};
                //	ALOGI("debug_zy %s
                //,%d,frameNumber:%d,shutterTimestamp:%" PRId64
                //",haveResultMetadata:%d,numBuffersLeft:%zu",
                //__FUNCTION__,__LINE__,frameNumber,inflightReq.shutterTimestamp,inflightReq.haveResultMetadata,inflightReq.numBuffersLeft);
                mInflightMap.add(frameNumber, &inflightReq);
            }
            CaptureRequest captureRequests_buf = {
                frameNumber++, 0 /* fmqSettingsSize */, requestSettings,
                emptyInputBuffer, outputBuffers};
            std::vector<native_handle_t *> handlesCreated;
            cb->wrapAsHidlRequest(&request_buf, /*out*/ &captureRequests_buf,
                                  /*out*/ &handlesCreated);
            // captureRequests_buf.fmqSettingsSize = 1;

            Status status = Status::INTERNAL_ERROR;
            uint32_t numRequestProcessed = 0;
            hidl_vec<BufferCache> cachesToRemove;
            /*if(previewstop == false)
            {
                      ALOGE("frame num reach 200 ,flush and close camera");
                      break;
                      //session->flush();
                      // session->close();
                       previewstop = true;
            }*/
            if (previewstop || g_need_exit) {
                ALOGI("preview stop ,break prevew thread");
                break;
            }

            if (!previewstop) {
                ret = session->processCaptureRequest(
                    {captureRequests_buf}, cachesToRemove,
                    [&status,
                     &numRequestProcessed](auto s, // captureRequests {request}
                                           uint32_t n) {
                        status = s;
                        ALOGV("%s,%d,status: %d", __FUNCTION__, __LINE__,
                              static_cast<uint32_t>(status));
                        numRequestProcessed = n;
                    });
            }

            for (auto &handle : handlesCreated) {
                native_handle_delete(handle);
            }
            /* surface code */
            /*  bufferIdx++;
              if (bufferIdx >= totalBuffers) {
                  bufferIdx = 0;
              }*/
        }
#endif
        // Flush before waiting for request to complete.
        Return<Status> returnStatus = session->flush();

        {
            std::unique_lock<std::mutex> l(mLock);

            if (!inflightReq.errorCodeValid) {
            } else {
                switch (inflightReq.errorCode) {
                case ErrorCode::ERROR_REQUEST:
                case ErrorCode::ERROR_RESULT:
                case ErrorCode::ERROR_BUFFER:
                    // Expected
                    break;
                case ErrorCode::ERROR_DEVICE:
                default:
                    ALOGE("Unexpected error:%d",
                          static_cast<uint32_t>(inflightReq.errorCode));
                }
            }
            mInflightMap.clear();
            /* surface code */
            /*  for (int i = 0; i < totalBuffers; i++) {
                  //ALOGI("debug_zy totalBuffers:%d",totalBuffers);
                  ui_status = nativeWindow->cancelBuffer(nativeWindow.get(),
              anwBuffers[i], -1);
                  if (ui_status != android::OK) {
                      E("cancelBuffer error");
                  }
              }
              native_window_api_disconnect(nativeWindow.get(),
              NATIVE_WINDOW_API_CAMERA);
              surface.clear();
              composerClient->dispose();
              delete []anwBuffers;*/
            session->flush();
            ret = session->close();
        }
    }
    g_need_exit = true;
    char pid_name[50] = "cameraserver";
    long pid;
    pid = find_pid_by_name(pid_name);
    kill(pid, SIGKILL);
    ALOGE("pid =%d", pid);
    return 0;
}

int NativeCameraHidl::freeAllBuffers() // create preview running thread
{
    int ret = 0;
    /*free preview buffer*/
    for (int i = 0; i < max_buffers + 1; i++) {
        freeOneBuffer(&mLocalBuffer[i]);
        freeOneBuffer(&mCallbackBuffer[i]);
        freeOneBuffer(&mVideoBuffer[i]);
    }
    /*free snapshot buffer */
    freeOneBuffer(&snapshot_buffer);
    free(g_streamConfig);
    return ret;
}

void NativeCameraHidl::freeOneBuffer(new_mem_t *buffer) {
    if (buffer != NULL) {
        if (buffer->graphicBuffer != NULL) {
            buffer->graphicBuffer.clear();
            buffer->graphicBuffer = NULL;
            buffer->phy_addr = NULL;
            buffer->vir_addr = NULL;
        }
        buffer->native_handle = NULL;
    } else {
        ALOGI("Not allocated, No need to free");
    }
    ALOGD("X");
    return;
}

ModuleWrapperHAL::ModuleWrapperHAL() {
    pVec_Result = gMap_Result[THIS_MODULE_NAME];
    IT_LOGD("module_hal succeeds in get Vec_Result");
}

ModuleWrapperHAL::~ModuleWrapperHAL() { IT_LOGD(""); }

int ModuleWrapperHAL::SetUp() {
    int ret = IT_OK;
    return ret;
}

int ModuleWrapperHAL::TearDown() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}

int ModuleWrapperHAL::Run(IParseJson *Json2) {
    int ret = IT_OK;
    HalCaseComm *_json2 = (HalCaseComm *)Json2;

    int g_rotate = 0;
    int mirror = 0;

    g_sensor_width = 1440;
    g_sensor_height = 1080;
    g_rotate = 0;
    mirror = 0;

    int status = 0;
    int g_camera_id = _json2->m_cameraID;
    status = native_camera.transferMetaData(_json2);

    ALOGI("metacount =%d", metacount);

    // if (_json2->getID() == 0 && !g_first_open) {
    if (strcmp(_json2->m_funcName.c_str(), "opencamera") == 0 &&
        !g_first_open) {
        native_camera.SetupSprdTags();
        PERFORMANCE_MONITOR("nativecamerainit",
                             status = native_camera.nativecamerainit(g_rotate, mirror));
        PERFORMANCE_MONITOR("openNativeCamera",
                             native_camera.openNativeCamera(g_camera_id));
        g_first_open = true;
        resultData_t openResult;
        openResult.available = 1;
        openResult.funcID = _json2->getID();
        openResult.funcName = _json2->m_funcName;
        pVec_Result->push_back(openResult);
    }

    if (strcmp(_json2->m_funcName.c_str(), "startpreview") == 0 &&
        !g_first_preview) {
        g_stream_count = 0;
        g_streamConfig = (streamconfig *)malloc(8 * sizeof(streamconfig));
        for (auto &stream : _json2->m_StreamArr) {
            g_streamConfig[g_stream_count].stream_type =
                (camera_stream_type_t)stream->s_type;
            g_streamConfig[g_stream_count].width = stream->s_width;
            g_streamConfig[g_stream_count].height = stream->s_height;
            g_streamConfig[g_stream_count].pixel =
                android_pixel_format_t(stream->s_format);
            ALOGI("stream_type =%d,width =%d,height =%d,pixel "
                  "=%d,g_stream_count=%d",
                  g_streamConfig[g_stream_count].stream_type,
                  g_streamConfig[g_stream_count].width = stream->s_width,
                  g_streamConfig[g_stream_count].height,
                  g_streamConfig[g_stream_count].pixel, g_stream_count);
            g_stream_count++;
        }
        native_camera.previewstop = 0;
        native_camera.startpreview();
        g_first_preview = true;
        while (!g_need_exit) // result check
        {
            if ((g_result[CAMERA_START_PREVIEW] &&
                 strcmp(_json2->m_funcName.c_str(), "startpreview") == 0)) {
                D("start preview success ,return");
                break;
            }
        }
        resultData previewResult;
        previewResult.available = 1;
        previewResult.ret = 1;
        previewResult.funcID = _json2->getID();
        previewResult.funcName = _json2->m_funcName;
        pVec_Result = gMap_Result[THIS_MODULE_NAME];
        pVec_Result->push_back(previewResult);
    }

    if (strcmp(_json2->m_funcName.c_str(), "takesnapshot") == 0) {
        capture_flag = true;
        while (!g_need_exit) // result check
        {
            if ((g_result[CAMERA_TAKE_PIC] &&
                 strcmp(_json2->m_funcName.c_str(), "takesnapshot") == 0)) {
                D("take pic success ,return");
                break;
            }
        }
        resultData snapshotResult;
        snapshotResult.available = 1;
        snapshotResult.funcID = _json2->getID();
        snapshotResult.funcName = _json2->m_funcName;
        snapshotResult.data[snapshotResult.funcName] = memory_gloable.addr_vir;
        pVec_Result->push_back(snapshotResult);
    }

    if (strcmp(_json2->m_funcName.c_str(), "closecamera") == 0) {
        // close camera
        native_camera.previewstop = 1;
        ALOGI("stop preview and closecamera");
        while (!g_need_exit) // result check
        {
            // if((g_result[CAMERA_CLOSE_CAMERA] && _json2->getID() == 3))
            if ((g_result[CAMERA_CLOSE_CAMERA])) {
                D("exit camera");
                break;
            }
        }
        ALOGI("exit success,g_result[CAMERA_CLOSE_CAMERA]=%d,g_need_exit=%d",
              g_result[CAMERA_CLOSE_CAMERA], g_need_exit);
        g_first_open = false;
        g_first_preview = false;
        g_need_exit = false;
        native_camera.streamlist.clear();
        for (int i = 0; i < CAMERA_TEST_MAX; i++) {
            g_result[i] = false;
        }

        resultData CloseResult;
        PERFORMANCE_MONITOR("freeAllBuffers",
                             native_camera.freeAllBuffers());
        CloseResult.available = 1;
        CloseResult.funcID = _json2->getID();
        CloseResult.funcName = _json2->m_funcName;
        pVec_Result->push_back(CloseResult);
    }
    if (status != 0) {
        return -1;
    }
    return ret;
}

int ModuleWrapperHAL::InitOps() {
    int ret = IT_OK;
    IT_LOGD("");
    return ret;
}
