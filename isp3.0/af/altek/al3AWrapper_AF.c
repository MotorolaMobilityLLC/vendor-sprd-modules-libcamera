/*
 * al3AWrapper_AF.c
 *
 *  Created on: 2015/12/06
 *      Author: ZenoKuo
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AE lib to framework
 *       5. translate input parameter from AP framework to AP
*/


 /* test build in local */ 
 #ifdef LOCAL_NDK_BUILD
#include ".\..\..\INCLUDE\mtype.h"
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"
/* AF lib define */
#include ".\..\..\INCLUDE\al3ALib_AF.h"

/* Wrapper define */
#include "al3AWrapper.h"
#include "al3AWrapper_AF.h"
#include "al3AWrapper_AFErrCode.h"

/* normal release in AP*/
#else
#include "mtype.h"
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"
/* AE lib define */
#include "al3ALib_AF.h"
#include "al3ALib_AF_ErrCode.h"

/* Wrapper define */
#include "al3AWrapper.h"
#include "al3AWrapper_AF.h"
#include "al3AWrapper_AFErrCode.h"

#endif

#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#if 0
#define WRAP_LOG(...) printf(__VA_ARGS__)
#else
#define WRAP_LOG(...) do { } while(0)
#endif

/* VCM inf step*/
#define VCM_INF_STEP_ADDR_OFFSET (1714)
/*VCM macro step*/
#define VCM_MACRO_STEP_ADDR_OFFSET (1715)
/*VCM calibration inf distance in mm.*/
#define VCM_INF_STEP_CALIB_DISTANCE (20000)
/*VCM calibration macro distance in mm.*/
#define VCM_MACRO_STEP_CALIB_DISTANCE (700)
/* f-number, ex. f2.0, then input 2.0*/
#define MODULE_F_NUMBER (2.0)
/* focal length in mm*/
#define MODULE_FOCAL_LENGTH (4400)
/* mechanical limitation top position (macro side) in step.*/
#define VCM_INF_STEP_MECH_TOP (1023)
/*mechanical limitation bottom position(inf side) in step.*/
#define VCM_INF_STEP_MECH_BOTTOM (0)
/* time consume of lens from moving to stable in ms. */
#define VCM_MOVING_STATBLE_TIME (20)

#define AF_DEFAULT_AFBLK_TH {\
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define AF_DEFAULT_THLUT {0, 0, 0, 0}
#define AF_DEFAULT_MASK {0, 0, 0, 1, 1, 2}
#define AF_DEFAULT_LUT {   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,\
                            11, 12, 13, 14, 15, 16, 17, 18, 19, 20,\
                            21, 22, 23, 24, 25, 26, 27, 28, 29, 30,\
                            31, 32, 33, 34, 35, 36, 37, 38, 39, 40,\
                            41, 42, 43, 44, 45, 46, 47, 48, 49, 50,\
                            51, 52, 53, 54, 55, 56, 57, 58, 59, 60,\
                            61, 62, 63, 64, 65, 66, 67, 68, 69, 70,\
                            71, 72, 73, 74, 75, 76, 77, 78, 79, 80,\
                            81, 82, 83, 84, 85, 86, 87, 88, 89, 90,\
                            91, 92, 93, 94, 95, 96, 97, 98, 99,100,\
                           101,102,103,104,105,106,107,108,109,110,\
                           111,112,113,114,115,116,117,118,119,120,\
                           121,122,123,124,125,126,127,128,129,130,\
                           131,132,133,134,135,136,137,138,139,140,\
                           141,142,143,144,145,146,147,148,149,150,\
                           151,152,153,154,155,156,157,158,159,160,\
                           161,162,163,164,165,166,167,168,169,170,\
                           171,172,173,174,175,176,177,178,179,180,\
                           181,182,183,184,185,186,187,188,189,190,\
                           191,192,193,194,195,196,197,198,199,200,\
                           201,202,203,204,205,206,207,208,209,210,\
                           211,212,213,214,215,216,217,218,219,220,\
                           221,222,223,224,225,226,227,228,229,230,\
                           231,232,233,234,235,236,237,238,239,240,\
                           241,242,243,244,245,246,247,248,249,250,\
                           251,252,253,254,255,255,255,255}

