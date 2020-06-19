
#ifndef SPRD_DFA_H
#define SPRD_DFA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include "sprd_dfa_api.h"
#include <cutils/properties.h>

struct class_dfa {
    DFA_OPTION dfa_option;
    DFA_IMAGE_YUV420 dfa_image;
    DFA_IMAGE_YUV420SP dfa_image_sp;
    DFA_FACEINFO dfa_face[10];
    DFA_IMAGE_BGR dfa_bgr;
    DFA_HANDLE hSprdDfa;
};

void create_dfa_handle(struct class_dfa *dfa, int workMode);
void deinit_dfa_handle(struct class_dfa *dfa);

void construct_dfa_yuv420sp(struct class_dfa *dfa, int picWidth,
                            int picHeight, unsigned char *addrY,
                            unsigned char *addrUV, int format);
void construct_dfa_yuv420(struct class_dfa *dfa, int picWidth,
                          int picHeight, int picYRowStride, int picUVRowStride,
                          int picUVPixelStride,unsigned char *addrY,
                          unsigned char *addrU, unsigned char *addrV);
void construct_dfa_bgr(struct class_dfa *dfa, int picWidth,
                          int picHeight,unsigned char *pBdata,
                          unsigned char *pGdata, unsigned char *pRdata);
void construct_dfa_face(struct class_dfa *dfa, int i, int rX, int rY, int rWidth,
                        int rHeight,int rRollAngle,unsigned char rType);

void do_dfa_image_yuv420sp(struct class_dfa *dfa, int faceCount, DFA_RESULT *dfa_result);
void do_dfa_image_yuv420(struct class_dfa *dfa, int faceCount, DFA_RESULT *dfa_result);
void do_dfa_image_bgr(struct class_dfa *dfa, int faceCount, DFA_RESULT *dfa_result);

#ifdef __cplusplus
}
#endif

#endif /* SPRD_FB_H */