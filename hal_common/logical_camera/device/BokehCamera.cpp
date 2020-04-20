#define LOG_TAG "BokehCamera"

#include <log/log.h>
#include "BokehCamera.h"

using namespace sprdcamera;
using namespace std;

shared_ptr<ICameraCreator> BokehCamera::getCreator() {
    return shared_ptr<ICameraCreator>(new BokehCreator());
}

int BokehCamera::openCamera(hw_device_t **dev) {
    if (!dev)
        return -EINVAL;

    // TODO
    *dev = &common;
    return -ENODEV;
}

int BokehCamera::getCameraInfo(camera_info_t *info) {
    if (!info)
        return -EINVAL;

    // TODO
    return -ENODEV;
}

BokehCamera::BokehCamera() {}

BokehCamera::~BokehCamera() {}

int BokehCamera::close() { return -ENODEV; }

int BokehCamera::initialize(const camera3_callback_ops_t *callback_ops) {
    return -ENODEV;
}

int BokehCamera::configure_streams(
    camera3_stream_configuration_t *stream_list) {
    return -ENODEV;
}

const camera_metadata_t *
BokehCamera::construct_default_request_settings(int type) {
    return nullptr;
}

int BokehCamera::process_capture_request(camera3_capture_request_t *request) {
    return -ENODEV;
}

void BokehCamera::dump(int fd) {}

int BokehCamera::flush() { return -ENODEV; }

shared_ptr<ICameraBase>
BokehCamera::BokehCreator::createInstance(shared_ptr<Configurator> cfg) {
    // TODO
    return shared_ptr<ICameraBase>(new BokehCamera());
}
