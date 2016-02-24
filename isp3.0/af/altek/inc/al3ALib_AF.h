
#ifndef _ALAFLIB_H_
#define _ALAFLIB_H_
/*alAFLib_port.h*/

#include "mtype.h"

/*the definition is depends on isp (hw3a), please confirm the value defined if isp changed
* MAX_STATS_ROI_NUM: The maxima number of UI setting ROI
* MAX_STATS_ROI_NUM: The maxima number of UI setting ROI
* MAX_STATS_COLUMN_NUM: The maxima column number of the stats
* MAX_STATS_ROW_NUM: The maxima row number of the stats
* ALAF_MAX_STATS_NUM: corresponds to hw3a definition (AL_MAX_AF_STATS_NUM)
* ALAF_MAX_ZONE:  coresp. to (A3CTRL_A_AF_MAX_BLOCKS)
*/

#define ALAF_EXIF_INFO_SIZE (379)
#define ALAF_DEBUG_INFO_SIZE (7168)
#define MAX_STATS_ROI_NUM (5)
#define MAX_STATS_COLUMN_NUM (9)
#define MAX_STATS_ROW_NUM (9)
#define ALAF_MAX_STATS_NUM (MAX_STATS_COLUMN_NUM*MAX_STATS_ROW_NUM)
#define ALAF_MAX_ZONE (9)

/*_alAFLib_time_stamp_t

* time_stamp_sec  time stamp second
* time_stamp_us   time stamp microsecond

*/
typedef struct _alAFLib_time_stamp_t{
        UINT32 time_stamp_sec;
        UINT32 time_stamp_us;
}alAFLib_time_stamp_t;

/*_alAFLib_input_move_lens_cb_t

*device_id: devices ID.
**move_lens_cb: callback function.
*pHandle:    handler from HAL, only have to do is input to callback function.
 lens working status. to info alAFLib.
*/
typedef struct _alAFLib_input_move_lens_cb_t{
        UINT8 ucDevice_id;
        PVOID pHandle;
        BOOL (*move_lens_cb)(PVOID p_handle,INT16 wVCM_dac,UINT8 ucDevice_id);
}alAFLib_input_move_lens_cb_t;

/*alAFLib_lens_status

*LENS_STOP:    lens is in stop status.
*LENS_MOVING:   lens is moving now.
*LENS_MOVE_DONE: lens is move done which change from MOVING.

 lens working status. to info alAFLib.
*/
typedef enum _alAFLib_lens_status{
        LENS_STOP = 1,
        LENS_MOVING,
        LENS_MOVE_DONE,
        LENS_MAX
}alAFLib_lens_status;

/*alAFLib_input_lens_info_t

*lens_pos_updated:true, if lens position is update.
*lens_pos:current position
*lens_status:current lens status
*manual_lens_pos_dac: input lens vcm dac value to alAFLib

 set image buffer to alAFLib. the sub-cam buf only avalible when sub-cam supported.
*/
typedef struct _alAFLib_input_lens_info_t{
        BOOL lens_pos_updated;
        UINT16 lens_pos;
        alAFLib_lens_status lens_status;
        UINT16 manual_lens_pos_dac;
}alAFLib_input_lens_info_t;

/*alAFLib_input_sof_id

*sof_time:   system time when updating sof id.
*sof_frame_id: sof frame id.

 send sof id number and system time at sof updating moment.
*/
typedef struct _alAFLib_input_sof_id_t{
        alAFLib_time_stamp_t sof_time;
        UINT32 sof_frame_id;
}alAFlib_input_sof_t;

/*alAFLib_input_image_buf_t

*frame_ready: true if img is ready.
*img_time: system time when DL image.
*p_main_cam: pointer to image buf from main-cam
*p_sub_cam: pointer to image buf from sub-cam, set to null if sub-cam not support.

 set image buffer to alAFLib. the sub-cam buf only avalible when sub-cam supported.
*/
typedef struct _alAFLib_input_image_buf_t{
        INT32 frame_ready;
        alAFLib_time_stamp_t img_time;
        UINT8 *p_main_cam;
        UINT8 *p_sub_cam;
}alAFLib_input_image_buf_t;