/* for AF ctrl layer */
/**
\API name: al3AWrapper_DispatchHW3A_AFStats
\This API used for patching HW3A stats from ISP(Altek) for AF libs(Altek), after patching completed, AF ctrl should prepare patched 
\stats to AF libs
\param alISP_MetaData_AF[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AF stats for AF lib
\param alWrappered_AF_Dat[Out]: result patched AF stats
\return: error code
*/
UINT32 al3AWrapper_DispatchHW3A_AFStats(void *alISP_MetaData_AF,void *alWrappered_AF_Dat)
{
        UINT32 ret = ERR_WPR_AF_SUCCESS;
        ISP_DRV_META_AF_t *pMetaData_AF;
        alAFLib_hw_stats_t *pPatched_AFDat;
        UINT32 udTotalBlocks;
        UINT32 uwBankSize;
        UINT16 i = 0,j = 0,blocks, banks, index;
        UINT32 *StatsAddr_32;
        UINT64 *StatsAddr_64;
        WRAP_LOG("al3AWrapper_DispatchHW3A_AFStats start\n");

         /* check input parameter validity*/
        if(alISP_MetaData_AF == NULL){
                WRAP_LOG("ERR_WRP_AF_EMPTY_METADATA\n");
                return ERR_WRP_AF_EMPTY_METADATA;
        }

        if(alWrappered_AF_Dat == NULL){
                WRAP_LOG("ERR_WRP_AF_INVALID_INPUT_PARAM\n");
                return ERR_WRP_AF_INVALID_INPUT_PARAM;
        }

        WRAP_LOG("alISP_MetaData_AF %p\n",alISP_MetaData_AF);
        pMetaData_AF = (ISP_DRV_META_AF_t *)alISP_MetaData_AF;
        WRAP_LOG("alWrappered_AF_Dat %p\n",alWrappered_AF_Dat);
        pPatched_AFDat = (alAFLib_hw_stats_t *)alWrappered_AF_Dat;
        udTotalBlocks = pMetaData_AF->af_stats_info.ucValidBlocks * pMetaData_AF->af_stats_info.ucValidBanks;
        WRAP_LOG("udTotalBlocks %d\n",udTotalBlocks);
        uwBankSize = pMetaData_AF->af_stats_info.udBankSize;
        WRAP_LOG("uwBankSize %d\n",uwBankSize);
        blocks = pMetaData_AF->af_stats_info.ucValidBlocks;
        WRAP_LOG("blocks %d\n",blocks);

        if(blocks > MAX_STATS_COLUMN_NUM)
                blocks = MAX_STATS_COLUMN_NUM;

        pPatched_AFDat->valid_column_num = blocks;
        banks = pMetaData_AF->af_stats_info.ucValidBanks;
        WRAP_LOG("banks %d\n",banks);

        if(banks > MAX_STATS_ROW_NUM)
                banks = MAX_STATS_ROW_NUM;

        pPatched_AFDat->valid_row_num = banks;
        index = 0;
        WRAP_LOG("uHWengineID %d\n",pMetaData_AF->uHWengineID);

        /* AP3AMGR_HW3A_A_0 and AP3AMGR_HW3A_B_0, see the AP3AMgr.h*/
        if(AL3A_HW3A_DEV_ID_A_0 == pMetaData_AF->uHWengineID){
                for(j = 0;j < banks;j++){
                        StatsAddr_32 = (UINT32 *)(pMetaData_AF->pAF_Stats)+j* uwBankSize/4;
                        StatsAddr_64 = (UINT64 *)StatsAddr_32;

                        for(i = 0;i < blocks;i++){
                                index = i+j*banks;
                                pPatched_AFDat->cnt_hor[index] = StatsAddr_32[1];
                                pPatched_AFDat->cnt_ver[index] = StatsAddr_32[0];
                                pPatched_AFDat->filter_value1[index] = StatsAddr_64[3];
                                pPatched_AFDat->filter_value2[index] = StatsAddr_64[4];
                                pPatched_AFDat->Yfactor[index] = StatsAddr_64[1];
                                pPatched_AFDat->fv_hor[index] = StatsAddr_32[5];
                                pPatched_AFDat->fv_ver[index] = StatsAddr_32[4];
                                /*40 Bytes(HW3A AF Block Size) = 10 * UINT32(4 Bytes)*/
                                StatsAddr_32+= 10;
                                /*40 Bytes(HW3A AF Block Size) = 5 * UINT64(8 Bytes)*/
                                StatsAddr_64+= 5; 
                                WRAP_LOG("fv_hor[%d] %d\n",index,pPatched_AFDat->fv_hor[index]);
                        }
                }
        }else if(AL3A_HW3A_DEV_ID_B_0 == pMetaData_AF->uHWengineID){
                for(j = 0; j < banks; j++){
                        StatsAddr_32 = (UINT32 *)(pMetaData_AF->pAF_Stats)+j*uwBankSize/4;
                        for(i = 0;i < blocks;i++){
                                index = i+j*banks;
                                pPatched_AFDat->cnt_hor[index] = StatsAddr_32[1];
                                pPatched_AFDat->cnt_ver[index] = StatsAddr_32[0];
                                pPatched_AFDat->filter_value1[index] = 0;
                                pPatched_AFDat->filter_value2[index] = 0;
                                pPatched_AFDat->Yfactor[index] = StatsAddr_32[2];
                                pPatched_AFDat->fv_hor[index] = StatsAddr_32[4];
                                pPatched_AFDat->fv_ver[index] = StatsAddr_32[3];
                                /*24 Bytes(HW3A AF Block Size) = 6 * UINT32(4 Bytes)*/
                                StatsAddr_32+= 6; 
                                WRAP_LOG("fv_hor[%d] %d\n",index,pPatched_AFDat->fv_hor[index]);
                        }
                }
        }else{
                WRAP_LOG("ERR_WRP_AF_INVALID_ENGINE\n");
                return ERR_WRP_AF_INVALID_ENGINE;
        }

        pPatched_AFDat->AF_token_id = pMetaData_AF->uAFTokenID;
        pPatched_AFDat->hw3a_frame_id = pMetaData_AF->uFrameIdx;
        pPatched_AFDat->time_stamp.time_stamp_sec = pMetaData_AF->systemTime.tv_sec;
        pPatched_AFDat->time_stamp.time_stamp_us = pMetaData_AF->systemTime.tv_usec;
        return ERR_WPR_AF_SUCCESS;
}


