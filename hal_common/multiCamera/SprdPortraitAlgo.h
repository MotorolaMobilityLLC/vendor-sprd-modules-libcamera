#ifndef SPRDPORTRAITALGO_H_HEADER
#define SPRDPORTRAITALGO_H_HEADER

#include "./spreadst/sprd_depth_configurable_param.h"
#include "IBokehAlgo.h"
#include "../arithmetic/portrait/inc/PortraitCapture_Interface.h"
#include "../arithmetic/lightportrait/inc/camera_light_portrait.h"
#include "../arithmetic/face_dense_align/inc/camera_face_dense_align.h"
#ifdef CONFIG_FACE_BEAUTY
#include "sprd_facebeauty_adapter.h"
#endif

namespace sprdcamera {

class SprdPortraitAlgo : public IBokehAlgo {
  public:
    SprdPortraitAlgo();
    ~SprdPortraitAlgo();
    int initParam(BokehSize *size, OtpData *data, bool galleryBokeh);

    void getVersionInfo();

    void getBokenParam(void *param);

    void setBokenParam(void *param);

    void setCapFaceParam(void *param);

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
                            int vcmCurValue, int vcmUp, int vcmDown, void *mask);
    int deinitPortrait();

    int initPortraitParams(BokehSize *mSize, OtpData *mCalData,
                           bool galleryBokeh);

    int initPortraitLightParams();

    int deinitLightPortrait();

    void setLightPortraitParam(int param1, int param2, int param3, int param4);

    void getLightPortraitParam(int *param);

    int prevLPT(void *input_buff, int picWidth, int picHeight);

    int capLPT(void *output_buff, int picWidth, int picHeight, 
                        unsigned char *outPortraitMask, int lightPortraitType);

    int runDFA(void *input_buff, int picWidth, int picHeight, int mode);

    int doFaceBeauty(unsigned char *mask, void *input_buff, int picWidth, int picHeight, 
                    int mode, faceBeautyLevels *facebeautylevel);

    void setFaceInfo(int *angle, int *pose, int *fd_score);

    int initFaceBeautyParams();

    int deinitFaceBeauty();

    int getPortraitMask(void *para1, void *para2, void *output_buff, void *input_buf1_addr, 
                    int vcmCurValue, unsigned char *result);

  private:
    bool mFirstSprdBokeh;
    bool mFirstSprdLightPortrait;
    bool mFirstSprdFB;
    bool mFirstSprdDfa;
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

    class_lpt lpt_prev;
    class_dfa dfa_prev;
    class_dfa dfa_cap;
    class_lpt lpt_cap;
    lpt_options lptOptions;
    lpt_options lptOptions_cap;
    int lpt_return_val;
    int32_t Fd_score[10];

#ifdef CONFIG_FACE_BEAUTY
    fb_beauty_param_t fb_prev;
    fb_beauty_param_t fb_cap;
    FACE_Tag face_info;
    faceBeautyLevelsT beautyLevels;
    fbBeautyFacetT beauty_face;
    fb_beauty_image_t beauty_image;
#endif
};
}

#endif
