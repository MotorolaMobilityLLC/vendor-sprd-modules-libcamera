#include "sensor_raw.h"

struct sensor_raw_info *ov8856_drv_init_raw_info(int sensor_id, int vendor_id,
                                                 int flash_ic_type,
                                                 int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;
#ifdef _SENSOR_RAW_PIKE2_H_
#include "parameter_pike2/sensor_ov8856_raw_param_main.c"
    return &s_ov8856_mipi_raw_info;
#elif defined _SENSOR_RAW_SHARKL3_H_
    // back master
    if (0 == sensor_id) {
#include "parameters_sharkl3_go_back/sensor_ov8856_back_raw_param_main.c"
        return &s_ov8856_back_mipi_raw_info;
    }
    // front matser
    if (1 == sensor_id) {
#ifdef SENSOR_OV8856_TELE
#include "parameters_sharkl3_Tele/sensor_ov8856_front_raw_param_main.c"
#else
#include "parameters_sharkl3_front/sensor_ov8856_front_raw_param_main.c"
#endif
        return &s_ov8856_front_mipi_raw_info;
    }
    // back slave
    if (2 == sensor_id) {
#include "parameters_sharkl3_back/sensor_ov8856_back_raw_param_main.c"
        return &s_ov8856_back_mipi_raw_info;
    }
#elif defined(_SENSOR_RAW_SHARKL5_H_)
#include "parameters_sharkl5_front/sensor_ov8856_raw_param_main.c"
    return &s_ov8856_mipi_raw_info;
#else
#include "parameters_sharkle/sensor_ov8856_raw_param_main.c"
    return &s_ov8856_mipi_raw_info;
#endif

    return NULL;
}
