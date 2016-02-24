////////////////////////////////////////////////////////////////////
//  File name: al3AWrapper_Flicker.h
//  Create Date:
//
//  Comment:
//
//
////////////////////////////////////////////////////////////////////
#ifndef _AL_3AWRAPPER_FLICKER_H_
#define _AL_3AWRAPPER_FLICKER_H_

#ifdef LOCAL_NDK_BUILD
#include ".\..\..\INCLUDE\mtype.h"
/* ISP Framework define */
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"

/* Flicker lib define */
#include ".\..\..\INCLUDE\al3ALib_Flicker.h"
#include ".\..\..\INCLUDE\al3ALib_Flicker_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"

#else  // normal release
#include "mtype.h"
/* ISP Framework define */
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"

/* Flicker lib define */
#include "al3ALib_Flicker.h"
#include "al3ALib_Flicker_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"
#endif

#define _WRAPPER_ANTIF_VER 0.8000

#ifdef __cplusplus
extern "C"
{
#endif

/**
\API name: al3AWrapper_DispatchHW3A_FlickerStats
\This API used for patching HW3A stats from ISP(Altek) for Flicker libs(Altek), after patching completed, Flicker ctrl should prepare patched
\stats to Flicker libs
\param alISP_MetaData_Flicker[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch Flicker stats for Flicker lib
\param alWrappered_Flicker_Dat[Out]: result patched Flicker stats
\param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
unsigned int al3AWrapper_DispatchHW3A_FlickerStats( ISP_DRV_META_AntiF_t * alISP_MetaData_Flicker, al3AWrapper_Stats_Flicker_t * alWrappered_Flicker_Dat, alFlickerRuntimeObj_t *aFlickerLibCallback, void * flicker_runtimeDat  );


/**
\API name: al3AWrapperFlicker_GetDefaultCfg
\This API is used for query default ISP config before calling al3AWrapperFlicker_UpdateISPConfig_Flicker
\param aFlickerConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
unsigned int al3AWrapperFlicker_GetDefaultCfg( alHW3a_Flicker_CfgInfo_t* aFlickerConfig );


/**
\API name: al3AWrapperFlicker_UpdateISPConfig_Flicker
\This API is used for updating ISP config
\param a_ucSensor[in]: AHB sensor ID
\param aFlickerConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
unsigned int al3AWrapperFlicker_UpdateISPConfig_Flicker( unsigned char a_ucSensor, alHW3a_Flicker_CfgInfo_t* aFlickerConfig );


/**
\API name: al3AWrapperFlicker_QueryISPConfig_Flicker
\This API is used for query ISP config before calling al3AWrapperFlicker_UpdateISPConfig_Flicker
\param aFlickerConfig[out]: API would manage parameter and return via this pointer
\param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
unsigned int al3AWrapperFlicker_QueryISPConfig_Flicker( alHW3a_Flicker_CfgInfo_t* aFlickerConfig, alFlickerRuntimeObj_t *aFlickerLibCallback, void * flicker_runtimeDat, raw_info *aFlickerSensorInfo );

/**
\API name: al3AWrapperAntiF_GetVersion
\This API would return labeled version of wrapper
\fWrapVersion[out], return current wapper version
\return: error code
*/
unsigned int al3AWrapperAntiF_GetVersion( float *fWrapVersion );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // _AL_3AWRAPPER_FLICKER_H_