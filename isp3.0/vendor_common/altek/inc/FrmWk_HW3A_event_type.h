/*
 * ////////////////////////////////////////////////////////////////////
 * //  File name: FrmWk_HW3A_event_type.h
 * //  Create Date:
 * //
 * //  Comment:
 * //  Describe common difinition of HW3A event
 * //
 * ////////////////////////////////////////////////////////////////////
 */

#ifndef _AL_FRMWK_HW3A_EVENT_TYPE_H_
#define _AL_FRMWK_HW3A_EVENT_TYPE_H_

/*
 * ////////////////////////////////////////////////////////////////////
 * Include files
 * ////////////////////////////////////////////////////////////////////
 */
#include "mtype.h"


/*
 * ////////////////////////////////////////////////////////////////////
 * Macro definitions
 * ////////////////////////////////////////////////////////////////////
 */

#define MAX_RUNTIME_AE_DEBUG_DATA       (3072)
#define MAX_RUNTIME_AWB_DEBUG_DATA      (2048)
#define MAX_RUNTIME_AF_DEBUG_DATA       (8192)
#define MAX_AE_DEBUG_ARRAY_NUM          MAX_RUNTIME_AE_DEBUG_DATA
#define MAX_AE_COMMON_EXIF_DATA         (1024)

/*
 * ////////////////////////////////////////////////////////////////////
 * Static declarations
 * ////////////////////////////////////////////////////////////////////
 */


/*
 * ////////////////////////////////////////////////////////////////////
 * Type declarations
 * ////////////////////////////////////////////////////////////////////
 */

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
        ANTIFLICKER_50HZ,
        ANTIFLICKER_60HZ,
        ANTIFLICKER_OFF,
} ae_antiflicker_mode_t;

typedef enum {
        COLOR_RGGB,
        COLOR_GRGB,
        COLOR_GBGR,
        COLOR_BGGR,
} color_order_mode_t;

typedef enum {
        AL3A_OPMODE_PREVIEW = 0,
        AL3A_OPMODE_VIDEO,
        AL3A_OPMODE_SNAPSHOT,
        AL3A_OPMODE_MAX_MODE
}  AL3A_OPERATION_MODE;

typedef enum {
        AL3A_ROI_DEFAULT = 0,
        AL3A_ROI_FACE ,
        AL3A_ROI_TOUCH,
        AL3A_ROI_MAX
}  AL3A_ROI_TYPE;

typedef enum {
        AL3A_FLASH_OFF = 0,
        AL3A_FLASH_PRE,
        AL3A_FLASH_MAIN,
        AL3A_FLASH_RAMPUP,
        AL3A_FLASH_RELEASE,
} AL3A_FE_FLASH_STAT;

/* flash setting mode for flash */
typedef enum {
        AL3A_UI_FLASH_OFF = 0,
        AL3A_UI_FLASH_AUTO,
        AL3A_UI_FLASH_ON,
        AL3A_UI_FLASH_TORCH,
        AL3A_UI_FLASH_MAX,
} AL3A_FE_UI_FLASH_MODE;

/* MWB Mode */
typedef enum    {
        AL3A_WB_MODE_OFF = 0,
        AL3A_WB_MODE_AUTO,
        AL3A_WB_MODE_INCANDESCENT,
        AL3A_WB_MODE_FLUORESCENT,
        AL3A_WB_MODE_WARM_FLUORESCENT,
        AL3A_WB_MODE_DAYLIGHT,
        AL3A_WB_MODE_CLOUDY_DAYLIGHT,
        AL3A_WB_MODE_TWILIGHT,
        AL3A_WB_MODE_SHADE,
        AL3A_WB_MODE_MANUAL_CT,
        AL3A_WB_MODE_UNDEF
} awb_mode_type_t;

/*
 * //////////////////////////////////////////////////
 * //  Enumrator for EXIF debug switch
 * //  @EXIF_DEBUG_MASK_OFF : turn off all exif debug embedding
 * //  @EXIF_DEBUG_MASK_AE :  turn on AE exif debug embedding
 * //  @EXIF_DEBUG_MASK_AWB :  turn on AWB exif debug embedding
 * //  @EXIF_DEBUG_MASK_AF :  turn on AF exif debug embedding
 * //  @EXIF_DEBUG_MASK_AS :  turn on auto scene exif debug embedding
 * //  @EXIF_DEBUG_MASK_AFD : turn on anti-flicker exif debug embedding
 * //  @EXIF_DEBUG_MASK_UDEF : turn on user defined exif debug embedding (3rd party info)
 * //
 */
