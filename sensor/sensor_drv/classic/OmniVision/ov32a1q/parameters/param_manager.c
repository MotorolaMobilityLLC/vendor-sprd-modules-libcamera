#include "sensor_raw.h"

struct sensor_raw_info *ov32a1q_drv_init_raw_info(int sensor_id, int vendor_id,
                                                  int flash_ic_type,
                                                  int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;

#ifdef _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_ov32a1q_raw_param_main.c"
#elif defined(_SENSOR_RAW_SHARKL5PRO_H_)
#include "parameters_sharkl5pro/sensor_ov32a1q_raw_param_main.c"
#else
#include "parameters_sharkl5/sensor_ov32a1q_raw_param_main.c"
#endif

return &s_ov32a1q_mipi_raw_info;
}
