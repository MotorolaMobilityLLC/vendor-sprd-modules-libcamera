#define HI846_OTP_BLOCK_FLAG_OFFSET 0x0201
static cmr_int hi846_sensor_otp_init(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    OTP_LOGD("init sensor otp start");

    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0a02, 0x01, BITS_ADDR16_REG8); // fast sleep on
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0a00, 0x00, BITS_ADDR16_REG8); // stand by on
    usleep(10 * 1000);
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0f02, 0x00, BITS_ADDR16_REG8); // pll disable
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x071a, 0x01, BITS_ADDR16_REG8); // CP TRIM_H;
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x071b, 0x09, BITS_ADDR16_REG8); // IPGM TRIM_H
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0d04, 0x01,
                            BITS_ADDR16_REG8); // Fsync Output enable
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0d00, 0x07,
                            BITS_ADDR16_REG8); // Fsync Output Drivability
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x003e, 0x10, BITS_ADDR16_REG8); // OTP R/W mode
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070f, 0x05, BITS_ADDR16_REG8); // OTP data rewrite
    usleep(10 * 1000);
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0a00, 0x01, BITS_ADDR16_REG8); // stand by off

    OTP_LOGV("init sensor otp finished");
    return ret;
}

static cmr_int hi846_sensor_otp_deinit(void *otp_drv_handle) {
    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0a00, 0x00, BITS_ADDR16_REG8); // stand by on
    usleep(10 * 1000);
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x003e, 0x00, BITS_ADDR16_REG8); // display mode
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0a00, 0x01, BITS_ADDR16_REG8); // stand by off

    OTP_LOGD("deinit sensor otp");
    return ret;
}

static cmr_u8 hi846_sensor_otp_read_8bits(cmr_handle otp_drv_handle,
                                          cmr_u16 offset) {

    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u16 val = 0;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070a, (offset >> 8) & 0xff,
                            BITS_ADDR16_REG8); // address H
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070b, offset & 0xff,
                            BITS_ADDR16_REG8); // address L
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0702, 0x01, BITS_ADDR16_REG8); // single read
    val = hw_sensor_grc_read_i2c(otp_cxt->hw_handle,
                                 otp_cxt->eeprom_i2c_addr >> 1, 0x0708,
                                 BITS_ADDR16_REG8); // OTP data read

    // OTP_LOGD("sensor otp[0x%x] = 0x%x", offset, val);
    return val & 0xff;
}

static cmr_int hi846_sensor_otp_read_continuous(cmr_handle otp_drv_handle,
                                                cmr_u8 *buffer,
                                                cmr_u16 start_offset,
                                                cmr_u32 size) {

    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u32 i = 0;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070a, (start_offset >> 8) & 0xff,
                            BITS_ADDR16_REG8); // address H
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070b, start_offset & 0xff,
                            BITS_ADDR16_REG8); // address L
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0702, 0x01, BITS_ADDR16_REG8); // single read

    for (i = 0; i < size; i++) {
        buffer[i] = hw_sensor_grc_read_i2c(
            otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1, 0x0708,
            BITS_ADDR16_REG8); // OTP data read
        // OTP_LOGD("buffer[0x%x] = 0x%x", i, buffer[i]);
    }

    return ret;
}

static cmr_int hi846_sensor_otp_read_burst(cmr_handle otp_drv_handle,
                                           cmr_u8 *buffer, cmr_u16 start_offset,
                                           cmr_u32 size) {

    cmr_int ret = OTP_CAMERA_SUCCESS;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;

    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070a, (start_offset >> 8) & 0xff,
                            BITS_ADDR16_REG8); // address H
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x070b, start_offset & 0xff,
                            BITS_ADDR16_REG8); // address L
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0702, 0x01, BITS_ADDR16_REG8); // single read

    // Burst read register on
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0f12, 0x01, BITS_ADDR16_REG8);

    // OTP burst read using I2C Burst Read
    buffer[0] = (0x0708 >> 8) & 0xff;
    buffer[1] = 0x0708 & 0xff;
    ret = hw_sensor_read_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                             buffer, size << 16 | SENSOR_I2C_REG_16BIT);

    // Burst read register off
    hw_sensor_grc_write_i2c(otp_cxt->hw_handle, otp_cxt->eeprom_i2c_addr >> 1,
                            0x0f12, 0x00, BITS_ADDR16_REG8);

    return ret;
}
static cmr_int hi846_sensor_otp_read(cmr_handle otp_drv_handle) {

    cmr_int ret = OTP_CAMERA_SUCCESS;
    cmr_u8 flag = 0;
    cmr_u16 block_offset = 0x0203;
    CHECK_PTR(otp_drv_handle);
    otp_drv_cxt_t *otp_cxt = (otp_drv_cxt_t *)otp_drv_handle;
    cmr_u32 size = otp_cxt->eeprom_size;

    OTP_LOGD("sensor otp read burst");
    /*init read sensor otp data*/
    hi846_sensor_otp_init(otp_drv_handle);

    /*read sensor otp data*/
    // hi846_sensor_otp_read_burst(otp_drv_handle, otp_cxt->otp_raw_data.buffer, block_offset, size);
    // hi846_sensor_otp_read_continuous(otp_drv_handle, otp_cxt->otp_raw_data.buffer, block_offset, size);

    flag = hi846_sensor_otp_read_8bits(otp_drv_handle,
                                       HI846_OTP_BLOCK_FLAG_OFFSET);

    if (flag == 0x01) {
        OTP_LOGD("group1, size %d, block flag 0x01", size);
        block_offset = HI846_OTP_BLOCK_FLAG_OFFSET + 2;
        hi846_sensor_otp_read_burst(
            otp_drv_handle, otp_cxt->otp_raw_data.buffer, block_offset, size);
    } else if (flag == 0x13) {
        OTP_LOGD("group2, size %d, block flag 0x13", size);
        block_offset = HI846_OTP_BLOCK_FLAG_OFFSET + 2 + size;
        hi846_sensor_otp_read_burst(
            otp_drv_handle, otp_cxt->otp_raw_data.buffer, block_offset, size);
    } else if (flag == 0x37) {
        OTP_LOGD("group3, size %d, block flag 0x37", size);
        block_offset = HI846_OTP_BLOCK_FLAG_OFFSET + 2 + size * 2;
        hi846_sensor_otp_read_burst(
            otp_drv_handle, otp_cxt->otp_raw_data.buffer, block_offset, size);
    } else {
        ret = OTP_CAMERA_FAIL;
        OTP_LOGE("invalid block flag 0x%x", flag);
    }

    /*deinit read sensor otp data*/
    hi846_sensor_otp_deinit(otp_drv_handle);

    return ret;
}
