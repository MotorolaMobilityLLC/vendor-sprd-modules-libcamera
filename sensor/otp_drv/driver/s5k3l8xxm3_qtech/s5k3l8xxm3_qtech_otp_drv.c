#include "s5k3l8xxm3_qtech_otp_drv.h"

/** s5k3l8xxm3_qtech_section_data_checksum:
 *    @buff: address of otp buffer
 *    @offset: the start address of the section
 *    @data_count: data count of the section
 *    @check_sum_offset: the section checksum offset
 *Return: unsigned char.
 **/
static cmr_int _s5k3l8xxm3_qtech_i2c_read(void *otp_drv_handle,
                                          uint16_t slave_addr,
                                          uint16_t memory_addr,
                                          uint8_t *memory_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint16_t cmd_len = 0;

    cmd_val[0] = memory_addr >> 8;
    cmd_val[1] = memory_addr & 0xff;
    cmd_len = 2;

    ret = hw_Sensor_ReadI2C(otp_cxt->hw_handle, slave_addr,
                            (uint8_t *)&cmd_val[0], cmd_len);
    if (OTP_CAMERA_SUCCESS == ret) {
        *memory_data = cmd_val[0];
    }
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_i2c_write(void *otp_drv_handle,
                                           uint16_t slave_addr,
                                           uint16_t memory_addr,
                                           uint16_t memory_data) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    uint8_t cmd_val[5] = {0x00};
    uint16_t cmd_len = 0;

    cmd_val[0] = memory_addr >> 8;
    cmd_val[1] = memory_addr & 0xff;
    cmd_val[2] = memory_data;
    cmd_len = 3;
    ret = hw_Sensor_WriteI2C(otp_cxt->hw_handle, slave_addr,
                             (uint8_t *)&cmd_val[0], cmd_len);

    return ret;
}
static cmr_int
_s5k3l8xxm3_qtech_section_checksum(unsigned char *buf, unsigned int offset,
                                   unsigned int data_count,
                                   unsigned int check_sum_offset) {
    uint32_t ret = OTP_CAMERA_SUCCESS;
    uint32_t i = 0, sum = 0;

    OTP_LOGI("in");
    for (i = offset; i < offset + data_count; i++) {
        sum += buf[i];
    }
    if (((sum % 256)) == buf[check_sum_offset]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out: offset:0x%x, sum:0x%x, checksum:0x%x", check_sum_offset, sum,
             buf[check_sum_offset]);
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_buffer_init(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_int otp_len;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*include random and golden lsc otp data,add reserve*/
    otp_len = sizeof(otp_format_data_t) + LSC_FORMAT_SIZE + OTP_RESERVE_BUFFER +
              DUAL_DATA_SIZE;
    otp_format_data_t *otp_data =
        (otp_format_data_t *)sensor_otp_get_formatted_buffer(
            otp_len, otp_cxt->sensor_id);
    if (NULL == otp_data) {
        OTP_LOGE("malloc otp data buffer failed.\n");
        ret = OTP_CAMERA_FAIL;
    } else {
        otp_cxt->otp_data_len = otp_len;
        lsccalib_data_t *lsc_data = &(otp_data->lsc_cali_dat);
        lsc_data->lsc_calib_golden.length =
            LSC_INFO_END_OFFSET - LSC_INFO_OFFSET;
        lsc_data->lsc_calib_golden.offset = sizeof(lsccalib_data_t);

        lsc_data->lsc_calib_random.length =
            LSC_INFO_END_OFFSET - LSC_INFO_OFFSET;
        lsc_data->lsc_calib_random.offset =
            sizeof(lsccalib_data_t) + lsc_data->lsc_calib_golden.length;
    }
    otp_cxt->otp_data = otp_data;
    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_ae_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    aecalib_data_t *ae_cali_dat = &(otp_cxt->otp_data->ae_cali_dat);

    /*TODO*/

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

    /*END*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);
    cmr_u8 lsc_grid_size = 0;
    /*begain read raw data, save module info */
    /*TODO*/
    cmr_u8 *module_info = NULL;

    module_info = otp_cxt->otp_raw_data.buffer + MODULE_INFO_OFFSET;
    module_dat->vendor_id = module_info[1];
    module_dat->moule_id =
        (module_info[1] << 16) | (module_info[2] << 8) | module_info[3];
    module_dat->calib_version = (module_info[4] << 8) | module_info[5];
    module_dat->year = module_info[6];
    module_dat->month = module_info[7];
    module_dat->day = module_info[8];
    module_dat->work_stat_id = (module_info[9] << 8) | module_info[10];
    module_dat->env_record = (module_info[11] << 8) | module_info[12];
    lsc_grid_size = module_info[13];
    /*END*/
    OTP_LOGI("moule_id:0x%x vendor_id:0x%x calib_version:%d work_stat_id:0x%x  "
             "env_record :0x%x",
             module_dat->moule_id, module_dat->vendor_id,
             module_dat->calib_version, module_dat->work_stat_id,
             module_dat->env_record);

    OTP_LOGI("lsc grid size=%d", lsc_grid_size);
    if (module_dat->vendor_id != MODULE_VENDOR_ID) {
        ret = OTP_CAMERA_FAIL;
    }

    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    afcalib_data_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);

    /*TODO*/

    cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer + AF_INFO_OFFSET;
    ret = _s5k3l8xxm3_qtech_section_checksum(
        otp_cxt->otp_raw_data.buffer, AF_INFO_OFFSET,
        AF_INFO_CHECKSUM - AF_INFO_OFFSET, AF_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("auto focus checksum error,parse failed");
        return ret;
    }
    af_cali_dat->infinity_dac = (af_src_dat[1] << 8) | af_src_dat[0];
    af_cali_dat->macro_dac = (af_src_dat[3] << 8) | af_src_dat[2];
    af_cali_dat->afc_direction = af_src_dat[4];

    /*END*/
    OTP_LOGD("af_infinity:0x%x,af_macro:0x%x,afc_direction:0x%x",
             af_cali_dat->infinity_dac, af_cali_dat->macro_dac,
             af_cali_dat->afc_direction);
    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_oc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    optical_center_t *opt_dst = &(otp_cxt->otp_data->opt_center_dat);
    ret = _s5k3l8xxm3_qtech_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGI("oc otp data checksum error,parse failed.");
        return ret;
    }
    cmr_u8 *opt_src = otp_cxt->otp_raw_data.buffer + OPTICAL_INFO_OFFSET;
    opt_dst->R.x = (opt_src[1] << 8) | opt_src[0];
    opt_dst->R.y = (opt_src[3] << 8) | opt_src[2];
    opt_dst->GR.x = (opt_src[5] << 8) | opt_src[4];
    opt_dst->GR.y = (opt_src[7] << 8) | opt_src[6];
    opt_dst->GB.x = (opt_src[9] << 8) | opt_src[8];
    opt_dst->GB.y = (opt_src[11] << 8) | opt_src[10];
    opt_dst->B.x = (opt_src[13] << 8) | opt_src[12];
    opt_dst->B.y = (opt_src[15] << 8) | opt_src[14];
    OTP_LOGD("optical_center:\n R=(0x%x,0x%x)\n GR=(0x%x,0x%x)\n "
             "GB=(0x%x,0x%x)\n B=(0x%x,0x%x)",
             opt_dst->R.x, opt_dst->R.y, opt_dst->GR.x, opt_dst->GR.y,
             opt_dst->GB.x, opt_dst->GB.y, opt_dst->B.x, opt_dst->B.y);
    OTP_LOGI("out");
    return ret;
}
static cmr_int _s5k3l8xxm3_qtech_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

    /*TODO*/

    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer + AWB_INFO_OFFSET;

    ret = _s5k3l8xxm3_qtech_section_checksum(
        otp_cxt->otp_raw_data.buffer, AWB_INFO_OFFSET,
        AWB_INFO_CHECKSUM - AWB_INFO_OFFSET, AWB_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("awb otp data checksum error,parse failed");
        return ret;
    }
    cmr_u32 i;
    OTP_LOGI("awb section count:0x%x", AWB_SECTION_NUM);
    for (i = 0; i < AWB_SECTION_NUM; i++, awb_src_dat += AWB_INFO_SIZE) {
        awb_cali_dat->awb_rdm_info[i].R =
            (awb_src_dat[1] << 8) | awb_src_dat[0];
        awb_cali_dat->awb_rdm_info[i].G =
            (awb_src_dat[3] << 8) | awb_src_dat[2];
        awb_cali_dat->awb_rdm_info[i].B =
            (awb_src_dat[5] << 8) | awb_src_dat[4];
        /*golden awb data ,you should ensure awb group number*/
        awb_cali_dat->awb_gld_info[i].R = golden_awb[i].R;
        awb_cali_dat->awb_gld_info[i].G = golden_awb[i].G;
        awb_cali_dat->awb_gld_info[i].B = golden_awb[i].B;
    }

    /*END*/
    OTP_LOGD("rdm:R=0x%x,G=0x%x,B=0x%x.gold:R=0x%x,G=0x%x,B=0x%x",
             awb_cali_dat->awb_rdm_info[0].R, awb_cali_dat->awb_rdm_info[0].G,
             awb_cali_dat->awb_rdm_info[0].B, awb_cali_dat->awb_gld_info[0].R,
             awb_cali_dat->awb_gld_info[0].G, awb_cali_dat->awb_gld_info[0].B);

    OTP_LOGD(
        "rdm:R/G=0x%x,Gr/Gb=0x%x,B/G=0x%x.gold:R/G=0x%x,Gr/Gb=0x%x,B/G=0x%x",
        awb_cali_dat->awb_rdm_info[0].rg_ratio,
        awb_cali_dat->awb_rdm_info[0].GrGb_ratio,
        awb_cali_dat->awb_rdm_info[0].bg_ratio,
        awb_cali_dat->awb_gld_info[0].rg_ratio,
        awb_cali_dat->awb_gld_info[0].GrGb_ratio,
        awb_cali_dat->awb_gld_info[0].bg_ratio);

    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    lsccalib_data_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *rdm_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_random.offset;
    cmr_u8 *gld_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_golden.offset;

    /*TODO*/

    ret = _s5k3l8xxm3_qtech_section_checksum(
        otp_cxt->otp_raw_data.buffer, OPTICAL_INFO_OFFSET,
        LSC_INFO_CHECKSUM - OPTICAL_INFO_OFFSET, LSC_INFO_CHECKSUM);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("lsc checksum error,parse failed");
        return ret;
    }

    /*random data*/
    memcpy(rdm_dst, otp_cxt->otp_raw_data.buffer + LSC_INFO_OFFSET,
           LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
    lsc_dst->lsc_calib_random.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;

    /*END*/

    /*gold data*/
    memcpy(gld_dst, golden_lsc, LSC_INFO_CHECKSUM - LSC_INFO_OFFSET);
    lsc_dst->lsc_calib_golden.length = LSC_INFO_CHECKSUM - LSC_INFO_OFFSET;

    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*TODO*/

    cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer + PDAF_INFO_OFFSET;
    ret = _s5k3l8xxm3_qtech_section_checksum(
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

    /*END*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_parse_dualcam_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;

    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    /*TODO*/

    /*dualcam data*/
    cmr_u8 *dualcam_src_dat = otp_cxt->otp_raw_data.buffer + DUAL_INFO_OFFSET;
    otp_cxt->otp_data->dual_cam_cali_dat.buffer = dualcam_src_dat;
    otp_cxt->otp_data->dual_cam_cali_dat.size =
        DUAL_INFO_CHECKSUM - DUAL_INFO_OFFSET;

    /*END*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_awb_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    awbcalib_data_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);

    /*TODO*/

    /*END*/

    OTP_LOGI("out");
    return ret;
}
static cmr_int _s5k3l8xxm3_qtech_lsc_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    lsccalib_data_t *lsc_dst = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *rdm_dst = (cmr_u8 *)lsc_dst + lsc_dst->lsc_calib_random.offset;

    /*TODO*/

    /*END*/
    OTP_LOGI("out");
    return ret;
}

