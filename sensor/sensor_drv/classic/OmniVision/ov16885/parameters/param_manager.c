#include "sensor_raw.h"

struct sensor_raw_info *ov16885_drv_init_raw_info(int sensor_id, int vendor_id,
                                                  int flash_ic_type,
                                                  int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;

#ifdef _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_ov16885_raw_param_main.c"
    return &s_ov16885_mipi_raw_info;
#else
    if (0 == sensor_id) {
#include "parameters_sharkl5/sensor_ov16885_raw_param_main.c"
        return &s_ov16885_mipi_raw_info;
    }
    if (1 == sensor_id) {
#include "parameters_roc1/sensor_ov16885_raw_param_main.c"
        return &s_ov16885_mipi_raw_info;
    }
#endif

    return NULL;
}
