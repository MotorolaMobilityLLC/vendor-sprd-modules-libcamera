/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef SPRDCAMERA3SINGLEPORTRAIT_H_HEADER
#define SPRDCAMERA3SINGLEPORTRAIT_H_HEADER

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
#include <ui/GraphicBuffer.h>
#include "../SprdCamera3HWI.h"
#include "SprdMultiCam3Common.h"
//#include "ts_makeup_api.h"
#include "SprdCamera3MultiBase.h"
#include "../SprdCamera3Setting.h"
#include "SprdCamera3FaceBeautyBase.h"

#if (defined WIN32 || defined REALVIEW)
#define JNIEXPORT
#else
#include <jni.h>
#endif
#include "../arithmetic/portrait/inc/PortraitCapture_Interface.h"
#include "../arithmetic/lightportrait/inc/camera_light_portrait.h"
#include "../arithmetic/face_dense_align/inc/camera_face_dense_align.h"

#ifdef CONFIG_FACE_BEAUTY
#include "sprd_facebeauty_adapter.h"
#endif

namespace sprdcamera {

//#define YUV_CONVERT_TO_JPEG

#define BLUR_LOCAL_CAPBUFF_NUM (4)
#define BLUR3_REFOCUS_COMMON_PARAM_NUM (12)
#define BLUR_REFOCUS_PARAM2_NUM (11)

#define BLUR_REFOCUS_COMMON_PARAM_NUM (27)
#define BLUR_MAX_NUM_STREAMS (3)
#define BLUR_THREAD_TIMEOUT 50e6
#define BLUR_LIB_BOKEH_PREVIEW "libbokeh_gaussian.so"
#define BLUR_LIB_BOKEH_NUM (2)

#define BLUR_REFOCUS_2_PARAM_NUM (17)
#define BLUR_AF_WINDOW_NUM (9)
#define BLUR_MAX_ROI (10)
#define BLUR_CALI_SEQ_LEN (20)
#define BLUR_GET_SEQ_FROM_AF (0)
#define BLUR_REFOCUS_PARAM_NUM                                                 \
    (BLUR_AF_WINDOW_NUM + BLUR_REFOCUS_2_PARAM_NUM +                           \
     BLUR_REFOCUS_COMMON_PARAM_NUM + BLUR_MAX_ROI * 5 + BLUR_CALI_SEQ_LEN)

#define BLUR_CIRCLE_SIZE_SCALE (3)
#define BLUR_SMOOTH_SIZE_SCALE (8)
#define BLUR_CIRCLE_VALUE_MIN (20)

typedef struct {
    unsigned int af_peak_pos;
    unsigned int near_peak_pos;
    unsigned int far_peak_pos;
    unsigned int distance_reminder;
} single_portrait_isp_info_t;

typedef struct {
    uint32_t frame_number;
    buffer_handle_t *buffer;
    camera3_stream_t *stream;
    camera3_stream_buffer_t *input_buffer;
} request_saved_single_portrait_t;

typedef enum {
    SINGLE_PORTRAIT_MSG_DATA_PROC = 1,
    SINGLE_PORTRAIT_MSG_COMBAIN_PROC,
    SINGLE_PORTRAIT_MSG_EXIT
} singleportraitMsgType;

typedef struct {
    uint32_t frame_number;
    const camera3_stream_buffer_t *input_buffer;
    buffer_handle_t *buffer;
} buffer_combination_single_portrait_t;

typedef struct {
    void *buffer_addr;
    uint32_t frame_number;
    uint32_t use_size;
    uint32_t jpeg_size;
} dump_single_portrait_t;

typedef struct {
    singleportraitMsgType msg_type;
    buffer_combination_single_portrait_t combo_buff;
} single_portrait_queue_msg_t;

typedef enum { SINGLE_PORTRAIT_CAP_NOAI = 0, SINGLE_PORTRAIT_CAP_AI } SINGLE_PORTRAIT_CAPTURE_VERSION;

typedef struct {
    int width;                       // image width
    int height;                      // image height
    float min_slope;                 // 0.001~0.01, default is 0.005
    float max_slope;                 // 0.01~0.1, default is 0.05
    float findex2gamma_adjust_ratio; // 2~11, default is 6.0
    int box_filter_size;
} preview_single_portrait_init_params_t;

typedef struct {
    int roi_type;         // 0: circle 1:rectangle
    int f_number;         // 1 ~ 20
    unsigned short sel_x; /* The point which be touched */
    unsigned short sel_y; /* The point which be touched */
    int circle_size;      // for version 1.0 only
    // for rectangle
    int valid_roi;
    int x1[BLUR_MAX_ROI], y1[BLUR_MAX_ROI]; // left-top point of roi
    int x2[BLUR_MAX_ROI], y2[BLUR_MAX_ROI]; // right-bottom point of roi
    int flag[BLUR_MAX_ROI];                 // 0:face 1:body
} preview_single_portrait_weight_params_t;

typedef struct {
    int width;                       // image width
    int height;                      // image height
    int depthW; // mask depth width
    int depthH; // mask depth height
    int platform_id;                 // defined in SprdCtrl.mk
    float min_slope;                 // 0.001~0.01, default is 0.005
    float max_slope;                 // 0.01~0.1, default is 0.05
    float findex2gamma_adjust_ratio; // 2~11, default is 6.0
    int Scalingratio;                // only support 2,4,6,8
    int SmoothWinSize;               // odd number
    int box_filter_size; // odd number, default: the same as SmoothWinSize
    // below for 2.0 only
    /* Register Parameters : depend on sensor module */
    unsigned short vcm_dac_up_bound;  /* Default value : 0 */
    unsigned short vcm_dac_low_bound; /* Default value : 0 */
    unsigned short *vcm_dac_info;     /* Default value : NULL (Resaved) */
    /* Register Parameters : For Tuning */
    unsigned char vcm_dac_gain;     /* Default value : 16 */
    unsigned char valid_depth_clip; /* The up bound of valid_depth */
    unsigned char method;           /* The depth method. (Resaved) */
    /* Register Parameters : depend on AF Scanning */
    /* The number of AF windows with row (i.e. vertical) */
    unsigned char row_num;
    /* The number of AF windows with row (i.e. horizontal) */
    unsigned char column_num;
    unsigned char boundary_ratio; /*  (Unit : Percentage) */
    /* Customer Parameter */
    /* Range: [0, 16] Default value : 0*/
    unsigned char sel_size;
    /* For Tuning Range : [0, 32], Default value : 4 */
    unsigned char valid_depth;
    unsigned short slope; /* For Tuning : Range : [0, 16] default is 8 */
    unsigned char valid_depth_up_bound;  /* For Tuning */
    unsigned char valid_depth_low_bound; /* For Tuning */
    unsigned short *cali_dist_seq;       /* Pointer */
    unsigned short *cali_dac_seq;        /* Pointer */
    unsigned short cali_seq_len;
} capture_single_portrait_init_params_t;

typedef struct {
    int version;                  // 1~2, 1: 1.0 bokeh; 2: 2.0 bokeh with AF
    int roi_type;                 // // 0: circle 1:rectangle 2:seg
    int f_number;                 // 1 ~ 20
    unsigned short sel_x;         /* The point which be touched */
    unsigned short sel_y;         /* The point which be touched */
    unsigned short *win_peak_pos; /* The seqence of peak position which be
                                     provided via struct isp_af_fullscan_info */
    int circle_size;              // for version 1.0 only
    // for rectangle
    int valid_roi;
    int total_roi;
    int x1[BLUR_MAX_ROI], y1[BLUR_MAX_ROI]; // left-top point of roi
    int x2[BLUR_MAX_ROI], y2[BLUR_MAX_ROI]; // right-bottom point of roi
    int flag[BLUR_MAX_ROI];                 // 0:face 1:body
    int rotate_angle;   // counter clock-wise. 0:face up body down 90:face left
                        // body right 180:face down body up 270:face right body
                        // left  ->
    bool rear_cam_en;   // 1:rear camera capture 0:front camera capture
    short camera_angle; // sensor angle init 270
    short mobile_angle; // hal angle
} capture_single_portrait_weight_params_t;

typedef struct {
    void *handle;
    int (*iSmoothInit)(void **handle, int width, int height, float min_slope,
                       float max_slope, float findex2gamma_adjust_ratio,
                       int box_filter_size);
    int (*iSmoothDeinit)(void *handle);
    int (*iSmoothCreateWeightMap)(void *handle,
                                  preview_single_portrait_weight_params_t *params);
    int (*iSmoothBlurImage)(void *handle, unsigned char *Src_YUV,
                            unsigned char *Output_YUV);
    void *mHandle;
} SinglePortraitAPI_t;

typedef struct {
    cmr_u32 enable;
    cmr_u32 fir_mode;
    cmr_u32 fir_len;
    cmr_s32 hfir_coeff[7];
    cmr_s32 vfir_coeff[7];
    cmr_u32 fir_channel;
    cmr_u32 fir_cal_mode;
    cmr_s32 fir_edge_factor;
    cmr_u32 depth_mode;
    cmr_u32 smooth_thr;
    cmr_u32 touch_factor;
    cmr_u32 scale_factor;
    cmr_u32 refer_len;
    cmr_u32 merge_factor;
    cmr_u32 similar_factor;
    cmr_u32 similar_coeff[3];
    cmr_u32 tmp_mode;
    cmr_s32 tmp_coeff[8];
    cmr_u32 tmp_thr;
} capture2_single_portrait_init_params_t;

typedef struct {
    int f_number;         // 1 ~ 20
    unsigned short sel_x; /* The point which be touched */
    unsigned short sel_y; /* The point which be touched */
} capture2_single_portrait_weight_params_t;

typedef enum {
    DUMP_SINGLE_PORTRAIT_COMBO,  // process start
    DUMP_SINGLE_PORTRAIT_OUTPUT, // preocess end
    DUMP_SINGLE_PORTRAIT_RESULT, // dump image and params
    DUMP_SINGLE_PORTRAIT_TYPE_MAX
} dump_single_portrait_type;

typedef enum {
    CAM_BlUR_MODE= 0,
    CAM_SINGLE_PORTRAIT_MODE = 1
} CameraSinglePortraitMode;

class SprdCamera3SinglePortrait : SprdCamera3MultiBase, SprdCamera3FaceBeautyBase {
  public:
    static void getCameraBlur(SprdCamera3SinglePortrait **pBlur);
    static int camera_device_open(__unused const struct hw_module_t *module,
                                  const char *id,
                                  struct hw_device_t **hw_device);
    static int close_camera_device(struct hw_device_t *device);
    static int get_camera_info(int camera_id, struct camera_info *info);
    static int initialize(const struct camera3_device *device,
                          const camera3_callback_ops_t *ops);
    static int configure_streams(const struct camera3_device *device,
                                 camera3_stream_configuration_t *stream_list);
    static const camera_metadata_t *
    construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *device,
                                       camera3_capture_request_t *request);
    static void notifyMain(const struct camera3_callback_ops *ops,
                           const camera3_notify_msg_t *msg);
    static void
    process_capture_result_main(const struct camera3_callback_ops *ops,
                                const camera3_capture_result_t *result);
    static void dump(const struct camera3_device *device, int fd);
    static int flush(const struct camera3_device *device);
    static camera3_device_ops_t mCameraCaptureOps;
    static camera3_callback_ops callback_ops_main;
    static void
    process_capture_result_aux(const struct camera3_callback_ops *ops,
                               const camera3_capture_result_t *result);
    static void notifyAux(const struct camera3_callback_ops *ops,
                          const camera3_notify_msg_t *msg);
    static camera3_callback_ops callback_ops_aux;

