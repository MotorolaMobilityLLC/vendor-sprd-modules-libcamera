
#include "ov13855_sunny_otp_drv.h"
#include <cutils/properties.h>

/** ov13855_sunny_section_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static cmr_int _ov13855_sunny_section_checksum(cmr_u8 *buf, cmr_uint offset,
                                               cmr_uint data_count,
                                               cmr_uint check_sum_offset,
                                               cmr_uint module_idx) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int i = 0, sum = 0;
    cmr_u32 check_sum = 0;

    OTP_LOGV("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    check_sum = (sum % 256);
    if (module_idx == OTP_TRULY)
        check_sum = (sum % 255 + 1);
    if (check_sum == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = CMR_CAMERA_FAIL;
    }
    OTP_LOGI("module_idx:%d,out: offset:%d, checksum:%d buf: %d", module_idx,
             check_sum_offset, sum, buf[check_sum_offset]);
    return ret;
}

static cmr_int _ov13855_sunny_buffer_init(cmr_handle otp_drv_handle) {
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
    } else {
        otp_cxt->otp_data_len = otp_len;
        lsccalib_data_t *lsc_data =
            &((otp_format_data_t *)otp_data)->lsc_cali_dat;
        lsc_data->lsc_calib_golden.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        lsc_data->lsc_calib_golden.offset = sizeof(lsccalib_data_t);

        lsc_data->lsc_calib_random.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        lsc_data->lsc_calib_random.offset =
            sizeof(lsccalib_data_t) + lsc_data->lsc_calib_random.length;
    }
    otp_cxt->otp_data = (otp_format_data_t *)otp_data;
    OTP_LOGV("out");
    return ret;
}
static cmr_int _ov13855_sunny_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);
    cmr_u8 *module_info = NULL;

    /*begain read raw data, save module info */
    module_info = (cmr_u8 *)(otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET);
    module_dat->vendor_id = module_info[0];
    module_dat->moule_id =
        (module_info[1] << 16) | (module_info[2] << 8) | module_info[3];
    module_dat->calib_version = (module_info[4] << 8) | module_info[5];
    module_dat->year = module_info[6];
    module_dat->month = module_info[7];
    module_dat->day = module_info[8];
    module_dat->work_stat_id = (module_info[9] << 8) | module_info[10];
    module_dat->env_record = (module_info[11] << 8) | module_info[12];

    if (module_dat->calib_version == 0x0400) {
        module_dat->calib_version = 0x0004;
    }
    if (module_dat->calib_version == 0x0001) {
        otp_cxt->otp_data_module_index = OTP_TRULY;
    }
    OTP_LOGI("moule_id:0x%x\n vendor_id:0x%x\n calib_version:%d\n "
             "work_stat_id:0x%x \n env_record :0x%x",
             module_dat->moule_id, module_dat->vendor_id,
             module_dat->calib_version, module_dat->work_stat_id,
             module_dat->env_record);

    return ret;
}
static cmr_int _ov13855_sunny_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    afcalib_data_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;

    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
        AF_INFO_CHECKSUM - AF_INFO_OFFSET, AF_INFO_CHECKSUM,
        otp_cxt->otp_data_module_index);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("auto focus checksum error,parse failed");
        return ret;
    } else {
        af_cali_dat->infinity_dac = (af_src_dat[1] << 8) | af_src_dat[0];
        af_cali_dat->macro_dac = (af_src_dat[3] << 8) | af_src_dat[2];
        af_cali_dat->afc_direction = af_src_dat[4];
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 section_num = 1;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;

    if (otp_cxt->otp_data_module_index == OTP_TRULY) {
        section_num = 2;
    }

    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
        AWB_INFO_CHECKSUM - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM,
        otp_cxt->otp_data_module_index);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb otp data checksum error,parse failed");
        return ret;
    } else {
        cmr_u32 i;
        /*random*/
        OTP_LOGI("awb section count:0x%x", section_num);
        for (i = 0; i < section_num; i++, awb_src_dat += AWB_INFO_SIZE) {
            awb_cali_dat->awb_rdm_info[i].R =
                (awb_src_dat[1] << 8) | awb_src_dat[0];
            awb_cali_dat->awb_rdm_info[i].G =
                (awb_src_dat[3] << 8) | awb_src_dat[2];
            awb_cali_dat->awb_rdm_info[i].B =
                (awb_src_dat[5] << 8) | awb_src_dat[4];
            /*golden awb data ,you should ensure awb group number*/
            if (otp_cxt->otp_data_module_index == OTP_TRULY) {
                awb_cali_dat->awb_gld_info[i].R = truly_awb[i].R;
                awb_cali_dat->awb_gld_info[i].G = truly_awb[i].G;
                awb_cali_dat->awb_gld_info[i].B = truly_awb[i].B;
            } else {
                awb_cali_dat->awb_gld_info[i].R = golden_awb[i].R;
                awb_cali_dat->awb_gld_info[i].G = golden_awb[i].G;
                awb_cali_dat->awb_gld_info[i].B = golden_awb[i].B;
            }
        }
        for (i = 0; i < AWB_MAX_LIGHT; i++)
            OTP_LOGV("rdm:R=0x%x,G=0x%x,B=0x%x.god:R=0x%x,G=0x%x,B=0x%x",
                     awb_cali_dat->awb_rdm_info[i].R,
                     awb_cali_dat->awb_rdm_info[i].G,
                     awb_cali_dat->awb_rdm_info[i].B,
                     awb_cali_dat->awb_gld_info[i].R,
                     awb_cali_dat->awb_gld_info[i].G,
                     awb_cali_dat->awb_gld_info[i].B);
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    cmr_u32 i = 0;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    lsccalib_data_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    optical_center_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
    cmr_u8 *rdm_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_random.offset;
    cmr_u8 *gld_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_golden.offset;

    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM,
        otp_cxt->otp_data_module_index);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("lsc otp data checksum error,parse failed.\n");
    } else {
        /*optical center data*/
        cmr_u8 *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
        opt_dst->R.x = (opt_src[1] << 8) | opt_src[0];
        opt_dst->R.y = (opt_src[3] << 8) | opt_src[2];
        opt_dst->GR.x = (opt_src[5] << 8) | opt_src[4];
        opt_dst->GR.y = (opt_src[7] << 8) | opt_src[6];
        opt_dst->GB.x = (opt_src[9] << 8) | opt_src[8];
        opt_dst->GB.y = (opt_src[11] << 8) | opt_src[10];
        opt_dst->B.x = (opt_src[13] << 8) | opt_src[12];
        opt_dst->B.y = (opt_src[15] << 8) | opt_src[14];

        /*R channel raw data*/
        memcpy(rdm_dst, otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET,
               LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
        lsc_dst->lsc_calib_random.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;

        /*gold data*/
        if (otp_cxt->otp_data_module_index == OTP_TRULY) {
            memcpy(gld_dst, truly_lsc, LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
            lsc_dst->lsc_calib_golden.length =
                LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        } else {
            memcpy(gld_dst, golden_lsc, LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
            lsc_dst->lsc_calib_golden.length =
                LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
        }
    }

    OTP_LOGI("optical_center:\nR=(0x%x,0x%x)\n GR=(0x%x,0x%x)\n "
             "GB=(0x%x,0x%x)\n B=(0x%x,0x%x)",
             opt_dst->R.x, opt_dst->R.y, opt_dst->GR.x, opt_dst->GR.y,
             opt_dst->GB.x, opt_dst->GB.y, opt_dst->B.x, opt_dst->B.y);
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
        PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET, PDAF_INFO_CHECKSUM,
        otp_cxt->otp_data_module_index);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
        return ret;
    } else {
        otp_cxt->otp_data->pdaf_cali_dat.buffer = pdaf_src_dat;
        otp_cxt->otp_data->pdaf_cali_dat.size =
            PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET;
    }
    OTP_LOGV("out");
    return ret;
}