typedef enum {
        EXIF_DEBUG_MASK_OFF     = 0,
        EXIF_DEBUG_MASK_AE      = 1 ,
        EXIF_DEBUG_MASK_AWB     = 1 << 1,
        EXIF_DEBUG_MASK_AF      = 1 << 2,
        EXIF_DEBUG_MASK_AS      = 1 << 3,
        EXIF_DEBUG_MASK_AFD     = 1 << 4,
        EXIF_DEBUG_MASK_FRMWK   = 1 << 5,
        EXIF_DEBUG_MASK_ISP     = 1 << 6,  /* For ISP debug */
        EXIF_DEBUG_MASK_UDEF    = 1 << 7,
} EXIF_DEBUG_MASK_MODE;

typedef enum {
        AE_NOT_INITED = 0,
        AE_RUNNING,
        AE_CONVERGED,
        AE_ROI_CONVERGED,
        AE_PRECAP_CONVERGED,
        AE_LMT_CONVERGED,
        AE_DISABLED,
        AE_LOCKED,
        AE_STATE_MAX
} ae_state_t;

/* flash running status */
typedef enum {
        AE_EST_WITH_LED_OFF,
        AE_EST_WITH_LED_PRE_DONE,
        AE_EST_WITH_LED_RUNNING,
        AE_EST_WITH_LED_DONE,
        AE_EST_WITH_LED_REQUEST_LED_OFF,    /* reserved */
        AE_EST_WITH_LED_RECOVER_LV_EXP,       /* reserved */
        AE_EST_WITH_LED_RECOVER_LV_EXP_DONE,   /* reserved */

        AE_EST_WITH_LED_STATE_MAX
} fe_state_t;

/* AWB States */
typedef enum {
        AL3A_WB_STATE_UNSTABLE = 0,
        AL3A_WB_STATE_STABLE,
        AL3A_WB_STATE_LOCK,
        AL3A_WB_STATE_PREPARE_UNDER_FLASHON_DONE,
        AL3A_WB_STATE_UNDER_FLASHON_AWB_DONE,
} awb_states_type_t;


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16 r_gain;              /* scale 256 */
        UINT16 g_gain;              /* scale 256 */
        UINT16 b_gain;              /* scale 256 */
} wbgain_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32 calib_r_gain;        /* scale 1000 */
        UINT32 calib_g_gain;        /* scale 1000 */
        UINT32 calib_b_gain;        /* scale 1000 */
} calibration_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32  ad_gain;            /* sensor AD gain */
        UINT32  isp_gain;           /* ISP D gain, reserved */
        UINT32  exposure_time;      /* exposure time */
        UINT32  exposure_line;      /* exposure line */
        UINT32  ISO;
} exposure_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        INT16  ucDICTotal_idx;      /* driving IC total index, -1 : invalid index */
        INT16  ucDIC1_idx;          /* driving IC LED1 index, -1 : invalid index */
        INT16  ucDIC2_idx;          /* driving IC LED2 index, -1 : invalid index */

        INT32  udLEDTotalCurrent;   /* -1 : invalid index */
        INT32  udLED1Current;       /* scale 100, -1 : invalid index */
        INT32  udLED2Current;       /* scale 100, -1 : invalid index */

} flash_control_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        wbgain_data_t   flash_gain;
        wbgain_data_t   flash_gain_led2;    /* to AWB */
        UINT32          flash_ratio;        /* to AWB */
        UINT32          flash_ratio_led2;   /* to AWB */

        UINT32          LED1_CT;            /* uints: K */
        UINT32          LED2_CT;            /* uints: K */

} flash_report_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */



