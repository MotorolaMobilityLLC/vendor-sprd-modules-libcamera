/******************************************************************************
 *  File name: alwrapper_yhist.c
 *
 *  Created on: 2016/03/10
 *      Author: Mark Tseng
 *  Latest update:
 *      Reviser:
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. Patch stats from ISP
 ******************************************************************************/

#include <math.h>
#include <string.h>
#include "mtype.h"
#include "hw3a_stats.h"
/* Wrapper define */
#include "alwrapper_3a.h"
#include "alwrapper_yhist.h"
#include "alwrapper_yhist_errcode.h"

/*
 * API name: al3awrapper_dispatchhw3a_yhiststats
 * This API used for patching HW3A stats from ISP(Altek) of Y-histogram, after patching completed, AF ctrl layer should passing patched stats to CAF lib
 * param alisp_metadata_yhist[In]: patched data after calling al3AWrapper_DispatchHW3AStats, used for patch yhist stats
 * param al3awrapper_stats_yhist_t[Out]: result patched yhist stats
 * return: error code
 */
uint32 al3awrapper_dispatchhw3a_yhiststats( struct isp_drv_meta_yhist_t * alisp_metadata_yhist, struct al3awrapper_stats_yhist_t * alwrappered_yhist_dat )
{
	uint32 ret = ERR_WPR_YHIST_SUCCESS;
	struct isp_drv_meta_yhist_t *pmetadata_yhist;
	struct al3awrapper_stats_yhist_t *ppatched_yhistdat;
	uint8 *stats;
	uint32 udValidStatsSize;

	/* check input parameter validity */
	if ( alisp_metadata_yhist == NULL )
		return ERR_WRP_YHIST_EMPTY_METADATA;
	if ( alwrappered_yhist_dat == NULL )
		return ERR_WRP_YHIST_INVALID_INPUT_PARAM;

	pmetadata_yhist = (struct isp_drv_meta_yhist_t *)alisp_metadata_yhist;
	ppatched_yhistdat = (struct al3awrapper_stats_yhist_t *)alwrappered_yhist_dat;
	stats = (uint8 *)pmetadata_yhist->pyhist_stats;
	/* update sturcture size, this would be double checked in yhist libs */
	ppatched_yhistdat->ustructuresize = sizeof( struct al3awrapper_stats_yhist_t );
	/* store patched data/common info/yhist info from wrapper */
	ppatched_yhistdat->umagicnum           = pmetadata_yhist->umagicnum;
	ppatched_yhistdat->uhwengineid         = pmetadata_yhist->uhwengineid;
	ppatched_yhistdat->uframeidx           = pmetadata_yhist->uframeidx;
	ppatched_yhistdat->u_yhist_tokenid     = pmetadata_yhist->uyhisttokenid;
	ppatched_yhistdat->u_yhist_statssize   = pmetadata_yhist->uyhiststatssize;

	/* store frame & timestamp */
	memcpy( &ppatched_yhistdat->systemtime, &pmetadata_yhist->systemtime, sizeof(struct timeval));
	ppatched_yhistdat->udsys_sof_idx       = pmetadata_yhist->udsys_sof_idx;

	if ( pmetadata_yhist->uyhiststatssize >= AL_MAX_HIST_NUM * 4 )
		udValidStatsSize = AL_MAX_HIST_NUM * sizeof(uint32); /* each size   */

	/* switched by stats addr or fix array */
	if ( ppatched_yhistdat->b_is_stats_byaddr ) {
		if ( ppatched_yhistdat->pt_hist_y == NULL )
			return ERR_WRP_YHIST_INVALID_STATS_ADDR;
		memcpy( ppatched_yhistdat->pt_hist_y, stats, udValidStatsSize );
	} else
		memcpy( ppatched_yhistdat->hist_y,    stats, udValidStatsSize );

	return ret;
}

/*
 * API name: al3awrapper_yhist_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
uint32 al3awrapper_yhist_getversion( float *fwrapversion )
{
	uint32 ret = ERR_WPR_YHIST_SUCCESS;
	*fwrapversion = _WRAPPER_YHIST_VER;
	return ret;
}