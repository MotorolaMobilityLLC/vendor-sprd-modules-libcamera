#include "ov13855_sunny_otp_drv.h"

/** ov13855_sunny_section_data_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static int _ov13855_sunny_section_checksum(unsigned char *buf,
                                           unsigned int offset,
                                           unsigned int data_count,
                                           unsigned int check_sum_offset) {
    uint32_t ret = OTP_CAMERA_SUCCESS;
    unsigned int i = 0, sum = 0;

    OTP_LOGI("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if (((sum % 255) + 1) == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out: offset:0x%x, sum:0x%x, checksum:0x%x", check_sum_offset, sum,
             buf[check_sum_offset]);
    return ret;
}

static int _ov13855_sunny_parse_checksum(void *otp_drv_handle) {
    uint32_t ret = OTP_CAMERA_SUCCESS;
    unsigned int i = 0, sum = 0;
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    /*module info*/
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, MODULE_INFO_OFFSET,
        MODULE_INFO_END_OFFSET - MODULE_INFO_OFFSET, MODULE_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("checksum error,parse failed");
        return ret;
    }
    /*ae data*/
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, AE_INFO_OFFSET,
        AE_INFO_CHECKSUM - AE_INFO_OFFSET, AE_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("ae otp data checksum error,parse failed.\n");
        return ret;
    }
    /*af data*/
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
        AF_INFO_END_OFFSET - AF_INFO_OFFSET, AF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("checksum error,parse failed");
        return ret;
    }
    /*awb data*/
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
        AWB_INFO_END_OFFSET - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb checksum error,parse failed");
        return ret;
    }
    /*lsc data*/
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_END_OFFSET - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("lsc checksum error,parse failed");
        return ret;
    }
    /*pdaf data*/
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, PDAF_LEVEL_INFO_OFFSET,
        PDAF_INFO_CHECKSUM - PDAF_LEVEL_INFO_OFFSET, PDAF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("pdaf otp data checksum error,parse failed.\n");
        return ret;
    }
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, DUAL_INFO_OFFSET,
        DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET, DUAL_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("dualcam otp data checksum error,parse failed.\n");
        return ret;
    }
    ret = _ov13855_sunny_section_checksum(
        otp_cxt->otp_raw_data.buffer, RES_INFO_OFFSET,
        RES_INFO_CHECKSUM - RES_INFO_OFFSET, RES_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("reserve otp data checksum error,parse failed.\n");
        return ret;
    }
    return ret;
};

