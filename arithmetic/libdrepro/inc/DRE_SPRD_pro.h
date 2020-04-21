#ifndef __SPRD_DRE_H__
#define __SPRD_DRE_H__

#include "stdint.h"
#include "sprd_camalg_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CHANNEL_NUM	3

#define ECALLOC 1
#define EPARAM 2
#define EINPUT 3

/*
 * sprd_dre_init: initialize the dre library handle.
 * @param:
 * - handle: [in & out] the pointer to the handle pointer.
 * - init_param: [in] init params.
 * @return: 0 is success, other is failure.
 */
int sprd_dre_pro_init(void **handle, int max_width, int max_height, void *init_param);

/*
 * sprd_dre_run: run the dre process.
 * @param:
 * - handle: [in] the handle pointer.
 * - input: [in] input image
 * - output: [out] output image, can reuse input image.
 * - param: [in] reserved, set 0 is ok.
 * @return: 0 is success, other is failure.
 */
//int sprd_dre_run(void *handle, sprd_camalg_image_t *input, sprd_camalg_image_t *output, void *param);
int sprd_dre_pro_run(void *handle, sprd_camalg_image_t *input, void *param);

/*
 * sprd_dre_fast_stop: set stop flag.
 * @param:
 * - handle: [in] the handle pointer.
 */
void sprd_dre_pro_fast_stop(void *handle);

/*
 * sprd_dre_deinit: deinitialize the dre library handle.
 * @param:
 * - handle: [in & out] the pointer to the handle pointer.
 * @return: 0 is success, other is failure.
 */
int sprd_dre_pro_deinit(void **handle);

#ifdef __cplusplus
}
#endif

#endif