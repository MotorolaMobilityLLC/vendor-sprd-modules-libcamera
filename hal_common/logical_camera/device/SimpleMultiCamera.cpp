#define LOG_TAG "SimpleMultiCamera"

#include <log/log.h>

#include <SprdCamera3Factory.h>
#include <SprdCamera3HWI.h>
#include <SprdCamera3Setting.h>
#include "SimpleMultiCamera.h"
#include <math.h>

#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

using namespace sprdcamera;
using namespace std;

const float EPSINON = 0.0001f;
SimpleMultiCamera *mSimpleMultiCamera = NULL;
camera3_callback_ops SimpleMultiCamera::callback_ops_main = {
    .process_capture_result = SimpleMultiCamera::process_capture_result,
    .notify = SimpleMultiCamera::notify};
shared_ptr<ICameraCreator> SimpleMultiCamera::getCreator() {
    return shared_ptr<ICameraCreator>(new SimpleMultiCameraCreator());
}

int SimpleMultiCamera::openCameraDevice() {
    SprdCamera3HWI *hwi = new SprdCamera3HWI(mMasterId);
    hw_device_t *device = nullptr;
    int ret = hwi->openCamera(&device);
    if (ret < 0) {
        ALOGE("fail to open camera %d, ret %d", mMasterId, ret);
        delete hwi;
        return -ENODEV;
    }

    /* simply save dev here since hwi is managed by SprdCamera3HWI */
    mDevice = reinterpret_cast<camera3_device_t *>(device);

    ALOGI("camera %d open finished", mCameraId);

    return 0;
}

int SimpleMultiCamera::getCameraInfo(camera_info_t *info) {
    if (!info)
        return -EINVAL;

    constructCharacteristics();
    constructConflictDevices();

    /* camera info follow master camera except those changed in constructor */
    SprdCamera3Setting::getCameraInfo(mMasterId, info);
    info->device_version = common.version;
    info->static_camera_characteristics = mStaticCameraCharacteristics;
    info->resource_cost = 100;
    info->conflicting_devices = mConflictingDevices;
    info->conflicting_devices_length = mConflictingDevicesLength;

    return 0;
}

int SimpleMultiCamera::isStreamCombinationSupported(
    const camera_stream_combination_t *streams) {
    /* don't support any physical stream for now */
    if (!streams)
        return -EINVAL;

    for (uint32_t i = 0; i < streams->num_streams; i++) {
        camera_stream_t *s = &streams->streams[i];
        if (s->physical_camera_id && strlen(s->physical_camera_id)) {
            ALOGE("physical stream is not supported for %s",
                  s->physical_camera_id);
            return -EINVAL;
        }
    }

    return 0;
}

int SimpleMultiCamera::initialize(const camera3_callback_ops_t *callback_ops) {
    mCallbackOps = callback_ops;
    return SprdCamera3HWI::initialize(mDevice, &callback_ops_main);
}

int SimpleMultiCamera::configure_streams(
    camera3_stream_configuration_t *stream_list) {
    /* don't support any physical stream for now */
    if (!stream_list)
        return -EINVAL;

    for (uint32_t i = 0; i < stream_list->num_streams; i++) {
        camera3_stream_t *stream = stream_list->streams[i];
        if (stream->physical_camera_id && strlen(stream->physical_camera_id)) {
            ALOGE("physical stream is not supported for %s",
                  stream->physical_camera_id);
            return -EINVAL;
        }
    }

    return SprdCamera3HWI::configure_streams(mDevice, stream_list);
}

const camera_metadata_t *
SimpleMultiCamera::construct_default_request_settings(int type) {
    if (mSettingMap.find(type) == mSettingMap.cend()) {
        auto origin =
            SprdCamera3HWI::construct_default_request_settings(mDevice, type);
        if (!origin)
            return nullptr;

        CameraMetadata data = clone_camera_metadata(origin);

        {
            /* mActiveArrayWidth/Height is ready after getCameraInfo call */
            int32_t cropRegion[4] = {0, 0, mActiveArrayWidth,
                                     mActiveArrayHeight};
            data.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);
            ALOGV("update crop region to (%d, %d, %d, %d) for type %d",
                  cropRegion[0], cropRegion[1], cropRegion[2], cropRegion[3],
                  type);
        }

        mSettingMap[type] = data.release();
    }

    return mSettingMap[type];
}

