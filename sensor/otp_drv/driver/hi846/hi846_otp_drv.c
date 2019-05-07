/*
 * Copyright (C) 2012 The Android Open Source Project
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
 * V1.0
 */
/*History
*Date                  Modification                                 Reason
*
*/

#include "hi846_otp_drv.h"

/*==============================================================================
 * Description:
 * read register value from sensor
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_i2c_read(void *otp_drv_handle, uint16_t slave_addr,
                              uint16_t reg_addr, uint8_t *reg_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint32_t cmd_len = 0;

    cmd_val[0] = reg_addr >> 8;
    cmd_val[1] = reg_addr & 0xff;
    cmd_len = 2;

    ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle, slave_addr,
                            (uint8_t *)&cmd_val[0], cmd_len);
    if (OTP_CAMERA_SUCCESS == ret) {
        *reg_data = cmd_val[0];
    }
    return ret;
}

/*==============================================================================
 * Description:
 * read register value from sensor
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_i2c_write(void *otp_drv_handle, uint16_t slave_addr,
                               uint16_t reg_addr, uint8_t reg_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint32_t cmd_len = 0;

    cmd_val[0] = reg_addr >> 8;
    cmd_val[1] = reg_addr & 0xff;
    cmd_val[2] = reg_data;
    cmd_len = 3;

    ret = hw_Sensor_WriteI2C(otp_cxt->hw_handle, slave_addr,
                             (uint8_t *)&cmd_val[0], cmd_len);

    return ret;
}

static cmr_u8 hi846_sensor_otp_read(cmr_handle otp_drv_handle,
                                    uint16_t otp_addr) {

    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u8 val = 0x00;
    CHECK_PTR(otp_drv_handle);

    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x070a,
                    (otp_addr >> 8) & 0xff); // start address
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x070b,
                    (otp_addr & 0xff)); // start address L
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0702, 0x01); // single read

    hi846_i2c_read(otp_drv_handle, OTP_I2C_ADDR, 0x0708, &val); // OTP data read

    return val;
}

/*==============================================================================
* Description:
* otp data checksum
*	-buff:address of otp buffer
*	-offset: the start address of the section
*	-data_count: data count of the section
*	-check_sum_offset: the section checksum offset
* please modify this function acording your spec
*============================================================================*/
static cmr_int hi846_section_checksum(unsigned char *buf, unsigned int offset,
                                      unsigned int data_count,
                                      unsigned int check_sum_offset) {
    uint32_t ret = OTP_CAMERA_SUCCESS;
    uint32_t i = 0, sum = 0;

    OTP_LOGI("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if ((sum % 255 + 1) == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("checksum: sum_val:0x%x, offset:0x%x, sum:0x%x, checksum:0x%x",
             ((sum % 255) + 1), check_sum_offset, sum, buf[check_sum_offset]);
    return ret;
}

