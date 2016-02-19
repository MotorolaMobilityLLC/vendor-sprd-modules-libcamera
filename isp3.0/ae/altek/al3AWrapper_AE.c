/*
 * al3AWrapper_AE.c
 *
 *  Created on: 2015/12/06
 *      Author: MarkTseng
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
#include ".\..\..\INCLUDE\mtype.h"
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"
/* AE lib define */
#include ".\..\..\INCLUDE\al3ALib_AE.h"
#include ".\..\..\INCLUDE\al3ALib_AE_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"
#include "al3AWrapper_AE.h"  // include wrapper AE define
#include "al3AWrapper_AEErrCode.h"

#else  // normal release in AP 
#include "mtype.h"
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"
/* AE lib define */
#include "al3ALib_AE.h"
#include "al3ALib_AE_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"
#include "al3AWrapper_AE.h"  // include wrapper AE define
#include "al3AWrapper_AEErrCode.h"

#endif

#include "ae_altek_adpt.h"

#include <math.h>   // math lib, depends on AP OS
#include <string.h>

#include <stdio.h>  // for debug printf
#include <stdlib.h>  // for debug printf

/* for AE ctrl layer */
/**
\API name: al3AWrapper_DispatchHW3A_AEStats
\This API used for patching HW3A stats from ISP(Altek) for AE libs(Altek), after patching completed, AE ctrl should prepare patched
\stats to AE libs
\param alISP_MetaData_AE[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AE stats for AE lib
\param alWrappered_AE_Dat[Out]: result patched AE stats, must input WB gain
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapper_DispatchHW3A_AEStats( ISP_DRV_META_AE_t * alISP_MetaData_AE, al3AWrapper_Stats_AE_t * alWrappered_AE_Dat, alAERuntimeObj_t *aAELibCallback, void * ae_runtimeDat  )
{
    UINT32 ret = ERR_WPR_AE_SUCCESS;
    ISP_DRV_META_AE_t *pMetaData_AE;
    UINT8 *Stats;
    al3AWrapper_Stats_AE_t *pPatched_AEDat;
    al3AWrapper_AE_wb_gain_t wb_gain;
    UINT32 udtotalBlocks;
    UINT32 udPixelsPerBlocks;
    UINT32 udBankSize;
    UINT32 udOffset;
    UINT16 i,j,blocks, banks, index;
    ae_get_param_t localParam;

    // check input parameter validity
    if ( alISP_MetaData_AE == NULL )
        return ERR_WRP_AE_EMPTY_METADATA;
    if ( alWrappered_AE_Dat == NULL || ae_runtimeDat == NULL )
        return ERR_WRP_AE_INVALID_INPUT_PARAM;

    pMetaData_AE = (ISP_DRV_META_AE_t *)alISP_MetaData_AE;
    pPatched_AEDat = (al3AWrapper_Stats_AE_t *)alWrappered_AE_Dat;
    Stats = (UINT8 *)pMetaData_AE->pAE_Stats;

    // update sturcture size, this would be double checked in AE libs
    pPatched_AEDat->uStructureSize = sizeof( al3AWrapper_Stats_AE_t );

    udtotalBlocks = pMetaData_AE->ae_stats_info.ucValidBlocks * pMetaData_AE->ae_stats_info.ucValidBanks;
    udBankSize = pMetaData_AE->ae_stats_info.udBankSize;
    blocks = pMetaData_AE->ae_stats_info.ucValidBlocks;
    banks = pMetaData_AE->ae_stats_info.ucValidBanks;

    // check data AE blocks validity
    if ( udtotalBlocks > AL_MAX_AE_STATS_NUM )
        return ERR_WRP_AE_INVALID_BLOCKS;

    udPixelsPerBlocks = pMetaData_AE->ae_stats_info.udPixelsPerBlocks;

    // query AWB from AE libs
    memset( &localParam, 0, sizeof(ae_get_param_t) );
    localParam.ae_get_param_type = AE_GET_CURRENT_CALIB_WB;  // ask AE lib for HW3A setting
    ret = aAELibCallback->get_param( &localParam, ae_runtimeDat );
    if ( ret != ERR_WPR_AE_SUCCESS )
        return ret;

    // check WB validity, calibration WB is high priority
    if ( localParam.para.calib_data.calib_r_gain == 0 ||  localParam.para.calib_data.calib_g_gain == 0 || localParam.para.calib_data.calib_b_gain == 0 ) { // calibration gain from OTP, scale 1000, UINT32 type
        memset( &localParam, 0, sizeof(ae_get_param_t) );
        localParam.ae_get_param_type = AE_GET_CURRENT_WB;  // ask AE lib for HW3A setting
        ret = aAELibCallback->get_param( &localParam, ae_runtimeDat );
        if ( ret != ERR_WPR_AE_SUCCESS )
            return ret;

        if ( localParam.para.wb_data.r_gain == 0 || localParam.para.wb_data.g_gain == 0 || localParam.para.wb_data.b_gain == 0  )   // WB from AWB, scale 256, UINT16 type
            return ERR_WRP_AE_INVALID_INPUT_WB;
        else {
            wb_gain.r = (UINT32)(localParam.para.wb_data.r_gain);
            wb_gain.g = (UINT32)(localParam.para.wb_data.g_gain);
            wb_gain.b = (UINT32)(localParam.para.wb_data.b_gain);
#if print_ae_log
            /* debug printf, removed for release version */
            ISP_LOGI( "al3AWrapper_DispatchHW3A_AEStats get LV WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );
            // LOGD( "al3AWrapper_DispatchHW3A_AEStats get LV WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );
#endif
        }
    } else {
        wb_gain.r = (UINT32)(localParam.para.calib_data.calib_r_gain *256/localParam.para.calib_data.calib_g_gain);
        wb_gain.g = (UINT32)(localParam.para.calib_data.calib_g_gain *256/localParam.para.calib_data.calib_g_gain);
        wb_gain.b = (UINT32)(localParam.para.calib_data.calib_b_gain *256/localParam.para.calib_data.calib_g_gain);

#if print_ae_log
        /* debug printf, removed for release version */
        ISP_LOGI( "al3AWrapper_DispatchHW3A_AEStats get OTP WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );
        // LOGD( "al3AWrapper_DispatchHW3A_AEStats get OTP WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );        
#endif
    }

    // store to stats info, translate to unit base 1000 (= 1.0x)
    pPatched_AEDat->stat_WB_2Y.r = wb_gain.r*1000/256;
    pPatched_AEDat->stat_WB_2Y.g = wb_gain.g*1000/256;
    pPatched_AEDat->stat_WB_2Y.b = wb_gain.b*1000/256;

    wb_gain.r = (UINT32)(wb_gain.r *  0.299 + 0.5);  //  76.544: 0.299 * 256.
    wb_gain.g = (UINT32)(wb_gain.g *  0.587 + 0.5);  // 150.272: 0.587 * 256. , Combine Gr Gb factor
    wb_gain.b = (UINT32)(wb_gain.b *  0.114 + 0.5);  //  29.184: 0.114 * 256.
#if print_ae_log
    /* debug printf, removed for release version */
    ISP_LOGI( "al3AWrapper_DispatchHW3A_AEStats get patching WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );
    // LOGD( "al3AWrapper_DispatchHW3A_AEStats get patching WB R/G/B : (%d, %d, %d) \r\n", wb_gain.r, wb_gain.g, wb_gain.b );    
#endif
    udOffset =0;
    index = 0;
    pPatched_AEDat->ucStatsDepth = 10;

    // store patched data/common info/ae info from Wrapper
    pPatched_AEDat->uMagicNum           = pMetaData_AE->uMagicNum;
    pPatched_AEDat->uHWengineID         = pMetaData_AE->uHWengineID;
    pPatched_AEDat->uFrameIdx           = pMetaData_AE->uFrameIdx;
    pPatched_AEDat->uAETokenID          = pMetaData_AE->uAETokenID;
    pPatched_AEDat->uAEStatsSize        = pMetaData_AE->uAEStatsSize;
    pPatched_AEDat->udPixelsPerBlocks   = udPixelsPerBlocks;
    pPatched_AEDat->udBankSize          = udBankSize;
    pPatched_AEDat->ucValidBlocks       = blocks;
    pPatched_AEDat->ucValidBanks        = banks;
    pPatched_AEDat->uPseudoFlag         = pMetaData_AE->uPseudoFlag;

#if print_ae_log
    /* debug printf, removed for release version */
    ISP_LOGI( "al3AWrapper_DispatchHW3A_AEStats VerNum: %d, HWID: %d, Frmidx: %d, TokID: %d, Size: %d, PPB: %d, BZ: %d, ValidBlock: %d, ValidBank:%d, SFlag:%d \r\n", 
    pPatched_AEDat->uMagicNum, pPatched_AEDat->uHWengineID, pPatched_AEDat->uFrameIdx, pPatched_AEDat->uAETokenID, pPatched_AEDat->uAEStatsSize,
    pPatched_AEDat->udPixelsPerBlocks, pPatched_AEDat->udBankSize, pPatched_AEDat->ucValidBlocks, pPatched_AEDat->ucValidBanks, pPatched_AEDat->uPseudoFlag  );    
#endif
    // store frame & timestamp
    memcpy( &pPatched_AEDat->systemTime, &pMetaData_AE->systemTime, sizeof(struct timeval));
    pPatched_AEDat->udsys_sof_idx       = pMetaData_AE->udsys_sof_idx;
#if print_ae_log
    /* debug printf, removed for release version */
    ISP_LOGI( "al3AWrapper_DispatchHW3A_AEStats SOF idx :%d \r\n", pPatched_AEDat->udsys_sof_idx  );  
#endif
    // patching R/G/G/B from WB or calibration WB to Y/R/G/B,
    // here is the sample which use array passing insetad of pointer passing of prepared buffer from AE ctrl layer
    pPatched_AEDat->bIsStatsByAddr = FALSE;
    for ( j =0; j <banks; j++ ) {
        udOffset = udBankSize*j;
        for ( i = 0; i <blocks; i++ ) {

            // due to data from HW, use direct address instead of casting
            pPatched_AEDat->statsR[index] = ( Stats[udOffset] + (Stats[udOffset+1]<<8) + (Stats[udOffset+2]<<16) + (Stats[udOffset+3]<<24) )/udPixelsPerBlocks; //10 bits
            pPatched_AEDat->statsG[index] = (( ( Stats[udOffset+4] + (Stats[udOffset+5]<<8) + (Stats[udOffset+6]<<16) + (Stats[udOffset+7]<<24) ) +
                                               ( Stats[udOffset+8] + (Stats[udOffset+9]<<8) + (Stats[udOffset+10]<<16) + (Stats[udOffset+11]<<24) ) )/udPixelsPerBlocks)>>1; //10 bits
            pPatched_AEDat->statsB[index] = ( Stats[udOffset+12] + (Stats[udOffset+13]<<8) + (Stats[udOffset+14]<<16) + (Stats[udOffset+15]<<24) )/udPixelsPerBlocks; //10 bits

            // calculate Y
            pPatched_AEDat->statsY[index] = ( pPatched_AEDat->statsR[index]* wb_gain.r + pPatched_AEDat->statsG[index] * wb_gain.g +
                                              pPatched_AEDat->statsB[index] * wb_gain.b ) >> 8;  // 10 bits
#if print_ae_log
            /* debug printf, removed for release version */
            ISP_LOGI( "al3AWrapper_DispatchHW3A_AEStats stats[%d] Y/R/G/B: %d, %d, %d, %d \r\n", index, 
            pPatched_AEDat->statsY[index],pPatched_AEDat->statsR[index], pPatched_AEDat->statsG[index], pPatched_AEDat->statsB[index] ); 
#endif
            index++;
            udOffset += 16;
            
            
        }
    }

    return ret;
}

/**
\API name: al3AWrapperAE_UpdateOTP2AELib
\This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
\param aCalibWBGain[in]: calibration data from OTP
\param alAERuntimeObj_t[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_output[in]: returned output result after calling ae lib estimation
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapperAE_UpdateOTP2AELib( calib_wb_gain_t aCalibWBGain,  alAERuntimeObj_t *aAELibCallback, ae_output_data_t *ae_output , void * ae_runtimeDat )
{
    UINT32 ret = ERR_WPR_AE_SUCCESS;
    ae_set_param_t localParam;  // local parameter set

    if ( aCalibWBGain.r == 0 || aCalibWBGain.g == 0 || aCalibWBGain.b == 0 )
      return ERR_WRP_AE_INVALID_INPUT_WB;
    
    memset( &localParam, 0, sizeof(ae_set_param_content_t) );
    localParam.ae_set_param_type = AE_SET_PARAM_OTP_WB_DAT;  // ask AE lib for HW3A setting
    localParam.set_param.ae_calib_wb_gain.calib_r_gain = aCalibWBGain.r;   // scale 1000 base, if source is already scaled by 1000, direcly passing to AE lib
    localParam.set_param.ae_calib_wb_gain.calib_g_gain = aCalibWBGain.g;   // scale 1000 base, if source is already scaled by 1000, direcly passing to AE lib
    localParam.set_param.ae_calib_wb_gain.calib_b_gain = aCalibWBGain.b;   // scale 1000 base, if source is already scaled by 1000, direcly passing to AE lib

    ret = aAELibCallback->set_param( &localParam, ae_output, ae_runtimeDat );
    if ( ret != ERR_WPR_AE_SUCCESS )
        return ret;

    return ret;
}

/**
\API name: al3AWrapperAE_UpdateOTP2AELib
\This API is used for set correct HW3A config (Altek ISP) to generate correct AE stats
\param aAEReport[in]: ae report from AE update
\return: error code
*/
UINT32 al3AWrapper_UpdateAEState( ae_report_update_t* aAEReport )
{
    UINT32 ret = ERR_WPR_AE_SUCCESS;

    /* state should follow framework define */
    // initial state, before AE running, state should be non-initialed state


    // converged state, after converge for over 2 ~ x run, should be converge state


    // running state, when lib running, not converged, should be running state


    // lock state, if framework (or other lib, such as AF) request AE to pause running, would be locked state


    // disable state, only when framework disable AE lib, state would be disable state

    return ret;
}

/**
\API name: al3AWrapperAE_TranslateSceneModeFromAELib2AP
\This API used for translating AE lib scene define to framework
\param aSceneMode[In] :   scene mode define of AE lib (Altek)
\return: scene mode define for AP framework
*/
UINT32 al3AWrapperAE_TranslateSceneModeFromAELib2AP( UINT32 aSceneMode )
{
    UINT32 retAPSceneMode;

    // here just sample code, need to be implement by Framework define
    switch (aSceneMode) {
    case 0:  // scene 0
        retAPSceneMode = 0;
        break;
    default:
        retAPSceneMode = 0;
        break;
    }


    return retAPSceneMode;
}

/**
\API name: al3AWrapperAE_TranslateSceneModeFromAP2AELib
\This API used for translating framework scene mode to AE lib define
\aSceneMode[In] :   scene mode define of AP framework define
\return: scene mode define for AE lib (Altek)
*/
UINT32 al3AWrapperAE_TranslateSceneModeFromAP2AELib( UINT32 aSceneMode )
{
    UINT32 retAESceneMode;

    // here just sample code, need to be implement by Framework define
    switch (aSceneMode) {
    case 0:  // scene 0
        retAESceneMode = 0;
        break;
    default:
        retAESceneMode = 0;
        break;
    }
    return retAESceneMode;
}

/**
\API name: al3AWrapperAE_QueryISPConfig_AE
\This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
\param aAEConfig[out]: API would manage parameter and return via this pointer
\param aAELibCallback[in]: callback lookup table, must passing correct table into this API for querying HW3A config
\param ae_runtimeDat[in]: AE lib runtime buffer after calling init, must passing correct addr to into this API
\return: error code
*/
UINT32 al3AWrapperAE_QueryISPConfig_AE( alHW3a_AE_CfgInfo_t* aAEConfig, alAERuntimeObj_t *aAELibCallback, void * ae_runtimeDat )
{
    UINT32 ret = ERR_WPR_AE_SUCCESS;
    ae_get_param_t localParam;

    if ( aAEConfig == NULL || ae_runtimeDat == NULL )
        return ERR_WRP_AE_INVALID_INPUT_PARAM;

    memset( &localParam, 0, sizeof(ae_get_param_t) );
    localParam.ae_get_param_type = AE_GET_ALHW3A_CONFIG;  // ask AE lib for HW3A setting

    // send command to AE lib, query setting parameter
    ret = aAELibCallback->get_param( &localParam, ae_runtimeDat );
    if ( ret != ERR_WPR_AE_SUCCESS )
        return ret;

    memcpy( aAEConfig, &localParam.para.alHW3A_AEConfig, sizeof( alHW3a_AE_CfgInfo_t ) );  //copy data, prepare for return

    return ret;
}

/**
\API name: al3AWrapperAE_GetDefaultCfg
\This API is used for query default ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
\param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
UINT32 al3AWrapperAE_GetDefaultCfg( alHW3a_AE_CfgInfo_t* aAEConfig)
{
    UINT32 ret = ERR_WPR_AE_SUCCESS;
    alHW3a_AE_CfgInfo_t localParam;

    if ( aAEConfig == NULL )
        return ERR_WRP_AE_INVALID_INPUT_PARAM;

    localParam.TokenID = 0x01;
    localParam.tAERegion.uwBorderRatioX = 100;   // 100% use of current sensor cropped area
    localParam.tAERegion.uwBorderRatioY = 100;   // 100% use of current sensor cropped area
    localParam.tAERegion.uwBlkNumX = 16;         // fixed value for AE lib
    localParam.tAERegion.uwBlkNumY = 16;         // fixed value for AE lib
    localParam.tAERegion.uwOffsetRatioX = 0;     // 0% offset of left of current sensor cropped area
    localParam.tAERegion.uwOffsetRatioY = 0;     // 0% offset of top of current sensor cropped area

    memcpy( aAEConfig, &localParam, sizeof(alHW3a_AE_CfgInfo_t ) );
    return ret;
}



/**
\API name: al3AWrapperAE_QueryISPConfig_AE
\This API is used for query ISP config before calling al3AWrapperAE_UpdateISPConfig_AE
\param a_ucSensor[in]: AHB sensor ID
\param aAEConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
UINT32 al3AWrapperAE_UpdateISPConfig_AE( UINT8 a_ucSensor, alHW3a_AE_CfgInfo_t* aAEConfig )
{
    UINT32 ret = ERR_WPR_AE_SUCCESS;
#ifndef LOCAL_NDK_BUILD  // test build in local  
//    ret = ISPDRV_AP3AMGR_SetAECfg( a_ucSensor, aAEConfig );
#endif
    return ret;
}

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