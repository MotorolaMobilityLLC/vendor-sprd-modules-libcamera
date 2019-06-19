/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "otp_parser_v10"
#include "otp_parser.h"

extern struct otp_parser_camera_info camera_otp_info[4];

static cmr_int _otp_parser_checksum(void *raw_data, cmr_uint offset,
                                    cmr_uint data_count,
                                    cmr_uint check_sum_offset) {
    cmr_int ret = OTP_PARSER_SUCCESS;
    cmr_uint i = 0, sum = 0;
    cmr_u8 *buf = (cmr_u8 *)raw_data;

    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if ((sum % 256) == buf[check_sum_offset]) {
        ret = OTP_PARSER_SUCCESS;
    } else {
        ret = OTP_PARSER_CHECKSUM_ERR;
    }

    CMR_LOGV("checksum_offset:%ld, calculate_checksum_value:%lu, "
             "buf_checksum_value:%d",
             check_sum_offset, sum % 256, buf[check_sum_offset]);
    return ret;
}

static cmr_uint _otp_parser_get_start_offset_v10(void *raw_data,
                                                 cmr_int camera_id) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_u8 *buffer = (cmr_u8 *)raw_data;
    char value[255];

    property_get("debug.parse.otp.open.camera", value, "0");
    if (atoi(value) == 0) {
        if (camera_otp_info[camera_id].has_parsered == 1) {
            CMR_LOGD("otp section addr and size has been parsered before");
            return rtn;
        }
    }

    // get otp start address and size of modules
    CMR_LOGV("camera_id=%ld", camera_id);
    if (0 == camera_id || 1 == camera_id) {
        // for master  or single camera

        // af
        camera_otp_info[camera_id].af_info.data_offset =
            (buffer[0x0000001D] << 8) | buffer[0x0000001C];
        camera_otp_info[camera_id].af_info.data_size = buffer[0x00000038];
        CMR_LOGV("af data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].af_info.data_offset,
                 camera_otp_info[camera_id].af_info.data_size);

        // awb
        camera_otp_info[camera_id].awb_info.data_offset =
            (buffer[0x0000001F] << 8) | buffer[0x0000001E];
        camera_otp_info[camera_id].awb_info.data_size = buffer[0x00000039];
        CMR_LOGV("awb data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].awb_info.data_offset,
                 camera_otp_info[camera_id].awb_info.data_size);

        // lsc
        camera_otp_info[camera_id].lsc_info.data_offset =
            (buffer[0x00000021] << 8) | buffer[0x00000020];
        camera_otp_info[camera_id].lsc_info.data_size =
            (buffer[0x0000003B] << 8) | buffer[0x0000003A];
        CMR_LOGV("lsc data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].lsc_info.data_offset,
                 camera_otp_info[camera_id].lsc_info.data_size);

        // pdaf_sprd
        camera_otp_info[camera_id].pdaf1_info.data_offset =
            (buffer[0x00000023] << 8) | buffer[0x00000022];
        camera_otp_info[camera_id].pdaf1_info.data_size =
            (buffer[0x0000003D] << 8) | buffer[0x0000003C];
        CMR_LOGV("pdaf_sprd_isp data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].pdaf1_info.data_offset,
                 camera_otp_info[camera_id].pdaf1_info.data_size);

        // pdaf spc from sensor vendor
        camera_otp_info[camera_id].pdaf2_info.data_offset =
            (buffer[0x00000025] << 8) | buffer[0x00000024];
        camera_otp_info[camera_id].pdaf2_info.data_size =
            (buffer[0x0000003F] << 8) | buffer[0x0000003E];
        CMR_LOGV("pdaf_sensor data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].pdaf2_info.data_offset,
                 camera_otp_info[camera_id].pdaf2_info.data_size);

        // ae
        camera_otp_info[camera_id].ae_info.data_offset =
            (buffer[0x00000027] << 8) | buffer[0x00000026];
        camera_otp_info[camera_id].ae_info.data_size = buffer[0x00000040];
        CMR_LOGV("ae data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].ae_info.data_offset,
                 camera_otp_info[camera_id].ae_info.data_size);

        // dual camera bokeh
        camera_otp_info[camera_id].bokeh_info.data_offset =
            (buffer[0x00000029] << 8) | buffer[0x00000028];
        camera_otp_info[camera_id].bokeh_info.data_size =
            (buffer[0x00000042] << 8) | buffer[0x00000041];
        CMR_LOGV("dualcam bokeh data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].bokeh_info.data_offset,
                 camera_otp_info[camera_id].bokeh_info.data_size);

        // cross talk for 4in1
        camera_otp_info[camera_id].xtalk_4in1_info.data_offset =
            (buffer[0x0000002B] << 8) | buffer[0x0000002A];
        camera_otp_info[camera_id].xtalk_4in1_info.data_size =
            (buffer[0x00000044] << 8) | buffer[0x00000043];
        CMR_LOGV("4in1 xtalk data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].xtalk_4in1_info.data_offset,
                 camera_otp_info[camera_id].xtalk_4in1_info.data_size);

        // dpc for 4in1
        camera_otp_info[camera_id].dpc_4in1_info.data_offset =
            (buffer[0x0000002D] << 8) | buffer[0x0000002C];
        camera_otp_info[camera_id].dpc_4in1_info.data_size =
            (buffer[0x00000046] << 8) | buffer[0x00000045];
        CMR_LOGV("4in1 dpc data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].dpc_4in1_info.data_offset,
                 camera_otp_info[camera_id].dpc_4in1_info.data_size);

    } else if (2 == camera_id || 3 == camera_id) {
        // for slave camera

        // af
        camera_otp_info[camera_id].af_info.data_offset =
            (buffer[0x0000002F] << 8) | buffer[0x0000002E];
        camera_otp_info[camera_id].af_info.data_size = buffer[0x00000047];
        CMR_LOGV("af data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].af_info.data_offset,
                 camera_otp_info[camera_id].af_info.data_size);

        // awb
        camera_otp_info[camera_id].awb_info.data_offset =
            (buffer[0x00000031] << 8) | buffer[0x00000030];
        camera_otp_info[camera_id].awb_info.data_size = buffer[0x00000048];
        CMR_LOGV("awb data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].awb_info.data_offset,
                 camera_otp_info[camera_id].awb_info.data_size);

        // lsc
        camera_otp_info[camera_id].lsc_info.data_offset =
            (buffer[0x00000033] << 8) | buffer[0x00000032];
        camera_otp_info[camera_id].lsc_info.data_size =
            (buffer[0x0000004A] << 8) | buffer[0x00000049];
        CMR_LOGV("lsc data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].lsc_info.data_offset,
                 camera_otp_info[camera_id].lsc_info.data_size);

        // pdaf_sprd
        camera_otp_info[camera_id].pdaf1_info.data_offset = 0;
        camera_otp_info[camera_id].pdaf1_info.data_size = 0;
        CMR_LOGV("pdaf_sprd_isp data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].pdaf1_info.data_offset,
                 camera_otp_info[camera_id].pdaf1_info.data_size);

        // pdaf spc from sensor vendor
        camera_otp_info[camera_id].pdaf2_info.data_offset = 0;
        camera_otp_info[camera_id].pdaf2_info.data_size = 0;
        CMR_LOGV("pdaf_sensor vendor data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].pdaf2_info.data_offset,
                 camera_otp_info[camera_id].pdaf2_info.data_size);

        // ae
        camera_otp_info[camera_id].ae_info.data_offset =
            (buffer[0x00000035] << 8) | buffer[0x00000034];
        camera_otp_info[camera_id].ae_info.data_size = buffer[0x0000004B];
        CMR_LOGV("ae data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].ae_info.data_offset,
                 camera_otp_info[camera_id].ae_info.data_size);

        // dual camera bokeh
        camera_otp_info[camera_id].bokeh_info.data_offset = 0;
        camera_otp_info[camera_id].bokeh_info.data_size = 0;
        CMR_LOGV("dualcam bokeh data_offset=0x%x, data_size=%u",
                 camera_otp_info[camera_id].bokeh_info.data_offset,
                 camera_otp_info[camera_id].bokeh_info.data_size);
    }

    camera_otp_info[camera_id].has_parsered = 1;

    return rtn;
}

