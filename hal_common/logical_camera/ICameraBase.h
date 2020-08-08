#pragma once

#include <hardware/camera3.h>

namespace sprdcamera {

struct ICameraBase {
    virtual int openCamera(hw_device_t **dev) = 0;
    virtual int getCameraInfo(camera_info_t *info) = 0;
    virtual int isStreamCombinationSupported(
        const camera_stream_combination_t *streams) = 0;
    virtual int
    getPhysicalCameraInfoForId(int id, camera_metadata_t **static_metadata) = 0;

    struct CameraClosedListener {
        virtual void onCameraClosed(int camera_id) = 0;
        virtual ~CameraClosedListener() = 0;
    };

    virtual void setCameraClosedListener(CameraClosedListener *listener) = 0;
    virtual ~ICameraBase() = 0;
};

inline ICameraBase::~ICameraBase() {}

inline ICameraBase::CameraClosedListener::~CameraClosedListener() {}
}
