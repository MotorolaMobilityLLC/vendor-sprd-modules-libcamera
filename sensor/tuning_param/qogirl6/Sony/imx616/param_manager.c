#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
    #include "parameters/sensor_imx616_raw_param_main.c"
    return &s_imx616_mipi_raw_info;
}

