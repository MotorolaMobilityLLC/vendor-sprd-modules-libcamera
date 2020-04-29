#ifndef __SPRD_FACEBEAUTY_ADAPTER_H__
#define __SPRD_FACEBEAUTY_ADAPTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "sprdfacebeauty.h"
#include "faceparam.h"
#include "sprd_fb_vdsp_api.h"
#include <cutils/properties.h>
#include "sprd_camalg_adapter.h"

#define JNIEXPORT  __attribute__ ((visibility ("default")))

typedef enum {
    FB_BEAUTY_GET_VERSION_CMD = 0,
    FB_BEAUTY_CONSTRUCT_FACE_CMD,
    FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
    FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
    FB_BEAUTY_CONSTRUCT_MASK_CMD,
    FB_BEAUTY_PROCESS_CMD,
    FB_BEAUTY_FAST_STOP_CMD,
    FB_BEAUTY_CONSTRUCT_FACEMAP_CMD,
    FB_BEAUTY_MAX_CMD
} fb_beauty_cmd_t;

typedef struct {
    const char *pVersion;
} fb_beauty_version_t;

typedef struct {
    int faceCount;
    unsigned char lightPortraitType;
}fb_beauty_lptparam_t;

typedef struct {
    int idx;
    int startX;
    int startY;
    int endX;
    int endY;
    int angle;
    int pose;
    int score;
    unsigned char faceAttriRace;    /* Skin color of race: yellow, white, black, or indian        */
    unsigned char faceAttriGender;  /* Gender from face attribute detection demo */
    unsigned char faceAttriAge;     /* Age from face attribute detection demo    */
} fbBeautyFacetT;

typedef struct {
    FB_PORTRAITMASK fb_mask;
}fb_beauty_mask_t;

typedef struct {
    struct sprd_camalg_image inputImage;
} fb_beauty_image_t;

typedef struct fb_beauty_param {
    int fb_mode;
    int firstFrm;
    int noFaceFrmCnt;
    int isFaceGot;
    char sprdAlgorithm[PROPERTY_VALUE_MAX];
    FB_BEAUTY_OPTION_VDSP fb_option;
    FB_IMAGE_YUV420SP_VDSP fb_image;
    FB_FACEINFO_VDSP fb_face[10];
    FB_BEAUTY_HANDLE hSprdFB;
    enum camalg_run_type runType;
}fb_beauty_param_t;

typedef struct faceBeautyLevels {
    unsigned char blemishLevel; /* Flag for removing blemish; 0 --> OFF; 1 --> ON    */
    unsigned char smoothLevel; /* Smooth skin level. Value range [0, 10] */
    unsigned char skinColor; /* The target skin color: white, rosy, or wheat */
    unsigned char skinLevel; /* The level to tune skin color. Value range [0, 10] */
    unsigned char brightLevel; /* Skin brightness level. Value range [0, 10] */
    unsigned char lipColor; /* The target lip color: crimson, pink or fuchsia */
    unsigned char lipLevel; /* Red lips level. Value range [0, 10] */
    unsigned char slimLevel;  /* Slim face level. Value range [0, 10] */
    unsigned char largeLevel; /* Enlarge eye level. Value range [0, 10] */
    int cameraWork; /* The work camera; front or rear*/
    int cameraBV; /* The value of bv for judjing ambient brightness */
    int cameraISO; /* The value of iso for judjing light sensitivity */
    int cameraCT; /* The value of ct for judjing color temperature */
} faceBeautyLevelsT;

JNIEXPORT void face_beauty_init(fb_beauty_param_t *faceBeauty, int workMode, int threadNum, fb_chipinfo chipinfo);
JNIEXPORT void face_beauty_deinit(fb_beauty_param_t *faceBeauty);
/*JNIEXPORT fb_beauty_mask_t *get_fb_mask();*/
/*
	fb adapter cmd process interface
	return value: 0 is ok, other value is failed
	@param: depend on cmd type:
		- FB_BEAUTY_GET_VERSION_CMD:    fb_beauty_version_t
		- FB_BEAUTY_CONSTRUCT_FACE_CMD: fb_beauty_face_t
                - FB_BEAUTY_CONSTRUCT_IMAGE_CMD:fb_beauty_image_t
		- FB_BEAUTY_CONSTRUCT_LEVEL_CMD:face_beauty_levels_t
                - FB_BEAUTY_PROCESS_CMD:        int faceCount
*/
JNIEXPORT int face_beauty_ctrl(fb_beauty_param_t *faceBeauty, fb_beauty_cmd_t cmd, void *param);

/*
	hdr adapter get running type, the output is type, such as cpu/gpu/vdsp
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int face_beauty_get_devicetype(fb_beauty_param_t *faceBeauty,enum camalg_run_type *type);

/*
	hdr adapter set running type
	return value: 0 is ok, other value is failed
*/
JNIEXPORT int face_beauty_set_devicetype(fb_beauty_param_t *faceBeauty, enum camalg_run_type type);

#ifdef __cplusplus
}
#endif

#endif /* __SPRD_FACEBEAUTY_ADAPTER_H__ */