/**
\API name: al3AWrapperAF_TranslateFocusModeToAPType
\This API used for translating AF lib focus mode define to framework
\param aFocusMode[In] :   Focus mode define of AF lib (Altek)
\return: Focus mode define for AP framework
*/
UINT32 al3AWrapperAF_TranslateFocusModeToAPType(UINT32 aFocusMode)
{
          UINT32 retAPFocusMode;

          switch(aFocusMode){
/* here just sample code, need to be implement by Framework define
              case alAFLib_AF_MODE_AUTO: 
                  retAPFocusMode = FOCUS_MODE_AUTO;
                  break;
              case alAFLib_AF_MODE_MANUAL: 
                  retAPFocusMode = FOCUS_MODE_MANUAL;
                  break;
              case alAFLib_AF_MODE_MACRO: 
                  retAPFocusMode = FOCUS_MODE_MACRO;
                  break;
              case alAFLib_AF_MODE_OFF: 
                  retAPFocusMode = FOCUS_MODE_OFF;
                  break;
              case alAFLib_AF_MODE_OFF:
                  retAPFocusMode = FOCUS_MODE_FIXED;
                  break;
              case alAFLib_AF_MODE_EDOF:
                  retAPFocusMode = FOCUS_MODE_EDOF;
                  break;
                  */
        default:
                retAPFocusMode = 0;
                break;
        }

        return retAPFocusMode;
}

