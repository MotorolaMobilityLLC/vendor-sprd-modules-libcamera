/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * V1.1
 * History
 *    Date                    Modification                   Reason
 * 2018-09-27                   Original
 * 2019-06-26              Add otp v1.1 support
 */

#include "general_otp_drv.h"
#include "otp_parser.h"
/*==================================================
*                Internal Functions
====================================================*/
static cmr_int _general_otp_section_checksum(cmr_u8 *buffer, cmr_uint offset,
                                             cmr_uint size,
                                             cmr_uint checksum_offset,
                                             enum otp_version_t otp_version) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 i = 0, sum = 0, checksum_cal = 0;
    char *otp_ver[] = {"0",   "0.1", "0.2", "0.3", "0.4", "0.5",
                       "0.6", "0.7", "0.8", "0.9", "1.0", "1.1"};

    OTP_LOGV("E");
    for (i = offset; i < offset + size; i++) {
        sum += buffer[i];
    }

    if (sum == 0) {
        ret = OTP_CAMERA_FAIL;
        OTP_LOGD("exception: all data is 0");
    } else if (sum == 0xff * size) {
        ret = OTP_CAMERA_FAIL;
        OTP_LOGD("exception: all data is 0xff");
    } else {
        if (otp_version == OTP_0_1) {
            checksum_cal = (sum % 255 + 1);
        } else {
            checksum_cal = (sum % 256);
        }
        if (checksum_cal == buffer[checksum_offset]) {
            ret = OTP_CAMERA_SUCCESS;
            OTP_LOGD("passed: otp_version = %s, checksum_offset = 0x%lx, "
                     "checksum_value = %d, sum = %d",
                     otp_ver[otp_version], checksum_offset,
                     buffer[checksum_offset], sum);
        } else {
            ret = CMR_CAMERA_FAIL;
            OTP_LOGD("failed: otp_version = %s, checksum_offset = 0x%lx, "
                     "checksum_value = %d, sum = %d, checksum_calculate = %d",
                     otp_ver[otp_version], checksum_offset,
                     buffer[checksum_offset], sum, checksum_cal);
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_get_lsc_channel_size(cmr_u16 width, cmr_u16 height,
                                                 cmr_u8 grid) {
    OTP_LOGV("E");

    cmr_u32 resolution = 0;
    cmr_u32 half_width = 0;
    cmr_u32 half_height = 0;
    cmr_u32 lsc_width = 0;
    cmr_u32 lsc_height = 0;
    cmr_u32 lsc_channel_size = 0;

    /*             common resolution grid and lsc_channel_size
    |----------|----------------------|------|------|----|----------------|
    |resolution|       example        |width |height|grid|lsc_channel_size|
    |          |                      |      |      |    |    (14bits)    |
    |----------|----------------------|------|------|----|----------------|
    |    2M    | ov2680               | 1600 | 1200 | 64 |      270       |
    |    5M    | ov5675,s5k5e8yx      | 2592 | 1944 | 64 |      656       |
    |    8M    | ov8858,ov8856,sp8407 | 3264 | 2448 | 96 |      442       |
    |   12M    | imx386               | 4032 | 3016 | 96 |      656       |
    |   13M    | ov13855              | 4224 | 3136 | 96 |      726       |
    |   13M    | imx258               | 4208 | 3120 | 96 |      726       |
    |   16M    | imx351               | 4656 | 3492 |128 |      526       |
    |   16M    | s5k3p9sx04           | 4640 | 3488 |128 |      526       |
    |    4M    | ov16885(4in1)        | 2336 | 1752 | 64 |      526       |
    |   32M    | ov32a1q              | 6528 | 4896 |192 |      442       |
    |   16M    | ov16885              | 4672 | 3504 |128 |      526       |
    |----------|----------------------|------|------|----|----------------|*/
    if (grid == 0) {
        OTP_LOGE("lsc grid is 0!");
        return 0;
    }

    resolution = (width * height + 500000) / 1000000;

    half_width = width / 2;
    half_height = height / 2;
    lsc_width =
        (half_width % grid) ? (half_width / grid + 2) : (half_width / grid + 1);
    lsc_height = (half_height % grid) ? (half_height / grid + 2)
                                      : (half_height / grid + 1);

    lsc_channel_size = ((lsc_width * lsc_height) * 14 % 8)
                           ? (((lsc_width * lsc_height) * 14 / 8) + 1)
                           : ((lsc_width * lsc_height) * 14 / 8);
    lsc_channel_size =
        lsc_channel_size % 2 ? lsc_channel_size + 1 : lsc_channel_size;

    OTP_LOGI("resolution = %dM, lsc_channel_size = %d, lsc_width = %d, "
             "lsc_height = %d",
             resolution, lsc_channel_size, lsc_width, lsc_height);

    OTP_LOGV("X");
    return lsc_channel_size;
}

static cmr_int _general_otp_parse_map_version(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);

    OTP_LOGD("otp first six bytes: %02x %02x %02x %02x %02x %02x", buffer[0],
             buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

    module_info->calib_version = (buffer[4] << 8) | buffer[5];
    if (module_info->calib_version == 0x0101) {
        module_info->otp_version = OTP_1_1;
        OTP_LOGI("otp version is 1.1");
    } else if (buffer[0] == 0x53 && buffer[1] == 0x50 && buffer[2] == 0x52 &&
               buffer[3] == 0x44 && module_info->calib_version == 0x0100) {
        module_info->otp_version = OTP_1_0;
        OTP_LOGI("otp version is 1.0");
    } else if ((module_info->calib_version == 0x0005) ||
               (module_info->calib_version == 0x0500)) {
        module_info->otp_version = OTP_0_5;
        OTP_LOGI("otp version is 0.5");
    } else if ((module_info->calib_version == 0x0004) ||
               (module_info->calib_version == 0x0400)) {
        module_info->otp_version = OTP_0_4;
        OTP_LOGI("otp version is 0.4");
    } else if ((module_info->calib_version == 0x0003) ||
               (module_info->calib_version == 0x0300)) {
        module_info->otp_version = OTP_0_3;
        OTP_LOGI("otp version is 0.3");
    } else if ((module_info->calib_version == 0x0002) ||
               (module_info->calib_version == 0x0200)) {
        module_info->otp_version = OTP_0_2;
        OTP_LOGI("otp version is 0.2");
    } else if ((module_info->calib_version == 0x0001) ||
               ((module_info->calib_version == 0x0100) &&
                (buffer[0] != 0x53 || buffer[1] != 0x50 || buffer[2] != 0x52 ||
                 buffer[3] != 0x44))) {
        module_info->otp_version = OTP_0_1;
        OTP_LOGI("otp version is 0.1");
    } else {
        module_info->otp_version = VER_ERROR;
        OTP_LOGE("otp version error! calib_version = 0x%04x",
                 module_info->calib_version);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_module_data_0v4(cmr_handle otp_drv_handle,
                                                  cmr_u16 module_info_offset) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer + module_info_offset;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info_offset, 15,
        module_info_offset + 15, module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("module info checksum error!");
        return ret;
    }
    OTP_LOGD("module info checksum passed");

    module_dat->rdm_info.buffer = buffer;
    module_dat->rdm_info.size = 15;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    OTP_LOGD("otp v0.4 head hex: %02x %02x %02x %02x %02x %02x %02x %02x %02x "
             "%02x %02x %02x %02x %02x %02x",
             buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5],
             buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11],
             buffer[12], buffer[13], buffer[14]);

    module_info->module_id_info.year = buffer[6];
    module_info->module_id_info.month = buffer[7];
    module_info->module_id_info.day = buffer[8];
    OTP_LOGI("module calibration date = %d-%d-%d", buffer[6], buffer[7],
             buffer[8]);

    module_info->lsc_grid = buffer[13];
    module_info->resolution =
        (module_info->sensor_max_width * module_info->sensor_max_height +
         500000) /
        1000000;
    OTP_LOGI("sensor_max_width = %d, sensor_max_height = %d, lsc_grid = %d, "
             "resolution = %dM",
             module_info->sensor_max_width, module_info->sensor_max_height,
             module_info->lsc_grid, module_info->resolution);

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_get_single_addr_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);

    cmr_u32 lsc_channel_size = 0;
    lsc_channel_size = _general_otp_get_lsc_channel_size(
        module_info->sensor_max_width, module_info->sensor_max_height,
        module_info->lsc_grid);

    module_info->master_af_info.offset = 0x0010;
    module_info->master_af_info.size = 5;

    module_info->master_awb_info.offset = 0x0016;
    module_info->master_awb_info.size = 12;

    module_info->master_lsc_info.offset = 0x0023;
    module_info->master_lsc_info.size = 16 + lsc_channel_size * 4;

    module_info->master_pdaf1_info.offset =
        module_info->master_lsc_info.offset +
        module_info->master_lsc_info.size + 1;
    module_info->master_pdaf1_info.size = 384;

    module_info->master_pdaf2_info.offset =
        module_info->master_pdaf1_info.offset +
        module_info->master_pdaf1_info.size + 1;
    module_info->master_pdaf2_info.size = 255;

    OTP_LOGD(
        "single(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
        "pdaf1(0x%x, %d), pdaf2(0x%x, %d)",
        module_info->master_af_info.offset, module_info->master_af_info.size,
        module_info->master_awb_info.offset, module_info->master_awb_info.size,
        module_info->master_lsc_info.offset, module_info->master_lsc_info.size,
        module_info->master_pdaf1_info.offset,
        module_info->master_pdaf1_info.size,
        module_info->master_pdaf2_info.offset,
        module_info->master_pdaf2_info.size);
    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_get_master_addr_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);

    cmr_u32 lsc_channel_size = 0;
    lsc_channel_size = _general_otp_get_lsc_channel_size(
        module_info->sensor_max_width, module_info->sensor_max_height,
        module_info->lsc_grid);

    module_info->master_af_info.offset = 0x0010;
    module_info->master_af_info.size = 5;

    module_info->master_awb_info.offset = 0x0016;
    module_info->master_awb_info.size = 12;

    module_info->master_lsc_info.offset = 0x0023;
    module_info->master_lsc_info.size = 16 + lsc_channel_size * 4;

    module_info->master_pdaf1_info.offset =
        module_info->master_lsc_info.offset +
        module_info->master_lsc_info.size + 1;
    module_info->master_pdaf1_info.size = 384;

    module_info->master_ae_info.offset = module_info->master_pdaf1_info.offset +
                                         module_info->master_pdaf1_info.size +
                                         1;
    module_info->master_ae_info.size = 24;

    module_info->master_bokeh_info.offset = module_info->master_ae_info.offset +
                                            module_info->master_ae_info.size +
                                            1;
    if (module_info->otp_version == OTP_0_5) {
        module_info->master_bokeh_info.size = 256;
    } else {
        module_info->master_bokeh_info.size = 230;
    }

    OTP_LOGD(
        "dual_master(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, "
        "%d), pdaf1(0x%x, %d), ae(0x%x, %d), bokeh(0x%x, %d)",
        module_info->master_af_info.offset, module_info->master_af_info.size,
        module_info->master_awb_info.offset, module_info->master_awb_info.size,
        module_info->master_lsc_info.offset, module_info->master_lsc_info.size,
        module_info->master_pdaf1_info.offset,
        module_info->master_pdaf1_info.size, module_info->master_ae_info.offset,
        module_info->master_ae_info.size, module_info->master_bokeh_info.offset,
        module_info->master_bokeh_info.size);

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_get_slave_addr_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);

    cmr_u32 lsc_channel_size = 0;
    lsc_channel_size = _general_otp_get_lsc_channel_size(
        module_info->sensor_max_width, module_info->sensor_max_height,
        module_info->lsc_grid);

    if (otp_cxt->eeprom_num == DUAL_CAM_TWO_EEPROM) {
        /* 13M + 5M (ov13855+ov5675) */
        module_info->slave_af_info.offset = 0x0010;
        module_info->slave_af_info.size = 5;

        module_info->slave_awb_info.offset = 0x0016;
        module_info->slave_awb_info.size = 12;

        module_info->slave_lsc_info.offset = 0x0023;
        module_info->slave_lsc_info.size = 16 + lsc_channel_size * 4;

        module_info->slave_ae_info.offset = module_info->slave_lsc_info.offset +
                                            module_info->slave_lsc_info.size +
                                            1;
        module_info->slave_ae_info.size = 24;
    }

    if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM) {
        /* 8M + 2M (ov8858+ov2680) */
        module_info->slave_af_info.offset = 0x1a10;
        module_info->slave_af_info.size = 5;

        module_info->slave_awb_info.offset = 0x1a16;
        module_info->slave_awb_info.size = 12;

        module_info->slave_lsc_info.offset = 0x1a23;
        module_info->slave_lsc_info.size = 16 + lsc_channel_size * 4;

        if (!strcmp(otp_cxt->dev_name, "ov2680_mipi_raw")) {
            /* ov2680 cmk module, otp 0.4 or 0.5*/
            module_info->slave_ae_info.offset =
                module_info->slave_lsc_info.offset +
                module_info->slave_lsc_info.size + 148 + 1; // lsc_reserve: 148
            module_info->slave_ae_info.size = 31;
        } else {
            module_info->slave_ae_info.offset =
                module_info->slave_lsc_info.offset +
                module_info->slave_lsc_info.size + 1;
            module_info->slave_ae_info.size = 24;
        }
    }

    OTP_LOGD(
        "dual_slave(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
        "ae(0x%x, %d)",
        module_info->slave_af_info.offset, module_info->slave_af_info.size,
        module_info->slave_awb_info.offset, module_info->slave_awb_info.size,
        module_info->slave_lsc_info.offset, module_info->slave_lsc_info.size,
        module_info->slave_ae_info.offset, module_info->slave_ae_info.size);

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_module_data_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    char *module_vendor[23] = {
        "Invalid", "Sunny",   "Truly",   "ReachTech", "Q-Tech",   "Altek",
        "CMK",     "Shine",   "Darling", "Broad",     "Invalid",  "Invalid",
        "Invalid", "Invalid", "Invalid", "Invalid",   "DMEGC",    "Seasons",
        "Sunwin",  "O-Flim",  "Hongshi", "Holitech",  "Zhengqiao"};

    ret = _general_otp_section_checksum(otp_cxt->otp_raw_data.buffer, 0x00, 80,
                                        0x50, module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("module info checksum error!");
        return ret;
    }
    OTP_LOGD("module info checksum passed");

    module_dat->rdm_info.buffer = buffer;
    module_dat->rdm_info.size = 80;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    if (buffer[8]) {
        if (!buffer[9]) {
            module_info->otp_map_index =
                (buffer[6] << 16) | (buffer[7] << 8) | buffer[8];
        } else {
            module_info->otp_map_index = (buffer[6] << 24) | (buffer[7] << 16) |
                                         (buffer[8] << 8) | buffer[9];
        }
    } else {
        module_info->otp_map_index = (buffer[6] << 8) | buffer[7];
    }
    OTP_LOGI("otp_map_index is %x", module_info->otp_map_index);

    module_info->module_id_info.master_vendor_id = buffer[10];
    module_info->module_id_info.master_lens_id = buffer[11];
    module_info->module_id_info.master_vcm_id = buffer[12];
    module_info->module_id_info.slave_vendor_id = buffer[13];
    module_info->module_id_info.slave_lens_id = buffer[14];
    module_info->module_id_info.slave_vcm_id = buffer[15];
    module_info->module_id_info.year = buffer[16];
    module_info->module_id_info.month = buffer[17];
    module_info->module_id_info.day = buffer[18];
    module_info->module_id_info.work_station_id =
        (buffer[19] << 8) | buffer[20];
    module_info->module_id_info.env_record = (buffer[21] << 8) | buffer[22];

    if (module_info->module_id_info.master_vendor_id < 23) {
        OTP_LOGI("master_vendor_id = 0x%x, master module vendor is %s",
                 module_info->module_id_info.master_vendor_id,
                 module_vendor[module_info->module_id_info.master_vendor_id]);
    } else {
        OTP_LOGI("Illegal master module vendor id!");
    }
    if (module_info->module_id_info.slave_vendor_id < 23) {
        OTP_LOGI("slave_vendor_id = 0x%x, slave module vendor is %s",
                 module_info->module_id_info.slave_vendor_id,
                 module_vendor[module_info->module_id_info.slave_vendor_id]);
    } else {
        OTP_LOGI("Illegal slave module vendor id!");
    }

    OTP_LOGI("master_lens_id = 0x%x, master_vcm_id = 0x%x",
             module_info->module_id_info.master_lens_id,
             module_info->module_id_info.master_vcm_id);
    OTP_LOGI("slave_lens_id = 0x%x, slave_vcm_id = 0x%x",
             module_info->module_id_info.slave_lens_id,
             module_info->module_id_info.slave_vcm_id);
    OTP_LOGI(
        "module calibration date = %d-%d-%d", module_info->module_id_info.year,
        module_info->module_id_info.month, module_info->module_id_info.day);
    OTP_LOGI("work_station_id = 0x%x, env_record = 0x%x",
             module_info->module_id_info.work_station_id,
             module_info->module_id_info.env_record);

    module_info->sensor_setting.eeprom_info = buffer[23];
    module_info->sensor_setting.master_setting = buffer[24];
    module_info->sensor_setting.master_ob = buffer[25];
    module_info->sensor_setting.slave_setting = buffer[26];
    module_info->sensor_setting.slave_ob = buffer[27];

    OTP_LOGI("otp_eeprom_info is 0x%x",
             module_info->sensor_setting.eeprom_info);
    switch (module_info->sensor_setting.eeprom_info & 0x03) {
    case 1:
        OTP_LOGI("module with 1st eeprom");
        break;
    case 2:
        OTP_LOGI("module with 2nd eeprom");
        break;
    case 3:
        OTP_LOGI("module with 1st & 2nd eeprom");
        break;
    default:
        OTP_LOGI("invalid eeprom numbers");
        break;
    }
    switch ((module_info->sensor_setting.eeprom_info & 0x0c) >> 2) {
    case 1:
        OTP_LOGI("current eeprom id 1");
        break;
    case 2:
        OTP_LOGI("current eeprom id 2");
        break;
    default:
        OTP_LOGI("invalid eeprom currunt id");
        break;
    }

    OTP_LOGI("master_sensor_setting is 0x%x, master_sensor_ob = %d",
             module_info->sensor_setting.master_setting,
             module_info->sensor_setting.master_ob);
    switch (module_info->sensor_setting.master_setting & 0x03) {
    case 0:
        OTP_LOGI("master sensor raw image bayer pattern: Gr R  B  Gb");
        break;
    case 1:
        OTP_LOGI("master sensor raw image bayer pattern: R  Gr Gb B");
        break;
    case 2:
        OTP_LOGI("master sensor raw image bayer pattern: B  Gb Gr R");
        break;
    case 3:
        OTP_LOGI("master sensor raw image bayer pattern: Gb B  R  Gr");
        break;
    }
    switch ((module_info->sensor_setting.master_setting & 0x0c) >> 2) {
    case 0:
        OTP_LOGI("master sensor normal setting, no mirror, no flip");
        break;
    case 1:
        OTP_LOGI("master sensor with mirror");
        break;
    case 2:
        OTP_LOGI("master sensor with flip");
        break;
    case 3:
        OTP_LOGI("master sensor with mirror & flip");
        break;
    }

    OTP_LOGI("slave_sensor_setting is 0x%x, slave_sensor_ob = %d",
             module_info->sensor_setting.slave_setting,
             module_info->sensor_setting.slave_ob);
    switch (module_info->sensor_setting.slave_setting & 0x03) {
    case 0:
        OTP_LOGI("slave sensor raw image bayer pattern: Gr R  B  Gb");
        break;
    case 1:
        OTP_LOGI("slave sensor raw image bayer pattern: R  Gr Gb B");
        break;
    case 2:
        OTP_LOGI("slave sensor raw image bayer pattern: B  Gb Gr R");
        break;
    case 3:
        OTP_LOGI("slave sensor raw image bayer pattern: Gb B  R  Gr");
        break;
    }
    switch ((module_info->sensor_setting.slave_setting & 0x0c) >> 2) {
    case 0:
        OTP_LOGI("slave sensor normal setting, no mirror, no flip");
        break;
    case 1:
        OTP_LOGI("slave sensor with mirror");
        break;
    case 2:
        OTP_LOGI("slave sensor with flip");
        break;
    case 3:
        OTP_LOGI("slave sensor with mirror & flip");
        break;
    }

    module_info->master_af_info.offset = (buffer[29] << 8) | buffer[28];
    module_info->master_awb_info.offset = (buffer[31] << 8) | buffer[30];
    module_info->master_lsc_info.offset = (buffer[33] << 8) | buffer[32];
    module_info->master_pdaf1_info.offset = (buffer[35] << 8) | buffer[34];
    module_info->master_pdaf2_info.offset = (buffer[37] << 8) | buffer[36];
    module_info->master_ae_info.offset = (buffer[39] << 8) | buffer[38];
    module_info->master_bokeh_info.offset = (buffer[41] << 8) | buffer[40];

    module_info->slave_af_info.offset = (buffer[47] << 8) | buffer[46];
    module_info->slave_awb_info.offset = (buffer[49] << 8) | buffer[48];
    module_info->slave_lsc_info.offset = (buffer[51] << 8) | buffer[50];
    module_info->slave_ae_info.offset = (buffer[53] << 8) | buffer[52];

    module_info->master_af_info.size = buffer[56];
    module_info->master_awb_info.size = buffer[57];
    module_info->master_lsc_info.size = (buffer[59] << 8) | buffer[58];
    module_info->master_pdaf1_info.size = (buffer[61] << 8) | buffer[60];
    module_info->master_pdaf2_info.size = (buffer[63] << 8) | buffer[62];
    module_info->master_ae_info.size = buffer[64];
    module_info->master_bokeh_info.size = (buffer[66] << 8) | buffer[65];

    module_info->slave_af_info.size = buffer[71];
    module_info->slave_awb_info.size = buffer[72];
    module_info->slave_lsc_info.size = (buffer[74] << 8) | buffer[73];
    module_info->slave_ae_info.size = buffer[75];

    OTP_LOGD(
        "master(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
        "pdaf1(0x%x, %d), pdaf2(0x%x, %d), ae(0x%x, %d), bokeh(0x%x, %d)",
        module_info->master_af_info.offset, module_info->master_af_info.size,
        module_info->master_awb_info.offset, module_info->master_awb_info.size,
        module_info->master_lsc_info.offset, module_info->master_lsc_info.size,
        module_info->master_pdaf1_info.offset,
        module_info->master_pdaf1_info.size,
        module_info->master_pdaf2_info.offset,
        module_info->master_pdaf2_info.size, module_info->master_ae_info.offset,
        module_info->master_ae_info.size, module_info->master_bokeh_info.offset,
        module_info->master_bokeh_info.size);

    OTP_LOGD(
        "slave(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
        "ae(0x%x, %d)",
        module_info->slave_af_info.offset, module_info->slave_af_info.size,
        module_info->slave_awb_info.offset, module_info->slave_awb_info.size,
        module_info->slave_lsc_info.offset, module_info->slave_lsc_info.size,
        module_info->slave_ae_info.offset, module_info->slave_ae_info.size);

    module_info->resolution =
        (module_info->sensor_max_width * module_info->sensor_max_height +
         500000) /
        1000000;
    OTP_LOGI("sensor_max_width = %d, sensor_max_height = %d, resolution = %dM",
             module_info->sensor_max_width, module_info->sensor_max_height,
             module_info->resolution);

    OTP_LOGV("X");
    return ret;
}

