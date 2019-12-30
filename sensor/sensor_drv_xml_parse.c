#define LOG_TAG "sns_drv_xml"
#include "sensor_drv_xml.h"
#include "cmr_log.h"
#define RECURSIVE_TRAVERSAL_ENABLE

int sensor_drv_xml_load_file(char *file_name, xmlDocPtr *pDocPtr,
                             xmlNodePtr *pRootPtr, const char *nodeName) {
    int ret = -1;
    xmlDocPtr docPtr = NULL;
    xmlNodePtr rootPtr = NULL;
    xmlNodePtr cur = NULL;

    if (!file_name) {
        SENSOR_LOGE("camera xml config file is null");
        goto exit;
    }

    docPtr = xmlParseFile(file_name);
    if (!docPtr) {
        SENSOR_LOGE("xmlParseFile failed");
        goto exit;
    }

    rootPtr = xmlDocGetRootElement(docPtr);
    if (!rootPtr) {
        SENSOR_LOGE("xmlDocGetRootElement failed");
        goto error;
    }

    if (xmlStrcmp(rootPtr->name, (const xmlChar *)nodeName)) {
        SENSOR_LOGE("node %s not found invalid node %s", nodeName,
                    rootPtr->name);
        goto error;
    }

    *pDocPtr = docPtr;
    *pRootPtr = rootPtr;
    ret = 0;
    goto exit;

error:
    xmlFreeDoc(docPtr);

exit:
    return ret;
}

int sensor_drv_xml_unload_file(xmlDocPtr docPtr) {
    xmlFreeDoc(docPtr);
    return 0;
}

xmlNodePtr sensor_drv_xml_get_node(xmlNodePtr pNodePtr, const char *nodeName,
                                   int node_index) {
#ifndef RECURSIVE_TRAVERSAL_ENABLE
    int node_num = 0;
    xmlNodePtr curPtr = NULL;
    xmlNodePtr nodePtr = NULL;

    for (curPtr = xmlFirstElementChild(pNodePtr); curPtr;
         curPtr = xmlNextElementSibling(curPtr)) {
        if (!xmlStrcmp(curPtr->name, (const xmlChar *)nodeName)) {
            if (node_index == node_num) {
                nodePtr = curPtr;
                SENSOR_LOGD("the node index is %d of %s", node_num, nodeName);
                break;
            }
            node_num++;
        }
    }
    return nodePtr;
#else
    return sensor_drv_xml_get_node_recursive(pNodePtr, nodeName, node_index);
#endif
}

xmlNodePtr sensor_drv_xml_get_node_recursive(xmlNodePtr pNodePtr,
                                             const char *nodeName,
                                             int node_index) {
    int node_num = 0;
    xmlNodePtr curPtr = NULL;
    xmlNodePtr nodePtr = NULL;

    if (!xmlStrcmp(pNodePtr->name, (const xmlChar *)nodeName)) {
        if (node_index == 0)
            return pNodePtr;
        else
            node_index--;
    }

    for (curPtr = xmlFirstElementChild(pNodePtr); curPtr;
         curPtr = xmlNextElementSibling(curPtr)) {
        node_num = sensor_drv_xml_get_node_num(curPtr, nodeName);
        if (node_num > node_index)
            return sensor_drv_xml_get_node(curPtr, nodeName, node_index);
        else
            node_index -= node_num;
    }
    return NULL;
}

int sensor_drv_xml_get_node_num(xmlNodePtr pNodePtr, const char *nodeName) {
#ifndef RECURSIVE_TRAVERSAL_ENABLE
    int node_num = 0;
    xmlNodePtr curPtr = NULL;

    if (!pNodePtr) {
        SENSOR_LOGE("pNodePtr not found");
        goto exit;
    }
    if (!nodeName) {
        SENSOR_LOGE("nodeName is null");
        goto exit;
    }

    for (curPtr = xmlFirstElementChild(pNodePtr); curPtr;
         curPtr = xmlNextElementSibling(curPtr)) {
        if (!xmlStrcmp(curPtr->name, (const xmlChar *)nodeName)) {
            node_num++;
        }
    }

exit:
    SENSOR_LOGD("the num is %d of %s", node_num, nodeName);
    return node_num;
#else
    return sensor_drv_xml_get_node_num_recursive(pNodePtr, nodeName);
#endif
}

