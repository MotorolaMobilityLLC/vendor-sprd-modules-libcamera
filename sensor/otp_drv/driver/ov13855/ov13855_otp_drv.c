#include "ov13855_otp_drv.h"

/** ov13855_section_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static cmr_int _ov13855_section_checksum(cmr_u8 *buf, cmr_uint offset,
                                         cmr_uint data_count,
                                         cmr_uint check_sum_offset) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int i = 0, sum = 0;

    OTP_LOGI("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if ((sum % 255 + 1) == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = CMR_CAMERA_FAIL;
    }
    OTP_LOGI("out: offset:%d, checksum:%d buf: %d", check_sum_offset, sum,
             buf[check_sum_offset]);
    return ret;
}

static cmr_int _ov13855_buffer_init(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int otp_len;
    cmr_u8 *otp_data = NULL;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    /*include random and truly lsc otp data,add reserve*/
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
    OTP_LOGI("out");
    return ret;
}
static cmr_int _ov13855_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    afcalib_data_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
    ret =
        _ov13855_section_checksum(otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
                                  AF_INFO_SIZE - 1, AF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("auto focus checksum error,parse failed");
        return ret;
    } else {
        af_cali_dat->infinity_dac = (af_src_dat[1] << 8) | af_src_dat[0];
        af_cali_dat->macro_dac = (af_src_dat[3] << 8) | af_src_dat[2];
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;

    ret = _ov13855_section_checksum(
        otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
        AWB_INFO_CHECKSUM - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb otp data checksum error,parse failed");
        return ret;
    } else {
        cmr_u32 i;
        /*random*/
        OTP_LOGI("awb section count:0x%x", AWB_SECTION_NUM);
        for (i = 0; i < AWB_SECTION_NUM; i++, awb_src_dat += AWB_INFO_SIZE) {
            awb_cali_dat->awb_rdm_info[i].R =
                (awb_src_dat[1] << 8) | awb_src_dat[0];
            awb_cali_dat->awb_rdm_info[i].G =
                (awb_src_dat[3] << 8) | awb_src_dat[2];
            awb_cali_dat->awb_rdm_info[i].B =
                (awb_src_dat[5] << 8) | awb_src_dat[4];
            /*truly awb data ,you should ensure awb group number*/
            awb_cali_dat->awb_gld_info[i].R = truly_awb[i].R;
            awb_cali_dat->awb_gld_info[i].G = truly_awb[i].G;
            awb_cali_dat->awb_gld_info[i].B = truly_awb[i].B;
        }
        for (i = 0; i < AWB_MAX_LIGHT; i++)
            OTP_LOGD("rdm:R=0x%x,G=0x%x,B=0x%x.god:R=0x%x,G=0x%x,B=0x%x",
                     awb_cali_dat->awb_rdm_info[i].R,
                     awb_cali_dat->awb_rdm_info[i].G,
                     awb_cali_dat->awb_rdm_info[i].B,
                     awb_cali_dat->awb_gld_info[i].R,
                     awb_cali_dat->awb_gld_info[i].G,
                     awb_cali_dat->awb_gld_info[i].B);
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    lsccalib_data_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    optical_center_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
    cmr_u8 *rdm_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_random.offset;
    cmr_u8 *gld_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_golden.offset;

    ret = _ov13855_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
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
        memcpy(gld_dst, truly_lsc, LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
        lsc_dst->lsc_calib_golden.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;
    }

    OTP_LOGD("optical_center:\nR=(0x%x,0x%x)\n GR=(0x%x,0x%x)\n "
             "GB=(0x%x,0x%x)\n B=(0x%x,0x%x)",
             opt_dst->R.x, opt_dst->R.y, opt_dst->GR.x, opt_dst->GR.y,
             opt_dst->GB.x, opt_dst->GB.y, opt_dst->B.x, opt_dst->B.y);
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
    ret = _ov13855_section_checksum(
        otp_cxt->otp_raw_data.buffer, PDAF_INFO_OFFSET,
        PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET, PDAF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
        return ret;
    } else {
        otp_cxt->otp_data->pdaf_cali_dat.buffer = pdaf_src_dat;
        otp_cxt->otp_data->pdaf_cali_dat.size =
            PDAF_INFO_CHECKSUM - PDAF_INFO_OFFSET;
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cal_items = &(ov13855_drv_entry.otp_cfg.cali_items);
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
    OTP_LOGI("r_Gain=0x%x\n", R_gain);
    OTP_LOGI("g_Gain=0x%x\n", G_gain);
    OTP_LOGI("b_Gain=0x%x\n", B_gain);

    if (cal_items->is_awbc_self_cal) {
        OTP_LOGD("Do wb calibration local");
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGI("out");
    return ret;
}

static cmr_int _ov13855_pdaf_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGI("out");
    return ret;
}

/*==================================================
*                External interface
====================================================*/

static cmr_int ov13855_otp_drv_create(otp_drv_init_para_t *input_para,
                                      cmr_handle *sns_af_drv_handle) {
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int ov13855_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int ov13855_otp_drv_read(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u8 cmd_val[3];
    cmr_uint i = 0;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("E");

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
        _ov13855_buffer_init(otp_drv_handle);
    }

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp raw data has read before,return directly");
        if (p_data) {
            p_data->buffer = otp_raw_data->buffer;
            p_data->num_bytes = otp_raw_data->num_bytes;
        }
        goto exit;
    }

    for (i = 0; i < OTP_LEN; i++) {
        cmd_val[0] = ((OTP_START_ADDR + i) >> 8) & 0xff;
        cmd_val[1] = (OTP_START_ADDR + i) & 0xff;
        hw_sensor_read_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                           (cmr_u8 *)&cmd_val[0], 2);
        otp_raw_data->buffer[i] = cmd_val[0];
    }
    sensor_otp_dump_raw_data(otp_raw_data->buffer, OTP_LEN, otp_cxt->dev_name);

exit:
    OTP_LOGI("X");
    return ret;
}

