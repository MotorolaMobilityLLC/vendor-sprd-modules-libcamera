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

#include "s5k3l6_common_otp_drv.h"
#include "otp_parser.h"
static cmr_u8 af_buffer[10] ={0x01, 0x00, 0x00};

static cmr_u8 module_buffer[80] ={0x53, 0x50, 0x52, 0x44, 0x01, 0, 0x0d, 0, 0x01, 0, 0, 0, 0, 0, 0};
/*==================================================
*                Internal Functions
====================================================*/
static cmr_int _s5k3l6_otp_section_checksum(cmr_u8 *buffer, cmr_uint offset,
                                             cmr_uint size,
                                             cmr_uint checksum_offset) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 i = 0, sum = 0, checksum_cal = 0;

    OTP_LOGV("E");
    for (i = offset; i < offset + size; i++) {
        sum += buffer[i];
    }

    if (sum == 0) {
        ret = OTP_CAMERA_FAIL;
        OTP_LOGD("exception: all data is 0!");
    } else if (sum == 0xff * size) {
        ret = OTP_CAMERA_FAIL;
        OTP_LOGD("exception: all data is 0xff!");
    } else {
            checksum_cal = (sum % 256);
        if (checksum_cal == buffer[checksum_offset]) {
            ret = OTP_CAMERA_SUCCESS;
            OTP_LOGD("passed: checksum_addr = 0x%lx, "
                     "checksum_value = %d, sum = %d",checksum_offset,
                     buffer[checksum_offset], sum);
        } else {
            ret = CMR_CAMERA_FAIL;
            OTP_LOGD("failed: checksum_addr = 0x%lx, "
                     "checksum_value = %d, sum = %d, checksum_calulate = %d", checksum_offset,
                     buffer[checksum_offset], sum, checksum_cal);
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_get_lsc_channel_size(cmr_u16 width, cmr_u16 height,
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

static cmr_int _s5k3l6_otp_parse_module_data_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);

    ret = _s5k3l6_otp_section_checksum(buffer, 0x01, (0x07 - 0x01 + 1), 0x08);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("module info checksum error!");
        return ret;
    }
    OTP_LOGD("module info checksum passed");

    cmr_int module_info_valide = buffer[0];
    if(0x01 != module_info_valide){
        OTP_LOGE("module info invalide!!!");
        return OTP_CAMERA_FAIL;
    }
    module_dat->rdm_info.buffer = module_buffer;
    module_dat->rdm_info.size = 80;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    module_info->module_id_info.master_lens_id = buffer[5];
    module_info->module_id_info.master_vcm_id = buffer[6];
    module_info->module_id_info.year = buffer[2];
    module_info->module_id_info.month = buffer[3];
    module_info->module_id_info.day = buffer[4];
 

    OTP_LOGI("master_lens_id = 0x%x, master_vcm_id = 0x%x",
             module_info->module_id_info.master_lens_id,
             module_info->module_id_info.master_vcm_id);
    OTP_LOGI("slave_lens_id = 0x%x, slave_vcm_id = 0x%x",
             module_info->module_id_info.slave_lens_id,
             module_info->module_id_info.slave_vcm_id);
    OTP_LOGI(
        "module calibration date = %d-%d-%d", module_info->module_id_info.year,
        module_info->module_id_info.month, module_info->module_id_info.day);


    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_parse_master_af_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);

    ret = _s5k3l6_otp_section_checksum(buffer, 0x37, (0x3e - 0x37 + 1), 0x3f);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("af OTP info checksum error!");
        return ret;
    }
    OTP_LOGD("af OTP info checksum passed");
    cmr_int af_info_valide = buffer[0x36];
    if(0x01 != af_info_valide){
        OTP_LOGE("af OTP info invalide!!!");
        return OTP_CAMERA_FAIL;
    }

    af_buffer[1] = buffer[0x3c];
    af_buffer[2] = buffer[0x3b];
    af_buffer[3] = buffer[0x38];
    af_buffer[4] = buffer[0x37];
    af_cali_dat->rdm_info.buffer = af_buffer;
    af_cali_dat->rdm_info.size = 10;
    af_cali_dat->gld_info.buffer = NULL;
    af_cali_dat->gld_info.size = 0;

    //property_get("debug.camera.parse.otp.hal.log", value, "0");
    //if (atoi(value) == 1) {
        struct af_data_t af_data;

        af_data.af_infinity_position = (buffer[0x3b] << 8) | buffer[0x3c];
        af_data.af_macro_position = (buffer[0x37] << 8) | buffer[0x38];

        OTP_LOGI("af_infinity_position = %d, af_macro_position = %d",
                 af_data.af_infinity_position, af_data.af_macro_position);
    //}

    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_parse_master_awb_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;

    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    char value[255];



    ret = _s5k3l6_otp_section_checksum(buffer, 0xd10, (0xd1c-0xd10 + 1), 0xd1d);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = &(buffer[0xd10]);
    awb_cali_dat->rdm_info.size = 13;
    awb_cali_dat->gld_info.buffer = NULL;
    awb_cali_dat->gld_info.size = 0;

    //property_get("debug.camera.parse.otp.hal.log", value, "0");
    //if (atoi(value) == 1) {
        struct awb_data_t awb_data;

        awb_data.awb_version = buffer[0xd10];
        awb_data.awb_random_r = (buffer[0xd12] << 8) | buffer[0xd11];
        awb_data.awb_random_g = (buffer[0xd14] << 8) | buffer[0xd13];
        awb_data.awb_random_b = (buffer[0xd16] << 8) | buffer[0xd15];
        awb_data.awb_golden_r = (buffer[0xd18] << 8) | buffer[0xd17];
        awb_data.awb_golden_g = (buffer[0xd1a] << 8) | buffer[0xd19];
        awb_data.awb_golden_b = (buffer[0xd1c] << 8) | buffer[0xd1b];

        OTP_LOGI("awb_version = %d", awb_data.awb_version);
        OTP_LOGI("awb_random_r = %d, awb_random_g = %d, awb_random_b = %d",
                 awb_data.awb_random_r, awb_data.awb_random_g,
                 awb_data.awb_random_b);
        OTP_LOGI("awb_golden_r = %d, awb_golden_g = %d, awb_golden_b = %d",
                 awb_data.awb_golden_r, awb_data.awb_golden_g,
                 awb_data.awb_golden_b);
    //}

    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_parse_master_lsc_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    char value[255];


    ret = _s5k3l6_otp_section_checksum(buffer, 0xd20, (0x188d - 0xd20 + 1), 0x188e);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    lsc_cali_dat->rdm_info.buffer = &(buffer[0xd20]);
    lsc_cali_dat->rdm_info.size = 0x188d - 0xd20 + 1;
    lsc_cali_dat->gld_info.buffer = NULL;
    lsc_cali_dat->gld_info.size = 0;

    //property_get("debug.camera.parse.otp.hal.log", value, "0");
    //if (atoi(value) == 1) {
        struct lsc_data_t lsc_data;

        lsc_data.lsc_version = buffer[0xd20];
        lsc_data.lsc_oc_r_x = (buffer[0xd22] << 8) | buffer[0xd21];
        lsc_data.lsc_oc_r_y = (buffer[0xd24] << 8) | buffer[0xd23];
        lsc_data.lsc_oc_gr_x = (buffer[0xd26] << 8) | buffer[0xd25];
        lsc_data.lsc_oc_gr_y = (buffer[0xd28] << 8) | buffer[0xd27];
        lsc_data.lsc_oc_gb_x = (buffer[0xd2a] << 8) | buffer[0xd29];
        lsc_data.lsc_oc_gb_y = (buffer[0xd2c] << 8) | buffer[0xd2b];
        lsc_data.lsc_oc_b_x = (buffer[0xd2e] << 8) | buffer[0xd2d];
        lsc_data.lsc_oc_b_y = (buffer[0xd30] << 8) | buffer[0xd2f];
        lsc_data.lsc_img_width = (buffer[0xd32] << 8) | buffer[0xd31];
        lsc_data.lsc_img_height = (buffer[0xd34] << 8) | buffer[0xd33];
        lsc_data.lsc_grid = buffer[0xd35];

        OTP_LOGI("lsc_version = %d", lsc_data.lsc_version);
        OTP_LOGI("optical_center: R=(%d,%d), Gr=(%d,%d), Gb=(%d,%d), B=(%d,%d)",
                 lsc_data.lsc_oc_r_x, lsc_data.lsc_oc_r_y, lsc_data.lsc_oc_gr_x,
                 lsc_data.lsc_oc_gr_y, lsc_data.lsc_oc_gb_x,
                 lsc_data.lsc_oc_gb_y, lsc_data.lsc_oc_b_x,
                 lsc_data.lsc_oc_b_y);
        OTP_LOGI("lsc_img_width = %d, lsc_img_height = %d, lsc_grid = %d",
                 lsc_data.lsc_img_width, lsc_data.lsc_img_height,
                 lsc_data.lsc_grid);

        lsc_data.lsc_channel_size = _s5k3l6_otp_get_lsc_channel_size(
            lsc_data.lsc_img_width, lsc_data.lsc_img_height, lsc_data.lsc_grid);

    //}

    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_parse_master_pdaf(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    /* including single and dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;

    ret = _s5k3l6_otp_section_checksum(buffer, 0x1890, (0x1a21 - 0x1890 + 1), 0x1a22);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("pdaf checksum error");
        return ret;
    }
    OTP_LOGD("pdaf checksum passed");

    pdaf_cali_dat->rdm_info.buffer = &(buffer[0x1890]);
    pdaf_cali_dat->rdm_info.size = 0x1a21 - 0x1890 + 1;
    pdaf_cali_dat->gld_info.buffer = NULL;
    pdaf_cali_dat->gld_info.size = 0;

    cmr_u8 pdaf_version = buffer[0x1890];
    OTP_LOGI("pdaf_version = %d", pdaf_version);

    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_parse_master_ae_1v0(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");
#if 0
    /* including dual_master */
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat = otp_cxt->otp_raw_data.buffer +
                         module_info->master_start_addr.master_ae_addr;
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
#endif

    OTP_LOGV("X");
    return ret;
}

