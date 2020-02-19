#ifndef __SPRD_FACE_PARAM_H___
#define __SPRD_FACE_PARAM_H___

#ifdef __cplusplus
extern "C" {
#endif

enum {
	FB_SKIN_DEFAULT,
	FB_SKIN_YELLOW,
	FB_SKIN_WHITE,
	FB_SKIN_BLACK,
	FB_SKIN_INDIAN,
	FB_SKIN_NUM
};

struct facebeauty_level_map
{
    unsigned char skinSmoothLevel[11];
    unsigned char skinSmoothDefaultLevel;
    unsigned char skinTextureHiFreqLevel[11];
    unsigned char skinTextureHiFreqDefaultLevel;
    unsigned char skinTextureLoFreqLevel[11];
    unsigned char skinTextureLoFreqDefaultLevel;
    unsigned char skinSmoothRadiusCoeff[11];
    unsigned char skinSmoothRadiusDefaultLevel;
    unsigned char skinBrightLevel[11];
    unsigned char skinBrightDefaultLevel;
    unsigned char largeEyeLevel[11];
    unsigned char largeEyeDefaultLevel;
    unsigned char slimFaceLevel[11];
    unsigned char slimFaceDefaultLevel;
    unsigned char skinColorLevel[11];
    unsigned char skinColorDefaultLevel;
    unsigned char lipColorLevel[11];
    unsigned char lipColorDefaultLevel;
};
struct facebeauty_param_t
{
    unsigned char removeBlemishFlag;
    unsigned char blemishSizeThrCoeff;
    unsigned char skinColorType;
    unsigned char lipColorType;
    struct facebeauty_level_map fb_layer;
};

struct facebeauty_param_cfg_info
{
	struct facebeauty_param_t fb_param[FB_SKIN_NUM];
};

typedef struct facebeauty_param_info {
	struct facebeauty_param_cfg_info cur;
} facebeauty_param_info_t;

#ifdef __cplusplus
}
#endif

#endif /* __SPRD_FACE_PARAM_H__ */