int sensor_drv_xml_get_node_num_recursive(xmlNodePtr pNodePtr,
                                          const char *nodeName) {
    int node_num = 0;
    xmlNodePtr curPtr = NULL;

    if (!pNodePtr) {
        SENSOR_LOGE("pNodePtr not found");
        goto exit;
    }
    if (!nodeName) {
        SENSOR_LOGE("nodeName is null");
        goto exit;
    }

    if (!xmlStrcmp(pNodePtr->name, (const xmlChar *)nodeName))
        node_num++;
    for (curPtr = xmlFirstElementChild(pNodePtr); curPtr;
         curPtr = xmlNextElementSibling(curPtr)) {
        node_num += sensor_drv_xml_get_node_num(curPtr, nodeName);
    }

exit:
    SENSOR_LOGV("the num is %d of %s", node_num, nodeName);
    return node_num;
}

int sensor_drv_xml_parse_node_data(xmlNodePtr pNodePtr, const char *nodeName,
                                   struct xmlHashMap *xml_hash_map,
                                   unsigned int data_size, int node_index) {
    int i = 0;
    xmlNodePtr nodePtr = NULL;
    xmlNodePtr curPtr = NULL;
    char *xml_str = NULL;
    int ret = -1;

    if (!pNodePtr) {
        SENSOR_LOGE("pNodePtr not found");
        goto exit;
    }
    if (!nodeName) {
        SENSOR_LOGE("nodeName is null");
        goto exit;
    }

#ifdef RECURSIVE_TRAVERSAL_ENABLE
    nodePtr = sensor_drv_xml_get_node(pNodePtr, nodeName, node_index);
    if (!nodePtr) {
        SENSOR_LOGE("node %s get null", nodeName);
        goto exit;
    }
#else
    nodePtr = pNodePtr;
#endif
    for (i = 0; i < data_size; i++) {
        if (!xml_hash_map[i].value)
            goto exit;

        switch (xml_hash_map[i].elem_type) {
        case XML_ELEMENT_NODE: {
            SENSOR_LOGV("element node data name %s", nodePtr->name);

            for (curPtr = xmlFirstElementChild(nodePtr);
                 curPtr &&
                 xmlStrcmp(curPtr->name, (const xmlChar *)xml_hash_map[i].key);
                 curPtr = xmlNextElementSibling(curPtr))
                ;

            if (!curPtr) {
                SENSOR_LOGW("Node Tag %s not present in XML file",
                            xml_hash_map[i].key);
                continue;
            }
            xml_str = (char *)xmlNodeGetContent(curPtr->children);
        } break;
        case XML_ATTRIBUTE_NODE: {
            SENSOR_LOGV("attribute node data name %s", nodePtr->name);

            xml_str = (char *)xmlGetProp(nodePtr,
                                         (const xmlChar *)xml_hash_map[i].key);
        } break;
        default: {
            SENSOR_LOGE("data_type %d is invaild", xml_hash_map[i].data_type);
            goto exit;
        } break;
        }

        if (!xml_str) {
            SENSOR_LOGW("node %s not found in XML file", xml_hash_map[i].key);
            continue;
        }

        SENSOR_LOGD("index i = %d xm_str = %s", i, xml_str);

        switch (xml_hash_map[i].data_type) {
        case XML_DATA_STRING:
            strlcpy(xml_hash_map[i].value, xml_str,
                    xmlStrlen((const xmlChar *)xml_str) + 1);
            break;
        case XML_DATA_INT8:
            *((int8_t *)xml_hash_map[i].value) =
                (int8_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_UINT8:
            *((uint8_t *)xml_hash_map[i].value) =
                (uint8_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_INT16:
            *((int16_t *)xml_hash_map[i].value) =
                (int16_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_UINT16:
            *((uint16_t *)xml_hash_map[i].value) =
                (uint16_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_UINT64:
            *((uint64_t *)xml_hash_map[i].value) =
                (uint64_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_INT32:
            *((int32_t *)xml_hash_map[i].value) =
                (int32_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_UINT32:
            *((uint32_t *)xml_hash_map[i].value) =
                (uint32_t)strtoll(xml_str, NULL, 0);
            break;
        case XML_DATA_FLOAT:
            *((float *)xml_hash_map[i].value) = atof(xml_str);
            break;
        default:
            SENSOR_LOGE("data_type is invalid = %d", xml_hash_map[i].data_type);
            xmlFree(xml_str);
            goto exit;
        }

        xmlFree(xml_str);
    }

    ret = 0;
exit:
    return ret;
}

static int sensor_drv_xml_str_to_integer(char *str, int *digit) {
    if (!strcmp(str, "BACK"))
        *digit = SNS_FACE_BACK;
    else if (!strcmp(str, "FRONT"))
        *digit = SNS_FACE_FRONT;
    else
        return -1;

    return 0;
}

int sensor_drv_xml_parse_camera_module_info(
    struct xml_camera_cfg_info *camera_cfg) {
    int ret = 0;
    uint32_t elem_num = 0;
    xmlDocPtr docPtr = camera_cfg->docPtr;
    xmlNodePtr nodePtr = camera_cfg->nodePtr;
    xml_camera_module_cfg_t *cfgPtr = camera_cfg->cfgPtr;
    struct xmlHashMap xml_hash_map[MAX_HASH_MAP_SIZE];
    char facing[16];

    XML_NODE_CHECK_PTR(docPtr);
    XML_NODE_CHECK_PTR(nodePtr);
    XML_NODE_CHECK_PTR(cfgPtr);

    memset(cfgPtr, 0, sizeof(xml_camera_module_cfg_t));
    cfgPtr->slot_id = 0xff;

    strlcpy(xml_hash_map[elem_num].key, "SlotId", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->slot_id;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT8;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "SensorName", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = cfgPtr->sensor_name;
    xml_hash_map[elem_num].data_type = XML_DATA_STRING;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "Facing", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = facing;
    xml_hash_map[elem_num].data_type = XML_DATA_STRING;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "Orientation", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->orientation;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT16;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "Resource_cost", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->resource_cost;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT8;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    ret = sensor_drv_xml_parse_node_data(nodePtr, "CameraModuleCfg",
                                         xml_hash_map, elem_num, 0);

    if (ret)
        goto exit;

    sensor_drv_xml_str_to_integer(facing, &cfgPtr->facing);

exit:
    return ret;
}

int sensor_drv_xml_parse_vcm_info(struct xml_camera_cfg_info *camera_cfg) {
    int ret = 0;
    uint32_t elem_num = 0;
    xmlDocPtr docPtr = camera_cfg->docPtr;
    xmlNodePtr nodePtr = camera_cfg->nodePtr;
    xml_camera_module_cfg_t *cfgPtr = camera_cfg->cfgPtr;
    struct xmlHashMap xml_hash_map[MAX_HASH_MAP_SIZE];
#ifndef RECURSIVE_TRAVERSAL_ENABLE
    xmlNodePtr nodeVcmPtr = NULL;
#endif

    XML_NODE_CHECK_PTR(docPtr);
    XML_NODE_CHECK_PTR(nodePtr);
    XML_NODE_CHECK_PTR(cfgPtr);

    strlcpy(xml_hash_map[elem_num].key, "AfName", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = cfgPtr->vcm_info.af_name;
    xml_hash_map[elem_num].data_type = XML_DATA_STRING;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "Mode", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->vcm_info.work_mode;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT8;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

#ifndef RECURSIVE_TRAVERSAL_ENABLE
    nodeVcmPtr = sensor_drv_xml_get_node(camera_cfg->nodePtr, "VCM", 0);
    if (nodeVcmPtr) {
        SENSOR_LOGD("parse vcm info node name %s", nodeVcmPtr->name);
        ret = sensor_drv_xml_parse_node_data(nodeVcmPtr, "VCM", xml_hash_map,
                                             elem_num, 0);
    }
#else
    ret = sensor_drv_xml_parse_node_data(nodePtr, "VCM", xml_hash_map, elem_num,
                                         0);
#endif
    return ret;
}

int sensor_drv_xml_parse_otp_info(struct xml_camera_cfg_info *camera_cfg) {
    int ret = 0;
    uint32_t elem_num = 0;
    xmlDocPtr docPtr = camera_cfg->docPtr;
    xmlNodePtr nodePtr = camera_cfg->nodePtr;
    xml_camera_module_cfg_t *cfgPtr = camera_cfg->cfgPtr;
    struct xmlHashMap xml_hash_map[MAX_HASH_MAP_SIZE];
#ifndef RECURSIVE_TRAVERSAL_ENABLE
    xmlNodePtr nodeOtpPtr = NULL;
#endif

    XML_NODE_CHECK_PTR(docPtr);
    XML_NODE_CHECK_PTR(nodePtr);
    XML_NODE_CHECK_PTR(cfgPtr);
#ifndef RECURSIVE_TRAVERSAL_ENABLE
    nodeOtpPtr = sensor_drv_xml_get_node(camera_cfg->nodePtr, "OTP", 0);
    if (nodeOtpPtr)
        SENSOR_LOGD("parse otp info node name %s", nodeOtpPtr->name);
#endif

    /*==================E2prom section=============================*/

    strlcpy(xml_hash_map[elem_num].key, "OtpName", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = cfgPtr->otp_info.e2p_otp.otp_name;
    xml_hash_map[elem_num].data_type = XML_DATA_STRING;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "I2cAddr", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->otp_info.e2p_otp.eeprom_i2c_addr;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT16;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "E2promNum", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->otp_info.e2p_otp.eeprom_num;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT8;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

    strlcpy(xml_hash_map[elem_num].key, "E2promSize", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = &cfgPtr->otp_info.e2p_otp.eeprom_size;
    xml_hash_map[elem_num].data_type = XML_DATA_UINT16;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

#ifndef RECURSIVE_TRAVERSAL_ENABLE
    if (nodeOtpPtr) {
        nodePtr = sensor_drv_xml_get_node(nodeOtpPtr, "E2prom", 0);
        if (nodePtr)
            SENSOR_LOGD("parse e2prom info node name %s", nodePtr->name);
    }
    if (nodePtr)
#endif
        ret = sensor_drv_xml_parse_node_data(nodePtr, "E2prom", xml_hash_map,
                                             elem_num, 0);
    /*==================SensorOtp section=============================*/

    /*
        elem_num = 0;
        memset(xml_hash_map, 0, sizeof(xmlHashMap)*MAX_HASH_MAP_SIZE);

#ifndef RECURSIVE_TRAVERSAL_ENABLE
    if (nodeOtpPtr) {
    nodePtr = sensor_drv_xml_get_node(nodeOtpPtr,"SensorOtp",0);
    if (nodePtr)
        SENSOR_LOGD("parse sensorOtp info node name %s", nodePtr->name);
    }
    if (nodePtr)
#endif
        ret = sensor_drv_xml_parse_node_data(nodePtr, "SensorOtp", xml_hash_map,
       elem_num, 0);
    */
    return ret;
}

int sensor_drv_xml_parse_tuning_param_info(
    struct xml_camera_cfg_info *camera_cfg) {
    int ret = 0;
    uint32_t elem_num = 0;
    xmlDocPtr docPtr = camera_cfg->docPtr;
    xmlNodePtr nodePtr = camera_cfg->nodePtr;
    xml_camera_module_cfg_t *cfgPtr = camera_cfg->cfgPtr;
    struct xmlHashMap xml_hash_map[MAX_HASH_MAP_SIZE];
#ifndef RECURSIVE_TRAVERSAL_ENABLE
    xmlNodePtr nodeParamPtr = NULL;
#endif

    XML_NODE_CHECK_PTR(docPtr);
    XML_NODE_CHECK_PTR(nodePtr);
    XML_NODE_CHECK_PTR(cfgPtr);

    strlcpy(xml_hash_map[elem_num].key, "TuningName", MAX_KEY_LEN);
    xml_hash_map[elem_num].value = cfgPtr->tuning_info.tuning_para_name;
    xml_hash_map[elem_num].data_type = XML_DATA_STRING;
    xml_hash_map[elem_num].elem_type = XML_ELEMENT_NODE;
    elem_num++;

#ifndef RECURSIVE_TRAVERSAL_ENABLE
    nodeParamPtr =
        sensor_drv_xml_get_node(camera_cfg->nodePtr, "TuningParameter", 0);
    if (nodeParamPtr) {
        SENSOR_LOGD("parse tuningParameter info node name %s",
                    nodeParamPtr->name);
        ret = sensor_drv_xml_parse_node_data(nodeParamPtr, "TuningParameter",
                                             xml_hash_map, elem_num, 0);
    }
#else
    ret = sensor_drv_xml_parse_node_data(nodePtr, "TuningParameter",
                                         xml_hash_map, elem_num, 0);
#endif
    return ret;
}
