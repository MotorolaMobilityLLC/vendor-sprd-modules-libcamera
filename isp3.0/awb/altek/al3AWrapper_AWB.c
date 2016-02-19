/*
 * al3AWrapper_AWB.c
 *
 *  Created on: 2015/12/07
 *      Author: HanTseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AWB lib to framework
 *       5. translate input parameter from AP framework to AP
 */

#include <math.h>   /* math lib, depends on AP OS */
#include <string.h>
#include "al3AWrapper.h"
#include "al3AWrapper_AWB.h"  /* include wrapper AWB define */
#include "al3AWrapper_AWBErrCode.h"

/*
 * API name: al3AWrapper_DispatchHW3A_AWBStats
 * This API used for patching HW3A stats from ISP(Altek) for AWB libs(Altek),
 * after patching completed, AWB ctrl should prepare patched
 * stats to AWB libs
 * param alISP_MetaData_AWB[In]: patched data after calling al3AWrapper_DispatchHW3AStats, 
 *                               used for patch AWB stats for AWB lib
 * param alWrappered_AWB_Dat[Out]: result AWB stats
 * return: error code
 */
unsigned int al3AWrapper_DispatchHW3A_AWBStats(void *alISP_MetaData_AWB, void *alWrappered_AWB_Dat)
{
        unsigned int ret = ERR_WPR_AWB_SUCCESS;
        unsigned int utotalBlocks;
        ISP_DRV_META_AWB_t      *pMetaData_AWB;
        al3AWrapper_Stats_AWB_t *pWrapper_Stat_AWB;

        /* check input parameter validity */
        if (alISP_MetaData_AWB == NULL)
                return ERR_WRP_AWB_EMPTY_METADATA;

        if (alWrappered_AWB_Dat == NULL)
                return ERR_WRP_AWB_INVALID_INPUT_PARAM;

        pMetaData_AWB = (ISP_DRV_META_AWB_t *) alISP_MetaData_AWB;
        pWrapper_Stat_AWB = (al3AWrapper_Stats_AWB_t *) alWrappered_AWB_Dat;

        utotalBlocks = pMetaData_AWB->awb_stats_info.ucValidBlocks * pMetaData_AWB->awb_stats_info.ucValidBanks;

        /* check data AWB blocks validity */
        if (utotalBlocks > AL_MAX_AWB_STATS_NUM)
                return ERR_WRP_AWB_INVALID_BLOCKS;

        /*  Common info */
        pWrapper_Stat_AWB->uMagicNum   = pMetaData_AWB->uMagicNum;
        pWrapper_Stat_AWB->uHWengineID = pMetaData_AWB->uHWengineID;
        pWrapper_Stat_AWB->uFrameIdx   = pMetaData_AWB->uFrameIdx;

        /*  AWB info */
        pWrapper_Stat_AWB->pAWB_Stats    = pMetaData_AWB->pAWB_Stats;
        pWrapper_Stat_AWB->uAWBTokenID   = pMetaData_AWB->uAWBTokenID;
        pWrapper_Stat_AWB->uAWBStatsSize = pMetaData_AWB->uAWBStatsSize;
        /* uPseudoFlag 0: normal stats, 1: PseudoFlag flag (for lib, smoothing/progressive run) */
        pWrapper_Stat_AWB->uPseudoFlag   = pMetaData_AWB->uPseudoFlag;
        /* AWB stats info */
        pWrapper_Stat_AWB->udPixelsPerBlocks = pMetaData_AWB->awb_stats_info.udPixelsPerBlocks;
        pWrapper_Stat_AWB->udBankSize        = pMetaData_AWB->awb_stats_info.udBankSize;
        pWrapper_Stat_AWB->ucValidBlocks     = pMetaData_AWB->awb_stats_info.ucValidBlocks;
        pWrapper_Stat_AWB->ucValidBanks      = pMetaData_AWB->awb_stats_info.ucValidBanks;
        pWrapper_Stat_AWB->ucStatsDepth      = 8;
        pWrapper_Stat_AWB->ucStats_format    = 0;

        /* store frame & timestamp */
        memcpy(&pWrapper_Stat_AWB->systemTime, &pMetaData_AWB->systemTime, sizeof(struct timeval));
        pWrapper_Stat_AWB->udsys_sof_idx       = pMetaData_AWB->udsys_sof_idx;

        return ret;
}