static int _ov13855_sunny_buffer_init(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int otp_len;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*include random and golden lsc otp data,add reserve*/
    otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER;
    otp_format_data_t *otp_data = malloc(otp_len);
    if (NULL == otp_data) {
        OTP_LOGE("malloc otp data buffer failed.\n");
        ret = OTP_CAMERA_FAIL;
    } else {
        otp_cxt->otp_data_len = otp_len;
        memset(otp_data, 0, otp_len);
        lsccalib_data_t *lsc_data = &(otp_data->lsc_cali_dat);
        lsc_data->lsc_calib_golden.length = LSC_FORMAT_SIZE / 2;
        lsc_data->lsc_calib_golden.offset = sizeof(lsccalib_data_t);

        lsc_data->lsc_calib_random.length = LSC_FORMAT_SIZE / 2;
        lsc_data->lsc_calib_random.offset =
            sizeof(lsccalib_data_t) + LSC_FORMAT_SIZE / 2;
    }
    otp_cxt->otp_data = otp_data;
    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_ae_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    aecalib_data_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);
    uint8_t *ae_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;

    // for ae calibration
    ae_cali_dat->target_lum = (ae_src_dat[0] << 8) | ae_src_dat[1];
    ae_cali_dat->gain_1x_exp = (ae_src_dat[2] << 24) | (ae_src_dat[3] << 16) |
                               (ae_src_dat[4] << 8) | ae_src_dat[5];
    ae_cali_dat->gain_2x_exp = (ae_src_dat[6] << 24) | (ae_src_dat[7] << 16) |
                               (ae_src_dat[8] << 8) | ae_src_dat[9];
    ae_cali_dat->gain_4x_exp = (ae_src_dat[10] << 24) | (ae_src_dat[11] << 16) |
                               (ae_src_dat[12] << 8) | ae_src_dat[13];
    ae_cali_dat->gain_8x_exp = (ae_src_dat[14] << 24) | (ae_src_dat[15] << 16) |
                               (ae_src_dat[16] << 8) | ae_src_dat[17];

    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_module_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);
    uint8_t *module_info = NULL;

    /*begain read raw data, save module info */
    module_info =
        (uint8_t *)(otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET);
    module_dat->vendor_id = module_info[0];
    module_dat->moule_id =
        (module_info[1] << 16) | (module_info[2] << 8) | module_info[3];
    module_dat->calib_version = (module_info[4] << 8) | module_info[5];
    module_dat->year = module_info[6];
    module_dat->month = module_info[7];
    module_dat->day = module_info[8];
    module_dat->work_stat_id = (module_info[9] << 8) | module_info[10];
    module_dat->env_record = (module_info[11] << 8) | module_info[12];

    OTP_LOGI("moule_id:0x%x\n vendor_id:0x%x\n calib_version:%d\n "
             "work_stat_id:0x%x \n env_record :0x%x",
             module_dat->moule_id, module_dat->vendor_id,
             module_dat->calib_version, module_dat->work_stat_id,
             module_dat->env_record);

    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_af_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    afcalib_data_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    uint8_t *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;

    af_cali_dat->infinity_dac = (af_src_dat[1] << 8) | af_src_dat[0];
    af_cali_dat->macro_dac = (af_src_dat[3] << 8) | af_src_dat[2];
    af_cali_dat->afc_direction = af_src_dat[4];

    OTP_LOGD("af_infinity:0x%x,af_macro:0x%x,afc_direction:0x%x",
             af_cali_dat->infinity_dac, af_cali_dat->macro_dac,
             af_cali_dat->afc_direction);
    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_awb_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    uint8_t *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;
    uint32_t i;

    awb_cali_dat->awb_rdm_info[0].R = (awb_src_dat[1] << 8) | awb_src_dat[0];
    awb_cali_dat->awb_rdm_info[0].G = (awb_src_dat[3] << 8) | awb_src_dat[2];
    awb_cali_dat->awb_rdm_info[0].B = (awb_src_dat[5] << 8) | awb_src_dat[4];
    /*golden awb data ,you should ensure awb group number*/
    awb_cali_dat->awb_gld_info[0].R = golden_awb[0].R;
    awb_cali_dat->awb_gld_info[0].G = golden_awb[0].G;
    awb_cali_dat->awb_gld_info[0].B = golden_awb[0].B;

    OTP_LOGD("rdm:R=0x%x,G=0x%x,B=0x%x.gold:R=0x%x,G=0x%x,B=0x%x",
             awb_cali_dat->awb_rdm_info[0].R, awb_cali_dat->awb_rdm_info[0].G,
             awb_cali_dat->awb_rdm_info[0].B, awb_cali_dat->awb_gld_info[0].R,
             awb_cali_dat->awb_gld_info[0].G, awb_cali_dat->awb_gld_info[0].B);
    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_lsc_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    lsccalib_data_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    optical_center_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
    uint8_t *rdm_dst = (uint8_t *)lsc_dst + lsc_dst->lsc_calib_random.offset;
    uint8_t *gld_dst = (uint8_t *)lsc_dst + lsc_dst->lsc_calib_golden.offset;

    /*optical center data*/
    uint8_t *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
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
    memcpy(gld_dst, golden_lsc, LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
    lsc_dst->lsc_calib_golden.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;

    OTP_LOGD("optical_center:\nR=(0x%x,0x%x)\n GR=(0x%x,0x%x)\n "
             "GB=(0x%x,0x%x)\n B=(0x%x,0x%x)",
             opt_dst->R.x, opt_dst->R.y, opt_dst->GR.x, opt_dst->GR.y,
             opt_dst->GB.x, opt_dst->GB.y, opt_dst->B.x, opt_dst->B.y);

    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_pdaf_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    pdafcalib_data_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
    uint8_t *pdaf_level_src_dat =
        otp_cxt->otp_raw_data.buffer + PDAF_LEVEL_INFO_OFFSET;
    uint8_t *pdaf_phase_src_dat =
        otp_cxt->otp_raw_data.buffer + PDAF_PHASE_INFO_OFFSET;

    // for pdaf calibration
    pdaf_cali_dat->pdaf_level_x = pdaf_level_src_dat[0];
    pdaf_cali_dat->pdaf_level_y = pdaf_level_src_dat[1];
    pdaf_cali_dat->pdaf_level_cali_dat.buffer = pdaf_level_src_dat + 2;
    pdaf_cali_dat->pdaf_level_cali_dat.size = PDAF_LEVEL_DATA_SIZE;
    pdaf_cali_dat->pdaf_phase_x = pdaf_phase_src_dat[0];
    pdaf_cali_dat->pdaf_phase_y = pdaf_phase_src_dat[1];
    pdaf_cali_dat->pdaf_phase_cali_dat.buffer = pdaf_phase_src_dat + 2;
    pdaf_cali_dat->pdaf_phase_cali_dat.size = PDAF_PHASE_DATA_SIZE;

    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_parse_dualcam_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*dualcam data*/
    uint8_t *dualcam_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
    otp_cxt->otp_data->dual_cam_cali_dat.buffer = dualcam_src_dat;
    otp_cxt->otp_data->dual_cam_cali_dat.size =
        DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET;

    OTP_LOGI("out");
    return ret;
}

