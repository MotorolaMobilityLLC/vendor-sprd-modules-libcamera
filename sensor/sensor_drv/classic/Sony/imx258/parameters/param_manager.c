#include "sensor_raw.h"

struct sensor_raw_info *imx258_drv_init_raw_info(int sensor_id, int vendor_id, int flash_ic_type, int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;

#if defined(CONFIG_CAMERA_ISP_DIR_3)
#include "parameters_sharkl2/sensor_imx258_raw_param_v3.c"
#elif defined _SENSOR_RAW_SHARKL3_H_
#include "parameters_sharkl3/sensor_imx258_raw_param_main.c"
#elif defined(CONFIG_ISP_2_3)
#include "parameters_sharkle/sensor_imx258_raw_param_main.c"
#elif defined(_SENSOR_RAW_SHARKL5_H_)
#include "parameters_sharkl5/sensor_imx258_raw_param_main.c"
return &s_imx258_mipi_raw_info;

#else
#include "parameters_sharkl2/sensor_imx258_raw_param_main.c"
return &s_imx258_mipi_raw_info;
#endif

return NULL;
}

