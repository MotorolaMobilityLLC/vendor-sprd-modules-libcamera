#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_ov02b10_syp_raw_param_main.c"
    return &s_ov02b10_syp_mipi_raw_info;
}