/*alAFLib_input_from_calib_t

*inf_step: VCM inf step
        $macro_step: VCM macro step
        $inf_distance: VCM calibration inf distance in mm.
        $macro_distance: VCM calibration macro distance in mm.
        $mech_top: mechanical limitation top position (macro side) in step.
        $mech_bottom: mechanical limitation bottom position(inf side) in step.
        $lens_move_stable_time: time consume of lens from moving to stable in ms.
        $extend_calib_ptr: extra lib calibraion data, if not support, set to null.
        $extend_calib_data_size: size of extra lib calibration data, if not support set to zero.

 data from OTP must input to alAFLib
*/
typedef struct _alAFLib_input_from_calib_t{
        INT16 inf_step;
        INT16 macro_step;
        INT32 inf_distance;
        INT32 macro_distance;
        INT16 mech_top;
        INT16 mech_bottom;
        UINT8 lens_move_stable_time;
        PVOID extend_calib_ptr;
        INT32 extend_calib_data_size;
}alAFLib_input_from_calib_t;

/*alAFLib_input_module_info_t

 f_number: f-number, ex. f2.0, then input 2.0
 focal_lenth: focal length in um

 the data should settled before initialing.
*/
typedef struct _alAFLib_input_module_info_t{
        FLOAT32 f_number;
        FLOAT32 focal_lenth;
}alAFLib_input_module_info_t;

/*alAFLib_input_init_info_t

 module_info: info of module spec
 calib_data:  calibration data

 the data should settled before initialing.
*/
typedef struct _alAFLib_input_init_info_t{
        alAFLib_input_module_info_t module_info;
        alAFLib_input_from_calib_t calib_data;
}alAFLib_input_init_info_t;

/*HAF third-party calib-param*/
/*if there is external HAF lib*/
typedef struct _alAFLib_input_initial_set_t{
        PVOID p_initial_set_data;
        UINT32 ud_bin_size;
}alAFLib_input_initial_set_t;

/*alAFLib_input_enable_hybrid_t
 enable_hybrid: enable hybrid AF
 type:  hybrid af type.

 haf setting data, if support.
*/
typedef struct _alAFLib_input_enable_hybrid_t{
        BOOL enable_hybrid;
        UINT8 type;
}alAFLib_input_enable_hybrid_t;

/*alAFLib_input_focus_mode_type
*
* input af mode which should be set from HAL control.
* alAFLib_AF_MODE_OFF  : 1 AF turn off
* alAFLib_AF_MODE_AUTO  : 2 Auto mode
* alAFLib_AF_MODE_MACRO   : 3 Macro mode
* alAFLib_AF_MODE_CONTINUOUS_VIDEO   : 4 continues video mode
* alAFLib_AF_MODE_CONTINUOUS_PICTURE : 5 continues liveview picture.
* alAFLib_AF_MODE_EDOF,              : 6 set focus lens position by HAL
* alAFLib_AF_MODE_MANUAL,            : 7 manual mode.
* alAFLib_AF_MODE_HYBRID_AUTO,       : 8 hybrid af auto mode, if haf qualify
* alAFLib_AF_MODE_HYBRID_CONTINUOUS_VIDEO   : 9 hybrid af continues video mode, if haf qualify
* alAFLib_AF_MODE_HYBRID_CONTINUOUS_PICTURE : 10 hybrid af continues liveview picture mode, if haf qualify
* alAFLib_AF_MODE_TUNING,                   : 11 Special mode for tuning.
* alAFLib_AF_MODE_NOT_SUPPORTED,            : 12 not support.
* alAFLib_AF_MODE_MAX
*/
typedef enum _alAFLib_input_focus_mode_type{
        alAFLib_AF_MODE_OFF = 1,
        alAFLib_AF_MODE_AUTO,
        alAFLib_AF_MODE_MACRO,
        alAFLib_AF_MODE_CONTINUOUS_VIDEO,
        alAFLib_AF_MODE_CONTINUOUS_PICTURE,
        alAFLib_AF_MODE_EDOF,
        alAFLib_AF_MODE_MANUAL,
        alAFLib_AF_MODE_HYBRID_AUTO,
        alAFLib_AF_MODE_HYBRID_CONTINUOUS_VIDEO,
        alAFLib_AF_MODE_HYBRID_CONTINUOUS_PICTURE,
        alAFLib_AF_MODE_TUNING,
        alAFLib_AF_MODE_NOT_SUPPORTED,
        alAFLib_AF_MODE_MAX
}alAFLib_input_focus_mode_type;

