#include <linux/errno.h>

#include "Camera3DeviceBase.h"

using namespace sprdcamera;

Camera3DeviceBase::Camera3DeviceBase() {
    /* camera3_device_t */
    common.tag = HARDWARE_DEVICE_TAG;
    common.version = CAMERA_DEVICE_API_VERSION_CURRENT;
    common.module = NULL;
    common.close = close;
    ops = static_cast<camera3_device_ops_t *>(this);
    priv = this; // just in case

    /* camera3_device_ops_t */
    camera3_device_ops_t::initialize = initialize;
    camera3_device_ops_t::configure_streams = configure_streams;
    camera3_device_ops_t::register_stream_buffers = register_stream_buffers;
    camera3_device_ops_t::construct_default_request_settings =
        construct_default_request_settings;
    camera3_device_ops_t::process_capture_request = process_capture_request;
    camera3_device_ops_t::get_metadata_vendor_tag_ops =
        get_metadata_vendor_tag_ops;
    camera3_device_ops_t::dump = dump;
    camera3_device_ops_t::flush = flush;
    camera3_device_ops_t::signal_stream_flush = signal_stream_flush;
    camera3_device_ops_t::is_reconfiguration_required =
        is_reconfiguration_required;
}

Camera3DeviceBase::~Camera3DeviceBase() {}

int Camera3DeviceBase::close(hw_device_t *device) {
    if (!device)
        return -ENODEV;

    camera3_device_t *cam3dev = reinterpret_cast<camera3_device_t *>(device);
    Camera3DeviceBase *dev = static_cast<Camera3DeviceBase *>(cam3dev);

    return dev->close();
}

int Camera3DeviceBase::initialize(const struct camera3_device *cam3dev,
                                  const camera3_callback_ops_t *callback_ops) {
    if (!cam3dev)
        return -ENODEV;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->initialize(callback_ops);
}

int Camera3DeviceBase::configure_streams(
    const struct camera3_device *cam3dev,
    camera3_stream_configuration_t *stream_list) {
    if (!cam3dev)
        return -ENODEV;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->configure_streams(stream_list);
}

int Camera3DeviceBase::register_stream_buffers(
    const struct camera3_device *cam3dev,
    const camera3_stream_buffer_set_t *buffer_set) {
    if (!cam3dev)
        return -ENODEV;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->register_stream_buffers(buffer_set);
}

const camera_metadata_t *Camera3DeviceBase::construct_default_request_settings(
    const struct camera3_device *cam3dev, int type) {
    if (!cam3dev)
        return NULL;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->construct_default_request_settings(type);
}

int Camera3DeviceBase::process_capture_request(
    const struct camera3_device *cam3dev, camera3_capture_request_t *request) {
    if (!cam3dev)
        return -ENODEV;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->process_capture_request(request);
}

void Camera3DeviceBase::get_metadata_vendor_tag_ops(
    const struct camera3_device *cam3dev, vendor_tag_query_ops_t *ops) {
    if (!cam3dev)
        return;

    static_cast<Camera3DeviceBase *>(const_cast<camera3_device_t *>(cam3dev))
        ->get_metadata_vendor_tag_ops(ops);
}

void Camera3DeviceBase::dump(const struct camera3_device *cam3dev, int fd) {
    if (!cam3dev)
        return;

    static_cast<Camera3DeviceBase *>(const_cast<camera3_device_t *>(cam3dev))
        ->dump(fd);
}

int Camera3DeviceBase::flush(const struct camera3_device *cam3dev) {
    if (!cam3dev)
        return -ENODEV;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->flush();
}

void Camera3DeviceBase::signal_stream_flush(
    const struct camera3_device *cam3dev, uint32_t num_streams,
    const camera3_stream_t *const *streams) {
    if (!cam3dev)
        return;

    static_cast<Camera3DeviceBase *>(const_cast<camera3_device_t *>(cam3dev))
        ->signal_stream_flush(num_streams, streams);
}

int Camera3DeviceBase::is_reconfiguration_required(
    const struct camera3_device *cam3dev,
    const camera_metadata_t *old_session_params,
    const camera_metadata_t *new_session_params) {
    if (!cam3dev)
        return -ENODEV;

    return static_cast<Camera3DeviceBase *>(
               const_cast<camera3_device_t *>(cam3dev))
        ->is_reconfiguration_required(old_session_params, new_session_params);
}
