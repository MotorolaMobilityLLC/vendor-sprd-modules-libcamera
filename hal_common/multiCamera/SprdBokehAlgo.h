#ifndef SPRDBOKEHALGO_H_HEADER
#define SPRDBOKEHALGO_H_HEADER

#include "./spreadst/sprd_depth_configurable_param.h"
#include "IBokehAlgo.h"

namespace sprdcamera {

class SprdBokehAlgo : public IBokehAlgo {
  public:
    SprdBokehAlgo();
    ~SprdBokehAlgo();
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
                     int depthH);

    int onLine(void *para1, void *para2, void *para3, void *para4);

    int getGDepthInfo(void *para1, gdepth_outparam *para2);

    int setUserset(char *ptr, int size);

  private:
    bool mFirstSprdBokeh;
    bool mReadOtp;
    void *mBokehCapHandle;
    void *mBokehDepthPrevHandle;
    void *mDepthCapHandle;
    void *mDepthPrevHandle;
    Mutex mSetParaLock;
    BokehSize mSize;
    OtpData mCalData;
    bokeh_prev_params_t mPreviewbokehParam;
    bokeh_cap_params_t mCapbokehParam;
    SPRD_BOKEH_PARAM mBokehParams;
    int checkDepthPara(struct sprd_depth_configurable_para *depth_config_param);
    void loadDebugOtp();
};
}

#endif /* IBOKEHALGO_H_HEADER*/
