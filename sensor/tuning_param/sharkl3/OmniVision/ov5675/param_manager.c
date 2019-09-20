#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_ov5675_raw_param_main.c"
    return &s_ov5675_mipi_raw_info;
}