static cmr_int _otp_parser_af_v10(void *raw_data, cmr_uint start_offset,
                                  cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_af_data *af_data = (struct otp_parser_af_data *)result;
    cmr_u32 af_start_offset = start_offset;
    cmr_u32 af_data_size = data_size;
    cmr_u32 af_end_offset = start_offset + data_size;
    cmr_u8 *af_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no af data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, af_start_offset, af_data_size,
                               af_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("af checksum error");
        return rtn;
    }

    af_buf = (cmr_u8 *)raw_data + af_start_offset;

    af_data->version = af_buf[0];
    af_data->inf_position = (af_buf[2] << 8) | af_buf[1];
    af_data->macro_position = (af_buf[4] << 8) | af_buf[3];
    af_data->posture = af_buf[5];
    af_data->temperature1 = (af_buf[7] << 8) | af_buf[6];
    af_data->temperature2 = (af_buf[9] << 8) | af_buf[8];

    CMR_LOGV("af version=%u, inf=%u, macro=%u, posture=%u, temperature1=%u, "
             "temperature2=%u",
             af_data->version, af_data->inf_position, af_data->macro_position,
             af_data->posture, af_data->temperature1, af_data->temperature2);

    return rtn;
}

static cmr_int _otp_parser_awb_v10(void *raw_data, cmr_uint start_offset,
                                   cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_awb_data *awb_data = (struct otp_parser_awb_data *)result;
    cmr_u32 awb_start_offset = start_offset;
    cmr_u32 awb_end_offset = start_offset + data_size;
    cmr_u32 awb_data_size = data_size;
    cmr_u8 *awb_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no awb data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, awb_start_offset, awb_data_size,
                               awb_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("awb checksum error");
        return rtn;
    }

    awb_buf = (cmr_u8 *)raw_data + awb_start_offset;

    awb_data->version = awb_buf[0];
    awb_data->awb1_r_gain = (awb_buf[2] << 8) | awb_buf[1];
    awb_data->awb1_g_gain = (awb_buf[4] << 8) | awb_buf[3];
    awb_data->awb1_b_gain = (awb_buf[6] << 8) | awb_buf[5];
    awb_data->awb2_r_gain = (awb_buf[8] << 8) | awb_buf[7];
    awb_data->awb2_g_gain = (awb_buf[10] << 8) | awb_buf[9];
    awb_data->awb2_b_gain = (awb_buf[12] << 8) | awb_buf[11];

    CMR_LOGV("awb version=%u, awb1:(r, g, b)=(%u, %u, %u), awb2:(r, g, b)=(%u, "
             "%u, %u)",
             awb_data->version, awb_data->awb1_r_gain, awb_data->awb1_g_gain,
             awb_data->awb1_b_gain, awb_data->awb2_r_gain,
             awb_data->awb2_g_gain, awb_data->awb2_b_gain);

    return rtn;
}

