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

#include "./arcsoft/AISFCommonDef.h"
#include "./arcsoft/AISFReferenceInter.h"
#include "./arcsoft/arcsoft_dualcam_common_refocus.h"
#include "./arcsoft/amcomdef.h"
#include "./arcsoft/ammem.h"
#include "./arcsoft/asvloffscreen.h"
#include "./arcsoft/merror.h"
#include "./arcsoft/arcsoft_dualcam_video_refocus.h"
#include "./arcsoft/arcsoft_dualcam_image_refocus.h"
#include "./arcsoft/altek/arcsoft_calibration_parser.h"
#include "./arcsoft/arcsoft_configurable_param.h"

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
    int sel_x;    /* The point which be touched */
    int sel_y;    /* The point which be touched */
    int f_number; // The strength of bokeh region 0~255
    int vcm;
    bool isFocus;
    MRECT face_rect[CAMERA3MAXFACE];
    int face_num;
} bokeh_params;

typedef struct {
    InitParams init_params;
    WeightParams weight_params;
    weightmap_param depth_param;
} bokeh_prev_params_t;

typedef struct {
    ARC_DCVR_PARAM prev;
    ARC_DCIR_REFOCUS_PARAM cap;
    ARC_DCIR_PARAM dcr;
    MInt32 size;
    MVoid *map;
} ARC_BOKEH_PARAM;

typedef struct { bokeh_cap_params_t cap; } SPRD_BOKEH_PARAM;

typedef union {
    ARC_BOKEH_PARAM arc;
    SPRD_BOKEH_PARAM sprd;
} BOKEH_PARAM;

class IBokehAlgo {
  public:
    IBokehAlgo(){};
    virtual ~IBokehAlgo(){};
    virtual int initParam(BokehSize *size, OtpData *data, bool galleryBokeh) = 0;

    virtual void getVersionInfo() = 0;

    virtual void getBokenParam(void *param) = 0;

    virtual void setBokenParam(void *param) = 0;

    virtual int prevDepthRun(void *para1, void *para2, void *para3, void *para4) = 0;

    virtual int prevBluImage(sp<GraphicBuffer> &srcBuffer,
                             sp<GraphicBuffer> &dstBuffer, void *param) = 0;

    virtual int initPrevDepth() = 0;

    virtual int deinitPrevDepth() = 0;

    virtual int initAlgo() = 0;

    virtual int deinitAlgo() = 0;

    virtual int setFlag() = 0;

    virtual int initCapDepth() = 0;

    virtual int deinitCapDepth() = 0;

    virtual int capDepthRun(void *para1, void *para2, void *para3,
                            void *para4) = 0;

    virtual int capBlurImage(void *para1, void *para2, void *para3, int depthW, int depthH) = 0;

    virtual int onLine(void *para1, void *para2, void *para3, void *para4) = 0;
};
}

#endif /* IBOKEHALGO_H_HEADER*/
