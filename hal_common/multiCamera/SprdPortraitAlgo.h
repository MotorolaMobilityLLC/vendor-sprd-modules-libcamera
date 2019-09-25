#ifndef SPRDPORTRAITALGO_H_HEADER
#define SPRDPORTRAITALGO_H_HEADER

#include "./spreadst/sprd_depth_configurable_param.h"
#include "IBokehAlgo.h"
#include "../arithmetic/portrait/inc/PortraitCapture_Interface.h"

namespace sprdcamera {

class SprdPortraitAlgo : public IBokehAlgo {
  public:
    SprdPortraitAlgo();
    ~SprdPortraitAlgo();
    int initParam(BokehSize *size, OtpData *data, bool galleryBokeh);

    void getVersionInfo();

    void getBokenParam(void *param);

    void setBokenParam(void *param);

    int prevDepthRun(void *para1, void *para2, void *para3, void *para4);

    int prevBluImage(sp<GraphicBuffer> &srcBuffer, sp<GraphicBuffer> &dstBuffer,
                     void *param);

    int initPrevDepth();

    int deinitPrevDepth();

    int initAlgo();

    int deinitAlgo();

    int setFlag();

    int initCapDepth();

    int deinitCapDepth();

    int capDepthRun(void *para1, void *para2, void *para3, void *para4,
                    int vcmCurValue, int vcmUp, int vcmDown);

    int capBlurImage(void *para1, void *para2, void *para3, int depthW,
                     int depthH, int mode);

    int onLine(void *para1, void *para2, void *para3, void *para4);

    int getGDepthInfo(void *para1, gdepth_outparam *para2);

    int setUserset(char *ptr, int size);

    int capPortraitDepthRun(void *para1, void *para2, void *para3, void *para4,
                            void *input_buf1_addr, void *output_buf,
                            int vcmCurValue, int vcmUp, int vcmDown);
    int deinitPortrait();

    int initPortraitParams(BokehSize *mSize, OtpData *mCalData,
                           bool galleryBokeh);

  private:
    bool mFirstSprdBokeh;
    bool mReadOtp;
    void *mBokehCapHandle;
    void *mBokehDepthPrevHandle;
    void *mDepthCapHandle;
    void *mDepthPrevHandle;
    void *mPortraitHandle;
    Mutex mSetParaLock;
    BokehSize mSize;
    OtpData mCalData;
    bokeh_prev_params_t mPreviewbokehParam;
    bokeh_cap_params_t mCapbokehParam;
    SPRD_BOKEH_PARAM mBokehParams;
    bokeh_params mPortraitCapParam;
    int checkDepthPara(struct sprd_depth_configurable_para *depth_config_param);
    void loadDebugOtp();
};
}

#endif
