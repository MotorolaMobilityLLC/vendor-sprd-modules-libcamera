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

struct sensor_zoom_info_pair {
    combination_type comb_type;
    char module_name[30];
};

/*it must be matching json file*/
struct sensor_zoom_info_pair zoom_info_pairs[COMBINATION_MAX] = {
    {W_SW_T_THREESECTION_ZOOM, "three_section_zoom_config"},
    {SW_T_DUALVIDEO_ZOOM, "dualvideo_zoom_config"},
    {W_SW_TWOSECTION_ZOOM, "two_section_zoom_config"},
};

static cmr_int _sensor_drv_json_init_root(const char *filename, struct cJSON **proot)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    char *buf;
    long len;
    char *str;
    char *retstr;
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
    fseek(f, 0, SEEK_SET);
    str = (char *)malloc(len + 1);
    fread(str, 1, len, f);
    fclose(f);

    *proot = cJSON_Parse(str);

    if (NULL == *proot) {
        CMR_LOGE("config format error");
        ret = CMR_CAMERA_FAIL;
        free(str);
        goto exit;
    }
    free(str);
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

static cmr_int _sensor_drv_json_fill_sub_char(struct cJSON *subroot, const char *keyanme,
                                              char *deschar)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *strroot = NULL;

    strroot = cJSON_GetObjectItem(subroot, keyanme);
    if (NULL == strroot) {
        CMR_LOGI("KEY LOSS IN %s", keyanme);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (strroot->type == cJSON_String)
            strcpy(deschar, strroot->valuestring);
        else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u8(struct cJSON *subroot, const char *keyanme,
                                            cmr_u8 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyanme);
    if (NULL == num_sub) {
        CMR_LOGI("KEY LOSS IN %s", keyanme);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = (cmr_u8)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u16(struct cJSON *subroot, const char *keyanme,
                                             cmr_u16 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyanme);
    if (NULL == num_sub) {
        CMR_LOGI("KEY LOSS IN %s", keyanme);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = (cmr_u16)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u32(struct cJSON *subroot, const char *keyanme,
                                             cmr_u32 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyanme);
    if (NULL == num_sub) {
        CMR_LOGI("KEY LOSS IN %s", keyanme);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = (cmr_u32)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_array(struct cJSON *subroot, const char *keyanme,
                                               cJSON **desarray)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyanme);
    if (NULL == num_sub) {
        CMR_LOGI("KEY LOSS IN %s", keyanme);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_Array) {
            *desarray = num_sub;
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_u32_array(struct cJSON *subroot, const char *keyanme,
                                                   cmr_u32 *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub, *array = NULL;
    cmr_u32 *input_pointer = desnum;
    int size;

    ret = _sensor_drv_json_fill_sub_array(subroot, keyanme, &array);
    if (ret)
        goto exit;

    size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        num_sub = cJSON_GetArrayItem(array, i);
        if (num_sub->type == cJSON_String) {
            *input_pointer = (cmr_u32)strtol(num_sub->valuestring, NULL, 0);
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        input_pointer++;
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_float(struct cJSON *subroot, const char *keyanme,
                                               float *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub = NULL;

    num_sub = cJSON_GetObjectItem(subroot, keyanme);
    if (NULL == num_sub) {
        CMR_LOGI("KEY LOSS IN %s", keyanme);
        ret = CMR_CAMERA_FAIL;
        goto exit;
    } else {
        if (num_sub->type == cJSON_String) {
            *desnum = strtof(num_sub->valuestring, NULL);
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
        }
    }

exit:
    return ret;
}

static cmr_int _sensor_drv_json_fill_sub_float_array(struct cJSON *subroot, const char *keyanme,
                                                     float *desnum)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;
    struct cJSON *num_sub, *array = NULL;
    float *input_pointer = desnum;
    int size;

    ret = _sensor_drv_json_fill_sub_array(subroot, keyanme, &array);
    if (ret)
        goto exit;

    size = cJSON_GetArraySize(array);
    for (int i = 0; i < size; i++) {
        num_sub = cJSON_GetArrayItem(array, i);
        if (num_sub->type == cJSON_String) {
            *input_pointer = strtof(num_sub->valuestring, NULL);
        } else {
            CMR_LOGI("TYPE Failed IN %s", keyanme);
            ret = CMR_CAMERA_FAIL;
            goto exit;
        }
        input_pointer++;
    }

exit:
    return ret;
}

/*fill zoom_info*/
static cmr_int _sensor_drv_json_fill_zoom_info(struct sensor_zoom_info_pair zoom_info_pair,
                                               struct sensor_zoom_info *pZoomInfo,
                                               struct cJSON *root)
{
    cmr_int ret = CMR_CAMERA_SUCCESS;

    struct cJSON *subroot = cJSON_GetObjectItem(root, zoom_info_pair.module_name);

    if (subroot == NULL) {
        CMR_LOGE("%s KEY LOSS", zoom_info_pair.module_name);
        ret = CMR_CAMERA_FAIL;
    } else {
        ret |= _sensor_drv_json_fill_sub_u16(
            subroot, "PhyCameras", 
            &pZoomInfo->zoom_info_cfg[zoom_info_pair.comb_type].PhyCameras);
        ret |= _sensor_drv_json_fill_sub_float(
            subroot, "MaxDigitalZoom",
            &pZoomInfo->zoom_info_cfg[zoom_info_pair.comb_type].MaxDigitalZoom);
        ret |= _sensor_drv_json_fill_sub_float_array(
            subroot, "ZoomRatioSection",
            &pZoomInfo->zoom_info_cfg[zoom_info_pair.comb_type].ZoomRatioSection);
        ret |= _sensor_drv_json_fill_sub_float(
            subroot, "BinningRatio",
            &pZoomInfo->zoom_info_cfg[zoom_info_pair.comb_type].BinningRatio);
        ret |= _sensor_drv_json_fill_sub_float(
            subroot, "IspMaxHwScalCap",
            &pZoomInfo->zoom_info_cfg[zoom_info_pair.comb_type].IspMaxHwScalCap);
    }

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
    ret |= _sensor_drv_json_fill_zoom_info(zoom_info_pairs[W_SW_T_THREESECTION_ZOOM], pZoomInfo, root);
    ret |= _sensor_drv_json_fill_zoom_info(zoom_info_pairs[SW_T_DUALVIDEO_ZOOM], pZoomInfo, root);
    ret |= _sensor_drv_json_fill_zoom_info(zoom_info_pairs[W_SW_TWOSECTION_ZOOM], pZoomInfo, root);
    _sensor_drv_json_deinit_root(root);

    if (!ret) {
        pZoomInfo->init = true;
    }

exit:
    return ret;
}
