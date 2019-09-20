#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
    #include "parameters/sensor_s5k5e8yx_raw_param_main.c"
    return &s_s5k5e8yx_mipi_raw_info;
}
