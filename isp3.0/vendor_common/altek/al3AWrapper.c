/*
 * al3AWrapper.c
 *
 *  Created on: 2015/12/05
 *      Author: MarkTseng
 *  Latest update: 2016/2/17
 *      Reviser: MarkTseng
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Dispatch ISP stats to seperated stats
 *       2. Get DL seuqence by opmode
 *       3. Set DL sequence to ISP (Altek)
 */


#include <math.h>   // math lib, depends on AP OS
#include <string.h>
#include <sys/time.h>   // for timestamp calling

#ifdef  LOCAL_NDK_BUILD  // test build in local
#include ".\..\..\INCLUDE\mtype.h"
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"
#include "al3AWrapper.h"
#include "al3AWrapperErrCode.h"
#else  // normal release in AP
#include "mtype.h"
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"
#include "al3AWrapper.h"
#include "al3AWrapperErrCode.h"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// function prototype


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Function code
//

/*  for 3A ctrl layer */
UINT32 al3AWrapper_DispatchHW3AStats( void * alISP_Metadata, ISP_DRV_META_AE_t* alISP_MetaData_AE, ISP_DRV_META_AWB_t* alISP_MetaData_AWB, ISP_DRV_META_AF_t * alISP_MetaData_AF, ISP_DRV_META_YHist_t * alISP_MetaData_YHist,
                                      ISP_DRV_META_AntiF_t * alISP_MetaData_AntiF, ISP_DRV_META_Subsample_t * alISP_MetaData_Subsample, UINT32 udSOF_Idx  )
{
    UINT32 ret = ERR_WRP_SUCCESS;
    UINT8 *pAddrLocal;
    ISP_DRV_META_t pISPMeta;
    ISP_DRV_META_AE_t *pISPMetaAE;
    ISP_DRV_META_AWB_t *pISPMetaAWB;
    ISP_DRV_META_AF_t *pISPMetaAF;
    ISP_DRV_META_YHist_t *pISPMetaYHist;
    ISP_DRV_META_AntiF_t *pISPMetaAntiF;
    ISP_DRV_META_Subsample_t *pISPMetaSubsample;

    UINT8 *pTempAddr;
    UINT32 offset_start, offset, padding;

    struct timeval systemTime;

    // store timestamp at beginning, for AF usage
    gettimeofday(&systemTime, NULL);

    pAddrLocal = (UINT8 *)alISP_Metadata;
    // by byte parsing, use casting would cause some compiler padding shift issue
    /* common Info */
    memcpy( &pISPMeta.uMagicNum        , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uHWengineID      , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uFrameIdx        , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uCheckSum        , pAddrLocal, 4 );
    pAddrLocal+= 4;
    /*  AE  stats info */
    memcpy( &pISPMeta.uAETag           , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAETokenID       , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAEStatsSize     , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uAEStatsAddr     , pAddrLocal, 4 );
    pAddrLocal+= 4;
    /*   AWB stats info */
    memcpy( &pISPMeta.uAWBTag          , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAWBTokenID      , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAWBStatsSize    , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uAWBStatsAddr    , pAddrLocal, 4 );
    pAddrLocal+= 4;
    /*   AF stats info  */
    memcpy( &pISPMeta.uAFTag           , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAFTokenID       , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAFStatsSize     , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uAFStatsAddr     , pAddrLocal, 4 );
    pAddrLocal+= 4;
    /*   Y-Hist stats info  */
    memcpy( &pISPMeta.uYHistTag        , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uYHistTokenID    , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uYHistStatsSize  , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uYHistStatsAddr  , pAddrLocal, 4 );
    pAddrLocal+= 4;
    /*   Anfi-flicker stats info */
    memcpy( &pISPMeta.uAntiFTag        , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAntiFTokenID    , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uAntiFStatsSize  , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uAntiFStatsAddr  , pAddrLocal, 4 );
    pAddrLocal+= 4;
    /*   subsample image info   */
    memcpy( &pISPMeta.uSubsampleTag        , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uSubsampleTokenID    , pAddrLocal, 2 );
    pAddrLocal+= 2;
    memcpy( &pISPMeta.uSubsampleStatsSize  , pAddrLocal, 4 );
    pAddrLocal+= 4;
    memcpy( &pISPMeta.uSubsampleStatsAddr  , pAddrLocal, 4 );
    pAddrLocal+= 4;

    // check HW3A stats define version with magic number, which should be maintained & update at each ISP release
    if ( pISPMeta.uMagicNum != HW3A_MAGIC_NUMBER_VERSION )
        ret =  ERR_WRP_MISMACTHED_STATS_VER;    // keep error code only in early phase, if need return immediatedly then return here

    // parsing AE if AE pointer is valid
    if ( alISP_MetaData_AE == NULL )
        ; // do nothing, even if HW3A stats passing valid data
    else if ( pISPMeta.uAEStatsSize != 0 && pISPMeta.uAEStatsAddr != 0 ) {
        pISPMetaAE = (ISP_DRV_META_AE_t *)alISP_MetaData_AE;
        // udpate common info
        pISPMetaAE->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaAE->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaAE->uFrameIdx = pISPMeta.uFrameIdx;

        // update AE info
        pISPMetaAE->uAETokenID = pISPMeta.uAETokenID;
        pISPMetaAE->uAEStatsSize = pISPMeta.uAEStatsSize;
        pISPMetaAE->uPseudoFlag  = 0;

        memcpy( &pISPMetaAE->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaAE->udsys_sof_idx = udSOF_Idx;

        // retriving AE info of stats
        pAddrLocal = (UINT8 *)alISP_Metadata + pISPMeta.uAEStatsAddr;
        offset = 0;
        memcpy( &alISP_MetaData_AE->ae_stats_info.udPixelsPerBlocks  , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+=4;
        memcpy( &alISP_MetaData_AE->ae_stats_info.udBankSize         , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+=4;
        memcpy( &alISP_MetaData_AE->ae_stats_info.ucValidBlocks      , pAddrLocal, 1 );
        pAddrLocal+= 1;
        offset+=1;
        memcpy( &alISP_MetaData_AE->ae_stats_info.ucValidBanks       , pAddrLocal, 1 );
        pAddrLocal+= 1;
        offset+=1;

        // copy AE stats to indicated buffer address, make 8 alignment
        offset_start = pISPMeta.uAEStatsAddr + offset; // 10 is accumulation value of AE info of stats, 4+4+1+1
        padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
        offset_start = offset_start + padding;

        // allocate memory buffer base on meta size of AE stats
        // if ( alISP_MetaData_AE->uAEStatsSize- ( offset+ padding) > HW3A_AE_STATS_BUFFER_SIZE )
        if ( alISP_MetaData_AE->uAEStatsSize > HW3A_AE_STATS_BUFFER_SIZE ) {
            return ERR_WRP_ALLOCATE_BUFFER;
        }

        // shift to data addr
        pTempAddr = (UINT8 *)alISP_Metadata + offset_start;
        // copy data to buffer addr
        memcpy( (void *)alISP_MetaData_AE->pAE_Stats , (void *)pTempAddr, alISP_MetaData_AE->uAEStatsSize  );
    } else { // set pseudo flag, for AE lib progressive runing
        pISPMetaAE = (ISP_DRV_META_AE_t *)alISP_MetaData_AE;
        // udpate common info
        pISPMetaAE->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaAE->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaAE->uFrameIdx = pISPMeta.uFrameIdx;

        // update AE info
        pISPMetaAE->uAETokenID = pISPMeta.uAETokenID;
        pISPMetaAE->uAEStatsSize = pISPMeta.uAEStatsSize;
        pISPMetaAE->uPseudoFlag = 1;

        memcpy( &pISPMetaAE->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaAE->udsys_sof_idx = udSOF_Idx;
    }

    // parsing AWB if AWB pointer is valid
    if ( alISP_MetaData_AWB == NULL )
        ; // do nothing, even if HW3A stats passing valid data
    else if ( pISPMeta.uAWBStatsSize != 0 && pISPMeta.uAWBStatsAddr != 0 ) {
        pISPMetaAWB = (ISP_DRV_META_AWB_t *)alISP_MetaData_AWB;
        // udpate common info
        pISPMetaAWB->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaAWB->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaAWB->uFrameIdx = pISPMeta.uFrameIdx;

        // update AWB info
        pISPMetaAWB->uAWBTokenID = pISPMeta.uAWBTokenID;
        pISPMetaAWB->uAWBStatsSize = pISPMeta.uAWBStatsSize;
        pISPMetaAWB->uPseudoFlag  = 0;

        memcpy( &pISPMetaAWB->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaAWB->udsys_sof_idx = udSOF_Idx;


        // retriving AWB info of stats
        pAddrLocal = (UINT8 *)alISP_Metadata + pISPMeta.uAWBStatsAddr;
        offset = 0;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.udPixelsPerBlocks  , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+= 4;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.udBankSize         , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+= 4;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwSub_x            , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwSub_y            , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwWin_x            , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwWin_y            , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwWin_w            , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwWin_h            , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwTotalCount       , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.uwGrassCount       , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.ucValidBlocks      , pAddrLocal, 1 );
        pAddrLocal+= 1;
        offset+= 1;
        memcpy( &alISP_MetaData_AWB->awb_stats_info.ucValidBanks       , pAddrLocal, 1 );
        pAddrLocal+= 1;
        offset+= 1;

        // copy AWB stats to indicated buffer address, make 8 alignment
        offset_start = pISPMeta.uAWBStatsAddr + offset; // offset is accumulation value of AWB info of stats
        padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
        offset_start = offset_start + padding;

        // allocate memory buffer base on meta size of AWB stats
        if ( alISP_MetaData_AWB->uAWBStatsSize > HW3A_AWB_STATS_BUFFER_SIZE  ) {
            return ERR_WRP_ALLOCATE_BUFFER;
        }

        // shift to data addr
        pTempAddr = (UINT8 *)alISP_Metadata + offset_start;
        // copy data to buffer addr
        memcpy( (void *)alISP_MetaData_AWB->pAWB_Stats , (void *)pTempAddr, alISP_MetaData_AWB->uAWBStatsSize );
    } else { // set pseudo flag, for AWB lib progressive runing
        pISPMetaAWB = (ISP_DRV_META_AWB_t *)alISP_MetaData_AWB;
        // udpate common info
        pISPMetaAWB->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaAWB->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaAWB->uFrameIdx = pISPMeta.uFrameIdx;

        // update AWB info
        pISPMetaAWB->uAWBTokenID = pISPMeta.uAWBTokenID;
        pISPMetaAWB->uAWBStatsSize = pISPMeta.uAWBStatsSize;
        pISPMetaAWB->uPseudoFlag  = 1;

        memcpy( &pISPMetaAWB->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaAWB->udsys_sof_idx = udSOF_Idx;
    }

    // parsing AF if AF pointer is valid
    if ( alISP_MetaData_AF == NULL )
        ; // do nothing, even if HW3A stats passing valid data
    else if ( pISPMeta.uAFStatsSize != 0 && pISPMeta.uAFStatsAddr != 0 ) {
        pISPMetaAF = (ISP_DRV_META_AF_t *)alISP_MetaData_AF;
        // udpate common info
        pISPMetaAF->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaAF->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaAF->uFrameIdx = pISPMeta.uFrameIdx;

        // update AF info
        pISPMetaAF->uAFTokenID = pISPMeta.uAFTokenID;
        pISPMetaAF->uAFStatsSize = pISPMeta.uAFStatsSize;

        memcpy( &pISPMetaAF->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaAF->udsys_sof_idx = udSOF_Idx;

        // retriving AF info of stats
        pAddrLocal = (UINT8 *)alISP_Metadata + pISPMeta.uAFStatsAddr;
        offset = 0;
        memcpy( &alISP_MetaData_AF->af_stats_info.udPixelsPerBlocks  , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+= 4;
        memcpy( &alISP_MetaData_AF->af_stats_info.udBankSize         , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+= 4;
        memcpy( &alISP_MetaData_AF->af_stats_info.ucValidBlocks      , pAddrLocal, 1 );
        pAddrLocal+= 1;
        offset+= 1;
        memcpy( &alISP_MetaData_AF->af_stats_info.ucValidBanks       , pAddrLocal, 1 );
        pAddrLocal+= 1;
        offset+= 1;

        // copy AF stats to indicated buffer address, make 8 alignment
        offset_start = pISPMeta.uAFStatsAddr + offset; // offset is accumulation value of AF info of stats
        padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
        offset_start = offset_start + padding;

        // allocate memory buffer base on meta size of AF stats
        if ( alISP_MetaData_AF->uAFStatsSize > HW3A_AF_STATS_BUFFER_SIZE ) {
            return ERR_WRP_ALLOCATE_BUFFER;
        }

        // shift to data addr
        pTempAddr = (UINT8 *)alISP_Metadata + offset_start;
        // copy data to buffer addr
        memcpy( (void *)alISP_MetaData_AF->pAF_Stats , (void *)pTempAddr, alISP_MetaData_AF->uAFStatsSize );
    }

    // parsing YHist if YHist pointer is valid
    if ( alISP_MetaData_YHist == NULL )
        ; // do nothing, even if HW3A stats passing valid data
    else if ( pISPMeta.uYHistStatsSize != 0 && pISPMeta.uYHistStatsAddr != 0 ) {
        pISPMetaYHist = (ISP_DRV_META_YHist_t *)alISP_MetaData_YHist;
        // udpate common info
        pISPMetaYHist->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaYHist->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaYHist->uFrameIdx = pISPMeta.uFrameIdx;

        // update YHist info
        pISPMetaYHist->uYHistTokenID = pISPMeta.uYHistTokenID;
        pISPMetaYHist->uYHistStatsSize = pISPMeta.uYHistStatsSize;

        memcpy( &pISPMetaYHist->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaYHist->udsys_sof_idx = udSOF_Idx;

        // retriving YHist info of stats
        pAddrLocal = (UINT8 *)alISP_Metadata + pISPMeta.uYHistStatsAddr;
        offset = 0;

        // copy YHist stats to indicated buffer address, make 8 alignment
        offset_start = pISPMeta.uYHistStatsAddr + offset; // offset is accumulation value of YHist info of stats
        padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
        offset_start = offset_start + padding;

        // allocate memory buffer base on meta size of YHist stats
        if ( alISP_MetaData_YHist->uYHistStatsSize > HW3A_YHIST_STATS_BUFFER_SIZE ) {
            return ERR_WRP_ALLOCATE_BUFFER;
        }

        // shift to data addr
        pTempAddr = (UINT8 *)alISP_Metadata + offset_start;
        // copy data to buffer addr
        memcpy( (void *)alISP_MetaData_YHist->pYHist_Stats , (void *)pTempAddr, alISP_MetaData_YHist->uYHistStatsSize );
    }

    // parsing AntiF if AntiF pointer is valid
    if ( alISP_MetaData_AntiF == NULL )
        ; // do nothing, even if HW3A stats passing valid data
    else if ( pISPMeta.uAntiFStatsSize != 0 && pISPMeta.uAntiFStatsAddr != 0 ) {
        pISPMetaAntiF = (ISP_DRV_META_AntiF_t *)alISP_MetaData_AntiF;
        // udpate common info
        pISPMetaAntiF->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaAntiF->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaAntiF->uFrameIdx = pISPMeta.uFrameIdx;

        // update AntiF info
        pISPMetaAntiF->uAntiFTokenID = pISPMeta.uAntiFTokenID;
        pISPMetaAntiF->uAntiFStatsSize = pISPMeta.uAntiFStatsSize;

        memcpy( &pISPMetaAntiF->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaAntiF->udsys_sof_idx = udSOF_Idx;

        // retriving AntiF info of stats
        pAddrLocal = (UINT8 *)alISP_Metadata + pISPMeta.uAntiFStatsAddr;
        offset = 0;

        // copy AntiF stats to indicated buffer address, make 8 alignment
        offset_start = pISPMeta.uAntiFStatsAddr + offset; // offset is accumulation value of AntiF info of stats
        padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
        offset_start = offset_start + padding;

        // allocate memory buffer base on meta size of AntiF stats
        if ( alISP_MetaData_AntiF->uAntiFStatsSize > HW3A_ANTIF_STATS_BUFFER_SIZE ) {
            return ERR_WRP_ALLOCATE_BUFFER;
        }

        // shift to data addr
        pTempAddr = (UINT8 *)alISP_Metadata + offset_start;
        // copy data to buffer addr
        memcpy( (void *)alISP_MetaData_AntiF->pAntiF_Stats , (void *)pTempAddr, alISP_MetaData_AntiF->uAntiFStatsSize );
    }

    // parsing Subsample if Subsample pointer is valid
    if ( alISP_MetaData_Subsample == NULL )
        ; // do nothing, even if HW3A stats passing valid data
    else if ( pISPMeta.uSubsampleStatsSize != 0 && pISPMeta.uSubsampleStatsAddr != 0 ) {
        pISPMetaSubsample = (ISP_DRV_META_Subsample_t *)alISP_MetaData_Subsample;
        // udpate common info
        pISPMetaSubsample->uMagicNum = pISPMeta.uMagicNum;
        pISPMetaSubsample->uHWengineID = pISPMeta.uHWengineID;
        pISPMetaSubsample->uFrameIdx = pISPMeta.uFrameIdx;

        // update Subsample info
        pISPMetaSubsample->uSubsampleTokenID = pISPMeta.uSubsampleTokenID;
        pISPMetaSubsample->uSubsampleStatsSize = pISPMeta.uSubsampleStatsSize;

        memcpy( &pISPMetaSubsample->systemTime, &systemTime, sizeof(struct timeval));
        pISPMetaSubsample->udsys_sof_idx = udSOF_Idx;

        // retriving Subsample info of stats
        pAddrLocal = (UINT8 *)alISP_Metadata + pISPMeta.uSubsampleStatsAddr;
        offset = 0;
        memcpy( &alISP_MetaData_Subsample->Subsample_stats_info.udBufferTotalSize  , pAddrLocal, 4 );
        pAddrLocal+= 4;
        offset+= 4;
        memcpy( &alISP_MetaData_Subsample->Subsample_stats_info.ucValidW           , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;
        memcpy( &alISP_MetaData_Subsample->Subsample_stats_info.ucValidH           , pAddrLocal, 2 );
        pAddrLocal+= 2;
        offset+= 2;

        // copy Subsample stats to indicated buffer address, make 8 alignment
        offset_start = pISPMeta.uSubsampleStatsAddr + offset; // offset is accumulation value of Subsample info of stats
        padding = (offset_start % 8 == 0 ) ? 0:(8- (offset_start % 8));
        offset_start = offset_start + padding;

        // allocate memory buffer base on meta size of Subsample stats
        if ( alISP_MetaData_Subsample->uSubsampleStatsSize > HW3A_SUBIMG_STATS_BUFFER_SIZE ) {
            return ERR_WRP_ALLOCATE_BUFFER;
        }

        // shift to data addr
        pTempAddr = (UINT8 *)alISP_Metadata + offset_start;
        // copy data to buffer addr
        memcpy( (void *)alISP_MetaData_Subsample->pSubsample_Stats , (void *)pTempAddr, alISP_MetaData_Subsample->uSubsampleStatsSize );
    }

    return ret;
}

/* DL sequence query API */
UINT32 al3AWrapper_GetCurrentDLSequence( UINT8 ucAHBSensoreID, alISP_DldSequence_t* aDldSequence, UINT8 aIsSingle3AMode, alISP_OPMODE_IDX_t opMode )
{
    UINT32 ret = ERR_WRP_SUCCESS;
    alISP_DldSequence_t* aDLSelist;

    if ( aDldSequence == NULL )
        return ERR_WRP_INVALID_DL_PARAM;

    aDLSelist = (alISP_DldSequence_t* )aDldSequence;

    aDLSelist->ucAHBSensoreID = ucAHBSensoreID;  //fulfill AHB sensor index which would be used when calling al3AWrapper_SetDLSequence

    switch ( opMode ) {
    case OPMODE_NORMALLV:
        // W9 config
        aDLSelist->ucPreview_Baisc_DldSeqLength = 4;
        aDLSelist->aucPreview_Baisc_DldSeq[0] = HA3ACTRL_B_DL_TYPE_AE;
        aDLSelist->aucPreview_Baisc_DldSeq[1] = HA3ACTRL_B_DL_TYPE_AWB;
        aDLSelist->aucPreview_Baisc_DldSeq[2] = HA3ACTRL_B_DL_TYPE_AF;
        aDLSelist->aucPreview_Baisc_DldSeq[3] = HA3ACTRL_B_DL_TYPE_AWB;
        // W10 config
        aDLSelist->ucPreview_Adv_DldSeqLength = 1;
        aDLSelist->aucPreview_Adv_DldSeq[0] = HA3ACTRL_B_DL_TYPE_AntiF;
        // W9
        aDLSelist->ucFastConverge_Baisc_DldSeqLength = 0;
        aDLSelist->aucFastConverge_Baisc_DldSeq[0] = HA3ACTRL_B_DL_TYPE_NONE;
        // W10

        break;
    case OPMODE_AF_FLASH_AF:
        // W9 config
        aDLSelist->ucPreview_Baisc_DldSeqLength = 2;
        aDLSelist->aucPreview_Baisc_DldSeq[0] = HA3ACTRL_B_DL_TYPE_AF;
        aDLSelist->aucPreview_Baisc_DldSeq[1] = HA3ACTRL_B_DL_TYPE_AF;
        // W10 config
        aDLSelist->ucPreview_Adv_DldSeqLength = 1;
        aDLSelist->aucPreview_Adv_DldSeq[0] = HA3ACTRL_B_DL_TYPE_Sub;

        aDLSelist->ucFastConverge_Baisc_DldSeqLength = 0;
        aDLSelist->aucFastConverge_Baisc_DldSeq[0] = HA3ACTRL_B_DL_TYPE_NONE;
        break;
    case OPMODE_FLASH_AE:
        // W9 config
        aDLSelist->ucPreview_Baisc_DldSeqLength = 2;
        aDLSelist->aucPreview_Baisc_DldSeq[0] = HA3ACTRL_B_DL_TYPE_AE;
        aDLSelist->aucPreview_Baisc_DldSeq[1] = HA3ACTRL_B_DL_TYPE_AWB;
        // W10 config
        aDLSelist->ucPreview_Adv_DldSeqLength = 1;
        aDLSelist->aucPreview_Adv_DldSeq[0] = HA3ACTRL_B_DL_TYPE_AntiF;

        aDLSelist->ucFastConverge_Baisc_DldSeqLength = 0;
        aDLSelist->aucFastConverge_Baisc_DldSeq[0] = HA3ACTRL_B_DL_TYPE_NONE;
        break;

    default:
        return ERR_WRP_INVALID_DL_OPMODE;
        break;
    }
    return ret;
}

// should include ISPDRV_SetBasicPreivewDldSeq/ISPDRV_SetAdvPreivewDldSeq/ISPDRV_SetBasicFastConvergeDldSeq from ISP_Driver layer
UINT32 al3AWrapper_SetDLSequence( alISP_DldSequence_t aDldSequence )
{
    UINT32 ret = ERR_WRP_SUCCESS;
    UINT8 ucAHBSensoreID, ucIsSingle3AMode;

    ucAHBSensoreID = aDldSequence.ucAHBSensoreID;
#if 0//#ifndef LOCAL_NDK_BUILD   // test build in local
    // W9 config
    ret = ISPDRV_SetBasicPreivewDldSeq( ucAHBSensoreID, (UINT8 *)(&aDldSequence.aucPreview_Baisc_DldSeq[0]), aDldSequence.ucPreview_Baisc_DldSeqLength );
    if ( ret!= ERR_WRP_SUCCESS )
        return ret;
    // W10 Config
    ret = ISPDRV_SetAdvPreivewDldSeq( ucAHBSensoreID, (UINT8 *)(&aDldSequence.aucPreview_Adv_DldSeq[0]), aDldSequence.ucPreview_Adv_DldSeqLength );
    if ( ret!= ERR_WRP_SUCCESS )
        return ret;
    // Fast W9 config
    ret = ISPDRV_SetBasicFastConvergeDldSeq( ucAHBSensoreID, (UINT8 *)(&aDldSequence.aucPreview_Baisc_DldSeq[0]), aDldSequence.ucPreview_Baisc_DldSeqLength );
    if ( ret!= ERR_WRP_SUCCESS )
        return ret;
#endif
    return ret;
}

/**
\API name: al3AWrapper_GetVersion
\This API would return labeled version of wrapper
\fWrapVersion[out], return current wapper version
\return: error code
*/
UINT32 al3AWrapper_GetVersion( float *fWrapVersion )
{
	UINT32 ret = ERR_WRP_SUCCESS;

	*fWrapVersion = _WRAPPER_VER;

	return ret;
}