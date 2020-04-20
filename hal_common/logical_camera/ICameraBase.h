#pragma once

#include <hardware/camera3.h>

namespace sprdcamera {

struct ICameraBase {
    virtual int openCamera(hw_device_t **dev) = 0;
    virtual int getCameraInfo(camera_info_t *info) = 0;
    virtual ~ICameraBase() = 0;
};

inline ICameraBase::~ICameraBase() {}
}