static cmr_int _otp_parser_lsc_v10(void *raw_data, cmr_uint start_offset,
                                   cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_lsc_data *lsc_data = (struct otp_parser_lsc_data *)result;
    cmr_u32 lsc_start_offset = start_offset;
    cmr_u32 lsc_end_offset = start_offset + data_size;
    cmr_u32 lsc_data_size = data_size;
    cmr_u8 *lsc_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no lsc data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, lsc_start_offset, lsc_data_size,
                               lsc_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("lsc checksum error");
        return rtn;
    }

    lsc_buf = (cmr_u8 *)raw_data + lsc_start_offset;

    lsc_data->version = lsc_buf[0];
    lsc_data->oc_r_x = (lsc_buf[2] << 8) | lsc_buf[1];
    lsc_data->oc_r_y = (lsc_buf[4] << 8) | lsc_buf[3];
    lsc_data->oc_gr_x = (lsc_buf[6] << 8) | lsc_buf[5];
    lsc_data->oc_gr_y = (lsc_buf[8] << 8) | lsc_buf[7];
    lsc_data->oc_gb_x = (lsc_buf[10] << 8) | lsc_buf[9];
    lsc_data->oc_gb_y = (lsc_buf[12] << 8) | lsc_buf[11];
    lsc_data->oc_b_x = (lsc_buf[14] << 8) | lsc_buf[13];
    lsc_data->oc_b_y = (lsc_buf[16] << 8) | lsc_buf[15];
    lsc_data->sensor_resolution_h = (lsc_buf[18] << 8) | lsc_buf[17];
    lsc_data->sensor_resolution_v = (lsc_buf[20] << 8) | lsc_buf[19];
    lsc_data->lsc_grid = lsc_buf[21];
    lsc_data->lsc_table_size = lsc_data_size - 22;
    lsc_data->lsc_table_addr = &lsc_buf[22];

    CMR_LOGV("lsc version=%u, oc r(%u, %u), gr(%u, %u), gb(%u, %u), b(%u, %u)",
             lsc_data->version, lsc_data->oc_r_x, lsc_data->oc_r_y,
             lsc_data->oc_gr_x, lsc_data->oc_gr_y, lsc_data->oc_gb_x,
             lsc_data->oc_gb_y, lsc_data->oc_b_x, lsc_data->oc_b_y);
    CMR_LOGV("lsc grid=%u, h=%u, v=%u", lsc_data->lsc_grid,
             lsc_data->sensor_resolution_h, lsc_data->sensor_resolution_v);
    CMR_LOGV("lsc table size = %u", lsc_data->lsc_table_size);

    return rtn;
}