static int _ov13855_sunny_parse_ae_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    aecalib_data_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    cmr_u8 *ae_src_dat = otp_cxt->otp_raw_data.buffer + AE_INFO_OFFSET;

    // for ae calibration
    ae_cali_dat->target_lum = (ae_src_dat[1] << 8) | ae_src_dat[0];
    ae_cali_dat->gain_1x_exp = (ae_src_dat[5] << 24) | (ae_src_dat[4] << 16) |
                               (ae_src_dat[3] << 8) | ae_src_dat[2];
    ae_cali_dat->gain_2x_exp = (ae_src_dat[9] << 24) | (ae_src_dat[8] << 16) |
                               (ae_src_dat[7] << 8) | ae_src_dat[6];
    ae_cali_dat->gain_4x_exp = (ae_src_dat[13] << 24) | (ae_src_dat[12] << 16) |
                               (ae_src_dat[11] << 8) | ae_src_dat[10];
    ae_cali_dat->gain_8x_exp = (ae_src_dat[17] << 24) | (ae_src_dat[16] << 16) |
                               (ae_src_dat[15] << 8) | ae_src_dat[14];
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_parse_dualcam_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_uint data_len = DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET;
    cmr_uint data_sum = DUAL_INFO_CHECKSUM;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    if (otp_cxt->otp_data_module_index == OTP_TRULY) {
        data_len -= VCM_DATA_SIZE;
        data_sum -= VCM_DATA_SIZE;
    }
    afcalib_data_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);

    /*dualcam data*/
    cmr_u8 *dualcam_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
    cmr_u8 *vcm_src_dat = otp_cxt->otp_raw_data.buffer + VCM_INFO_OFFSET;
    ret = _ov13855_sunny_section_checksum(otp_cxt->otp_raw_data.buffer,
                                          DUAL_INFO_OFFSET, data_len, data_sum,
                                          otp_cxt->otp_data_module_index);

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("dualcamera otp data checksum error,parse failed.\n");
        af_cali_dat->vcm_step = -1;
        otp_cxt->otp_data->dual_cam_cali_dat.buffer = NULL;
        otp_cxt->otp_data->dual_cam_cali_dat.size = 0;
        return ret;
    } else {
        if (otp_cxt->otp_data_module_index == OTP_SUNNY) {
            af_cali_dat->vcm_step = 0;
            af_cali_dat->vcm_step_min = vcm_src_dat[0];
            af_cali_dat->vcm_step_max = vcm_src_dat[1];
        } else {
            af_cali_dat->vcm_step = -1;
        }
        otp_cxt->otp_data->dual_cam_cali_dat.buffer = dualcam_src_dat;
        otp_cxt->otp_data->dual_cam_cali_dat.size = data_len;
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_sunny_parse_arcsoft_ip_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    if (otp_cxt->otp_data_module_index == OTP_SUNNY) {
        cmr_u8 *arcsoft_src_dat =
            otp_cxt->otp_raw_data.buffer + ARCSOFT_INFO_OFFSET;
        ret = _ov13855_sunny_section_checksum(
            otp_cxt->otp_raw_data.buffer, ARCSOFT_INFO_OFFSET,
            ARCSOFT_INFO_CHECKSUM - ARCSOFT_INFO_OFFSET, ARCSOFT_INFO_CHECKSUM,
            otp_cxt->otp_data_module_index);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGI("arcsoft otp data checksum error,parse failed.\n");
            otp_cxt->otp_data->third_cali_dat.buffer = NULL;
            otp_cxt->otp_data->third_cali_dat.size = 0;
            return ret;
        } else {
            otp_cxt->otp_data->third_cali_dat.buffer = arcsoft_src_dat;
            otp_cxt->otp_data->third_cali_dat.size =
                ARCSOFT_INFO_CHECKSUM - ARCSOFT_INFO_OFFSET;
        }
    } else {
        otp_cxt->otp_data->third_cali_dat.buffer = NULL;
        otp_cxt->otp_data->third_cali_dat.size = 0;
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cal_items =
        &(ov13855_sunny_drv_entry.otp_cfg.cali_items);
    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    int rg, bg, R_gain, G_gain, B_gain, Base_gain, temp, i;

    // calculate G gain

    R_gain = awb_cali_dat->awb_gld_info[0].rg_ratio * 1000 /
             awb_cali_dat->awb_rdm_info[0].rg_ratio;
    B_gain = awb_cali_dat->awb_gld_info[0].bg_ratio * 1000 /
             awb_cali_dat->awb_rdm_info[0].bg_ratio;
    G_gain = 1000;

    if (R_gain < 1000 || B_gain < 1000) {
        if (R_gain < B_gain)
            Base_gain = R_gain;
        else
            Base_gain = B_gain;
    } else {
        Base_gain = G_gain;
    }
    if (Base_gain != 0) {
        R_gain = 0x400 * R_gain / (Base_gain);
        B_gain = 0x400 * B_gain / (Base_gain);
        G_gain = 0x400 * G_gain / (Base_gain);
    } else {
        OTP_LOGE("awb parse problem!");
    }
    OTP_LOGI("r_Gain=0x%x,g_Gain=0x%x,b_Gain=0x%x\n", R_gain, G_gain, B_gain);

    if (cal_items->is_awbc_self_cal) {
        OTP_LOGD("Do wb calibration local");
    }
    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGV("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("out");
    return ret;
}

static cmr_int _ov13855_sunny_pdaf_calibration(cmr_handle otp_drv_handle) {
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

static cmr_int ov13855_sunny_otp_drv_create(otp_drv_init_para_t *input_para,
                                            cmr_handle *sns_af_drv_handle) {
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int ov13855_sunny_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int ov13855_sunny_otp_drv_read(cmr_handle otp_drv_handle,
                                          void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u8 cmd_val[3];
    cmr_uint i = 0;
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
        _ov13855_sunny_buffer_init(otp_drv_handle);
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

    /*
        for (i = 0; i < OTP_LEN; i++) {
            cmd_val[0] = ((OTP_START_ADDR + i) >> 8) & 0xff;
            cmd_val[1] = (OTP_START_ADDR + i) & 0xff;
            hw_sensor_read_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                               (cmr_u8 *)&cmd_val[0], 2);
            otp_raw_data->buffer[i] = cmd_val[0];
        }
    */
    if (OTP_CAMERA_SUCCESS == ret) {
        property_get("debug.camera.save.otp.raw.data", value, "0");
        if (atoi(value) == 1) {
            if (sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_LEN,
                                         otp_cxt->dev_name))
                OTP_LOGE("dump failed");
        }
    }

exit:
    OTP_LOGI("X");
    return ret;
}

static cmr_int ov13855_sunny_otp_drv_write(cmr_handle otp_drv_handle,
                                           void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_data);
    OTP_LOGI("dualotp write in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = (otp_params_t *)p_data;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dualcamera.write.otp", value, "false");

    if (!strcmp(value, "false")) {
        OTP_LOGI("do not set dual camera otp write property,directly return.");
        goto exit;
    }
    /*for before write*/
    cmr_int bSum = 0;
    /*for after  write*/
    cmr_int aSum = 0;
    /*open write permission*/
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_WR_ADDR, 0x0000,
                            0x00, BITS_ADDR16_REG8);
    sensor_otp_dump_raw_data(otp_write_data->buffer, DUAL_DATA_SIZE, "write");

    if (NULL != otp_write_data->buffer) {
        unsigned char *buffer = (unsigned char *)malloc(DUAL_DATA_SIZE);
        otp_write_data->reg_addr = DUAL_INFO_OFFSET;
        int i;

        for (i = 0; i < otp_write_data->num_bytes; i++) {
            bSum += otp_write_data->buffer[i];
            hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                                    otp_write_data->reg_addr + i,
                                    otp_write_data->buffer[i],
                                    BITS_ADDR16_REG8);
            buffer[i] = hw_sensor_grc_read_i2c(
                otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                otp_write_data->reg_addr + i, BITS_ADDR16_REG8);

            aSum += buffer[i];
        }

        /*for check reg write successful*/
        if (bSum == aSum) {
            hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                                    DUAL_INFO_CHECKSUM, (aSum % 255 + 1),
                                    BITS_ADDR16_REG8);

        } else {
            ret = OTP_CAMERA_FAIL;
            OTP_LOGI("crc failed bSum = 0x%x,aSum=0x%x", bSum, aSum);
        }
        OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d", otp_cxt->dev_name,
                 otp_write_data->buffer, otp_write_data->num_bytes);
        free(buffer);
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }

    /*close write permission*/
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_RD_ADDR, 0x0000,
                            0x00, BITS_ADDR16_REG8);

