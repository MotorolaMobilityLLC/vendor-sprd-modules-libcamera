#ifndef __SPRD_DRE_ADAPTER_HEADER_PRO_H_
#define __SPRD_DRE_ADAPTER_HEADER_PRO_H_

#include "DRE_SPRD_pro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))


typedef enum {
        SPRD_DRE_GET_VERSION_CMD = 0,
        SPRD_DRE_PROCESS_CMD,
        SPRD_DRE_FAST_STOP_CMD,
        SPRD_DRE_MAX_CMD
} sprd_dre_pro_cmd_t;

typedef struct {
        uint8_t            major;              /*!< API major version */
        uint8_t            minor;              /*!< API minor version */
        uint8_t            micro;              /*!< API micro version */
        uint8_t            nano;               /*!< API nano version */
        char        built_date[0x20];   /*!< API built date */
        char        built_time[0x20];   /*!< API built time */
        char        built_rev[0x100];   /*!< API built version, linked with vcs resivion> */
} sprd_dre_pro_version_t;

typedef struct {
        struct sprd_camalg_image input;
        struct sprd_camalg_image output;
} sprd_dre_pro_param_t;

/*
        init dre adapter instance
        return value: handle;
        @max_width/height: max supported width/height
        @param: reserved, pass 0 is ok
*/
JNIEXPORT int sprd_dre_pro_adpt_init(void **handle, int max_width, int max_height, void *param);

/*
        deinit dre adapter instance
        return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_dre_pro_adpt_deinit(void *handle);

/*
        dre adapter cmd process interface
        return value: 0 is ok, other value is failed
        @param: depend on cmd type:
                - SPRD_HDR_GET_VERSION_CMD: sprd_hdr_version_t
                - SPRD_HDR_PROCESS_CMD: sprd_hdr_param_t
                - SPRD_HDR_FAST_STOP_CMD: 0
*/
JNIEXPORT int sprd_dre_pro_adpt_ctrl(void *handle, sprd_dre_pro_cmd_t cmd, sprd_camalg_image_t* input, void *param);

/*
        dre adapter get running type, the output is type, such as cpu/gpu/vdsp
        return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_dre_pro_get_devicetype(enum camalg_run_type *type);

/*
        dre adapter set running type
        return value: 0 is ok, other value is failed
*/
JNIEXPORT int sprd_dre_pro_set_devicetype(enum camalg_run_type type);

#ifdef __cplusplus
}
#endif

#endif