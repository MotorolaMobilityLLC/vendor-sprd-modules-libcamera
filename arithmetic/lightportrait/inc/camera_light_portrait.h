
#ifndef SPRD_LPT_H
#define SPRD_LPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "lightportraitapi.h"
#include <cutils/properties.h>

struct class_lpt {
    int lpt_mode;
    int firstFaceFrm;
    int noFaceFrmCnt;
    int isFaceGot;
    LPT_PORTRAIT_OPTION lpt_option;
    LPT_IMAGE_YUV420SP lpt_image;
    LPT_PORTRAITMASK lpt_mask;
    LPT_FACEINFO lpt_face[10];
    LPT_DFAINFO lpt_dfa;
    LPT_PORTRAIT_HANDLE hSprdLPT;
};
/* Light portrait options */
struct lpt_options {
    unsigned char lightPortraitType;       /* The target light portrait type: no lighting, soft lighting, butterfly lighting, split lighting, contour lighting, stage lighting or classic lighting */
    unsigned char lightCursor;             /* Control fill light and shade ratio. Value range [0, 35]           */
    unsigned char lightWeight;             /* Control fill light intensity. Value range [0, 20]                 */
    unsigned char lightSplitMode;          /* The LightSplit direction. Value: LPT_LIGHTSPLIT_RIGHT or LPT_LIGHTSPLIT_LEFT    */
    unsigned char debugMode;               /* Debug mode. 0 --> OFF; 1 --> ON                       */
    int cameraWork;                        /* The work camera; front or rear                        */
    int cameraBV;                          /* The value of bv for judjing ambient brightness        */
    int cameraISO;                         /* The value of iso for judjing light sensitivity        */
    int cameraCT;                          /* The value of ct for judjing color temperature         */
};

void init_lpt_options (struct class_lpt *lpt);
void create_lpt_handle(struct class_lpt *lpt, int workMode, int threadNum);
void deinit_lpt_handle(struct class_lpt *lpt);
void construct_lpt_options(struct class_lpt *lpt, struct lpt_options lptOptions);
void construct_lpt_mask(struct class_lpt *lpt, int pWidth, int pHeight, unsigned char *pData);
void construct_lpt_image(struct class_lpt *lpt, int picWidth,
                        int picHeight, unsigned char *addrY,
                        unsigned char *addrU, int format);
void construct_lpt_face(struct class_lpt *lpt, int j, int sx, int sy,
                       int ex, int ey, int yaw, int roll, int fScore,
                       unsigned char attriRace, unsigned char attriGender, unsigned char attriAge);
void construct_lpt_dfaInfo(struct class_lpt *lpt, float picPitch, float picYaw,
                          float picRoll, float pitT3d[],int tLen, float picScale, float picR[][3],int rLen,float picAlpha_shp[],int sLen, float picAlpha_exp[],int eLen);
int do_image_lpt(struct class_lpt *lpt, int faceCount);

#ifdef __cplusplus
}
#endif

#endif