#if 0
static int _ov13855_sunny_parse_reserve_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*reserve data*/
	uint8_t *reserve_src_dat = otp_cxt->otp_raw_data.buffer + RES_INFO_OFFSET;
    else {
		//for reserve
		memcpy(reserve_src_dat, otp_cxt->otp_raw_data.buffer + RES_INFO_OFFSET,RES_DATA_SIZE);
        otp_cxt->otp_data->dual_cam_cali_dat.size =RES_DATA_SIZE;
	    }

   OTP_LOGI("out");
    return ret;
}
#endif

static int _ov13855_sunny_awb_calibration(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
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

    if (R_gain < G_gain || B_gain < G_gain) {
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
static int _ov13855_sunny_lsc_calibration(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    OTP_LOGI("out");
    return ret;
}

static int _ov13855_sunny_pdaf_calibration(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    OTP_LOGI("out");
    return ret;
}

/*==================================================
*                  External interface
====================================================*/

static int ov13855_sunny_otp_create(otp_drv_init_para_t *input_para,
                                    cmr_handle *sns_af_drv_handle) {
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static int ov13855_sunny_otp_drv_delete(void *otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

static int ov13855_sunny_otp_drv_read(void *otp_drv_handle,
                                      otp_params_t *p_data)

{
    cmr_int ret = OTP_CAMERA_SUCCESS;
    uint8_t try
        = 2;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    if (!otp_cxt->otp_raw_data.buffer) {
        uint8_t *buffer = malloc(OTP_LEN);
        if (NULL == buffer) {
            OTP_LOGE("malloc otp raw buffer failed\n");
            ret = OTP_CAMERA_FAIL;
        } else {
            memset(buffer, 0, OTP_LEN);
            otp_raw_data->buffer = buffer;
            otp_raw_data->num_bytes = OTP_LEN;
            _ov13855_sunny_buffer_init(otp_drv_handle);
        }
        /*start read otp data one time*/
        OTP_LOGI("read_length = 0x%x\n", SENSOR_I2C_REG_16BIT | OTP_LEN << 16);
#if 0
        while(try--){
            buffer[0] = (OTP_START_ADDR >> 8) & 0xFF;
            buffer[1] = OTP_START_ADDR & 0xFF;
            ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle,GT24C64A_I2C_ADDR, buffer, SENSOR_I2C_REG_16BIT | OTP_LEN << 16);
            OTP_LOGI("1,ret:0x%x,try:0x%x",ret,try);
            if (ret == OTP_CAMERA_SUCCESS)
                break;
        }
#endif
        int i = 0;
        uint8_t cmd_val[2];
        for (i = 0; i < OTP_LEN; i++) {
            cmd_val[0] = ((OTP_START_ADDR + i) >> 8) & 0xff;
            cmd_val[1] = (OTP_START_ADDR + i) & 0xff;
            hw_Sensor_ReadI2C(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
                              (uint8_t *)&cmd_val[0], 2);
            buffer[i] = cmd_val[0];
        }
    } else if (p_data) { /* if NULL read otp internal*/
        p_data->buffer = otp_raw_data->buffer;
        p_data->num_bytes = otp_raw_data->num_bytes;
    }
    sensor_otp_dump_raw_data(otp_cxt->otp_raw_data.buffer, OTP_LEN,
                             otp_cxt->dev_name);
    OTP_LOGI("out");
    return ret;
}

static int ov13855_sunny_otp_drv_write(void *otp_drv_handle,
                                       otp_params_t *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_data);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = p_data;

    if (NULL != otp_write_data->buffer) {
        int i;
        for (i = 0; i < otp_write_data->num_bytes; i++) {
            hw_Sensor_WriteI2C(otp_cxt->hw_handle, GT24C64A_I2C_ADDR,
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

static int _ov13855_sunny_split_data(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI(" split in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    _ov13855_sunny_parse_module_data(otp_drv_handle);
    _ov13855_sunny_parse_af_data(otp_drv_handle);
    _ov13855_sunny_parse_awb_data(otp_drv_handle);
    _ov13855_sunny_parse_lsc_data(otp_drv_handle);
    _ov13855_sunny_parse_pdaf_data(otp_drv_handle);
    _ov13855_sunny_parse_ae_data(otp_drv_handle);
    _ov13855_sunny_parse_dualcam_data(otp_drv_handle);
    // _ov13855_sunny_parse_reserve_data(otp_drv_handle);

    return ret;
};

static int ov13855_sunny_otp_drv_parse(void *otp_drv_handle, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(ov13855_sunny_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);

    uint8_t *otp_data_file = NULL;
    uint8_t otp_len = 0;

    ret = sensor_otp_rawdata_from_file(
        OTP_READ_FORMAT_FROM_BIN, otp_cxt->dev_name, &otp_data_file, OTP_LEN);

    if (ret == OTP_CAMERA_SUCCESS) {
        OTP_LOGI("otp has parse before,return directly");
        if (!otp_raw_data->buffer) {
            uint8_t *buffer = malloc(OTP_LEN);
            if (buffer == NULL) {
                OTP_LOGE("malloc otp raw buffer failed");
                ret = OTP_CAMERA_FAIL;
                return ret;
            } else {
                memset(buffer, 0, OTP_LEN);
                otp_raw_data->buffer = buffer;
                otp_raw_data->num_bytes = OTP_LEN;
            }
        }
        if (otp_raw_data->buffer) {
            memcpy((void *)otp_raw_data->buffer, otp_data_file, OTP_LEN);
        }

        if (!otp_cxt->otp_data) {
            otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE +
                      OTP_RESERVE_BUFFER;
            otp_cxt->otp_data = (otp_format_data_t *)malloc(otp_len);
            if (NULL == otp_cxt->otp_data) {
                OTP_LOGE("malloc otp data buffer failed.\n");
                ret = OTP_CAMERA_FAIL;
                return ret;
            } else {
                otp_cxt->otp_data_len = otp_len;
                memset(otp_cxt->otp_data, 0, otp_len);
            }
        }

        _ov13855_sunny_split_data(otp_drv_handle);

        return ret;
    } else if (otp_raw_data->buffer) {
        OTP_LOGI("drver has read otp raw data,start parsed.");

        ret = _ov13855_sunny_parse_checksum(otp_drv_handle);
        if (ret != OTP_CAMERA_SUCCESS) {
            OTP_LOGI(" otp checksum failed.");
            return OTP_CAMERA_FAIL;
        }
        _ov13855_sunny_split_data(otp_drv_handle);
        if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
            base_info->is_lsc_drv_decompression == TRUE) {
            ret = sensor_otp_lsc_decompress(
                &ov13855_sunny_drv_entry.otp_cfg.base_info_cfg,
                &otp_cxt->otp_data->lsc_cali_dat);
            if (ret != OTP_CAMERA_SUCCESS) {
                OTP_LOGE("base_info_cfg failed");
                return OTP_CAMERA_FAIL;
            }
        }

    } else {
        OTP_LOGE("should read otp before parse");
        return OTP_CAMERA_FAIL;
    }
    ret = sensor_otp_rawdata_from_file(
        OTP_WRITE_FORMAT_TO_BIN, otp_cxt->dev_name,
        &otp_cxt->otp_raw_data.buffer, otp_cxt->otp_raw_data.num_bytes);
    OTP_LOGI("out");

    return ret;
}

static int ov13855_sunny_otp_drv_calibration(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
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
    OTP_LOGI("Exit");
    return ret;
}

static int _ov13855_sunny_compatible_convert(otp_drv_cxt_t *otp_drv_handle,
                                             void *p_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_format_data_t *format_data = otp_cxt->otp_data;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;

    struct sensor_otp_cust_info *convert_data =
        malloc(sizeof(struct sensor_otp_cust_info));
    cmr_bzero(convert_data, sizeof(*convert_data));
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
    /*af convert*/
    convert_data->single_otp.af_info.flag = format_data->extend_dat.af_flag;
    convert_data->single_otp.af_info.infinite_cali =
        format_data->af_cali_dat.infinity_dac;
    convert_data->single_otp.af_info.macro_cali =
        format_data->af_cali_dat.macro_dac;
    /*pdaf convert*/
    convert_data->single_otp.pdaf_info.pdaf_level_x =
        format_data->dual_pdaf_cali_dat.pdaf_level_x;
    convert_data->single_otp.pdaf_info.pdaf_level_y =
        format_data->dual_pdaf_cali_dat.pdaf_level_y;
    convert_data->single_otp.pdaf_info.pdaf_data_addr =
        format_data->dual_pdaf_cali_dat.pdaf_level_cali_dat.buffer;
    convert_data->single_otp.pdaf_info.pdaf_data_size =
        format_data->dual_pdaf_cali_dat.pdaf_level_cali_dat.size;

    convert_data->single_otp.pdaf_info.pdaf_phase_x =
        format_data->dual_pdaf_cali_dat.pdaf_phase_x;
    convert_data->single_otp.pdaf_info.pdaf_phase_y =
        format_data->dual_pdaf_cali_dat.pdaf_phase_y;
    convert_data->single_otp.pdaf_info.pdaf_phase_data_addr =
        format_data->dual_pdaf_cali_dat.pdaf_phase_cali_dat.buffer;
    convert_data->single_otp.pdaf_info.pdaf_phase_data_size =
        format_data->dual_pdaf_cali_dat.pdaf_phase_cali_dat.size;

    /*spc*/
    convert_data->single_otp.spc_info.pdaf_data_addr =
        format_data->spc_cali_dat.buffer;
    convert_data->single_otp.spc_info.pdaf_data_size =
        format_data->spc_cali_dat.size;
    /*other*/
    convert_data->single_otp.program_flag =
        format_data->extend_dat.program_flag;
    convert_data->single_otp.checksum = format_data->extend_dat.checksum;

    /*dual camera*/
    convert_data->dual_otp.dual_flag = 1;
    convert_data->dual_otp.data_3d.data_ptr =
        format_data->dual_cam_cali_dat.buffer;
    convert_data->dual_otp.data_3d.size = format_data->dual_cam_cali_dat.size;
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
    /*awb convert*/
    convert_data->dual_otp.master_iso_awb_info.iso = format_data->iso_dat;
    convert_data->dual_otp.master_iso_awb_info.gain_r =
        format_data->awb_cali_dat.awb_rdm_info[0].R;
    convert_data->dual_otp.master_iso_awb_info.gain_g =
        format_data->awb_cali_dat.awb_rdm_info[0].G;
    convert_data->dual_otp.master_iso_awb_info.gain_b =
        format_data->awb_cali_dat.awb_rdm_info[0].B;
    /*awb golden data*/
    convert_data->single_otp.awb_golden_info.gain_r =
        format_data->awb_cali_dat.awb_gld_info[0].R;
    convert_data->single_otp.awb_golden_info.gain_g =
        format_data->awb_cali_dat.awb_gld_info[0].G;
    convert_data->single_otp.awb_golden_info.gain_b =
        format_data->awb_cali_dat.awb_gld_info[0].B;
    /*optical center*/
    memcpy((void *)&convert_data->dual_otp.master_optical_center_info,
           (void *)&format_data->opt_center_dat, sizeof(optical_center_t));
    /*lsc convert*/
    convert_data->dual_otp.master_lsc_info.lsc_data_addr =
        (uint8_t *)&format_data->lsc_cali_dat +
        format_data->lsc_cali_dat.lsc_calib_random.offset;
    convert_data->dual_otp.master_lsc_info.lsc_data_size =
        format_data->lsc_cali_dat.lsc_calib_random.length;
    /*lsc golden data*/
    convert_data->single_otp.lsc_golden_info.lsc_data_addr =
        (uint8_t *)&format_data->lsc_cali_dat +
        format_data->lsc_cali_dat.lsc_calib_golden.offset;
    convert_data->single_otp.lsc_golden_info.lsc_data_size =
        format_data->lsc_cali_dat.lsc_calib_golden.length;

    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGI("out");
    return 0;
}

/*just for expend*/
static int ov13855_sunny_otp_drv_ioctl(otp_drv_cxt_t *otp_drv_handle, int cmd,
                                       void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    params = params;
    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        _ov13855_sunny_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGI("out");
    return ret;
}
