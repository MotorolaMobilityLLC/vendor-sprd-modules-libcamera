#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
#include "parameters/sensor_s5kgm1sp_syp_2_raw_param_main.c"
    return &s_s5kgm1sp_syp_2_mipi_raw_info;
}
