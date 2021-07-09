#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_ov08d10_syp_raw_param_main.c"
    return &s_ov08d10_syp_mipi_raw_info;
}