int SimpleMultiCamera::process_capture_request(
    camera3_capture_request_t *request) {
    resetRequest(request);
    return SprdCamera3HWI::process_capture_request(mDevice, request);
}

void SimpleMultiCamera::process_capture_result(
    const struct camera3_callback_ops *ops,
    const camera3_capture_result_t *result) {
    if (result->output_buffers == NULL) {
        camera3_capture_result_t curResult;
        memcpy(&curResult, result, sizeof(camera3_capture_result_t));
        Mutex::Autolock l(mSimpleMultiCamera->mMetaDataLock);
        List<metadata_t>::iterator itor =
            mSimpleMultiCamera->mReqMetadataList.begin();
        while (itor != mSimpleMultiCamera->mReqMetadataList.end()) {
            if (itor->frame_number == result->frame_number) {
                CameraMetadata curMetadata =
                    clone_camera_metadata(result->result);
                HAL_LOGV("result metadata %d", result->frame_number);
                mSimpleMultiCamera->setResultMetadata(curMetadata, itor->meta);
                curResult.result = curMetadata.release();
                mSimpleMultiCamera->mReqMetadataList.erase(itor);
                break;
            }
            itor++;
        }
        mSimpleMultiCamera->mCallbackOps->process_capture_result(
            mSimpleMultiCamera->mCallbackOps, &curResult);
    } else {
        mSimpleMultiCamera->mCallbackOps->process_capture_result(
            mSimpleMultiCamera->mCallbackOps, result);
    }
}

void SimpleMultiCamera::notify(const struct camera3_callback_ops *ops,
                               const camera3_notify_msg_t *msg) {
    mSimpleMultiCamera->mCallbackOps->notify(mSimpleMultiCamera->mCallbackOps,
                                             msg);
}

void SimpleMultiCamera::dump(int fd) { SprdCamera3HWI::dump(mDevice, fd); }

int SimpleMultiCamera::flush() { return SprdCamera3HWI::flush(mDevice); }

int SimpleMultiCamera::closeCameraDevice() {
    ALOGI("closing camera %d", mCameraId);
    return SprdCamera3HWI::close_camera_device(&mDevice->common);
}

SimpleMultiCamera::~SimpleMultiCamera() {
    destructConflictDevices();
    destructCharacteristics();
}

SimpleMultiCamera::SimpleMultiCamera(int cameraId, const vector<int> &sensorIds,
                                     const vector<int> &physicalIds)
    : CameraDevice_3_5(cameraId), mSensorIds(sensorIds),
      mPhysicalIds(physicalIds), mMasterId(mSensorIds[0]), mDevice(nullptr),
      mStaticCameraCharacteristics(nullptr), mConflictingDevicesLength(0),
      mConflictingDevices(nullptr), mActiveArrayWidth(0),
      mActiveArrayHeight(0) {
    mReqMetadataList.clear();

    ALOGI("create SimpleMultiCamera instance with camera ID %d, master sensor "
          "ID %d",
          mCameraId, mMasterId);
}

