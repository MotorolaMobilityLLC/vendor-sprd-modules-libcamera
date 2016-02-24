////////////////////////////////////////////////////////////////////
//  File name: al3AWrapper.h
//  Create Date:
//
//  Comment:
//
//
////////////////////////////////////////////////////////////////////
#ifndef _AL_3AWRAPPER_H_
#define _AL_3AWRAPPER_H_

#ifdef LOCAL_NDK_BUILD   // test build in local
#include ".\..\..\INCLUDE\mtype.h"
#include ".\..\..\INCLUDE\FrmWk_HW3A_event_type.h"
#include ".\..\..\INCLUDE\HW3A_Stats.h"
#else  // normal release in AP
#include "mtype.h"
#include "FrmWk_HW3A_event_type.h"
#include "HW3A_Stats.h"
#endif
#include <sys/time.h>   // for timestamp calling

#ifdef __cplusplus
extern "C"
{
#endif

#define _WRAPPER_VER 0.8000

/**
\API name: al3AWrapper_DispatchHW3AStats
\This API used for copying stats data from HW ISP(Altek) to seperated buffer, but without further patching
\Framework should call al3AWrapper_DispatchHW3A_XXStats in certain thread for patching, after patching completed, send event
\to XX Ctrl layer, prepare for process
\param alISP_Metadata[In] :   meta data address from ISP driver, passing via AP framework
\param alISP_MetaData_AE[Out] : AE stats buffer addr, should be arranged via AE ctrl/3A ctrl layer
\param alISP_MetaData_AWB[Out] : AWB stats buffer addr, should be arranged via AWB ctrl/3A ctrl layer
\param alISP_MetaData_AF[Out] : AF stats buffer addr, should be arranged via AF ctrl/3A ctrl layer
\param alISP_MetaData_YHist[Out] : YHist stats buffer addr, should be arranged via 3A ctrl layer
\param alISP_MetaData_AntiF[Out] : AntiFlicker stats buffer addr, should be arranged via anti-flicker ctrl/3A ctrl layer
\param alISP_MetaData_Subsample[Out] : subsample buffer addr, should be arranged via AF ctrl/3A ctrl layer
\param udSOF_Idx[In] : current SOF index, should be from ISP driver layer
\ return: error code
*/
UINT32 al3AWrapper_DispatchHW3AStats( void * alISP_Metadata, ISP_DRV_META_AE_t* alISP_MetaData_AE, ISP_DRV_META_AWB_t* alISP_MetaData_AWB, ISP_DRV_META_AF_t * alISP_MetaData_AF, ISP_DRV_META_YHist_t * alISP_MetaData_YHist,
                                      ISP_DRV_META_AntiF_t * alISP_MetaData_AntiF, ISP_DRV_META_Subsample_t * alISP_MetaData_Subsample, UINT32 udSOF_Idx );

/**
\API name: al3AWrapper_GetCurrentDLSequence
\Comments:
\This API is used for AP, to set correct DL sequence setting (both for basic/advanced)
\Parameter:
\ucAHBSensoreID[In]: used for operating AHB HW channel
\aDldSequence[Out]: prepared for setup to AHB HW, help to schedule correct 3A HW stats output
\aIsSingle3AMode[In]: 0: DL all 3A (AE or AWB or AF ) stats at same frame, used for rear camera
\                     1: DL single 3A (AE/AWB/AF) stats per frame, follow aDldSequence setting, used for front camera
\opMode[In]: 0: normal LV, (default value)
\            1: AF, flash AF
\            2: flash AE
\return: error code
*/
UINT32 al3AWrapper_GetCurrentDLSequence( UINT8 ucAHBSensoreID, alISP_DldSequence_t* aDldSequence, UINT8 aIsSingle3AMode, alISP_OPMODE_IDX_t opMode );

/**
\API name: al3AWrapper_SetDLSequence
\Commets:
\This API would update to Altek ISP via ISP driver layer
\AP should call al3AWrapper_GetCurrentDLSequence before set to HW
\param aDldSequence[In]
\return: error code
*/
UINT32 al3AWrapper_SetDLSequence( alISP_DldSequence_t aDldSequence );

/**
\API name: al3AWrapper_GetVersion
\This API would return labeled version of wrapper
\fWrapVersion[out], return current wapper version
\return: error code
*/
UINT32 al3AWrapper_GetVersion( float *fWrapVersion );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif // _AL_3AWRAPPER_H_