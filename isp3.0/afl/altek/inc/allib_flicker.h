/******************************************************************************
 * File name: allib_flicker.h
 * brief	Structure of Setting Parameters
 * author		Hubert Huang
 *
 * date		2015/10/23
 *****************************************************************************/
#ifndef _AL_FLICKER_H_
#define _AL_FLICKER_H_

#include "hw3a_stats.h"

//Public initial setting
#define FLICKERINIT_FLICKER_ENABLE          ( 1 )  /*  1:enable, 0:disable */
#define FLICKERINIT_TOTAL_QUEUE             ( 7 )
#define FLICKERINIT_REF_QUEUE               ( 4 )
#define FLICKERINIT_RAWSIZE_X               ( 1396 )
#define FLICKERINIT_RAWSIZE_Y               ( 1044 )
#define FLICKERINIT_LINETIME                ( 9900 ) /* unit: ns(nano second) */
#define REFERENCE_PREVIOUS_DATA_INTERVAL    ( 1 )


#ifdef __cplusplus
extern "C"
{
#endif

enum flicker_set_param_type_t {
	FLICKER_SET_PARAM_INVALID_ENTRY = 0,     /*  invalid entry start */
	/* Basic Command */
	FLICKER_SET_PARAM_INIT_SETTING,
	FLICKER_SET_PARAM_ENABLE,
	FLICKER_SET_PARAM_RAW_SIZE,
	FLICKER_SET_PARAM_LINE_TIME,
	FLICKER_SET_PARAM_CURRENT_FREQUENCY,
	FLICKER_SET_PARAM_REFERENCE_DATA_INTERVAL,
	FLICKER_SET_PARAM_ENABLE_DEBUG_REPORT,
	FLICKER_SET_PARAM_MAX,

};

enum flicker_get_param_type_t {
	FLICKER_GET_PARAM_INVALID_ENTRY = 0,     /* invalid entry start */
	FLICKER_GET_ALHW3A_CONFIG,
	FLICKER_GET_SUCCESS_NUM,
	FLICKER_GET_PARAM_MAX,

};

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/*  Flicker set data (content) */
struct flicker_set_param_content_t {
	/* initial setting, basic setting related */
	/* basic command */
	uint8 flicker_enable;
	uint8 flicker_enableDebugLog;

	/* threshold command */
	uint8 totalqueue;               /* number of total queues */
	uint8 refqueue;                 /* number of consulting queues */
	uint8 referencepreviousdata;    /* the frame that we refered.(at least 1), ex: 1:previous data, 2:the one before previous data */

	/* raw info command */
	uint16 rawsizex;
	uint16 rawsizey;

	/* sensor info command */
	uint32 line_time;               /* unit: nano second */
	uint16 currentfreq;

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/*  Flicker set data */
struct flicker_set_param_t {
	enum    flicker_set_param_type_t      flicker_set_param_type;
	struct  flicker_set_param_content_t   set_param;

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/* Flicker get data */
struct flicker_get_param_content_t {
	uint16 SuccessNum50;
	uint16 SuccessNum60;

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
/* Flicker get data */
struct flicker_get_param_t {
	enum   flicker_get_param_type_t      flicker_get_param_type;
	struct flicker_get_param_content_t   get_param;
	struct alhw3a_flicker_cfginfo_t      alhw3a_flickerconfig;

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct flicker_output_data_t {
	struct report_update_t rpt_flicker_update;   // store each module report update result
	unsigned char finalfreq;

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct al_flickerlib_version_t {
	uint16 major_version;
	uint16 minor_version;

} ;
#pragma pack(pop)  /* restore old alignment setting from stack  */

// public function declaration
typedef uint32 (* allib_flicker_intial )(void ** flicker_init_buffer );
typedef uint32 (* allib_flicker_deinit )( void * flicker_obj );
typedef uint32 (* allib_flicker_set_param )( struct flicker_set_param_t *param, struct flicker_output_data_t *flicker_output , void *flicker_runtime_dat );
typedef uint32 (* allib_flicker_get_param )( struct flicker_get_param_t *param, void *flicker_runtime_dat );
typedef uint32 (* allib_flicker_process )( void * hw3a_stats_data, void *flicker_runtime_dat, struct flicker_output_data_t *flicker_output );

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting  */
struct alflickerruntimeobj_t {
	allib_flicker_intial     initial;
	allib_flicker_deinit     deinit;
	allib_flicker_set_param  set_param;
	allib_flicker_get_param  get_param;
	allib_flicker_process    process;

};
#pragma pack(pop)  /* restore old alignment setting from stack  */

void allib_flicker_getlibversion( struct al_flickerlib_version_t* flicker_libversion );

/*
 *\brief get anti-flicker detection lib API address
 *\param flicker_run_obj [out], object of anti-flicker detection lib APIs address
 *\param identityID  [in],  framework tag for current instance
 *\return value , TRUE: loading with no error , FALSE: false loading function APIs address
 */
uint8 allib_flicker_loadfunc( struct alflickerruntimeobj_t *flicker_run_obj, uint32 identityid );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _AL_FLICKER_H_