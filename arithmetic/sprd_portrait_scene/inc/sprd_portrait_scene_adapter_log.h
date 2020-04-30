#ifndef __SPRD_PORTRAIT_SCENE_ADAPTER_LOG_H__
#define __SPRD_PORTRAIT_SCENE_ADAPTER_LOG_H__

#include <utils/Log.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "ADPT_portraitScene"

#define DEBUG_STR "L %d, %s: "
#define DEBUG_ARGS __LINE__,__FUNCTION__

#define PScene_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define PScene_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define PScene_LOGV(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#endif
