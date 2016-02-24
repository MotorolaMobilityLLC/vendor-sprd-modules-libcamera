////////////////////////////////////////////////////////////////////
//  File name: al3AWrapper_AF.h
//  Create Date:
//
//  Comment:
//
//
////////////////////////////////////////////////////////////////////

#define _WRAPPER_AF_VER 0.8000

/* for AF ctrl layer */
/**
\API name: al3AWrapper_DispatchHW3A_AFStats
\This API used for patching HW3A stats from ISP(Altek) for AF libs(Altek), after patching completed, AF ctrl should prepare patched
\stats to AF libs
\param alISP_MetaData_AF[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch AF stats for AF lib
\param alWrappered_AF_Dat[Out]: result patched AF stats
\return: error code
*/
UINT32 al3AWrapper_DispatchHW3A_AFStats(void *alISP_MetaData_AF,void *alWrappered_AF_Dat);

/**
\API name: al3AWrapperAF_TranslateFocusModeToAPType
\This API used for translating AF lib focus mode define to framework
\param aFocusMode[In] :   Focus mode define of AF lib (Altek)
\return: Focus mode define for AP framework
*/
UINT32 al3AWrapperAF_TranslateFocusModeToAPType(UINT32 aFocusMode);

/**
\API name: al3AWrapperAF_TranslateFocusModeToAFLibType
\This API used for translating framework focus mode to AF lib define
\aFocusMode[In] :   Focus mode define of AP framework define
\return: Focus mode define for AF lib (Altek)
*/
UINT32 al3AWrapperAF_TranslateFocusModeToAFLibType(UINT32 aFocusMode);

/**
\API name: al3AWrapperAF_TranslateCalibDatToAFLibType
\This API used for translating EEPROM data to AF lib define
\EEPROM_Addr[In] :   EEPROM data address
\AF_Calib_Dat[Out] :   Altek data format
\return: Error code
*/

UINT32 al3AWrapperAF_TranslateCalibDatToAFLibType(void *EEPROM_Addr,alAFLib_input_init_info_t *AF_Init_Dat);

/**
\API name: al3AWrapperAF_TranslateROIToAFLibType
\This API used for translating ROI info to AF lib define
\frame_id[In] :   Current frame id
\AF_ROI_Info[Out] :   Altek data format
\return: Error code
*/

UINT32 al3AWrapperAF_TranslateROIToAFLibType(unsigned int frame_id,alAFLib_input_roi_info_t *AF_ROI_Info);

/**
\API name: al3AWrapperAF_UpdateISPConfig_AF
\This API is used for query ISP config before calling al3AWrapperAF_UpdateISPConfig_AF
\param aAFLibStatsConfig[in]: AF stats config info from AF lib
\param aAFConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/
UINT32 al3AWrapperAF_UpdateISPConfig_AF(alAFLib_af_out_stats_config_t *aAFLibStatsConfig,alHW3a_AF_CfgInfo_t *aAFConfig);

/**
\API name: al3AWrapperAF_GetDefaultCfg
\This API is used for query default ISP config before calling al3AWrapperAF_UpdateISPConfig_AF
\param aAFConfig[out]: input buffer, API would manage parameter and return via this pointer
\return: error code
*/

UINT32 al3AWrapperAF_GetDefaultCfg(alHW3a_AF_CfgInfo_t *aAFConfig);

/**
\API name: al3AWrapper_UpdateAFReport
\This API is used for translate AF status  to generate AF report for AP
\param aAFLibStatus[in]: af status from AF update
\param aAFReport[out]: AF report for AP
\return: error code
*/

UINT32 al3AWrapper_UpdateAFReport(alAFLib_af_out_status_t *aAFLibStatus,af_report_update_t *aAFReport);

/**
\API name: al3AWrapperAF_GetVersion
\This API would return labeled version of wrapper
\fWrapVersion[out], return current wapper version
\return: error code
*/
UINT32 al3AWrapperAF_GetVersion( float *fWrapVersion );
