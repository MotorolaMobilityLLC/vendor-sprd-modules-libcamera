/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "vcm_dw9800.h"

uint32_t dw9800_init(SENSOR_HW_HANDLE handle) {
    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;

    slave_addr = DW9800_VCM_SLAVE_ADDR;
    SENSOR_LOGI("E");
    cmd_len = 2;
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
    usleep(1000);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x80;
    Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);
    usleep(1000);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x75;
    Sensor_WriteI2C(slave_addr, (uint8_t *)&cmd_val[0], cmd_len);

    return ret_value;
}

uint32_t dw9800_set_motor_pos(SENSOR_HW_HANDLE handle, uint16_t pos) {
    uint8_t cmd_val[2] = {0};

    // set pos
    cmd_val[0] = 0x03;
    cmd_val[1] = (pos >> 8) & 0x03;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);

    cmd_val[0] = 0x04;
    cmd_val[1] = pos & 0xff;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    SENSOR_LOGI("set_position:0x%x", pos);
    return 0;
}

uint32_t dw900_get_otp(SENSOR_HW_HANDLE handle, uint16_t *inf,
                       uint16_t *macro) {
    uint8_t bTransfer[2] = {0, 0};

    // get otp
    bTransfer[0] = 0x00;
    bTransfer[1] = 0x00;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
    usleep(1000);
    // Macro
    bTransfer[0] = 0x01;
    bTransfer[1] = 0x0c;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
    *macro = bTransfer[0];
    bTransfer[0] = 0x01;
    bTransfer[1] = 0x0d;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
    *macro += bTransfer[0] << 8;
    // Infi
    bTransfer[0] = 0x01;
    bTransfer[1] = 0x10;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
    *inf = bTransfer[0];
    bTransfer[0] = 0x01;
    bTransfer[1] = 0x11;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, &bTransfer[0], 2);
    *inf += bTransfer[0] << 8;
    return 0;
}

uint32_t dw9800_get_motor_pos(SENSOR_HW_HANDLE handle, uint16_t *pos) {
    uint8_t cmd_val[2];
    // read
    SENSOR_LOGI("handle:0x%x", handle);
    cmd_val[0] = 0x03;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    *pos = (cmd_val[0] & 0x03) << 8;
    usleep(200);
    cmd_val[0] = 0x04;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    *pos += cmd_val[0];
    SENSOR_LOGI("get_position:0x%x", *pos);
    return 0;
}

uint32_t dw9800_set_motor_bestmode(SENSOR_HW_HANDLE handle) {

    uint8_t ctrl, mode, freq;
    uint8_t pos1, pos2;
    uint8_t cmd_val[2];

    // set
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x80;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x75;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    usleep(200 * 1000);

    CMR_LOGI("VCM ctrl mode freq pos 2nd,%d %d %d %d", ctrl, mode, freq,
             (pos1 << 8) + pos2);
    return 0;
}

uint32_t dw9800_get_test_vcm_mode(SENSOR_HW_HANDLE handle) {

    uint8_t ctrl, mode, freq;
    uint8_t pos1, pos2;
    uint8_t cmd_val[2];

    FILE *fp = NULL;
    fp = fopen("/data/misc/cameraserver/cur_vcm_info.txt", "wb");
    // read
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    ctrl = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x06;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    mode = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x07;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    freq = cmd_val[0];

    // read
    cmd_val[0] = 0x03;
    cmd_val[1] = 0x03;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    pos1 = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x04;
    cmd_val[1] = 0x04;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    pos2 = cmd_val[0];

    fprintf(fp, "VCM ctrl mode freq pos ,%d %d %d %d", ctrl, mode, freq,
            (pos1 << 8) + pos2);
    fclose(fp);
    return 0;
}

uint32_t dw9800_set_test_vcm_mode(SENSOR_HW_HANDLE handle, char *vcm_mode) {

    uint8_t ctrl, mode, freq;
    uint8_t pos1, pos2;
    uint8_t cmd_val[2];
    char *p1 = vcm_mode;

    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    ctrl = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    mode = atoi(vcm_mode);
    vcm_mode = p1;
    while (*p1 != '~' && *p1 != '\0')
        p1++;
    *p1++ = '\0';
    freq = atoi(vcm_mode);
    CMR_LOGI("VCM ctrl mode freq pos 1nd,%d %d %d", ctrl, mode, freq);
    // set
    cmd_val[0] = 0x02;
    cmd_val[1] = ctrl;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = mode;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = freq;
    Sensor_WriteI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 2);
    usleep(200 * 1000);
    // read
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    ctrl = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x06;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    mode = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x07;
    Sensor_ReadI2C(DW9800_VCM_SLAVE_ADDR, (uint8_t *)&cmd_val[0], 1);
    freq = cmd_val[0];
    CMR_LOGI("VCM ctrl mode freq pos 2nd,%d %d %d", ctrl, mode, freq);
    return 0;
}

af_drv_info_t dw9800_drv_info = {
    .af_work_mode = 0,
    .af_ops =
        {
            .set_motor_pos = dw9800_set_motor_pos,
            .get_motor_pos = dw9800_get_motor_pos,
            .set_motor_bestmode = dw9800_set_motor_bestmode,
            .motor_deinit = NULL,
            .set_test_motor_mode = dw9800_set_test_vcm_mode,
            .get_test_motor_mode = dw9800_get_test_vcm_mode,
        },
};
