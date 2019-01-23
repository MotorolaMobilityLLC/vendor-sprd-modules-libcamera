#include "sensor_raw.h"

struct sensor_raw_info *imx363_drv_init_raw_info(int sensor_id, int vendor_id,
                                                 int flash_ic_type,
                                                 int producer_id) {
    cmr_int rtn = SENSOR_SUCCESS;

#ifdef SHARKL5_OIS_IMX363
#include "parameters_sharkl5/sensor_imx363_raw_param_main.c"
#else
#include "parameters_roc1/sensor_imx363_raw_param_main.c"
#endif

    return &s_imx363_mipi_raw_info;
}
