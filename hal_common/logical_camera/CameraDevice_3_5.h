#pragma once

#include <linux/errno.h>
#include <Camera3DeviceBase.h>
#include <ICameraBase.h>

namespace sprdcamera {

/*
 * A camera 3.5 device
 */
class CameraDevice_3_5 : public Camera3DeviceBase, public ICameraBase {
  public:
    int openCamera(hw_device_t **dev) final;

    /* reserve for logical multi-camera */
    int getPhysicalCameraInfoForId(int id,
                                   camera_metadata_t **static_metadata) {
        return -EINVAL;
    }

    void
    setCameraClosedListener(ICameraBase::CameraClosedListener *listener) final;

  protected:
    CameraDevice_3_5(int cameraId);
    virtual ~CameraDevice_3_5();

    const int mCameraId;

  private:
    virtual int openCameraDevice() = 0;

    virtual int closeCameraDevice() = 0;

    int close() final;

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

    ICameraBase::CameraClosedListener *mListener;
};
}
