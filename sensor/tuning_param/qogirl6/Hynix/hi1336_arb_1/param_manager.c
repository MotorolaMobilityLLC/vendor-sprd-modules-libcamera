#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
    #include "parameters/sensor_hi1336_arb_1_raw_param_main.c"
    return &s_hi1336_arb_1_mipi_raw_info;
}

