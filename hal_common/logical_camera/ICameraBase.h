#pragma once

#include <hardware/camera3.h>

namespace sprdcamera {

struct ICameraBase {
    virtual int openCamera(hw_device_t **dev) = 0;
    virtual int getCameraInfo(camera_info_t *info) = 0;
    virtual int
    isStreamCombinationSupported(const camera_stream_combination_t *streams);
    virtual int getPhysicalCameraInfoForId(int id,
                                           camera_metadata_t **static_metadata);
    virtual ~ICameraBase() = 0;
};

inline ICameraBase::~ICameraBase() {}

inline int ICameraBase::isStreamCombinationSupported(
    const camera_stream_combination_t *streams) {
    return -ENOSYS;
}

inline int
ICameraBase::getPhysicalCameraInfoForId(int id,
                                        camera_metadata_t **static_metadata) {
    if (!static_metadata)
        return -EINVAL;

    *static_metadata = nullptr;
    return -ENODEV;
}
}