exit:
    OTP_LOGV("X");
    return ret;
}

static cmr_int ov13855_sunny_otp_drv_parse(cmr_handle otp_drv_handle,
                                           void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(ov13855_sunny_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);
    module_info_t *module_info = NULL;

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    } else if (otp_raw_data->buffer) {
        /*begain read raw data, save module info */
        OTP_LOGI("drver has read otp raw data,start parsed.");
        _ov13855_sunny_parse_module_data(otp_drv_handle);
        _ov13855_sunny_parse_af_data(otp_drv_handle);
        _ov13855_sunny_parse_awb_data(otp_drv_handle);
        _ov13855_sunny_parse_lsc_data(otp_drv_handle);
        _ov13855_sunny_parse_pdaf_data(otp_drv_handle);
        _ov13855_sunny_parse_ae_data(otp_drv_handle);
        _ov13855_sunny_parse_dualcam_data(otp_drv_handle);
        _ov13855_sunny_parse_arcsoft_ip_data(otp_drv_handle);
        /*decompress lsc data if needed*/
        if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
            base_info->is_lsc_drv_decompression == TRUE) {
            ret = sensor_otp_lsc_decompress(
                &ov13855_sunny_drv_entry.otp_cfg.base_info_cfg,
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

    OTP_LOGV("out");
    return ret;
}

static cmr_int ov13855_sunny_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cali_items =
        &(ov13855_sunny_drv_entry.otp_cfg.cali_items);

    if (cali_items) {
        if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal)
            _ov13855_sunny_pdaf_calibration(otp_drv_handle);
        /*calibration at sensor or isp */
        if (cali_items->is_awbc && cali_items->is_awbc_self_cal)
            _ov13855_sunny_awb_calibration(otp_drv_handle);
        if (cali_items->is_lsc && cali_items->is_awbc_self_cal)
            _ov13855_sunny_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/
    OTP_LOGV("Exit");
    return ret;
}

