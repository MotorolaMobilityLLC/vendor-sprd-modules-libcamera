////////////////////////////////////////////////////////////////////
//  File name: al3AWrapper_AE.h
//  Create Date:
//
//  Comment:
//
//
////////////////////////////////////////////////////////////////////
#ifndef _AL_3AWRAPPER_AE_H_
#define _AL_3AWRAPPER_AE_H_

#ifdef LOCAL_NDK_BUILD
#include ".\..\..\INCLUDE\mtype.h"
/* ISP Framework define */
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"

/* AE lib define */
#include ".\..\..\INCLUDE\al3ALib_AE.h"
#include ".\..\..\INCLUDE\al3ALib_AE_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"

#else  // normal release
#include "mtype.h"
/* ISP Framework define */
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"

/* AE lib define */
#include "al3ALib_AE.h"
#include "al3ALib_AE_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/**
\API name: al3AWrapperAE_QueryISPConfig_AE
\This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
\param aAEConfig[out]: API would manage parameter and return via this pointer
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapperAE_QueryISPConfig_AE( alHW3a_AE_CfgInfo_t* aAEConfig, alAERuntimeObj_t *aAELibCallback, void * ae_runtimeDat );

/**
\API name: al3AWrapperAE_GetDefaultCfg
\This API is used for query default ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
\param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
UINT32 al3AWrapperAE_GetDefaultCfg( alHW3a_AE_CfgInfo_t* aAEConfig);

/**
\API name: al3AWrapperAE_QueryISPConfig_AE
\This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
\param a_ucSensor[in]: AHB sensor ID
\param aAEConfig[in]: setting after query via calling al3AWrapperAE_QueryISPConfig_AE
\return: error code
*/
UINT32 al3AWrapperAE_UpdateISPConfig_AE( UINT8 a_ucSensor, alHW3a_AE_CfgInfo_t* aAEConfig );

/**
\API name: al3AWrapperAE_UpdateOTP2AELib
\This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
\param aCalibWBGain[in]: calibration data from OTP
\param alAERuntimeObj_t[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_output[in]: returned output result after calling ae lib estimation
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapperAE_UpdateOTP2AELib( calib_wb_gain_t aCalibWBGain,  alAERuntimeObj_t *aAELibCallback, ae_output_data_t *ae_output , void * ae_runtimeDat );

/**
\API name: al3AWrapperAE_UpdateOTP2AELib
\This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
\param aAEReport[in]: ae report from AE update
\return: error code
*/
UINT32 al3AWrapper_UpdateAEState( ae_report_update_t* aAEReport );

/**
\API name: al3AWrapperAE_TranslateSceneModeFromAELib2AP
\This API used for translating AE lib scene define to framework
\param aSceneMode[In] :   scene mode define of AE lib (Altek)
\return: scene mode define for AP framework
*/
UINT32 al3AWrapperAE_TranslateSceneModeFromAELib2AP( UINT32 aSceneMode );

/**
\API name: al3AWrapperAE_TranslateSceneModeFromAP2AELib
\This API used for translating framework scene mode to AE lib define
\aSceneMode[In] :   scene mode define of AP framework define
\return: scene mode define for AE lib (Altek)
*/
UINT32 al3AWrapperAE_TranslateSceneModeFromAP2AELib( UINT32 aSceneMode );

/**
\API name: al3AWrapper_DispatchHW3A_AEStats
\This API used for patching HW3A stats from ISP(Altek) for AE libs(Altek), after patching completed, AE ctrl should prepare patched
\stats to AE libs
\param alISP_MetaData_AE[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AE stats for AE lib
\param alWrappered_AE_Dat[Out]: result patched AE stats
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapper_DispatchHW3A_AEStats( ISP_DRV_META_AE_t * alISP_MetaData_AE, al3AWrapper_Stats_AE_t * alWrappered_AE_Dat, alAERuntimeObj_t *aAELibCallback, void * ae_runtimeDat  );


/**
\API name: al3AWrapper_SetAELib_Param
\This API should be interface between framework with AE lib, if framework define differ from libs, should be translated here
\uDatatype[in], usually should be set_param type, such as set ROI, etc.
\set_param[in], would be framework defined structure, casting case by case
\ae_output[in], returned output result after calling ae lib estimation
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapper_SetAELib_Param( void * uDatatype, void * set_param, alAERuntimeObj_t *aAELibCallback, ae_output_data_t *ae_output, void * ae_runtimeDat );

/**
\API name: al3AWrapper_SetAELib_Param
\This API should be interface between framework with AE lib, if framework define differ from libs, should be translated here
\uDatatype[in], usually should be set_param type, such as set ROI, etc.
\set_param[out], would be framework defined structure, casting case by case
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapper_GetAELib_Param( void * uDatatype, void * get_param, alAERuntimeObj_t *aAELibCallback, void * ae_runtimeDat );

/**
\API name: al3AWrapper_UpdateAESettingFile
\This API should be interface between framework with AE lib, if framework define differ from libs, should be translated here
\ae_set_file[in], handle of AE setting file, remapping here if neccessary
\ae_output[in], returned output result after calling ae lib estimation
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapper_UpdateAESettingFile( void * ae_set_file, alAERuntimeObj_t *aAELibCallback, ae_output_data_t *ae_output, void * ae_runtimeDat );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _AL_3AWRAPPER_AE_H_