  private:
    sprdcamera_physical_descriptor_t *m_pPhyCamera;
    sprd_virtual_camera_t m_VirtualCamera;
    uint8_t m_nPhyCameras;
    Mutex mLock;
    camera_metadata_t *mStaticMetadata;
    int mCaptureWidth;
    int mCaptureHeight;
    new_mem_t mLocalCapBuffer[BLUR_LOCAL_CAPBUFF_NUM];
    bool mFlushing;
    bool mInitThread;
    List<request_saved_single_portrait_t> mSavedRequestList;
    camera3_stream_t *mSavedReqStreams[BLUR_MAX_NUM_STREAMS];
    int mPreviewStreamsNum;
    Mutex mRequestLock;
    int mjpegSize;
    void *m_pNearYuvBuffer;
    int mNearJpegSize;
    int mFarJpegSize;
    buffer_handle_t *m_pNearJpegBuffer;
    buffer_handle_t *m_pFarJpegBuffer;
    void *weight_map;
    uint8_t mCameraId;
    int32_t mPerfectskinlevel;
    int mCoverValue;
    uint8_t mBlurMode;
    bool mSnapshotResultReturn;
#ifdef CONFIG_FACE_BEAUTY
    faceBeautyLevels fbLevels;
    bool mFaceBeautyFlag;
    bool mFirstSprdFB;
    fb_beauty_param_t fb_prev;
    fb_beauty_param_t fb_cap;
    fbBeautyFacetT beauty_face;
    fb_beauty_image_t beauty_image;
#endif
    int cameraDeviceOpen(int camera_id, struct hw_device_t **hw_device);
    int setupPhysicalCameras();
    int getCameraInfo(int blur_camera_id, struct camera_info *info);
    void freeLocalCapBuffer();
    void saveRequest(camera3_capture_request_t *request);