/*pass data to ispalg or depth*/
static cmr_int _s5k3l6_otp_compatible_convert_single(cmr_handle otp_drv_handle,
                                                      void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    struct sensor_otp_cust_info *convert_data = NULL;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;

    if (otp_cxt->compat_convert_data) {
        convert_data = otp_cxt->compat_convert_data;
    } else {
        OTP_LOGE("otp convert data buffer is null");
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

    OTP_LOGD(" otp_section_buffer: "
             "module_info:%p,af:%p,awb:%p,oc:%p,lsc:%p,pdaf:%p",
             convert_data->single_otp.module_info->rdm_info.data_addr,
             convert_data->single_otp.af_info->rdm_info.data_addr,
             convert_data->single_otp.iso_awb_info->rdm_info.data_addr,
             convert_data->single_otp.optical_center_info->rdm_info.data_addr,
             convert_data->single_otp.lsc_info->rdm_info.data_addr,
             convert_data->single_otp.pdaf_info->rdm_info.data_addr);

    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;

    OTP_LOGV("X");
    return ret;
}

static cmr_int _s5k3l6_otp_compatible_convert_master(cmr_handle otp_drv_handle,
                                                      void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    struct sensor_otp_cust_info *convert_data = NULL;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;


    if (otp_cxt->compat_convert_data) {
        convert_data = otp_cxt->compat_convert_data;
    } else {
        OTP_LOGE("otp convert data buffer is null");

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

        convert_data->dual_otp.dual_flag = 1;

        convert_data->dual_otp.data_3d.data_ptr =
            format_data->dual_cam_cali_dat.rdm_info.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->dual_cam_cali_dat.rdm_info.size;

    OTP_LOGD("dual_flag:%d, dualcam_buffer:%p, dualcam_size:%d",
             convert_data->dual_otp.dual_flag,
             convert_data->dual_otp.data_3d.data_ptr,
             convert_data->dual_otp.data_3d.size);
    OTP_LOGD(
        "otp_section_buffer: "
        "module_info:%p,af:%p,awb:%p,oc:%p,lsc:%p,pdaf:%p,ae:%p",
        convert_data->dual_otp.master_module_info->rdm_info.data_addr,
        convert_data->dual_otp.master_af_info->rdm_info.data_addr,
        convert_data->dual_otp.master_iso_awb_info->rdm_info.data_addr,
        convert_data->dual_otp.master_optical_center_info->rdm_info.data_addr,
        convert_data->dual_otp.master_lsc_info->rdm_info.data_addr,
        convert_data->dual_otp.master_pdaf_info->rdm_info.data_addr,
        convert_data->dual_otp.master_ae_info->rdm_info.data_addr);


    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;

    OTP_LOGV("X");
    return ret;
}

/*==================================================
*                External interface
====================================================*/

static cmr_int s5k3l6_otp_drv_create(otp_drv_init_para_t *input_para,
                                      cmr_handle *otp_drv_handle) {
    return sensor_otp_drv_create(input_para, otp_drv_handle);
}

static cmr_int s5k3l6_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

/*malloc buffer and read otp raw data from eeprom or bin file*/
static cmr_int s5k3l6_otp_drv_read(cmr_handle otp_drv_handle, void *param) {
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
        if (otp_cxt->sensor_id == 0) {
            /* including rear dual_master and single */
            snprintf(otp_read_bin_path, sizeof(otp_read_bin_path),
                     "%s%s_otp.bin", "/data/vendor/cameraserver/",
                     "rear_master");
        } else if (otp_cxt->sensor_id == 2) {
            /* including rear dual_slave */
            snprintf(otp_read_bin_path, sizeof(otp_read_bin_path),
                     "%s%s_otp.bin", "/data/vendor/cameraserver/",
                     "rear_slave");
        } else if (otp_cxt->sensor_id == 1) {
            /* including front dual_master and single */
            snprintf(otp_read_bin_path, sizeof(otp_read_bin_path),
                     "%s%s_otp.bin", "/data/vendor/cameraserver/",
                     "front_master");
        } else if (otp_cxt->sensor_id == 3) {
            /* including front dual_slave */
            snprintf(otp_read_bin_path, sizeof(otp_read_bin_path),
                     "%s%s_otp.bin", "/data/vendor/cameraserver/",
                     "front_slave");
        }

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
            ret = hw_sensor_read_i2c(
                otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                otp_cxt->otp_raw_data.buffer,
                otp_cxt->eeprom_size << 16 | SENSOR_I2C_REG_16BIT);
            if (OTP_CAMERA_SUCCESS != ret) {
                OTP_LOGE("kernel read i2c error, failed to read eeprom");
                goto exit;
            } else {
                OTP_LOGD("read otp raw data from eeprom successfully, size %d",
                         otp_cxt->eeprom_size);
            }
        }
    }
exit:
    if (OTP_CAMERA_SUCCESS == ret) {
        property_get("debug.camera.save.otp.raw.data", value3, "0");
        if (atoi(value3) == 1) {
            /* dump otp to bin file */
            if (otp_cxt->sensor_id == 0) {
                /* including rear dual_master and single */
                snprintf(otp_dump_name, sizeof(otp_dump_name), "%s_%s",
                         otp_cxt->dev_name, "rear_master");
            } else if (otp_cxt->sensor_id == 2) {
                /* including rear dual_slave */
                snprintf(otp_dump_name, sizeof(otp_dump_name), "%s_%s",
                         otp_cxt->dev_name, "rear_slave");
            } else if (otp_cxt->sensor_id == 1) {
                /* including front dual_master and single */
                snprintf(otp_dump_name, sizeof(otp_dump_name), "%s_%s",
                         otp_cxt->dev_name, "front_master");
            } else if (otp_cxt->sensor_id == 3) {
                /* including front dual_slave */
                snprintf(otp_dump_name, sizeof(otp_dump_name), "%s_%s",
                         otp_cxt->dev_name, "front_slave");
            }
            if (sensor_otp_dump_raw_data(otp_cxt->otp_raw_data.buffer,
                                         otp_cxt->eeprom_size, otp_dump_name))
                OTP_LOGE("dump otp bin file failed");
        }
    }
    OTP_LOGV("X");
    return ret;
}

