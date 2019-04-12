#include "sensor_raw.h"

struct sensor_raw_info *ov7251_drv_init_raw_info(int sensor_id, int vendor_id,
                                                 int flash_ic_type,
                                                 int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;

#ifdef _SENSOR_RAW_SHARKL5_H_
#include "parameters_sharkl5/sensor_ov7251_raw_param_main.c"
#else
#include "parameters_sharkl3/sensor_ov7251_raw_param_main.c"
#endif
    return &s_ov7251_mipi_raw_info;
}
