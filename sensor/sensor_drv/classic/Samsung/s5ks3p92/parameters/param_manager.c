#include "sensor_raw.h"

struct sensor_raw_info *s5ks3p92_drv_init_raw_info(int sensor_id, int vendor_id,
                                                  int flash_ic_type,
                                                  int producer_id) {
#ifdef _SENSOR_RAW_SHARKL5PRO_H_
#include "parameters_sharkl5pro/sensor_s5ks3p92_raw_param_main.c"
#endif
return &s_s5ks3p92_mipi_raw_info;
}