/*_altek_roi_type

 alAFLib_ROI_TYPE_NORMAL: ROI info for continuous AF.
 alAFLib_ROI_TYPE_TOUCH:  ROI info for touch AF.
 alAFLib_ROI_TYPE_FACE:  Input face detected roi info

 alAFLib support roi type
*/
typedef enum _altek_roi_type{
        alAFLib_ROI_TYPE_NORMAL = 0,
        alAFLib_ROI_TYPE_TOUCH,
        alAFLib_ROI_TYPE_FACE,
}altek_roi_type;

/*alAFLib_roi_t

*uw_top: top-left x position
*uw_left: top-left y position
*uw_dx: crop width
*uw_dy: crop height

 alAFLib roi info.
*/
typedef struct _alAFLib_roi_t{
        UINT16 uw_top;
        UINT16 uw_left;
        UINT16 uw_dx;
        UINT16 uw_dy;
}alAFLib_roi_t;

/*alAFLib_img_t

*uwWidth: img width
*uwHeight: img height

 image info structure.
*/
typedef struct _alAFLib_img_t{
        UINT16 uwWidth;
        UINT16 uwHeight;
}alAFLib_img_t;

/*alAFLib_crop_t

*uwx: top-left x position
*uwy: top-left y position
*dx: crop width
*dy: crop height

 img crop info struct.
*/
typedef struct _alAFLib_crop_t{
        UINT16 uwx;
        UINT16 uwy;
        UINT16 dx;
        UINT16 dy;
}alAFLib_crop_t;

/*alAFLib_input_roi_info_t

*roi_updated: does roi update
*type:    input roi type, refer to altek_roi_type
*frame_id:  frame id correspond to current roi
*num_roi:   number of roi are qualify, max number is MAX_STATS_ROI_NUM.
*roi:     array of roi info, refer to type alAFLib_roi_t
*weight:   weighting for each roi.
*src_img_sz: the source image which is refered by current input roi.

 roi info
*/
typedef struct _alAFLib_input_roi_info_t{
        BOOL roi_updated;
        altek_roi_type type;
        UINT32 frame_id;
        UINT32 num_roi;
        alAFLib_roi_t roi[MAX_STATS_ROI_NUM];
        UINT32 weight[MAX_STATS_ROI_NUM];
        alAFLib_img_t src_img_sz;
}alAFLib_input_roi_info_t;

/*alAFLib_input_sensor_info_t

*preview_img_sz:  size of live view image
*sensor_crop_info:sensor crop image size info.
*actuator_info:  info from actuator, refer to alAFLib_actuator_info_t
 sensor info
*/
typedef struct _alAFLib_input_sensor_info_t{
        alAFLib_img_t preview_img_sz;
        alAFLib_crop_t sensor_crop_info;
}alAFLib_input_sensor_info_t;

/*alAFLib_input_isp_info_t

*liveview_img_sz: size of live view image

 isp info
*/
typedef struct _alAFLib_input_isp_info_t{
        alAFLib_img_t liveview_img_sz;
}alAFLib_input_isp_info_t;

/*alAFLib_input_aec_info_t

*ae_settled: true for ae report converged.
*cur_intensity: current intensity.
*target_intensity: target intensity.
*brightness:  current brightness.
*cur_gain:  gain.
*exp_time:  exposure time.
*preview_fr:preview frame rate in fps.

*/
typedef struct _alAFLib_input_aec_info_t{
        BOOL ae_settled;
        FLOAT32 cur_intensity;
        FLOAT32 target_intensity;
        INT16 brightness;
        FLOAT32 cur_gain;
        FLOAT32 exp_time;
        INT32 preview_fr;
}alAFLib_input_aec_info_t;

/*_alAFLib_input_awb_info_t

*awb_ready: true for awb report ready.
*p_awb_report: pointer to awb report, if not support, point to null.

 pointer awb report which is from awb library.
*/
typedef struct _alAFLib_input_awb_info_t{
        BOOL awb_ready;
        PVOID p_awb_report;
}alAFLib_input_awb_info_t;

/*alAFLib_input_gyro_info_t

 gyro_enable:  true for gyro available.
 gyro_ready:  true for gyro set data ready.
 gyro_value[3]:  gyro value for different direction.

 Gyro data
*/
typedef struct _alAFLib_input_gyro_info_t{
        BOOL gyro_enable;
        BOOL gyro_ready;
        INT32 gyro_value[3];
}alAFLib_input_gyro_info_t;

/*alAFLib_input_gravity_vector_t

 gravity_enable: true for gravity available.
 gravity_ready: true for gravity set data ready.
 g_vactor[3]:  gravity value.

 Gravity data
*/
typedef struct _alAFLib_input_gravity_vector_t{
        BOOL gravity_enable;
        BOOL gravity_ready;
        FLOAT32 g_vactor[3];
} alAFLib_input_gravity_vector_t;

