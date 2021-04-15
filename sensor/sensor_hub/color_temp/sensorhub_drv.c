#include "sensorhub_drv.h"

cmr_u32 color_temp_calib[4] = {0};

static void get_color_temp_data(struct sensorhub_tcs_data *color_temp_data) {

    if(color_temp_callback_func == NULL) {
        SENSOR_LOGI("sensor hub color_temp_callback_func function pointer is null");
        return;
    }
    color_temp_callback_func(color_temp_calib);
    color_temp_data->x_data = color_temp_calib[0];
    color_temp_data->y_data = color_temp_calib[1];
    color_temp_data->z_data = color_temp_calib[2];
    color_temp_data->ir_data = color_temp_calib[3];
    color_temp_data->x_raw = 0;
    color_temp_data->y_raw = 0;
    color_temp_data->z_raw = 0;
    color_temp_data->ir_raw = 0;
    color_temp_data->atime_data = 0;
    color_temp_data->atime_data = 0;
    color_temp_data->lux_data = 0;
    color_temp_data->cct_data = 0;

    SENSOR_LOGI("X");
    return;
}

int sensorhub_read_data(void *param) {
    struct sensorhub_tcs_data *tcs_data_calib = (struct sensorhub_tcs_data *)param;

    memset(tcs_data_calib, 0, sizeof(struct sensorhub_tcs_data));
    get_color_temp_data(tcs_data_calib);
    SENSOR_LOGV("sensor_hub x_data:%d, y_data:%d, z_data:%d, ir_data:%d, "
                "x_raw:%d, y_raw:%d, z_raw:%d, ir_raw:%d\n",
                tcs_data_calib->x_data, tcs_data_calib->y_data,
                tcs_data_calib->z_data, tcs_data_calib->ir_data,
                tcs_data_calib->x_raw, tcs_data_calib->y_raw,
                tcs_data_calib->z_raw, tcs_data_calib->ir_raw);

    SENSOR_LOGI("X");
    return 0;
}