void SimpleMultiCamera::constructCharacteristics() {
    if (mStaticCameraCharacteristics)
        return;

    camera_metadata_t *default_metadata = nullptr;
    SprdCamera3Setting::getSensorStaticInfo(mMasterId);
    SprdCamera3Setting::initDefaultParameters(mMasterId);
    SprdCamera3Setting::getStaticMetadata(mMasterId, &default_metadata);
    CameraMetadata defaultMetadata = clone_camera_metadata(default_metadata);

    {
        /* save android.sensor.info.activeArraySize for later use */
        camera_metadata_entry_t entry =
            defaultMetadata.find(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE);
        if (entry.count < 4) {
            ALOGE("malformed android.sensor.info.activeArraySize has %zu count",
                  entry.count);
            return;
        }
        mActiveArrayWidth = entry.data.i32[2];
        mActiveArrayHeight = entry.data.i32[3];
        HAL_LOGV("mActiveArraySize %d, %d", mActiveArrayWidth,
                 mActiveArrayHeight);
    }
    {
        bzero(mMaxRegions, sizeof(mMaxRegions));
        camera_metadata_entry_t entry =
            defaultMetadata.find(ANDROID_CONTROL_MAX_REGIONS);
        if (entry.count < 3) {
            ALOGE("malformed android.control.maxRegions has %zu count",
                  entry.count);
            return;
        }
        for (int i = 0; i < 3; i++)
            mMaxRegions[i] = entry.data.i32[i];
    }

    {
        /* don't support high speed here */
        defaultMetadata.erase(
            ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS);
    }

    {
        /* don't support AE lock */
        uint8_t available = ANDROID_CONTROL_AE_LOCK_AVAILABLE_FALSE;
        defaultMetadata.update(ANDROID_CONTROL_AE_LOCK_AVAILABLE, &available,
                               1);
    }

    {
        /* don't support AWB lock */
        uint8_t available = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_FALSE;
        defaultMetadata.update(ANDROID_CONTROL_AWB_LOCK_AVAILABLE, &available,
                               1);
    }

    {
        uint8_t availableModes[3] = {ANDROID_CONTROL_MODE_OFF,
                                     ANDROID_CONTROL_MODE_AUTO,
                                     ANDROID_CONTROL_MODE_USE_SCENE_MODE};
        defaultMetadata.update(ANDROID_CONTROL_AVAILABLE_MODES, availableModes,
                               3);
    }

    {
        /* available capabilities */
        uint8_t capabilities[2] = {
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA};
        defaultMetadata.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                               capabilities, 2);
    }

    {
        /* available characteristics keys */
        camera_metadata_entry_t entry = defaultMetadata.find(
            ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS);
        vector<int32_t> keys(entry.data.i32, entry.data.i32 + entry.count);
        keys.push_back(ANDROID_CONTROL_AVAILABLE_MODES);
        keys.push_back(ANDROID_CONTROL_AE_LOCK_AVAILABLE);
        keys.push_back(ANDROID_CONTROL_AWB_LOCK_AVAILABLE);
        keys.push_back(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS);
        keys.push_back(ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE);
        defaultMetadata.update(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
                               keys.data(), keys.size());
    }

    {
        /* available request keys */
        camera_metadata_entry_t entry =
            defaultMetadata.find(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS);
        vector<int32_t> keys(entry.data.i32, entry.data.i32 + entry.count);
        keys.push_back(ANDROID_CONTROL_ZOOM_RATIO);
        defaultMetadata.update(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS,
                               keys.data(), keys.size());
    }

    {
        /* zoom ratio range */
        camera_metadata_entry_t entry =
            defaultMetadata.find(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM);
        if (entry.count < 1) {
            ALOGE("missing ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM for "
                  "camera %d",
                  mMasterId);
            return;
        }
        float range[2] = {1.0f, entry.data.f[0]};
        defaultMetadata.update(ANDROID_CONTROL_ZOOM_RATIO_RANGE, range, 2);
    }

    {
        /* physical ids */
        uint8_t *physicalIds = new uint8_t[mPhysicalIds.size() * 2];
        for (size_t i = 0; i < mPhysicalIds.size(); i++) {
            /* TODO currently physical ID is 0~9 only */
            physicalIds[i * 2] = '0' + mPhysicalIds[i];
            physicalIds[i * 2 + 1] = '\0';
        }
        defaultMetadata.update(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS,
                               physicalIds, mPhysicalIds.size() * 2);
        delete[] physicalIds;
    }

    {
        /* sensor sync type */
        uint8_t type = ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE_CALIBRATED;
        defaultMetadata.update(ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE,
                               &type, 1);
    }

    // TODO update other keys
    mStaticCameraCharacteristics = defaultMetadata.release();
}

void SimpleMultiCamera::destructCharacteristics() {
    if (!mStaticCameraCharacteristics)
        return;

    free_camera_metadata(mStaticCameraCharacteristics);
    mStaticCameraCharacteristics = nullptr;
}

void SimpleMultiCamera::constructConflictDevices() {
    if (mConflictingDevicesLength)
        return;

    int numberOfCameras = SprdCamera3Factory::get_number_of_cameras();
    if (numberOfCameras < 1) {
        ALOGE("invalid number of cameras %d", numberOfCameras);
        mConflictingDevicesLength = 0;
        mConflictingDevices = nullptr;
        return;
    }

    mConflictingDevicesLength = numberOfCameras - 1;
    if (mConflictingDevicesLength) {
        mConflictingDevices = new char *[mConflictingDevicesLength];
        const int DEVICE_ID_LENGTH = 4;
        int id = 0;
        size_t i = 0;
        while (id < numberOfCameras && i < mConflictingDevicesLength) {
            if (id != mCameraId) {
                mConflictingDevices[i] = new char[DEVICE_ID_LENGTH + 1];
                int s = snprintf(mConflictingDevices[i], DEVICE_ID_LENGTH, "%d",
                                 id);
                if (s < 0) {
                    ALOGE("fail to format string, id %d, ret %d", id, s);
                    return;
                }

                mConflictingDevices[i][s] = '\0';
                i++;
            }
            id++;
        }

        ALOGD("make camera %d conflicting with other %zu device(s)", mCameraId,
              mConflictingDevicesLength);
    }
}

