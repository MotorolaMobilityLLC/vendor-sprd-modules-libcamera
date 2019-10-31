#include "sensor_raw.h"

void *tuning_param_get_ptr(void) {
    cmr_int rtn = 0;
#if defined _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_ov2680_raw_param_main.c"
    return &s_ov2680_mipi_raw_info;
#elif defined(_SENSOR_RAW_SHARKL5PRO_H_)
#include "parameters_sharkl5pro/sensor_ov2680_raw_param_main.c"
    return &s_ov2680_mipi_raw_info;
#else
#include "parameters_sharkl5pro/sensor_ov2680_raw_param_main.c"
    return &s_ov2680_mipi_raw_info;
#endif

}
