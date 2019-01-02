#include <cutils/properties.h>
#define LOG_TAG "cmr_isptool"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include "cmr_oem.h"
#include "isp_video.h"

cmr_int cmr_isp_simulation_proc(cmr_handle oem_handle,
                                struct snapshot_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct frm_info frame;
    char file_name[200];
    cmr_int read_size;
    cmr_u32 sec = 0;
    cmr_u32 usec = 0;
    char value[PROPERTY_VALUE_MAX];
    struct isptool_scene_param scene_param;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct img_frm isp_cap_raw = param_ptr->post_proc_setting.mem[0].cap_raw;

    cmr_sensor_update_isparm_from_file(cxt->sn_cxt.sensor_handle,
                                       cxt->camera_id);
    if (raw_filename[0]) {
        memcpy(value, raw_filename + strlen(CAMERA_DUMP_PATH) + 1,
               PROPERTY_VALUE_MAX);
    } else {
        property_get("debug.camera.isptool.raw.name", value, "none");
    }
    CMR_LOGI("parse file_name = %s", value);
    if (CMR_CAMERA_SUCCESS == camera_parse_raw_filename(value, &scene_param)) {
        sprintf(file_name, "%s%s", CAMERA_DUMP_PATH, value);
        //	4208X3120_gain_123_awbgain_r_1659_g_1024_b_1757_ct_4901_bv_64.mipi_raw

        CMR_LOGI(
            "w/h %d/%d, gain %d awb_r %d, awb_g %d awb_b %d ct %d bv %d glb %d",
            scene_param.width, scene_param.height, scene_param.gain,
            scene_param.awb_gain_r, scene_param.awb_gain_g,
            scene_param.awb_gain_b, scene_param.smart_ct, scene_param.smart_bv,
            scene_param.global_gain);

        ret = isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_TOOL_SET_SCENE_PARAM,
                        (void *)&scene_param);
        if (ret) {
            CMR_LOGE("failed isp ioctl %ld", ret);
        }

        if ((scene_param.width > isp_cap_raw.size.width) ||
            (scene_param.height > isp_cap_raw.size.height)) {
            ret = -CMR_CAMERA_INVALID_PARAM;
            CMR_LOGE("get scene param error");
            goto exit;
        }
        read_size = camera_get_data_from_file(
            file_name, CAM_IMG_FMT_BAYER_MIPI_RAW, scene_param.width, scene_param.height,
            &isp_cap_raw.addr_vir);
        CMR_LOGI("raw data read_size = %ld", read_size);
    }

    sem_wait(&cxt->access_sm);
    ret = cmr_grab_get_cap_time(cxt->grab_cxt.grab_handle, &sec, &usec);
    CMR_LOGI("cap time %d %d", sec, usec);
    sem_post(&cxt->access_sm);

    frame.channel_id = param_ptr->channel_id;
    frame.sec = sec;
    frame.usec = usec;
    frame.base = CMR_CAP0_ID_BASE;
    frame.frame_id = CMR_CAP0_ID_BASE;
    frame.fmt = CAM_IMG_FMT_BAYER_MIPI_RAW;
    frame.yaddr = isp_cap_raw.addr_phy.addr_y;
    frame.uaddr = isp_cap_raw.addr_phy.addr_u;
    frame.vaddr = isp_cap_raw.addr_phy.addr_v;
    frame.yaddr_vir = isp_cap_raw.addr_vir.addr_y;
    frame.uaddr_vir = isp_cap_raw.addr_vir.addr_u;
    frame.vaddr_vir = isp_cap_raw.addr_vir.addr_v;
    frame.fd = isp_cap_raw.fd;

    // call cmr_snapshot_receive_data for post-processing
    ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle,
                                    SNAPSHOT_EVT_CHANNEL_DONE, (void *)&frame);
exit:
    return ret;
}