/*
 * API name: al3AWrapperAWB_TranslateSceneModeFromAWBLib2AP
 * This API used for translating AWB lib scene define to framework
 * param aSceneMode[In] :   scene mode define of AWB lib (Altek)
 * return: scene mode define for AP framework
 */
unsigned int al3AWrapperAWB_TranslateSceneModeFromAWBLib2AP(unsigned int aSceneMode)
{
        unsigned int retAPSceneMode;

        switch (aSceneMode) {
        case 0:
                retAPSceneMode = 0;
                break;
        default:
                retAPSceneMode = 0;
                break;
        }

        return retAPSceneMode;
}

/*
 * API name: al3AWrapperAWB_TranslateSceneModeFromAP2AWBLib
 * This API used for translating framework scene mode to AWB lib define
 * aSceneMode[In] :   scene mode define of AP framework define
 * return: scene mode define for AWB lib (Altek)
 */
unsigned int al3AWrapperAWB_TranslateSceneModeFromAP2AWBLib(unsigned int aSceneMode)
{
        unsigned int retAWBSceneMode;

        switch (aSceneMode) {
        case 0:
                retAWBSceneMode = 0;
                break;
        default:
                retAWBSceneMode = 0;
                break;
        }

        return retAWBSceneMode;
}

/*
 * API name: al3AWrapperAWB_GetDefaultCfg
 * This API is used for query default ISP config before calling al3AWrapperAWB_QueryISPConfig_AWB,
 * which default config use without correct OTP
 * param aAWBConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
unsigned int al3AWrapperAWB_GetDefaultCfg(alHW3a_AWB_CfgInfo_t *aAWBConfig)
{
        unsigned int ret = ERR_WPR_AWB_SUCCESS;
        alHW3a_AWB_CfgInfo_t localParam;
        unsigned char   i;
        unsigned char   yFactor[16] = { 0, 0, 0, 4, 7, 10, 12, 14, 
                                        15, 15, 15, 15, 14, 13, 10, 5};
        signed char     bbrFactor[33] = {22, 20, 18, 16, 15, 13, 11, 10, 8, 8,
                                        6, 5, 3, 1, -1, -3, -4, -5, -6, -7, -8,
                                        -9, -10, -11, -12, -13, -14, -15, -16,
                                        -18, -18, -18, -18};

        if (aAWBConfig == NULL)
                return ERR_WRP_AWB_INVALID_INPUT_PARAM;

        localParam.TokenID                   = 0x01;
        localParam.tAWBRegion.uwBorderRatioX = 100;
        localParam.tAWBRegion.uwBorderRatioY = 100;
        localParam.tAWBRegion.uwBlkNumX      = 64;
        localParam.tAWBRegion.uwBlkNumY      = 48;
        localParam.tAWBRegion.uwOffsetRatioX = 0;
        localParam.tAWBRegion.uwOffsetRatioY = 0;
        memcpy(localParam.ucYFactor, yFactor, 16 * sizeof(unsigned char));
        memcpy(localParam.BBrFactor, bbrFactor, 33 * sizeof(signed char));
        localParam.uwRGain                   = 0;
        localParam.uwGGain                   = 0;
        localParam.uwBGain                   = 0;
        localParam.ucCrShift                 = 100;
        localParam.ucOffsetShift             = 100;
        localParam.ucQuantize                = 0;
        localParam.ucDamp                    = 7;
        localParam.ucSumShift                = 5;
        localParam.tHis.bEnable              = TRUE;
        localParam.tHis.cCrStart             = -46;
        localParam.tHis.cCrEnd               = 110;
        localParam.tHis.cOffsetUp            = 10;
        localParam.tHis.cOffsetDown          = -90;
        localParam.tHis.cCrPurple            = 0;
        localParam.tHis.ucOffsetPurPle       = 2;
        localParam.tHis.cGrassOffset         = -22;
        localParam.tHis.cGrassStart          = -30;
        localParam.tHis.cGrassEnd            = 25;
        localParam.tHis.ucDampGrass          = 4;
        localParam.tHis.cOffset_bbr_w_start  = -2;
        localParam.tHis.cOffset_bbr_w_end    = 2;
        localParam.tHis.ucYFac_w             = 2;
        localParam.tHis.dHisInterp           = -178;
        localParam.uwRLinearGain             = 128;
        localParam.uwBLinearGain             = 128;
        memcpy(aAWBConfig, &localParam, sizeof(alHW3a_AWB_CfgInfo_t));

        return ret;
}

/*
 * API name: al3AWrapperAWB_QueryISPConfig_AWB
 * This API is used for query ISP config before calling al3AWrapperAWB_UpdateISPConfig_AWB
 * param aAWBConfig[out]: API would manage parameter and return via this pointer
 * param aAWBLibCallback[in]: callback lookup table, 
 * must passing correct table into this API for querying HW3A config
 * return: error code
 */
