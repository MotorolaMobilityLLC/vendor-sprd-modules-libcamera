#ifndef __SPRD_HDR_ADAPTER_LOG_H__
#define __SPRD_HDR_ADAPTER_LOG_H__

#include <utils/Log.h>

#define LOG_TAG		"sprd_hdr_adapter"

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define HDR_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define HDR_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define HDR_LOGW(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif
