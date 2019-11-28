#include <cutils/properties.h>
#define LOG_TAG "cmr_isptool"
#define ATRACE_TAG (ATRACE_TAG_CAMERA | ATRACE_TAG_HAL)
#include "cmr_oem.h"
#include "isp_video.h"
#include "isp_simulation.h"

cmr_int cmr_isp_simulation_proc(cmr_handle oem_handle,
                                struct snapshot_param *param_ptr) {
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct camera_context *cxt = (struct camera_context *)oem_handle;
    struct frm_info frame;
    char file_name[MAX_PATH_LEN] = {0};
    cmr_int read_size = 0;
    cmr_u32 sec = 0;
    cmr_u32 usec = 0;
    cmr_u32 image_index = 0;
    cmr_u32 is_loose = 0;
    struct isp_raw_image *image_info = NULL;
    char value[PROPERTY_VALUE_MAX];
    struct isptool_scene_param scene_param;
    struct isp_context *isp_cxt = &cxt->isp_cxt;
    struct img_frm isp_cap_raw = param_ptr->post_proc_setting.mem[0].cap_raw;

    cmr_sensor_update_isparm_from_file(cxt->sn_cxt.sensor_handle,
                                       cxt->camera_id);
    is_loose = cxt->sn_cxt.sensor_info.sn_interface.is_loose;

    image_index = isp_video_get_image_processed_index();
    image_info = isp_video_get_raw_images_info();
    if (image_info) {
        if (image_info->count) {
            strcpy(file_name, image_info->raw_image_ptr[image_index].filename);
            isp_sim_set_mipi_raw_file_name(
                image_info->raw_image_ptr[image_index].filename);
        }
        CMR_LOGI("parse file_name = %s", file_name);
        memset(&scene_param, 0, sizeof(struct isptool_scene_param));
        scene_param.width = image_info->raw_image_ptr[image_index].uWidth;
        scene_param.height = image_info->raw_image_ptr[image_index].uHeight;
        scene_param.awb_gain_r = image_info->raw_image_ptr[image_index].uRGain;
        scene_param.awb_gain_g = image_info->raw_image_ptr[image_index].uGGain;
        scene_param.awb_gain_b = image_info->raw_image_ptr[image_index].uBGain;
        scene_param.gain = image_info->raw_image_ptr[image_index].sTotalGain;
        scene_param.global_gain = image_info->raw_image_ptr[image_index].sdGain;
        scene_param.smart_bv = image_info->raw_image_ptr[image_index].sBv;
        scene_param.smart_ct = image_info->raw_image_ptr[image_index].uCt;

        CMR_LOGI("scene_param w[%d]h[%d] r_gain[%d] g_gain[%d] b_gain[%d] isp_cap_raw w[%d]h[%d]\n",
            scene_param.width,
            scene_param.height,
            scene_param.awb_gain_r,
            scene_param.awb_gain_g,
            scene_param.awb_gain_b,
            isp_cap_raw.size.width,
            isp_cap_raw.size.height);


        ret = isp_sim_set_scene_parm(&scene_param);
        if (ret) {
            CMR_LOGE("failed save scene parameter %ld", ret);
        }

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
        CMR_LOGD("is_loose =%d",is_loose);
        if (is_loose == ISP_RAW_HALF14) {
            read_size = camera_get_data_from_file(
                file_name, CAM_IMG_FMT_RAW14BIT, scene_param.width, scene_param.height,
                &isp_cap_raw.addr_vir);
        } else {
            read_size = camera_get_data_from_file(
                file_name, CAM_IMG_FMT_BAYER_MIPI_RAW, scene_param.width, scene_param.height,
                &isp_cap_raw.addr_vir);
        }
        CMR_LOGI("raw data read_size = %ld", read_size);
    } else {
        if (raw_filename[0]) {
            // only copy the filename without the path
            memcpy(value, raw_filename + strlen(CAMERA_DUMP_PATH) + 1,
                   PROPERTY_VALUE_MAX);
        } else {
            property_get("debug.camera.isptool.raw.name", value, "none");
        }
        CMR_LOGI("parse file_name = %s", value);
        if (CMR_CAMERA_SUCCESS ==
            camera_parse_raw_filename(value, &scene_param)) {
            sprintf(file_name, "%s%s", CAMERA_DUMP_PATH, value);
            //	4208X3120_gain_123_awbgain_r_1659_g_1024_b_1757_ct_4901_bv_64.mipi_raw

            CMR_LOGI("w/h %d/%d, gain %d awb_r %d, awb_g %d awb_b %d ct %d bv "
                     "%d glb %d",
                     scene_param.width, scene_param.height, scene_param.gain,
                     scene_param.awb_gain_r, scene_param.awb_gain_g,
                     scene_param.awb_gain_b, scene_param.smart_ct,
                     scene_param.smart_bv, scene_param.global_gain);

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
                file_name, IMG_DATA_TYPE_RAW, scene_param.width,
                scene_param.height, &isp_cap_raw.addr_vir);
            CMR_LOGI("raw data read_size = %ld", read_size);
        }
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