/*alAFLib_hw_stats_t
*      AF_token_id: valid setting number, which same when doing AF configuration, for 3A libs synchronization
*      curr_frame_id: frame id when send
*      hw3a_frame_id: frame id when hw3a compute stats.
*      valid_column_num;  Number of valid column which hw output
*      valid_row_num;      Number of valid row which hw output
*      time_stamp: time when hw3a compute stats. unit: ms
*      fv_hor[ALAF_MAX_STATS_NUM]:  focus value horizontal;
*      fv_ver[ALAF_MAX_STATS_NUM]:  focus value vertical;
*      filter_value1[ALAF_MAX_STATS_NUM]: IIR filter type 1;
*      filter_value2[ALAF_MAX_STATS_NUM]: IIR filter type 2;
*      Yfactor[ALAF_MAX_STATS_NUM]:  Arrays with the sums for the intensity.
*      cnt_hor[ALAF_MAX_STATS_NUM]:  Counts for the horizontal focus value above the threshold
*      cnt_ver[ALAF_MAX_STATS_NUM]:  Counts for the vertical focus value above the threshold
*/

typedef struct _alAFLib_hw_stats_t{
        UINT16 AF_token_id;
        UINT16 curr_frame_id;
        UINT32 hw3a_frame_id;
        UINT8 valid_column_num;
        UINT8 valid_row_num;
        alAFLib_time_stamp_t time_stamp;
        UINT32 fv_hor[ALAF_MAX_STATS_NUM];
        UINT32 fv_ver[ALAF_MAX_STATS_NUM];
        UINT64 filter_value1[ALAF_MAX_STATS_NUM];
        UINT64 filter_value2[ALAF_MAX_STATS_NUM];
        UINT64 Yfactor[ALAF_MAX_STATS_NUM];
        UINT32 cnt_hor[ALAF_MAX_STATS_NUM];
        UINT32 cnt_ver[ALAF_MAX_STATS_NUM];
}alAFLib_hw_stats_t;

/*alAFLib_input_hwaf_info_t
        $hw_stats_ready:        af_stats data from hw is ready
        $max_num_stats:         setting param from hw defined, corrsp. to ALAF_MAX_STATS_NUM
        $max_num_zone:  setting param from hw defined, corrsp. to A3CTRL_A_AF_MAX_BLOCKS
        $max_num_banks:         setting param from hw defined, corrsp. to A3CTRL_A_AF_MAX_BANKS
        $hw_stats:  hw3a stats, refer to alAFLib_hw_stats_t.
*/
typedef struct _alAFLib_input_hwaf_info_t{
        BOOL hw_stats_ready;
        UINT16 max_num_stats;
        UINT8 max_num_zone;
        UINT8 max_num_banks;
        alAFLib_hw_stats_t hw_stats;
}alAFLib_input_hwaf_info_t;

/*alAFLib_input_altune_t

 cbaf_tuning_enable: enable cabf tuning, if necessary.
 scdet_tuning_enable:  enable cabf tuning, if necessary.
 haf_tuning_enable: enable cabf tuning, if necessary.

 p_cbaf_tuning_ptr: linke to cbaf tuning table header, ptr to cbaf tuning table, if enable.
 p_scdet_tuning_ptr: link to scdet tuning table header, ptr to scdet tuning table, if enable.
 p_haf_tuning_ptr: need to include extend tuning table from provider, ptr to extend haf lib tuning table, if enable.

 To set alAF tuning data in.
*/
typedef struct _alAFLib_input_altune_t{
        BOOL cbaf_tuning_enable;
        BOOL scdet_tuning_enable;
        BOOL haf_tuning_enable;
        PVOID p_cbaf_tuning_ptr;
        PVOID p_scdet_tuning_ptr;
        PVOID p_haf_tuning_ptr;
}alAFLib_input_altune_t;

/*alAFLib_input_special_event_type

*alAFLib_ROI_READ_DONE: special event type, reference to alAFLib_input_special_event_type
*alAFLib_AE_IS_LOCK: true for word meaning.
 alAFLib_AF_STATS_CONFIG_UPDATED: After token ID of the input AF stats was the same as true for word meaning.
*/
typedef enum _alAFLib_input_special_event_type{
        alAFLib_ROI_READ_DONE,
        alAFLib_AE_IS_LOCK,
        alAFLib_AF_STATS_CONFIG_UPDATED,
        alAFLib_SPECIAL_MAX
}alAFLib_input_special_event_type;

