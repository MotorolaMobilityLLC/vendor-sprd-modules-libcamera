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

#define LOG_TAG "otp_parser_v04"
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

static cmr_uint _otp_parser_get_lsc_chn_len(cmr_int raw_height,
                                            cmr_int raw_width, cmr_int grid) {
    cmr_int lsc_otp_len_chn = 0;
    cmr_int half_width = 0;
    cmr_int half_height = 0;
    cmr_int lsc_otp_width = 0;
    cmr_int lsc_otp_height = 0;

    if (0 == grid) {
        CMR_LOGE("lsc grid is 0");
        return 0;
    }

    half_width = raw_width / 2;
    half_height = raw_height / 2;
    lsc_otp_width = ((half_width % grid) > 0) ? (half_width / grid + 2)
                                              : (half_width / grid + 1);
    lsc_otp_height = ((half_height % grid) > 0) ? (half_height / grid + 2)
                                                : (half_height / grid + 1);

    lsc_otp_len_chn = ((lsc_otp_width * lsc_otp_height) * 14 % 8)
                          ? (((lsc_otp_width * lsc_otp_height) * 14 / 8) + 1)
                          : ((lsc_otp_width * lsc_otp_height) * 14 / 8);
    lsc_otp_len_chn =
        lsc_otp_len_chn % 2 ? lsc_otp_len_chn + 1 : lsc_otp_len_chn;

    CMR_LOGV("lsc channel length=%ld", lsc_otp_len_chn);

    return lsc_otp_len_chn;
}

