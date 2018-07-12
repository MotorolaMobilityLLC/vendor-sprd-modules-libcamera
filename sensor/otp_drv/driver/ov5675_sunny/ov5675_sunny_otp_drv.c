#include "ov5675_sunny_otp_drv.h"

/** ov5675_sunny: dual camera - slave camera otp
 * compat otp v0.4 and v1.0
 **/
static cmr_int _ov5675_sunny_section_checksum(cmr_u8 *buffer, cmr_uint offset,
                                              cmr_uint size,
                                              cmr_uint checksum_offset,
                                              enum otp_version_t otp_version) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 i = 0, sum = 0, checksum_cal = 0;
    char *otp_ver[] = {
        "0",   "0.1", "0.2", "0.3", "0.4", "0.5",
        "0.6", "0.7", "0.8", "0.9", "1.0",
    };

    OTP_LOGV("E");
    for (i = offset; i < offset + size; i++) {
        sum += buffer[i];
    }

    if (otp_version == OTP_0_1) {
        checksum_cal = (sum % 255 + 1);
    } else {
        checksum_cal = (sum % 256);
    }
    if (checksum_cal == buffer[checksum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
        OTP_LOGD("passed:otp_version = "
                 "%s,checksum_addr=0x%lx,checksum_value=%d,sum=%d,"
                 "checksum_calulate=%d",
                 otp_ver[otp_version], checksum_offset, buffer[checksum_offset],
                 sum, checksum_cal);
    } else {
        ret = CMR_CAMERA_FAIL;
        OTP_LOGD("failed:otp_version = "
                 "%s,checksum_addr=0x%lx,checksum_value=%d,sum=%d,"
                 "checksum_calulate=%d",
                 otp_ver[otp_version], checksum_offset, buffer[checksum_offset],
                 sum, checksum_cal);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _ov5675_sunny_buffer_init(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int otp_len;
    cmr_u8 *otp_data = NULL;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    /*include random and golden lsc otp data,add reserve*/
    otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER;
    otp_data = sensor_otp_get_formatted_buffer(otp_len, otp_cxt->sensor_id);
    if (NULL == otp_data) {
        OTP_LOGE("malloc otp data buffer failed.\n");
        ret = CMR_CAMERA_FAIL;
    }
    otp_cxt->otp_data = (otp_format_data_t *)otp_data;
    OTP_LOGV("out");
    return ret;
}

static module_info_t ov5675_sunny_module_info;

static cmr_int _ov5675_sunny_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    module_info_t *local_module_info = &ov5675_sunny_module_info;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);
    cmr_u8 *module_info = otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET;

    local_module_info->calib_version = (module_info[4] << 8) | module_info[5];
    if (module_info[0] == 0x53 && module_info[1] == 0x50 &&
        module_info[2] == 0x52 && module_info[3] == 0x44 &&
        local_module_info->calib_version == 0x0100) {
        local_module_info->otp_version = OTP_1_0;
        OTP_LOGI("otp version is 1.0");
    } else if ((local_module_info->calib_version == 0x0005) ||
               (local_module_info->calib_version == 0x0500)) {
        local_module_info->otp_version = OTP_0_5;
        OTP_LOGI("otp version is 0.5");
    } else if ((local_module_info->calib_version == 0x0004) ||
               (local_module_info->calib_version == 0x0400)) {
        local_module_info->otp_version = OTP_0_4;
        OTP_LOGI("otp version is 0.4");
    } else if ((local_module_info->calib_version == 0x0003) ||
               (local_module_info->calib_version == 0x0300)) {
        local_module_info->otp_version = OTP_0_3;
        OTP_LOGI("otp version is 0.3");
    } else if ((local_module_info->calib_version == 0x0002) ||
               (local_module_info->calib_version == 0x0200)) {
        local_module_info->otp_version = OTP_0_2;
        OTP_LOGI("otp version is 0.2");
    } else if ((local_module_info->calib_version == 0x0001) ||
               ((local_module_info->calib_version == 0x0100) &&
                (module_info[0] != 0x53 || module_info[1] != 0x50 ||
                 module_info[2] != 0x52 || module_info[3] != 0x44))) {
        local_module_info->otp_version = OTP_0_1;
        OTP_LOGI("otp version is 0.1");
    } else {
        local_module_info->otp_version = VER_ERROR;
        OTP_LOGE("otp version error! calib_version = 0x%04x",
                 local_module_info->calib_version);
    }
    if (local_module_info->otp_version == OTP_1_0) {
        module_dat->rdm_info.buffer = module_info;
        module_dat->rdm_info.size =
            MODULE_INFO_CHECKSUM_1V0 - MODULE_INFO_OFFSET;
        module_dat->gld_info.buffer = NULL;
        module_dat->gld_info.size = 0;

        local_module_info->year = module_info[16];
        local_module_info->month = module_info[17];
        local_module_info->day = module_info[18];
        OTP_LOGI("otp v1.0 head, module calibration date = %d-%d-%d",
                 local_module_info->year, local_module_info->month,
                 local_module_info->day);
    } else {
        module_dat->rdm_info.buffer = module_info;
        module_dat->rdm_info.size = MODULE_INFO_CHECKSUM - MODULE_INFO_OFFSET;
        module_dat->gld_info.buffer = NULL;
        module_dat->gld_info.size = 0;

        local_module_info->year = module_info[6];
        local_module_info->month = module_info[7];
        local_module_info->day = module_info[8];
        OTP_LOGI("otp v0.4 head, module calibration date = %d-%d-%d",
                 local_module_info->year, local_module_info->month,
                 local_module_info->day);
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _ov5675_sunny_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    module_info_t *local_module_info = &ov5675_sunny_module_info;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat = NULL;

    if (local_module_info->otp_version == OTP_1_0) {
        awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET_1V0;
        ret = _ov5675_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET_1V0,
            AWB_INFO_CHECKSUM_1V0 - AWB_INFO_OFFSET_1V0, AWB_INFO_CHECKSUM_1V0,
            local_module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("v1.0 awb otp data checksum error,parse failed");
            return ret;
        }
        awb_cali_dat->rdm_info.buffer = awb_src_dat;
        awb_cali_dat->rdm_info.size =
            AWB_INFO_CHECKSUM_1V0 - AWB_INFO_OFFSET_1V0;
        awb_cali_dat->gld_info.buffer = NULL;
        awb_cali_dat->gld_info.size = 0;
        OTP_LOGI("v1.0 AWB checksum passed");
    } else {
        awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;
        ret = _ov5675_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
            AWB_INFO_CHECKSUM - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM,
            local_module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("v0.4 awb otp data checksum error,parse failed");
            return ret;
        }
        awb_cali_dat->rdm_info.buffer = awb_src_dat;
        awb_cali_dat->rdm_info.size = 6;
        awb_cali_dat->gld_info.buffer = golden_awb;
        awb_cali_dat->gld_info.size = 6;
        OTP_LOGI("v0.4 AWB checksum passed");
    }

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov5675_sunny_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    module_info_t *local_module_info = &ov5675_sunny_module_info;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_section_info_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    otp_section_info_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
    cmr_u8 *rdm_dst = NULL;
    cmr_u8 *opt_src = NULL;

    if (local_module_info->otp_version == OTP_1_0) {
        ret = _ov5675_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, LSC_INFO_OFFSET_1V0,
            LSC_INFO_CHECKSUM_1V0 - LSC_INFO_OFFSET_1V0, LSC_INFO_CHECKSUM_1V0,
            local_module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("v1.0 lsc otp data checksum error,parse failed");
            return ret;
        }
        rdm_dst = otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET_1V0;
        lsc_dst->rdm_info.buffer = rdm_dst;
        lsc_dst->rdm_info.size = LSC_INFO_CHECKSUM_1V0 - LSC_INFO_OFFSET_1V0;
        lsc_dst->gld_info.buffer = NULL;
        lsc_dst->gld_info.size = 0;
        OTP_LOGI("v1.0 LSC checksum passed");
    } else {
        ret = _ov5675_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
            LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM,
            local_module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGI("v0.4 lsc otp data checksum error,parse failed");
            return ret;
        }
        /*optical center data*/
        opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
        opt_dst->rdm_info.buffer = opt_src;
        opt_dst->rdm_info.size = LSC_INFO_OFFSET - OPTICAL_INFO_OFFSET;
        opt_dst->gld_info.buffer = NULL;
        opt_dst->gld_info.size = 0;

        /*lsc data*/
        rdm_dst = otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET;
        lsc_dst->rdm_info.buffer = rdm_dst;
        lsc_dst->rdm_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        lsc_dst->gld_info.buffer = golden_lsc;
        lsc_dst->gld_info.size = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        OTP_LOGI("v0.4 LSC checksum passed");
    }

    OTP_LOGV("out");
    return ret;
}