/*alAFLib_input_special_event_t

*sys_time: input system time when set event.
*type:  special event type, reference to alAFLib_input_special_event_type
*flag:  true for word meaning.
*/
typedef struct _alAFLib_input_special_event{
        alAFLib_time_stamp_t sys_time;
        alAFLib_input_special_event_type type;
        BOOL flag;
}alAFLib_input_special_event_t;

/* alAFLib_set_param_type
 the type of which parameter setin to alAFLib.
 alAFLIB_SET_PARAM_UNKNOWN,  // 0 //         No type input
        alAFLIB_SET_PARAM_SET_CALIBDATA, // 1 //         Set OTP, calibration info and module info from hardware
        alAFLIB_SET_PARAM_SET_SETTING_FILE, // 2 //         send initial af library setting data
        alAFLIB_SET_PARAM_INIT,   // 3 //         Initial alAFLib when open camera, allocate af thread data
        alAFLIB_SET_PARAM_AF_START,  // 4 //         Except CAF, start AF scan process once. TAF
        alAFLIB_SET_PARAM_CANCEL_FOCUS,  // 5 //         Cancel current AF process. Interrupt focusing
        alAFLIB_SET_PARAM_FOCUS_MODE,  // 6 //         Set AF focus mode
        alAFLIB_SET_PARAM_UPDATE_LENS_INFO, // 7 //        when lens status change, update lens info to inform alAFLib.
        alAFLIB_SET_PARAM_RESET_LENS_POS, // 8 //         reset lens position to default.
        alAFLIB_SET_PARAM_SET_LENS_MOVE_CB, // 9 //         Set Lens move callback function when initialization
        alAFLIB_SET_PARAM_SET_MANUAL_FOCUS_DIST,// 10 //         Set Manual focus distance
        alAFLIB_SET_PARAM_SET_ROI,  // 11 //         Set Region of interest info, e.q. face roi, Top- Left x,y position and ROI's width Height
        alAFLIB_SET_PARAM_TUNING_ENABLE, // 12 //         Enable Tuning, the tuning parameter is send into by tuning header alAFLIB_SET_PARAM_UPDATA_TUNING_PTR
        alAFLIB_SET_PARAM_UPDATE_TUNING_PTR, // 13 //         TODO, Update tuning header when enable tuning
        alAFLIB_SET_PARAM_UPDATE_AEC_INFO, // 14 //         Update AEC info to alAFLib
        alAFLIB_SET_PARAM_UPDATE_AWB_INFO, // 15 //         Update AWB indo to alAFLib
        alAFLIB_SET_PARAM_UPDATE_SENSOR_INFO, // 16 //         Update sensor info.
        alAFLIB_SET_PARAM_UPDATE_GYRO_INFO, // 17 //         Update Gyro Info.
        alAFLIB_SET_PARAM_UPDATE_GRAVITY_VECTOR,// 18 //         Update gravity vector, if gravity is avaliable
        alAFLIB_SET_PARAM_UPDATE_ISP_INFO, // 19 //         Update ISP Info
        alAFLIB_SET_PARAM_UPDATE_SOF,  // 20 //         Update Start of frame(SOF) number, if new image comes
        alAFLIB_SET_PARAM_WAIT_FOR_FLASH, // 21 //         Inform alAFLib, it is waiting AE converge
        alAFLIB_SET_PARAM_LOCK_CAF,  // 22 //         To Luck CAF process, when capture, mode change, and so on.
        alAFLIB_SET_PARAM_RESET_CAF,  // 23 //         To reset CAF
        alAFLIB_SET_PARAM_HYBIRD_AF_ENABLE, // 24 //         Enable Hybrid AF
        alAFLIB_SET_PARAM_SET_IMGBUF,  // 25 //         Set Image Buffer into alAFLib
        alAFLIB_SET_PARAM_SET_DEFAULT_SETTING, // 26 //        Set alAFLib to default setting. The current setting will be remove, and no longer to recover until restart camera and load such setting file.
        alAFLIB_SET_PARAM_SPECIAL_EVENT, // 27 //        To inform alAFLib if event change status. Event Type please reference to alAFLib_input_special_event_type
        alAFLIB_SET_PARAM_MAX   // 28
*/
typedef enum _alAFLib_set_param_type{
 alAFLIB_SET_PARAM_UNKNOWN,
        alAFLIB_SET_PARAM_SET_CALIBDATA,
        alAFLIB_SET_PARAM_SET_SETTING_FILE,
        alAFLIB_SET_PARAM_INIT,
        alAFLIB_SET_PARAM_AF_START,
        alAFLIB_SET_PARAM_CANCEL_FOCUS,
        alAFLIB_SET_PARAM_FOCUS_MODE,
        alAFLIB_SET_PARAM_UPDATE_LENS_INFO,
        alAFLIB_SET_PARAM_RESET_LENS_POS,
        alAFLIB_SET_PARAM_SET_LENS_MOVE_CB,
        alAFLIB_SET_PARAM_SET_MANUAL_FOCUS_DIST,
        alAFLIB_SET_PARAM_SET_ROI,
        alAFLIB_SET_PARAM_TUNING_ENABLE,
        alAFLIB_SET_PARAM_UPDATE_TUNING_PTR,
        alAFLIB_SET_PARAM_UPDATE_AEC_INFO,
        alAFLIB_SET_PARAM_UPDATE_AWB_INFO,
        alAFLIB_SET_PARAM_UPDATE_SENSOR_INFO,
        alAFLIB_SET_PARAM_UPDATE_GYRO_INFO,
        alAFLIB_SET_PARAM_UPDATE_GRAVITY_VECTOR,
        alAFLIB_SET_PARAM_UPDATE_ISP_INFO,
        alAFLIB_SET_PARAM_UPDATE_SOF,
        alAFLIB_SET_PARAM_WAIT_FOR_FLASH,
        alAFLIB_SET_PARAM_LOCK_CAF,
        alAFLIB_SET_PARAM_RESET_CAF,
        alAFLIB_SET_PARAM_HYBIRD_AF_ENABLE,
        alAFLIB_SET_PARAM_SET_IMGBUF,
        alAFLIB_SET_PARAM_SET_DEFAULT_SETTING,
        alAFLIB_SET_PARAM_SPECIAL_EVENT,
        alAFLIB_SET_PARAM_MAX
}alAFLib_set_param_type;

