#ifndef _SWISP_INTERFACE_H
#define _SWISP_INTERFACE_H
#include "sprd_realtimebokeh.h"
#ifdef __cplusplus
extern "C" {
#endif
void* init_sw_isp(struct soft_isp_init_param* initparam);
int sw_isp_process(void* handle , uint16_t *img_data, uint8_t* outputyuv , void**aem_addr , int32_t* aem_size);
int sw_isp_update_param(void* handle , struct soft_isp_block_param *isp_blockparam);
void deinit_sw_isp(void* handle);
void load_img(const char* filename , int width ,int height , uint16_t *buffer);
#ifdef __cplusplus
}
#endif

#endif

