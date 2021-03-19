#ifndef _TCS_3430_DRV_H_
#define _TCS_3430_DRV_H_

#include <stdio.h>
#include <string.h>
#include "cmr_types.h"
#include "cmr_log.h"

static double MH[3][5] = {
    2.33897,    0.09788535, -0.1558221, -0.1214886, 0.000000,
    -0.3191509, 2.92959058, -0.280898,  -0.062711,  0.000000,
    0.03999683, 0.69583218, 6.54702145, -0.3912507, 0.000000,
};
static double ML[3][5] = {
    1.92101354, 0.23747312, -0.026573,  2.64961428, 0.000000,
    -0.6363603, 3.1938705,  -0.4210471, 1.69042099, 0.000000,
    -0.2610055, -0.1044673, 9.50239707, 1.29158642, 0.000000,
};

struct tcs_data {
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

struct tcs_calib_data {
    cmr_u16 x_raw_golden;
    cmr_u16 y_raw_golden;
    cmr_u16 z_raw_golden;
    cmr_u16 ir_raw_golden;
    cmr_u16 x_raw_unit;
    cmr_u16 y_raw_unit;
    cmr_u16 z_raw_unit;
    cmr_u16 ir_raw_unit;
};

int tcs3430_read_data(void *param);

#endif