/*_alAFLib_input_set_param_t
* To set info to alAFLib, refernce to type
*
* type: set_param type to which you are going to setin alAFLib, refer to alAFLib_set_param_type
* current_sof_id: frame id from sof;
* union{
*  init_info:
*  init_set:
*  afctrl_initialized: is af_ctrl laer initialized.
*  focus_mode_type: alAFLib_input_focus_mode_type
*  lens_info:
*  move_lens:
*  roi_info:
*  sensor_info:
*  isp_info:
*  aec_info:Need double check.
*  awb_info:
*  gyro_info:
*  gravity_info:  g-sensor info
*  alaf_tuning
*  alaf_tuning_enable
*  sof_id:
*  lock_caf:
*  wait_for_flash:
*  special_event:
*  haf_info:
*  img_buf:
* }u_set_data;
*/
typedef struct _alAFLib_input_set_param_t{
        alAFLib_set_param_type type;
        UINT32 current_sof_id;
        union{
        alAFLib_input_init_info_t init_info;
        alAFLib_input_initial_set_t init_set;
        BOOL afctrl_initialized;
        INT32 focus_mode_type;
        alAFLib_input_lens_info_t lens_info;
        alAFLib_input_move_lens_cb_t move_lens;
        alAFLib_input_roi_info_t roi_info;
        alAFLib_input_sensor_info_t sensor_info;
        alAFLib_input_isp_info_t isp_info;
        alAFLib_input_aec_info_t aec_info;
        alAFLib_input_awb_info_t awb_info;
        alAFLib_input_gyro_info_t gyro_info;
        alAFLib_input_gravity_vector_t gravity_info;
        alAFLib_input_altune_t alaf_tuning;
        BOOL alaf_tuning_enable;
        alAFlib_input_sof_t sof_id;
        BOOL lock_caf;
        BOOL wait_for_flash;
        alAFLib_input_special_event_t special_event;
        alAFLib_input_enable_hybrid_t haf_info;
        alAFLib_input_image_buf_t img_buf;
        }u_set_data;
}alAFLib_input_set_param_t;

