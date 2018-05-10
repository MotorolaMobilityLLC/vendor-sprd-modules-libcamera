#include "single_1e_otp_drv_1v0.h"

/*
 *  single camera one eeprom - single camera otp driver
 *  for otp v1.0
 *  not compat otp v0.4 or others
 *  must make sure module info correct
 */

/*==================================================
*                Internal Functions
====================================================*/
static cmr_int _single_otp_section_checksum(cmr_u8 *buffer, cmr_uint start_addr,
                                            cmr_uint size,
                                            cmr_uint checksum_addr) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 i = 0, sum = 0;

    OTP_LOGV("E");
    for (i = start_addr; i < start_addr + size; i++) {
        sum += buffer[i];
    }
    if ((sum % 256) == buffer[checksum_addr]) {
        ret = OTP_CAMERA_SUCCESS;
    } else {
        ret = CMR_CAMERA_FAIL;
    }

    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGD("X:failed:checksum_addr=0x%lx, checksum_value=%d, sum=%d, "
                 "sum%%256=%d",
                 checksum_addr, buffer[checksum_addr], sum, sum % 256);
    } else {
        OTP_LOGD("X:passed:checksum_addr=0x%lx, checksum_value=%d, sum=%d",
                 checksum_addr, buffer[checksum_addr], sum);
    }
    return ret;
}

static struct module_info_t single_module_info;