void SimpleMultiCamera::destructConflictDevices() {
    if (!mConflictingDevicesLength)
        return;

    for (size_t i = 0; i < mConflictingDevicesLength; i++) {
        /* to avoid null pointer in case constructing failed */
        if (mConflictingDevices[i])
            delete[] mConflictingDevices[i];
    }

    delete[] mConflictingDevices;
    mConflictingDevicesLength = 0;
}

void SimpleMultiCamera::resetRequest(camera3_capture_request_t *request) {
    CameraMetadata meta = clone_camera_metadata(request->settings);
    metadata_t curMetadata = {request->frame_number, meta};
    {
        Mutex::Autolock l(mMetaDataLock);
        mReqMetadataList.push_back(curMetadata);
    }
    float zoomRatio = 1.0f;
    if (meta.exists(ANDROID_CONTROL_ZOOM_RATIO)) {
        zoomRatio = meta.find(ANDROID_CONTROL_ZOOM_RATIO).data.f[0];
        HAL_LOGI("%d, request zoomRatio %f", request->frame_number, zoomRatio);
        img_region dstCropRegion = {0, 0, mActiveArrayWidth,
                                    mActiveArrayHeight};
        if (meta.exists(ANDROID_SCALER_CROP_REGION)) {
            img_region src = {0, 0, 0, 0};
            src.x = meta.find(ANDROID_SCALER_CROP_REGION).data.i32[0];
            src.y = meta.find(ANDROID_SCALER_CROP_REGION).data.i32[1];
            src.w = meta.find(ANDROID_SCALER_CROP_REGION).data.i32[2];
            src.h = meta.find(ANDROID_SCALER_CROP_REGION).data.i32[3];
            HAL_LOGV("%d,request0 cropRegion[%d, %d, %d, %d]",
                     request->frame_number, src.x, src.y, src.w, src.h);
            dstCropRegion = transferCropRegion(src, zoomRatio);
            int32_t cropRegion[4] = {dstCropRegion.x, dstCropRegion.y,
                                     dstCropRegion.w, dstCropRegion.h};
            HAL_LOGV("%d,request1 cropRegion[%d, %d, %d, %d]",
                     request->frame_number, dstCropRegion.x, dstCropRegion.y,
                     dstCropRegion.w, dstCropRegion.h);
            meta.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);
        }
        if (fabs(zoomRatio - 1.0f) > EPSINON) {
            if (meta.exists(ANDROID_CONTROL_AE_REGIONS) && mMaxRegions[0] > 0) {
                img_region src = {0, 0, 0, 0};
                src.x = meta.find(ANDROID_CONTROL_AE_REGIONS).data.i32[0];
                src.y = meta.find(ANDROID_CONTROL_AE_REGIONS).data.i32[1];
                src.w =
                    meta.find(ANDROID_CONTROL_AE_REGIONS).data.i32[2] - src.x;
                src.h =
                    meta.find(ANDROID_CONTROL_AE_REGIONS).data.i32[3] - src.y;
                int32_t weight =
                    meta.find(ANDROID_CONTROL_AE_REGIONS).data.i32[4];
                HAL_LOGV("%d,request0 aeRegions[%d, %d, %d, %d, %d]",
                         request->frame_number, src.x, src.y, src.w + src.x,
                         src.h + src.y, weight);
                img_region dst =
                    transfer3ARegions(src, dstCropRegion, zoomRatio);
                int32_t aeRegions[5] = {dst.x, dst.y, dst.w + dst.x,
                                        dst.h + dst.y, weight};
                HAL_LOGV("%d,request1 aeRegions[%d, %d, %d, %d, %d]",
                         request->frame_number, aeRegions[0], aeRegions[1],
                         aeRegions[2], aeRegions[3], aeRegions[4]);
                meta.update(ANDROID_CONTROL_AE_REGIONS, aeRegions, 5);
            }
            if (meta.exists(ANDROID_CONTROL_AWB_REGIONS) &&
                mMaxRegions[1] > 0) {
                img_region src = {0, 0, 0, 0};
                src.x = meta.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[0];
                src.y = meta.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[1];
                src.w =
                    meta.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[2] - src.x;
                src.h =
                    meta.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[3] - src.y;
                int32_t weight =
                    meta.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[4];
                HAL_LOGV("%d,request0 awbRegions[%d, %d, %d, %d, %d]",
                         request->frame_number, src.x, src.y, src.w + src.x,
                         src.h + src.y, weight);
                img_region dst =
                    transfer3ARegions(src, dstCropRegion, zoomRatio);
                int32_t awbRegions[5] = {dst.x, dst.y, dst.w + dst.x,
                                         dst.h + dst.y, weight};
                HAL_LOGV("%d,request1 awbRegions[%d, %d, %d, %d, %d]",
                         request->frame_number, awbRegions[0], awbRegions[1],
                         awbRegions[2], awbRegions[3], awbRegions[4]);
                meta.update(ANDROID_CONTROL_AWB_REGIONS, awbRegions, 5);
            }
            if (meta.exists(ANDROID_CONTROL_AF_REGIONS) && mMaxRegions[2] > 0) {
                img_region src = {0, 0, 0, 0};
                src.x = meta.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
                src.y = meta.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
                src.w =
                    meta.find(ANDROID_CONTROL_AF_REGIONS).data.i32[2] - src.x;
                src.h =
                    meta.find(ANDROID_CONTROL_AF_REGIONS).data.i32[3] - src.y;
                int32_t weight =
                    meta.find(ANDROID_CONTROL_AF_REGIONS).data.i32[4];
                HAL_LOGV("%d,request0 afRegions[%d, %d, %d, %d, %d]",
                         request->frame_number, src.x, src.y, src.w + src.x,
                         src.h + src.y, weight);
                img_region dst =
                    transfer3ARegions(src, dstCropRegion, zoomRatio);
                int32_t afRegions[5] = {dst.x, dst.y, dst.w + dst.x,
                                        dst.h + dst.y, weight};
                HAL_LOGV("%d,request1 afRegions[%d, %d, %d, %d, %d]",
                         request->frame_number, afRegions[0], afRegions[1],
                         afRegions[2], afRegions[3], afRegions[4]);
                meta.update(ANDROID_CONTROL_AF_REGIONS, afRegions, 5);
            }
        }
        request->settings = meta.release();
    }
}