static cmr_int ov13855_sunny_compatible_convert(cmr_handle otp_drv_handle,
                                                void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    char value[255];
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    struct sensor_single_otp_info *single_otp = NULL;
    struct sensor_otp_cust_info *convert_data = NULL;

    convert_data = malloc(sizeof(struct sensor_otp_cust_info));
    if (NULL == convert_data) {
        OTP_LOGE("malloc otp convert_data failed.\n");
        return CMR_CAMERA_FAIL;
    }
    cmr_bzero(convert_data, sizeof(*convert_data));
    single_otp = &convert_data->single_otp;
    /*otp vendor type*/
    convert_data->otp_vendor = OTP_VENDOR_DUAL_CAM_DUAL;
    /*otp raw data*/
    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;
    /*module data*/
    convert_data->dual_otp.master_module_info.year =
        format_data->module_dat.year;
    convert_data->dual_otp.master_module_info.month =
        format_data->module_dat.month;
    convert_data->dual_otp.master_module_info.day = format_data->module_dat.day;
    convert_data->dual_otp.master_module_info.mid =
        format_data->module_dat.moule_id;
    convert_data->dual_otp.master_module_info.vcm_id =
        format_data->module_dat.vcm_id;
    convert_data->dual_otp.master_module_info.driver_ic_id =
        format_data->module_dat.drvier_ic_id;
    /*awb convert*/
    convert_data->dual_otp.master_iso_awb_info.iso = format_data->iso_dat;
    convert_data->dual_otp.master_iso_awb_info.gain_r =
        format_data->awb_cali_dat.awb_rdm_info[0].R;
    convert_data->dual_otp.master_iso_awb_info.gain_g =
        format_data->awb_cali_dat.awb_rdm_info[0].G;
    convert_data->dual_otp.master_iso_awb_info.gain_b =
        format_data->awb_cali_dat.awb_rdm_info[0].B;

    /*awb golden data*/
    convert_data->dual_otp.master_awb_golden_info.gain_r =
        format_data->awb_cali_dat.awb_gld_info[0].R;
    convert_data->dual_otp.master_awb_golden_info.gain_g =
        format_data->awb_cali_dat.awb_gld_info[0].G;
    convert_data->dual_otp.master_awb_golden_info.gain_b =
        format_data->awb_cali_dat.awb_gld_info[0].B;

    /*optical center*/
    memcpy((void *)&convert_data->dual_otp.master_optical_center_info,
           (void *)&format_data->opt_center_dat, sizeof(optical_center_t));

    /*lsc convert*/
    convert_data->dual_otp.master_lsc_info.lsc_data_addr =
        (cmr_u8 *)&format_data->lsc_cali_dat +
        format_data->lsc_cali_dat.lsc_calib_random.offset;
    convert_data->dual_otp.master_lsc_info.lsc_data_size =
        format_data->lsc_cali_dat.lsc_calib_random.length;
    convert_data->dual_otp.master_lsc_info.full_img_width =
        ov13855_sunny_drv_entry.otp_cfg.base_info_cfg.full_img_width;
    convert_data->dual_otp.master_lsc_info.full_img_height =
        ov13855_sunny_drv_entry.otp_cfg.base_info_cfg.full_img_height;
    convert_data->dual_otp.master_lsc_info.lsc_otp_grid =
        ov13855_sunny_drv_entry.otp_cfg.base_info_cfg.lsc_otp_grid;

    /*lsc golden data*/
    convert_data->dual_otp.master_lsc_golden_info.lsc_data_addr =
        (cmr_u8 *)&format_data->lsc_cali_dat +
        format_data->lsc_cali_dat.lsc_calib_golden.offset;
    convert_data->dual_otp.master_lsc_golden_info.lsc_data_size =
        format_data->lsc_cali_dat.lsc_calib_golden.length;

    /*ae convert*/
    convert_data->dual_otp.master_ae_info.ae_target_lum =
        format_data->ae_cali_dat.target_lum;
    convert_data->dual_otp.master_ae_info.gain_1x_exp =
        format_data->ae_cali_dat.gain_1x_exp;
    convert_data->dual_otp.master_ae_info.gain_2x_exp =
        format_data->ae_cali_dat.gain_2x_exp;
    convert_data->dual_otp.master_ae_info.gain_4x_exp =
        format_data->ae_cali_dat.gain_4x_exp;
    convert_data->dual_otp.master_ae_info.gain_8x_exp =
        format_data->ae_cali_dat.gain_8x_exp;

    /*af convert*/
    single_otp->af_info.infinite_cali = format_data->af_cali_dat.infinity_dac;
    single_otp->af_info.macro_cali = format_data->af_cali_dat.macro_dac;
    single_otp->af_info.vcm_step = format_data->af_cali_dat.vcm_step;
    single_otp->af_info.vcm_step_min = format_data->af_cali_dat.vcm_step_min;
    single_otp->af_info.vcm_step_max = format_data->af_cali_dat.vcm_step_max;

    /*pdaf convert*/
    single_otp->pdaf_info.pdaf_data_addr = format_data->pdaf_cali_dat.buffer;
    single_otp->pdaf_info.pdaf_data_size = format_data->pdaf_cali_dat.size;

    /*dual camera*/
    property_get("persist.sys.cam.api.version", value, "0");
    convert_data->dual_otp.dual_flag = 1;
    if (atoi(value) == 0) {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->dual_cam_cali_dat.buffer;
        convert_data->dual_otp.data_3d.size =
            format_data->dual_cam_cali_dat.size;
        convert_data->dual_otp.data_3d.dualcam_cali_lib_type = OTP_CALI_SPRD;
    } else {
        convert_data->dual_otp.data_3d.data_ptr =
            format_data->third_cali_dat.buffer;
        convert_data->dual_otp.data_3d.size = format_data->third_cali_dat.size;
        convert_data->dual_otp.data_3d.dualcam_cali_lib_type = OTP_CALI_ARCSOFT;
    }
    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGV("out");
    return 0;
}

/*just for expend*/
static cmr_int ov13855_sunny_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                           cmr_uint cmd, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        ov13855_sunny_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGV("out");
    return ret;
}
