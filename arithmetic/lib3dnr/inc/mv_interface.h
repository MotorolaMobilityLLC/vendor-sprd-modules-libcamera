#ifndef _3DNR_MV_INTERFACE_H_
#define _3DNR_MV_INTERFACE_H_

#include <stdint.h>
#include <utils/Log.h>

typedef enum {
	THREAD_NUM_ONE = 1,
	THREAD_NUM_MAX,
} THREAD_NUMBER;

#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define BL_LOGV(format,...) ALOGV(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define BL_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define BL_LOGI(format,...) ALOGI(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define BL_LOGW(format,...) ALOGW(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

void ProjectMatching(int32_t *xProjRef, int32_t *yProjRef, int32_t *xProjIn, int32_t *yProjIn, uint32_t width, uint32_t height, int8_t  *pOffset, uint32_t num, int32_t *extra);
int DNR_init_threadPool();
int DNR_destroy_threadPool();
void IntegralProjection1D_process(uint8_t *img, uint32_t w, uint32_t h,int32_t* xProjIn,int32_t* yProjIn, int32_t* extra);

#endif //_3DNR_MV_INTERFACE_H_
