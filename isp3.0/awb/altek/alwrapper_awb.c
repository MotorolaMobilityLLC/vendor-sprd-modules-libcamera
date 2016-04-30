/******************************************************************************
 * al3AWrapper_AWB.c
 *
 *  Created on: 2015/12/07
 *      Author: HanTseng
 *  Latest update: 2016/4/21
 *      Reviser: Allenwang
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Query HW3A config setting
 *       2. Set HW3A via AP IPS driver of altek
 *       3. translate AP framework index from al3A to framework define, such as scene mode, etc.
 *       4. packed output report from AWB lib to framework
 *       5. translate input parameter from AP framework to AP
 ******************************************************************************/

#include <math.h>   /* math lib, depends on AP OS */
#include <string.h>
#include "alwrapper_3a.h"
#include "alwrapper_awb.h"  /* include wrapper AWB define */
#include "alwrapper_awb_errcode.h"

/*
 * API name: al3awrapper_dispatchhw3a_awbstats
 * This API used for patching HW3A stats from ISP(Altek) for AWB libs(Altek),
 * after patching completed, AWB ctrl should prepare patched
 * stats to AWB libs
 * param alISP_MetaData_AWB[In]: patched data after calling al3AWrapper_DispatchHW3AStats,
 *                               used for patch AWB stats for AWB lib
 * param alWrappered_AWB_Dat[Out]: result AWB stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_awbstats(void *alisp_metadata_awb, void *alwrappered_awb_dat)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;
	uint32 utotalblocks;
	struct isp_drv_meta_awb_t      *pmetadata_awb;
	struct al3awrapper_stats_awb_t *pwrapper_stat_awb;

	/* check input parameter validity */
	if (alisp_metadata_awb == NULL)
		return ERR_WRP_AWB_EMPTY_METADATA;

	if (alwrappered_awb_dat == NULL)
		return ERR_WRP_AWB_INVALID_INPUT_PARAM;

	pmetadata_awb = (struct isp_drv_meta_awb_t *) alisp_metadata_awb;
	pwrapper_stat_awb = (struct al3awrapper_stats_awb_t *) alwrappered_awb_dat;

	utotalblocks = pmetadata_awb->awb_stats_info.ucvalidblocks * pmetadata_awb->awb_stats_info.ucvalidbanks;

	/* check data AWB blocks validity */
	if (utotalblocks > AL_MAX_AWB_STATS_NUM)
		return ERR_WRP_AWB_INVALID_BLOCKS;

	/*  Common info */
	pwrapper_stat_awb->umagicnum   = pmetadata_awb->umagicnum;
	pwrapper_stat_awb->uhwengineid = pmetadata_awb->uhwengineid;
	pwrapper_stat_awb->uframeidx   = pmetadata_awb->uframeidx;

	/*  AWB info */
	if ( pmetadata_awb->b_isstats_byaddr == 1 ) {
		if ( pmetadata_awb->puc_awb_stats == NULL )
			return ERR_WRP_AWB_INVALID_STATS_ADDR;
		pwrapper_stat_awb->pawb_stats    = pmetadata_awb->puc_awb_stats;
	} else
		pwrapper_stat_awb->pawb_stats    = pmetadata_awb->pawb_stats;

	pwrapper_stat_awb->uawbtokenid   = pmetadata_awb->uawbtokenid;
	pwrapper_stat_awb->uawbstatssize = pmetadata_awb->uawbstatssize;
	/* uPseudoFlag 0: normal stats, 1: PseudoFlag flag (for lib, smoothing/progressive run) */
	pwrapper_stat_awb->upseudoflag   = pmetadata_awb->upseudoflag;
	/* AWB stats info */
	pwrapper_stat_awb->udpixelsperblocks = pmetadata_awb->awb_stats_info.udpixelsperblocks;
	pwrapper_stat_awb->udbanksize        = pmetadata_awb->awb_stats_info.udbanksize;
	pwrapper_stat_awb->ucvalidblocks     = pmetadata_awb->awb_stats_info.ucvalidblocks;
	pwrapper_stat_awb->ucvalidbanks      = pmetadata_awb->awb_stats_info.ucvalidbanks;
	pwrapper_stat_awb->ucstatsdepth      = 8;
	pwrapper_stat_awb->ucstats_format    = 0;

	/* store frame & timestamp */
	memcpy(&pwrapper_stat_awb->systemtime, &pmetadata_awb->systemtime, sizeof(struct timeval));
	pwrapper_stat_awb->udsys_sof_idx       = pmetadata_awb->udsys_sof_idx;

	return ret;
}