static cmr_int s5k3l6_otp_drv_write(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(param);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("X");
    return ret;
}

/*chesksum in parse operation*/
static cmr_int s5k3l6_otp_drv_parse(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &(otp_cxt->otp_module_info);
    char value[255];

    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    if (buffer[0] != 1) {
        OTP_LOGI("All otp data is invalid!");
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

    //if (module_info->otp_version == OTP_1_1) {
        _s5k3l6_otp_parse_module_data_1v0(otp_drv_handle);


        if (otp_cxt->sensor_id == 0 || otp_cxt->sensor_id == 1){
            _s5k3l6_otp_parse_master_af_1v0(otp_drv_handle);
            _s5k3l6_otp_parse_master_awb_1v0(otp_drv_handle);
            _s5k3l6_otp_parse_master_lsc_1v0(otp_drv_handle);
            _s5k3l6_otp_parse_master_pdaf(otp_drv_handle);
            _s5k3l6_otp_parse_master_ae_1v0(otp_drv_handle);
            //_s5k3l6_otp_parse_master_dualcam_1v0(otp_drv_handle);
        } else {
            OTP_LOGE("illegal sensor id");
        }
  
    sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1);

    OTP_LOGV("X");
    return ret;
}

static cmr_int s5k3l6_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("X");
    return ret;
}

/*for expand*/
static cmr_int s5k3l6_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                     void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:

        if (otp_cxt->sensor_id == 0 || otp_cxt->sensor_id == 1){
            if (otp_cxt->eeprom_num == DUAL_CAM_ONE_EEPROM ||
                otp_cxt->eeprom_num == DUAL_CAM_TWO_EEPROM ||
                otp_cxt->eeprom_num == MULTICAM_INDEPENDENT_EEPROM) {
                _s5k3l6_otp_compatible_convert_master(otp_drv_handle, param);
            } else {
                _s5k3l6_otp_compatible_convert_single(otp_drv_handle, param);
            }
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

void *otp_driver_open_lib(void) {
    OTP_LOGD("s5k3l6_otp_entry lib is open");
    return &s5k3l6_otp_entry;
}