static int _s5k3l8xxm3_qtech_pdaf_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    OTP_LOGI("in");
    CHECK_PTR(otp_drv_handle);

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    OTP_LOGI("out");
    return ret;
}

static cmr_int _s5k3l8xxm3_qtech_split_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_calib_items_t *cal_items =
        &(s5k3l8xxm3_qtech_drv_entry.otp_cfg.cali_items);

    ret = _s5k3l8xxm3_qtech_parse_module_data(otp_drv_handle);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("parse module info failed");
        return ret;
    }
    if (cal_items->is_self_cal) {
        ret = _s5k3l8xxm3_qtech_parse_oc_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse oc data failed");
            return ret;
        }
    }

    if (cal_items->is_afc) {
        ret = _s5k3l8xxm3_qtech_parse_af_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse af data failed");
            return ret;
        }
    }

    if (cal_items->is_awbc) {
        ret = _s5k3l8xxm3_qtech_parse_awb_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse awb data failed");
            return ret;
        }
    }

    if (cal_items->is_lsc) {
        ret = _s5k3l8xxm3_qtech_parse_lsc_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse lsc data failed");
            return ret;
        }
    }

    if (cal_items->is_pdafc) {
        ret = _s5k3l8xxm3_qtech_parse_pdaf_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse pdaf data failed");
            return ret;
        }
    }

    if (cal_items->is_dul_camc) {
        ret = _s5k3l8xxm3_qtech_parse_ae_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse ae data failed");
            return ret;
        }
        ret = _s5k3l8xxm3_qtech_parse_dualcam_data(otp_drv_handle);
        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGE("parse dualcam data failed");
            return ret;
        }
    }

    OTP_LOGI("out");
    return ret;
};

