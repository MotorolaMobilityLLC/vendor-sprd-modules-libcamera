#ifndef __SPRD_PORTRAIT_SCENE_ADAPTER_HEADER_H__
#define __SPRD_PORTRAIT_SCENE_ADAPTER_HEADER_H__

#include "sprd_camalg_adapter.h"
#include "unisoc_portrait_scene_interface.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum sprd_portrait_scene_cmd{
    SPRD_PORTRAIT_SCENE_WEIGHT_CMD = 0,
    SPRD_PORTRAIT_SCENE_PROCESS_CMD,
    SPRD_PORTRAIT_SCENE_FUSE_CMD,
    SPRD_PORTRAIT_SCENE_MAX_CMD,
} sprd_portrait_scene_cmd_t;

typedef struct sprd_portrait_scene_adapter_mask {
	sprd_portrait_scene_channel_t ch;
	uint8_t *src_YUV;
	uint8_t *dst_YUV;
	uint16_t *mask;
} sprd_portrait_scene_adapter_mask_t;

typedef struct sprd_portrait_scene_adapter_fuse {
	uint8_t *src_YUV;
	uint8_t *dst_YUV;
	sprd_portrait_scene_fuse_t fuse;
} sprd_portrait_scene_adapter_fuse_t;

JNIEXPORT void *sprd_portrait_scene_adpt_init(sprd_portrait_scene_init_t *_param);
/* sprd_portrait_scene_adpt_init
* usage: call once at initialization
* return value: handle
* @ param: reserved, can be set NULL
*/

JNIEXPORT int sprd_portrait_scene_adpt_deinit(void *handle, sprd_portrait_scene_channel_t ch);
/* sprd_portrait_scene_adpt_deinit
* usage: call once at deinitialization
* return value: 0 is ok, other value is failed
* @ handle: algo handle
*/

JNIEXPORT int sprd_portrait_scene_adpt_ctrl(void *handle, sprd_portrait_scene_cmd_t cmd, void *param);
/* sprd_portrait_scene_adpt_ctrl
* usage: call each frame	
* return value: 0 is ok, other value is failed
* @ handle: algo handle
* @ cmd: switch  executing path
* @ param: depend on cmd type:
*- SPRD_PORTRAIT_SCENE_WEIGHT_CMD
*- SPRD_PORTRAIT_SCENE_PROCESS_CMD
*- SPRD_PORTRAIT_SCENE_FUSE_CMD
*/

JNIEXPORT int sprd_portrait_scene_get_devicetype(enum camalg_run_type *type);
/* sprd_portrait_scene_get_devicetype
* usage: portrait_scene adapter get running type, the output is type, such as cpu/vdsp/cpu_vdsp_fusion
* return value: 0 is ok, other value is failed
*/

#ifdef __cplusplus
}
#endif

#endif
