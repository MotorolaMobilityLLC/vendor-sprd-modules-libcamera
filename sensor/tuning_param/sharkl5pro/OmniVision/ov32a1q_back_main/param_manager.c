#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
	#include "parameters/sensor_ov32a1q_raw_param_main.c"
    return &s_ov32a1q_mipi_raw_info;
}
