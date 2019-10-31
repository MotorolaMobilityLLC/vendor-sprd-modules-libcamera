#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_gc2375h_raw_param_main.c"
    return &s_gc2375h_mipi_raw_info;
}
