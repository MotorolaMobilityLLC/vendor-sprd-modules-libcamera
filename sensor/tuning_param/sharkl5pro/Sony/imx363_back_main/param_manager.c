#include "sensor_raw.h"

*tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
    #include "parameters/sensor_imx363_raw_param_main.c"
    return &s_imx363_mipi_raw_info;
}