static cmr_int _s5k3l8xxm3_qtech_compatible_convert(cmr_handle otp_drv_handle,
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
        s5k3l8xxm3_qtech_drv_entry.otp_cfg.base_info_cfg.full_img_width;
    convert_data->dual_otp.master_lsc_info.full_img_height =
        s5k3l8xxm3_qtech_drv_entry.otp_cfg.base_info_cfg.full_img_height;
    convert_data->dual_otp.master_lsc_info.lsc_otp_grid =
        s5k3l8xxm3_qtech_drv_entry.otp_cfg.base_info_cfg.lsc_otp_grid;

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

    /*pdaf convert*/
    single_otp->pdaf_info.pdaf_data_addr = format_data->pdaf_cali_dat.buffer;
    single_otp->pdaf_info.pdaf_data_size = format_data->pdaf_cali_dat.size;

    /*dual camera*/
    convert_data->dual_otp.dual_flag = 1;
    convert_data->dual_otp.data_3d.data_ptr =
        format_data->dual_cam_cali_dat.buffer;
    convert_data->dual_otp.data_3d.size = format_data->dual_cam_cali_dat.size;

    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
    OTP_LOGI("out");
    return ret;
}

/*==================================================
*                  External interface
====================================================*/

static cmr_int s5k3l8xxm3_qtech_otp_drv_create(otp_drv_init_para_t *input_para,
                                               cmr_handle *sns_af_drv_handle) {
    OTP_LOGI("in");
    return sensor_otp_drv_create(input_para, sns_af_drv_handle);
}

