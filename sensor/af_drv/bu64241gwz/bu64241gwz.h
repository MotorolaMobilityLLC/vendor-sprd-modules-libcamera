#ifndef _BU64241_H_
#define _BU64241_H_
#include "af_drv.h"

#define BU64241GWZ_VCM_SLAVE_ADDR (0x18 >> 1)

uint32_t  BU64241GWZ_set_pos(SENSOR_HW_HANDLE handle, uint16_t pos);
uint32_t  BU64241GWZ_get_otp(SENSOR_HW_HANDLE handle, uint16_t *inf, uint16_t *macro);
uint32_t  BU64241GWZ_get_motor_pos(SENSOR_HW_HANDLE handle, uint16_t *pos);
uint32_t  BU64241GWZ_set_motor_bestmode(SENSOR_HW_HANDLE handle);
uint32_t  BU64241GWZ_get_test_vcm_mode(SENSOR_HW_HANDLE handle);
uint32_t  BU64241GWZ_set_test_vcm_mode(SENSOR_HW_HANDLE handle, char* mode);

#endif