static cmr_uint _otp_parser_get_start_offset_v04(cmr_u8 *raw_data,
                                                 cmr_int eeprom_num,
                                                 cmr_int camera_id,
                                                 cmr_int height,
                                                 cmr_int width) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_uint lsc_chn_len = 0;
    cmr_uint resolution = 0;
    cmr_uint start_addr = 0;
    cmr_uint lsc_grid = 0;

    if (camera_otp_info[camera_id].has_parsered == 1) {
        CMR_LOGD("otp section addr and size has been parsered before");
        return rtn;
    }

    // get otp start address of this camera
    CMR_LOGV("camera_id=%ld, eeprom_num=%ld", camera_id, eeprom_num);
    if (0 == camera_id || 1 == camera_id) {
        // for master or single camera
        start_addr = 0x00000000;
    } else if (2 == camera_id || 3 == camera_id) {
        // for slave camera
        if (OTP_EEPROM_SINGLE_CAM_DUAL == eeprom_num) {
            start_addr = 0x00001A00;
        } else if (OTP_EEPROM_DUAL_CAM_DUAL == eeprom_num) {
            start_addr = 0x00000000;
        }
    }

    // calculate resolution by width and height
    resolution = (width * height + 500000) / 1000000;

    // get lsc table grid
    if (NULL != raw_data) {
        lsc_grid = raw_data[start_addr + 0x0000000D];
        CMR_LOGD("lsc grid: %ld", lsc_grid);
        if (0 == lsc_grid) {
            rtn = OTP_PARSER_PARAM_ERR;
            return rtn;
        }
    } else {
        rtn = OTP_PARSER_PARAM_ERR;
        CMR_LOGE("raw data is null");
        return rtn;
    }

    // af
    camera_otp_info[camera_id].af_info.data_offset = start_addr + 16;
    camera_otp_info[camera_id].af_info.data_size = 5;
    CMR_LOGV("af data_offset=0x%x, data_size=%u",
             camera_otp_info[camera_id].af_info.data_offset,
             camera_otp_info[camera_id].af_info.data_size);

    // awb
    camera_otp_info[camera_id].awb_info.data_offset =
        camera_otp_info[camera_id].af_info.data_offset +
        camera_otp_info[camera_id].af_info.data_size + 1;
    camera_otp_info[camera_id].awb_info.data_size = 12;
    CMR_LOGV("awb data_offset=0x%x, data_size=%u",
             camera_otp_info[camera_id].awb_info.data_offset,
             camera_otp_info[camera_id].awb_info.data_size);

    // lsc
    camera_otp_info[camera_id].lsc_info.data_offset =
        camera_otp_info[camera_id].awb_info.data_offset +
        camera_otp_info[camera_id].awb_info.data_size + 1;
    lsc_chn_len = _otp_parser_get_lsc_chn_len(height, width, lsc_grid);
    if (resolution == 2) {
        camera_otp_info[camera_id].lsc_info.data_size =
            lsc_chn_len * 4 + 16 + 148; // 148 bytes is reserved in 2M otp map
    } else {
        camera_otp_info[camera_id].lsc_info.data_size = lsc_chn_len * 4 + 16;
    }
    CMR_LOGV("lsc data_offset=0x%x, data_size =%u",
             camera_otp_info[camera_id].lsc_info.data_offset,
             camera_otp_info[camera_id].lsc_info.data_size);

    // pdaf_sprd
    if (0 == camera_id || 1 == camera_id) {
        camera_otp_info[camera_id].pdaf1_info.data_offset =
            camera_otp_info[camera_id].lsc_info.data_offset +
            camera_otp_info[camera_id].lsc_info.data_size + 1;
        camera_otp_info[camera_id].pdaf1_info.data_size = 384;
    } else {
        camera_otp_info[camera_id].pdaf1_info.data_offset = 0;
        camera_otp_info[camera_id].pdaf1_info.data_size = 0;
    }
    CMR_LOGV("pdaf_sprd data_offset=0x%x, data_size=%u",
             camera_otp_info[camera_id].pdaf1_info.data_offset,
             camera_otp_info[camera_id].pdaf1_info.data_size);

    // ae
    // only dual camera modules have ae otp data
    if (OTP_EEPROM_SINGLE_CAM_DUAL == eeprom_num ||
        OTP_EEPROM_DUAL_CAM_DUAL == eeprom_num) {
        if (0 == camera_id || 1 == camera_id) {
            // for master module
            camera_otp_info[camera_id].ae_info.data_offset =
                camera_otp_info[camera_id].pdaf1_info.data_offset +
                camera_otp_info[camera_id].pdaf1_info.data_size + 1;
            camera_otp_info[camera_id].ae_info.data_size = 24;
        } else {
            // for slave module
            camera_otp_info[camera_id].ae_info.data_offset =
                camera_otp_info[camera_id].lsc_info.data_offset +
                camera_otp_info[camera_id].lsc_info.data_size + 1;
            switch (resolution) {
            case 2:
                camera_otp_info[camera_id].ae_info.data_size = 31;
                break;
            case 5:
                camera_otp_info[camera_id].ae_info.data_size = 24;
                break;
            default:
                camera_otp_info[camera_id].ae_info.data_size = 0;
                break;
            }
        }
    } else {
        camera_otp_info[camera_id].ae_info.data_offset = 0;
        camera_otp_info[camera_id].ae_info.data_size = 0;
    }
    CMR_LOGV("ae data_offset=0x%x, data_size=%u",
             camera_otp_info[camera_id].ae_info.data_offset,
             camera_otp_info[camera_id].ae_info.data_size);

    // dual camera
    // only dual master module have dual camera otp data
    if ((OTP_EEPROM_SINGLE_CAM_DUAL == eeprom_num ||
         OTP_EEPROM_DUAL_CAM_DUAL == eeprom_num) &&
        (0 == camera_id || 1 == camera_id)) {
        camera_otp_info[camera_id].bokeh_info.data_offset =
            camera_otp_info[camera_id].ae_info.data_offset +
            camera_otp_info[camera_id].ae_info.data_size + 1;
        camera_otp_info[camera_id].bokeh_info.data_size = 230;
    } else {
        camera_otp_info[camera_id].bokeh_info.data_offset = 0;
        camera_otp_info[camera_id].bokeh_info.data_size = 0;
    }
    CMR_LOGV("dualcam data_offset=0x%x, data_size=%u",
             camera_otp_info[camera_id].bokeh_info.data_offset,
             camera_otp_info[camera_id].bokeh_info.data_size);

    // pdaf spc from sensor vendor
    if (OTP_EEPROM_SINGLE_CAM_DUAL == eeprom_num ||
        OTP_EEPROM_DUAL_CAM_DUAL == eeprom_num) {
        if (0 == camera_id || 1 == camera_id) {
            switch (resolution) {
            case 13:
                camera_otp_info[camera_id].pdaf2_info.data_offset =
                    start_addr + 0x00001900;
                camera_otp_info[camera_id].pdaf2_info.data_size = 255;
                break;
            case 8:
                camera_otp_info[camera_id].pdaf2_info.data_offset =
                    start_addr + 0x00001000;
                camera_otp_info[camera_id].pdaf2_info.data_size = 2559;
                break;
            default:
                camera_otp_info[camera_id].pdaf2_info.data_offset = 0;
                camera_otp_info[camera_id].pdaf2_info.data_size = 0;
                break;
            }

        } else {
            camera_otp_info[camera_id].pdaf2_info.data_offset = 0;
            camera_otp_info[camera_id].pdaf2_info.data_size = 0;
        }
    } else if (OTP_EEPROM_SINGLE == eeprom_num) {
        camera_otp_info[camera_id].pdaf2_info.data_offset =
            camera_otp_info[camera_id].pdaf1_info.data_offset +
            camera_otp_info[camera_id].pdaf1_info.data_size + 1;
        camera_otp_info[camera_id].pdaf2_info.data_size = 255;
    }
    CMR_LOGV("pdaf spc from sensor vendor data_offset=0x%x, data_size=%u",
             camera_otp_info[camera_id].pdaf2_info.data_offset,
             camera_otp_info[camera_id].pdaf2_info.data_size);

    camera_otp_info[camera_id].has_parsered = 1;

    return rtn;
}

