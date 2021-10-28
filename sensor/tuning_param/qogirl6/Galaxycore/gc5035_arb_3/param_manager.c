#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
    #include "parameters/sensor_gc5035_arb_3_raw_param_main.c"
    return &s_gc5035_arb_3_mipi_raw_info;
}

