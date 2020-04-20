#pragma once

#include <linux/errno.h>
#include <Camera3DeviceBase.h>

namespace sprdcamera {

class CameraDevice_3_2 : public Camera3DeviceBase {
  protected:
    CameraDevice_3_2();
    virtual ~CameraDevice_3_2();

    int register_stream_buffers(const camera3_stream_buffer_set_t *) final {
        return -ENODEV;
    }

    void get_metadata_vendor_tag_ops(vendor_tag_query_ops_t *) final {}

    void signal_stream_flush(uint32_t, const camera3_stream_t *const *) final {}

    int is_reconfiguration_required(const camera_metadata_t *,
                                    const camera_metadata_t *) final {
        return -ENODEV;
    }
};
}
