#ifndef __SPRD_ISPALG_ADPT_HEADER_H__
#define __SPRD_ISPALG_ADPT_HEADER_H__

#include "stdint.h"

#define SPRD_CAMALG_DATA_RESERVED_POINTER_NUM   4
#define SPRD_CAMALG_DATA_RESERVED_INT_NUM       4

#define SPRD_CAMALG_IMG_CHN_MAX     3

/*cam alg run on different hw, such as cpu/gpu/vdsp */
typedef enum camalg_run_type {
    SPRD_CAMALG_RUN_TYPE_CPU = 0,
    SPRD_CAMALG_RUN_TYPE_GPU,
    SPRD_CAMALG_RUN_TYPE_VDSP,
    SPRD_CAMALG_RUN_TYPE_MAX
} sprd_camalg_device_type;

/*Camera image format , IMG_BUF is special buffer ,such as depth*/
typedef enum sprd_camalg_image_format {
    SPRD_CAMALG_IMG_RAW16_GRBG,
    SPRD_CAMALG_IMG_RAW16_RGGB,
    SPRD_CAMALG_IMG_RAW16_BGGR,
    SPRD_CAMALG_IMG_RAW16_GBRG,
    SPRD_CAMALG_IMG_MIPI_GRBG,
    SPRD_CAMALG_IMG_MIPI_RGGB,
    SPRD_CAMALG_IMG_MIPI_BGGR,
    SPRD_CAMALG_IMG_MIPI_GBRG,
    SPRD_CAMALG_IMG_RAW10_GRBG,
    SPRD_CAMALG_IMG_RAW10_RGGB,
    SPRD_CAMALG_IMG_RAW10_BGGR,
    SPRD_CAMALG_IMG_RAW10_GBRG,
    SPRD_CAMALG_IMG_RAW14_GRBG,
    SPRD_CAMALG_IMG_RAW14_RGGB,
    SPRD_CAMALG_IMG_RAW14_BGGR,
    SPRD_CAMALG_IMG_RAW14_GBRG,
    SPRD_CAMALG_IMG_NV21 = 0x10,
    SPRD_CAMALG_IMG_NV12,
    SPRD_CAMALG_IMG_RGB8 = 0x100,
    SPRD_CAMALG_IMG_RGB10,
    SPRD_CAMALG_IMG_RGB14,
    SPRD_CAMALG_IMG_BUF = 0x1000,
    SPRD_CAMALG_MAX
} sprd_camalg_imageformat;

/*sprd_camalg_image struct*/
typedef struct sprd_camalg_image {
    sprd_camalg_imageformat format;
    void* addr[SPRD_CAMALG_IMG_CHN_MAX];//virtual addr for y/u/v
    int32_t ion_fd;
    uint32_t offset[SPRD_CAMALG_IMG_CHN_MAX];//offset for y/u/v
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t size;
    void* graphicbuffer_handle;//pointer to GraphicBuffer
    void* reserved[SPRD_CAMALG_DATA_RESERVED_POINTER_NUM];
    uint32_t reserved_int[SPRD_CAMALG_DATA_RESERVED_INT_NUM];
} sprd_camalg_image_t;

#endif