static cmr_int s5k3l8xxm3_qtech_otp_drv_delete(cmr_handle otp_drv_handle) {

    OTP_LOGI("in");
    return sensor_otp_drv_delete(otp_drv_handle);
}

static cmr_int s5k3l8xxm3_qtech_otp_drv_read(cmr_handle otp_drv_handle,
                                             void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    // CHECK_PTR(p_params);
    OTP_LOGI("in");

    cmr_u8 *buffer = NULL;
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    otp_params_t *p_data = (otp_params_t *)p_params;

    if (!otp_cxt->otp_raw_data.buffer) {
        /*when mobile power on , it will init*/
        buffer =
            sensor_otp_get_raw_buffer(OTP_RAW_DATA_LEN, otp_cxt->sensor_id);
        if (NULL == buffer) {
            OTP_LOGE("malloc otp raw buffer failed\n");
            ret = OTP_CAMERA_FAIL;
        } else {
            otp_raw_data->buffer = buffer;
            otp_raw_data->num_bytes = OTP_RAW_DATA_LEN;
            _s5k3l8xxm3_qtech_buffer_init(otp_drv_handle);
        }
    }

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp raw data has read before,return directly");
        if (p_data) {
            p_data->buffer = otp_raw_data->buffer;
            p_data->num_bytes = otp_raw_data->num_bytes;
        }
        return ret;
    } else {
        /*start read otp data one time*/
        /*TODO*/

        ret = hw_sensor_read_i2c(otp_cxt->hw_handle, OTP_I2C_ADDR,
                                 (cmr_u8 *)buffer,
                                 SENSOR_I2C_REG_16BIT | OTP_RAW_DATA_LEN << 16);

        if (OTP_CAMERA_SUCCESS != ret) {
            OTP_LOGI("i2c read error");
        } else {
            OTP_LOGI("i2c read sucess");
        }
        /*END*/
    }

    sensor_otp_dump_raw_data(otp_cxt->otp_raw_data.buffer, OTP_RAW_DATA_LEN,
                             otp_cxt->dev_name);
    OTP_LOGI("out");
    return ret;
}

