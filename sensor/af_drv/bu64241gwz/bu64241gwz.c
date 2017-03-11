#include "bu64241gwz.h"

uint32_t BU64241GWZ_set_pos(SENSOR_HW_HANDLE handle, uint16_t pos) {

    uint8_t cmd_val[2] = {((pos >> 8) & 0x03) | 0xc4, pos & 0xff};

    // set pos
    CMR_LOGE("BU64241GWZ_set_pos %d", pos);
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    return 0;
}

uint32_t BU64241GWZ_get_otp(SENSOR_HW_HANDLE handle, uint16_t *inf,
                            uint16_t *macro) {

    // get otp
    // spec not specifi otp,don't know how to get inf and macro
    return 0;
}

uint32_t BU64241GWZ_get_motor_pos(SENSOR_HW_HANDLE handle, uint16_t *pos) {

    uint8_t cmd_val[2] = {0xc4, 0x00};

    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    *pos = (cmd_val[0] & 0x03) << 8;
    *pos += cmd_val[1];
    CMR_LOGI("vcm pos %d", *pos);
    return 0;
}

uint32_t BU64241GWZ_set_motor_bestmode(SENSOR_HW_HANDLE handle) {

    uint16_t A_code = 80, B_code = 90;
    uint8_t rf = 0x0F, slew_rate = 0x02, stt = 0x01, str = 0x02;
    uint8_t cmd_val[2];

    // set
    cmd_val[0] = 0xcc;
    cmd_val[1] = (rf << 3) | slew_rate;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xd4 | (A_code >> 8);
    cmd_val[1] = A_code & 0xff;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xdc | (B_code >> 8);
    cmd_val[1] = B_code & 0xff;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xe4;
    cmd_val[1] = (str << 5) | stt;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);

    CMR_LOGI("VCM mode set");
    return 0;
}

uint32_t BU64241GWZ_get_test_vcm_mode(SENSOR_HW_HANDLE handle) {

    uint16_t A_code = 80, B_code = 90, C_code = 0;
    uint8_t rf = 0x0F, slew_rate = 0x02, stt = 0x01, str = 0x02;
    uint8_t cmd_val[2];

    FILE *fp = NULL;
    fp = fopen("/data/misc/media/cur_vcm_info.txt", "wb");

    // read
    cmd_val[0] = 0xcc;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    rf = cmd_val[1] >> 3;
    slew_rate = cmd_val[1] & 0x03;
    usleep(200);
    cmd_val[0] = 0xd4;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    A_code = (cmd_val[0] & 0x03) << 8;
    A_code = A_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xdc;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    B_code = (cmd_val[0] & 0x03) << 8;
    B_code = B_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xc4;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    C_code = (cmd_val[0] & 0x03) << 8;
    C_code = C_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xe4;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    str = cmd_val[1] >> 5;
    stt = cmd_val[1] & 0x1f;
    CMR_LOGI("VCM A_code B_code rf slew_rate stt str pos,%d %d %d %d %d %d %d",
             A_code, B_code, rf, slew_rate, stt, str, C_code);

    fprintf(fp,
            "VCM A_code B_code rf slew_rate stt str pos,%d %d %d %d %d %d %d",
            A_code, B_code, rf, slew_rate, stt, str, C_code);
    fflush(fp);
    fclose(fp);
    return 0;
}

uint32_t BU64241GWZ_set_test_vcm_mode(SENSOR_HW_HANDLE handle, char *vcm_mode) {

    uint16_t A_code = 80, B_code = 90;
    uint8_t rf = 0x0F, slew_rate = 0x02, stt = 0x01, str = 0x02;
    uint8_t cmd_val[2];
    char *p1 = vcm_mode;

    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    A_code = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    B_code = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    rf = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    slew_rate = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    stt = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    str = atoi(vcm_mode);
    CMR_LOGI("VCM A_code B_code rf slew_rate stt str 1nd,%d %d %d %d %d %d",
             A_code, B_code, rf, slew_rate, stt, str);
    // set
    cmd_val[0] = 0xcc;
    cmd_val[1] = (rf << 3) | slew_rate;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xd4 | (A_code >> 8);
    cmd_val[1] = A_code & 0xff;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xdc | (B_code >> 8);
    cmd_val[1] = B_code & 0xff;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0xe4;
    cmd_val[1] = (str << 5) | stt;
    Sensor_WriteI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    // read
    cmd_val[0] = 0xcc;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    rf = cmd_val[1] >> 3;
    slew_rate = cmd_val[1] & 0x03;
    usleep(200);
    cmd_val[0] = 0xd4;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    A_code = (cmd_val[0] & 0x03) << 8;
    A_code = A_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xdc;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    B_code = (cmd_val[0] & 0x03) << 8;
    B_code = B_code + cmd_val[1];
    usleep(200);
    cmd_val[0] = 0xe4;
    Sensor_ReadI2C((0x18 >> 1), (uint8_t *)&cmd_val[0], 2);
    str = cmd_val[1] >> 5;
    stt = cmd_val[1] & 0x1f;
    CMR_LOGI("VCM A_code B_code rf slew_rate stt str 2nd,%d %d %d %d %d %d",
             A_code, B_code, rf, slew_rate, stt, str);
    return 0;
}

af_drv_info_t bu64241gwz_drv_info = {
    .af_work_mode = 0,
    .af_ops =
        {
            .set_motor_pos = BU64241GWZ_set_pos,
            .get_motor_pos = BU64241GWZ_get_motor_pos,
            .set_motor_bestmode = BU64241GWZ_set_motor_bestmode,
            .motor_deinit = NULL,
            .set_test_motor_mode = BU64241GWZ_set_test_vcm_mode,
            .get_test_motor_mode = BU64241GWZ_get_test_vcm_mode,
        },
};
