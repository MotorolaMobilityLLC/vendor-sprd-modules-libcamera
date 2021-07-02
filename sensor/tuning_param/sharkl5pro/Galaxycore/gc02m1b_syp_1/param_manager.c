#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_gc02m1b_syp_1_raw_param_main.c"
    return &s_gc02m1b_syp_1_mipi_raw_info;
}
