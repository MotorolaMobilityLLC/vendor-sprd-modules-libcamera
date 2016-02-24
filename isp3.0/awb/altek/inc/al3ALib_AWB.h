/*
 * File name: al3ALib_AWB.h
 * Create Date:
 *
 * Comment:
 * Describe common difinition of alAWBLib algo type definition / interface
 */
#ifndef _ALTEK_AWB_LIB_
#define _ALTEK_AWB_LIB_

/* Include files */

#include "pthread.h"

#include "mtype.h"
#include "HW3A_Stats.h"
#include "al3ALib_AWB_ErrCode.h"
#include "FrmWk_HW3A_event_type.h"

/* Macro definitions */
#define alAWBLib_Debug_Size         (512)

/* Static declarations */


/* Type declarations */

typedef enum {
        alawb_lightsource_d75 = 0,      /* about 7500K */
        alawb_lightsource_d65,          /* about 6500K */
        alawb_lightsource_d55,          /* about 5500K */
        alawb_lightsource_cwf,          /* about 4000K */
        alawb_lightsource_t_light,      /* about 3800K */
        alawb_lightsource_a_light,      /* about 2850K */
        alawb_lightsource_h_light,      /* about 2380K */
        alawb_lightsource_flash,        /* flash light */
        alawb_lightsource_unknown
}   alAWBLib_lightsource_t;

typedef enum {
        alawb_response_stable = 0,
        alawb_response_quick_act,
        alawb_response_direct,
        alawb_response_unknow
}   alAWBLib_response_type_t;

typedef enum {
        alawb_response_level_normal = 0,
        alawb_response_level_slow,
        alawb_response_level_fast,
        alawb_response_level_unknown
}   alAWBLib_response_level_t;

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        awb_mode_type_t             wbmode_type;            /* 0: Auto, others are MWB type */
        UINT32                      manual_ct;              /* manual color temperature */
}   alAWBLib_awb_mode_setting_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        BOOL                        flag_face;
        UINT16                      x;
        UINT16                      y;
        UINT16                      w;
        UINT16                      h;
        UINT16                      frame_w;
        UINT16                      frame_h;
}   alAWBLib_faceInfo_param_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        alAWBLib_response_type_t    response_type;          /* AWB Stable type */
        alAWBLib_response_level_t   response_level;         /* AWB Stable level */
}   alAWBLib_response_setting_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        ae_state_t                  ae_state;               /* referrence to alAELib AE states */
        fe_state_t                  fe_state;               /* referrence to alAELib FE states */
        INT16                       ae_converge;
        INT32                       BV;
        INT32                       non_comp_BV;            /* non compensated bv value */
        UINT32                      ISO;                    /* ISO Speed */

        flash_report_data_t         flash_param_preview;
        flash_report_data_t         flash_param_capture;
}   alAWBLib_ae_param_setting_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

typedef enum {
        alawb_flow_none = 0,
        alawb_flow_lock,
        alawb_flow_bypass,
}   alAWBLib_awb_flow_setting_t;

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        alAWBLib_awb_flow_setting_t manual_setting;         /* manual lock flag */
        wbgain_data_t               manual_wbgain;          /* manual lock wb gain */
        UINT16                      manual_ct;              /* manual lock color temperature */
}   alAWBLib_manual_flow_setting_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        wbgain_data_t               initial_wbgain;
        UINT16                      color_temperature;
}   alAWBLib_initial_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

typedef enum {
        alawb_set_flash_none = 0,
        alawb_set_flash_prepare_under_flashon,
        alawb_set_flash_under_flashon,
        alawb_set_flash_under_flashon_capture,
}   alAWBLib_set_flash_states_t;

typedef enum {
        alawb_dbg_none = 0,
        alawb_dbg_enable_output,
        alawb_dbg_enable_log,
        alawb_dbg_manual,
        alawb_dbg_manual_flowCheck,
}   alAWBLib_awb_debug_type;

