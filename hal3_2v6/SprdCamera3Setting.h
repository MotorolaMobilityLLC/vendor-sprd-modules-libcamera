/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#ifndef __SPRDCAMERA3_SETTING_H__
#define __SPRDCAMERA3_SETTING_H__

#include <pthread.h>
#include <utils/Mutex.h>
#include <utils/List.h>
#include <utils/KeyedVector.h>
#include <hardware/camera3.h>
#include <CameraMetadata.h>
#include "include/SprdCamera3Tags.h"
#include "SprdCamera3HALHeader.h"
#include "cmr_common.h"
#include <dlfcn.h>
using namespace ::android::hardware::camera::common::V1_0::helper;
using namespace android;

namespace sprdcamera {

#define LOGV ALOGV
#define LOGE ALOGE
#define LOGI ALOGI
#define LOGW ALOGW
#define LOGD ALOGD
#define PRINT_TIME 0
#define ROUND_TO_PAGE(x) (((x) + 0xfff) & ~0xfff)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#endif
#define METADATA_SIZE                                                          \
    (2 * sizeof(unsigned long) + 5 * sizeof(unsigned int)) // 28/* (7 * 4) */
#define SET_PARM(h, x, y, z)                                                   \
    do {                                                                       \
        LOGV("%s: set camera param: %s, %d", __func__, #x, y);                 \
        if (NULL != h && NULL != h->ops)                                       \
            h->ops->camera_set_param(x, y, z);                                 \
    } while (0)
#define SIZE_ALIGN(x) (((x) + 15) & (~15))

#ifndef UNUSED
#define UNUSED(x) (void) x
#endif

#define CAMERA_ID_COUNT 6
#define BLUR_FNUM_COUNT 10

#define MIN_DIGITAL_ZOOM_RATIO (1.0f)

#ifndef CONFIG_ISP_2_3
#define MAX_DIGITAL_ZOOM_RATIO (4.0f)
#else
#define MAX_DIGITAL_ZOOM_RATIO (2.0f)
#endif

#define TELE_MAX_DIGITAL_ZOOM_RATIO (5.0f)

#define MULTI_MAX_DIGITAL_ZOOM_RATIO (10.0f)

/* Time related macros */
typedef int64_t nsecs_t;
#define NSEC_PER_SEC 1000000000LL
#define NSEC_PER_USEC 1000
#define NSEC_PER_33MSEC 33000000LL
#define NSEC_PER_40MSEC 40000000LL
#define MIN_FPS_RANGE 5
#define MAX_FPS_RANGE_BACK_CAM 30
#define MAX_FPS_RANGE_FRONT_CAM 30
#define MIDDLE_FPS_RANGE 20
#define FPS_RANGE_COUNT 20
#define SPRD_SHADING_FACTOR_NUM (2 * 2) //(>1*1*4,<=64*64*4)
#define SPRD_NUM_SHADING_MODES 3        // Shading mode
#define SPRD_NUM_LENS_SHADING_MODES 2   // Shading mode
#define SPRD_MAX_TONE_CURVE_POINT 64    //>=64
#define SPRD_FACE_BEAUTY_PARAM_NUM 9
#ifdef CONFIG_CAMERA_FACE_DETECT
#define SPRD_MAX_AVAILABLE_FACE_DETECT_MODES 2
#else
#define SPRD_MAX_AVAILABLE_FACE_DETECT_MODES 1
#endif

#define THIRD_OTP_SIZE 8192

#define CAMERA_SETTINGS_CONFIG_ARRAYSIZE 240
#define CAMERA_SETTINGS_THUMBNAILSIZE_ARRAYSIZE 14

#define SPRD_3DCALIBRATION_CAPSIZE_ARRAYSIZE 2

#define MAX_PREVIEW_SIZE_WIDTH 1920
#define MAX_PREVIEW_SIZE_HEIGHT 1080

#define EV_EFFECT_FRAME_NUM 3

typedef struct {
    uint8_t correction_mode;
    uint8_t aberration_mode;
    uint8_t available_aberration_modes[3];
    float gains[4];
    camera_metadata_rational_t transform[9];
} COLOR_Tag;

typedef struct {
    uint8_t capture_intent;
    uint8_t available_video_stab_modes[1];
    int32_t max_regions[3];
    uint8_t available_effects[9];
    uint8_t available_scene_modes[18];
    uint8_t mode;
    uint8_t effect_mode;
    uint8_t scene_mode;
    uint8_t video_stabilization_mode;
    uint8_t antibanding_mode;

    uint8_t af_trigger;
    int32_t af_trigger_Id;
    uint8_t af_state;
    uint8_t af_mode;
    int32_t af_regions[5];
    int32_t ot_frame_id;
    uint8_t af_available_modes[6];
    int32_t af_roi[5];

    int32_t ae_available_fps_ranges[FPS_RANGE_COUNT];
    int32_t ae_compensation_range[2];
    uint8_t ae_available_abtibanding_modes[4];
    uint8_t ae_available_modes[5];
    int32_t ae_target_fps_range[2];
    int32_t ae_regions[5];

    uint8_t am_available_modes[4];
    camera_metadata_rational ae_compensation_step;
    int32_t ae_exposure_compensation;
    uint8_t ae_lock;
    uint8_t ae_mode;
    uint8_t ae_abtibanding_mode;
    uint8_t ae_precap_trigger;
    uint8_t ae_manual_trigger;
    uint8_t ae_state;
    int32_t ae_precapture_id;
    uint8_t ae_comp_effect_frames_cnt;

    uint8_t awb_available_modes[9];
    uint8_t awb_lock;
    uint8_t awb_mode;
    uint8_t awb_state;
    int32_t awb_regions[5];

    uint8_t enable_zsl;
} CONTROL_Tag;

typedef struct {
    uint8_t mode;
    uint8_t strength;
    uint8_t available_edge_modes[3];
} EDGE_Tag;

typedef struct {
    uint8_t firing_power;
    uint8_t mode;
    uint8_t state;
} FLASH_Tag;

typedef struct {
    int64_t charge_duration;
    uint8_t available;
} FLASH_INFO_Tag;

typedef struct {
} GEOMETRIC_Tag;

typedef struct { uint8_t mode; } HOT_PIXEL_Tag;

typedef struct {
    uint8_t reduction_mode;
    uint8_t reduction_available_noise_reduction_modes[3];
} NOISE_Tag;

typedef struct {
    float focus_range[4];
    float focus_distance;
    float focal_length;
    uint8_t state;
    uint8_t facing;
    float aperture;
    float f_number;
    float filter_density;
    uint8_t optical_stabilization_mode;
    uint8_t distortion_correction_modes[1];
} LENS_Tag;

typedef struct {
    uint8_t quality;
    uint8_t thumbnail_quality;
    int32_t thumbnail_size[2];
    int32_t orientation_original;
    int32_t orientation;
    int32_t available_thumbnail_sizes[CAMERA_SETTINGS_THUMBNAILSIZE_ARRAYSIZE];
    int32_t max_size;
    double gps_coordinates[3];
    uint8_t gps_processing_method[36];
    int64_t gps_timestamp;
} JPEG_Tag;

typedef struct {
    float mini_focus_distance;
    float hyperfocal_distance;
    float available_focal_lengths;
    float available_apertures;
    float filter_density;
    uint8_t optical_stabilization;
    int32_t shading_map_size[2];
    uint8_t focus_distance_calibration;
} LENS_INFO_Tag;

typedef struct {
    float physical_size[2];
    int64_t exposupre_time_range[2];
    int64_t max_frame_duration;
    uint8_t color_filter_arrangement;
    int32_t pixer_array_size[2];
    int32_t active_array_size[4];
    int32_t white_level;
    int32_t sensitivity_range[2];
    uint8_t timestamp_source;
} SENSOR_INFO_Tag;

typedef struct {
    uint8_t mode;
    uint8_t factor_count;
    float gain_factor[SPRD_SHADING_FACTOR_NUM];
    uint8_t available_lens_shading_map_modes[SPRD_NUM_LENS_SHADING_MODES];
    uint8_t available_shading_modes[SPRD_NUM_SHADING_MODES];
} SHADING_Tag;

typedef struct {
    uint8_t face_detect_mode;
    uint8_t lens_shading_map_mode;
    uint8_t hot_pixel_map_mode;
    uint8_t scene_flicker;
} STATISTICS_Tag;

typedef struct {
} LED_Tag;

typedef struct {
    int32_t max_curve_points;
    uint8_t mode;
    uint8_t available_tone_map_modes[3];
    float curve_blue[SPRD_MAX_TONE_CURVE_POINT];
    float curve_green[SPRD_MAX_TONE_CURVE_POINT];
    float curve_red[SPRD_MAX_TONE_CURVE_POINT];
} TONEMAP_Tag;

typedef struct {
    int32_t max_face_count;
    int32_t histogram_bucket_count;
    int32_t max_histogram_count;
    int32_t sharpness_map_size[2];
    int32_t max_sharpness_map_size;
    uint8_t available_face_detect_modes[SPRD_MAX_AVAILABLE_FACE_DETECT_MODES];
} STATISTICS_INFO_Tag;

typedef struct {
    int64_t raw_min_duration[1];
    int32_t formats[4];
    int32_t processed_size[20];
    int64_t processed_min_durations[1];
    float max_digital_zoom;
    int32_t jpeg_size[20];
    int64_t jpeg_min_durations[2];
    int32_t crop_region[4];
    int32_t
        available_stream_configurations[CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4];
    int64_t min_frame_durations[CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4];
    int64_t stall_durations[CAMERA_SETTINGS_CONFIG_ARRAYSIZE * 4];
    uint8_t cropping_type;
} SCALER_Tag;

typedef struct {
    int64_t rollingShutterSkew;
    int64_t exposure_time;
    int64_t frame_duration;
    int32_t sensitivity;
    int64_t timestamp;
    int32_t orientation;
    int32_t black_level_pattern[4];
    int32_t available_test_pattern_modes[6];
    int32_t max_analog_sensitivity;
    int32_t test_pattern_mode;
} SENSOR_Tag;

typedef struct { uint8_t use_parital_result; } QUIRKS_Tag;

typedef struct {
    int32_t max_num_output_streams[3];
    uint8_t type;
    int32_t id;
    int32_t frame_count;
    int32_t available_characteristics_keys[100];
    int32_t available_request_keys[50];
    int32_t available_result_keys[50];
    uint8_t available_capabilites[5];
    int32_t partial_result_count;
    uint8_t pipeline_max_depth;
    uint8_t pipeline_depth;
    /*Frame number to save request frame for auto tracking*/
    int32_t frame_id;
    uint32_t ot_frame_num;
} REQUEST_Tag;

typedef struct {
    uint8_t availabe_brightness[7];
    uint8_t availabe_contrast[7];
    uint8_t availabe_saturation[7];
    uint8_t availabe_iso[7];
    uint8_t availabe_slow_motion[3];
    uint8_t flash_mode_support;
    uint8_t prev_rec_size_diff_support;
    uint8_t rec_snap_support;
    uint8_t burst_cap_cnt;
    uint8_t capture_mode;
    uint8_t brightness;
    uint8_t ai_scene_enabled;
    uint8_t contrast;
    uint8_t saturation;
    uint8_t slowmotion;
    uint8_t iso;
    uint8_t am_mode;
    uint8_t sensor_orientation;
    int32_t sensor_rotation;
    int32_t am_regions[5];
    int32_t perfect_skin_level[SPRD_FACE_BEAUTY_PARAM_NUM];
    uint8_t sprd_zsl_enabled;
    uint8_t flip_on;
    uint8_t sprd_pipviv_enabled;
    uint8_t sprd_highiso_enabled;
    uint8_t availabe_smile_enable;
    uint8_t sprd_eis_enabled;
    uint8_t availabe_antiband_auto_supported;
    uint8_t is_support_refocus;
    uint8_t refocus_enable;
    uint32_t touchxy[2];
    uint8_t is_macro_fixed;
    uint8_t sprd_3dcalibration_enabled;
    int32_t verification_enable;
    uint32_t sprd_3dcalibration_cap_size[SPRD_3DCALIBRATION_CAPSIZE_ARRAYSIZE];
    uint8_t sprd_burstmode_enable;
    uint8_t sprd_3dcapture_enabled;
    int32_t max_preview_size[2];
    uint8_t sprd_3dnr_enabled;
    uint8_t sprd_auto_3dnr_enable;
    uint8_t sprd_fixedfps_enabled;
    int32_t sprd_appmode_id;
    uint8_t is_takepicture_with_flash;
    uint32_t sprd_filter_type;
    uint8_t sprd_available_flash_level;
    uint8_t sprd_adjust_flash_level;
    uint8_t sprd_auto_hdr_enable;
    uint8_t sprd_auto_fdr_enable;
    uint8_t sprd_is_hdr_scene;
    uint8_t availabe_auto_hdr;
    uint8_t sprd_is_fdr_scene;
    uint8_t availabe_auto_fdr;
    uint8_t sprd_cam_feature_list[CAMERA_SETTINGS_CONFIG_ARRAYSIZE];
    uint8_t sprd_cam_feature_list_size;
    uint8_t availabe_ai_scene;
    uint8_t sprd_ai_scene_type_current;
    uint8_t availabe_sensor_type;
    int32_t device_orietation;
    int32_t ae_info[AE_CB_MAX_INDEX];
    uint8_t availabe_gender_race_age_enable;
    uint8_t gender_race_age_enable;
    int32_t ultrawide_id;
    uint8_t sprd_flash_lcd_mode;
    int32_t top_app_id;
    uint8_t availabe_auto_3dnr;
    uint8_t sprd_is_3dnr_scene;
    uint8_t sprd_is_lowev_scene;
    uint8_t sprd_ot_switch;
    uint8_t is_smile_capture;
    uint8_t sprd_is_logo_watermark;
    uint8_t sprd_is_time_watermark;
    uint8_t available_logo_watermark;
    uint8_t available_time_watermark;
    uint8_t af_support;
    uint8_t af_type;
    uint8_t face_num;
    int32_t face_angle_info[20];
    uint8_t high_resolution_mode;
    uint8_t fin1_highlight_mode;
    uint8_t return_previewframe_after_nozsl_cap;
    int32_t stl3d_id;
    uint8_t sprd_super_macro;
    uint8_t smile_capture_enable;
} SPRD_DEF_Tag;

typedef struct {
    camera_face_t face[10];
    int angle[10];
    int pose[10];
    uint8_t face_num;
    int gender_age_race[10];
    int fd_score[10];
} FACE_Tag;

typedef struct {
    int32_t at_start_info[3];
    int32_t at_cb_info[3];
    int32_t frame_id;
    float w_ratio;
    float h_ratio;
} AUTO_TRACKING_Tag;

typedef struct { int32_t max_latency; } SYNC_Tag;

typedef struct { int32_t crop[4]; } EIS_CROP_Tag;

typedef struct {
    uint8_t otp_data[SPRD_DUAL_OTP_SIZE];
    uint8_t dual_otp_flag;
    uint8_t otp_type;
    int otp_size;
} OTP_Tag;

typedef struct {
    uint8_t otp_data[SPRD_DUAL_OTP_SIZE];
    uint8_t dual_otp_flag;
    uint8_t cal_otp_result;
    int otp_size;
} CAL_OTP_Tag;

typedef struct {
    int32_t vcm_step;
    int32_t vcm_step_for_bokeh;
} VCM_Tag;

typedef struct {
    cmr_s64 prev_timestamp;
    cmr_s64 cap_timestamp;
    cmr_s32 otp_size;
    int32_t fps;
    cmr_s32 cropping_type;
    int32_t vcm_dac;
    int32_t vcm_num;
    int32_t vcm_step;
    int face_type;
} MLOG_Tag;

typedef struct {
    uint16_t vcm_dist_count;
    uint16_t vcm_dist[VCM_DISTANCE_COUNT];
} VCM_DIST_TAG;

enum vcm_result {
    VCM_RESULT_NO = 0x0,
    VCM_RESULT_IN = 0x1,
    VCM_RESULT_DONE = 0x2,
    VCM_RESULT_FAIL = 0x3
};

typedef struct cam_stream_info {
    struct img_size stream_sizes_tbl;
    int64_t stream_min_duration;
    int64_t stream_stall_duration;
} cam_stream_info_t;

typedef struct {
    struct img_size preview_size;
    struct img_size picture_size;
    struct img_size video_size;
    struct img_size callback_size;

    COLOR_Tag colorInfo;
    CONTROL_Tag controlInfo;
    CONTROL_Tag resultInfo;
    EDGE_Tag edgeInfo;
    FLASH_Tag flashInfo;
    FLASH_INFO_Tag flash_InfoInfo;
    GEOMETRIC_Tag geometricInfo;
    HOT_PIXEL_Tag hotpixerInfo;
    JPEG_Tag jpgInfo;
    LENS_Tag lensInfo;
    LENS_INFO_Tag lens_InfoInfo;
    NOISE_Tag noiseInfo;
    QUIRKS_Tag quirksInfo;
    REQUEST_Tag requestInfo;
    SCALER_Tag scalerInfo;
    SENSOR_Tag sensorInfo;
    SENSOR_INFO_Tag sensor_InfoInfo;
    SHADING_Tag shadingInfo;
    STATISTICS_Tag statisticsInfo;
    STATISTICS_INFO_Tag statis_InfoInfo;
    TONEMAP_Tag toneInfo;
    LED_Tag ledInfo;
    FACE_Tag faceInfo;
    FACE_Tag orifaceInfo;
    SYNC_Tag syncInfo;
    meta_info_t metaInfo;
    uint8_t info_supported_hardware_level;
    uint8_t black_level_lock;

    SPRD_DEF_Tag sprddefInfo;

    uint8_t supported_hardware_level;
    uint8_t demosaic_mode;
    EIS_CROP_Tag eiscrop_Info;
    OTP_Tag otpInfo;
    VCM_Tag vcmInfo;
    int32_t vcm_dac[VCM_DISTANCE_COUNT];
    int32_t vcm_num;
    int32_t vcm_result;
    int32_t verification_enable;
    VCM_DIST_TAG vcmDist;
    CAL_OTP_Tag calOtpInfo;
    int32_t hist_report[CAMERA_ISP_HIST_ITEMS];
    AUTO_TRACKING_Tag autotrackingInfo;
    MLOG_Tag mlogInfo;
    int32_t fd_score[10];
} sprd_setting_info_t;
typedef int (*CAMIP_INTERFACE_INIT)(char **);
class SprdCamera3Setting {
  public:
    SprdCamera3Setting(int cameraId);
    virtual ~SprdCamera3Setting();

    static int getSensorStaticInfo(int32_t cameraId);
    static int getLargestSensorSize(int32_t cameraId, cmr_u16 *width,
                                    cmr_u16 *height);
    static int setLargestSensorSize(int32_t cameraId, cmr_u16 width,
                                    cmr_u16 height);
    static int getJpegStreamSize(int32_t cameraId, cmr_u16 width,
                                 cmr_u16 height);
    static int getLargestPictureSize(int32_t cameraId, cmr_u16 *width,
                                     cmr_u16 *height);
    static int getCameraInfo(int32_t cameraId, struct camera_info *cameraInfo);
    static int getNumberOfCameras();
    static int getCameraIPInited();
    static void * getCameraIdentifyState();
    static int initDefaultParameters(int32_t cameraId);
    static int getStaticMetadata(int32_t cameraId,
                                 camera_metadata_t **static_metadata);
    static const char *getVendorSectionName(const vendor_tag_query_ops_t *v,
                                            uint32_t tag);
    static int getVendorTagType(const vendor_tag_query_ops_t *v, uint32_t tag);
    static const char *getVendorTagName(const vendor_tag_query_ops_t *v,
                                        uint32_t tag);
    static int getVendorTagCnt(const vendor_tag_query_ops_t *v);
    static void getVendorTags(const vendor_tag_query_ops_t *v,
                              uint32_t *tag_array);

    /*HAL 3.2*/
    static int get_tag_count(const vendor_tag_ops_t *ops);
    static void get_all_tags(const vendor_tag_ops_t *ops, uint32_t *tag_array);
    static const char *get_section_name(const vendor_tag_ops_t *ops,
                                        uint32_t tag);
    static const char *get_tag_name(const vendor_tag_ops_t *ops, uint32_t tag);
    static int get_tag_type(const vendor_tag_ops_t *ops, uint32_t tag);
    static int32_t stream_limit(const cam_stream_info_t *p, int32_t total,
                                int32_t w_limit, int32_t h_limit);
    /* get max capture size by sensor max size */
    static int getMaxCapSize(int32_t cameraId, int32_t *w, int32_t *h);
    /* for high resolution, return point */
    static int getHighResCapSize(int32_t cameraId, const struct img_size **pRet);
    /* for 4in1 sensor auto mode, return size */
    static int getHighResBinCapSize(int32_t cameraId, struct img_size *pRet,
                                    struct img_size sensor_max);

    int constructDefaultMetadata(int type, camera_metadata_t **metadata);
    int UpdateWorkParameters(const CameraMetadata &frame_settings);
    int initialize(const camera3_callback_ops_t *callback_ops);
    camera_metadata_t *translateLocalToFwMetadata();
    int updateWorkParameters(const CameraMetadata &frame_settings);
    int popAndroidParaTag();
    int popSprdParaTag();
    void releaseAndroidParaTag();

    int setPreviewSize(struct img_size size);
    int getPreviewSize(struct img_size *size);

    int setPictureSize(struct img_size size);
    int getPictureSize(struct img_size *size);

    int setCallbackSize(struct img_size size);
    int getCallbackSize(struct img_size *size);

    int setVideoSize(struct img_size size);
    int getVideoSize(struct img_size *size);

    int setCOLORTag(COLOR_Tag colorInfo);
    int getCOLORTag(COLOR_Tag *colorInfo);

    int setCONTROLTag(CONTROL_Tag *controlInfo);
    int getCONTROLTag(CONTROL_Tag *controlInfo);

    int setResultTag(CONTROL_Tag *resultInfo);
    int getResultTag(CONTROL_Tag *resultInfo);

    int setAeCONTROLTag(CONTROL_Tag *controlInfo);
    int setAfCONTROLTag(CONTROL_Tag *controlInfo);
    int setAwbCONTROLTag(CONTROL_Tag *controlInfo);
    int setAfRoiCONTROLTag(CONTROL_Tag *controlInfo);

    int setEDGETag(EDGE_Tag edgeInfo);
    int getEDGETag(EDGE_Tag *edgeInfo);

    int setLENSTag(LENS_Tag lensInfo);
    int getLENSTag(LENS_Tag *lensInfo);

    int setJPEGTag(JPEG_Tag jpgInfo);
    int getJPEGTag(JPEG_Tag *jpgInfo);

    int setLENSINFOTag(LENS_INFO_Tag lens_InfoInfo);
    int getLENSINFOTag(LENS_INFO_Tag *lens_InfoInfo);

    int setSENSORINFOTag(SENSOR_INFO_Tag sensor_InfoInfo);
    int getSENSORINFOTag(SENSOR_INFO_Tag *sensor_InfoInfo);

    int setFLASHTag(FLASH_Tag flashInfo);
    int getFLASHTag(FLASH_Tag *flashInfo);

    int setFLASHINFOTag(FLASH_INFO_Tag flash_InfoInfo);
    int getFLASHINFOTag(FLASH_INFO_Tag *flash_InfoInfo);
    int androidAeModeToDrvAeMode(uint8_t androidAeMode, int8_t *convertDrvMode);
    int androidFlashModeToDrvFlashMode(uint8_t androidFlashMode,
                                       int8_t *convertDrvMode);
    int androidSceneModeToDrvMode(uint8_t androidScreneMode,
                                  int8_t *convertDrvMode);
    int androidIsoToDrvMode(int32_t androidIso, int8_t *convertDrvMode);
    int androidEffectModeToDrvMode(uint8_t androidEffectMode,
                                   int8_t *convertDrvMode);
    int androidAwbModeToDrvAwbMode(uint8_t androidAwbMode,
                                   int8_t *convertDrvMode);
    int
    androidAntibandingModeToDrvAntibandingMode(uint8_t androidAntibandingMode,
                                               int8_t *convertAntibandingMode);

    int androidAfModeToDrvAfMode(uint8_t androidAfMode, int8_t *convertDrvMode);
    int flashLcdModeToDrvFlashMode(uint8_t flashLcdMode,
                                   int8_t *convertDrvMode);
    int setTONEMAPTag(TONEMAP_Tag *toneInfo);
    int getTONEMAPTag(TONEMAP_Tag *toneInfo);

    int setSTATISTICSINFOTag(STATISTICS_INFO_Tag statis_InfoInfo);
    int getSTATISTICSINFOTag(STATISTICS_INFO_Tag *statis_InfoInfo);

    int setSCALERTag(SCALER_Tag *scalerInfo);
    int getSCALERTag(SCALER_Tag *scalerInfo);

    int setQUIRKSTag(QUIRKS_Tag quirksInfo);
    int getQUIRKSTag(QUIRKS_Tag *quirksInfo);

    int setREQUESTTag(REQUEST_Tag *requestInfo);
    int getREQUESTTag(REQUEST_Tag *requestInfo);

    int setSPRDDEFTag(SPRD_DEF_Tag sprddefInfo);
    int getSPRDDEFTag(SPRD_DEF_Tag *sprddefInfo);
    SPRD_DEF_Tag *getSPRDDEFTagPTR(void);

    int setGEOMETRICTag(GEOMETRIC_Tag geometricInfo);
    int getGEOMETRICTag(GEOMETRIC_Tag *geometricInfo);

    int setHOTPIXERTag(HOT_PIXEL_Tag hotpixerInfo);
    int getHOTPIXERTag(HOT_PIXEL_Tag *hotpixerInfo);

    int setNOISETag(NOISE_Tag noiseInfo);
    int getNOISETag(NOISE_Tag *noiseInfo);

    int setSENSORTag(SENSOR_Tag sensorInfo);
    int getSENSORTag(SENSOR_Tag *sensorInfo);

    int setExposureTimeTag(int64_t exposureTime);

    int setSHADINGTag(SHADING_Tag shadingInfo);
    int getSHADINGTag(SHADING_Tag *shadingInfo);

    int setSTATISTICSTag(STATISTICS_Tag statisticsInfo);
    int getSTATISTICSTag(STATISTICS_Tag *statisticsInfo);

    int setLEDTag(LED_Tag ledInfo);
    int getLEDTag(LED_Tag *ledInfo);

    int setFACETag(FACE_Tag *faceInfo);
    int setORIFACETag(FACE_Tag *faceInfo);

    int getFACETag(FACE_Tag *faceInfo);

    int setEISCROPTag(EIS_CROP_Tag eiscrop_Info);
    int getEISCROPTag(EIS_CROP_Tag *eiscrop_Info);

    int setMETAInfo(
        meta_info_t metaInfo); /*   for some metadata,   result meta should be
                                  corresponding with request meta */
    int getMETAInfo(meta_info_t *metaInfo);

    int setOTPTag(OTP_Tag *otpInfo);
    int setOTPTag(OTP_Tag *otpInfo, int size, uint8_t type);
    int getOTPTag(OTP_Tag *otpInfo);

    int setVCMTag(VCM_Tag vcmInfo);
    int getVCMTag(VCM_Tag *vcmInfo);
    int clearMLOGTag();
    int getMLOGTag(MLOG_Tag **mlogInfo);
    int setVCMDACTag(uint16_t *vcmInfo, uint8_t num);
    int getVCMDISTTag(VCM_DIST_TAG *vcmDist);
    int getCALOTPTag(CAL_OTP_Tag *calOTP);
    int setCALOTPRETag(uint8_t calOtpResult);
    int setVCMDISTTag(VCM_DIST_TAG *vcmDist);
    int setVCMRETag(int32_t result);
    int getVCMRETag(int32_t *result);
    int getVERIFITag();
    int setVERIFITag(int32_t veri_enable);
    int setHISTOGRAMTag(int32_t *hist_report);
    int setAUTOTRACKINGTag(AUTO_TRACKING_Tag *autotrackingInfo);
    int getAUTOTRACKINGTag(AUTO_TRACKING_Tag *autotrackingInfo);
    int getSensorFov(float *w_fov,float  *sw_fov);
    int setFdScore(int32_t *fd_score, int num);
    int getFdScore(int32_t *fd_score, int num);


    static uint8_t mMaxCameraCount;
    static camera_metadata_t *mStaticMetadata[CAMERA_ID_COUNT];
    camera_metadata_t *mDefaultMetadata[CAMERA3_TEMPLATE_COUNT];
    static sprd_setting_info_t s_setting[CAMERA_ID_COUNT];
    static CameraMetadata mStaticInfo[CAMERA_ID_COUNT];
    static uint8_t mSensorFocusEnable[CAMERA_ID_COUNT];
    static uint8_t mSensorType[CAMERA_ID_COUNT];
    static uint16_t mModuleId[CAMERA_ID_COUNT];
    static char mSensorName[CAMERA_ID_COUNT][SENSOR_NAME_LEN];
    static uint16_t mpdaf_type[CAMERA_ID_COUNT];
    static int mPhysicalSensorNum;
    static int mLogicalSensorNum;
    static uint8_t camera_identify_state[CAMERA_ID_COUNT];

  private:
    void pushAndroidParaTag(camera_metadata_tag_t tag);
    void pushAndroidParaTag(sprd_camera_metadata_tag_t tag);

    Mutex mLock;
    List<camera_metadata_tag_t> mParaChangedTagQueue;
    List<sprd_camera_metadata_tag_t> mSprdParaChangedTagQueue;
    uint8_t mCameraId;

    static int parse_int(const char *str, int *data, char delim,
                         char **endptr = NULL);
    static int parse_float(const char *str, float *data, char delim,
                           char **endptr = NULL);
    static int parse_string(char *str, char *data, char delim,
                            char **endptr = NULL);
    static void parseStringInt(const char *sizesStr, int *fdStr);
    static void parseStringfloat(const char *sizesStr, float *fdStr);

    static int setDefaultParaInfo(int32_t cameraId);
    static bool getLcdSize(uint32_t *width, uint32_t *height);
    //
    static int initStaticParametersforSensorInfo(int32_t cameraId);
    static int initStaticParametersforLensInfo(int32_t cameraId);
    static int initStaticParametersforScalerInfo(int32_t cameraId);
    //
    static int initStaticMetadataforSensorInfo(int32_t cameraId,
                                               CameraMetadata &staticInfo);
    static int initStaticMetadataforLensInfo(int32_t cameraId,
                                             CameraMetadata &staticInfo);
    static int initStaticMetadataforScalerInfo(int32_t cameraId,
                                               CameraMetadata &staticInfo);
    //
    static int initStaticParameters(int32_t cameraId);
    static int initStaticMetadata(int32_t cameraId,
                                  camera_metadata_t **static_metadata);
    static void convertToRegions(int32_t *rect, int32_t *region, int weight);
    static int checkROIValid(int32_t *roi_area, int32_t *crop_area);
    static int checkTouchAreaUpdate(int32_t *am_area, int32_t *touch_area);
    void coordinate_struct_convert(int *rect_arr, int arr_size);
    int coordinate_convert(int *rect_arr, int arr_size, int angle,
                           int is_mirror, struct img_size *preview_size,
                           struct img_rect *preview_rect);
    static int GetFovParam(int32_t cameraId);
    bool isFaceBeautyOn(SPRD_DEF_Tag *sprddefInfo);
    void autotrackingCoordinateConvert(int32_t *area);
    static int resetFeatureStatus(const char* fea_ip,const char* fea_eb);
};

}; // namespace sprdcamera

#endif