static cmr_int _single_otp_parse_module_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u8 *buffer = otp_cxt->otp_raw_data.buffer;
    struct module_info_t *module_info = &single_module_info;
    otp_section_info_t *module_dat = &(otp_cxt->otp_data->module_dat);

    module_info->calib_version = (buffer[4] << 8) | buffer[5];
    OTP_LOGI("calibration version is 0x%04x", module_info->calib_version);
    if (buffer[0] == 0x53 && buffer[1] == 0x50 && buffer[2] == 0x52 &&
        buffer[3] == 0x44 && module_info->calib_version == 0x0100) {
        module_info->otp_version = OTP_1_0;
        OTP_LOGI("this is SPRD otp map, otp version is 1.0");
    } else {
        OTP_LOGE("this isn't otp 1.0, please check otp config");
        /* include v0.1, v0.2, v0.3, v0.4, v0.5 */
        module_info->otp_version = OTP_0_4;

        if ((module_info->calib_version == 0x0001) ||
            ((module_info->calib_version == 0x0100) &&
             (buffer[0] != 0x53 || buffer[1] != 0x50 || buffer[2] != 0x52 ||
              buffer[3] != 0x44)))
            OTP_LOGI("otp version is 0.1");

        if ((module_info->calib_version == 0x0002) ||
            (module_info->calib_version == 0x0200))
            OTP_LOGI("otp version is 0.2");

        if ((module_info->calib_version == 0x0003) ||
            (module_info->calib_version == 0x0300))
            OTP_LOGI("otp version is 0.3");

        if ((module_info->calib_version == 0x0004) ||
            (module_info->calib_version == 0x0400))
            OTP_LOGI("otp version is 0.4");

        if ((module_info->calib_version == 0x0005) ||
            (module_info->calib_version == 0x0500))
            OTP_LOGI("otp version is 0.5");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _single_otp_section_checksum(otp_cxt->otp_raw_data.buffer, 0x00, 80,
                                       0x50);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("module info checksum error!");
        return ret;
    }
    OTP_LOGD("module info checksum passed, otp version is 1.0");

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

    switch (module_info->module_id_info.master_vendor_id) {
    case 1:
        OTP_LOGI("module vendor is Sunny");
        break;
    case 2:
        OTP_LOGI("module vendor is Truly");
        break;
    case 3:
        OTP_LOGI("module vendor is ReachTech");
        break;
    case 4:
        OTP_LOGI("module vendor is Q-Tech");
        break;
    case 5:
        OTP_LOGI("module vendor is Altek");
        break;
    case 6:
        OTP_LOGI("module vendor is CameraKing");
        break;
    case 7:
        OTP_LOGI("module vendor is Shine");
        break;
    case 8:
        OTP_LOGI("module vendor is Darling");
        break;
    default:
        OTP_LOGI("module vendor id is not in list");
        break;
    }
    OTP_LOGI(
        "master_vendor_id = 0x%x, master_lens_id = 0x%x, master_vcm_id = 0x%x",
        module_info->module_id_info.master_vendor_id,
        module_info->module_id_info.master_lens_id,
        module_info->module_id_info.master_vcm_id);
    OTP_LOGI(
        "slave_vendor_id = 0x%x, slave_lens_id = 0x%x, slave_vcm_id = 0x%x",
        module_info->module_id_info.slave_vendor_id,
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

    module_info->master_start_addr.master_af_addr =
        (buffer[29] << 8) | buffer[28];
    module_info->master_start_addr.master_awb_addr =
        (buffer[31] << 8) | buffer[30];
    module_info->master_start_addr.master_lsc_addr =
        (buffer[33] << 8) | buffer[32];
    module_info->master_start_addr.master_pdaf1_addr =
        (buffer[35] << 8) | buffer[34];
    module_info->master_start_addr.master_pdaf2_addr =
        (buffer[37] << 8) | buffer[36];
    module_info->master_start_addr.master_ae_addr =
        (buffer[39] << 8) | buffer[38];
    module_info->master_start_addr.master_dualcam_addr =
        (buffer[41] << 8) | buffer[40];

    module_info->slave_start_addr.slave_af_addr =
        (buffer[47] << 8) | buffer[46];
    module_info->slave_start_addr.slave_awb_addr =
        (buffer[49] << 8) | buffer[48];
    module_info->slave_start_addr.slave_lsc_addr =
        (buffer[51] << 8) | buffer[50];
    module_info->slave_start_addr.slave_ae_addr =
        (buffer[53] << 8) | buffer[52];

    module_info->master_size.master_af_size = buffer[56];
    module_info->master_size.master_awb_size = buffer[57];
    module_info->master_size.master_lsc_size = (buffer[59] << 8) | buffer[58];
    module_info->master_size.master_pdaf1_size = (buffer[61] << 8) | buffer[60];
    module_info->master_size.master_pdaf2_size = (buffer[63] << 8) | buffer[62];
    module_info->master_size.master_ae_size = buffer[64];
    module_info->master_size.master_dualcam_size =
        (buffer[66] << 8) | buffer[65];

    module_info->slave_size.slave_af_size = buffer[71];
    module_info->slave_size.slave_awb_size = buffer[72];
    module_info->slave_size.slave_lsc_size = (buffer[74] << 8) | buffer[73];
    module_info->slave_size.slave_ae_size = buffer[75];

    OTP_LOGD("master(addr, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
             "pdaf1(0x%x, %d), pdaf2(0x%x, %d), ae(0x%x, %d), dualcam(0x%x, "
             "%d)",
             module_info->master_start_addr.master_af_addr,
             module_info->master_size.master_af_size,
             module_info->master_start_addr.master_awb_addr,
             module_info->master_size.master_awb_size,
             module_info->master_start_addr.master_lsc_addr,
             module_info->master_size.master_lsc_size,
             module_info->master_start_addr.master_pdaf1_addr,
             module_info->master_size.master_pdaf1_size,
             module_info->master_start_addr.master_pdaf2_addr,
             module_info->master_size.master_pdaf2_size,
             module_info->master_start_addr.master_ae_addr,
             module_info->master_size.master_ae_size,
             module_info->master_start_addr.master_dualcam_addr,
             module_info->master_size.master_dualcam_size);

    OTP_LOGD("slave(addr, size): af(0x%x, %d), awb(0x%x, %d), lsc(0x%x, %d), "
             "ae(0x%x, %d)",
             module_info->slave_start_addr.slave_af_addr,
             module_info->slave_size.slave_af_size,
             module_info->slave_start_addr.slave_awb_addr,
             module_info->slave_size.slave_awb_size,
             module_info->slave_start_addr.slave_lsc_addr,
             module_info->slave_size.slave_lsc_size,
             module_info->slave_start_addr.slave_ae_addr,
             module_info->slave_size.slave_ae_size);

    module_info->sensor_max_width = (buffer[124] << 8) | buffer[123];
    module_info->sensor_max_height = (buffer[126] << 8) | buffer[125];
    module_info->lsc_grid = buffer[127];
    module_info->resolution =
        (module_info->sensor_max_width * module_info->sensor_max_height +
         500000) /
        1000000;
    OTP_LOGI("lsc_img_width = %d, lsc_img_height = %d, lsc_grid = %d, "
             "resolution = %dM",
             module_info->sensor_max_width, module_info->sensor_max_height,
             module_info->lsc_grid, module_info->resolution);

    module_dat->rdm_info.buffer = buffer;
    module_dat->rdm_info.size = 80;
    module_dat->gld_info.buffer = NULL;
    module_dat->gld_info.size = 0;

    OTP_LOGV("X");
    return ret;
}