/* Report */
typedef enum {
        alawb_state_unstable = 0,
        alawb_state_stable,
        alawb_state_lock,
        alawb_state_prepare_under_flashon_done,
        alawb_state_under_flashon_awb_done,
}   alAWBLib_awb_states_t;

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        Report_update_t             report_3a_update;       /* update awb information */
        UINT32                      hw3a_curframeidx;       /* hw3a frame Index */
        UINT32                      sys_cursof_frameidx;    /* sof frame Index */
        UINT16                      awb_update;             /* valid awb to update or not */
        awb_mode_type_t             awb_mode;               /* WB mode, 0: Auto, others are MWB type */
        wbgain_data_t               wbgain;                 /* WB gain for final result */
        wbgain_data_t               wbgain_balanced;        /* All balanced WB gain, for shading or else */
        wbgain_data_t               wbgain_capture;         /* WB gain for single shot */
        wbgain_data_t               wbgain_flash_off;       /* WB gain for flash control, flash off status, all-balanced */
        UINT32                      color_temp;             /* (major) color temperature */
        UINT32                      color_temp_capture;     /* one shot color temperature */
        UINT32                      color_temp_flash_off;   /* flash off color temperature */
        alAWBLib_lightsource_t      light_source;           /* light source */
        UINT16                      awb_decision;           /* simple scene detect */
        BOOL                        flag_shading_on;        /* 0: False, 1: True */
        alAWBLib_awb_states_t       awb_states;             /* alAWBLib states */
        alAWBLib_awb_debug_type     awb_debug_mask;         /* awb debug mask, can print different information with different mask */
        UINT32                      awb_debug_data_size;    /* awb debug data size */
        CHAR                        awb_debug_data_array[alAWBLib_Debug_Size]; /* awb debug data */
        void                        *awb_debug_data_full;   /* awb debug data full size, Structure2, about 10K [TBD] */
} alAWBLib_output_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* Set / Get Param */
typedef enum {
        alawb_set_param_camera_calib_data = 1,
        alawb_set_param_tuning_file,
        alawb_set_param_sys_sof_frame_idx,
        alawb_set_param_awb_mode_setting,
        alawb_set_param_response_setting,
        alawb_set_param_update_ae_report,
        alawb_set_param_update_af_report,
        alawb_set_param_dzoom_factor,
        alawb_set_param_face_info_setting,
        alawb_set_param_flag_shading,
        alawb_set_param_awb_debug_mask,
        alawb_set_param_manual_flow,
        alawb_set_param_test_fix_patten,
        alawb_set_param_state_under_flash,
        alawb_set_param_max
}   alAWBLib_set_parameter_type_t;

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        alAWBLib_set_parameter_type_t       type;
        union {
                calibration_data_t              awb_calib_data;         /* alawb_set_param_camera_calib_data */
                void                            *tuning_file;           /* alawb_set_param_tuning_file */
                UINT32                          sys_sof_frame_idx;      /* alawb_set_param_sys_sof_frame_idx */
                alAWBLib_awb_mode_setting_t     awb_mode_setting;       /* alawb_set_param_awb_mode_setting */
                alAWBLib_response_setting_t     awb_response_setting;   /* alawb_set_param_response_setting */
                alAWBLib_ae_param_setting_t     ae_report_update;       /* alawb_set_param_update_report */
                af_report_update_t              af_report_update;       /* alawb_set_param_update_af_report */
                FLOAT32                         dzoom_factor;           /* alawb_set_param_dzoom_factor */
                alAWBLib_faceInfo_param_t       face_info;              /* alawb_set_param_face_info_setting */
                BOOL                            flag_shading;           /* alawb_set_param_flag_shading */
                alAWBLib_awb_debug_type         awb_debug_mask;         /* alawb_set_param_awb_debug_mask */
                alAWBLib_manual_flow_setting_t  awb_manual_flow;        /* alawb_set_param_manual_flow */
                BOOL                            test_fix_patten;        /* alawb_set_param_test_fix_patten */
                alAWBLib_set_flash_states_t     state_under_flash;      /* alawb_set_param_state_under_flash */
        }   para;
}   alAWBLib_set_parameter_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

typedef enum {
        alawb_get_param_init_isp_config = 1,
        alawb_get_param_init_setting,
        alawb_get_param_wbgain,
        alawb_get_param_wbgain_balanced,
        alawb_get_param_wbgain_flash_off,
        alawb_get_param_color_temperature,
        alawb_get_param_light_source,
        alawb_get_param_awb_decision,
        alawb_get_param_manual_flow,
        alawb_get_param_test_fix_patten,
        alawb_get_param_awb_states,
        alawb_get_param_max
}   alAWBLib_get_parameter_type_t;

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        alAWBLib_get_parameter_type_t       type;
        UINT32                              hw3a_curframeidx;
        UINT32                              sys_cursof_frameidx;
        union {
                alHW3a_AWB_CfgInfo_t            *awb_hw_config;         /* alawb_get_param_init_isp_config */
                alAWBLib_initial_data_t         awb_init_data;          /* alawb_get_param_init_setting */
                wbgain_data_t                   wbgain;                 /* alawb_get_param_wbgain */
                wbgain_data_t                   wbgain_balanced;        /* alawb_get_param_wbgain_balanced */
                wbgain_data_t                   wbgain_flash_off;       /* alawb_get_param_wbgain_flash_off */
                UINT32                          color_temp;             /* alawb_get_param_color_temperature */
                UINT16                          light_source;           /* alawb_get_param_light_source */
                UINT16                          awb_decision;           /* alawb_get_param_awb_decision */
                alAWBLib_manual_flow_setting_t  awb_manual_flow;        /* alawb_get_param_manual_flow */
                BOOL                            test_fix_patten;        /* alawb_get_param_test_fix_patten */
                alAWBLib_awb_states_t           awb_states;             /* alawb_get_param_awb_states */
        }   para;
}   alAWBLib_get_parameter_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* public APIs */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16 major_version;
        UINT16 minor_version;
}   alAWB_Lib_Version_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

void alAWBLib_GetLibVersion(alAWB_Lib_Version_t *AWB_LibVersion);

typedef UINT32 (*alAWBLib_init_func)(void *awb_obj);
typedef UINT32 (*alAWBLib_deinit_func)(void *awb_obj);
typedef UINT32 (*alAWBLib_set_param_func)(alAWBLib_set_parameter_t *param, void *awb_dat);
typedef UINT32 (*alAWBLib_get_param_func)(alAWBLib_get_parameter_t *param, void *awb_dat);
typedef UINT32 (*alAWBLib_estimation_func)(void *HW3a_stats_Data, void *awb_dat, alAWBLib_output_data_t *awb_output);

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        pthread_mutex_t             mutex_obj;
        void                        *awb;
        alAWBLib_init_func          initial;
        alAWBLib_deinit_func        deinit;
        alAWBLib_set_param_func     set_param;
        alAWBLib_get_param_func     get_param;
        alAWBLib_estimation_func    estimation;
}   alAWBRuntimeObj_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* Return: TRUE: loadFunc success. FALSE: loadFunc error. */
BOOL    alAWBLib_loadFunc(alAWBRuntimeObj_t *awb_run_obj);

#endif /*_ALTEK_AWB_LIB_ */

