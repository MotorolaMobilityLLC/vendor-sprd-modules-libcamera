#define LOG_TAG "SingleCamera"

#include <log/log.h>
#include <SprdCamera3HWI.h>
#include "SingleCameraWrapper.h"

using namespace sprdcamera;
using namespace std;

shared_ptr<ICameraCreator> SingleCamera::getCreator() {
    return shared_ptr<ICameraCreator>(new SingleCameraCreator());
}

int SingleCamera::openCameraDevice() {
    SprdCamera3HWI *hwi = new SprdCamera3HWI(mSensorId);
    if (!hwi)
        return -ENODEV;

    hw_device_t *dev;
    int rc = hwi->openCamera(&dev);
    if (rc < 0) {
        ALOGE("fail to open single camera %d", mSensorId);
        /* just follow SprdCamera3Factory... */
        delete hwi;
        return rc;
    }

    mDev = reinterpret_cast<camera3_device_t *>(dev);
    return rc;
}

int SingleCamera::getCameraInfo(camera_info_t *info) {
    if (!info)
        return -EINVAL;

    /*
     * TODO
     * same in SprdCamera3Factory, consider wrapper it into
     * SprdCamera3Setting
     */
    camera_metadata_t *staticMetadata;
    int rc;

    SprdCamera3Setting::getSensorStaticInfo(mSensorId);
    SprdCamera3Setting::initDefaultParameters(mSensorId);
    rc = SprdCamera3Setting::getStaticMetadata(mSensorId, &staticMetadata);
    if (rc < 0) {
        ALOGE("fail to get static metadata for %d", mSensorId);
        return rc;
    }

    SprdCamera3Setting::getCameraInfo(mSensorId, info);
    info->device_version = CAMERA_DEVICE_API_VERSION_3_2;
    info->static_camera_characteristics = staticMetadata;
    info->conflicting_devices_length = 0;

    return rc;
}

SingleCamera::SingleCamera(int cameraId, int sensorId)
    : CameraDevice_3_2(cameraId), mSensorId(sensorId), mDev(nullptr) {
    ALOGI("create SingleCamera instance with camera ID %d, sensor ID %d",
          mCameraId, mSensorId);
}

SingleCamera::~SingleCamera() {}

int SingleCamera::closeCameraDevice() {
    return SprdCamera3HWI::close_camera_device(&mDev->common);
}

int SingleCamera::initialize(const camera3_callback_ops_t *callback_ops) {
    return SprdCamera3HWI::initialize(mDev, callback_ops);
}

int SingleCamera::configure_streams(
    camera3_stream_configuration_t *stream_list) {
    return SprdCamera3HWI::configure_streams(mDev, stream_list);
}

const camera_metadata_t *
SingleCamera::construct_default_request_settings(int type) {
    return SprdCamera3HWI::construct_default_request_settings(mDev, type);
}

int SingleCamera::process_capture_request(camera3_capture_request_t *request) {
    return SprdCamera3HWI::process_capture_request(mDev, request);
}

void SingleCamera::dump(int fd) { SprdCamera3HWI::dump(mDev, fd); }

int SingleCamera::flush() { return SprdCamera3HWI::flush(mDev); }

int SingleCamera::isStreamCombinationSupported(
    const camera_stream_combination_t *streams) {
#if NEEDED_FOR_testSharedSurfaceSwitch
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
#else
    return -EINVAL;
#endif
}

shared_ptr<ICameraBase> SingleCamera::SingleCameraCreator::createInstance(
    shared_ptr<Configurator> cfg) {
    if (!cfg)
        return nullptr;

    return shared_ptr<ICameraBase>(
        new SingleCamera(cfg->getCameraId(), cfg->getSensorIds()[0]));
}
