#pragma once

#include <linux/errno.h>
#include <Camera3DeviceBase.h>
#include <ICameraBase.h>

namespace sprdcamera {

/*
 * A camera 3.2 device
 */
class CameraDevice_3_2 : public Camera3DeviceBase, public ICameraBase {
  protected:
    CameraDevice_3_2(int cameraId);
    virtual ~CameraDevice_3_2();

    const int mCameraId;

  private:
    int openCamera(hw_device_t **dev) final;
    int close() final;
    void
    setCameraClosedListener(ICameraBase::CameraClosedListener *listener) final;

    virtual int openCameraDevice() = 0;
    virtual int closeCameraDevice() = 0;

    ICameraBase::CameraClosedListener *mListener;

    /* filter out unavailable functions */
    int register_stream_buffers(const camera3_stream_buffer_set_t *) final {
        return -ENODEV;
    }

    void get_metadata_vendor_tag_ops(vendor_tag_query_ops_t *) final {}

    void signal_stream_flush(uint32_t, const camera3_stream_t *const *) final {}

    int is_reconfiguration_required(const camera_metadata_t *,
                                    const camera_metadata_t *) final {
        return -ENODEV;
    }

    int getPhysicalCameraInfoForId(int id,
                                   camera_metadata_t **static_metadata) final {
        return -EINVAL;
    }
};
}
