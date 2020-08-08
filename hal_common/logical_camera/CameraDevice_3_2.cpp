#include "CameraDevice_3_2.h"

using namespace sprdcamera;

CameraDevice_3_2::CameraDevice_3_2(int cameraId)
    : mCameraId(cameraId), mListener(nullptr) {
    common.version = CAMERA_DEVICE_API_VERSION_3_2;
    /* deprecated or not needed in this version */
    camera3_device_ops_t::register_stream_buffers = nullptr;
    camera3_device_ops_t::get_metadata_vendor_tag_ops = nullptr;
    camera3_device_ops_t::signal_stream_flush = nullptr;
    camera3_device_ops_t::is_reconfiguration_required = nullptr;
}

CameraDevice_3_2::~CameraDevice_3_2() {}

int CameraDevice_3_2::openCamera(hw_device_t **dev) {
    if (!dev)
        return -EINVAL;

    /* assign @dev here */
    *dev = &common;

    return openCameraDevice();
}

int CameraDevice_3_2::close() {
    int ret = closeCameraDevice();
    if (ret < 0)
        return ret;

    /* notify close here */
    if (mListener)
        mListener->onCameraClosed(mCameraId);

    return 0;
}

void CameraDevice_3_2::setCameraClosedListener(
    ICameraBase::CameraClosedListener *listener) {
    if (listener)
        mListener = listener;
}
