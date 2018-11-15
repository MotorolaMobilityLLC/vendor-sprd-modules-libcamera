#ifndef ARCSOFTBOKEHALGO_H_HEADER
#define ARCSOFTBOKEHALGO_H_HEADER

#include "IBokehAlgo.h"

#define ARCSOFT_CALIB_DATA_SIZE (2048)
#define ARCSOFT_DEPTH_DATA_SIZE (561616)

namespace sprdcamera {

class ArcSoftBokehAlgo : public IBokehAlgo {
  public:
    ArcSoftBokehAlgo();
    ~ArcSoftBokehAlgo();
    int initParam(BokehSize *size, OtpData *data);

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

    int capDepthRun(void *para1, void *para2, void *para3, void *para4);

    int capBlurImage(void *para1, void *para2, void *para3, int depthW, int depthH);

    int onLine(void *para1, void *para2, void *para3i, void *para4);

  private:
    bool mFirstArcBokeh;
    bool mGallery;
#ifdef CONFIG_ALTEK_ZTE_CALI
    bool mReadZteOtp;
    int mVcmSteps;
#endif
    bool mReadOtp;
    Mutex mSetParaLock;
    BokehSize mSize;
    OtpData mCalData;
    MHandle mPrevHandle;
    MHandle mCapHandle;
    ArcParam mArcParam;
    MInt32 mDepthSize;
    MVoid *mDepthMap;
    ARC_DC_CALDATA mData;
    ARC_DCVR_PARAM mPrevParam;
    ARC_DCIR_REFOCUS_PARAM mCapParam;
    ARC_BOKEH_PARAM mBokehParams;
    ARC_DCIR_PARAM mDcrParam;
    ARC_REFOCUSCAMERAIMAGE_PARAM mArcSoftInfo;
    MVoid *mArcSoftDepthMap;
    MInt32 mArcSoftDepthSize;
    char mArcSoftCalibData[THIRD_OTP_SIZE];
#ifdef CONFIG_ALTEK_ZTE_CALI
    int createArcSoftCalibrationData(unsigned char *pBuffer, int nBufSize);
#endif
    void loadDebugOtp();
};
}

#endif /* ARCSOFTIBOKEHALGO_H_HEADER*/
