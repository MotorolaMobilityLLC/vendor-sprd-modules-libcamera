/******************************************************************************
*  File name: alwrapper_3a.h
*  Create Date:
*
*  Comment:
*  3A main wrapper, used to seperat packed stats to single
*  stats
 ******************************************************************************/

#ifndef _AL_3AWRAPPER_H_
#define _AL_3AWRAPPER_H_

#ifdef LOCAL_NDK_BUILD   /* test build in local */

#include ".\..\..\INCLUDE\mtype.h"
#include ".\..\..\INCLUDE\frmwk_hw3a_event_type.h"
#include ".\..\..\INCLUDE\hw3a_stats.h"

#else  /* normal release in AP */

#include "mtype.h"
#include "frmwk_hw3a_event_type.h"
#include "hw3a_stats.h"

#endif
#include <sys/time.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define _WRAPPER_VER 0.8020

/*
 * API name: al3awrapper_dispatchhw3astats
 * This API used for copying stats data from HW ISP(Altek) to seperated buffer, but without further patching
 * Framework should call al3AWrapper_DispatchHW3A_XXStats in certain thread for patching, after patching completed, send event
 * to XX Ctrl layer, prepare for process
 * param alisp_metadata[In] :   meta data address from ISP driver, passing via AP framework
 * param alisp_metadata_ae[Out] : AE stats buffer addr, should be arranged via AE ctrl/3A ctrl layer
 * param alisp_metadata_awb[Out] : AWB stats buffer addr, should be arranged via AWB ctrl/3A ctrl layer
 * param alisp_metadata_af[Out] : AF stats buffer addr, should be arranged via AF ctrl/3A ctrl layer
 * param alisp_metadata_yhist[Out] : YHist stats buffer addr, should be arranged via 3A ctrl layer
 * param alisp_metadata_antif[Out] : AntiFlicker stats buffer addr, should be arranged via anti-flicker ctrl/3A ctrl layer
 * param alisp_metadata_subsample[Out] : subsample buffer addr, should be arranged via AF ctrl/3A ctrl layer
 * param udsof_idx[In] : current SOF index, should be from ISP driver layer
 * return: error code
 */
uint32 al3awrapper_dispatchhw3astats( void * alisp_metadata, struct isp_drv_meta_ae_t* alisp_metadata_ae, struct isp_drv_meta_awb_t* alisp_metadata_awb, struct isp_drv_meta_af_t * alisp_metadata_af, struct isp_drv_meta_yhist_t * alisp_metadata_yhist,
                                      struct isp_drv_meta_antif_t * alisp_metadata_antif, struct isp_drv_meta_subsample_t * alisp_metadata_subsample, uint32 udsof_idx );

/*
 * API name: al3AWrapper_GetCurrentDLSequence
 * Comments:
 * This API is used for AP, to set correct DL sequence setting (both for basic/advanced)
 * Parameter:
 * ucAHBSensoreID[In]: used for operating AHB HW channel
 * aDldSequence[Out]: prepared for setup to AHB HW, help to schedule correct 3A HW stats output
 * aIsSingle3AMode[In]: 0: DL all 3A (AE or AWB or AF ) stats at same frame, used for rear camera
 *                      1: DL single 3A (AE/AWB/AF) stats per frame, follow aDldSequence setting, used for front camera
 * opMode[In]: 0: normal LV, (default value)
 *             1: AF, flash AF
 *             2: flash AE
 * return: error code
 */
uint32 al3awrapper_getcurrentdlsequence( uint8 ucahbsensoreid, struct alisp_dldsequence_t* adldsequence, uint8 aissingle3amode, enum alisp_opmode_idx_t opmode );

/*
 * API name: al3AWrapper_SetDLSequence
 * Commets:
 * This API would update to Altek ISP via ISP driver layer
 * AP should call al3AWrapper_GetCurrentDLSequence before set to HW
 * param aDldSequence[In]
 * return: error code
 */
uint32 al3awrapper_setdlsequence( struct alisp_dldsequence_t adldsequence );

/*
 * API name: al3AWrapper_GetVersion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapper_getversion( float *fwrapversion );

#ifdef __cplusplus
}  // extern "C"
#endif

void Separate3ABin(uint32* a_pc3ABinAdd, uint32** a_pcAEAdd, uint32** a_pcAFAdd, uint32** a_pcAWBAdd);
void SeparateShadingIRPBin(uint32* a_pcShadingIRPBinAdd, uint32** a_pcShadingAdd, uint32** a_pcIRPAdd);
#endif // _AL_3AWRAPPER_H_