static cmr_int s5k3l8xxm3_qtech_otp_drv_write(cmr_handle otp_drv_handle,
                                              void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_params_t *otp_write_data = p_params;

    if (NULL != otp_write_data->buffer) {
        OTP_LOGI("write %s dev otp,buffer:0x%x,size:%d", otp_cxt->dev_name,
                 otp_write_data->buffer, otp_write_data->num_bytes);

        /*TODO*/

        /*END*/
    } else {
        OTP_LOGE("ERROR:buffer pointer is null");
        ret = OTP_CAMERA_FAIL;
    }
    OTP_LOGI("out");
    return ret;
}

static cmr_int s5k3l8xxm3_qtech_otp_drv_parse(cmr_handle otp_drv_handle,
                                              void *p_params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    // CHECK_PTR(p_params);
    OTP_LOGI("in");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    otp_base_info_cfg_t *base_info =
        &(s5k3l8xxm3_qtech_drv_entry.otp_cfg.base_info_cfg);
    otp_params_t *otp_raw_data = &(otp_cxt->otp_raw_data);
    module_data_t *module_dat = &(otp_cxt->otp_data->module_dat);

    if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
        OTP_LOGI("otp has parse before,return directly");
        return ret;
    }

    if (!otp_raw_data->buffer) {
        OTP_LOGI("should read otp before parse");
        ret = s5k3l8xxm3_qtech_otp_drv_read(otp_drv_handle, NULL);
        if (ret != OTP_CAMERA_SUCCESS) {
            return OTP_CAMERA_FAIL;
        }
    }

    /*begin read raw data, save module info */
    OTP_LOGI("drver has read otp raw data,start parsed.");

    ret = _s5k3l8xxm3_qtech_split_data(otp_drv_handle);
    if (ret != OTP_CAMERA_SUCCESS) {
        return OTP_CAMERA_FAIL;
    }

    /*decompress lsc data if needed*/
    if ((base_info->compress_flag != GAIN_ORIGIN_BITS) &&
        base_info->is_lsc_drv_decompression == TRUE) {
        ret = sensor_otp_lsc_decompress(
            &s5k3l8xxm3_qtech_drv_entry.otp_cfg.base_info_cfg,
            &otp_cxt->otp_data->lsc_cali_dat);
        if (ret != OTP_CAMERA_SUCCESS) {
            return OTP_CAMERA_FAIL;
        }
    }
    sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1); /*read to memory*/

    OTP_LOGI("out");
    return ret;
}

static cmr_int s5k3l8xxm3_qtech_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    otp_calib_items_t *cali_items =
        &(s5k3l8xxm3_qtech_drv_entry.otp_cfg.cali_items);

    if (cali_items->is_pdafc && cali_items->is_pdaf_self_cal) {
        _s5k3l8xxm3_qtech_pdaf_calibration(otp_drv_handle);
    }
    /*calibration at sensor or isp */
    if (cali_items->is_awbc && cali_items->is_awbc_self_cal) {
        _s5k3l8xxm3_qtech_awb_calibration(otp_drv_handle);
    }

    if (cali_items->is_lsc && cali_items->is_awbc_self_cal) {
        _s5k3l8xxm3_qtech_lsc_calibration(otp_drv_handle);
    }
    /*If there are other items that need calibration, please add to here*/

    OTP_LOGI("out");
    return ret;
}

/*just for expend*/
static cmr_int s5k3l8xxm3_qtech_otp_drv_ioctl(cmr_handle otp_drv_handle,
                                              cmr_uint cmd, void *params) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGI("in");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        _s5k3l8xxm3_qtech_compatible_convert(otp_drv_handle, params);
        break;
    default:
        break;
    }
    OTP_LOGI("out");
    return ret;
}