/*==============================================================================
 * Description:
 * inital otp data buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_buffer_init(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*include random and golden lsc otp data,add reserve*/
    otp_format_data_t *otp_data =
        (otp_format_data_t *)sensor_otp_get_formatted_buffer(
            sizeof(otp_format_data_t), otp_cxt->sensor_id);

    if (NULL == otp_data) {
        OTP_LOGE("malloc otp data buffer failed.\n");
        ret = OTP_CAMERA_FAIL;
    }

    otp_cxt->otp_data = otp_data;
    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * parse otp data of module info
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int module_info_offset = 0x00;
    cmr_int module_checksum_offset = 0x00;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*module info data*/
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    cmr_u8 *module_dat_flag =
        otp_cxt->otp_raw_data.buffer + MODULE_INFO_GROUP_FLAG_OFFSET;
    cmr_u8 *module_src_dat = otp_cxt->otp_raw_data.buffer;
    cmr_u8 *module_info = otp_cxt->otp_raw_data.buffer + OTP_LEN;

    if (*module_dat_flag == 0x01) {
        module_info_offset = MODULE_INFO_GROUP1_REG - OTP_START_ADDR;
        module_checksum_offset =
            MODULE_INFO_CHECKSUM_GROUP1_REG - OTP_START_ADDR;
    } else if (*module_dat_flag == 0x13) {
        module_info_offset = MODULE_INFO_GROUP2_REG - OTP_START_ADDR;
        module_checksum_offset =
            MODULE_INFO_CHECKSUM_GROUP2_REG - OTP_START_ADDR;
    } else if (*module_dat_flag == 0x37) {
        module_info_offset = MODULE_INFO_GROUP3_REG - OTP_START_ADDR;
        module_checksum_offset =
            MODULE_INFO_CHECKSUM_GROUP3_REG - OTP_START_ADDR;
    } else {
        OTP_LOGE("get module group flag fail.\n");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    module_src_dat += module_info_offset;

    OTP_LOGI("module id = 0x%x", *module_src_dat);
    OTP_LOGI("focus mode flag = 0x%x", *(module_src_dat + 1));
    OTP_LOGI("date:%d-%d-%d", *(module_src_dat + 2), *(module_src_dat + 3),
             *(module_src_dat + 4));
    OTP_LOGI("sensor id = 0x%x", *(module_src_dat + 5));
    OTP_LOGI("lens id = 0x%x", *(module_src_dat + 6));
    OTP_LOGI("vcm id = 0x%x", *(module_src_dat + 7));
    OTP_LOGI("driver IC id = 0x%x", *(module_src_dat + 8));

    /*TODO*/
    ret =
        hi846_section_checksum(otp_cxt->otp_raw_data.buffer, module_info_offset,
                               MODULE_INFO_SIZE, module_checksum_offset);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("module data checksum error,parse failed");
        return ret;
    } else {
        OTP_LOGI("module data checksum success.");
        /* V0.4*/
        *(module_info + 0) = 0x00;
        *(module_info + 1) = 0x00;
        *(module_info + 2) = 0x00;
        *(module_info + 3) = 0x00;
        *(module_info + 4) = 0x00;
        *(module_info + 5) = 0x01;
        module_dat->rdm_info.buffer = module_info;
        module_dat->rdm_info.size = 6;
        module_dat->gld_info.buffer = NULL;
        module_dat->gld_info.size = 0;
    }
    /*END*/
    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * parse otp data of af
 * please modify this function acording your spec
 *============================================================================*/
#if AF_OTP_SUPPORT
static cmr_int hi846_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int af_info_offset = 0x00;
    cmr_int af_checksum_offset = 0x00;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*af data*/
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_dat_flag =
        otp_cxt->otp_raw_data.buffer + AF_INFO_GROUP_FLAG_OFFSET;
    cmr_u8 *af_src_dat = otp_raw_data.buffer;
    cmr_u8 *af_info = otp_cxt->otp_raw_data.buffer + OTP_LEN + 6;

    if (*af_dat_flag == 0x01) {
        af_info_offset = AF_INFO_GROUP1_REG - OTP_START_ADDR;
        af_checksum_offset = AF_INFO_CHECKSUM_GROUP1_REG - OTP_START_ADDR;
    } else if (*af_dat_flag == 0x13) {
        af_info_offset = AF_INFO_GROUP2_REG - OTP_START_ADDR;
        af_checksum_offset = AF_INFO_CHECKSUM_GROUP2_REG - OTP_START_ADDR;
    } else if (*af_dat_flag == 0x37) {
        af_info_offset = AF_INFO_GROUP3_REG - OTP_START_ADDR;
        af_checksum_offset = AF_INFO_CHECKSUM_GROUP3_REG - OTP_START_ADDR;
    } else {
        OTP_LOGE("get af group flag fail.\n");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    af_src_dat += af_info_offset;

    /*TODO*/
    ret = hi846_section_checksum(otp_cxt->otp_raw_data.buffer, af_info_offset,
                                 AF_INFO_SIZE, af_checksum_offset);
    /*END*/

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("auto focus checksum error,parse failed");
        return ret;
    } else {
        OTP_LOGI("auto focus checksum success.");
        *(af_info + 0) = *(af_src_dat + 0);
        *(af_info + 1) = *(af_src_dat + 1);
        *(af_info + 2) = *(af_src_dat + 2);
        *(af_info + 3) = *(af_src_dat + 3);
        af_cali_dat->rdm_info.buffer = af_info;
        af_cali_dat->rdm_info.size = 4;
        af_cali_dat->gld_info.buffer = NULL;
        af_cali_dat->gld_info.size = 0;
    }

    OTP_LOGI("out");
    return ret;
}
#endif