static cmr_int _otp_parser_af_v04(void *raw_data, cmr_uint start_offset,
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

    af_data->version = 0;
    af_data->inf_position = (af_buf[1] << 8) | af_buf[0];
    af_data->macro_position = (af_buf[3] << 8) | af_buf[2];
    af_data->posture = af_buf[4];
    af_data->temperature1 = 0;
    af_data->temperature2 = 0;

    CMR_LOGV("af inf=%u, macro=%u, posture=%u", af_data->inf_position,
             af_data->macro_position, af_data->posture);

    return rtn;
}

static cmr_int _otp_parser_awb_v04(void *raw_data, cmr_uint start_offset,
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

    awb_data->version = 0;
    awb_data->awb1_r_gain = (awb_buf[1] << 8) | awb_buf[0];
    awb_data->awb1_g_gain = (awb_buf[3] << 8) | awb_buf[2];
    awb_data->awb1_b_gain = (awb_buf[5] << 8) | awb_buf[4];
    awb_data->awb2_r_gain = (awb_buf[7] << 8) | awb_buf[6];
    awb_data->awb2_g_gain = (awb_buf[9] << 8) | awb_buf[8];
    awb_data->awb2_b_gain = (awb_buf[11] << 8) | awb_buf[10];

    CMR_LOGV("awb1:(r, g, b)=(%u, %u, %u), awb2:(r, g, b)=(%u, %u, %u)",
             awb_data->awb1_r_gain, awb_data->awb1_g_gain,
             awb_data->awb1_b_gain, awb_data->awb2_r_gain,
             awb_data->awb2_g_gain, awb_data->awb2_b_gain);

    return rtn;
}