/*sensor number is 1, only for otp info of only one sensor in one eeprom*/
static cmr_int _general_otp_parse_module_data_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);

    ret = _general_otp_section_checksum(otp_cxt->otp_raw_data.buffer, 0x00, 89,
                                        0x59, module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("module info checksum error!");
        return ret;
    }
    OTP_LOGD("module info checksum passed");

    module_dat->rdm_info.buffer = buffer;
    module_dat->rdm_info.size = 89;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    module_info->otp_map_index =
        (buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];
    OTP_LOGD("otp_map_index is %x", module_info->otp_map_index);

    module_info->module_id_info.year = buffer[10];
    module_info->module_id_info.month = buffer[11];
    module_info->module_id_info.day = buffer[12];
    module_info->module_id_info.sensor_num = buffer[18];
    OTP_LOGI("module calibration date = %d-%d-%d, sensor number = %d",
             module_info->module_id_info.year,
             module_info->module_id_info.month, module_info->module_id_info.day,
             module_info->module_id_info.sensor_num);

    module_info->module_id_info.master_vendor_id = buffer[19];
    module_info->module_id_info.master_lens_id = buffer[20];
    module_info->module_id_info.master_vcm_id = buffer[21];
    OTP_LOGD(
        "master_vendor_id = 0x%x, master_lens_id = 0x%x, master_vcm_id = 0x%x",
        module_info->module_id_info.master_vendor_id,
        module_info->module_id_info.master_lens_id,
        module_info->module_id_info.master_vcm_id);

    module_info->master_af_info.offset = (buffer[26] << 8) | buffer[25];
    module_info->master_af_info.size = (buffer[28] << 8) | buffer[27];
    module_info->master_awb_info.offset = (buffer[30] << 8) | buffer[29];
    module_info->master_awb_info.size = (buffer[32] << 8) | buffer[31];
    module_info->master_lsc_info.offset = (buffer[34] << 8) | buffer[33];
    module_info->master_lsc_info.size = (buffer[36] << 8) | buffer[35];
    module_info->master_pdaf1_info.offset = (buffer[38] << 8) | buffer[37];
    module_info->master_pdaf1_info.size = (buffer[40] << 8) | buffer[39];
    module_info->master_pdaf2_info.offset = (buffer[42] << 8) | buffer[41];
    module_info->master_pdaf2_info.size = (buffer[44] << 8) | buffer[43];
    module_info->master_xtalk_4in1_info.offset = (buffer[46] << 8) | buffer[45];
    module_info->master_xtalk_4in1_info.size = (buffer[48] << 8) | buffer[47];
    module_info->master_dpc_4in1_info.offset = (buffer[50] << 8) | buffer[49];
    module_info->master_dpc_4in1_info.size = (buffer[52] << 8) | buffer[51];
    module_info->master_spw_info.offset = (buffer[54] << 8) | buffer[53];
    module_info->master_spw_info.size = (buffer[56] << 8) | buffer[55];
    module_info->master_bokeh_info.offset = (buffer[58] << 8) | buffer[57];
    module_info->master_bokeh_info.size = (buffer[60] << 8) | buffer[59];
    module_info->master_wt_info.offset = (buffer[62] << 8) | buffer[61];
    module_info->master_wt_info.size = (buffer[64] << 8) | buffer[63];

    OTP_LOGD(
        "master(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
        "pdaf1(0x%x, %d), pdaf2(0x%x, %d), xtalk_4in1(0x%x, %d), "
        "dpc_4in1(0x%x, %d), superwide(0x%x, %d), bokeh(0x%x, %d), w+t(0x%x, "
        "%d)",
        module_info->master_af_info.offset, module_info->master_af_info.size,
        module_info->master_awb_info.offset, module_info->master_awb_info.size,
        module_info->master_lsc_info.offset, module_info->master_lsc_info.size,
        module_info->master_pdaf1_info.offset,
        module_info->master_pdaf1_info.size,
        module_info->master_pdaf2_info.offset,
        module_info->master_pdaf2_info.size,
        module_info->master_xtalk_4in1_info.offset,
        module_info->master_xtalk_4in1_info.size,
        module_info->master_dpc_4in1_info.offset,
        module_info->master_dpc_4in1_info.size,
        module_info->master_spw_info.offset, module_info->master_spw_info.size,
        module_info->master_bokeh_info.offset,
        module_info->master_bokeh_info.size, module_info->master_wt_info.offset,
        module_info->master_wt_info.size);

    module_info->module_id_info.slave_vendor_id = buffer[19];
    module_info->module_id_info.slave_lens_id = buffer[20];
    module_info->module_id_info.slave_vcm_id = buffer[21];
    OTP_LOGD(
        "slave_vendor_id = 0x%x, slave_lens_id = 0x%x, slave_vcm_id = 0x%x",
        module_info->module_id_info.slave_vendor_id,
        module_info->module_id_info.slave_lens_id,
        module_info->module_id_info.slave_vcm_id);

    module_info->slave_af_info.offset = (buffer[26] << 8) | buffer[25];
    module_info->slave_af_info.size = (buffer[28] << 8) | buffer[27];
    module_info->slave_awb_info.offset = (buffer[30] << 8) | buffer[29];
    module_info->slave_awb_info.size = (buffer[32] << 8) | buffer[31];
    module_info->slave_lsc_info.offset = (buffer[34] << 8) | buffer[33];
    module_info->slave_lsc_info.size = (buffer[36] << 8) | buffer[35];
    module_info->slave_pdaf1_info.offset = (buffer[38] << 8) | buffer[37];
    module_info->slave_pdaf1_info.size = (buffer[40] << 8) | buffer[39];
    module_info->slave_pdaf2_info.offset = (buffer[42] << 8) | buffer[41];
    module_info->slave_pdaf2_info.size = (buffer[44] << 8) | buffer[43];
    module_info->slave_xtalk_4in1_info.offset = (buffer[46] << 8) | buffer[45];
    module_info->slave_xtalk_4in1_info.size = (buffer[48] << 8) | buffer[47];
    module_info->slave_dpc_4in1_info.offset = (buffer[50] << 8) | buffer[49];
    module_info->slave_dpc_4in1_info.size = (buffer[52] << 8) | buffer[51];
    module_info->slave_spw_info.offset = (buffer[54] << 8) | buffer[53];
    module_info->slave_spw_info.size = (buffer[56] << 8) | buffer[55];
    module_info->slave_bokeh_info.offset = (buffer[58] << 8) | buffer[57];
    module_info->slave_bokeh_info.size = (buffer[60] << 8) | buffer[59];
    module_info->slave_wt_info.offset = (buffer[62] << 8) | buffer[61];
    module_info->slave_wt_info.size = (buffer[64] << 8) | buffer[63];

    OTP_LOGD(
        "slave(offset, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
        "pdaf1(0x%x, %d), pdaf2(0x%x, %d), xtalk_4in1(0x%x, %d), "
        "dpc_4in1(0x%x, %d), superwide(0x%x, %d), bokeh(0x%x, %d), w+t(0x%x, "
        "%d)",
        module_info->slave_af_info.offset, module_info->slave_af_info.size,
        module_info->slave_awb_info.offset, module_info->slave_awb_info.size,
        module_info->slave_lsc_info.offset, module_info->slave_lsc_info.size,
        module_info->slave_pdaf1_info.offset,
        module_info->slave_pdaf1_info.size,
        module_info->slave_pdaf2_info.offset,
        module_info->slave_pdaf2_info.size,
        module_info->slave_xtalk_4in1_info.offset,
        module_info->slave_xtalk_4in1_info.size,
        module_info->slave_dpc_4in1_info.offset,
        module_info->slave_dpc_4in1_info.size,
        module_info->slave_spw_info.offset, module_info->slave_spw_info.size,
        module_info->slave_bokeh_info.offset,
        module_info->slave_bokeh_info.size, module_info->slave_wt_info.offset,
        module_info->slave_wt_info.size);

    module_info->resolution =
        (module_info->sensor_max_width * module_info->sensor_max_height +
         500000) /
        1000000;
    OTP_LOGI("sensor_max_width = %d, sensor_max_height = %d, resolution = %dM",
             module_info->sensor_max_width, module_info->sensor_max_height,
             module_info->resolution);

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_master_af_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_af_info.offset;
    char value[255];

    if (!module_info->master_af_info.offset) {
        OTP_LOGE("AF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_af_info.size) {
        OTP_LOGE("AF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_af_info.offset,
        module_info->master_af_info.size,
        module_info->master_af_info.offset + module_info->master_af_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AF checksum error");
        return ret;
    }
    OTP_LOGD("AF checksum passed");
    af_cali_dat->rdm_info.buffer = af_src_dat;
    af_cali_dat->rdm_info.size = module_info->master_af_info.size;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct af_data_t af_data;

        af_data.af_version = 0;
        af_data.af_infinity_position = (af_src_dat[1] << 8) | af_src_dat[0];
        af_data.af_macro_position = (af_src_dat[3] << 8) | af_src_dat[2];
        af_data.af_posture = af_src_dat[4];

        OTP_LOGI("af_infinity_position = %d, af_macro_position = %d",
                 af_data.af_infinity_position, af_data.af_macro_position);

        switch (af_data.af_posture & 0x03) {
        case 1:
            OTP_LOGI("AF posture is upward");
            break;
        case 2:
            OTP_LOGI("AF posture is horizon");
            break;
        case 3:
            OTP_LOGI("AF posture is downward");
            break;
        default:
            OTP_LOGI("invalid AF posture");
            break;
        }

        switch ((af_data.af_posture & 0x0c) >> 2) {
        case 0:
            OTP_LOGI("master sensor normal setting, no mirror, no flip");
            break;
        case 1:
            OTP_LOGI("master sensor with mirror");
            break;
        case 2:
            OTP_LOGI("master sensor with flip");
            break;
        case 3:
            OTP_LOGI("master sensor with mirror & flip");
            break;
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_af_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_af_info.offset;
    char value[255];

    /* including dual_slave */
    if (!module_info->slave_af_info.offset) {
        OTP_LOGE("AF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_af_info.size) {
        OTP_LOGE("AF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_af_info.offset,
        module_info->slave_af_info.size,
        module_info->slave_af_info.offset + module_info->slave_af_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AF checksum error");
        return ret;
    }
    OTP_LOGD("AF checksum passed");
    af_cali_dat->rdm_info.buffer = af_src_dat;
    af_cali_dat->rdm_info.size = module_info->slave_af_info.size;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct af_data_t af_data;

        af_data.af_version = 0;
        af_data.af_infinity_position = (af_src_dat[1] << 8) | af_src_dat[0];
        af_data.af_macro_position = (af_src_dat[3] << 8) | af_src_dat[2];
        af_data.af_posture = af_src_dat[4];

        OTP_LOGI("af_infinity_position = %d, af_macro_position = %d",
                 af_data.af_infinity_position, af_data.af_macro_position);

        switch (af_data.af_posture & 0x03) {
        case 1:
            OTP_LOGI("AF posture is upward");
            break;
        case 2:
            OTP_LOGI("AF posture is horizon");
            break;
        case 3:
            OTP_LOGI("AF posture is downward");
            break;
        default:
            OTP_LOGI("invalid AF posture");
            break;
        }

        switch ((af_data.af_posture & 0x0c) >> 2) {
        case 0:
            OTP_LOGI("slave sensor normal setting, no mirror, no flip");
            break;
        case 1:
            OTP_LOGI("slave sensor with mirror");
            break;
        case 2:
            OTP_LOGI("slave sensor with flip");
            break;
        case 3:
            OTP_LOGI("slave sensor with mirror & flip");
            break;
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_master_af_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_af_info.offset;
    char value[255];

    if (!module_info->master_af_info.offset) {
        OTP_LOGE("AF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_af_info.size) {
        OTP_LOGE("AF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_af_info.offset,
        module_info->master_af_info.size,
        module_info->master_af_info.offset + module_info->master_af_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AF checksum error");
        return ret;
    }
    OTP_LOGD("AF checksum passed");
    af_cali_dat->rdm_info.buffer = af_src_dat;
    af_cali_dat->rdm_info.size = module_info->master_af_info.size;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct af_data_t af_data;

        af_data.af_version = af_src_dat[0];
        af_data.af_infinity_position = (af_src_dat[2] << 8) | af_src_dat[1];
        af_data.af_macro_position = (af_src_dat[4] << 8) | af_src_dat[3];
        af_data.af_posture = af_src_dat[5];
        af_data.af_temperature_start = (af_src_dat[7] << 8) | af_src_dat[6];
        af_data.af_temperature_end = (af_src_dat[9] << 8) | af_src_dat[8];

        OTP_LOGI("af_version = %d", af_data.af_version);
        OTP_LOGI("af_infinity_position = %d, af_macro_position = %d",
                 af_data.af_infinity_position, af_data.af_macro_position);

        switch (af_data.af_posture & 0x03) {
        case 1:
            OTP_LOGI("AF posture is upward");
            break;
        case 2:
            OTP_LOGI("AF posture is horizon");
            break;
        case 3:
            OTP_LOGI("AF posture is downward");
            break;
        default:
            OTP_LOGI("invalid AF posture");
            break;
        }

        OTP_LOGI("af_temperature_start = %d, af_temperature_end = %d",
                 af_data.af_temperature_start, af_data.af_temperature_end);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_af_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_af_info.offset;
    char value[255];

    /* including dual_slave */
    if (!module_info->slave_af_info.offset) {
        OTP_LOGE("AF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_af_info.size) {
        OTP_LOGE("AF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_af_info.offset,
        module_info->slave_af_info.size,
        module_info->slave_af_info.offset + module_info->slave_af_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AF checksum error");
        return ret;
    }
    OTP_LOGD("AF checksum passed");
    af_cali_dat->rdm_info.buffer = af_src_dat;
    af_cali_dat->rdm_info.size = module_info->slave_af_info.size;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct af_data_t af_data;

        af_data.af_version = af_src_dat[0];
        af_data.af_infinity_position = (af_src_dat[2] << 8) | af_src_dat[1];
        af_data.af_macro_position = (af_src_dat[4] << 8) | af_src_dat[3];
        af_data.af_posture = af_src_dat[5];
        af_data.af_temperature_start = (af_src_dat[7] << 8) | af_src_dat[6];
        af_data.af_temperature_end = (af_src_dat[9] << 8) | af_src_dat[8];

        OTP_LOGI("af_version = %d", af_data.af_version);
        OTP_LOGI("af_infinity_position = %d, af_macro_position = %d",
                 af_data.af_infinity_position, af_data.af_macro_position);

        switch (af_data.af_posture & 0x03) {
        case 1:
            OTP_LOGI("AF posture is upward");
            break;
        case 2:
            OTP_LOGI("AF posture is horizon");
            break;
        case 3:
            OTP_LOGI("AF posture is downward");
            break;
        default:
            OTP_LOGI("invalid AF posture");
            break;
        }

        OTP_LOGI("af_temperature_start = %d, af_temperature_end = %d",
                 af_data.af_temperature_start, af_data.af_temperature_end);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_af_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    struct otp_parser_section af_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_AF,
                        otp_cxt->sensor_id, &af_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AF checksum error");
        return ret;
    }
    OTP_LOGD("AF checksum passed");
    af_cali_dat->rdm_info.buffer = af_section.data_addr;
    af_cali_dat->rdm_info.size = af_section.data_size;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_master_awb_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_awb_info.offset;
    char value[255];
    static awb_target_packet_t master_golden_awb = {0, 0, 0, 0, 0, 0};

    if (!module_info->master_awb_info.offset) {
        OTP_LOGE("AWB section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_awb_info.size) {
        OTP_LOGE("AWB section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_awb_info.offset,
        module_info->master_awb_info.size,
        module_info->master_awb_info.offset + module_info->master_awb_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = awb_src_dat;
    awb_cali_dat->rdm_info.size = module_info->master_awb_info.size;

    if (!strcmp(otp_cxt->dev_name, "ov13855_mipi_raw")) {
        if (awb_src_dat[6] + awb_src_dat[7] + awb_src_dat[8] + awb_src_dat[9] +
                awb_src_dat[10] + awb_src_dat[11] ==
            0) {
            /* ov13855 sunny module, otp 0.4 or 0.1*/
            master_golden_awb.R = 366;
            master_golden_awb.G = 705;
            master_golden_awb.B = 452;
        } else {
            /* ov13855 otp 0.1 truly module */
            master_golden_awb.R = 391;
            master_golden_awb.G = 736;
            master_golden_awb.B = 453;
        }
    }

    if (!strcmp(otp_cxt->dev_name, "imx258_mipi_raw")) {
        /* imx258 module, early sharkl2*/
        master_golden_awb.R = 369;
        master_golden_awb.G = 736;
        master_golden_awb.B = 452;
    }

    if (!strcmp(otp_cxt->dev_name, "ov8858_mipi_raw")) {
        /* ov8858 cmk module, otp 0.4 */
        master_golden_awb.R = 493;
        master_golden_awb.G = 783;
        master_golden_awb.B = 487;
    }

    if (!strcmp(otp_cxt->dev_name, "sp8407")) {
        /* sp8407 cmk module, otp 0.4 */
        master_golden_awb.R = 393;
        master_golden_awb.G = 630;
        master_golden_awb.B = 421;
    }

    if (!strcmp(otp_cxt->dev_name, "ov8856")) {
        /* ov8856 shine module, otp 0.4 */
        master_golden_awb.R = 493;
        master_golden_awb.G = 753;
        master_golden_awb.B = 449;
    }

    if (master_golden_awb.R) {
        awb_cali_dat->gld_info.buffer = &master_golden_awb;
        awb_cali_dat->gld_info.size = 6;
    } else {
        awb_cali_dat->gld_info.buffer = NULL;
        awb_cali_dat->gld_info.size = 0;
    }

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct awb_data_t awb_data;

        awb_data.awb_version = 0;
        awb_data.awb_random_r = (awb_src_dat[1] << 8) | awb_src_dat[0];
        awb_data.awb_random_g = (awb_src_dat[3] << 8) | awb_src_dat[2];
        awb_data.awb_random_b = (awb_src_dat[5] << 8) | awb_src_dat[4];

        OTP_LOGI("awb_random_r = %d, awb_random_g = %d, awb_random_b = %d",
                 awb_data.awb_random_r, awb_data.awb_random_g,
                 awb_data.awb_random_b);

        awb_data.awb_golden_r = master_golden_awb.R;
        awb_data.awb_golden_g = master_golden_awb.G;
        awb_data.awb_golden_b = master_golden_awb.B;

        OTP_LOGI("awb_golden_r = %d, awb_golden_g = %d, awb_golden_b = %d",
                 awb_data.awb_golden_r, awb_data.awb_golden_g,
                 awb_data.awb_golden_b);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_awb_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_slave */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_awb_info.offset;
    char value[255];
    static awb_target_packet_t slave_golden_awb = {0, 0, 0, 0, 0, 0};

    if (!module_info->slave_awb_info.offset) {
        OTP_LOGE("AWB section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_awb_info.size) {
        OTP_LOGE("AWB section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_awb_info.offset,
        module_info->slave_awb_info.size,
        module_info->slave_awb_info.offset + module_info->slave_awb_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = awb_src_dat;
    awb_cali_dat->rdm_info.size = module_info->slave_awb_info.size;

    if (!strcmp(otp_cxt->dev_name, "ov5675_mipi_raw")) {
        /* ov5675 sunny module, otp 0.3 or 0.1 */
        slave_golden_awb.R = 408;
        slave_golden_awb.G = 721;
        slave_golden_awb.B = 491;
    }

    if (!strcmp(otp_cxt->dev_name, "ov2680_mipi_raw")) {
        /* ov2680 cmk module, otp 0.4 */
        slave_golden_awb.R = 609;
        slave_golden_awb.G = 828;
        slave_golden_awb.B = 508;
    }

    if (slave_golden_awb.R) {
        awb_cali_dat->gld_info.buffer = &slave_golden_awb;
        awb_cali_dat->gld_info.size = 6;
    } else {
        awb_cali_dat->gld_info.buffer = NULL;
        awb_cali_dat->gld_info.size = 0;
    }

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct awb_data_t awb_data;

        awb_data.awb_version = 0;
        awb_data.awb_random_r = (awb_src_dat[1] << 8) | awb_src_dat[0];
        awb_data.awb_random_g = (awb_src_dat[3] << 8) | awb_src_dat[2];
        awb_data.awb_random_b = (awb_src_dat[5] << 8) | awb_src_dat[4];

        OTP_LOGI("awb_random_r = %d, awb_random_g = %d, awb_random_b = %d",
                 awb_data.awb_random_r, awb_data.awb_random_g,
                 awb_data.awb_random_b);

        awb_data.awb_golden_r = slave_golden_awb.R;
        awb_data.awb_golden_g = slave_golden_awb.G;
        awb_data.awb_golden_b = slave_golden_awb.B;

        OTP_LOGI("awb_golden_r = %d, awb_golden_g = %d, awb_golden_b = %d",
                 awb_data.awb_golden_r, awb_data.awb_golden_g,
                 awb_data.awb_golden_b);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_master_awb_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_awb_info.offset;
    char value[255];

    if (!module_info->master_awb_info.offset) {
        OTP_LOGE("AWB section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_awb_info.size) {
        OTP_LOGE("AWB section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_awb_info.offset,
        module_info->master_awb_info.size,
        module_info->master_awb_info.offset + module_info->master_awb_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = awb_src_dat;
    awb_cali_dat->rdm_info.size = module_info->master_awb_info.size;
    awb_cali_dat->gld_info.buffer = NULL;
    awb_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct awb_data_t awb_data;

        awb_data.awb_version = awb_src_dat[0];
        awb_data.awb_random_r = (awb_src_dat[2] << 8) | awb_src_dat[1];
        awb_data.awb_random_g = (awb_src_dat[4] << 8) | awb_src_dat[3];
        awb_data.awb_random_b = (awb_src_dat[6] << 8) | awb_src_dat[5];
        awb_data.awb_golden_r = (awb_src_dat[8] << 8) | awb_src_dat[7];
        awb_data.awb_golden_g = (awb_src_dat[10] << 8) | awb_src_dat[9];
        awb_data.awb_golden_b = (awb_src_dat[12] << 8) | awb_src_dat[11];

        OTP_LOGI("awb_version = %d", awb_data.awb_version);
        OTP_LOGI("awb_random_r = %d, awb_random_g = %d, awb_random_b = %d",
                 awb_data.awb_random_r, awb_data.awb_random_g,
                 awb_data.awb_random_b);
        OTP_LOGI("awb_golden_r = %d, awb_golden_g = %d, awb_golden_b = %d",
                 awb_data.awb_golden_r, awb_data.awb_golden_g,
                 awb_data.awb_golden_b);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_awb_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_slave */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_awb_info.offset;
    char value[255];

    if (!module_info->slave_awb_info.offset) {
        OTP_LOGE("AWB section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_awb_info.size) {
        OTP_LOGE("AWB section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_awb_info.offset,
        module_info->slave_awb_info.size,
        module_info->slave_awb_info.offset + module_info->slave_awb_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = awb_src_dat;
    awb_cali_dat->rdm_info.size = module_info->slave_awb_info.size;
    awb_cali_dat->gld_info.buffer = NULL;
    awb_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct awb_data_t awb_data;

        awb_data.awb_version = awb_src_dat[0];
        awb_data.awb_random_r = (awb_src_dat[2] << 8) | awb_src_dat[1];
        awb_data.awb_random_g = (awb_src_dat[4] << 8) | awb_src_dat[3];
        awb_data.awb_random_b = (awb_src_dat[6] << 8) | awb_src_dat[5];
        awb_data.awb_golden_r = (awb_src_dat[8] << 8) | awb_src_dat[7];
        awb_data.awb_golden_g = (awb_src_dat[10] << 8) | awb_src_dat[9];
        awb_data.awb_golden_b = (awb_src_dat[12] << 8) | awb_src_dat[11];

        OTP_LOGI("awb_version = %d", awb_data.awb_version);
        OTP_LOGI("awb_random_r = %d, awb_random_g = %d, awb_random_b = %d",
                 awb_data.awb_random_r, awb_data.awb_random_g,
                 awb_data.awb_random_b);
        OTP_LOGI("awb_golden_r = %d, awb_golden_g = %d, awb_golden_b = %d",
                 awb_data.awb_golden_r, awb_data.awb_golden_g,
                 awb_data.awb_golden_b);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_awb_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    struct otp_parser_section awb_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_AWB,
                        otp_cxt->sensor_id, &awb_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = awb_section.data_addr;
    awb_cali_dat->rdm_info.size = awb_section.data_size;
    awb_cali_dat->gld_info.buffer = NULL;
    awb_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_master_lsc_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *opt_center_dat = &(otp_cxt->otp_data->opt_center_dat);
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *lsc_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_lsc_info.offset;
    char value[255];

    if (!module_info->master_lsc_info.offset) {
        OTP_LOGE("LSC section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_lsc_info.size) {
        OTP_LOGE("LSC section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_lsc_info.offset,
        module_info->master_lsc_info.size,
        module_info->master_lsc_info.offset + module_info->master_lsc_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    opt_center_dat->rdm_info.buffer = lsc_src_dat;
    opt_center_dat->rdm_info.size = 16;
    opt_center_dat->gld_info.buffer = NULL;
    opt_center_dat->gld_info.size = 0;
    lsc_cali_dat->rdm_info.buffer = lsc_src_dat + 16;
    lsc_cali_dat->rdm_info.size = module_info->master_lsc_info.size - 16;
    lsc_cali_dat->gld_info.buffer = NULL;
    lsc_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct lsc_data_t lsc_data;

        lsc_data.lsc_version = 0;
        lsc_data.lsc_oc_r_x = (lsc_src_dat[1] << 8) | lsc_src_dat[0];
        lsc_data.lsc_oc_r_y = (lsc_src_dat[3] << 8) | lsc_src_dat[2];
        lsc_data.lsc_oc_gr_x = (lsc_src_dat[5] << 8) | lsc_src_dat[4];
        lsc_data.lsc_oc_gr_y = (lsc_src_dat[7] << 8) | lsc_src_dat[6];
        lsc_data.lsc_oc_gb_x = (lsc_src_dat[9] << 8) | lsc_src_dat[8];
        lsc_data.lsc_oc_gb_y = (lsc_src_dat[11] << 8) | lsc_src_dat[10];
        lsc_data.lsc_oc_b_x = (lsc_src_dat[13] << 8) | lsc_src_dat[12];
        lsc_data.lsc_oc_b_y = (lsc_src_dat[15] << 8) | lsc_src_dat[14];

        OTP_LOGI("optical_center: R=(%d,%d), Gr=(%d,%d), Gb=(%d,%d), B=(%d,%d)",
                 lsc_data.lsc_oc_r_x, lsc_data.lsc_oc_r_y, lsc_data.lsc_oc_gr_x,
                 lsc_data.lsc_oc_gr_y, lsc_data.lsc_oc_gb_x,
                 lsc_data.lsc_oc_gb_y, lsc_data.lsc_oc_b_x,
                 lsc_data.lsc_oc_b_y);
        OTP_LOGI("sensor_max_width = %d, sensor_max_height = %d, lsc_grid = %d",
                 module_info->sensor_max_width, module_info->sensor_max_height,
                 module_info->lsc_grid);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_lsc_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_slave */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *opt_center_dat = &(otp_cxt->otp_data->opt_center_dat);
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *lsc_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_lsc_info.offset;
    char value[255];

    if (!module_info->slave_lsc_info.offset) {
        OTP_LOGE("LSC section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_lsc_info.size) {
        OTP_LOGE("LSC section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    if (!strcmp(otp_cxt->dev_name, "ov2680_mipi_raw")) {
        /* ov2680 cmk module, otp 0.4 or 0.5, 148 is reserve */
        ret = _general_otp_section_checksum(
            otp_cxt->otp_raw_data.buffer, module_info->slave_lsc_info.offset,
            module_info->slave_lsc_info.size + 148,
            module_info->slave_lsc_info.offset +
                module_info->slave_lsc_info.size + 148,
            module_info->otp_version);
    } else {
        ret = _general_otp_section_checksum(
            otp_cxt->otp_raw_data.buffer, module_info->slave_lsc_info.offset,
            module_info->slave_lsc_info.size,
            module_info->slave_lsc_info.offset +
                module_info->slave_lsc_info.size,
            module_info->otp_version);
    }
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    opt_center_dat->rdm_info.buffer = lsc_src_dat;
    opt_center_dat->rdm_info.size = 16;
    opt_center_dat->gld_info.buffer = NULL;
    opt_center_dat->gld_info.size = 0;
    lsc_cali_dat->rdm_info.buffer = lsc_src_dat + 16;
    lsc_cali_dat->rdm_info.size = module_info->slave_lsc_info.size - 16;
    lsc_cali_dat->gld_info.buffer = NULL;
    lsc_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct lsc_data_t lsc_data;

        lsc_data.lsc_version = 0;
        lsc_data.lsc_oc_r_x = (lsc_src_dat[1] << 8) | lsc_src_dat[0];
        lsc_data.lsc_oc_r_y = (lsc_src_dat[3] << 8) | lsc_src_dat[2];
        lsc_data.lsc_oc_gr_x = (lsc_src_dat[5] << 8) | lsc_src_dat[4];
        lsc_data.lsc_oc_gr_y = (lsc_src_dat[7] << 8) | lsc_src_dat[6];
        lsc_data.lsc_oc_gb_x = (lsc_src_dat[9] << 8) | lsc_src_dat[8];
        lsc_data.lsc_oc_gb_y = (lsc_src_dat[11] << 8) | lsc_src_dat[10];
        lsc_data.lsc_oc_b_x = (lsc_src_dat[13] << 8) | lsc_src_dat[12];
        lsc_data.lsc_oc_b_y = (lsc_src_dat[15] << 8) | lsc_src_dat[14];

        OTP_LOGI("optical_center: R=(%d,%d), Gr=(%d,%d), Gb=(%d,%d), B=(%d,%d)",
                 lsc_data.lsc_oc_r_x, lsc_data.lsc_oc_r_y, lsc_data.lsc_oc_gr_x,
                 lsc_data.lsc_oc_gr_y, lsc_data.lsc_oc_gb_x,
                 lsc_data.lsc_oc_gb_y, lsc_data.lsc_oc_b_x,
                 lsc_data.lsc_oc_b_y);
        OTP_LOGI("sensor_max_width = %d, sensor_max_height = %d, lsc_grid = %d",
                 module_info->sensor_max_width, module_info->sensor_max_height,
                 module_info->lsc_grid);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_master_lsc_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *lsc_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_lsc_info.offset;
    char value[255];

    if (!module_info->master_lsc_info.offset) {
        OTP_LOGE("LSC section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_lsc_info.size) {
        OTP_LOGE("LSC section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_lsc_info.offset,
        module_info->master_lsc_info.size,
        module_info->master_lsc_info.offset + module_info->master_lsc_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    lsc_cali_dat->rdm_info.buffer = lsc_src_dat;
    lsc_cali_dat->rdm_info.size = module_info->master_lsc_info.size;
    lsc_cali_dat->gld_info.buffer = NULL;
    lsc_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct lsc_data_t lsc_data;

        lsc_data.lsc_version = lsc_src_dat[0];
        lsc_data.lsc_oc_r_x = (lsc_src_dat[2] << 8) | lsc_src_dat[1];
        lsc_data.lsc_oc_r_y = (lsc_src_dat[4] << 8) | lsc_src_dat[3];
        lsc_data.lsc_oc_gr_x = (lsc_src_dat[6] << 8) | lsc_src_dat[5];
        lsc_data.lsc_oc_gr_y = (lsc_src_dat[8] << 8) | lsc_src_dat[7];
        lsc_data.lsc_oc_gb_x = (lsc_src_dat[10] << 8) | lsc_src_dat[9];
        lsc_data.lsc_oc_gb_y = (lsc_src_dat[12] << 8) | lsc_src_dat[11];
        lsc_data.lsc_oc_b_x = (lsc_src_dat[14] << 8) | lsc_src_dat[13];
        lsc_data.lsc_oc_b_y = (lsc_src_dat[16] << 8) | lsc_src_dat[15];
        lsc_data.lsc_img_width = (lsc_src_dat[18] << 8) | lsc_src_dat[17];
        lsc_data.lsc_img_height = (lsc_src_dat[20] << 8) | lsc_src_dat[19];
        lsc_data.lsc_grid = lsc_src_dat[21];
        module_info->lsc_grid = lsc_data.lsc_grid;

        OTP_LOGI("lsc_version = %d", lsc_data.lsc_version);
        OTP_LOGI("optical_center: R=(%d,%d), Gr=(%d,%d), Gb=(%d,%d), B=(%d,%d)",
                 lsc_data.lsc_oc_r_x, lsc_data.lsc_oc_r_y, lsc_data.lsc_oc_gr_x,
                 lsc_data.lsc_oc_gr_y, lsc_data.lsc_oc_gb_x,
                 lsc_data.lsc_oc_gb_y, lsc_data.lsc_oc_b_x,
                 lsc_data.lsc_oc_b_y);
        OTP_LOGI("lsc_img_width = %d, lsc_img_height = %d, lsc_grid = %d",
                 lsc_data.lsc_img_width, lsc_data.lsc_img_height,
                 lsc_data.lsc_grid);

        lsc_data.lsc_channel_size = _general_otp_get_lsc_channel_size(
            lsc_data.lsc_img_width, lsc_data.lsc_img_height, lsc_data.lsc_grid);

        if ((22 + lsc_data.lsc_channel_size * 4) !=
            module_info->master_lsc_info.size) {
            OTP_LOGI("module info master_lsc_size doesn't match "
                     "lsc_channel_size rule");
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_lsc_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_slave */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *lsc_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_lsc_info.offset;
    char value[255];

    if (!module_info->slave_lsc_info.offset) {
        OTP_LOGE("LSC section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_lsc_info.size) {
        OTP_LOGE("LSC section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_lsc_info.offset,
        module_info->slave_lsc_info.size,
        module_info->slave_lsc_info.offset + module_info->slave_lsc_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    lsc_cali_dat->rdm_info.buffer = lsc_src_dat;
    lsc_cali_dat->rdm_info.size = module_info->slave_lsc_info.size;
    lsc_cali_dat->gld_info.buffer = NULL;
    lsc_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct lsc_data_t lsc_data;

        lsc_data.lsc_version = lsc_src_dat[0];
        lsc_data.lsc_oc_r_x = (lsc_src_dat[2] << 8) | lsc_src_dat[1];
        lsc_data.lsc_oc_r_y = (lsc_src_dat[4] << 8) | lsc_src_dat[3];
        lsc_data.lsc_oc_gr_x = (lsc_src_dat[6] << 8) | lsc_src_dat[5];
        lsc_data.lsc_oc_gr_y = (lsc_src_dat[8] << 8) | lsc_src_dat[7];
        lsc_data.lsc_oc_gb_x = (lsc_src_dat[10] << 8) | lsc_src_dat[9];
        lsc_data.lsc_oc_gb_y = (lsc_src_dat[12] << 8) | lsc_src_dat[11];
        lsc_data.lsc_oc_b_x = (lsc_src_dat[14] << 8) | lsc_src_dat[13];
        lsc_data.lsc_oc_b_y = (lsc_src_dat[16] << 8) | lsc_src_dat[15];
        lsc_data.lsc_img_width = (lsc_src_dat[18] << 8) | lsc_src_dat[17];
        lsc_data.lsc_img_height = (lsc_src_dat[20] << 8) | lsc_src_dat[19];
        lsc_data.lsc_grid = lsc_src_dat[21];
        module_info->lsc_grid = lsc_data.lsc_grid;

        OTP_LOGI("lsc_version = %d", lsc_data.lsc_version);
        OTP_LOGI("optical_center: R=(%d,%d), Gr=(%d,%d), Gb=(%d,%d), B=(%d,%d)",
                 lsc_data.lsc_oc_r_x, lsc_data.lsc_oc_r_y, lsc_data.lsc_oc_gr_x,
                 lsc_data.lsc_oc_gr_y, lsc_data.lsc_oc_gb_x,
                 lsc_data.lsc_oc_gb_y, lsc_data.lsc_oc_b_x,
                 lsc_data.lsc_oc_b_y);
        OTP_LOGI("lsc_img_width = %d, lsc_img_height = %d, lsc_grid = %d",
                 lsc_data.lsc_img_width, lsc_data.lsc_img_height,
                 lsc_data.lsc_grid);

        lsc_data.lsc_channel_size = _general_otp_get_lsc_channel_size(
            lsc_data.lsc_img_width, lsc_data.lsc_img_height, lsc_data.lsc_grid);

        if ((22 + lsc_data.lsc_channel_size * 4) !=
            module_info->slave_lsc_info.size) {
            OTP_LOGI("module info slave_lsc_size doesn't match "
                     "lsc_channel_size rule");
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_lsc_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    struct otp_parser_section lsc_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_LSC,
                        otp_cxt->sensor_id, &lsc_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    lsc_cali_dat->rdm_info.buffer = lsc_section.data_addr;
    lsc_cali_dat->rdm_info.size = lsc_section.data_size;
    lsc_cali_dat->gld_info.buffer = NULL;
    lsc_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_master_pdaf(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
    cmr_u8 *pdaf_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_pdaf1_info.offset;
    char value[255];

    if (!module_info->master_pdaf1_info.offset) {
        OTP_LOGE("PDAF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_pdaf1_info.size) {
        OTP_LOGE("PDAF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(otp_cxt->otp_raw_data.buffer,
                                        module_info->master_pdaf1_info.offset,
                                        module_info->master_pdaf1_info.size,
                                        module_info->master_pdaf1_info.offset +
                                            module_info->master_pdaf1_info.size,
                                        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("PDAF checksum error");
        return ret;
    }
    OTP_LOGD("PDAF checksum passed");
    pdaf_cali_dat->rdm_info.buffer = pdaf_src_dat;
    pdaf_cali_dat->rdm_info.size = module_info->master_pdaf1_info.size;
    pdaf_cali_dat->gld_info.buffer = NULL;
    pdaf_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        if (module_info->otp_version == OTP_1_0) {
            cmr_u8 pdaf_version = pdaf_src_dat[0];
            OTP_LOGI("pdaf_version = %d", pdaf_version);
        }
    }

    OTP_LOGV("X");
    return ret;
}
static cmr_int _general_otp_parse_pdaf_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
    struct otp_parser_section pdaf_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_PDAF1,
                        otp_cxt->sensor_id, &pdaf_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("PDAF checksum error");
        return ret;
    }
    OTP_LOGD("PDAF checksum passed");
    pdaf_cali_dat->rdm_info.buffer = pdaf_section.data_addr;
    pdaf_cali_dat->rdm_info.size = pdaf_section.data_size;
    pdaf_cali_dat->gld_info.buffer = NULL;
    pdaf_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_master_ae_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_ae_info.offset;
    char value[255];

    if (!module_info->master_ae_info.offset) {
        OTP_LOGE("AE section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_ae_info.size) {
        OTP_LOGE("AE section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_ae_info.offset,
        module_info->master_ae_info.size,
        module_info->master_ae_info.offset + module_info->master_ae_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AE checksum error");
        return ret;
    }
    OTP_LOGD("AE checksum passed");
    ae_cali_dat->rdm_info.buffer = ae_src_dat;
    ae_cali_dat->rdm_info.size = module_info->master_ae_info.size;
    ae_cali_dat->gld_info.buffer = NULL;
    ae_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct ae_data_t ae_data;

        ae_data.ae_version = 0;
        ae_data.ae_target_lum = (ae_src_dat[1] << 8) | ae_src_dat[0];
        ae_data.ae_gain_1x_exp = (ae_src_dat[5] << 24) | (ae_src_dat[4] << 16) |
                                 (ae_src_dat[3] << 8) | ae_src_dat[2];
        ae_data.ae_gain_2x_exp = (ae_src_dat[9] << 24) | (ae_src_dat[8] << 16) |
                                 (ae_src_dat[7] << 8) | ae_src_dat[6];
        ae_data.ae_gain_4x_exp = (ae_src_dat[13] << 24) |
                                 (ae_src_dat[12] << 16) |
                                 (ae_src_dat[11] << 8) | ae_src_dat[10];
        ae_data.ae_gain_8x_exp = (ae_src_dat[17] << 24) |
                                 (ae_src_dat[16] << 16) |
                                 (ae_src_dat[15] << 8) | ae_src_dat[14];

        OTP_LOGI("ae_target_lum = %d", ae_data.ae_target_lum);
        OTP_LOGI("ae_gain_1x_exp = %d us, ae_gain_2x_exp = %d us, "
                 "ae_gain_4x_exp = %d us, ae_gain_8x_exp = %d us",
                 ae_data.ae_gain_1x_exp, ae_data.ae_gain_2x_exp,
                 ae_data.ae_gain_4x_exp, ae_data.ae_gain_8x_exp);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_ae_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_slave */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_ae_info.offset;
    char value[255];

    if (!module_info->slave_ae_info.offset) {
        OTP_LOGE("AE section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_ae_info.size) {
        OTP_LOGE("AE section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_ae_info.offset,
        module_info->slave_ae_info.size,
        module_info->slave_ae_info.offset + module_info->slave_ae_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AE checksum error");
        return ret;
    }
    OTP_LOGD("AE checksum passed");
    ae_cali_dat->rdm_info.buffer = ae_src_dat;
    ae_cali_dat->rdm_info.size = module_info->slave_ae_info.size;
    ae_cali_dat->gld_info.buffer = NULL;
    ae_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct ae_data_t ae_data;

        ae_data.ae_version = 0;
        ae_data.ae_target_lum = (ae_src_dat[1] << 8) | ae_src_dat[0];
        ae_data.ae_gain_1x_exp = (ae_src_dat[5] << 24) | (ae_src_dat[4] << 16) |
                                 (ae_src_dat[3] << 8) | ae_src_dat[2];
        ae_data.ae_gain_2x_exp = (ae_src_dat[9] << 24) | (ae_src_dat[8] << 16) |
                                 (ae_src_dat[7] << 8) | ae_src_dat[6];
        ae_data.ae_gain_4x_exp = (ae_src_dat[13] << 24) |
                                 (ae_src_dat[12] << 16) |
                                 (ae_src_dat[11] << 8) | ae_src_dat[10];
        ae_data.ae_gain_8x_exp = (ae_src_dat[17] << 24) |
                                 (ae_src_dat[16] << 16) |
                                 (ae_src_dat[15] << 8) | ae_src_dat[14];

        OTP_LOGI("ae_target_lum = %d", ae_data.ae_target_lum);
        OTP_LOGI("ae_gain_1x_exp = %d us, ae_gain_2x_exp = %d us, "
                 "ae_gain_4x_exp = %d us, ae_gain_8x_exp = %d us",
                 ae_data.ae_gain_1x_exp, ae_data.ae_gain_2x_exp,
                 ae_data.ae_gain_4x_exp, ae_data.ae_gain_8x_exp);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_master_ae_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_ae_info.offset;
    char value[255];

    if (!module_info->master_ae_info.offset) {
        OTP_LOGE("AE section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_ae_info.size) {
        OTP_LOGE("AE section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->master_ae_info.offset,
        module_info->master_ae_info.size,
        module_info->master_ae_info.offset + module_info->master_ae_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AE checksum error");
        return ret;
    }
    OTP_LOGD("AE checksum passed");
    ae_cali_dat->rdm_info.buffer = ae_src_dat;
    ae_cali_dat->rdm_info.size = module_info->master_ae_info.size;
    ae_cali_dat->gld_info.buffer = NULL;
    ae_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct ae_data_t ae_data;

        ae_data.ae_version = ae_src_dat[0];
        ae_data.ae_target_lum = (ae_src_dat[2] << 8) | ae_src_dat[1];
        ae_data.ae_gain_1x_exp = (ae_src_dat[6] << 24) | (ae_src_dat[5] << 16) |
                                 (ae_src_dat[4] << 8) | ae_src_dat[3];
        ae_data.ae_gain_2x_exp = (ae_src_dat[10] << 24) |
                                 (ae_src_dat[9] << 16) | (ae_src_dat[8] << 8) |
                                 ae_src_dat[7];
        ae_data.ae_gain_4x_exp = (ae_src_dat[14] << 24) |
                                 (ae_src_dat[13] << 16) |
                                 (ae_src_dat[12] << 8) | ae_src_dat[11];
        ae_data.ae_gain_8x_exp = (ae_src_dat[18] << 24) |
                                 (ae_src_dat[17] << 16) |
                                 (ae_src_dat[16] << 8) | ae_src_dat[15];

        OTP_LOGI("ae_version = %d", ae_data.ae_version);
        OTP_LOGI("ae_target_lum = %d", ae_data.ae_target_lum);
        OTP_LOGI("ae_gain_1x_exp = %d us, ae_gain_2x_exp = %d us, "
                 "ae_gain_4x_exp = %d us, ae_gain_8x_exp = %d us",
                 ae_data.ae_gain_1x_exp, ae_data.ae_gain_2x_exp,
                 ae_data.ae_gain_4x_exp, ae_data.ae_gain_8x_exp);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_slave_ae_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_slave */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->slave_ae_info.offset;
    char value[255];

    if (!module_info->slave_ae_info.offset) {
        OTP_LOGE("AE section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->slave_ae_info.size) {
        OTP_LOGE("AE section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer, module_info->slave_ae_info.offset,
        module_info->slave_ae_info.size,
        module_info->slave_ae_info.offset + module_info->slave_ae_info.size,
        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AE checksum error");
        return ret;
    }
    OTP_LOGD("AE checksum passed");
    ae_cali_dat->rdm_info.buffer = ae_src_dat;
    ae_cali_dat->rdm_info.size = module_info->slave_ae_info.size;
    ae_cali_dat->gld_info.buffer = NULL;
    ae_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct ae_data_t ae_data;

        ae_data.ae_version = ae_src_dat[0];
        ae_data.ae_target_lum = (ae_src_dat[2] << 8) | ae_src_dat[1];
        ae_data.ae_gain_1x_exp = (ae_src_dat[6] << 24) | (ae_src_dat[5] << 16) |
                                 (ae_src_dat[4] << 8) | ae_src_dat[3];
        ae_data.ae_gain_2x_exp = (ae_src_dat[10] << 24) |
                                 (ae_src_dat[9] << 16) | (ae_src_dat[8] << 8) |
                                 ae_src_dat[7];
        ae_data.ae_gain_4x_exp = (ae_src_dat[14] << 24) |
                                 (ae_src_dat[13] << 16) |
                                 (ae_src_dat[12] << 8) | ae_src_dat[11];
        ae_data.ae_gain_8x_exp = (ae_src_dat[18] << 24) |
                                 (ae_src_dat[17] << 16) |
                                 (ae_src_dat[16] << 8) | ae_src_dat[15];

        OTP_LOGI("ae_version = %d", ae_data.ae_version);
        OTP_LOGI("ae_target_lum = %d", ae_data.ae_target_lum);
        OTP_LOGI("ae_gain_1x_exp = %d us, ae_gain_2x_exp = %d us, "
                 "ae_gain_4x_exp = %d us, ae_gain_8x_exp = %d us",
                 ae_data.ae_gain_1x_exp, ae_data.ae_gain_2x_exp,
                 ae_data.ae_gain_4x_exp, ae_data.ae_gain_8x_exp);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_xtalk_4in1_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *xtalk_4in1_cali_dat =
        &(otp_cxt->otp_data->xtalk_4in1_cali_dat);
    struct otp_parser_section xtalk_4in1_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer,
                        OTP_PARSER_SECTION_XTALK_4IN1, otp_cxt->sensor_id,
                        &xtalk_4in1_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("XTALK_4IN1 checksum error");
        return ret;
    }
    OTP_LOGD("XTALK_4IN1 checksum passed");
    xtalk_4in1_cali_dat->rdm_info.buffer = xtalk_4in1_section.data_addr;
    xtalk_4in1_cali_dat->rdm_info.size = xtalk_4in1_section.data_size;
    xtalk_4in1_cali_dat->gld_info.buffer = NULL;
    xtalk_4in1_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_dpc_4in1_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *dpc_4in1_cali_dat =
        &(otp_cxt->otp_data->dpc_4in1_cali_dat);
    struct otp_parser_section dpc_4in1_section;

    ret =
        otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_DPC_4IN1,
                      otp_cxt->sensor_id, &dpc_4in1_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("DPC_4IN1 checksum error");
        return ret;
    }
    OTP_LOGD("DPC_4IN1 checksum passed");
    dpc_4in1_cali_dat->rdm_info.buffer = dpc_4in1_section.data_addr;
    dpc_4in1_cali_dat->rdm_info.size = dpc_4in1_section.data_size;
    dpc_4in1_cali_dat->gld_info.buffer = NULL;
    dpc_4in1_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int _general_otp_parse_spw_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *spw_cali_dat = &(otp_cxt->otp_data->spw_cali_dat);
    struct otp_parser_section spw_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_SPW,
                        otp_cxt->sensor_id, &spw_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("SPW checksum error");
        return ret;
    }
    OTP_LOGD("SPW checksum passed");
    spw_cali_dat->rdm_info.buffer = spw_section.data_addr;
    spw_cali_dat->rdm_info.size = spw_section.data_size;
    spw_cali_dat->gld_info.buffer = NULL;
    spw_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

static cmr_int
_general_otp_parse_master_dualcam_0v4(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *dualcam_cali_dat =
        &(otp_cxt->otp_data->dual_cam_cali_dat);
    cmr_u8 *dualcam_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_bokeh_info.offset;
    char value[255];

    if (!module_info->master_bokeh_info.offset) {
        OTP_LOGE("Dualcam section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_bokeh_info.size) {
        OTP_LOGE("Dualcam section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    if (module_info->otp_version == OTP_0_5) {
        ret = _general_otp_section_checksum(
            otp_cxt->otp_raw_data.buffer, module_info->master_bokeh_info.offset,
            230, module_info->master_bokeh_info.offset + 230,
            module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("otp v0.5 dualcam 230bytes part checksum error");
            return ret;
        }
        ret = _general_otp_section_checksum(
            otp_cxt->otp_raw_data.buffer,
            module_info->master_bokeh_info.offset + 231, 25,
            module_info->master_bokeh_info.offset + 256,
            module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("otp v0.5 dualcam new part checksum error");
            return ret;
        }
    } else {
        ret = _general_otp_section_checksum(
            otp_cxt->otp_raw_data.buffer, module_info->master_bokeh_info.offset,
            module_info->master_bokeh_info.size,
            module_info->master_bokeh_info.offset +
                module_info->master_bokeh_info.size,
            module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("Dualcam 230bytes checksum error");
            return ret;
        }
    }

    OTP_LOGD("Dualcam checksum passed");
    dualcam_cali_dat->rdm_info.buffer = dualcam_src_dat;
    dualcam_cali_dat->rdm_info.size = module_info->master_bokeh_info.size;
    dualcam_cali_dat->gld_info.buffer = NULL;
    dualcam_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct dualcam_data_t dualcam_data;

        dualcam_data.dualcam_version = 0;
        dualcam_data.dualcam_vcm_position =
            (dualcam_src_dat[229] << 8) | dualcam_src_dat[228];
        OTP_LOGI("dualcam_vcm_position = %d",
                 dualcam_data.dualcam_vcm_position);

        if (module_info->otp_version == OTP_0_5) {
            dualcam_data.dualcam_location = dualcam_src_dat[231];
            OTP_LOGI("dualcam_location = %d", dualcam_data.dualcam_location);

            switch (dualcam_data.dualcam_location) {
            case 1:
                OTP_LOGI("back dual camera horizontal");
                break;
            case 2:
                OTP_LOGI("front dual camera horizontal");
                break;
            case 3:
                OTP_LOGI("back dual camera vertical");
                break;
            default:
                OTP_LOGI("illegal dual camera location");
                break;
            }
        } else {
            dualcam_data.dualcam_location = 0;
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int
_general_otp_parse_master_dualcam_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *dualcam_cali_dat =
        &(otp_cxt->otp_data->dual_cam_cali_dat);
    cmr_u8 *dualcam_src_dat =
        otp_cxt->otp_raw_data.buffer + module_info->master_bokeh_info.offset;
    char value[255];

    if (!module_info->master_bokeh_info.offset) {
        OTP_LOGE("Dualcam section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_bokeh_info.size) {
        OTP_LOGE("Dualcam section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _general_otp_section_checksum(otp_cxt->otp_raw_data.buffer,
                                        module_info->master_bokeh_info.offset,
                                        module_info->master_bokeh_info.size,
                                        module_info->master_bokeh_info.offset +
                                            module_info->master_bokeh_info.size,
                                        module_info->otp_version);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("Dualcam checksum error");
        return ret;
    }
    OTP_LOGD("Dualcam checksum passed");
    dualcam_cali_dat->rdm_info.buffer = dualcam_src_dat;
    dualcam_cali_dat->rdm_info.size = module_info->master_bokeh_info.size;
    dualcam_cali_dat->gld_info.buffer = NULL;
    dualcam_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        struct dualcam_data_t dualcam_data;

        dualcam_data.dualcam_version = dualcam_src_dat[0];
        dualcam_data.dualcam_vcm_position =
            (dualcam_src_dat[254] << 8) | dualcam_src_dat[253];
        OTP_LOGI("dualcam_version = %d", dualcam_data.dualcam_version);
        OTP_LOGI("dualcam_vcm_position = %d",
                 dualcam_data.dualcam_vcm_position);

        if (dualcam_data.dualcam_version == 2) {
            dualcam_data.dualcam_location = dualcam_src_dat[255];
            OTP_LOGI("dualcam_location = %d", dualcam_data.dualcam_location);

            switch (dualcam_data.dualcam_location) {
            case 1:
                OTP_LOGI("back dual camera horizontal");
                break;
            case 2:
                OTP_LOGI("front dual camera horizontal");
                break;
            case 3:
                OTP_LOGI("back dual camera vertical");
                break;
            default:
                OTP_LOGI("illegal dual camera location");
                break;
            }
        } else if (dualcam_data.dualcam_version == 1) {
            dualcam_data.dualcam_location = 0;
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_parse_bokeh_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *dualcam_cali_dat =
        &(otp_cxt->otp_data->dual_cam_cali_dat);
    struct otp_parser_section bokeh_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_BOKEH,
                        otp_cxt->sensor_id, &bokeh_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("BOKEH checksum error");
        return ret;
    }
    OTP_LOGD("BOKEH checksum passed");
    dualcam_cali_dat->rdm_info.buffer = bokeh_section.data_addr;
    dualcam_cali_dat->rdm_info.size = bokeh_section.data_size;
    dualcam_cali_dat->gld_info.buffer = NULL;
    dualcam_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}
static cmr_int _general_otp_parse_wt_1v1(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *wt_cali_dat = &(otp_cxt->otp_data->wt_cali_dat);
    struct otp_parser_section wt_section;

    ret = otp_parser_v1(otp_cxt->otp_raw_data.buffer, OTP_PARSER_SECTION_WT,
                        otp_cxt->sensor_id, &wt_section);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("W+T checksum error");
        return ret;
    }
    OTP_LOGD("W+T checksum passed");
    wt_cali_dat->rdm_info.buffer = wt_section.data_addr;
    wt_cali_dat->rdm_info.size = wt_section.data_size;
    wt_cali_dat->gld_info.buffer = NULL;
    wt_cali_dat->gld_info.size = 0;

    OTP_LOGD("X");
    return ret;
}

/*pass data to ispalg or depth*/
static cmr_int _general_otp_compatible_convert_single(cmr_handle otp_drv_handle,
                                                      void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    struct sensor_otp_cust_info *convert_data = NULL;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    char *otp_ver[] = {"0",   "0.1", "0.2", "0.3", "0.4", "0.5",
                       "0.6", "0.7", "0.8", "0.9", "1.0", "1.1"};

    if (otp_cxt->compat_convert_data) {
        convert_data = otp_cxt->compat_convert_data;
    } else {
        OTP_LOGE("otp convert data buffer is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    if (otp_cxt->otp_module_info.otp_version == VER_ERROR) {
        OTP_LOGE("otp version error! will not pass data to isp!");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;

    convert_data->otp_vendor = OTP_VENDOR_SINGLE;

    convert_data->single_otp.module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;
    convert_data->single_otp.af_info =
        (struct sensor_otp_section_info *)&format_data->af_cali_dat;
    convert_data->single_otp.iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
    convert_data->single_otp.optical_center_info =
        (struct sensor_otp_section_info *)&format_data->opt_center_dat;
    convert_data->single_otp.lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
    convert_data->single_otp.pdaf_info =
        (struct sensor_otp_section_info *)&format_data->pdaf_cali_dat;
    convert_data->single_otp.xtalk_4in1_info =
        (struct sensor_otp_section_info *)&format_data->xtalk_4in1_cali_dat;
    convert_data->single_otp.dpc_4in1_info =
        (struct sensor_otp_section_info *)&format_data->dpc_4in1_cali_dat;
    convert_data->single_otp.spw_info =
        (struct sensor_otp_section_info *)&format_data->spw_cali_dat;

    OTP_LOGD("sensor_id:%d, otp_version:%s, single(addr, size):total(%p, %d), "
             "module_info(%p, %d)",
             otp_cxt->sensor_id, otp_ver[otp_cxt->otp_module_info.otp_version],
             convert_data->total_otp.data_ptr, convert_data->total_otp.size,
             convert_data->single_otp.module_info->rdm_info.data_addr,
             convert_data->single_otp.module_info->rdm_info.data_size);
    OTP_LOGD("single(addr, size):af(%p, %d), awb(%p, %d), oc(%p, %d), lsc(%p, "
             "%d), pdaf1(%p, %d)",
             convert_data->single_otp.af_info->rdm_info.data_addr,
             convert_data->single_otp.af_info->rdm_info.data_size,
             convert_data->single_otp.iso_awb_info->rdm_info.data_addr,
             convert_data->single_otp.iso_awb_info->rdm_info.data_size,
             convert_data->single_otp.optical_center_info->rdm_info.data_addr,
             convert_data->single_otp.optical_center_info->rdm_info.data_size,
             convert_data->single_otp.lsc_info->rdm_info.data_addr,
             convert_data->single_otp.lsc_info->rdm_info.data_size,
             convert_data->single_otp.pdaf_info->rdm_info.data_addr,
             convert_data->single_otp.pdaf_info->rdm_info.data_size);
    OTP_LOGD("single(addr, size): xtalk_4in1(%p, %d), dpc_4in1(%p, %d), "
             "superwide(%p, %d)",
             convert_data->single_otp.xtalk_4in1_info->rdm_info.data_addr,
             convert_data->single_otp.xtalk_4in1_info->rdm_info.data_size,
             convert_data->single_otp.dpc_4in1_info->rdm_info.data_addr,
             convert_data->single_otp.dpc_4in1_info->rdm_info.data_size,
             convert_data->single_otp.spw_info->rdm_info.data_addr,
             convert_data->single_otp.spw_info->rdm_info.data_size);

    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_compatible_convert_master(cmr_handle otp_drv_handle,
                                                      void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    struct sensor_otp_cust_info *convert_data = NULL;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    cmr_u8 *dual_flag_ptr = (cmr_u8 *)p_val->pval;
    char *otp_ver[] = {"0",   "0.1", "0.2", "0.3", "0.4", "0.5",
                       "0.6", "0.7", "0.8", "0.9", "1.0", "1.1"};

    if (otp_cxt->compat_convert_data) {
        convert_data = otp_cxt->compat_convert_data;
    } else {
        OTP_LOGE("otp convert data buffer is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    if (otp_cxt->otp_module_info.otp_version == VER_ERROR) {
        OTP_LOGE("otp version error! will not pass data to isp!");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;

    if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM) {
        convert_data->otp_vendor = OTP_VENDOR_SINGLE_CAM_DUAL;
    } else {
        convert_data->otp_vendor = OTP_VENDOR_DUAL_CAM_DUAL;
    }

    convert_data->dual_otp.master_module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;
    convert_data->dual_otp.master_af_info =
        (struct sensor_otp_section_info *)&format_data->af_cali_dat;
    convert_data->dual_otp.master_iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
    convert_data->dual_otp.master_optical_center_info =
        (struct sensor_otp_section_info *)&format_data->opt_center_dat;
    convert_data->dual_otp.master_lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
    convert_data->dual_otp.master_pdaf_info =
        (struct sensor_otp_section_info *)&format_data->pdaf_cali_dat;
    convert_data->dual_otp.master_ae_info =
        (struct sensor_otp_section_info *)&format_data->ae_cali_dat;
    convert_data->dual_otp.master_xtalk_4in1_info =
        (struct sensor_otp_section_info *)&format_data->xtalk_4in1_cali_dat;
    convert_data->dual_otp.master_dpc_4in1_info =
        (struct sensor_otp_section_info *)&format_data->dpc_4in1_cali_dat;
    convert_data->dual_otp.master_spw_info =
        (struct sensor_otp_section_info *)&format_data->spw_cali_dat;

    if (dual_flag_ptr)
        convert_data->dual_otp.dual_flag = *dual_flag_ptr;
    else
        convert_data->dual_otp.dual_flag = 1;

    if (convert_data->dual_otp.dual_flag == 1) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->dual_cam_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->dual_cam_cali_dat.rdm_info.size;
    } else if (convert_data->dual_otp.dual_flag == 3) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->spw_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->spw_cali_dat.rdm_info.size;
    } else if (convert_data->dual_otp.dual_flag == 5 ||
               convert_data->dual_otp.dual_flag == 6) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->wt_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->wt_cali_dat.rdm_info.size;
    } else {
        CMR_LOGD("invalid dual_flag:%d", convert_data->dual_otp.dual_flag);
    }

    OTP_LOGD("dual_flag:%d, dualcam_buffer:%p, dualcam_size:%d",
             convert_data->dual_otp.dual_flag,
             convert_data->dual_otp.data_3d.data_ptr,
             convert_data->dual_otp.data_3d.size);

    OTP_LOGD("sensor_id:%d, otp_version:%s, master(addr, size):total(%p, %d), "
             "module_info(%p, %d)",
             otp_cxt->sensor_id, otp_ver[otp_cxt->otp_module_info.otp_version],
             convert_data->total_otp.data_ptr, convert_data->total_otp.size,
             convert_data->dual_otp.master_module_info->rdm_info.data_addr,
             convert_data->dual_otp.master_module_info->rdm_info.data_size);
    OTP_LOGD(
        "master(addr, size):af(%p, %d), awb(%p, %d), oc(%p, %d), lsc(%p, %d), "
        "pdaf1(%p, %d), ae(%p, %d)",
        convert_data->dual_otp.master_af_info->rdm_info.data_addr,
        convert_data->dual_otp.master_af_info->rdm_info.data_size,
        convert_data->dual_otp.master_iso_awb_info->rdm_info.data_addr,
        convert_data->dual_otp.master_iso_awb_info->rdm_info.data_size,
        convert_data->dual_otp.master_optical_center_info->rdm_info.data_addr,
        convert_data->dual_otp.master_optical_center_info->rdm_info.data_size,
        convert_data->dual_otp.master_lsc_info->rdm_info.data_addr,
        convert_data->dual_otp.master_lsc_info->rdm_info.data_size,
        convert_data->dual_otp.master_pdaf_info->rdm_info.data_addr,
        convert_data->dual_otp.master_pdaf_info->rdm_info.data_size,
        convert_data->dual_otp.master_ae_info->rdm_info.data_addr,
        convert_data->dual_otp.master_ae_info->rdm_info.data_size);
    OTP_LOGD("master(addr, size):xtalk_4in1(%p, %d), dpc_4in1(%p, %d), "
             "superwide(%p, %d)",
             convert_data->dual_otp.master_xtalk_4in1_info->rdm_info.data_addr,
             convert_data->dual_otp.master_xtalk_4in1_info->rdm_info.data_size,
             convert_data->dual_otp.master_dpc_4in1_info->rdm_info.data_addr,
             convert_data->dual_otp.master_dpc_4in1_info->rdm_info.data_size,
             convert_data->dual_otp.master_spw_info->rdm_info.data_addr,
             convert_data->dual_otp.master_spw_info->rdm_info.data_size);

    p_val->pval = convert_data;
    p_val->type = convert_data->dual_otp.dual_flag;

    OTP_LOGV("X");
    return ret;
}

static cmr_int _general_otp_compatible_convert_slave(cmr_handle otp_drv_handle,
                                                     void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    struct sensor_otp_cust_info *convert_data = NULL;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    cmr_u8 *dual_flag_ptr = (cmr_u8 *)p_val->pval;
    char *otp_ver[] = {"0",   "0.1", "0.2", "0.3", "0.4", "0.5",
                       "0.6", "0.7", "0.8", "0.9", "1.0", "1.1"};

    if (otp_cxt->compat_convert_data) {
        convert_data = otp_cxt->compat_convert_data;
    } else {
        OTP_LOGE("otp convert data buffer is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    if (otp_cxt->otp_module_info.otp_version == VER_ERROR) {
        OTP_LOGE("otp version error! will not pass data to isp!");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;

    if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM) {
        convert_data->otp_vendor = OTP_VENDOR_SINGLE_CAM_DUAL;
    } else {
        convert_data->otp_vendor = OTP_VENDOR_DUAL_CAM_DUAL;
    }

    convert_data->dual_otp.slave_module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;
    convert_data->dual_otp.slave_af_info =
        (struct sensor_otp_section_info *)&format_data->af_cali_dat;
    convert_data->dual_otp.slave_iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
    convert_data->dual_otp.slave_optical_center_info =
        (struct sensor_otp_section_info *)&format_data->opt_center_dat;
    convert_data->dual_otp.slave_lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
    convert_data->dual_otp.slave_pdaf_info =
        (struct sensor_otp_section_info *)&format_data->pdaf_cali_dat;
    convert_data->dual_otp.slave_ae_info =
        (struct sensor_otp_section_info *)&format_data->ae_cali_dat;
    convert_data->dual_otp.slave_xtalk_4in1_info =
        (struct sensor_otp_section_info *)&format_data->xtalk_4in1_cali_dat;
    convert_data->dual_otp.slave_dpc_4in1_info =
        (struct sensor_otp_section_info *)&format_data->dpc_4in1_cali_dat;
    convert_data->dual_otp.slave_spw_info =
        (struct sensor_otp_section_info *)&format_data->spw_cali_dat;

    if (dual_flag_ptr)
        convert_data->dual_otp.dual_flag = *dual_flag_ptr;
    else
        convert_data->dual_otp.dual_flag = 0;

    if (convert_data->dual_otp.dual_flag == 1 &&
        format_data->dual_cam_cali_dat.rdm_info.size > 0) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->dual_cam_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->dual_cam_cali_dat.rdm_info.size;
    } else if (convert_data->dual_otp.dual_flag == 3) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->spw_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->spw_cali_dat.rdm_info.size;
    } else if ((convert_data->dual_otp.dual_flag == 5 ||
                convert_data->dual_otp.dual_flag == 6) &&
               format_data->wt_cali_dat.rdm_info.size > 0) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->wt_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->wt_cali_dat.rdm_info.size;
    }

    OTP_LOGD("dual_flag:%d, dualcam_buffer:%p, dualcam_size:%d",
             convert_data->dual_otp.dual_flag,
             convert_data->dual_otp.data_3d.data_ptr,
             convert_data->dual_otp.data_3d.size);

    OTP_LOGD("sensor_id:%d, otp_version:%s, slave(addr, size):total(%p, %d), "
             "module_info(%p, %d)",
             otp_cxt->sensor_id, otp_ver[otp_cxt->otp_module_info.otp_version],
             convert_data->total_otp.data_ptr, convert_data->total_otp.size,
             convert_data->dual_otp.slave_module_info->rdm_info.data_addr,
             convert_data->dual_otp.slave_module_info->rdm_info.data_size);
    OTP_LOGD(
        "slave(addr, size):af(%p, %d), awb(%p, %d), oc(%p, %d), lsc(%p, %d), "
        "pdaf1(%p, %d), ae(%p, %d)",
        convert_data->dual_otp.slave_af_info->rdm_info.data_addr,
        convert_data->dual_otp.slave_af_info->rdm_info.data_size,
        convert_data->dual_otp.slave_iso_awb_info->rdm_info.data_addr,
        convert_data->dual_otp.slave_iso_awb_info->rdm_info.data_size,
        convert_data->dual_otp.slave_optical_center_info->rdm_info.data_addr,
        convert_data->dual_otp.slave_optical_center_info->rdm_info.data_size,
        convert_data->dual_otp.slave_lsc_info->rdm_info.data_addr,
        convert_data->dual_otp.slave_lsc_info->rdm_info.data_size,
        convert_data->dual_otp.slave_pdaf_info->rdm_info.data_addr,
        convert_data->dual_otp.slave_pdaf_info->rdm_info.data_size,
        convert_data->dual_otp.slave_ae_info->rdm_info.data_addr,
        convert_data->dual_otp.slave_ae_info->rdm_info.data_size);
    OTP_LOGD("slave(addr, size):xtalk_4in1(%p, %d), dpc_4in1(%p, %d), "
             "superwide(%p, %d)",
             convert_data->dual_otp.slave_xtalk_4in1_info->rdm_info.data_addr,
             convert_data->dual_otp.slave_xtalk_4in1_info->rdm_info.data_size,
             convert_data->dual_otp.slave_dpc_4in1_info->rdm_info.data_addr,
             convert_data->dual_otp.slave_dpc_4in1_info->rdm_info.data_size,
             convert_data->dual_otp.slave_spw_info->rdm_info.data_addr,
             convert_data->dual_otp.slave_spw_info->rdm_info.data_size);

    p_val->pval = convert_data;
    p_val->type = convert_data->dual_otp.dual_flag;

    OTP_LOGV("X");
    return ret;
}

/*==================================================
*                External interface
====================================================*/

static cmr_int general_otp_drv_create(otp_drv_init_para_t *input_para,
                                      cmr_handle *otp_drv_handle) {
    return sensor_otp_drv_create(input_para, otp_drv_handle);
}

static cmr_int general_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

#ifdef CONFIG_CAMERA_SENSOR_OTP
#include "sensor_otp/hi846_gj_1_sensor_otp_drv.c"
#endif

/*malloc buffer and read otp raw data from eeprom or bin file*/
static cmr_int general_otp_drv_read(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    char otp_read_bin_path[255] = "otp_read_bin_path";
    char otp_dump_name[255] = "otp_dump_name";
    char value1[255];
    char value2[255];
    char value3[255];
    FILE *fp = NULL;
    cmr_u32 read_size = 0;
    cmr_u16 calib_version = 0;

    if (!otp_cxt->otp_raw_data.buffer) {
        otp_cxt->otp_raw_data.buffer =
            sensor_otp_get_raw_buffer(otp_cxt->eeprom_size, otp_cxt->sensor_id);
        if (!otp_cxt->otp_raw_data.buffer) {
            OTP_LOGE("malloc otp raw buffer failed");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        otp_cxt->otp_raw_data.num_bytes = otp_cxt->eeprom_size;

        if (!otp_cxt->otp_data) {
            otp_cxt->otp_data =
                (otp_format_data_t *)sensor_otp_get_formatted_buffer(
                    sizeof(otp_format_data_t), otp_cxt->sensor_id);

            if (!otp_cxt->otp_data) {
                OTP_LOGE("malloc otp section info buffer failed");
                ret = OTP_CAMERA_FAIL;
                goto exit;
            }
        }
    }

    property_get("debug.read.otp.open.camera", value1, "0");
    if (atoi(value1) == 0) {
        if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
            OTP_LOGD("otp raw data has been read before, is still in memory, "
                     "will not be read again");
            goto exit;
        }
    }

    property_get("persist.vendor.read.otp.from.bin", value2, "0");
    if (atoi(value2) == 1) {
        /* read otp from bin file */
        snprintf(otp_read_bin_path, sizeof(otp_read_bin_path), "%s%s%d_otp.bin",
                 "/data/vendor/cameraserver/", "id", otp_cxt->sensor_id);
        OTP_LOGD("otp_data_read_path:%s", otp_read_bin_path);
        if (-1 == access(otp_read_bin_path, 0)) {
            OTP_LOGE("otp bin file don't exist");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        fp = fopen(otp_read_bin_path, "rb");
        if (!fp) {
            OTP_LOGE("fp is null, open otp bin failed");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        read_size =
            fread(otp_cxt->otp_raw_data.buffer, 1, otp_cxt->eeprom_size, fp);
        if (read_size != otp_cxt->eeprom_size) {
            OTP_LOGE("read otp bin error, read_size is %d, otp size is %d",
                     read_size, otp_cxt->eeprom_size);
            ret = OTP_CAMERA_FAIL;
        } else {
            OTP_LOGD("read otp raw data from bin file successfully, size %d",
                     otp_cxt->eeprom_size);
        }
        fclose(fp);
        fp = NULL;

    } else {
        if ((otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM) &&
            (otp_cxt->sensor_id == 2 || otp_cxt->sensor_id == 3)) {
            /* slave copy otp raw data from master */
            otp_cxt->otp_raw_data.buffer = sensor_otp_copy_raw_buffer(
                otp_cxt->eeprom_size, otp_cxt->sensor_id - 2,
                otp_cxt->sensor_id);
            OTP_LOGD("copy otp raw data from master");
            sensor_otp_set_buffer_state(otp_cxt->sensor_id, 0);
        } else {
            /* read otp from eeprom */
            otp_cxt->otp_raw_data.buffer[0] = 0;
            otp_cxt->otp_raw_data.buffer[1] = 0;
            OTP_LOGD("i2c addr:0x%x", otp_cxt->eeprom_i2c_addr);

#ifdef CONFIG_CAMERA_SENSOR_OTP
            /* read otp from sensor */
            if (!strcmp(otp_cxt->dev_name, "hi846_gj_1_2lane")) {
                hi846_sensor_otp_read(otp_drv_handle);
                OTP_LOGD(
                    "read otp raw data from sensor %s successfully, size %d",
                    otp_cxt->dev_name, otp_cxt->eeprom_size);
            } else
#endif
            {
                ret = hw_sensor_read_i2c(
                    otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                    otp_cxt->otp_raw_data.buffer,
                    otp_cxt->eeprom_size << 16 | SENSOR_I2C_REG_16BIT);

                if (OTP_CAMERA_SUCCESS != ret) {
                    OTP_LOGE("kernel read i2c error, failed to read eeprom");
                    goto exit;
                } else {
                    OTP_LOGD(
                        "read otp raw data from eeprom successfully, size %d",
                        otp_cxt->eeprom_size);
                }
            }

            if (!strcmp(otp_cxt->dev_name, "ov8856_shine") &&
                otp_cxt->sensor_id == 2) {
                /* sharkl3 Android 10.0 new ov8856 module, DUAL_CAM_ONE_EEPROM,
                 * slave copy raw data from master */
                calib_version = (otp_cxt->otp_raw_data.buffer[4] << 8) |
                                otp_cxt->otp_raw_data.buffer[5];
                if (calib_version == 0xffff || calib_version == 0x0000) {
                    otp_cxt->otp_raw_data.buffer =
                        sensor_otp_copy_raw_buffer(otp_cxt->eeprom_size, 0, 2);
                    OTP_LOGD("ov8856_shine copy otp raw data from master");
                    sensor_otp_set_buffer_state(otp_cxt->sensor_id, 0);
                }
            }
        }
    }
exit:
    if (OTP_CAMERA_SUCCESS == ret) {
        property_get("debug.camera.save.otp.raw.data", value3, "0");
        if (atoi(value3) == 1) {
            /* dump otp to bin file */
            snprintf(otp_dump_name, sizeof(otp_dump_name), "%s_%s%d",
                     otp_cxt->dev_name, "id", otp_cxt->sensor_id);
            if (sensor_otp_dump_raw_data(otp_cxt->otp_raw_data.buffer,
                                         otp_cxt->eeprom_size, otp_dump_name))
                OTP_LOGE("dump otp bin file failed");
        }
    }
    OTP_LOGV("X");
    return ret;
}

static cmr_int general_otp_drv_write(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(param);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("X");
    return ret;
}

/*chesksum in parse operation*/
static cmr_int general_otp_drv_parse(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    char value[255];

    _general_otp_parse_map_version(otp_drv_handle);
    if (module_info->otp_version == VER_ERROR) {
        OTP_LOGE("otp version error! will not start parse!");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    property_get("debug.parse.otp.open.camera", value, "0");
    if (atoi(value) == 0) {
        if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
            OTP_LOGD("otp data has been parsed before, is still in memory, "
                     "will not be parsed again");
            return ret;
        }
    }

    if (module_info->otp_version == OTP_1_1) {
        _general_otp_parse_module_data_1v1(otp_drv_handle);

        if (otp_cxt->sensor_id < 6) {
            _general_otp_parse_af_1v1(otp_drv_handle);
            _general_otp_parse_awb_1v1(otp_drv_handle);
            _general_otp_parse_lsc_1v1(otp_drv_handle);
            _general_otp_parse_pdaf_1v1(otp_drv_handle);
            _general_otp_parse_xtalk_4in1_1v1(otp_drv_handle);
            _general_otp_parse_dpc_4in1_1v1(otp_drv_handle);
            _general_otp_parse_spw_1v1(otp_drv_handle);
            _general_otp_parse_bokeh_1v1(otp_drv_handle);
            _general_otp_parse_wt_1v1(otp_drv_handle);
        } else {
            OTP_LOGE("illegal sensor id");
        }
    } else if (module_info->otp_version == OTP_1_0) {
        _general_otp_parse_module_data_1v0(otp_drv_handle);

#ifdef SENSOR_OV8856_TELE
        if (otp_cxt->sensor_id == 0)
#else
        if (otp_cxt->sensor_id == 0 || otp_cxt->sensor_id == 1)
#endif
        {
            _general_otp_parse_master_af_1v0(otp_drv_handle);
            _general_otp_parse_master_awb_1v0(otp_drv_handle);
            _general_otp_parse_master_lsc_1v0(otp_drv_handle);
            _general_otp_parse_master_pdaf(otp_drv_handle);
            _general_otp_parse_master_ae_1v0(otp_drv_handle);
            _general_otp_parse_master_dualcam_1v0(otp_drv_handle);
        }
#ifdef SENSOR_OV8856_TELE
        else if (otp_cxt->sensor_id == 1 || otp_cxt->sensor_id == 2)
#else
        else if (otp_cxt->sensor_id == 2 || otp_cxt->sensor_id == 3 ||
                 otp_cxt->sensor_id == 4 || otp_cxt->sensor_id == 5)
#endif
        {
            _general_otp_parse_slave_af_1v0(otp_drv_handle);
            _general_otp_parse_slave_awb_1v0(otp_drv_handle);
            _general_otp_parse_slave_lsc_1v0(otp_drv_handle);
            _general_otp_parse_slave_ae_1v0(otp_drv_handle);
        } else {
            OTP_LOGE("illegal sensor id");
        }
    } else {
        if (otp_cxt->sensor_id == 0 || otp_cxt->sensor_id == 1) {
            _general_otp_parse_module_data_0v4(otp_drv_handle, 0x0000);
            if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM ||
                otp_cxt->eeprom_num == DUAL_CAM_TWO_EEPROM) {
                _general_otp_get_master_addr_0v4(otp_drv_handle);
                _general_otp_parse_master_af_0v4(otp_drv_handle);
                _general_otp_parse_master_awb_0v4(otp_drv_handle);
                _general_otp_parse_master_lsc_0v4(otp_drv_handle);
                _general_otp_parse_master_pdaf(otp_drv_handle);
                _general_otp_parse_master_ae_0v4(otp_drv_handle);
                _general_otp_parse_master_dualcam_0v4(otp_drv_handle);
            } else { // eeprom_num == SINGLE_CAM_ONE_EEPROM
                _general_otp_get_single_addr_0v4(otp_drv_handle);
                _general_otp_parse_master_af_0v4(otp_drv_handle);
                _general_otp_parse_master_awb_0v4(otp_drv_handle);
                _general_otp_parse_master_lsc_0v4(otp_drv_handle);
                _general_otp_parse_master_pdaf(otp_drv_handle);
            }
        } else if (otp_cxt->sensor_id == 2 || otp_cxt->sensor_id == 3) {
            if (otp_cxt->eeprom_num == DUAL_CAM_TWO_EEPROM) {
                _general_otp_parse_module_data_0v4(otp_drv_handle, 0x0000);
            } else if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM) {
                /* 0x1a00: dual_slave otp start address, mostly for
                 * ov8858+ov2680 sbs module */
                _general_otp_parse_module_data_0v4(otp_drv_handle, 0x1a00);
            } else {
                OTP_LOGE("slave sensor_id must be dualcam!");
            }
            _general_otp_get_slave_addr_0v4(otp_drv_handle);
            _general_otp_parse_slave_af_0v4(otp_drv_handle);
            _general_otp_parse_slave_awb_0v4(otp_drv_handle);
            _general_otp_parse_slave_lsc_0v4(otp_drv_handle);
            _general_otp_parse_slave_ae_0v4(otp_drv_handle);
        } else {
            OTP_LOGE("illegal sensor id");
        }
    }
    /*set buffer_state to 1, means otp data saved in memory*/
    sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1);

    OTP_LOGV("X");
    return ret;
}

static cmr_int general_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("X");
    return ret;
}

/*for expand*/
static cmr_int general_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                     void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:

#ifdef SENSOR_OV8856_TELE
        if (otp_cxt->sensor_id == 0)
#else
        if (otp_cxt->sensor_id == 0 || otp_cxt->sensor_id == 1)
#endif
        {
            if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM ||
                otp_cxt->eeprom_num == DUAL_CAM_TWO_EEPROM ||
                otp_cxt->eeprom_num == MULTICAM_INDEPENDENT_EEPROM) {
                _general_otp_compatible_convert_master(otp_drv_handle, param);
            } else {
                _general_otp_compatible_convert_single(otp_drv_handle, param);
            }
        }
#ifdef SENSOR_OV8856_TELE
        else if (otp_cxt->sensor_id == 1 || otp_cxt->sensor_id == 2)
#else
        else if (otp_cxt->sensor_id == 2 || otp_cxt->sensor_id == 3 ||
                 otp_cxt->sensor_id == 4 || otp_cxt->sensor_id == 5)
#endif
        {
            _general_otp_compatible_convert_slave(otp_drv_handle, param);
        } else {
            OTP_LOGE("illegal sensor id");
        }
        break;
    default:
        break;
    }
    OTP_LOGD("X");
    return ret;
}

void *otp_driver_open_lib(void) { return &general_otp_entry; }