/* alAFLib_input_get_param_t

*alAFLIB_GET_PARAM_FOCUS_MODE:  get current focus mode
*alAFLIB_GET_PARAM_DEFAULT_LENS_POS: get default lens position (step)
*alAFLIB_GET_PARAM_GET_CUR_LENS_POS: get current lens position (step)
*alAFLIB_GET_PARAM_NOTHING:  set it if you don't need to get info from alAFlib this time.

 info type you want to get from alAFLib.
*/
typedef enum _alAFLib_get_param_type{
        alAFLIB_GET_PARAM_FOCUS_MODE = 0,
        alAFLIB_GET_PARAM_DEFAULT_LENS_POS,
        alAFLIB_GET_PARAM_GET_CUR_LENS_POS,
        alAFLIB_GET_PARAM_NOTHING,
        alAFLIB_GET_PARAM_MAX
}alAFLib_get_param_type;

/* alAFLib_input_get_param_t

*type:   refer to alAFLib_get_param_type.

*default_lens_pos: current lens pos (step).
*af_focus_mode:  current setted focused mode.
*uwdefault_lens_pos:  default lens pos (step)
 To inform upper layer (ex. af_wrapper) current AF status.
 the type is get by get_param.
*/
typedef struct _alAFLib_input_get_param_t{
        alAFLib_get_param_type type;
        UINT32 sof_frame_id;
        union{
                alAFLib_input_focus_mode_type af_focus_mode;
                INT16 wcurrent_lens_pos;
                UINT16 uwdefault_lens_pos;
        }u_get_data;
}alAFLib_input_get_param_t;

/* alAFLib_status_type
*
**alAFLib_STATUS_INVALID : alAF lib invalid.
**alAFLib_STATUS_INIT : alAF lib finished initialization, ready for waitinf trigger.
**alAFLib_STATUS_FOCUSED : alAF lib finished one round focus process.
**alAFLib_STATUS_UNKNOWN : alAF lib is know in unknown status. AF is not busy.
**alAFLib_STATUS_FOCUSING: alAF lib is busy know and doing focus process., the aec should be locked this time.
*
* To inform upper layer (ex. af_wrapper) current AF status.
* the type is get by get_param.
*/
typedef enum{
        alAFLib_STATUS_INVALID = -1,
        alAFLib_STATUS_INIT = 0,
        alAFLib_STATUS_FOCUSED,
        alAFLib_STATUS_UNKNOWN,
        alAFLib_STATUS_FOCUSING
}alAFLib_status_type;

/*alAFLib_af_out_status_t
*focus_done: alAFLib focus process done.
*t_status: refer to alAFLib_status_type.
*f_distance: focus distance in mm this AF round.

 Some information of alAFLib, when receive trigger.
*/
typedef struct _alAFLib_af_out_status_t{
        BOOL focus_done;
        alAFLib_status_type t_status;
        FLOAT32 f_distance;
}alAFLib_af_out_status_t;

/**
$typedef Median_filter_mode
$brief med mode
*/
typedef enum _alAFLib_med_filter_mode{
        MODE_51,
        MODE_31,
        DISABLE,
}alAFLib_med_filter_mode;

/* alAFLib_af_out_roi_t

        $roi:  AF use roi info.
        $src_img_sz: roi reference source img size, which is received from upper layer.
        $num_blk_ver: vertical block number.
        $num_blk_hor: horizontal block number.
        $bEnableAFLUT: enable flag for tone mapping, set false would disable auwAFLUT
        $auwLUT: tone mapping for common 3A, no longer used (default bypassing)
        $auwAFLUT: tone mapping for AF only
        $aucWeight: weight values when calculate the frequency.
        $uwSH:  bit for shift
        $ucThMode: configure 2 modes. Mode 1: First 82 blocks refer to Th, Tv from thelookup array. Mode 2: Every 4 blocks reference one element.
        $aucIndex: locate index in an 82 elements array and reference one of 4 stages Th, Tv.
        $auwTH:  thresholds for calculate the horizontal contrast.
        $pwTV:  thresholds for calculate the vertical contrast.
        $udAFoffset: input data offset
        $bAF_PY_Enable: turn pseudo Y on or off
        $bAF_LPF_Enable:turn low pass filter on or off
        $nFilterMode: median filter mode, suggest use DISABLE
        $ucFilterID: median filter device ID, for AF, use 1
        $uwLineCnt: AF timing control anchor point


 feadback AF current use roi info and block number.

*/
typedef struct _alAFLib_af_out_stats_config_t{
        UINT16 TokenID;
        alAFLib_roi_t roi;
        alAFLib_img_t src_img_sz;
        UINT8 num_blk_ver;
        UINT8 num_blk_hor;
        BOOL bEnableAFLUT;
        UINT16 auwLUT[259];
        UINT16 auwAFLUT[259];
        UINT8 aucWeight[6];
        UINT16 uwSH;
        UINT8 ucThMode;
        UINT8 aucIndex[82];
        UINT16 auwTH[4];
        UINT16 pwTV[4];
        UINT32 udAFoffset;
        BOOL bAF_PY_Enable;
        BOOL bAF_LPF_Enable;
        alAFLib_med_filter_mode nFilterMode;
        UINT8 ucFilterID;
        UINT16 uwLineCnt;
}alAFLib_af_out_stats_config_t;

