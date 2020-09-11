#pragma once

#include <hardware/camera3.h>

namespace sprdcamera {

/*
 * A wrapper to camera3_device_t
 */
class Camera3DeviceBase : public camera3_device_t, public camera3_device_ops_t {
  protected:
    Camera3DeviceBase();
    virtual ~Camera3DeviceBase();

  private:
    /* hw_device_t methods */
    virtual int close() = 0;

    /* camera3_device_ops_t methods */
    virtual int initialize(const camera3_callback_ops_t *callback_ops) = 0;
    virtual int
    configure_streams(camera3_stream_configuration_t *stream_list) = 0;
    virtual int
    register_stream_buffers(const camera3_stream_buffer_set_t *buffer_set) = 0;
    virtual const camera_metadata_t *
    construct_default_request_settings(int type) = 0;
    virtual int process_capture_request(camera3_capture_request_t *request) = 0;
    virtual void get_metadata_vendor_tag_ops(vendor_tag_query_ops_t *ops) = 0;
    virtual void dump(int fd) = 0;
    virtual int flush() = 0;
    virtual void
    signal_stream_flush(uint32_t num_streams,
                        const camera3_stream_t *const *streams) = 0;
    virtual int is_reconfiguration_required(
        const camera_metadata_t *old_session_params,
        const camera_metadata_t *new_session_params) = 0;

  private:
    static int close(hw_device_t *device);
    static int initialize(const struct camera3_device *,
                          const camera3_callback_ops_t *callback_ops);
    static int configure_streams(const struct camera3_device *,
                                 camera3_stream_configuration_t *stream_list);
    static int
    register_stream_buffers(const struct camera3_device *,
                            const camera3_stream_buffer_set_t *buffer_set);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *,
                                       camera3_capture_request_t *request);
    static void get_metadata_vendor_tag_ops(const struct camera3_device *,
                                            vendor_tag_query_ops_t *ops);
    static void dump(const struct camera3_device *, int fd);
    static int flush(const struct camera3_device *);
    static void signal_stream_flush(const struct camera3_device *,
                                    uint32_t num_streams,
                                    const camera3_stream_t *const *streams);
    static int
    is_reconfiguration_required(const struct camera3_device *,
                                const camera_metadata_t *old_session_params,
                                const camera_metadata_t *new_session_params);
};
}