static cmr_int _otp_parser_lsc_v04(void *raw_data, cmr_uint start_offset,
                                   cmr_uint data_size, cmr_uint height,
                                   cmr_uint width, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_lsc_data *lsc_data = (struct otp_parser_lsc_data *)result;
    cmr_u8 *raw_otp = (cmr_u8 *)raw_data;
    cmr_u32 lsc_start_offset = start_offset;
    cmr_u32 lsc_end_offset = start_offset + data_size;
    cmr_u32 lsc_data_size = data_size;
    cmr_u8 *lsc_buf = NULL;
    cmr_u32 lsc_chn_len = 0;

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

    lsc_data->version = 0;
    lsc_data->oc_r_x = (lsc_buf[1] << 8) | lsc_buf[0];
    lsc_data->oc_r_y = (lsc_buf[3] << 8) | lsc_buf[2];
    lsc_data->oc_gr_x = (lsc_buf[5] << 8) | lsc_buf[4];
    lsc_data->oc_gr_y = (lsc_buf[7] << 8) | lsc_buf[6];
    lsc_data->oc_gb_x = (lsc_buf[9] << 8) | lsc_buf[8];
    lsc_data->oc_gb_y = (lsc_buf[11] << 8) | lsc_buf[10];
    lsc_data->oc_b_x = (lsc_buf[13] << 8) | lsc_buf[12];
    lsc_data->oc_b_y = (lsc_buf[15] << 8) | lsc_buf[14];
    lsc_data->sensor_resolution_h = 0;
    lsc_data->sensor_resolution_v = 0;

    if (start_offset < 0x00001000) {
        lsc_data->lsc_grid = raw_otp[0x0000000D];
    } else {
        lsc_data->lsc_grid = raw_otp[0x00001A0D];
    }

    lsc_chn_len =
        _otp_parser_get_lsc_chn_len(height, width, lsc_data->lsc_grid);

    lsc_data->lsc_table_size = lsc_chn_len * 4; // exclude oc and checksum bytes
    lsc_data->lsc_table_addr = &lsc_buf[16];

    CMR_LOGV("lsc oc r(%u, %u), gr(%u, %u), gb(%u, %u), b(%u, %u)",
             lsc_data->oc_r_x, lsc_data->oc_r_y, lsc_data->oc_gr_x,
             lsc_data->oc_gr_y, lsc_data->oc_gb_x, lsc_data->oc_gb_y,
             lsc_data->oc_b_x, lsc_data->oc_b_y);
    CMR_LOGV("lsc grid=%u", lsc_data->lsc_grid);
    CMR_LOGV("lsc table size=%u", lsc_data->lsc_table_size);

    return rtn;
}

static cmr_int _otp_parser_pdaf1_v04(void *raw_data, cmr_uint start_offset,
                                     cmr_uint data_size, void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    struct otp_parser_pdaf1_data *pdaf1_data =
        (struct otp_parser_pdaf1_data *)result;
    cmr_u32 pdaf1_start_offset = start_offset;
    cmr_u32 pdaf1_end_offset = start_offset + data_size;
    cmr_u32 pdaf1_data_size = data_size;
    cmr_u8 *pdaf1_buf = NULL;

    if (0 == start_offset || 0 == data_size) {
        CMR_LOGE("there is no pdaf_sprd data");
        rtn = OTP_PARSER_CMD_ERR;
        return rtn;
    }

    rtn = _otp_parser_checksum(raw_data, pdaf1_start_offset, pdaf1_data_size,
                               pdaf1_end_offset);
    if (OTP_PARSER_SUCCESS != rtn) {
        CMR_LOGE("pdaf_sprd checksum error");
        return rtn;
    }

    pdaf1_buf = (cmr_u8 *)raw_data + pdaf1_start_offset;

    pdaf1_data->version = 0;
    pdaf1_data->pdaf1_data_size = pdaf1_data_size;
    pdaf1_data->pdaf1_data_addr = pdaf1_buf;
    CMR_LOGV("pdaf_sprd data_addr=%p, data_size=%u", pdaf1_buf,
             pdaf1_data_size);

    return rtn;
}

static cmr_int _otp_parser_pdaf2_v04(void *raw_data, cmr_uint start_offset,
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
    pdaf2_data->pdaf2_data_addr = pdaf2_buf;
    CMR_LOGV("pdaf spc vendor data_addr=%p, data_size=%u", pdaf2_buf,
             pdaf2_data_size);

    return rtn;
}

