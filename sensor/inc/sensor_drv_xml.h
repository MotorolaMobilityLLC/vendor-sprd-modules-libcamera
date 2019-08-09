#include <log/log.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libxml/parser.h>
#include "cmr_common.h"

#define XML_ROOT_TAG "root"
#define XML_CAM_MODULE_TAG "CameraModuleCfg"
#define XML_LENS_INFO_TAG "LensInfo"
#define XML_OTP_INFO_TAG "OTP"
#define XML_E2PROM_OTP_TAG "E2prom"
#define XML_SENSOR_OTP_TAG "SensorOtp"
#define XML_VCM_TAG "VCM"
#define XML_TUNING_PARA_TAG "TuningParameter"

#define MAX_NAME_LEN 36
#define MAX_KEY_LEN 24
#define MAX_HASH_MAP_SIZE 32

#define XML_NODE_CHECK_PTR(expr)                                               \
    do {                                                                       \
        if (NULL == (expr)) {                                                  \
            SENSOR_LOGE("sensor_drv_xml failed NULL pointer  " #expr);         \
            return -1;                                                         \
        }                                                                      \
    } while (0)

enum xmlDataType {
    XML_DATA_STRING = 0,
    XML_DATA_INT8,
    XML_DATA_UINT8,
    XML_DATA_INT16,
    XML_DATA_UINT16,
    XML_DATA_UINT64,
    XML_DATA_INT32,
    XML_DATA_UINT32,
    XML_DATA_FLOAT,
    XML_DATA_MAX
};

enum xmlE2prom_num_t {
    XML_SINGLE_CAM_ONE_EEPROM = 0,
    XML_DUAL_CAM_ONE_EEPROM = 1,
    XML_DUAL_CAM_TWO_EEPROM = 2,
    XML_MULTICAM_INDEPENDENT_EEPROM = 3,
};

struct xmlHashMap {
    char key[MAX_KEY_LEN];
    void *value;
    xmlElementType elem_type;
    enum xmlDataType data_type;
};

typedef struct xml_camera_lens_info {
    char lens_name[MAX_NAME_LEN];
} xml_camera_lens_info_t;

struct xml_e2prom_otp_info {
    char otp_name[MAX_NAME_LEN];
    unsigned char eeprom_i2c_addr;
    enum xmlE2prom_num_t eeprom_num;
    unsigned int eeprom_size;
};

struct xml_sensor_otp_info {
    char otp_name[MAX_NAME_LEN];
};

typedef struct xml_camera_otp_info {
    struct xml_e2prom_otp_info e2p_otp;
    struct xml_sensor_otp_info sensor_otp;
} xml_camera_otp_info_t;

typedef struct xml_camera_vcm_info {
    char af_name[MAX_NAME_LEN];
    unsigned char work_mode;
} xml_camera_vcm_info_t;

typedef struct xml_camera_tuning_info {
    char tuning_para_name[MAX_NAME_LEN];
} xml_camera_tuning_info_t;

typedef struct xml_camera_module_cfg {
    /*+++sensor section+++*/
    unsigned char slot_id;
    char sensor_name[MAX_NAME_LEN];
    int facing;
    int orientation;
    int resource_cost;
    /*---sensor section---*/

    /*+++lens section+++*/
    xml_camera_lens_info_t lens_info;
    /*---lens section---*/

    /*+++otp section+++*/
    xml_camera_otp_info_t otp_info;
    /*---otp section---*/

    /*+++vcm section+++*/
    xml_camera_vcm_info_t vcm_info;
    /*---vcm section---*/

    /*+++tuning parameter section+++*/
    xml_camera_tuning_info_t tuning_info;
    /*---tuning parameter section---*/
} xml_camera_module_cfg_t;

typedef struct xml_camera_cfg_info {
    xmlDocPtr docPtr;
    xmlNodePtr nodePtr;
    xml_camera_module_cfg_t *cfgPtr;
} xml_camera_cfg_info_t;

int sensor_drv_xml_load_file(char *file_name, xmlDocPtr *pDocPtr,
                             xmlNodePtr *pRootPtr, const char *nodeName);
int sensor_drv_xml_unload_file(xmlDocPtr docPtr);
xmlNodePtr sensor_drv_xml_get_node(xmlNodePtr pNodePtr, const char *nodeName,
                                   int node_index);
int sensor_drv_xml_get_node_num(xmlNodePtr pNodePtr, const char *nodeName);
xmlNodePtr sensor_drv_xml_get_node_recursive(xmlNodePtr pNodePtr, const char *nodeName,
                                   int node_index);
int sensor_drv_xml_get_node_num_recursive(xmlNodePtr pNodePtr, const char *nodeName);
int sensor_drv_xml_parse_node_data(xmlNodePtr pNodePtr, const char *nodeName,
                                   struct xmlHashMap *xml_hash_map,
                                   unsigned int data_size, int node_index);
int sensor_drv_xml_parse_camera_module_info(
    struct xml_camera_cfg_info *camera_cfg);
int sensor_drv_xml_parse_vcm_info(struct xml_camera_cfg_info *camera_cfg);
int sensor_drv_xml_parse_otp_info(struct xml_camera_cfg_info *camera_cfg);
int sensor_drv_xml_parse_tuning_param_info(struct xml_camera_cfg_info *camera_cfg);