typedef enum _alAFLib_output_type{
         alAFLIB_OUTPUT_UNKNOW = 1 ,
         alAFLIB_OUTPUT_STATUS =( 1 << 1),
         alAFLIB_OUTPUT_STATS_CONFIG =( 1 << 2),
         alAFLIB_OUTPUT_IMG_BUF_DONE =( 1 << 3),
         alAFLIB_OUTPUT_MOVE_LENS =( 1 << 4),
         alAFLIB_OUTPUT_DEBUG_INFO =( 1 << 5),
         alAFLIB_OUTPUT_MAX
}alAFLib_output_type;

/* alAFLib_output_report_t
* sof_frame_id: sof frame id of this output report.
* result: AFLib result. TRUE : AF control should updated AF report and response; FALSE : Do nothing.
* type:  refer to alAFLib_output_type

* focus_status: AFLib current status. if triggered.
* stats_config: ISP driver configuration for HW AF.
* alaf_debug_data: AF lib debug info data.
* alAF_debug_data_size: AF lib debug data size.
* alAF_EXIF_data: AF lib EXIF info.
* alAF_EXIF_data_size: AF lib EXIF data size.
* wrap_result: returns result, the result define in al3ALib_AF_ErrCode.h
* param_result: param_result returns the error result that only effect AF focus quality but do not make lib crash.
 it often occurs when set / get parameter behavior so naming it as param_result.
*/
typedef struct _alAFLib_output_report_t {
        INT32 sof_frame_id;
        INT32 result;
        alAFLib_output_type type;
        alAFLib_af_out_status_t focus_status;
        alAFLib_af_out_stats_config_t stats_config;
        PVOID p_alAF_debug_data;
        UINT32 alAF_debug_data_size;
        PVOID  p_alAF_EXIF_data;
        UINT32 alAF_EXIF_data_size;
        UINT32 wrap_result;
        UINT16 param_result;
} alAFLib_output_report_t;

////////////////////////////////////////////////////
/********** alAFLib API **************/

/*alAFLib_version_t

*m_uwMainVer: Main version
*m_uwSubVer: Sub version

*/
typedef struct _alAFLib_Version{
        UINT16 m_uwMainVer;
        UINT16 m_uwSubVer;
}alAFLib_version_t;

/*
* callback functions
*/
typedef BOOL (* alAFLib_set_param_func)(alAFLib_input_set_param_t *param,PVOID alAFLib_out_obj, PVOID alAFLib_runtim_obj);
typedef BOOL (* alAFLib_get_param_func)(alAFLib_input_get_param_t *param,PVOID alAFLib_out_obj, PVOID alAFLib_runtim_obj);
typedef BOOL (* alAFLib_process_func)(PVOID alAFLib_hw_stats, PVOID alAFLib_out_obj, PVOID alAFLib_runtim_obj);
typedef PVOID (* alAFLib_intial_func)(PVOID alAFLib_out_obj);
typedef BOOL (* alAFLib_deinit_func)(PVOID alAFLib_runtim_obj, PVOID alAFLib_out_obj);

/* _alAFLib_ops_t
        $initial: initialize alAFLib, when open camera or something else happened.
        $deinit: close alAFLib, when close camera or...etc.
        $set_param: set data to alAFlib.
        $get_param: get info from alAFLib.
        $process:
*/
typedef struct _alAFLib_ops_t{
        alAFLib_intial_func initial;
        alAFLib_deinit_func deinit;
        alAFLib_set_param_func set_param;
        alAFLib_get_param_func get_param;
        alAFLib_process_func process;
}alAFLib_ops_t;

void alAFLib_getLibVer(alAFLib_version_t *alAFLib_ver );
BOOL alAFLib_loadFunc(alAFLib_ops_t *alAFLib_ops);

#endif //end _ALAFLIB_H_
