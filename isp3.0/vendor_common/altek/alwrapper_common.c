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
 * brief Separate ShadongIRPBin
 * param a_pcShadingIRPBinAdd [IN], pointer of ShadingIRP Bin
 * param a_pcShadingAdd      [OUT], location pointer of Shading bin
 * param a_pcIRPAdd          [OUT], location pointer of IRP bin
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