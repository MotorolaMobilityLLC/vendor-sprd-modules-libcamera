#define LOG_TAG "SimpleMultiCamera"

#include <log/log.h>

#include <SprdCamera3Factory.h>
#include <SprdCamera3HWI.h>
#include <SprdCamera3Setting.h>
#include "SimpleMultiCamera.h"

using namespace sprdcamera;
using namespace std;

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
    return SprdCamera3HWI::initialize(mDevice, callback_ops);
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
    return SprdCamera3HWI::process_capture_request(mDevice, request);
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
            delete[] mConflictingDevices;
    }

    delete[] mConflictingDevices;
    mConflictingDevicesLength = 0;
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

    return shared_ptr<ICameraBase>(
        new SimpleMultiCamera(cfg->getCameraId(), sensorIds, physicalIds));
}
