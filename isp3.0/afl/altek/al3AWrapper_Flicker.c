/*
 * al3AWrapper_Flicker.c
 *
 *  Created on: 2016/01/05
 *      Author: Hubert Huang
 *  Latest update: 2016/2/17
 *      Reviser: MarkTseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AE lib to framework
 *       5. translate input parameter from AP framework to AP
 *       6. Patch stats from ISP
 *       7. WOI inform to AP
 */

#ifdef LOCAL_NDK_BUILD   // test build in local
#include <math.h>   // math lib, depends on AP OS
#include <string.h>
#include ".\..\..\INCLUDE\mtype.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"
/* Wrapper define */
#include "al3AWrapper.h"
#include "al3AWrapper_Flicker.h"
#include "al3AWrapper_FlickerErrCode.h"
/* Flicker lib define */
#include ".\..\..\INCLUDE\al3ALib_Flicker.h"

#else
#include <math.h>   // math lib, depends on AP OS
#include <string.h>
#include "mtype.h"
#include "HW3A_Stats.h"
/* Wrapper define */
#include "al3AWrapper.h"
#include "al3AWrapper_Flicker.h"
#include "al3AWrapper_FlickerErrCode.h"
/* Flicker lib define */
#include "al3ALib_Flicker.h"
#endif

/* for Flicker ctrl layer */
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
unsigned int al3AWrapper_DispatchHW3A_FlickerStats( ISP_DRV_META_AntiF_t * alISP_MetaData_Flicker, al3AWrapper_Stats_Flicker_t * alWrappered_Flicker_Dat, alFlickerRuntimeObj_t *aFlickerLibCallback, void * flicker_runtimeDat  )
{
    unsigned int ret = ERR_WPR_FLICKER_SUCCESS;
    ISP_DRV_META_AntiF_t *pMetaData_Flicker;
    al3AWrapper_Stats_Flicker_t *pPatched_FlickerDat;
    unsigned char *Stats;

    // check input parameter validity
    if ( alISP_MetaData_Flicker == NULL )
        return ERR_WRP_FLICKER_EMPTY_METADATA;
    if ( alWrappered_Flicker_Dat == NULL || flicker_runtimeDat == NULL )
        return ERR_WRP_FLICKER_INVALID_INPUT_PARAM;

    pMetaData_Flicker = (ISP_DRV_META_AntiF_t *)alISP_MetaData_Flicker;
    pPatched_FlickerDat = (al3AWrapper_Stats_Flicker_t *)alWrappered_Flicker_Dat;
    Stats = (unsigned char *)pMetaData_Flicker->pAntiF_Stats;
    // update sturcture size, this would be double checked in Flicker libs
    pPatched_FlickerDat->uStructureSize = sizeof( al3AWrapper_Stats_Flicker_t );
    // store patched data/common info/ae info from Wrapper
    pPatched_FlickerDat->uMagicNum           = pMetaData_Flicker->uMagicNum;
    pPatched_FlickerDat->uHWengineID         = pMetaData_Flicker->uHWengineID;
    pPatched_FlickerDat->uFrameIdx           = pMetaData_Flicker->uFrameIdx;
    pPatched_FlickerDat->uAntiFTokenID       = pMetaData_Flicker->uAntiFTokenID;
    pPatched_FlickerDat->uAntiFStatsSize     = pMetaData_Flicker->uAntiFStatsSize;

    // store frame & timestamp
    memcpy( &pPatched_FlickerDat->systemTime, &pMetaData_Flicker->systemTime, sizeof(struct timeval));
    pPatched_FlickerDat->udsys_sof_idx       = pMetaData_Flicker->udsys_sof_idx;

    memcpy( pPatched_FlickerDat->pAntiF_Stats, Stats, pMetaData_Flicker->uAntiFStatsSize );

    return ret;
}

