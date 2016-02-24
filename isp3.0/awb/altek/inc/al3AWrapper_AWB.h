/*
 * File name: al3AWrapper_AWB.h
 * Create Date:
 * Comment:
 *
 */

#ifndef _AL_3AWRAPPER_AWB_H_
#define _AL_3AWRAPPER_AWB_H_

#ifdef LOCAL_NDK_BUILD

#include ".\..\..\INCLUDE\mtype.h"
/* ISP Framework define */
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"

/* AWB lib define */
#include ".\..\..\INCLUDE\al3ALib_AWB.h"
#include ".\..\..\INCLUDE\al3ALib_AWB_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"

#else
#include "mtype.h"
/* ISP Framework define */
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"

/* AWB lib define */
#include "al3ALib_AWB.h"
#include "al3ALib_AWB_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define _WRAPPER_AWB_VER 0.8000

/*
 * API name: al3AWrapperAWB_SetOTPcalibration
 * This API is used for setting stylish file to alAWBLib
 * param calib_data[in]: calibration data, type: FLOAT32
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting calibration data
 * return: error code
 */
unsigned int al3AWrapperAWB_SetOTPcalibration(calibration_data_t *calib_data , alAWBRuntimeObj_t *aAWBLibCallback);

/*
 * API name: al3AWrapperAWB_GetDefaultCfg
 * This API is used for query default ISP config before calling al3AWrapperAWB_QueryISPConfig_AWB,
 * which default config use without correct OTP
 * param aAWBConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
unsigned int al3AWrapperAWB_GetDefaultCfg(alHW3a_AWB_CfgInfo_t* aAWBConfig);

/*
 * API name: al3AWrapperAWB_QueryISPConfig_AWB
 * This API is used for query ISP config before calling al3AWrapperAWB_UpdateISPConfig_AWB
 * param aAWBConfig[out]: API would manage parameter and return via this pointer
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
 * return: error code
 */
unsigned int al3AWrapperAWB_QueryISPConfig_AWB(alHW3a_AWB_CfgInfo_t *aAWBConfig, alAWBRuntimeObj_t *aAWBLibCallback);

/*
 * API name: al3AWrapperAWB_QueryISPConfig_AWB
 * This API is used for set correct HW3A config (Altek ISP) to generate correct AWB stats
 * param a_ucSensor[in]: AHB sensor ID
 * param aAWBConfig[in]: setting after query via calling al3AWrapperAWB_QueryISPConfig_AWB
 * return: error code
 */
unsigned int al3AWrapperAWB_UpdateISPConfig_AWB(unsigned char a_ucSensor, alHW3a_AWB_CfgInfo_t *aAWBConfig);

/*
 * API name: al3AWrapperAWB_TranslateSceneModeFromAWBLib2AP
 * This API used for translating AWB lib scene define to framework
 * param aSceneMode[In] :   scene mode define of AWB lib (Altek)
 * return: scene mode define for AP framework
 */
unsigned int al3AWrapperAWB_TranslateSceneModeFromAWBLib2AP(unsigned int aSceneMode);

/*
 * API name: al3AWrapperAWB_TranslateSceneModeFromAP2AWBLib
 * This API used for translating framework scene mode to AWB lib define
 * aSceneMode[In] :   scene mode define of AP framework define
 * return: scene mode define for AWB lib (Altek)
 */
unsigned int al3AWrapperAWB_TranslateSceneModeFromAP2AWBLib(unsigned int aSceneMode);

/*
 * API name: al3AWrapper_DispatchHW3A_AWBStats
 * This API used for patching HW3A stats from ISP(Altek) for AWB libs(Altek),
 * after patching completed, AWB ctrl should prepare patched
 * stats to AWB libs
 * param alISP_MetaData_AWB[In]: patched data after calling al3AWrapper_DispatchHW3AStats,
 *                               used for patch AWB stats for AWB lib
 * param alWrappered_AWB_Dat[Out]: result patched AWB stats
 * return: error code
 */
unsigned int al3AWrapper_DispatchHW3A_AWBStats(void *alISP_MetaData_AWB, void *alWrappered_AWB_Dat);

/*
 * API name: al3AWrapperAWB_SetTuningFile
 * This API is used for setting Tuning file to alAWBLib
 * param setting_file[in]: file address pointer to setting file [TBD, need confirm with 3A data file format
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting file
 * return: error code
 */
unsigned int al3AWrapperAWB_SetTuningFile(void *setting_file, alAWBRuntimeObj_t *aAWBLibCallback);

/**
\API name: al3AWrapperAWB_GetVersion
\This API would return labeled version of wrapper
\fWrapVersion[out], return current wapper version
\return: error code
*/
unsigned int al3AWrapperAWB_GetVersion( float *fWrapVersion );

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* _AL_3AWRAPPER_AWB_H_ */