static int _ov5675_sunny_parse_ae_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    module_info_t *local_module_info = &ov5675_sunny_module_info;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_section_info_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat = otp_cxt->otp_raw_data.buffer + AE_INFO_OFFSET;

    if (local_module_info->otp_version == OTP_1_0) {
        ae_src_dat = otp_cxt->otp_raw_data.buffer + AE_INFO_OFFSET_1V0;
        ret = _ov5675_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, AE_INFO_OFFSET_1V0,
            AE_INFO_CHECKSUM_1V0 - AE_INFO_OFFSET_1V0, AE_INFO_CHECKSUM_1V0,
            local_module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGI("v1.0 ae otp data checksum error,parse failed");
            return ret;
        }
        ae_cali_dat->rdm_info.buffer = ae_src_dat;
        ae_cali_dat->rdm_info.size = AE_INFO_CHECKSUM_1V0 - AE_INFO_OFFSET_1V0;
        ae_cali_dat->gld_info.buffer = NULL;
        ae_cali_dat->gld_info.size = 0;
        OTP_LOGI("v1.0 AE checksum passed");
    } else {
        ae_src_dat = otp_cxt->otp_raw_data.buffer + AE_INFO_OFFSET;
        ret = _ov5675_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, AE_INFO_OFFSET,
            AE_INFO_CHECKSUM - AE_INFO_OFFSET, AE_INFO_CHECKSUM,
            local_module_info->otp_version);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGI("v0.4 ae otp data checksum error,parse failed");
            return ret;
        }
        ae_cali_dat->rdm_info.buffer = ae_src_dat;
        ae_cali_dat->rdm_info.size = AE_INFO_CHECKSUM - AE_INFO_OFFSET;
        ae_cali_dat->gld_info.buffer = NULL;
        ae_cali_dat->gld_info.size = 0;
        OTP_LOGI("v0.4 AE checksum passed");
    }

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov5675_sunny_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov5675_sunny_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("out");
    return ret;
}

