#define LOG_TAG "sns_drv_json"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cjson.h"
#include "cmr_common.h"
#include "sensor_zoom_info.h"

#define SENSOR_DRV_JSON_MAX_CHAR_LEN 0xFF
#define SENSOR_DRV_JSON_DEFUALT_ERROR_VAL 0xFFFFFFFF

/*it must be matching json file*/
char comb_type_str[COMBINATION_MAX][15] = {
    [DEFAULT_ZOOM] = "DefaultZoom",
    [DUALVIDEO_ZOOM] = "DualVideoZoom",
};

char camera_type_str[CAM_TYPE_OPTIMUTI][15] = {
    [CAM_TYPE_AUX1] = "CameraSw",
    [CAM_TYPE_MAIN] = "CameraWide",
    [CAM_TYPE_AUX2] = "CameraTele",
};

static cmr_int _sensor_drv_json_init_root(const char *filename, struct cJSON **proot)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    long len;
    char *str = NULL;
    FILE *f;

    if (filename == NULL || proot == NULL) {
        CMR_LOGE("Invalid filename or root");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    f = fopen(filename, "rb");

    if (f == NULL) {
        CMR_LOGE("Invalid config path.");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);

    if (len < 0) {
        fclose(f);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    fseek(f, 0, SEEK_SET);
    str = (char *)malloc(len + 1);

    if (str == NULL) {
        fclose(f);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    memset((void *)str, 0, len + 1);
    fread(str, 1, len, f);
    fclose(f);

    *proot = cJSON_Parse(str);

    if (NULL == *proot) {
        CMR_LOGE("config format error");
        ret = CMR_CAMERA_FAIL;
    }

    free(str);
    str = NULL;
exit:
    return ret;
}

static void _sensor_drv_json_deinit_root(struct cJSON *root)
{
    if (root == NULL) {
        CMR_LOGE("Invalid filename or root");
        goto exit;
    }

    cJSON_Delete(root);

exit:
    return;
}

static cmr_int _sensor_drv_json_fill_sub_char(struct cJSON *subroot, const char *keyname,
                                              char *deschar)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *strroot = NULL;

    strroot = cJSON_GetObjectItem(subroot, keyname);
    if (strroot == NULL) {
        CMR_LOGE("KEY LOSS IN %s", keyname);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (strroot->type == cJSON_String)
            strcpy(deschar, strroot->valuestring);
        else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u8(struct cJSON *subroot, const char *keyname,
                                            cmr_u8 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyname);
    if (num_sub == NULL || desnum == NULL) {
        CMR_LOGE("KEY LOSS IN %s", keyname);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = (cmr_u8)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u32(struct cJSON *subroot, const char *keyname,
                                             cmr_u32 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyname);
    if (num_sub == NULL || desnum == NULL) {
        CMR_LOGE("KEY LOSS IN %s", keyname);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = (cmr_u32)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_array(struct cJSON *subroot, const char *keyname,
                                               cJSON **desarray)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyname);
    if (num_sub == NULL) {
        CMR_LOGE("KEY LOSS IN %s", keyname);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_Array) {
            *desarray = num_sub;
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u32_array(struct cJSON *subroot, const char *keyname,
                                                   cmr_u32 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub, *array = NULL;
    cmr_u32 *input_pointer = desnum;
    int size;

    if (input_pointer == NULL) {
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = _sensor_drv_json_fill_sub_array(subroot, keyname, &array);
    if (ret)
        goto exit;

    size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        num_sub = cJSON_GetArrayItem(array, i);
        if (num_sub->type == cJSON_String) {
            *input_pointer = (cmr_u32)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        input_pointer++;
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_float(struct cJSON *subroot, const char *keyname,
                                               float *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyname);
    if (num_sub == NULL) {
        CMR_LOGE("KEY LOSS IN %s", keyname);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = strtof(num_sub->valuestring, NULL);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_float_array(struct cJSON *subroot, const char *keyname,
                                                     float *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub, *array = NULL;
    float *input_pointer = desnum;
    int size;

    if (input_pointer == NULL) {
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = _sensor_drv_json_fill_sub_array(subroot, keyname, &array);
    if (ret)
        goto exit;

    size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        num_sub = cJSON_GetArrayItem(array, i);
        if (num_sub->type == cJSON_String) {
            *input_pointer = strtof(num_sub->valuestring, NULL);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        input_pointer++;
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_zoom_region(struct cJSON *root, const char *keyname,
                                                 struct zoom_region *cam_zoom_region)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *subroot = cJSON_GetObjectItem(root, keyname);

    if (subroot == NULL) {
        CMR_LOGE("%s KEY LOSS", keyname);
        ret = CMR_CAMERA_FAIL;
    } else {
        if (subroot->type == cJSON_Object) {
            ret |= _sensor_drv_json_fill_sub_float(subroot, "Min", &cam_zoom_region->Min);
            ret |= _sensor_drv_json_fill_sub_float(subroot, "Max", &cam_zoom_region->Max);
            ret |= _sensor_drv_json_fill_sub_u8(subroot, "Enable", &cam_zoom_region->Enable);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }

    return ret;
}

static cmr_int _sensor_drv_json_traverse_zoom_regions(struct cJSON *root, const char *keyname,
                                                      struct zoom_region *cam_zoom_region)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *subroot = cJSON_GetObjectItem(root, keyname);

    if (subroot == NULL) {
        CMR_LOGE("%s KEY LOSS", keyname);
        ret = CMR_CAMERA_FAIL;
    } else {
        if (subroot->type == cJSON_Object) {
            ret |= _sensor_drv_json_fill_zoom_region(subroot, camera_type_str[CAM_TYPE_AUX1],
                                                     &cam_zoom_region[CAM_TYPE_AUX1]);
            ret |= _sensor_drv_json_fill_zoom_region(subroot, camera_type_str[CAM_TYPE_MAIN],
                                                     &cam_zoom_region[CAM_TYPE_MAIN]);
            ret |= _sensor_drv_json_fill_zoom_region(subroot, camera_type_str[CAM_TYPE_AUX2],
                                                     &cam_zoom_region[CAM_TYPE_AUX2]);
        } else {
            CMR_LOGE("TYPE Failed IN %s", keyname);
            ret = CMR_CAMERA_FAIL;
        }
    }
    return ret;
}

/*fill zoom_info*/
static cmr_int _sensor_drv_json_fill_zoom_info(CombinationType comb_type,
                                               struct sensor_zoom_info *pZoomInfo,
                                               struct cJSON *root)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct cJSON *subroot = cJSON_GetObjectItem(root, comb_type_str[comb_type]);

    if (subroot == NULL) {
        CMR_LOGE("%s KEY LOSS", comb_type_str[comb_type]);
        ret = CMR_CAMERA_FAIL;
    } else {
        ret |= _sensor_drv_json_fill_sub_u32(subroot, "PhyCameras",
                                             &pZoomInfo->zoom_info_cfg[comb_type].PhyCameras);
        ret |= _sensor_drv_json_fill_sub_float(subroot, "MaxDigitalZoom",
                                               &pZoomInfo->zoom_info_cfg[comb_type].MaxDigitalZoom);
        ret |= _sensor_drv_json_fill_sub_float(subroot, "BinningRatio",
                                               &pZoomInfo->zoom_info_cfg[comb_type].BinningRatio);
        ret |= _sensor_drv_json_traverse_zoom_regions(
            subroot, "PhyCamZoomRegion", &pZoomInfo->zoom_info_cfg[comb_type].PhyCamZoomRegion);
    }
    return ret;
}

static cmr_int _sensor_drv_json_validate_param(CombinationType comb_type,
                                               struct sensor_zoom_info *pZoomInfo)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct zoom_info_config zoom_cfg = pZoomInfo->zoom_info_cfg[comb_type];
    int cam_type_arr[3] = {CAM_TYPE_MAIN, CAM_TYPE_AUX1, CAM_TYPE_AUX2};
    int count = 0;

    if (comb_type == DEFAULT_ZOOM &&
        !zoom_cfg.PhyCamZoomRegion[CAM_TYPE_MAIN].Enable) {
        CMR_LOGE("[%s] Does not support the combination of SW+T", comb_type_str[comb_type]);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    for (int i = 0; i < 3; i++) {
        if (zoom_cfg.PhyCamZoomRegion[cam_type_arr[i]].Enable) {
            count++;
        }
    }

    if (count != zoom_cfg.PhyCameras) {
        CMR_LOGE("[%s] Inconsistent number of available cameras", comb_type_str[comb_type]);
        ret = CMR_CAMERA_FAIL;
    }

exit:
    return ret;
}

cmr_int sensor_drv_json_get_zoom_config_info(const char *filename,
                                             struct sensor_zoom_info *pZoomInfo)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *root = NULL;

    if (filename == NULL || pZoomInfo == NULL) {
        CMR_LOGE("Invalid filename or pZoomInfo");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    ret = _sensor_drv_json_init_root(filename, &root);
    if (ret) {
        CMR_LOGE("_sensor_drv_json_init_root open fail");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    memset(pZoomInfo, 0, sizeof(struct sensor_zoom_info));
    ret |= _sensor_drv_json_fill_zoom_info(DEFAULT_ZOOM, pZoomInfo, root);
    ret |= _sensor_drv_json_fill_zoom_info(DUALVIDEO_ZOOM, pZoomInfo, root);
    _sensor_drv_json_deinit_root(root);

    if (ret) {
        CMR_LOGE("Failed to parse configuration file");
        ret = CMR_CAMERA_FAIL;
        goto exit;
    }

    /*validate json cfg*/
    ret |= _sensor_drv_json_validate_param(DEFAULT_ZOOM, pZoomInfo);
    ret |= _sensor_drv_json_validate_param(DUALVIDEO_ZOOM, pZoomInfo);

    if (!ret) {
        pZoomInfo->init = true;
    }

exit:
    return ret;
}