static cmr_int ov13855_otp_drv_write(cmr_handle otp_drv_handle, void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_data);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = p_data;

    if (NULL != otp_write_data->buffer) {
        int i;
        for (i = 0; i < otp_write_data->num_bytes; i++) {
            hw_sensor_write_i2c(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                                &otp_write_data->buffer[i], 2);
        }
        OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d", otp_cxt->dev_name,
                 otp_write_data->buffer, otp_write_data->num_bytes);
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int ov13855_otp_drv_parse(cmr_handle otp_drv_handle, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info = &(ov13855_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);
    module_info_t *module_info = NULL;

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    } else if (otp_raw_data->buffer) {
        /*begain read raw data, save module info */
        module_info = (module_info_t *)(otp_cxt->otp_raw_data.buffer +
                                        MODULE_INFO_OFFSET);
        module_dat->moule_id = module_info->moule_id;
        module_dat->vcm_id = module_info->vcm_id;
        module_dat->ir_bg_id = module_info->ir_bg_id;
        module_dat->drvier_ic_id = module_info->drvier_ic_id;

        OTP_LOGI("moule_id:%d\n vcm_id:%d\n ir_bg_id:%d\n drvier_ic_id:%d",
                 module_info->moule_id, module_info->vcm_id,
                 module_info->ir_bg_id, module_info->drvier_ic_id);
        OTP_LOGI("drver has read otp raw data,start parsed.");
        _ov13855_parse_af_data(otp_drv_handle);
        _ov13855_parse_awb_data(otp_drv_handle);
        _ov13855_parse_lsc_data(otp_drv_handle);
        _ov13855_parse_pdaf_data(otp_drv_handle);

        /*decompress lsc data if needed*/
        if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
            base_info->is_lsc_drv_decompression == TRUE) {
            ret = sensor_otp_lsc_decompress(
                &ov13855_drv_entry.otp_cfg.base_info_cfg,
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

static cmr_int ov13855_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cali_items = &(ov13855_drv_entry.otp_cfg.cali_items);

    if (cali_items) {
        if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal)
            _ov13855_pdaf_calibration(otp_drv_handle);
        /*calibration at sensor or isp */
        if (cali_items->is_awbc && cali_items->is_awbc_self_cal)
            _ov13855_awb_calibration(otp_drv_handle);
        if (cali_items->is_lsc && cali_items->is_awbc_self_cal)
            _ov13855_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/
    OTP_LOGI("Exit");
    return ret;
}

static cmr_int ov13855_compatible_convert(cmr_handle otp_drv_handle,
                                          void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    struct sensor_single_otp_info *single_otp = NULL;
    struct sensor_otp_cust_info *convert_data = NULL;

    convert_data = malloc(sizeof(struct sensor_otp_cust_info));
    single_otp = &convert_data->single_otp;
    /*otp raw data*/
    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;
    /*module data*/
    single_otp->module_info.vcm_id = format_data->module_dat.vcm_id;
    single_otp->module_info.driver_ic_id = format_data->module_dat.drvier_ic_id;
    /*awb convert*/
    single_otp->iso_awb_info.gain_r =
        format_data->awb_cali_dat.awb_rdm_info[0].R;
    single_otp->iso_awb_info.gain_g =
        format_data->awb_cali_dat.awb_rdm_info[0].G;
    single_otp->iso_awb_info.gain_b =
        format_data->awb_cali_dat.awb_rdm_info[0].B;
    /*awb golden data*/
    single_otp->awb_golden_info.gain_r =
        format_data->awb_cali_dat.awb_gld_info[0].R;
    single_otp->awb_golden_info.gain_g =
        format_data->awb_cali_dat.awb_gld_info[0].G;
    single_otp->awb_golden_info.gain_b =
        format_data->awb_cali_dat.awb_gld_info[0].B;
    /*optical center*/
    memcpy((void *)&single_otp->optical_center_info,
           (void *)&format_data->opt_center_dat, sizeof(optical_center_t));
    /*lsc convert*/
    single_otp->lsc_info.lsc_data_addr =
        (cmr_u8 *)&format_data->lsc_cali_dat +
        format_data->lsc_cali_dat.lsc_calib_random.offset;
    single_otp->lsc_info.lsc_data_size =
        format_data->lsc_cali_dat.lsc_calib_random.length;
    /*lsc golden data*/
    single_otp->lsc_golden_info.lsc_data_addr =
        (cmr_u8 *)&format_data->lsc_cali_dat +
        format_data->lsc_cali_dat.lsc_calib_golden.offset;
    single_otp->lsc_golden_info.lsc_data_size =
        format_data->lsc_cali_dat.lsc_calib_golden.length;
    /*af convert*/
    single_otp->af_info.infinite_cali = format_data->af_cali_dat.infinity_dac;
    single_otp->af_info.macro_cali = format_data->af_cali_dat.macro_dac;
    /*pdaf convert*/
    single_otp->pdaf_info.pdaf_data_addr = format_data->pdaf_cali_dat.buffer;
    single_otp->pdaf_info.pdaf_data_size = format_data->pdaf_cali_dat.size;
    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGI("out");
    return 0;
}

/*just for expend*/
static cmr_int ov13855_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                     void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        ov13855_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGI("out");
    return ret;
}