/**
\API name: al3AWrapperAF_TranslateFocusModeToAFLibType
\This API used for translating framework focus mode to AF lib define
\aFocusMode[In] :   Focus mode define of AP framework define
\return: Focus mode define for AF lib (Altek)
*/
UINT32 al3AWrapperAF_TranslateFocusModeToAFLibType(UINT32 aFocusMode)
{
        UINT32 retAFFocusMode;

        switch(aFocusMode) 
        {
/* here just sample code, need to be implement by Framework define
        
        case FOCUS_MODE_AUTO: 
        case FOCUS_MODE_CONTINUOUS_PICTURE: 
        case FOCUS_MODE_CONTINUOUS_VIDEO:
            retAFFocusMode = alAFLib_AF_MODE_AUTO;
            break;

        case FOCUS_MODE_MANUAL: 
        case FOCUS_MODE_INFINITY:
            retAFFocusMode = alAFLib_AF_MODE_MANUAL;
            break;

        case FOCUS_MODE_MACRO: 
            retAFFocusMode = alAFLib_AF_MODE_MACRO;
            break;

        case FOCUS_MODE_OFF: 
            retAFFocusMode = alAFLib_AF_MODE_OFF;
            break;
        case FOCUS_MODE_FIXED:
            retAFFocusMode = alAFLib_AF_MODE_OFF;
            break;
        case FOCUS_MODE_EDOF:
            retAFFocusMode = alAFLib_AF_MODE_EDOF;
            break;
*/
        default: 
                retAFFocusMode = alAFLib_AF_MODE_NOT_SUPPORTED;
                break;
        }

        return retAFFocusMode;
}

/**
\API name: al3AWrapperAF_TranslateCalibDatToAFLibType
\This API used for translating EEPROM data to AF lib define
\EEPROM_Addr[In] :   EEPROM data address
\AF_Calib_Dat[Out] :   Altek data format 
\return: Error code
*/

UINT32 al3AWrapperAF_TranslateCalibDatToAFLibType(void *EEPROM_Addr,alAFLib_input_init_info_t *AF_Init_Dat)
{
        if(NULL == EEPROM_Addr)
                return ERR_WRP_AF_EMPTY_EEPROM_ADDR;

        if(NULL == AF_Init_Dat)
                return ERR_WRP_AF_EMPTY_INIT_ADDR;

        UINT32 *EEPROM_Addr_32 = (UINT32 *)EEPROM_Addr;

        AF_Init_Dat->calib_data.inf_step = EEPROM_Addr_32[VCM_INF_STEP_ADDR_OFFSET];
        AF_Init_Dat->calib_data.macro_step = EEPROM_Addr_32[VCM_MACRO_STEP_ADDR_OFFSET];
        AF_Init_Dat->calib_data.inf_distance = VCM_INF_STEP_CALIB_DISTANCE;
        AF_Init_Dat->calib_data.macro_distance = VCM_MACRO_STEP_CALIB_DISTANCE;
        AF_Init_Dat->module_info.f_number = MODULE_F_NUMBER;
        AF_Init_Dat->module_info.focal_lenth = MODULE_FOCAL_LENGTH;
        AF_Init_Dat->calib_data.mech_top = VCM_INF_STEP_MECH_TOP;
        AF_Init_Dat->calib_data.mech_bottom = VCM_INF_STEP_MECH_BOTTOM;
        AF_Init_Dat->calib_data.lens_move_stable_time = VCM_MOVING_STATBLE_TIME; 
        /* extra lib calibration data, if not support, set to null.*/
        AF_Init_Dat->calib_data.extend_calib_ptr = NULL;
        /* size of extra lib calibration data, if not support set to zero.*/
        AF_Init_Dat->calib_data.extend_calib_data_size = 0;

        return ERR_WPR_AF_SUCCESS;
}

/**
\API name: al3AWrapperAF_TranslateROIToAFLibType
\This API used for translating ROI info to AF lib define
\frame_id[In] :   Current frame id
\AF_ROI_Info[Out] :   Altek data format 
\return: Error code
*/