static cmr_int _otp_parser_ae_v04(void *raw_data, cmr_uint start_offset,
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

    ae_data->version = 0;
    ae_data->target_lum = (ae_buf[1] << 8) | ae_buf[0];
    ae_data->exp_1x_gain =
        (ae_buf[5] << 24) | (ae_buf[4] << 16) | (ae_buf[3] << 8) | ae_buf[2];
    ae_data->exp_2x_gain =
        (ae_buf[9] << 24) | (ae_buf[8] << 16) | (ae_buf[7] << 8) | ae_buf[6];
    ae_data->exp_4x_gain = (ae_buf[13] << 24) | (ae_buf[12] << 16) |
                           (ae_buf[11] << 8) | ae_buf[10];
    ae_data->exp_8x_gain = (ae_buf[17] << 24) | (ae_buf[16] << 16) |
                           (ae_buf[15] << 8) | ae_buf[14];

    CMR_LOGV("ae lum:%u, gain_1x_exp:%u, gain_2x_exp:%u, gain_4x_exp:%u, "
             "gain_4x_exp:%u",
             ae_data->target_lum, ae_data->exp_1x_gain, ae_data->exp_2x_gain,
             ae_data->exp_4x_gain, ae_data->exp_8x_gain);

    return rtn;
}

static cmr_int _otp_parser_dualcamera_v04(void *raw_data, cmr_uint start_offset,
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

    dual_data->version = 0;
    dual_data->dual_flag = 1;
    dual_data->data_3d_data_size = dual_data_size;
    dual_data->data_3d_data = dual_buf;
    CMR_LOGV("dual camera dual_flag=%u, data_addr=%p, data_size=%u",
             dual_data->dual_flag, dual_data->data_3d_data,
             dual_data->data_3d_data_size);

    return rtn;
}

cmr_int otp_parser_map_v04(void *raw_data, enum otp_parser_cmd cmd,
                           cmr_int eeprom_num, cmr_int camera_id,
                           cmr_int raw_height, cmr_int raw_width,
                           void *result) {
    cmr_int rtn = OTP_PARSER_SUCCESS;
    cmr_uint start_offset = 0;
    cmr_uint data_size = 0;

    CMR_LOGI("E");

    rtn = _otp_parser_get_start_offset_v04((cmr_u8 *)raw_data, eeprom_num,
                                           camera_id, raw_height, raw_width);

    CMR_LOGI("cmd=%d", cmd);
    switch (cmd) {
    case OTP_PARSER_AF:
        start_offset = camera_otp_info[camera_id].af_info.data_offset;
        data_size = camera_otp_info[camera_id].af_info.data_size;
        rtn = _otp_parser_af_v04(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_AWB:
        start_offset = camera_otp_info[camera_id].awb_info.data_offset;
        data_size = camera_otp_info[camera_id].awb_info.data_size;
        rtn = _otp_parser_awb_v04(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_LSC:
        start_offset = camera_otp_info[camera_id].lsc_info.data_offset;
        data_size = camera_otp_info[camera_id].lsc_info.data_size;
        rtn = _otp_parser_lsc_v04(raw_data, start_offset, data_size, raw_height,
                                  raw_width, result);
        break;
    case OTP_PARSER_PDAF1:
        start_offset = camera_otp_info[camera_id].pdaf1_info.data_offset;
        data_size = camera_otp_info[camera_id].pdaf1_info.data_size;
        rtn = _otp_parser_pdaf1_v04(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_AE:
        start_offset = camera_otp_info[camera_id].ae_info.data_offset;
        data_size = camera_otp_info[camera_id].ae_info.data_size;
        rtn = _otp_parser_ae_v04(raw_data, start_offset, data_size, result);
        break;
    case OTP_PARSER_SECTION_BOKEH:
        start_offset = camera_otp_info[camera_id].bokeh_info.data_offset;
        data_size = camera_otp_info[camera_id].bokeh_info.data_size;
        rtn = _otp_parser_dualcamera_v04(raw_data, start_offset, data_size,
                                         result);
        break;
    default:
        CMR_LOGV("input cmd error");
        rtn = OTP_PARSER_CMD_ERR;
        break;
    }

    CMR_LOGI("X");

    return rtn;
}