#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* broadcase to other ISP/3A/sensor module from current 3A module */

        /* common stats info */
        UINT32 hw3a_frame_id;               /* frame ID when HW3a updated */
        UINT32 sys_sof_index;               /* current SOF ID, should be maintained by framework */
        UINT32 hw3a_ae_block_nums;          /* total blocks */
        UINT32 hw3a_ae_block_totalpixels;   /* total pixels in each blocks */
        UINT32 hw3a_ae_stats[256];          /* HW3a AE stats addr */

        /* AE runing status */
        UINT32 targetmean;                  /* converge basic target mean */
        UINT32 curmean;                     /* current AE weighted mean (after weighting mask) */
        UINT32 avgmean;                     /* current AE average mean  (weighting mask all 1.0),
                                             * used for background report, which would be update during AE locked status */
        UINT32 center_mean2x2;              /* current center mean in 2x2 blocks */
        INT32   bv_val;                     /* current BV value */
        INT32   BG_BVResult;                /* BV value from average mean, updated even at AE locked
                                             * (not update when AE disabled);  */

        INT16  ae_converged;                /* AE converged flag */
        UINT8   ae_need_flash_flg;          /* flash needed flag */

        ae_state_t  ae_LibStates;           /* AE running status, refer to ae_state_t */
        fe_state_t   ae_FlashStates;        /* Flash AE status, refer to fe_state_t */

        /* common exposure paraeter to sensor */
        UINT32 sensor_ad_gain;              /* sensor A/D gain setting */
        UINT32 isp_d_gain;                  /* ISP additional D gain supported, need ISP to generate HW3a stats, reserved */
        UINT32 exposure_time;               /* current exposure time */
        UINT32 exposure_line;               /* current exposure line */
        UINT32 exposure_max_line;           /* max exposure line in current sensor mode */
        UINT32 ae_cur_iso;                  /* current ISO speed */

        /* AE control status */
        INT16  ae_roi_change_st;            /* 0 : no response to ROI change, 1: ROI changed and taken,
                                             * this reset should be performed via framework once framework ack this message status */
        ae_antiflicker_mode_t  flicker_mode;/* current flicker mode, should be set via set_param API */
        UINT16 ae_metering_mode;            /* refer to ae_metering_mode_type_t */
        UINT16 ae_script_mode;              /* refer to ae_script_mode_t */

        UINT16 DigitalZoom;                 /* store current valid digital zoom */

        INT16  bv_delta;                    /* delta BV from converge */
        UINT16  cur_fps;                    /* current FPS, from framework control information */

        exposure_data_t    flash_off_exp_dat;
        exposure_data_t    snapshot_exp_dat;

        /* hdr control */
        UINT32  hdr_exp_ratio;              /* sacle by 100, control HDR sensor long/short exposure ratio */
        UINT32  hdr_ae_long_exp_line;       /* sensor */
        UINT32  hdr_ae_long_exp_time;       /* appplication */
        UINT32  hdr_ae_long_ad_gain;        /* sensor */
        UINT32  hdr_ae_short_exp_line;      /* sensor */
        UINT32  hdr_ae_short_exp_time;      /* application */
        UINT32  hdr_ae_short_ad_gain;       /* sensor */

        /* extreme color info */
        UINT32    ae_extreme_green_cnt;     /* green color count */
        UINT32    ae_extreme_blue_cnt;      /* blue color count */
        UINT32    ae_extreme_color_cnt;     /* pure color count */

        INT32    ae_non_comp_bv_val;        /* current non compensated BV value, would be refered by AWB */

        /* flash output report */
        flash_report_data_t      preflash_report;       /* to AWB */
        flash_report_data_t      mainflash_report;      /* to AWB */

        flash_control_data_t   preflash_ctrldat;        /* to Framework */
        flash_control_data_t   mainflash_ctrldat;       /* to framework */

        /* AE state machine, would be implemented by framework layer of Ae interface */
        UINT32             ae_Frmwkstates;              /* store states of framework depend state machine define */

        /* common exif info for Altek exif reader */
        UINT16  ae_commonexif_valid_size;
        UINT8 ae_commonexif_data[MAX_AE_COMMON_EXIF_DATA];

        UINT8   ucIsEnableAeDebugReport;             /* 0: disable debug report, 1: enable debug report */
        /* debug message for altek advanced debug reader info */
        UINT16  ae_debug_valid_size;
        UINT8 ae_debug_data[MAX_RUNTIME_AE_DEBUG_DATA];

} ae_report_update_t;
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* broadcase to other ISP/3A/sensor module from current 3A module */
        UINT32                  hw3a_frame_id;          /* hw3A frame ID, reported from HW3A stats */
        UINT32                  sys_cursof_frameidx;    /* sof frame Index */
        UINT16                  awb_update;             /* valid awb to update or not */
        awb_mode_type_t         awb_mode;               /* 0: Auto, others are MWB type */
        wbgain_data_t           wbgain;
        wbgain_data_t           wbgain_balanced;
        wbgain_data_t           wbgain_flash_off;       /* for flash contorl,
                                                         * stop updating uder set state_under_flash = TRUE
                                                         * (event: prepare_under_flash) */
        UINT32                  color_temp;
        UINT32                  color_temp_flash_off;   /* for flash contorl,
                                                         * stop updating uder set state_under_flash = TRUE
                                                         * (event: prepare_under_flash) */
        UINT16                  light_source;           /* light source */
        UINT16                  awb_decision;           /* simple scene detect [TBD] */
        awb_states_type_t       awb_states;
}   awb_report_update_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* broadcase to other ISP/3A/sensor module from current 3A module */
        BOOL         focus_done;
        UINT16       status;        /* refer to af_status_type define */
        float f_distance;

        /* debug message */
        UINT32 af_debug_data[MAX_RUNTIME_AF_DEBUG_DATA];

} af_report_update_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* broadcase to other ISP/Modules */
        ae_report_update_t ae_update;      /* latest AE update */
        awb_report_update_t awb_update;    /* latest AWB update */
        af_report_update_t  af_update;     /* latest AF update */

} Report_update_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#ifdef __cplusplus
}  /*extern "C" */
#endif

#endif /* _AL_FRMWK_HW3A_EVENT_TYPE_H_ */