UINT32 al3AWrapperAF_TranslateROIToAFLibType(unsigned int frame_id,alAFLib_input_roi_info_t *AF_ROI_Info)
{
/*Sample code for continuous AF default setting in sensor raw size  1280*960 , 30% * 30% crop*/
        AF_ROI_Info->roi_updated = TRUE;
        AF_ROI_Info->type = alAFLib_ROI_TYPE_NORMAL;
        AF_ROI_Info->frame_id = frame_id;
        AF_ROI_Info->num_roi = 1;
        AF_ROI_Info->roi[0].uw_left = 422;
        AF_ROI_Info->roi[0].uw_top = 317;
        AF_ROI_Info->roi[0].uw_dx = 422;
        AF_ROI_Info->roi[0].uw_dy = 317;
        AF_ROI_Info->weight[0] = 1;
        AF_ROI_Info->src_img_sz.uwWidth = 1280;
        AF_ROI_Info->src_img_sz.uwHeight = 960;

        return ERR_WPR_AF_SUCCESS;
}


/**
\API name: al3AWrapperAF_UpdateISPConfig_AF
\This API is used for query ISP config before calling al3AWrapperAF_UpdateISPConfig_AF
\param aAFLibStatsConfig[in]: AF stats config info from AF lib
\param aAFConfig[out]: input buffer, API would manage parameter and return via this pointer 
\return: error code
*/
UINT32 al3AWrapperAF_UpdateISPConfig_AF(alAFLib_af_out_stats_config_t *aAFLibStatsConfig,alHW3a_AF_CfgInfo_t *aAFConfig)
{
        UINT32 ret = ERR_WPR_AF_SUCCESS;
        UINT32 udTemp = 0;
        WRAP_LOG("al3AWrapperAF_UpdateISPConfig_AF start ID %d\n",aAFLibStatsConfig->TokenID);

        aAFConfig->TokenID = aAFLibStatsConfig->TokenID; 

        aAFConfig->tAFRegion.uwBlkNumX = aAFLibStatsConfig->num_blk_hor;
        WRAP_LOG("uwBlkNumX %d\n",aAFConfig->tAFRegion.uwBlkNumX);
        aAFConfig->tAFRegion.uwBlkNumY = aAFLibStatsConfig->num_blk_ver;
        WRAP_LOG("uwBlkNumY%d\n",aAFConfig->tAFRegion.uwBlkNumY);

        udTemp = 100*((UINT32)(aAFLibStatsConfig->roi.uw_dx))/((UINT32)(aAFLibStatsConfig->src_img_sz.uwWidth));
        WRAP_LOG("uw_dx%d uwWidth%d \n",aAFLibStatsConfig->roi.uw_dx,aAFLibStatsConfig->src_img_sz.uwWidth);

        aAFConfig->tAFRegion.uwSizeRatioX = (UINT16)udTemp;
        WRAP_LOG("uwSizeRatioX%d\n",aAFConfig->tAFRegion.uwSizeRatioX);
        udTemp = 100*((UINT32)(aAFLibStatsConfig->roi.uw_dy))/((UINT32)(aAFLibStatsConfig->src_img_sz.uwHeight));
        aAFConfig->tAFRegion.uwSizeRatioY = (UINT16)udTemp;
        WRAP_LOG("uwSizeRatioY%d\n",aAFConfig->tAFRegion.uwSizeRatioY);
        udTemp = 100*((UINT32)(aAFLibStatsConfig->roi.uw_left))/((UINT32)(aAFLibStatsConfig->src_img_sz.uwWidth));
        aAFConfig->tAFRegion.uwOffsetRatioX = (UINT16)udTemp;
        WRAP_LOG("uwOffsetRatioX%d\n",aAFConfig->tAFRegion.uwOffsetRatioX);
        udTemp = 100*((UINT32)(aAFLibStatsConfig->roi.uw_top))/((UINT32)(aAFLibStatsConfig->src_img_sz.uwHeight));
        aAFConfig->tAFRegion.uwOffsetRatioY = (UINT16)udTemp;
        WRAP_LOG("uwOffsetRatioY%d\n",aAFConfig->tAFRegion.uwOffsetRatioY);
        aAFConfig->bEnableAFLUT = aAFLibStatsConfig->bEnableAFLUT;
        WRAP_LOG("bEnableAFLUT%d\n",aAFConfig->bEnableAFLUT);
        memcpy(aAFConfig->auwLUT,aAFLibStatsConfig->auwLUT,sizeof(UINT16)*259);
        memcpy(aAFConfig->auwAFLUT,aAFLibStatsConfig->auwAFLUT,sizeof(UINT16)*259);
        memcpy(aAFConfig->aucWeight,aAFLibStatsConfig->aucWeight,sizeof(UINT8)*6);
        aAFConfig->uwSH = aAFLibStatsConfig->uwSH;
        WRAP_LOG("uwSH%d\n",aAFConfig->uwSH);
        aAFConfig->ucThMode = aAFLibStatsConfig->ucThMode;
        WRAP_LOG("ucThMode%d\n",aAFConfig->ucThMode);
        memcpy(aAFConfig->aucIndex,aAFLibStatsConfig->aucIndex,sizeof(UINT8)*82);
        memcpy(aAFConfig->auwTH,aAFLibStatsConfig->auwTH,sizeof(UINT16)*4);
        memcpy(aAFConfig->pwTV,aAFLibStatsConfig->pwTV,sizeof(UINT16)*4);
        aAFConfig->udAFoffset = aAFLibStatsConfig->udAFoffset;
        WRAP_LOG("udAFoffset%d\n",aAFConfig->udAFoffset);
        aAFConfig->bAF_PY_Enable = aAFLibStatsConfig->bAF_PY_Enable;
        WRAP_LOG("bAF_PY_Enable%d\n",aAFConfig->bAF_PY_Enable);
        aAFConfig->bAF_LPF_Enable = aAFLibStatsConfig->bAF_LPF_Enable;
        WRAP_LOG("bAF_LPF_Enable%d\n",aAFConfig->bAF_LPF_Enable);

        switch(aAFLibStatsConfig->nFilterMode) {
        case MODE_51:
                aAFConfig->nFilterMode = HW3A_MF_51_MODE;
                break;
        case MODE_31:
                aAFConfig->nFilterMode = HW3A_MF_31_MODE;
                break;
        case DISABLE:
                default:
                aAFConfig->nFilterMode = HW3A_MF_DISABLE;
                break;
        }

        WRAP_LOG("nFilterMode%d\n",aAFConfig->nFilterMode);
        aAFConfig->ucFilterID = aAFLibStatsConfig->ucFilterID;
        WRAP_LOG("ucFilterID%d\n",aAFConfig->ucFilterID );
        aAFConfig->uwLineCnt = aAFLibStatsConfig->uwLineCnt;
        WRAP_LOG("uwLineCnt%d\n",aAFConfig->uwLineCnt);

        return ret;
}