static cmr_int _otp_parser_pdaf1_v10(void *raw_data, cmr_uint start_offset,
                                     cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_pdaf1_data *pdaf1_data =
        (struct otp_parser_pdaf1_data *)result;
    cmr_u32 pdaf1_start_offset = start_offset;
    cmr_u32 pdaf1_end_offset = start_offset + data_size;
    cmr_u32 pdaf1_data_size = data_size;
    cmr_u8 *pdaf1_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no pdaf_sprd_isp data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, pdaf1_start_offset, pdaf1_data_size,
                               pdaf1_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("pdaf_sprd_isp checksum error");
        return rtn;
    }

    pdaf1_buf = (cmr_u8 *)raw_data + pdaf1_start_offset;

    pdaf1_data->version = pdaf1_buf[0];
    pdaf1_data->pdaf1_data_size = pdaf1_data_size - 1;
    pdaf1_data->pdaf1_data_addr = &pdaf1_buf[1];
    CMR_LOGV("pdaf_sprd_isp version=%u, data_addr=%p, data_size=%u",
             pdaf1_data->version, pdaf1_data->pdaf1_data_addr,
             pdaf1_data->pdaf1_data_size);

    return rtn;
}

static cmr_int _otp_parser_pdaf2_v10(void *raw_data, cmr_uint start_offset,
                                     cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_pdaf2_data *pdaf2_data =
        (struct otp_parser_pdaf2_data *)result;
    cmr_u32 pdaf2_start_offset = start_offset;
    cmr_u32 pdaf2_end_offset = start_offset + data_size;
    cmr_u32 pdaf2_data_size = data_size;
    cmr_u8 *pdaf2_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no pdaf spc vendor data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, pdaf2_start_offset, pdaf2_data_size,
                               pdaf2_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("pdaf spc vendor checksum error");
        return rtn;
    }

    pdaf2_buf = (cmr_u8 *)raw_data + pdaf2_start_offset;

    pdaf2_data->pdaf2_data_size = pdaf2_data_size;
    pdaf2_data->pdaf2_data_addr = &pdaf2_buf[0];
    CMR_LOGV("pdaf spc vendor data_addr=%p, data_size=%u",
             pdaf2_data->pdaf2_data_addr, pdaf2_data->pdaf2_data_size);

    return rtn;
}

static cmr_int _otp_parser_ae_v10(void *raw_data, cmr_uint start_offset,
                                  cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_ae_data *ae_data = (struct otp_parser_ae_data *)result;
    cmr_u32 ae_start_offset = start_offset;
    cmr_u32 ae_end_offset = start_offset + data_size;
    cmr_u32 ae_data_size = data_size;
    cmr_u8 *ae_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no ae data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, ae_start_offset, ae_data_size,
                               ae_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("ae checksum error");
        return rtn;
    }

    ae_buf = (cmr_u8 *)raw_data + ae_start_offset;

    ae_data->version = ae_buf[0];
    ae_data->target_lum = (ae_buf[2] << 8) | ae_buf[1];
    ae_data->exp_1x_gain =
        (ae_buf[6] << 24) | (ae_buf[5] << 16) | (ae_buf[4] << 8) | ae_buf[3];
    ae_data->exp_2x_gain =
        (ae_buf[10] << 24) | (ae_buf[9] << 16) | (ae_buf[8] << 8) | ae_buf[7];
    ae_data->exp_4x_gain = (ae_buf[14] << 24) | (ae_buf[13] << 16) |
                           (ae_buf[12] << 8) | ae_buf[11];
    ae_data->exp_8x_gain = (ae_buf[18] << 24) | (ae_buf[17] << 16) |
                           (ae_buf[16] << 8) | ae_buf[15];

    CMR_LOGV("ae lum: %u, gain_1x_exp: %u, gain_2x_exp: %u, gain_4x_exp: %u, "
             "gain_4x_exp: %u",
             ae_data->target_lum, ae_data->exp_1x_gain, ae_data->exp_2x_gain,
             ae_data->exp_4x_gain, ae_data->exp_8x_gain);

    return rtn;
}