/*
 * API name: al3awrapperawb_translatescenemodefromawblib2ap
 * This API used for translating AWB lib scene define to framework
 * param aSceneMode[In] :   scene mode define of AWB lib (Altek)
 * return: scene mode define for AP framework
 */
uint32 al3awrapperawb_translatescenemodefromawblib2ap(uint32 ascenemode)
{
	uint32 retapscenemode;

	switch (ascenemode) {
	case 0:
		retapscenemode = 0;
		break;
	default:
		retapscenemode = 0;
		break;
	}

	return retapscenemode;
}

/*
 * API name: al3awrapperawb_translatescenemodefromap2awblib
 * This API used for translating framework scene mode to AWB lib define
 * aSceneMode[In] :   scene mode define of AP framework define
 * return: scene mode define for AWB lib (Altek)
 */
uint32 al3awrapperawb_translatescenemodefromap2awblib(uint32 ascenemode)
{
	uint32 retawbscenemode;

	switch (ascenemode) {
	case 0:
		retawbscenemode = 0;
		break;
	default:
		retawbscenemode = 0;
		break;
	}

	return retawbscenemode;
}

/*
 * API name: al3awrapperawb_getdefaultcfg
 * This API is used for query default ISP config before calling al3awrapperawb_queryispconfig_awb,
 * which default config use without correct OTP
 * param aAWBConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperawb_getdefaultcfg(struct alhw3a_awb_cfginfo_t *aawbconfig)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;
	struct alhw3a_awb_cfginfo_t localparam;
	uint8   i;
	uint8   yfactor[16] = { 0, 0, 0, 4, 7, 10, 12, 14,
	                        15, 15, 15, 15, 14, 13, 10, 5
	                      };
	sint8   bbrfactor[33] = {22, 20, 18, 16, 15, 13, 11, 10, 8, 8,
	                         6, 5, 3, 1, -1, -3, -4, -5, -6, -7, -8,
	                         -9, -10, -11, -12, -13, -14, -15, -16,
	                         -18, -18, -18, -18
	                        };

	if (aawbconfig == NULL)
		return ERR_WRP_AWB_INVALID_INPUT_PARAM;

	localparam.tokenid                      = 0x01;
	localparam.tawbregion.uwborderratiox    = 100;
	localparam.tawbregion.uwborderratioy    = 100;
	localparam.tawbregion.uwblknumx         = 64;
	localparam.tawbregion.uwblknumy         = 48;
	localparam.tawbregion.uwoffsetratiox    = 0;
	localparam.tawbregion.uwoffsetratioy    = 0;
	memcpy(localparam.ucyfactor, yfactor, 16 * sizeof(uint8));
	memcpy(localparam.bbrfactor, bbrfactor, 33 * sizeof(sint8));
	localparam.uwrgain                      = 0;
	localparam.uwggain                      = 0;
	localparam.uwbgain                      = 0;
	localparam.uccrshift                    = 100;
	localparam.ucoffsetshift                = 100;
	localparam.ucquantize                   = 0;
	localparam.ucdamp                       = 7;
	localparam.ucsumshift                   = 5;
	localparam.tawbhis.benable              = TRUE;
	localparam.tawbhis.ccrstart             = -46;
	localparam.tawbhis.ccrend               = 110;
	localparam.tawbhis.coffsetup            = 10;
	localparam.tawbhis.coffsetdown          = -90;
	localparam.tawbhis.ccrpurple            = 0;
	localparam.tawbhis.ucoffsetpurple       = 2;
	localparam.tawbhis.cgrassoffset         = -22;
	localparam.tawbhis.cgrassstart          = -30;
	localparam.tawbhis.cgrassend            = 25;
	localparam.tawbhis.ucdampgrass          = 4;
	localparam.tawbhis.coffset_bbr_w_start  = -2;
	localparam.tawbhis.coffset_bbr_w_end    = 2;
	localparam.tawbhis.ucyfac_w             = 2;
	localparam.tawbhis.dhisinterp           = -178;
	localparam.uwrlineargain                = 128;
	localparam.uwblineargain                = 128;
	memcpy(aawbconfig, &localparam, sizeof(struct alhw3a_awb_cfginfo_t));

	return ret;
}

/*
 * API name: al3awrapperawb_queryispconfig_awb
 * This API is used for query ISP config before calling al3awrapperawb_updateispconfig_awb
 * param aAWBConfig[out]: API would manage parameter and return via this pointer
 * param aAWBLibCallback[in]: callback lookup table,
 * must passing correct table into this API for querying HW3A config
 * return: error code
 */