/*==================================================
*                External interface
====================================================*/

static cmr_int ov5675_sunny_otp_drv_create(otp_drv_init_para_t *input_para,
                                           cmr_handle *sns_af_drv_handle) {
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int ov5675_sunny_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int ov5675_sunny_otp_drv_read(cmr_handle otp_drv_handle,
                                         void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u16 calib_version = 0;
    char value[255];
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    otp_params_t *p_data = (otp_params_t *)param;

    if (!otp_raw_data->buffer) {
        otp_raw_data->buffer =
            sensor_otp_get_raw_buffer(OTP_LEN, otp_cxt->sensor_id);
        if (NULL == otp_raw_data->buffer) {
            OTP_LOGE("malloc otp raw buffer failed\n");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        otp_raw_data->num_bytes = OTP_LEN;
        _ov5675_sunny_buffer_init(otp_drv_handle);
    }

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp raw data has read before,return directly");
        if (p_data) {
            p_data->buffer = otp_raw_data->buffer;
            p_data->num_bytes = otp_raw_data->num_bytes;
        }
        goto exit;
    }

    /*the otp start address is stored in first two bytes, must be set
     * correctly.*/
    otp_raw_data->buffer[0] = 0;
    otp_raw_data->buffer[1] = 0;
    ret = hw_sensor_read_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                             (cmr_u8 *)otp_raw_data->buffer,
                             SENSOR_I2C_REG_16BIT | OTP_LEN << 16);

    /* otp 1.0 one eeprom, slave copy raw data from master */
    calib_version = (otp_raw_data->buffer[4] << 8) | otp_raw_data->buffer[5];
    OTP_LOGD("calib_version = 0x%04x", calib_version);
    if (calib_version == 0xffff) {
        otp_raw_data->buffer = sensor_otp_copy_raw_buffer(OTP_LEN, 0, 2);
        OTP_LOGD("copy otp raw data from master");
        sensor_otp_set_buffer_state(otp_cxt->sensor_id, 0);
    }

exit:
    if (OTP_CAMERA_SUCCESS == ret) {
        property_get("debug.camera.save.otp.raw.data", value, "0");
        if (atoi(value) == 1) {
            if (sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_LEN,
                                         otp_cxt->dev_name))
                OTP_LOGE("dump failed");
        }
    }
    OTP_LOGV("X");
    return ret;
}