static cmr_int _otp_parser_dualcamera_v10(void *raw_data, cmr_uint start_offset,
                                          cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_dualcam_for_sprd_3rd *dual_data =
        (struct otp_parser_dualcam_for_sprd_3rd *)result;
    cmr_u32 dual_start_offset = start_offset;
    cmr_u32 dual_end_offset = start_offset + data_size;
    cmr_u32 dual_data_size = data_size;
    cmr_u8 *dual_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no dual camera data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, dual_start_offset, dual_data_size,
                               dual_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("dual camera checksum error");
        return rtn;
    }

    dual_buf = (cmr_u8 *)raw_data + dual_start_offset;

    dual_data->version = dual_buf[0];
    dual_data->dual_flag = 1;
    dual_data->data_3d_data_size = dual_data_size;
    dual_data->data_3d_data = &dual_buf[1];
    CMR_LOGV("dual camera version=%u, dual_flag=%u, data_addr=%p, data_size=%u",
             dual_data->version, dual_data->dual_flag, dual_data->data_3d_data,
             dual_data->data_3d_data_size);

    return rtn;
}

static cmr_int _otp_parser_section_with_version_v10(void *raw_data,
                                                    cmr_uint start_offset,
                                                    cmr_uint data_size,
                                                    void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_section_with_version *section_data =
        (struct otp_parser_section_with_version *)result;
    cmr_u8 *section_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("no data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, start_offset, data_size,
                               start_offset + data_size);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("checksum error");
        return rtn;
    }

    section_buf = (cmr_u8 *)raw_data + start_offset;

    section_data->version = section_buf[0];
    section_data->data_size = data_size - 1;
    section_data->data_addr = &section_buf[1];
    CMR_LOGV("section version=%u, data_addr=%p, data_size=%u",
             section_data->version, section_data->data_addr,
             section_data->data_size);

    return rtn;
}

static cmr_int _otp_parser_section_v10(void *raw_data, cmr_uint start_offset,
                                       cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_section *section_data =
        (struct otp_parser_section *)result;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("no data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, start_offset, data_size,
                               start_offset + data_size);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("checksum error");
        return rtn;
    }

    section_data->data_size = data_size;
    section_data->data_addr = (cmr_u8 *)raw_data + start_offset;
    CMR_LOGV("section without version, data_addr=%p, data_size=%u",
             section_data->data_addr, section_data->data_size);

    return rtn;
}

cmr_int otp_parser_map_v10(void *raw_data, enum otp_parser_cmd cmd,
                           cmr_int camera_id, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_uint start_offset = 0;
    cmr_uint data_size = 0;

    CMR_LOGI("E");

    rtn = _otp_parser_get_start_offset_v10(raw_data, camera_id);

    CMR_LOGI("cmd=%d", cmd);
    switch (cmd) {
    case OTP_PARSER_AF:
        start_offset = camera_otp_info[camera_id].af_info.data_offset;
        data_size = camera_otp_info[camera_id].af_info.data_size;
        rtn = _otp_parser_af_v10(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_AWB:
        start_offset = camera_otp_info[camera_id].awb_info.data_offset;
        data_size = camera_otp_info[camera_id].awb_info.data_size;
        rtn = _otp_parser_awb_v10(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_LSC:
        start_offset = camera_otp_info[camera_id].lsc_info.data_offset;
        data_size = camera_otp_info[camera_id].lsc_info.data_size;
        rtn = _otp_parser_lsc_v10(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_PDAF1:
        start_offset = camera_otp_info[camera_id].pdaf1_info.data_offset;
        data_size = camera_otp_info[camera_id].pdaf1_info.data_size;
        rtn = _otp_parser_pdaf1_v10(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_AE:
        start_offset = camera_otp_info[camera_id].ae_info.data_offset;
        data_size = camera_otp_info[camera_id].ae_info.data_size;
        rtn = _otp_parser_ae_v10(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_SECTION_BOKEH:
        start_offset = camera_otp_info[camera_id].bokeh_info.data_offset;
        data_size = camera_otp_info[camera_id].bokeh_info.data_size;
        rtn = _otp_parser_dualcamera_v10(raw_data, start_offset, data_size,
                                         result);
        break;
    case OTP_PARSER_SECTION_XTALK_4IN1:
        start_offset = camera_otp_info[camera_id].xtalk_4in1_info.data_offset;
        data_size = camera_otp_info[camera_id].xtalk_4in1_info.data_size;
        rtn = _otp_parser_section_with_version_v10(raw_data, start_offset,
                                                   data_size, result);
        break;
    case OTP_PARSER_SECTION_DPC_4IN1:
        start_offset = camera_otp_info[camera_id].dpc_4in1_info.data_offset;
        data_size = camera_otp_info[camera_id].dpc_4in1_info.data_size;
        rtn = _otp_parser_section_with_version_v10(raw_data, start_offset,
                                                   data_size, result);
        break;
    default:
        CMR_LOGV("input cmd error");
        rtn = OTP_PARSER_CMD_ERR;
        break;
    }

    CMR_LOGI("X");

    return rtn;
}
