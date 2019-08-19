#include "sensor_raw.h"

struct sensor_raw_info *ov13855_drv_init_raw_info(int sensor_id, int vendor_id, int flash_ic_type, int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;
#ifdef _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_ov13855_raw_param_main.c"
#elif defined(_SENSOR_RAW_SHARKL5PRO_H_)
#include "parameters_sharkl5pro/sensor_ov13855_raw_param_main.c"
#elif defined(CONFIG_CAMERA_FLASH_OCP8137)
#include "parameters_sharkle/sensor_ov13855_raw_param_main.c"
#elif defined(CONFIG_DUAL_MODULE)
#include "parameters_dual/sensor_ov13855_raw_param_main.c"
#else
#include "parameters/sensor_ov13855_raw_param_main.c"
#endif
return &s_ov13855_mipi_raw_info;
}