static cmr_int _single_otp_parse_af_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &single_module_info;
    otp_section_info_t *af_cali_dat = &(otp_cxt->otp_data->af_cali_dat);
    cmr_u8 *af_src_dat = otp_cxt->otp_raw_data.buffer +
                         module_info->master_start_addr.master_af_addr;
    char value[255];

    if (!module_info->master_start_addr.master_af_addr) {
        OTP_LOGE("AF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_size.master_af_size) {
        OTP_LOGE("AF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _single_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer,
        module_info->master_start_addr.master_af_addr,
        module_info->master_size.master_af_size,
        module_info->master_start_addr.master_af_addr +
            module_info->master_size.master_af_size);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AF checksum error");
        return ret;
    }
    OTP_LOGD("AF checksum passed");
    af_cali_dat->rdm_info.buffer = af_src_dat;
    af_cali_dat->rdm_info.size = module_info->master_size.master_af_size;
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

static cmr_int _single_otp_parse_awb_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &single_module_info;
    otp_section_info_t *awb_cali_dat = &(otp_cxt->otp_data->awb_cali_dat);
    cmr_u8 *awb_src_dat = otp_cxt->otp_raw_data.buffer +
                          module_info->master_start_addr.master_awb_addr;
    char value[255];

    if (!module_info->master_start_addr.master_awb_addr) {
        OTP_LOGE("AWB section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_size.master_awb_size) {
        OTP_LOGE("AWB section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _single_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer,
        module_info->master_start_addr.master_awb_addr,
        module_info->master_size.master_awb_size,
        module_info->master_start_addr.master_awb_addr +
            module_info->master_size.master_awb_size);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("AWB checksum error");
        return ret;
    }
    OTP_LOGD("AWB checksum passed");
    awb_cali_dat->rdm_info.buffer = awb_src_dat;
    awb_cali_dat->rdm_info.size = module_info->master_size.master_awb_size;
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

static cmr_int _single_otp_parse_lsc_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &single_module_info;
    otp_section_info_t *lsc_cali_dat = &(otp_cxt->otp_data->lsc_cali_dat);
    cmr_u8 *lsc_src_dat = otp_cxt->otp_raw_data.buffer +
                          module_info->master_start_addr.master_lsc_addr;
    char value[255];

    if (!module_info->master_start_addr.master_lsc_addr) {
        OTP_LOGE("LSC section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_size.master_lsc_size) {
        OTP_LOGE("LSC section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _single_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer,
        module_info->master_start_addr.master_lsc_addr,
        module_info->master_size.master_lsc_size,
        module_info->master_start_addr.master_lsc_addr +
            module_info->master_size.master_lsc_size);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("LSC checksum error");
        return ret;
    }
    OTP_LOGD("LSC checksum passed");
    lsc_cali_dat->rdm_info.buffer = lsc_src_dat;
    lsc_cali_dat->rdm_info.size = module_info->master_size.master_lsc_size;
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

        OTP_LOGI("lsc_version = %d", lsc_data.lsc_version);
        OTP_LOGI("optical_center: R=(%d,%d), Gr=(%d,%d), Gb=(%d,%d), B=(%d,%d)",
                 lsc_data.lsc_oc_r_x, lsc_data.lsc_oc_r_y, lsc_data.lsc_oc_gr_x,
                 lsc_data.lsc_oc_gr_y, lsc_data.lsc_oc_gb_x,
                 lsc_data.lsc_oc_gb_y, lsc_data.lsc_oc_b_x,
                 lsc_data.lsc_oc_b_y);
        OTP_LOGI("lsc_img_width = %d, lsc_img_height = %d, lsc_grid = %d",
                 lsc_data.lsc_img_width, lsc_data.lsc_img_height,
                 lsc_data.lsc_grid);

        lsc_data.resolution =
            (lsc_data.lsc_img_width * lsc_data.lsc_img_height + 500000) /
            1000000;
        switch (lsc_data.resolution) {
        case 16:
            lsc_data.lsc_channel_size = 526; // 16M
            lsc_data.lsc_width = 20;
            lsc_data.lsc_height = 15;
            break;

        case 13:
            lsc_data.lsc_channel_size = 726; // 13M
            lsc_data.lsc_width = 23;
            lsc_data.lsc_height = 18;
            break;

        case 12:
            lsc_data.lsc_channel_size = 656; // 12M
            lsc_data.lsc_width = 22;
            lsc_data.lsc_height = 17;
            break;

        case 8:
            lsc_data.lsc_channel_size = 442; // 8M
            lsc_data.lsc_width = 18;
            lsc_data.lsc_height = 14;
            break;

        case 5:
            lsc_data.lsc_channel_size = 656; // 5M
            lsc_data.lsc_width = 22;
            lsc_data.lsc_height = 17;
            break;

        case 2:
            lsc_data.lsc_channel_size = 270; // 2M
            lsc_data.lsc_width = 14;
            lsc_data.lsc_height = 11;
            break;

        default:
            OTP_LOGI("sensor resolution is unusual, there's no "
                     "lsc_channel_size info in list");
            lsc_data.lsc_channel_size = 0;
            if (lsc_data.lsc_grid) {
                lsc_data.lsc_width =
                    (int)(lsc_data.lsc_img_width / (2 * lsc_data.lsc_grid)) + 1;
                lsc_data.lsc_height =
                    (int)(lsc_data.lsc_img_height / (2 * lsc_data.lsc_grid)) +
                    1;
                if (lsc_data.lsc_img_width % (2 * lsc_data.lsc_grid) != 0) {
                    lsc_data.lsc_width += 1;
                }
                if (lsc_data.lsc_img_height % (2 * lsc_data.lsc_grid) != 0) {
                    lsc_data.lsc_height += 1;
                }
            }
            break;
        }
        OTP_LOGI("resolution = %dM, lsc_channel_size = %d, lsc_width = %d, "
                 "lsc_height = %d",
                 lsc_data.resolution, lsc_data.lsc_channel_size,
                 lsc_data.lsc_width, lsc_data.lsc_height);

        if ((22 + lsc_data.lsc_channel_size * 4) !=
            module_info->master_size.master_lsc_size) {
            OTP_LOGI("module info master_lsc_size doesn't match "
                     "lsc_channel_size rule");
        }
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int _single_otp_parse_pdaf_data(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    struct module_info_t *module_info = &single_module_info;
    otp_section_info_t *pdaf_cali_dat = &(otp_cxt->otp_data->pdaf_cali_dat);
    cmr_u8 *pdaf_src_dat = otp_cxt->otp_raw_data.buffer +
                           module_info->master_start_addr.master_pdaf1_addr;
    char value[255];

    if (!module_info->master_start_addr.master_pdaf1_addr) {
        OTP_LOGE("PDAF section start address is null");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }
    if (!module_info->master_size.master_pdaf1_size) {
        OTP_LOGE("PDAF section size is 0");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    ret = _single_otp_section_checksum(
        otp_cxt->otp_raw_data.buffer,
        module_info->master_start_addr.master_pdaf1_addr,
        module_info->master_size.master_pdaf1_size,
        module_info->master_start_addr.master_pdaf1_addr +
            module_info->master_size.master_pdaf1_size);
    if (OTP_CAMERA_SUCCESS != ret) {
        OTP_LOGE("PDAF checksum error");
        return ret;
    }
    OTP_LOGD("PDAF checksum passed");
    pdaf_cali_dat->rdm_info.buffer = pdaf_src_dat;
    pdaf_cali_dat->rdm_info.size = module_info->master_size.master_pdaf1_size;
    pdaf_cali_dat->gld_info.buffer = NULL;
    pdaf_cali_dat->gld_info.size = 0;

    property_get("debug.camera.parse.otp.hal.log", value, "0");
    if (atoi(value) == 1) {
        cmr_u8 pdaf_version = pdaf_src_dat[0];
        OTP_LOGI("pdaf_version = %d", pdaf_version);
    }

    OTP_LOGV("X");
    return ret;
}

/*==================================================
*                External interface
====================================================*/

static cmr_int single_otp_drv_create(otp_drv_init_para_t *input_para,
                                     cmr_handle *otp_drv_handle) {
    return sensor_otp_drv_create(input_para, otp_drv_handle);
}

static cmr_int single_otp_drv_delete(cmr_handle otp_drv_handle) {
    return sensor_otp_drv_delete(otp_drv_handle);
}

/*malloc buffer and read otp raw data from eeprom or bin file*/
static cmr_int single_otp_drv_read(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    char otp_bin_ext_path[255];
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

    property_get("debug.camera.read.otp.from.bin", value2, "0");
    if (atoi(value2) == 1) {
        /* read otp from bin file */
        snprintf(otp_bin_ext_path, sizeof(otp_bin_ext_path), "%s%s_otp.bin",
                 "/data/vendor/cameraserver/", "single");
        OTP_LOGD("otp_data_read_path:%s", otp_bin_ext_path);
        if (-1 == access(otp_bin_ext_path, 0)) {
            OTP_LOGE("otp bin file don't exist");
            ret = OTP_CAMERA_FAIL;
            goto exit;
        }
        fp = fopen(otp_bin_ext_path, "rb");
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
        /* read otp from eeprom */
        otp_cxt->otp_raw_data.buffer[0] = 0;
        otp_cxt->otp_raw_data.buffer[1] = 0;
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
exit:
    if (OTP_CAMERA_SUCCESS == ret) {
        property_get("debug.camera.save.otp.raw.data", value3, "0");
        if (atoi(value3) == 1) {
            if (sensor_otp_dump_raw_data(otp_cxt->otp_raw_data.buffer,
                                         otp_cxt->eeprom_size, "single"))
                OTP_LOGE("dump otp bin file failed");
        }
    }
    OTP_LOGV("X");
    return ret;
}

static cmr_int single_otp_drv_write(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    CHECK_PTR(param);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("X");
    return ret;
}

/*chesksum in parse operation*/
static cmr_int single_otp_drv_parse(cmr_handle otp_drv_handle, void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    char value[255];

    property_get("debug.parse.otp.open.camera", value, "0");
    if (atoi(value) == 0) {
        if (sensor_otp_get_buffer_state(otp_cxt->sensor_id)) {
            OTP_LOGD("otp data has been parsed before, is still in memory, "
                     "will not be parsed again");
            return ret;
        }
    }

    if (otp_cxt->otp_raw_data.buffer) {
        OTP_LOGD("otp raw buffer is not null, start checksum");
        _single_otp_parse_module_data(otp_drv_handle);
        _single_otp_parse_af_data(otp_drv_handle);
        _single_otp_parse_awb_data(otp_drv_handle);
        _single_otp_parse_lsc_data(otp_drv_handle);
        _single_otp_parse_pdaf_data(otp_drv_handle);

        /*set buffer_state to 1, means otp data saved in memory*/
        sensor_otp_set_buffer_state(otp_cxt->sensor_id, 1);
    } else {
        OTP_LOGE("otp raw buffer is null, can't checksum");
        ret = OTP_CAMERA_FAIL;
        return ret;
    }

    OTP_LOGV("X");
    return ret;
}

static cmr_int single_otp_drv_calibration(cmr_handle otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGV("E");

    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGV("X");
    return ret;
}

/*pass data to ispalg or depth*/
static cmr_int single_otp_compatible_convert(cmr_handle otp_drv_handle,
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
    cmr_bzero(convert_data, sizeof(*convert_data));

    convert_data->total_otp.data_ptr = otp_cxt->otp_raw_data.buffer;
    convert_data->total_otp.size = otp_cxt->otp_raw_data.num_bytes;

    convert_data->otp_vendor = OTP_VENDOR_SINGLE;

    convert_data->single_otp.module_info =
        (struct sensor_otp_section_info *)&format_data->module_dat;
    convert_data->single_otp.af_info =
        (struct sensor_otp_section_info *)&format_data->af_cali_dat;
    convert_data->single_otp.iso_awb_info =
        (struct sensor_otp_section_info *)&format_data->awb_cali_dat;
    convert_data->single_otp.lsc_info =
        (struct sensor_otp_section_info *)&format_data->lsc_cali_dat;
    convert_data->single_otp.pdaf_info =
        (struct sensor_otp_section_info *)&format_data->pdaf_cali_dat;

    otp_cxt->compat_convert_data = convert_data;
    p_val->pval = convert_data;
    p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;

    OTP_LOGV("X");
    return ret;
}

/*for expand*/
static cmr_int single_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                    void *param) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    OTP_LOGD("E");

    /*you can add you command*/
    switch (cmd) {
    case CMD_SNS_OTP_DATA_COMPATIBLE_CONVERT:
        single_otp_compatible_convert(otp_drv_handle, param);
        break;
    default:
        break;
    }
    OTP_LOGD("X");
    return ret;
}