/**
\API name: al3AWrapperFlicker_GetDefaultCfg
\This API is used for query default ISP config before calling al3AWrapperFlicker_UpdateISPConfig_Flicker
\param aFlickerConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
unsigned int al3AWrapperFlicker_GetDefaultCfg( alHW3a_Flicker_CfgInfo_t* aFlickerConfig )
{
    unsigned int ret = ERR_WPR_FLICKER_SUCCESS;
    alHW3a_Flicker_CfgInfo_t localParam;

    if ( aFlickerConfig == NULL )
        return ERR_WRP_FLICKER_INVALID_INPUT_PARAM;

    localParam.TokenID = 0xFFFF;
    localParam.uwoffsetratiox = 0;
    localParam.uwoffsetratioy = 0;
    memcpy( aFlickerConfig, &localParam, sizeof(alHW3a_Flicker_CfgInfo_t ) );
    return ret;
}

/**
\API name: al3AWrapperFlicker_UpdateISPConfig_Flicker
\This API is used for updating ISP config
\param a_ucSensor[in]: AHB sensor ID
\param aFlickerConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
unsigned int al3AWrapperFlicker_UpdateISPConfig_Flicker( unsigned char a_ucSensor, alHW3a_Flicker_CfgInfo_t* aFlickerConfig )
{
    unsigned int ret = ERR_WPR_FLICKER_SUCCESS;

    return ret;
}

/**
\API name: al3AWrapperFlicker_QueryISPConfig_Flicker
\This API is used for query ISP config before calling al3AWrapperFlicker_UpdateISPConfig_Flicker
\param aFlickerConfig[out]: API would manage parameter and return via this pointer
\param aFlickerLibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param flicker_runtimeDat[in]: Flicker lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
unsigned int al3AWrapperFlicker_QueryISPConfig_Flicker( alHW3a_Flicker_CfgInfo_t* aFlickerConfig, alFlickerRuntimeObj_t *aFlickerLibCallback, void * flicker_runtimeDat, raw_info *aFlickerSensorInfo )
{
    unsigned int ret = ERR_WPR_FLICKER_SUCCESS;
    flicker_get_param_t localParam;
    void* outputParam = NULL;  //useless declaration
    flicker_set_param_t local_SetParam;

    if ( aFlickerConfig == NULL || flicker_runtimeDat == NULL )
        return ERR_WRP_FLICKER_INVALID_INPUT_PARAM;

    memset( &localParam, 0, sizeof(flicker_get_param_t) );
    localParam.flicker_get_param_type = FLICKER_GET_ALHW3A_CONFIG;  // ask Flicker lib for HW3A setting

    // send command to Flicker lib, query HW3A setting parameter
    ret = aFlickerLibCallback->get_param( &localParam, flicker_runtimeDat );
    if ( ret != ERR_WPR_FLICKER_SUCCESS )
        return ret;

    //Update raw info & Line time
    local_SetParam.set_param.RawSizeX = aFlickerSensorInfo->sensor_raw_w;
    local_SetParam.set_param.RawSizeY = aFlickerSensorInfo->sensor_raw_h;
    local_SetParam.set_param.Line_Time = aFlickerSensorInfo->Line_Time;
    local_SetParam.flicker_set_param_type = FLICKER_SET_PARAM_RAW_SIZE;
    ret = aFlickerLibCallback->set_param( &local_SetParam, outputParam, &flicker_runtimeDat );
    if ( ret != ERR_WPR_FLICKER_SUCCESS )
        return ret;
    local_SetParam.flicker_set_param_type = FLICKER_SET_PARAM_LINE_TIME;
    ret = aFlickerLibCallback->set_param( &local_SetParam, outputParam, &flicker_runtimeDat );
    if ( ret != ERR_WPR_FLICKER_SUCCESS )
        return ret;

    memcpy( aFlickerConfig, &localParam.alHW3A_FlickerConfig, sizeof( alHW3a_Flicker_CfgInfo_t ) );  //copy data, prepare for return

    return ret;
}

/**
\API name: al3AWrapperAntiF_GetVersion
\This API would return labeled version of wrapper
\fWrapVersion[out], return current wapper version
\return: error code
*/
unsigned int al3AWrapperAntiF_GetVersion( float *fWrapVersion )
{
	unsigned int ret = ERR_WPR_FLICKER_SUCCESS;

	*fWrapVersion = _WRAPPER_ANTIF_VER;

	return ret;
}