/*==============================================================================
 * Description:
 * parse otp data of awb
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int awb_ratio_offset = 0x00;
    cmr_int awb_checksum_offset = 0x00;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*awb data*/
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_dat_flag =
        otp_cxt->otp_raw_data.buffer + AWB_INFO_GROUP_FLAG_OFFSET;
    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer;

    if (*awb_dat_flag == 0x01) {
        awb_ratio_offset = AWB_RATIO_GROUP1_REG - OTP_START_ADDR;
        awb_checksum_offset = AWB_INFO_CHECKSUM_GROUP1_REG - OTP_START_ADDR;
    } else if (*awb_dat_flag == 0x13) {
        awb_ratio_offset = AWB_RATIO_GROUP2_REG - OTP_START_ADDR;
        awb_checksum_offset = AWB_INFO_CHECKSUM_GROUP2_REG - OTP_START_ADDR;
    } else if (*awb_dat_flag == 0x37) {
        awb_ratio_offset = AWB_RATIO_GROUP3_REG - OTP_START_ADDR;
        awb_checksum_offset = AWB_INFO_CHECKSUM_GROUP3_REG - OTP_START_ADDR;
    } else {
        OTP_LOGE("get awb group flag fail.\n");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    awb_src_dat += awb_ratio_offset;
    /*TODO*/
    ret = hi846_section_checksum(otp_cxt->otp_raw_data.buffer, awb_ratio_offset,
                                 AWB_RATIO_SIZE, awb_checksum_offset);
    /*END*/

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb otp data checksum error,parse failed");
        return ret;
    } else {
        OTP_LOGI("awb otp data checksum success.");
        awb_cali_dat->rdm_info.buffer = awb_src_dat;
        awb_cali_dat->rdm_info.size = AWB_RATIO_SIZE;
        awb_cali_dat->gld_info.buffer = hi846_awb;
        awb_cali_dat->gld_info.size = (sizeof(awb_target_packet_t));
    }

    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * parse otp data of lsc
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int lsc_info_offset = 0x00;
    cmr_int lsc_checksum_offset = 0x00;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*lsc and opt data*/
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *lsc_dat_flag =
        otp_cxt->otp_raw_data.buffer + LSC_INFO_GROUP_FLAG_OFFSET;
    cmr_u8 *lsc_src_dat = otp_cxt->otp_raw_data.buffer;

    if (*lsc_dat_flag == 0x01) {
        lsc_info_offset = LSC_INFO_GROUP1_REG - OTP_START_ADDR;
        lsc_checksum_offset = LSC_INFO_CHECKSUM_GROUP1_REG - OTP_START_ADDR;
    } else if (*lsc_dat_flag == 0x13) {
        lsc_info_offset = LSC_INFO_GROUP2_REG - OTP_START_ADDR;
        lsc_checksum_offset = LSC_INFO_CHECKSUM_GROUP2_REG - OTP_START_ADDR;
    } else if (*lsc_dat_flag == 0x37) {
        lsc_info_offset = LSC_INFO_GROUP3_REG - OTP_START_ADDR;
        lsc_checksum_offset = LSC_INFO_CHECKSUM_GROUP3_REG - OTP_START_ADDR;
    } else {
        OTP_LOGE("get lsc group flag fail.\n");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    lsc_src_dat += lsc_info_offset;
    /*TODO*/
    ret = hi846_section_checksum(otp_cxt->otp_raw_data.buffer, lsc_info_offset,
                                 LSC_INFO_SIZE, lsc_checksum_offset);
    /*END*/

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("lsc otp data checksum error,parse failed.\n");
    } else {
        OTP_LOGI("lsc otp data checksum success.");
        /*lsc data*/
        lsc_cali_dat->rdm_info.buffer = lsc_src_dat;
        lsc_cali_dat->rdm_info.size = LSC_INFO_SIZE;
        lsc_cali_dat->gld_info.buffer = hi846_lsc;
        lsc_cali_dat->gld_info.size = LSC_INFO_SIZE;
    }

    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * awb calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *buffer_current = awb_cali_dat->rdm_info.buffer;
    cmr_u16 R_gain, B_gain, G_gain;
    cmr_u16 wb_rg_golden, wb_bg_golden, wb_gg_golden;
    cmr_u16 rg_ratio_typical, bg_ratio_typical, grgb_ratio_typical,
        rg_ratio_val, bg_ratio_val;

    rg_ratio_typical = hi846_awb[0].rg_ratio;
    bg_ratio_typical = hi846_awb[0].bg_ratio;
    grgb_ratio_typical = hi846_awb[0].GrGb_ratio;

    rg_ratio_val = *buffer_current << 8 | *(buffer_current + 1);
    bg_ratio_val = *(buffer_current + 2) << 8 | *(buffer_current + 3);
    OTP_LOGI("get rg_ratio_val:0x%x, bg_ratio_val:0x%x\n", rg_ratio_val,
             bg_ratio_val);

    wb_rg_golden = *(buffer_current + 6) << 8 | *(buffer_current + 7);
    wb_bg_golden = *(buffer_current + 8) << 8 | *(buffer_current + 9);
    wb_gg_golden = *(buffer_current + 10) << 8 | *(buffer_current + 11);
    OTP_LOGI("get wb_rg_golden:0x%x, wb_bg_golden:0x%x, wb_gg_golden:0x%x\n",
             wb_rg_golden, wb_bg_golden, wb_gg_golden);

    if (wb_rg_golden && wb_bg_golden && wb_gg_golden) {
        rg_ratio_typical = wb_rg_golden;
        bg_ratio_typical = wb_bg_golden;
        grgb_ratio_typical = wb_gg_golden;
    }

    R_gain = rg_ratio_typical * 0x200 / rg_ratio_val;
    B_gain = bg_ratio_typical * 0x200 / bg_ratio_val;
    G_gain = 0x200;

    if (R_gain < B_gain) {
        if (R_gain < 0x0200) {
            B_gain = (0x0200 * B_gain) / R_gain;
            G_gain = (0x0200 * G_gain) / R_gain;
            R_gain = 0x0200;
        }
    } else {
        if (B_gain < 0x0200) {
            R_gain = (0x0200 * R_gain) / B_gain;
            G_gain = (0x0200 * G_gain) / B_gain;
            B_gain = 0x0200;
        }
    }
    /*TODO*/
    OTP_LOGI("before G_gain=0x%x,R_gain=0x%x,B_gain=0x%x\n", G_gain, R_gain,
             B_gain);

    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0078, G_gain >> 8);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0079, G_gain & 0xff);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x007a, G_gain >> 8);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x007b, G_gain & 0xff);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x007c, R_gain >> 8);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x007d, R_gain & 0xff);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x007e, B_gain >> 8);
    hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x007f, B_gain & 0xff);

    OTP_LOGI("after R_gain=0x%x,B_gain=0x%x,Gr_gain=0x%x,Gb_gain=0x%x",
             hi846_sensor_otp_read(otp_drv_handle, 0x007c),
             hi846_sensor_otp_read(otp_drv_handle, 0x007e),
             hi846_sensor_otp_read(otp_drv_handle, 0x0078),
             hi846_sensor_otp_read(otp_drv_handle, 0x007a));

    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * lsc calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);

    /*TODO*/

    /*END*/

    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * compatible convert, use for otp calibration through ISP
 * convert to format isp can recognize
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_compatible_convert(cmr_handle otp_drv_handle,
                                        void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    struct sensor_single_otp_info *single_otp = NULL;

    struct sensor_otp_cust_info *convert_data =
        (struct sensor_otp_cust_info *)malloc(
            sizeof(struct sensor_otp_cust_info));
    if (!convert_data) {
        OTP_LOGI("malloc convert data buffer failed");
        return OTP_CAMERA_FAIL;
    }

    cmr_bzero(convert_data, sizeof(*convert_data));
    single_otp = &convert_data->single_otp;

    /*otp vendor type*/
    convert_data->otp_vendor = OTP_VENDOR_SINGLE;

    /*otp raw data*/
    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;

    /*module data*/
    single_otp->module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;

    /*awb convert*/
    if (hi846_drv_entry.otp_cfg.cali_items.is_awbc) {
        if (hi846_drv_entry.otp_cfg.cali_items.is_awbc_self_cal) {
        } else {
            single_otp->iso_awb_info =
                (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
        }
    }

    /*lsc convert*/
    if (hi846_drv_entry.otp_cfg.cali_items.is_lsc) {
        if (hi846_drv_entry.otp_cfg.cali_items.is_lsc_self_cal) {

        } else {
            single_otp->lsc_info =
                (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
        }
    }

#if AF_OTP_SUPPORT
    /*af convert*/
    if (hi846_drv_entry.otp_cfg.cali_items.is_afc) {
        if (hi846_drv_entry.otp_cfg.cali_items.is_af_self_cal) {

        } else {
            single_otp->af_info =
                (struct sensor_otp_section_info *)&format_data->af_cali_dat;
        }
    }
#endif

    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;

    OTP_LOGI("out");
    return ret;
}

/*==================================================
*	External interface
====================================================*/

/*==============================================================================
 * Description:
 * create otp handle
 * please don't modify this function
 *============================================================================*/
static cmr_int hi846_otp_create(otp_drv_init_para_t *input_para,
                                cmr_handle *otp_drv_handle) {
    OTP_LOGI("in");
    return sensor_otp_drv_create(input_para, otp_drv_handle);
}

/*==============================================================================
 * Description:
 * create otp handle
 * please don't modify this function
 *============================================================================*/
static cmr_int hi846_otp_drv_delete(cmr_handle otp_drv_handle) {

    OTP_LOGI("in");
    return sensor_otp_drv_delete(otp_drv_handle);
}

/*==============================================================================
 * Description:
 * read otp data from sensor to buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_otp_drv_read(cmr_handle otp_drv_handle, void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int i = 0;
    uint8_t read_flag, status = 0x00;
    CHECK_PTR(otp_drv_handle);
    char value[255];
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    otp_params_t *p_data = (otp_params_t *)p_params;
    cmr_u8 *buff = NULL;

    if (!otp_raw_data->buffer) {
        /*when mobile power on , it will init*/
        otp_raw_data->buffer = sensor_otp_get_raw_buffer(
            OTP_LEN + OTP_UPLOAD_LEN, otp_cxt->sensor_id);
        if (!otp_raw_data->buffer) {
            OTP_LOGE("malloc otp raw buffer failed\n");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        otp_raw_data->num_bytes = OTP_LEN + OTP_UPLOAD_LEN;
        hi846_buffer_init(otp_drv_handle);
    }

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp raw data has read before,return directly");
        if (p_data) {
            p_data->buffer = buff;
            p_data->num_bytes = otp_raw_data->num_bytes;
        }
        goto exit;
    } else {
        OTP_LOGI("start init otp now!\n");
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0a02,
                        0x01); // fast sleep On
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0a00,
                        0x00); // stand by On
        usleep(10 * 1000);
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0f02,
                        0x00); // pll disable
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x071a,
                        0x01); // CP TRI_H;
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x071b,
                        0x09); // IPGM TRIM_H
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0d04,
                        0x01); // Fsync Output enable
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0d00,
                        0x07); // Fsync Output Drivability
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x003e, 0x10); // OTP R/W
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x070f,
                        0x05); // OTP data rewrite
        usleep(10 * 1000);
        hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0a00,
                        0x01); // stand by Off

        /*flag check*/
        read_flag = hi846_sensor_otp_read(otp_drv_handle, 0x0201);
        OTP_LOGI("hi846 otp group flag: 0x%x", read_flag);
        if (read_flag == 0x00) {
            status = hi846_sensor_otp_read(otp_drv_handle, 0x0a00);
            OTP_LOGI("otp read 0x0a00 = 0x%x\n", status);
            if (0x01 != status) {
                hi846_i2c_write(otp_drv_handle, OTP_I2C_ADDR, 0x0a00, 0x01);
                usleep(1000);
                status = hi846_sensor_otp_read(otp_drv_handle, 0x0a00);
                OTP_LOGI("otp read 0x0a00 = 0x%x\n", status);
            }
        }

        buff = otp_raw_data->buffer;
        /* start read otp data save for buffer */
        for (i = 0; i < OTP_LEN; i++) {
            *(buff + i) =
                hi846_sensor_otp_read(otp_drv_handle, OTP_START_ADDR + i);
            OTP_LOGV("data buffer[0x%x] = 0x%x", (OTP_START_ADDR + i),
                     *(buff + i));
        }
    }

