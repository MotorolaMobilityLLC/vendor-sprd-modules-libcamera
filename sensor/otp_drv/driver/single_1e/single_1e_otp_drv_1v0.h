/*
 *  otp 1.0 dynamic address version
 *  otp map files: \\shplm06\01-SRPD Camera OTP 标准\OTP map\v1.0
 *  single camera one eeprom - single camera otp driver
 *  */

#define LOG_TAG "single_1e_otp_1v0"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <utils/Log.h>
#include "sensor_drv_u.h"
#include "otp_common.h"
#include "cmr_sensor_info.h"

#define EEPROM_I2C_ADDR 0xA2 >> 1

static cmr_int _single_otp_section_checksum(cmr_u8 *buffer, cmr_uint start_addr,
                                            cmr_uint size,
                                            cmr_uint checksum_addr);
static cmr_int _single_otp_parse_af_data(cmr_handle otp_drv_handle);
static cmr_int _single_otp_parse_awb_data(cmr_handle otp_drv_handle);
static cmr_int _single_otp_parse_lsc_data(cmr_handle otp_drv_handle);
static cmr_int _single_otp_parse_pdaf_data(cmr_handle otp_drv_handle);
static cmr_int _single_otp_parse_ae_data(cmr_handle otp_drv_handle);
static cmr_int _single_otp_parse_dualcam_data(cmr_handle otp_drv_handle);

static cmr_int single_otp_drv_create(otp_drv_init_para_t *input_para,
                                     cmr_handle *otp_drv_handle);
static cmr_int single_otp_drv_delete(cmr_handle otp_drv_handle);
static cmr_int single_otp_drv_read(cmr_handle otp_drv_handle, void *param);
static cmr_int single_otp_drv_write(cmr_handle otp_drv_handle, void *param);
static cmr_int single_otp_drv_parse(cmr_handle otp_drv_handle, void *param);
static cmr_int single_otp_drv_calibration(cmr_handle otp_drv_handle);
static cmr_int single_otp_compatible_convert(cmr_handle otp_drv_handle,
                                             void *p_data);
static cmr_int single_otp_drv_ioctl(cmr_handle otp_drv_handle, cmr_uint cmd,
                                    void *param);

otp_drv_entry_t single_1e_otp_entry = {
    .otp_ops =
        {
            .sensor_otp_create = single_otp_drv_create,
            .sensor_otp_delete = single_otp_drv_delete,
            .sensor_otp_read = single_otp_drv_read,
            .sensor_otp_write = single_otp_drv_write,
            .sensor_otp_parse = single_otp_drv_parse,
            .sensor_otp_calibration = single_otp_drv_calibration,
            .sensor_otp_ioctl = single_otp_drv_ioctl, /*expand*/
        },
};
