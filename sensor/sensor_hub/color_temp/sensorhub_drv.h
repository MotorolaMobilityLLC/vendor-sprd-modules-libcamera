#ifndef _SENSOR_HUB_DRV_H_
#define _SENSOR_HUB_DRV_H_

#include <stdio.h>
#include <string.h>
#include "cmr_types.h"
#include "cmr_log.h"

struct sensorhub_tcs_data {
    cmr_u32 x_data;
    cmr_u32 y_data;
    cmr_u32 z_data;
    cmr_u32 ir_data;
    cmr_u32 x_raw;
    cmr_u32 y_raw;
    cmr_u32 z_raw;
    cmr_u32 ir_raw;
    cmr_u32 gain_data;
    cmr_u32 atime_data;
    cmr_u32 lux_data;
    cmr_u32 cct_data;
};

void (*color_temp_callback_func)(cmr_u32* parm);

int sensorhub_read_data(void *param);

#endif