/**
\API name: al3AWrapper_UpdateAFReport
\This API is used for translate AF status  to generate AF report for AP
\param aAFLibStatus[in]: af status from AF update
\param aAFReport[out]: AF report for AP 
\return: error code
*/
UINT32 al3AWrapper_UpdateAFReport(alAFLib_af_out_status_t *aAFLibStatus,af_report_update_t *aAFReport)
{
        UINT32 ret = ERR_WPR_AF_SUCCESS;

        if(NULL == aAFLibStatus)
                return ERR_WRP_AF_EMPTY_AF_STATUS;

        aAFReport->focus_done = aAFLibStatus->focus_done;
        aAFReport->status = aAFLibStatus->t_status;
        aAFReport->f_distance = aAFLibStatus->f_distance;

        return ret;
}

/**
\API name: al3AWrapperAF_TranslateAEInfoToAFLibType
\This API used for translating AE info to AF lib define
\ae_report_update_t[In] :   Iput AE data
\alAFLib_input_aec_info_t[Out] :   Altek data format 
\return: Error code
*/
UINT32 al3AWrapperAF_TranslateAEInfoToAFLibType(ae_report_update_t *aAEReport,alAFLib_input_aec_info_t *aAEInfo)
{
        if(NULL == aAEReport)
            return ERR_WRP_AF_EMPTY_AE_INFO_INPUT;

        if(NULL == aAEInfo)
            return ERR_WRP_AF_EMPTY_AF_INFO_OUTPUT;

        aAEInfo->ae_settled = aAEReport->ae_converged;
        aAEInfo->cur_intensity = (float)(aAEReport->curmean);
        aAEInfo->target_intensity = (float)(aAEReport->targetmean);

        if(aAEReport->bv_val+5000 > 30000)
                aAEInfo->brightness = 30000;
        else if(aAEReport->bv_val-5000 <-30000)
                aAEInfo->brightness =-30000;
        else
                aAEInfo->brightness = (short)(aAEReport->bv_val+5000);

        aAEInfo->preview_fr = aAEReport->cur_fps;
        aAEInfo->cur_gain = (float)(aAEReport->sensor_ad_gain);

        return ERR_WPR_AF_SUCCESS;
}

