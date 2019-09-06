#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_imx586_raw_param_main.c"
    return &s_imx586_mipi_raw_info;
}