img_region SimpleMultiCamera::transferCropRegion(img_region src, float ratio) {
    HAL_LOGV("src %d, %d, %d, %d", src.x, src.y, src.h, src.w);
    img_region dst = {0, 0, 0, 0};
    float w = (float)src.w / ratio;
    float h = (float)src.h / ratio;
    float x = (float)src.x + (src.w - w) / 2.0f;
    float y = (float)src.y + (src.h - h) / 2.0f;
    dst.w = (int32_t)w;
    dst.h = (int32_t)h;
    dst.x = (int32_t)x;
    dst.y = (int32_t)y;
    HAL_LOGV("dst %d, %d, %d, %d", dst.x, dst.y, dst.h, dst.w);
    return dst;
}

img_region SimpleMultiCamera::getImgRegion(img_region src, float ratio) {
    HAL_LOGV("src %d, %d, %d, %d", src.x, src.y, src.h, src.w);
    img_region dst = {0, 0, mActiveArrayWidth, mActiveArrayHeight};
    float w = src.w * ratio;
    float h = src.h * ratio;
    float x = (float)src.x - (float)(w - src.w) / 2.0f;
    float y = (float)src.y - (float)(h - src.h) / 2.0f;
    dst.w = MIN((int32_t)w, mActiveArrayWidth);
    dst.h = MIN((int32_t)h, mActiveArrayHeight);
    dst.x = MAX((int32_t)x, 0);
    dst.y = MAX((int32_t)y, 0);
    HAL_LOGV("dst %d, %d, %d, %d", dst.x, dst.y, dst.h, dst.w);
    return dst;
}
img_region SimpleMultiCamera::transfer3ARegions(img_region src,
                                                img_region cropRegion,
                                                float ratio) {
    img_region dst = {0, 0, 0, 0};
    float w = (float)src.w / ratio;
    float h = (float)src.h / ratio;
    float x = (float)src.x / ratio + (float)cropRegion.x;
    float y = (float)src.y / ratio + (float)cropRegion.y;
    dst.w = (int32_t)w;
    dst.h = (int32_t)h;
    dst.x = (int32_t)x;
    dst.y = (int32_t)y;
    return dst;
}