  public:
    SprdCamera3SinglePortrait();
    virtual ~SprdCamera3SinglePortrait();

    class CaptureThread : public Thread {
      public:
        CaptureThread();
        ~CaptureThread();
        virtual bool threadLoop();
        virtual void requestExit();
        int loadBlurApi();
        void unLoadBlurApi();
        int initBlur20Params();
        int initBlurInitParams();
        void initBlurWeightParams();
        int initPortraitParams();
        int initPortraitLightParams();
        int deinitLightPortrait();
        int prevLPT(void *input_buff, int picWidth, int picHeight);
        int runDFA(void *input_buff, int picWidth, int picHeight, int mode);
        int capLPT(void *output_buff, int picWidth, int picHeight,
                             unsigned char *outPortraitMask);
        int getPortraitMask(void *output_buff, void *input_buf1_addr,
                                      int vcmCurValue, unsigned char *result);
#ifdef CONFIG_FACE_BEAUTY
        int initFaceBeautyParams();
        int deinitFaceBeauty();
        int doFaceBeauty(unsigned char *mask, void *input_buff, int picWidth, int picHeight, 
                    int mode, faceBeautyLevels *facebeautylevel);
#endif
        void updateBlurWeightParams(CameraMetadata metaSettings, int type);
        void saveCaptureBlurParams(buffer_handle_t *result_buff,
                                   uint32_t jpeg_size);
        //        void getOutWeightMap(buffer_handle_t *output_buffer);
        bool yuvReprocessCaptureRequest(buffer_handle_t *combe_buffer,
                                        buffer_handle_t *output_buffer);
        int blurProcessVer1(buffer_handle_t *combo_buffer,
                            void *combo_buff_addr,
                            buffer_handle_t *output_buffer,
                            void *output_buff_addr, uint32_t combo_frm_num);
        void dumpBlurIMG(dump_single_portrait_type type,
                         dump_single_portrait_t *dump_buffs[DUMP_SINGLE_PORTRAIT_TYPE_MAX]);
        int prevBlurHandle(buffer_handle_t *input1, void *input1_addr,
                           void *input2, buffer_handle_t *output,
                           void *output_addr);
        int capBlurHandle(buffer_handle_t *input1, void *input1_addr,
                          void *input2, buffer_handle_t *output,
                          void *output_addr, void *mask);

