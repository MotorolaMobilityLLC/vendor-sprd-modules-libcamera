/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ver: 1.0
 */
#define LOG_TAG "af_dw9781b"
#include "af_dw9781b.h"


/*==============================================================================
 * Description:
 * write code to vcm driver
 * you can change this function acording your spec if it's necessary
 * code: Dac code for vcm driver
 *============================================================================*/
static uint32_t _dw9781b_write_dac_code(cmr_handle sns_af_drv_handle,
                                        int32_t param) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    uint8_t cmd_val[2] = {0x00};
    uint16_t slave_addr = DW9781B_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint16_t pos = 0;

    int32_t target_code = (param * 2) & 0x7FF;
    CMR_LOGD("param:%d", target_code);

    ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, slave_addr,
                                            0xD013, target_code, BITS_ADDR16_REG16);
    if(ret_value)
	CMR_LOGD("Failed to write dac");

    return 0;
}

static int dw9781b_drv_create(struct af_drv_init_para *input_ptr,
                              cmr_handle *sns_af_drv_handle) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(input_ptr);
    ret = af_drv_create(input_ptr, sns_af_drv_handle);
    if (ret != AF_SUCCESS) {
        ret = AF_FAIL;
    } else {
        _dw9781b_drv_power_on(*sns_af_drv_handle, AF_TRUE);
        ret = _dw9781b_drv_set_mode(*sns_af_drv_handle);
        if (ret != AF_SUCCESS)
            ret = AF_FAIL;
    }

    return ret;
}
static int dw9781b_drv_delete(cmr_handle sns_af_drv_handle, void *param) {
    cmr_int ret = AF_SUCCESS;
    CHECK_PTR(sns_af_drv_handle);
    _dw9781b_drv_power_on(sns_af_drv_handle, AF_FALSE);
    ret = af_drv_delete(sns_af_drv_handle, param);
    return ret;
}
/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * Param: ISP write dac code
 *============================================================================*/
static int dw9781b_drv_set_pos(cmr_handle sns_af_drv_handle, uint16_t pos) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);

    int32_t target_code = pos & 0x7FF;
    int32_t m_cur_dac_code = af_drv_cxt->current_pos;
    CMR_LOGI("target_code %d", target_code);
    m_cur_dac_code = target_code;
    _dw9781b_write_dac_code(sns_af_drv_handle, m_cur_dac_code);
    af_drv_cxt->current_pos = m_cur_dac_code;

    return ret_value;
}

static int dw9781b_drv_ioctl(cmr_handle sns_af_drv_handle, enum sns_cmd cmd,
                             void *param) {
    uint32_t ret_value = AF_SUCCESS;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    switch (cmd) {
    case CMD_SNS_AF_SET_BEST_MODE:
        break;
    case CMD_SNS_AF_GET_TEST_MODE:
        break;
    case CMD_SNS_AF_SET_TEST_MODE:
        break;
    default:
        break;
    }

    return ret_value;
}

struct sns_af_drv_entry dw9781b_drv_entry = {
    .motor_avdd_val = SENSOR_AVDD_2800MV,
    .default_work_mode = 2,
    .af_ops =
        {
            .create = dw9781b_drv_create,
            .delete = dw9781b_drv_delete,
            .set_pos = dw9781b_drv_set_pos,
            .get_pos = NULL,
            .ioctl = dw9781b_drv_ioctl,
        },
};

static int _dw9781b_drv_power_on(cmr_handle sns_af_drv_handle,
                                 uint16_t power_on) {
    CHECK_PTR(sns_af_drv_handle);
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;

    if (AF_TRUE == power_on) {
        hw_sensor_set_monitor_val(af_drv_cxt->hw_handle,
                                  dw9781b_drv_entry.motor_avdd_val);       
        usleep(DW9781B_POWERON_DELAY * 1000);
    } else {
        hw_sensor_set_monitor_val(af_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
    }

    SENSOR_PRINT("vcm:(1:on, 0:off): %d", power_on);
    return AF_SUCCESS;
}

static int _dw9781b_drv_set_mode(cmr_handle sns_af_drv_handle) {
    uint32_t ret_value = AF_SUCCESS;
    cmr_u16 pid_value = 0x0000;
    struct sns_af_drv_cxt *af_drv_cxt =
        (struct sns_af_drv_cxt *)sns_af_drv_handle;
    CHECK_PTR(sns_af_drv_handle);
    char value[128];
    uint16_t cmd_val[2] = {0x0000};
    uint16_t slave_addr = DW9781B_VCM_SLAVE_ADDR;
    uint16_t cmd_len = 0;
    uint8_t mode = 0;

    

    if (af_drv_cxt->af_work_mode) {
        mode = af_drv_cxt->af_work_mode;
    } else {
        mode = dw9781b_drv_entry.default_work_mode;
    }
    CMR_LOGI("mode: %d", mode);
    switch (mode) {
    case 1:
        break;
    case 2:
        /*Power Down */
        #if 0
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xD000, 0x0001, BITS_ADDR16_REG16);
        usleep(10 * 1000);
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xD002, 0x0001, BITS_ADDR16_REG16);
        usleep(4 * 1000);
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xD001, 0x0001, BITS_ADDR16_REG16);
        usleep(25 * 1000);
	
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xEBF1, 0x56FA, BITS_ADDR16_REG16);
        if (ret_value) {
            CMR_LOGE("write failed");
        }

        #endif
        #if 1
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xD000, 0x0001, BITS_ADDR16_REG16);
        usleep(10 * 1000);
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xD002, 0x0001, BITS_ADDR16_REG16);
        usleep(4 * 1000);
        ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xD001, 0x0001, BITS_ADDR16_REG16);
        usleep(25 * 1000);
	property_get("persist.vendor.cam.test.ois", value, "0");
	if(atoi(value)) {
            ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0xEBF1, 0x56FA, BITS_ADDR16_REG16);
            ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0x7015, 0x0002, BITS_ADDR16_REG16); 
            usleep(10 * 1000);                                    
            ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0x7011, 0x00AA, BITS_ADDR16_REG16);   
            usleep(20 * 1000);   
            ret_value = hw_sensor_grc_write_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR,
                                            0x7010, 0x8000, BITS_ADDR16_REG16);
	}

        usleep(200 * 1000);  
        pid_value = hw_sensor_grc_read_i2c(af_drv_cxt->hw_handle, DW9781B_VCM_SLAVE_ADDR, 0x7000, BITS_ADDR16_REG16);
        CMR_LOGI("pid_value = 0x%x", pid_value);
      #endif                                                                                                                                    
    }

    return 0;
}

void *vcm_driver_open_lib(void)
{
    return &dw9781b_drv_entry;
}
