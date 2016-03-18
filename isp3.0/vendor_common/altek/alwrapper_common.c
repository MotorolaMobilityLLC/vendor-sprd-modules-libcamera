/******************************************************************************
 * alwrapper_common.c
 *
 *  Created on: 2016/03/05
 *      Author: MarkTseng
 *  Latest update:
 *      Reviser:
 *  Comments:
 *       This c file is mainly used for AP framework to:
 *       1. dispatch common HW, such as IRP(ISP)
 *       2. dispatch bin file from tuning tool
 ******************************************************************************/

#include <string.h>
#include "mtype.h"

#include "alwrapper_common.h"
#include "alwrapper_common_errcode.h"
/******************************************************************************
 * function prototype
 ******************************************************************************/


/******************************************************************************
 * Function code
 ******************************************************************************/
/*
 * brief Separate 3ABin
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param a_pcAEAdd     [OUT], location pointer of AE bin
 * param a_pcAFAdd     [OUT], location pointer of AF bin
 * param a_pcAWBAdd    [OUT], location pointer of AWB bin
 * return None
 */
void al3awrapper_com_separate_3a_bin(uint8* a_pc3abinadd, uint8** a_pcaeadd, uint8** a_pcafadd, uint8** a_pcawbadd)
{
	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pc3abinadd;

	*a_pcaeadd = a_pc3abinadd + t_header_info->uwlocation1;
	*a_pcafadd = a_pc3abinadd + t_header_info->uwlocation2;
	*a_pcawbadd = a_pc3abinadd + t_header_info->uwlocation3;
}

/*
 * brief Separate 3ABin type2 API, add return size info
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_3a_bin_type2(uint8* a_pc3abinadd, uint32 uw_bin_size , struct bin1_sep_info* a_bin1_info )
{
	uint32 ret = ERR_WRP_COMM_SUCCESS;
	/* parameter validity check */
	if ( a_pc3abinadd == NULL || a_bin1_info == NULL )
		return ERR_WRP_COMM_INVALID_ADDR;

	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pc3abinadd;

	/* size over boundry check  */
	if ( t_header_info->uwlocation1 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( t_header_info->uwlocation2 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;
	else if ( t_header_info->uwlocation3 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_3;

	/* seperated size validity check */
	if ( t_header_info->uwlocation2 < t_header_info->uwlocation1 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( t_header_info->uwlocation3 < t_header_info->uwlocation2 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;
	else if ( uw_bin_size < t_header_info->uwlocation3 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_3;

	a_bin1_info->puc_ae_bin_addr  = a_pc3abinadd + t_header_info->uwlocation1;
	a_bin1_info->puc_awb_bin_addr = a_pc3abinadd + t_header_info->uwlocation2;
	a_bin1_info->puc_af_bin_addr  = a_pc3abinadd + t_header_info->uwlocation3;

	a_bin1_info->uw_ae_bin_size  = t_header_info->uwlocation2 - t_header_info->uwlocation1;
	a_bin1_info->uw_awb_bin_size = t_header_info->uwlocation3 - t_header_info->uwlocation2;
	a_bin1_info->uw_af_bin_size  = uw_bin_size - t_header_info->uwlocation3;
	return ret;
}


/*
 * brief Separate ShadongIRPBin
 * param a_pcshadingirpbinadd [IN], pointer of ShadingIRP Bin
 * param a_pcshadingadd      [OUT], location pointer of Shading bin
 * param a_pcirpadd          [OUT], location pointer of IRP bin
 * return None
 */
void al3awrapper_com_separate_shadingirp_bin(uint8* a_pcshadingirpbinadd, uint8** a_pcshadingadd, uint8** a_pcirpadd)
{
	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pcshadingirpbinadd;

	*a_pcshadingadd = a_pcshadingirpbinadd + t_header_info->uwlocation1;
	*a_pcirpadd = a_pcshadingirpbinadd + t_header_info->uwlocation2;
}

/*
 * brief Separate ShadongIRPBin type2 API, add return size info
 * param a_pcshadingirpbinadd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_shadingirp_bin_type2(uint8* a_pcshadingirpbinadd, uint32 uw_bin_size , struct bin2_sep_info* a_bin2_info )
{
	int32 ret = ERR_WRP_COMM_SUCCESS;
	/* parameter validity check */
	if ( a_pcshadingirpbinadd == NULL || a_bin2_info == NULL )
		return ERR_WRP_COMM_INVALID_ADDR;

	struct header_info *t_header_info;
	t_header_info = (struct header_info *)a_pcshadingirpbinadd;

	/* size over boundry check  */
	if ( t_header_info->uwlocation1 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( t_header_info->uwlocation2 > uw_bin_size)
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;

	/* seperated size validity check */
	if ( t_header_info->uwlocation2 < t_header_info->uwlocation1 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_1;
	else if ( uw_bin_size < t_header_info->uwlocation2 )
		return ERR_WRP_COMM_INVALID_SIZEINFO_2;

	a_bin2_info->puc_shading_bin_addr  = a_pcshadingirpbinadd + t_header_info->uwlocation1;
	a_bin2_info->puc_irp_bin_addr      = a_pcshadingirpbinadd + t_header_info->uwlocation2;

	a_bin2_info->uw_shading_bin_size  = t_header_info->uwlocation2 - t_header_info->uwlocation1;
	a_bin2_info->uw_irp_bin_size      = uw_bin_size - t_header_info->uwlocation2;

	return ret;
}

/*
 * API name: al3awrapper_com_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
UINT32 al3awrapper_com_getversion( float *fwrapversion )
{
	UINT32 ret = ERR_WRP_COMM_SUCCESS;
	*fwrapversion = _WRAPPER_COMM_VER;
	return ret;
}