img_region SimpleMultiCamera::get3ARegions(img_region src,
                                           img_region cropRegion, float ratio) {
    img_region dst = {0, 0, 0, 0};
    float w = (float)src.w * ratio;
    float h = (float)src.h * ratio;
    float x = (float)(src.x - cropRegion.x) * ratio;
    float y = (float)(src.y - cropRegion.y) * ratio;
    dst.w = (int32_t)w;
    dst.h = (int32_t)h;
    dst.x = (int32_t)x;
    dst.y = (int32_t)y;
    return dst;
}

void SimpleMultiCamera::setResultMetadata(CameraMetadata &curMetadata,
                                          CameraMetadata meta) {
    float zoomRatio = 1.0f, curZoom = 1.0f;
    if (meta.exists(ANDROID_CONTROL_ZOOM_RATIO))
        zoomRatio = meta.find(ANDROID_CONTROL_ZOOM_RATIO).data.f[0];
    if (!curMetadata.exists(ANDROID_CONTROL_ZOOM_RATIO)) {
        camera_metadata_entry_t entry =
            curMetadata.find(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS);
        vector<int32_t> keys(entry.data.i32, entry.data.i32 + entry.count);
        keys.push_back(ANDROID_CONTROL_ZOOM_RATIO);
        curMetadata.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS, keys.data(),
                           keys.size());
        curMetadata.update(ANDROID_CONTROL_ZOOM_RATIO, &curZoom, 1);
    }
    if (fabs(zoomRatio - 1.0f) > EPSINON) {
        img_region srcCropRegion = {0, 0, mActiveArrayWidth,
                                    mActiveArrayHeight};
        if (curMetadata.exists(ANDROID_SCALER_CROP_REGION)) {
            srcCropRegion.x =
                curMetadata.find(ANDROID_SCALER_CROP_REGION).data.i32[0];
            srcCropRegion.y =
                curMetadata.find(ANDROID_SCALER_CROP_REGION).data.i32[1];
            srcCropRegion.w =
                curMetadata.find(ANDROID_SCALER_CROP_REGION).data.i32[2];
            srcCropRegion.h =
                curMetadata.find(ANDROID_SCALER_CROP_REGION).data.i32[3];
            float aspectRatio = (float)srcCropRegion.w / (float)srcCropRegion.h;
            float aspectRatioActive =
                (float)mActiveArrayWidth / (float)mActiveArrayHeight;
            if (aspectRatio > aspectRatioActive)
                curZoom = (float)mActiveArrayWidth / (float)srcCropRegion.w;
            else
                curZoom = (float)mActiveArrayHeight / (float)srcCropRegion.h;
            HAL_LOGD("result zoomRatio %f", curZoom);
            curMetadata.update(ANDROID_CONTROL_ZOOM_RATIO, &curZoom, 1);
            HAL_LOGV("HAL result cropRegion[%d, %d, %d, %d]", srcCropRegion.x,
                     srcCropRegion.y, srcCropRegion.w, srcCropRegion.h);
            img_region dst = getImgRegion(srcCropRegion, curZoom);
            HAL_LOGV("result cropRegion[%d, %d, %d, %d]", dst.x, dst.y, dst.w,
                     dst.h);
            int32_t cropRegion[4] = {dst.x, dst.y, dst.w, dst.h};
            curMetadata.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);
        }
        if (curMetadata.exists(ANDROID_CONTROL_AE_REGIONS) &&
            mSimpleMultiCamera->mMaxRegions[0] > 0) {
            img_region src = {0, 0, 0, 0};
            src.x = curMetadata.find(ANDROID_CONTROL_AE_REGIONS).data.i32[0];
            src.y = curMetadata.find(ANDROID_CONTROL_AE_REGIONS).data.i32[1];
            src.w = curMetadata.find(ANDROID_CONTROL_AE_REGIONS).data.i32[2] -
                    src.x;
            src.h = curMetadata.find(ANDROID_CONTROL_AE_REGIONS).data.i32[3] -
                    src.y;
            int32_t weight =
                curMetadata.find(ANDROID_CONTROL_AE_REGIONS).data.i32[4];
            HAL_LOGV("HAL result aeRegions[%d, %d, %d, %d, %d]", src.x, src.y,
                     src.w + src.x, src.h + src.y, weight);
            img_region dst = get3ARegions(src, srcCropRegion, curZoom);
            int32_t aeRegions[5] = {dst.x, dst.y, dst.w + dst.x, dst.h + dst.y,
                                    weight};
            HAL_LOGV("result aeRegions[%d, %d, %d, %d, %d]", aeRegions[0],
                     aeRegions[1], aeRegions[2], aeRegions[3], aeRegions[4]);
            curMetadata.update(ANDROID_CONTROL_AE_REGIONS, aeRegions, 5);
        }
        if (curMetadata.exists(ANDROID_CONTROL_AWB_REGIONS) &&
            mSimpleMultiCamera->mMaxRegions[1] > 0) {
            img_region src = {0, 0, 0, 0};
            src.x = curMetadata.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[0];
            src.y = curMetadata.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[1];
            src.w = curMetadata.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[2] -
                    src.x;
            src.h = curMetadata.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[3] -
                    src.y;
            int32_t weight =
                curMetadata.find(ANDROID_CONTROL_AWB_REGIONS).data.i32[4];
            HAL_LOGV("HAL result awbRegions[%d, %d, %d, %d, %d]", src.x, src.y,
                     src.w + src.x, src.h + src.y, weight);
            img_region dst = get3ARegions(src, srcCropRegion, curZoom);
            int32_t awbRegions[5] = {dst.x, dst.y, dst.w + dst.x, dst.h + dst.y,
                                     weight};
            HAL_LOGV("result awbRegions[%d, %d, %d, %d, %d]", awbRegions[0],
                     awbRegions[1], awbRegions[2], awbRegions[3],
                     awbRegions[4]);
            curMetadata.update(ANDROID_CONTROL_AWB_REGIONS, awbRegions, 5);
        }
        if (curMetadata.exists(ANDROID_CONTROL_AF_REGIONS) &&
            mSimpleMultiCamera->mMaxRegions[2] > 0) {
            img_region src = {0, 0, 0, 0};
            src.x = curMetadata.find(ANDROID_CONTROL_AF_REGIONS).data.i32[0];
            src.y = curMetadata.find(ANDROID_CONTROL_AF_REGIONS).data.i32[1];
            src.w = curMetadata.find(ANDROID_CONTROL_AF_REGIONS).data.i32[2] -
                    src.x;
            src.h = curMetadata.find(ANDROID_CONTROL_AF_REGIONS).data.i32[3] -
                    src.y;
            int32_t weight =
                curMetadata.find(ANDROID_CONTROL_AF_REGIONS).data.i32[4];
            HAL_LOGV("HAL result afRegions[%d, %d, %d, %d, %d]", src.x, src.y,
                     src.w + src.x, src.h + src.y, weight);
            img_region dst = get3ARegions(src, srcCropRegion, curZoom);
            int32_t afRegions[5] = {dst.x, dst.y, dst.w + dst.x, dst.h + dst.y,
                                    weight};
            HAL_LOGV("result afRegions[%d, %d, %d, %d, %d]", afRegions[0],
                     afRegions[1], afRegions[2], afRegions[3], afRegions[4]);
            curMetadata.update(ANDROID_CONTROL_AF_REGIONS, afRegions, 5);
        }
    }
}

shared_ptr<ICameraBase>
SimpleMultiCamera::SimpleMultiCameraCreator::createInstance(
    shared_ptr<Configurator> cfg) {
    if (!cfg) {
        ALOGE("Configurator is not found.");
        return nullptr;
    }

    auto sensorIds = cfg->getSensorIds();
    auto physicalIds = cfg->getPhysicalIds();
    if (sensorIds.size() != physicalIds.size() || sensorIds.size() < 2) {
        ALOGE("invalid sensor/physical ID size: %zu, %zu", sensorIds.size(),
              physicalIds.size());
        return nullptr;
    }
    mSimpleMultiCamera =
        new SimpleMultiCamera(cfg->getCameraId(), sensorIds, physicalIds);
    return shared_ptr<ICameraBase>(mSimpleMultiCamera);
}
