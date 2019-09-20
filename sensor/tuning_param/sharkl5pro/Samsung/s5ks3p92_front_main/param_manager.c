#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_s5ks3p92_raw_param_main.c"
    return &s_s5ks3p92_mipi_raw_info;
}
