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

#define LOG_TAG "vcm_gt9772"
#include "vcm_gt9772.h"

static uint32_t _gt9772_set_motor_bestmode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2];

    // set
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x80;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x75;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200 * 1000);

    return 0;
}

static uint32_t _gt9772_get_test_vcm_mode(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t ctrl, mode, freq;
    uint8_t pos1, pos2;
    uint8_t cmd_val[2];

    FILE *fp = NULL;
    fp = fopen("/data/vendor/cameraserver/cur_vcm_info.txt", "wb");
    // read
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    ctrl = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x06;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    mode = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x07;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    freq = cmd_val[0];

    // read
    cmd_val[0] = 0x03;
    cmd_val[1] = 0x03;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    pos1 = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x04;
    cmd_val[1] = 0x04;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    pos2 = cmd_val[0];

    fprintf(fp, "VCM ctrl mode freq pos ,%d %d %d %d", ctrl, mode, freq,
            (pos1 << 8) + pos2);
    fclose(fp);
    return 0;
}

static uint32_t _gt9772_set_test_vcm_mode(cmr_handle sns_af_drv_handle,
                                          char *vcm_mode) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

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
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = mode;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = freq;
    hw_Sensor_WriteI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                       (uint8_t *)&cmd_val[0], 2);
    usleep(200 * 1000);
    // read
    cmd_val[0] = 0x02;
    cmd_val[1] = 0x02;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    ctrl = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x06;
    cmd_val[1] = 0x06;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    mode = cmd_val[0];
    usleep(200);
    cmd_val[0] = 0x07;
    cmd_val[1] = 0x07;
    hw_Sensor_ReadI2C(af_drv_cxt->hw_handle, GT9772_VCM_SLAVE_ADDR,
                      (uint8_t *)&cmd_val[0], 1);
    freq = cmd_val[0];
    CMR_LOGI("VCM ctrl mode freq pos 2nd,%d %d %d", ctrl, mode, freq);
    return 0;
}

static int gt9772_drv_create(struct af_drv_init_para *input_ptr,
                             cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        _gt9772_drv_power_on(*sns_af_drv_handle, AF_TRUE);
        ret = _gt9772_drv_init(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }
    CMR_LOGV("af_drv_handle:%p", *sns_af_drv_handle);
    return ret;
}

static int gt9772_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    _gt9772_drv_power_on(sns_af_drv_handle, AF_FALSE);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}

static int gt9772_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {

    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    // set pos
    uint16_t slave_addr = 0;
    slave_addr = GT9772_VCM_SLAVE_ADDR;
    CMR_LOGV("E");

    hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                       0x03, ((pos >> 8) & 0xff), BITS_ADDR8_REG8);
    hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                       0x04, (pos & 0xff), BITS_ADDR8_REG8);
    usleep(5000);
    return AF_SUCCESS;
}

static int gt9772_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                            void *param) {
    uint32_t ret_value = AF_SUCCESS;

    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    switch (cmd) {
    case CMD_SNS_AF_SET_BEST_MODE:
        //_gt9772_set_motor_bestmode(sns_af_drv_handle);
        break;
    case CMD_SNS_AF_GET_TEST_MODE:
        //_gt9772_get_test_vcm_mode(sns_af_drv_handle);
        break;
    case CMD_SNS_AF_SET_TEST_MODE:
        //_gt9772_set_test_vcm_mode(sns_af_drv_handle, param);
        break;
    default:
        break;
    }
    return ret_value;
}

struct sns_af_drv_entry gt9772_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 0,
    .af_ops =
        {
            .create = gt9772_drv_create,
            .delete = gt9772_drv_delete,
            .set_pos = gt9772_drv_set_pos,
            .get_pos = NULL,
            .ioctl = gt9772_drv_ioctl,
        },
};

static int _gt9772_drv_power_on(cmr_handle sns_af_drv_handle,
                                uint16_t power_on) {
    CHECK_PTR(sns_af_drv_handle);
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;

    if (AF_TRUE == power_on) {
        hw_sensor_set_monitor_val(af_drv_cxt->hw_handle,
                                  gt9772_drv_entry.motor_avdd_val);
        usleep(GT9772_POWERON_DELAY * 1000);
    } else {
        hw_sensor_set_monitor_val(af_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }

    SENSOR_PRINT("(1:on, 0:off): %d", power_on);
    return AF_SUCCESS;
}

static int _gt9772_drv_init(cmr_handle sns_af_drv_handle) {
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;

    slave_addr = GT9772_VCM_SLAVE_ADDR;
    CMR_LOGV("E");

    hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                       0xED, 0xAB, BITS_ADDR8_REG8);
    usleep(5 * 1000);
    hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                       0x06, 0x84, BITS_ADDR8_REG8);
    hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                       0x07, 0x01, BITS_ADDR8_REG8);
    hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                       0x08, 0x49, BITS_ADDR8_REG8);
    usleep(1000);
    return ret_value;
}

void *vcm_driver_open_lib(void)
{
     return &gt9772_drv_entry;
}
