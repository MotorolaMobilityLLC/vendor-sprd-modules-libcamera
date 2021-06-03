#include <stdio.h>
#include <string.h>
#include "cmr_type.h"

struct zoom_info_config {
    int PhyCameras;
    float MaxDigitalZoom;
    float ZoomRatioSection[6];
    float BinningRatio;
    float IspMaxHwScalCap;
};

typedef enum {
    W_SW_T_THREESECTION_ZOOM,
    SW_T_DUALVIDEO_ZOOM,
    W_SW_TWOSECTION_ZOOM,
    COMBINATION_MAX
} combination_type;

struct sensor_zoom_info {
    bool init;
    struct zoom_info_config zoom_info_cfg[COMBINATION_MAX];
};

cmr_int sensor_drv_json_get_zoom_config_info(const char *filename,
                                             struct sensor_zoom_info *pZoomInfo);