exit:
    if (OTP_CAMERA_SUCCESS == ret) {
        property_get("debug.camera.save.otp.raw.data", value, "0");
        if (atoi(value) == 1) {
            if (sensor_otp_dump_raw_data(buff, OTP_LEN + OTP_UPLOAD_LEN,
                                         otp_cxt->dev_name))
                OTP_LOGE("dump failed");
        }
    }

    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * write otp data
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_otp_drv_write(cmr_handle otp_drv_handle, void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = p_params;

    if (NULL != otp_write_data->buffer) {
        OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d", otp_cxt->dev_name,
                 (unsigned int)otp_write_data->buffer,
                 otp_write_data->num_bytes);
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * read otp to buffer  and parse otp data in buffer
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_otp_drv_parse(cmr_handle otp_drv_handle, void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info = &(hi846_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    } else if (otp_raw_data->buffer) {
        /*begain read raw data, save module info */
        OTP_LOGI("drver has read otp raw data,start parsed.");
        hi846_parse_module_data(otp_drv_handle);
        hi846_parse_lsc_data(otp_drv_handle);
        hi846_parse_awb_data(otp_drv_handle);
#if AF_OTP_SUPPORT
        hi846_parse_af_data(otp_drv_handle);
#endif
        /*decompress lsc data if needed*/
        if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
            base_info->is_lsc_drv_decompression == TRUE) {
            ret = sensor_otp_lsc_decompress(
                &hi846_drv_entry.otp_cfg.base_info_cfg,
                &otp_cxt->otp_data->lsc_cali_dat);
            if (ret != OTP_CAMERA_SUCCESS) {
                return OTP_CAMERA_FAIL;
            }
        }
        sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1); /*read to memory*/
    } else {
        OTP_LOGE("should read otp before parse");
        return OTP_CAMERA_FAIL;
    }

    OTP_LOGI("out");
    return ret;
}

/*==============================================================================
 * Description:
 * otp drv calibration
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_calib_items_t *cali_items = &(hi846_drv_entry.otp_cfg.cali_items);

    if (cali_items) {
        /*calibration at sensor or isp */
        if (cali_items->is_awbc && cali_items->is_awbc_self_cal) {
            hi846_awb_calibration(otp_drv_handle);
        }
        if (cali_items->is_lsc && cali_items->is_awbc_self_cal) {
            hi846_lsc_calibration(otp_drv_handle);
        }
    }
    /*If there are other items that need calibration, please add to here*/

    OTP_LOGI("out");
    return ret;
}

/*just for expend*/
/*==============================================================================
 * Description:
 * reserve interface: just for expending
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int hi846_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                   void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        // hi846_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGI("out");
    return ret;
}