uint32 al3awrapperawb_queryispconfig_awb(struct alhw3a_awb_cfginfo_t *aawbconfig, struct allib_awb_runtime_obj_t *aawblibcallback)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;
	struct allib_awb_get_parameter_t localparam;

	if (aawbconfig == NULL || aawblibcallback == NULL || aawblibcallback->awb == NULL)
		return ERR_WRP_AWB_INVALID_INPUT_PARAM;

	localparam.type = alawb_get_param_init_isp_config;
	ret = aawblibcallback->get_param(&localparam, aawblibcallback->awb);

	if (ret != ERR_WPR_AWB_SUCCESS)
		return ret;

	memcpy(aawbconfig, localparam.para.awb_hw_config, sizeof(struct alhw3a_awb_cfginfo_t));

	return ret;
}

/*
 * API name: al3awrapperawb_updateispconfig_awb
 * This API is used for query ISP config before calling al3awrapperawb_updateispconfig_awb
 * param a_ucSensor[in]: AHB sensor ID
 * param aAWBConfig[out]: input buffer, API would manage parameter and return via this pointer
 * return: error code
 */
uint32 al3awrapperawb_updateispconfig_awb(uint8 a_ucSensor, struct alhw3a_awb_cfginfo_t *aawbconfig)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;
#ifdef LINK_ALTEK_ISP_DRV_DEFINE   /* only build when link to ISP driver define */
	ret = ISPDRV_AP3AMGR_SetAWBCfg(a_ucSensor, aawbconfig);
#else
	UNUSED(a_ucSensor);
	UNUSED(aawbconfig);
#endif
	return ret;
}


/*
 * API name: al3awrapperawb_setotpcalibration
 * This API is used for setting stylish file to alAWBLib
 * param calib_data[in]: calibration data, scale 1000
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting calibration data
 * return: error code
 */
uint32 al3awrapperawb_setotpcalibration(struct calibration_data_t *calib_data , struct allib_awb_runtime_obj_t *aawblibcallback)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;
	struct allib_awb_set_parameter_t localparam;

	if (calib_data == NULL || aawblibcallback->awb == NULL)
		return ERR_WRP_AWB_INVALID_INPUT_PARAM;

	localparam.type = alawb_set_param_camera_calib_data;
	localparam.para.awb_calib_data.calib_r_gain = calib_data->calib_r_gain;
	localparam.para.awb_calib_data.calib_g_gain = calib_data->calib_g_gain;
	localparam.para.awb_calib_data.calib_b_gain = calib_data->calib_b_gain;
	ret = aawblibcallback->set_param(&localparam, aawblibcallback->awb);

	return ret;
}


/*
 * API name: al3awrapperawb_settuningfile
 * This API is used for setting Tuning file to alAWBLib
 * param setting_file[in]: file address pointer to setting file [TBD, need confirm with 3A data file format
 * param aAWBLibCallback[in]: callback lookup table, must passing correct table into this API for setting file
 * return: error code
 */
uint32 al3awrapperawb_settuningfile(void *setting_file, struct allib_awb_runtime_obj_t *aawblibcallback)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;
	struct allib_awb_set_parameter_t localparam;

	if (setting_file == NULL || aawblibcallback->awb == NULL)
		return ERR_WRP_AWB_INVALID_INPUT_PARAM;

	localparam.type = alawb_set_param_tuning_file;
	localparam.para.tuning_file = setting_file;
	ret = aawblibcallback->set_param(&localparam, aawblibcallback->awb);

	return ret;
}

/*
 * API name: al3awrapperawb_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapperawb_getversion(float *fwrapversion)
{
	uint32 ret = ERR_WPR_AWB_SUCCESS;

	*fwrapversion = _WRAPPER_AWB_VER;

	return ret;
}