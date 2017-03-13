#ifndef _OTP_COMMON_H_
#define _OTP_COMMON_H_

#include "cmr_common.h"
#include "sensor_drv_u.h"
#include <cutils/properties.h>
#include "otp_info.h"

int sensor_otp_rw_data_from_file(uint8_t cmd, char *sensor_name,
                                 otp_format_data_t **otp_data,
                                 int *format_otp_size);
int sensor_otp_lsc_decompress(otp_base_info_cfg_t *otp_base_info,
                              lsccalib_data_t *lsc_cal_data);
int sensor_otp_decompress_gain(uint16_t *src, uint32_t src_bytes,
                               uint32_t src_uncompensate_bytes, uint16_t *dst,
                               uint32_t GAIN_COMPRESSED_BITS,
                               uint32_t GAIN_MASK);
void sensor_otp_change_pattern(uint32_t pattern, uint16_t *interlaced_gain,
                               uint16_t *chn_gain[4], uint16_t gain_num);
int sensor_otp_dump_raw_data(uint8_t *buffer, int size, char *dev_name);
int sensor_otp_drv_create(otp_drv_init_para_t *input_para,
                              cmr_handle* sns_af_drv_handle);
int sensor_otp_drv_delete(void *otp_drv_handle);

#endif
