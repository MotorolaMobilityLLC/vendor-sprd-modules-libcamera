#include "sensor_raw.h"

struct sensor_raw_info *ov2680_drv_init_raw_info(int sensor_id, int vendor_id,
                                                 int flash_ic_type,
                                                 int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;
#if defined _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_ov2680_raw_param_main.c"
        return &s_ov2680_mipi_raw_info;
#elif defined(_SENSOR_RAW_SHARKL5PRO_H_)
#include "parameters_sharkl5pro/sensor_ov2680_raw_param_main.c"
    return &s_ov2680_mipi_raw_info;
#else
#include "parameters_sharkl3/sensor_ov2680_raw_param_main.c"
    return &s_ov2680_mipi_raw_info;
#endif
}