        void CallSnapBackResult(camera3_buffer_status_t buffer_status);
        // This queue stores matched buffer as frame_matched_info_t
        List<single_portrait_queue_msg_t> mCaptureMsgList;
        Mutex mMergequeueMutex;
        Condition mMergequeueSignal;
        const camera3_callback_ops_t *mCallbackOps;
        sprdcamera_physical_descriptor_t *mDevMain;
        buffer_handle_t *mSavedResultBuff;
        camera3_capture_request_t mSavedCapRequest;
        camera3_stream_buffer_t mSavedCapReqstreambuff;
        camera_metadata_t *mSavedCapReqsettings;
        camera3_stream_t mMainStreams[BLUR_MAX_NUM_STREAMS];
        uint8_t mCaptureStreamsNum;
        SinglePortraitAPI_t *mBlurApi[BLUR_LIB_BOKEH_NUM];
        int mFirstUpdateFrame;
        int mFaceInfoX;
        int mFaceInfoY;
        int mLastMinScope;
        int mLastMaxScope;
        int mLastAdjustRati;
        int mCircleSizeScale;
        int mUpdataxy;
        int64_t mstartUpdate;
        bool mFirstCapture;
        bool mFirstPreview;
        bool mUpdateCaptureWeightParams;
        bool mUpdatePreviewWeightParams;
        unsigned int mBuffSize;
        unsigned int mWeightSize;
        unsigned int mWeightWidth;
        unsigned int mWeightHeight;
        uint8_t mLastFaceNum;
        int mGaussEnable; // when back blur only have blur 1.0 and 1.2
        unsigned short mWinPeakPos[BLUR_AF_WINDOW_NUM];
        preview_single_portrait_init_params_t mPreviewInitParams;
        preview_single_portrait_weight_params_t mPreviewWeightParams;
        preview_single_portrait_weight_params_t mPreviewLptAndBeautyFaceinfo;
        capture_single_portrait_init_params_t mCaptureInitParams;
        capture_single_portrait_weight_params_t mCaptureWeightParams;
        capture2_single_portrait_init_params_t mCapture2InitParams;
        capture2_single_portrait_weight_params_t mCapture2WeightParams;
        int32_t mFaceInfo[4];
        uint32_t mRotation;
        int32_t mLastTouchX;
        int32_t mLastTouchY;
        bool mBlurBody;
        bool mUpdataTouch;
        int mVersion;
        bool mIsGalleryBlur;
        bool mIsBlurAlways;
        single_portrait_isp_info_t mIspInfo;
        unsigned short *mOutWeightBuff;
        bool mAlgorithmFlag;
        bool mFirstSprdLightPortrait;
        bool mFirstSprdDfa;
        int prev_sensor_orientation;
        FACE_Tag faceDetectionInfo;
        int lightPortraitType;
        int cameraBV;
        int cameraISO;
        int cameraCT;
        int lpt_return_val;
        class_lpt lpt_prev;
        class_lpt lpt_cap;
        class_dfa dfa_prev;
        class_dfa dfa_cap;
        lpt_options lptOptions_prev;
        lpt_options lptOptions_cap;
        int32_t fd_score[10];

//        unsigned short *mOutWeightMap;

      private:
        void waitMsgAvailable();
        void BlurFaceMakeup(buffer_handle_t *buffer_handle, void *buffer_addr);
    };
    sp<CaptureThread> mCaptureThread;

    int initialize(const camera3_callback_ops_t *callback_ops);
    int initThread();
    int resetVariablesToDefault();
    int configureStreams(const struct camera3_device *device,
                         camera3_stream_configuration_t *stream_list);
    int processCaptureRequest(const struct camera3_device *device,
                              camera3_capture_request_t *request);
    void notifyMain(const camera3_notify_msg_t *msg);
    void processCaptureResultMain(camera3_capture_result_t *result);
    const camera_metadata_t *
    constructDefaultRequestSettings(const struct camera3_device *device,
                                    int type);
    void _dump(const struct camera3_device *device, int fd);

    int _flush(const struct camera3_device *device);
    int closeCameraDevice();
};
};

#endif /* SPRDCAMERAMU*/