/**
\API name: al3AWrapperAF_GetDefaultCfg
\This API is used for query default ISP config before calling al3AWrapperAF_UpdateISPConfig_AF
\param aAFConfig[out]: input buffer, API would manage parameter and return via this pointer 
\return: error code
*/

UINT32 al3AWrapperAF_GetDefaultCfg(alHW3a_AF_CfgInfo_t *aAFConfig)
{
        UINT32 ret = ERR_WPR_AF_SUCCESS;
        UINT8 aucAFblk_ThIdx[82] = AF_DEFAULT_AFBLK_TH;
        UINT16 auwAF_TH_Lut[4] = AF_DEFAULT_THLUT;
        UINT8 aucWei[6] = AF_DEFAULT_MASK;
        UINT16 auwAF_Lut[259] = AF_DEFAULT_LUT;

        if(aAFConfig == NULL)
                return ERR_WRP_AF_INVALID_INPUT_PARAM;

        aAFConfig->TokenID = 0x01;
        aAFConfig->tAFRegion.uwBlkNumX = 3;
        aAFConfig->tAFRegion.uwBlkNumY = 3;
        aAFConfig->tAFRegion.uwSizeRatioX = 33;
        aAFConfig->tAFRegion.uwSizeRatioY = 33;
        aAFConfig->tAFRegion.uwOffsetRatioX = 33;
        aAFConfig->tAFRegion.uwOffsetRatioY = 33;
        aAFConfig->bEnableAFLUT = FALSE;
        memcpy(aAFConfig->auwLUT,auwAF_Lut,sizeof(UINT16)*259);
        memcpy(aAFConfig->auwAFLUT,auwAF_Lut,sizeof(UINT16)*259);
        memcpy(aAFConfig->aucWeight,aucWei,sizeof(UINT8)*6);
        aAFConfig->uwSH = 0;
        aAFConfig->ucThMode = 0;
        memcpy(aAFConfig->aucIndex,aucAFblk_ThIdx,sizeof(UINT8)*82);
        memcpy(aAFConfig->auwTH,auwAF_TH_Lut,sizeof(UINT16)*4);
        memcpy(aAFConfig->pwTV,auwAF_TH_Lut,sizeof(UINT16)*4);
        aAFConfig->udAFoffset = 0x80;
        aAFConfig->bAF_PY_Enable = FALSE;
        aAFConfig->bAF_LPF_Enable = FALSE;
        aAFConfig->nFilterMode = HW3A_MF_DISABLE;
        aAFConfig->ucFilterID = 0;
        aAFConfig->uwLineCnt = 1;

        return ret;
}


UINT32 al3AWrapperAF_TranslateImageBufInfoToAFLibType(void *aImgBuf,alAFLib_input_image_buf_t *aAFImageBuf);