unsigned int al3AWrapperAWB_QueryISPConfig_AWB(alHW3a_AWB_CfgInfo_t *aAWBConfig, alAWBRuntimeObj_t *aAWBLibCallback)
{
        unsigned int ret = ERR_WPR_AWB_SUCCESS;
        alAWBLib_get_parameter_t localParam;

        if (aAWBConfig == NULL || aAWBLibCallback == NULL || aAWBLibCallback->awb == NULL)
                return ERR_WRP_AWB_INVALID_INPUT_PARAM;

        localParam.type = alawb_get_param_init_isp_config;
        ret = aAWBLibCallback->get_param(&localParam, aAWBLibCallback->awb);

        if (ret != ERR_WPR_AWB_SUCCESS)
                return ret;

        memcpy(aAWBConfig, localParam.para.awb_hw_config, sizeof(alHW3a_AWB_CfgInfo_t));

        return ret;
}

/*
 * API name: al3AWrapperAWB_QueryISPConfig_AWB
 * This API is used for query ISP config before calling al3AWrapperAWB_UpdateISPConfig_AWB
 * param a_ucSensor[in]: AHB sensor ID
 * param aAWBConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
unsigned int al3AWrapperAWB_UpdateISPConfig_AWB(unsigned char a_ucSensor, alHW3a_AWB_CfgInfo_t *aAWBConfig)
{
        unsigned int ret = ERR_WPR_AWB_SUCCESS;
#ifndef LOCAL_NDK_BUILD
//        ret = ISPDRV_AP3AMGR_SetAWBCfg(a_ucSensor, aAWBConfig);
#endif
        return ret;
}


/*
 * API name: al3AWrapperAWB_SetOTPcalibration
 * This API is used for setting stylish file to alAWBLib
 * param calib_data[in]: calibration data, scale 1000
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting calibration data
 * return: error code
 */
unsigned int al3AWrapperAWB_SetOTPcalibration(calibration_data_t *calib_data , alAWBRuntimeObj_t *aAWBLibCallback)
{
        unsigned int ret = ERR_WPR_AWB_SUCCESS;
        alAWBLib_set_parameter_t localParam;

        if (calib_data == NULL || aAWBLibCallback->awb == NULL)
                return ERR_WRP_AWB_INVALID_INPUT_PARAM;

        localParam.type = alawb_set_param_camera_calib_data;
        localParam.para.awb_calib_data.calib_r_gain = calib_data->calib_r_gain;
        localParam.para.awb_calib_data.calib_g_gain = calib_data->calib_g_gain;
        localParam.para.awb_calib_data.calib_b_gain = calib_data->calib_b_gain;
        ret = aAWBLibCallback->set_param(&localParam, aAWBLibCallback->awb);

        return ret;
}


/*
 * API name: al3AWrapperAWB_SetTuningFile
 * This API is used for setting Tuning file to alAWBLib
 * param setting_file[in]: file address pointer to setting file [TBD, need confirm with 3A data file format
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting file
 * return: error code
 */
unsigned int al3AWrapperAWB_SetTuningFile(void *setting_file, alAWBRuntimeObj_t *aAWBLibCallback)
{
        unsigned int ret = ERR_WPR_AWB_SUCCESS;
        alAWBLib_set_parameter_t localParam;

        if (setting_file == NULL || aAWBLibCallback->awb == NULL)
                return ERR_WRP_AWB_INVALID_INPUT_PARAM;

        localParam.type = alawb_set_param_tuning_file;
        localParam.para.tuning_file = setting_file;
        ret = aAWBLibCallback->set_param(&localParam, aAWBLibCallback->awb);

        return ret;
}

