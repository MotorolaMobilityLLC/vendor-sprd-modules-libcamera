#ifndef IBOKEHALGO_H_HEADER
#define IBOKEHALGO_H_HEADER
#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include <sprd_ion.h>
#include "SprdMultiCam3Common.h"

#include "SGM_SPRD.h"
#include "sprdbokeh.h"
#include "iBokeh.h"

namespace sprdcamera {

typedef struct {
    int sel_x;       /* The point which be touched */
    int sel_y;       /* The point which be touched */
    int bokeh_level; // The strength of bokeh region 0~255
    char *config_param;
    bool param_state;
} bokeh_cap_params_t;

typedef struct {
    FACE_Tag face_tag_info;
    uint32_t cur_frame_number;
    int sel_x;    /* The point which be touched */
    int sel_y;    /* The point which be touched */
    int capture_x;
    int capture_y;
    int f_number; // The strength of bokeh region 0~255
    int vcm;
    bool isFocus;
    MRECT face_rect[CAMERA3MAXFACE];
    int face_num;
    struct af_relbokeh_oem_data relbokeh_oem_data;
    struct portrait_mode_param portrait_param;
} bokeh_params;

typedef struct {
    InitParams init_params;
    WeightParams weight_params;
    weightmap_param depth_param;
} bokeh_prev_params_t;

typedef struct { bokeh_cap_params_t cap; } SPRD_BOKEH_PARAM;

typedef union { SPRD_BOKEH_PARAM sprd; } BOKEH_PARAM;

class IBokehAlgo {
  public:
    IBokehAlgo(){};
    virtual ~IBokehAlgo(){};
    virtual int initParam(BokehSize *size, OtpData *data,
                          bool galleryBokeh) = 0;

    virtual void getVersionInfo() = 0;

    virtual void getBokenParam(void *param) = 0;

    virtual void setBokenParam(void *param) = 0;

    virtual void setCapFaceParam(void *param) = 0;

    virtual int prevDepthRun(void *para1, void *para2, void *para3,
                             void *para4) = 0;

    virtual int prevBluImage(sp<GraphicBuffer> &srcBuffer,
                             sp<GraphicBuffer> &dstBuffer, void *param) = 0;

    virtual int initPrevDepth() = 0;

    virtual int deinitPrevDepth() = 0;

    virtual int initAlgo() = 0;

    virtual int deinitAlgo() = 0;

    virtual int setFlag() = 0;

    virtual int initCapDepth() = 0;

    virtual int deinitCapDepth() = 0;

    virtual int capDepthRun(void *para1, void *para2, void *para3, void *para4,
                            int vcmCurValue, int vcmUp, int vcmDown) = 0;

    virtual int capBlurImage(void *para1, void *para2, void *para3, int depthW,
                             int depthH, int mode) = 0;

    virtual int onLine(void *para1, void *para2, void *para3, void *para4) = 0;

    virtual int getGDepthInfo(void *para1, gdepth_outparam *para2) = 0;

    virtual int setUserset(char *ptr, int size) = 0;

    virtual int capPortraitDepthRun(void *para1, void *para2, void *para3,
                                    void *para4, void *input_buf1_addr,
                                    void *output_buf, int vcmCurValue,
                                    int vcmUp, int vcmDown, void *mask) = 0;

    virtual int deinitPortrait() = 0;

    virtual int initPortraitParams(BokehSize *mSize, OtpData *mCalData,
                                   bool galleryBokeh) = 0;

    virtual int initPortraitLightParams() = 0;

    virtual int deinitLightPortrait() = 0;

    virtual void setLightPortraitParam(int param1, int param2, int param3, int param4) = 0;

    virtual void getLightPortraitParam(int *param) = 0;

    virtual int prevLPT(void *input_buff, int picWidth, int picHeight) = 0;

    virtual int capLPT(void *output_buff, int picWidth, int picHeight, 
                        unsigned char *outPortraitMask, int lightPortraitType) = 0;

    virtual int runDFA(void *input_buff, int picWidth, int picHeight, int mode) = 0;

    virtual int doFaceBeauty(unsigned char *mask, void *input_buff,
                         int picWidth, int picHeight, int mode, faceBeautyLevels *facebeautylevel) = 0;

    virtual void setFaceInfo(int *angle, int *pose, int *fd_score) = 0;

    virtual int initFaceBeautyParams() = 0;

    virtual int deinitFaceBeauty() = 0;

    virtual int getPortraitMask(void *para1, void *para2, void *output_buff, void *input_buf1_addr, 
                    int vcmCurValue, unsigned char *result) = 0;

};
}

#endif /* IBOKEHALGO_H_HEADER*/