static cmr_int ov5675_sunny_otp_drv_write(cmr_handle otp_drv_handle,
                                          void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_data);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = p_data;

    if (NULL != otp_write_data->buffer) {
        cmr_u32 i;
        for (i = 0; i < otp_write_data->num_bytes; i++) {
            hw_sensor_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                                &otp_write_data->buffer[i], 2);
        }
        OTP_LOGI("write %s dev otp,buffer:0x%p,size:%d", otp_cxt->dev_name,
                 otp_write_data->buffer, otp_write_data->num_bytes);
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int ov5675_sunny_otp_drv_parse(cmr_handle otp_drv_handle,
                                          void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(ov5675_sunny_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    cmr_u16 calib_version = 0;

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    } else if (otp_raw_data->buffer) {
        calib_version =
            (otp_raw_data->buffer[4] << 8) | otp_raw_data->buffer[5];
        OTP_LOGD("calib_version = 0x%04x", calib_version);

        /*begain read raw data, save module info */
        OTP_LOGI("drver has read otp raw data,start parsed.");
        _ov5675_sunny_parse_module_data(otp_drv_handle);
        _ov5675_sunny_parse_awb_data(otp_drv_handle);
        _ov5675_sunny_parse_lsc_data(otp_drv_handle);
        _ov5675_sunny_parse_ae_data(otp_drv_handle);

        /*decompress lsc data if needed*/
        if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
            base_info->is_lsc_drv_decompression == TRUE) {
            ret = sensor_otp_lsc_decompress(
                &ov5675_sunny_drv_entry.otp_cfg.base_info_cfg,
                &otp_cxt->otp_data->lsc_cali_dat);
            if (ret != OTP_CAMERA_SUCCESS) {
                return OTP_CAMERA_FAIL;
            }
        }

        if (calib_version != 0xffff) {
            sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1);
        }
    } else {
        OTP_LOGE("should read otp before parse");
        return OTP_CAMERA_FAIL;
    }

    OTP_LOGV("out");
    return ret;
}

static cmr_int ov5675_sunny_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cali_items =
        &(ov5675_sunny_drv_entry.otp_cfg.cali_items);

    if (cali_items) {
        /*calibration at sensor or isp */
        if (cali_items->is_awbc && cali_items->is_awbc_self_cal)
            _ov5675_sunny_awb_calibration(otp_drv_handle);
        if (cali_items->is_lsc && cali_items->is_awbc_self_cal)
            _ov5675_sunny_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/
    OTP_LOGV("Exit");
    return ret;
}

static cmr_int ov5675_sunny_compatible_convert(cmr_handle otp_drv_handle,
                                               void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    struct sensor_single_otp_info *single_otp = NULL;
    struct sensor_otp_cust_info *convert_data = NULL;

    if (otp_cxt->otp_raw_data.buffer) {
        cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
        cmr_u16 calib_version = buffer[4] << 8 | buffer[5];
        OTP_LOGD("%02x %02x %02x %02x, calib_version = %04x", buffer[0],
                 buffer[1], buffer[2], buffer[3], calib_version);
    }

    if (otp_cxt->compat_convert_data) {
        convert_data = otp_cxt->compat_convert_data;
    } else {
        OTP_LOGE("otp convert data buffer is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    single_otp = &convert_data->single_otp;
    /*otp vendor tyep*/
    convert_data->otp_vendor = OTP_VENDOR_DUAL_CAM_DUAL;
    /*otp raw data*/
    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;
    /*module data*/
    convert_data->dual_otp.slave_module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;

    /*awb convert*/
    convert_data->dual_otp.slave_iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;

    /*lsc convert*/
    convert_data->dual_otp.slave_lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
    ;

    /*optical center*/
    convert_data->dual_otp.slave_optical_center_info =
        (struct sensor_otp_section_info *)&format_data->opt_center_dat;

    /*ae convert*/
    convert_data->dual_otp.slave_ae_info =
        (struct sensor_otp_section_info *)&format_data->ae_cali_dat;

    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGV("out");
    return 0;
}

/*just for expend*/
static cmr_int ov5675_sunny_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                          cmr_uint cmd, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        ov5675_sunny_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGV("out");
    return ret;
}
