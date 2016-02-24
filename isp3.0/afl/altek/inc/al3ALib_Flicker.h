/**
*	\file	al3ALib_Flicker.h
*	\brief	Structure of Setting Parameters
*	\author		Hubert Huang
*	\version	1.0
*	\date		2015/10/23
*******************************************************************************/
#ifndef _AL_FLICKER_H_
#define _AL_FLICKER_H_

#include "HW3A_Stats.h"

//Public initial setting
#define FLICKERINIT_FLICKER_ENABLE          1  // 1:enable, 0:disable
#define FLICKERINIT_TOTAL_QUEUE             7
#define FLICKERINIT_REF_QUEUE               4
#define FLICKERINIT_RAWSIZE_X               1396
#define FLICKERINIT_RAWSIZE_Y               1044
#define FLICKERINIT_LINETIME                0.000024414  //unit: s(second)
#define REFERENCE_PREVIOUS_DATA_INTERVAL    1


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    FLICKER_SET_PARAM_INVALID_ENTRY=0,     // invalid entry start
    //Basic Command
    FLICKER_SET_PARAM_INIT_SETTING,
    FLICKER_SET_PARAM_ENABLE,
    FLICKER_SET_PARAM_RAW_SIZE,
    FLICKER_SET_PARAM_LINE_TIME,
    FLICKER_SET_PARAM_CURRENT_FREQUENCY,
    FLICKER_SET_PARAM_REFERENCE_DATA_INTERVAL,

    FLICKER_SET_PARAM_MAX

}flicker_set_param_type_t;

typedef enum
{
    FLICKER_GET_PARAM_INVALID_ENTRY=0,     // invalid entry start
    FLICKER_GET_ALHW3A_CONFIG,
    FLICKER_GET_SUCCESS_NUM,

    FLICKER_GET_PARAM_MAX

}flicker_get_param_type_t;

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/*  Flicker set data (content) */
typedef struct
{
    /* initial setting, basic setting related */
    //flicker_set_parameter_init_t  flicker_initial_content;
    //Basic Command
    unsigned char flicker_enable;

    //Threshold Command
    unsigned char TotalQueue;               //Number of total queues
    unsigned char RefQueue;                 //Number of consulting queues
    unsigned char ReferencePreviousData;    //The frame that we refered.(at least 1), ex: 1:Previous data, 2:The one before previous data

    //Raw Info Command
    unsigned short RawSizeX;
    unsigned short RawSizeY;

    //Sensor Info Command
    float Line_Time;                    //unit: second
    unsigned short CurrentFreq;

}flicker_set_param_content_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/*  Flicker set data */
typedef struct
{
    flicker_set_param_type_t flicker_set_param_type;
    flicker_set_param_content_t set_param;

}flicker_set_param_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/* Flicker get data */
typedef struct
{
    unsigned short SuccessNum50;
    unsigned short SuccessNum60;

}flicker_get_param_content_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/* Flicker get data */
typedef struct
{
    flicker_get_param_type_t   flicker_get_param_type;
    flicker_get_param_content_t get_param;
    alHW3a_Flicker_CfgInfo_t  alHW3A_FlickerConfig;

}flicker_get_param_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
typedef struct
{
    unsigned char FinalFreq;

}flicker_output_data_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
typedef struct
{
    unsigned short major_version;
    unsigned short minor_version;

} al_flickerlib_version_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

// public function declaration
typedef unsigned int (* alFlickerLib_intial )(void ** flicker_init_buffer );
typedef unsigned int (* alFlickerLib_deinit )( void * flicker_obj );
typedef unsigned int (* alFlickerLib_set_param )( flicker_set_param_t *param, flicker_output_data_t *flicker_output , void *flicker_runtime_dat );
typedef unsigned int (* alFlickerLib_get_param )( flicker_get_param_t *param, void *flicker_runtime_dat );
typedef unsigned int (* alFlickerLib_process )( void * HW3a_stats_Data, void *flicker_runtime_dat, flicker_output_data_t *flicker_output );

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
typedef struct
{
    alFlickerLib_intial    initial;
    alFlickerLib_deinit    deinit;
    alFlickerLib_set_param set_param;
    alFlickerLib_get_param get_param;
    alFlickerLib_process   process;

}alFlickerRuntimeObj_t;
#pragma pack(pop)  /* restore old alignment setting from stack  */

void alFlickerLib_GetLibVersion( al_flickerlib_version_t* flicker_LibVersion );


#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _AL_FLICKER_H_