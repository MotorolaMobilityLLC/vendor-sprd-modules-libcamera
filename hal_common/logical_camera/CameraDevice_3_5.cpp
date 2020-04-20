#include "CameraDevice_3_5.h"

using namespace sprdcamera;

CameraDevice_3_5::CameraDevice_3_5() {
    common.version = CAMERA_DEVICE_API_VERSION_3_5;
    /* deprecated or not needed in this version */
    camera3_device_ops_t::register_stream_buffers = NULL;
    camera3_device_ops_t::get_metadata_vendor_tag_ops = NULL;
    camera3_device_ops_t::signal_stream_flush = NULL;
    camera3_device_ops_t::is_reconfiguration_required = NULL;
}

CameraDevice_3_5::~CameraDevice_3_5() {}
