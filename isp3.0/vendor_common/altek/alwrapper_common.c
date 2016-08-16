/******************************************************************************
 * alwrapper_common.c
 *
 *  Created on: 2016/03/18
 *      Author: FishHuang
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


/*
 * brief Separate function bin API, add return size info
 * param a_cfunction [IN], function number
 * param a_pcFunctionBin [IN], pointer of function addr
 * param uw_IDX_number  [IN], input idx number
 * param a_pcBinAddr [OUT], seperated function addr
 * param a_pwBinSize [OUT], seperated function size
 * return None
 */
uint32 al3awrapper_com_separate_function_bin(uint8 a_cfunction, uint8* a_pcFunctionBin, uint32 uw_IDX_number, uint8 **a_pcBinAddr, uint32 *a_pwBinSize)
{
	uint32 *ptr_bin_addr;
	uint32 *ptr_bin_list;
	uint32 *ptr_bin_size;
	struct functionbin_header_info *functionHeader;

	/* parameter validity check */
	if( NULL == a_pcFunctionBin || NULL == a_pcBinAddr || NULL == a_pwBinSize )
		return ERR_WRP_COMM_INVALID_ADDR;

	if( a_cfunction > WRAPPER_COMM_BIN_NUMBER )
		return ERR_WRP_COMM_INVALID_IDX;

	functionHeader = (struct functionbin_header_info*) a_pcFunctionBin;

	/* header size check */
	if( sizeof(struct functionbin_header_info) != functionHeader->HeaderSize )
		return ERR_WRP_COMM_INVALID_FUNCTION(a_cfunction);

	/* IDX number check */
	if( uw_IDX_number > functionHeader->Index_Table_Number ||
	    functionHeader->Index_Table_Offset > functionHeader->TotalSize ||
	    functionHeader->Bin_Size_Table_Offset > functionHeader->TotalSize )
		return ERR_WRP_COMM_INVALID_FUNCTION(a_cfunction);

	ptr_bin_addr = (uint32*)(a_pcFunctionBin + functionHeader->Index_Table_Offset);
	ptr_bin_list = (uint32*)(a_pcFunctionBin + functionHeader->Bin_List_Offset);
	ptr_bin_size = (uint32*)(a_pcFunctionBin + functionHeader->Bin_Size_Table_Offset);

	*a_pcBinAddr = (a_pcFunctionBin + *(ptr_bin_addr + uw_IDX_number));
	*a_pwBinSize = *(ptr_bin_size + *(ptr_bin_list + uw_IDX_number));

	return ERR_WRP_COMM_SUCCESS;
}

/*
 * brief Separate one bin API to structure bin1_sep_info & bin2_sep_info
 * param a_pcbinadd [IN], pointer of bin addr
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param uw_IDX_number [IN], current IDX
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32   al3awrapper_com_separate_one_bin(uint8* a_pcbinadd, uint32 uw_bin_size, uint32 uw_IDX_number, struct bin1_sep_info* a_bin1_info, struct bin2_sep_info* a_bin2_info, struct bin3_sep_info* a_bin3_info)
{
	struct onebin_header_info *t_header_info;
	int32   ret = ERR_WRP_COMM_SUCCESS;
	uint8   i;
	uint8   *addr;
	uint32  offset;
	uint32  bin_size[WRAPPER_COMM_BIN_NUMBER];
	uint8   *function_addr[WRAPPER_COMM_BIN_NUMBER];
	uint8   *ptr_IDX_Enable;
	uint32  *ptr_function_offset;
	char    *ptr_IDX_name;
	char    IDX_Name[WRAPPER_COMM_BIN_PROJECT_STRLENGTH+1] = {0};

	/* parameter validity check */
	if (NULL == a_pcbinadd || NULL == a_bin1_info || NULL == a_bin2_info)
		return ERR_WRP_COMM_INVALID_ADDR;

	t_header_info = (struct onebin_header_info *) a_pcbinadd;

	/* IDX number check */
	if (uw_IDX_number > t_header_info->IDX_Application_Number)
		return ERR_WRP_COMM_INVALID_IDX;

	/* Bin format check */
	if( sizeof(struct onebin_header_info)   != t_header_info->HeaderSize ||
	    WRAPPER_COMM_BIN_PROJECT_STRLENGTH  != t_header_info->Prj_StrLength ||
	    WRAPPER_COMM_IDX_NUMBER             != t_header_info->IDX_Application_Number ||
	    WRAPPER_COMM_BIN_NUMBER             != t_header_info->FunctionBin_Table_Number)
		return ERR_WRP_COMM_INVALID_FORMAT;

	/* IDX Enable check */
	ptr_IDX_Enable = a_pcbinadd + t_header_info->IDX_ApplicationEnable_Offset;
	if( 0 == *(ptr_IDX_Enable + uw_IDX_number) ) {
		uw_IDX_number = 0;
	}

	/* IDX Name */
	ptr_IDX_name = (char*)(a_pcbinadd + t_header_info->IDX_ApplicationName_Offset);
	memcpy(IDX_Name, (ptr_IDX_name + uw_IDX_number * t_header_info->Prj_StrLength), t_header_info->Prj_StrLength);
	IDX_Name[WRAPPER_COMM_BIN_PROJECT_STRLENGTH] = '\n';

	ptr_function_offset = (uint32*)(a_pcbinadd + t_header_info->FunctionBin_Table_Offset);
	for(i=0; i<WRAPPER_COMM_BIN_NUMBER; i++) {
		offset = *ptr_function_offset;

		/* size over boundry check  */
		if( uw_bin_size < offset )
			return ERR_WRP_COMM_INVALID_SIZEINFO(i);

		addr = (a_pcbinadd + offset);

		if( i == WRAPPER_COMM_BIN_NUMBER - 1 ) {
			// IRP is SW format, not funciton Bin
			function_addr[i] = addr;
			bin_size[i] = uw_bin_size - offset;
		} else {
			ret = al3awrapper_com_separate_function_bin(i, addr, uw_IDX_number, &function_addr[i], &bin_size[i]);
			if( ERR_WRP_COMM_SUCCESS != ret )
				return ret;
		}

		ptr_function_offset++;
	}

	a_bin1_info->puc_af_bin_addr      = function_addr[0];
	a_bin1_info->uw_af_bin_size       = bin_size[0];

	a_bin1_info->puc_ae_bin_addr      = function_addr[1];
	a_bin1_info->uw_ae_bin_size       = bin_size[1];

	a_bin2_info->puc_shading_bin_addr = function_addr[2];
	a_bin2_info->uw_shading_bin_size  = bin_size[2];

	a_bin1_info->puc_awb_bin_addr     = function_addr[3];
	a_bin1_info->uw_awb_bin_size      = bin_size[3];

	a_bin3_info->puc_caf_bin_addr     = function_addr[4];
	a_bin3_info->uw_caf_bin_size      = bin_size[4];

	a_bin2_info->puc_irp_bin_addr     = function_addr[5];
	a_bin2_info->uw_irp_bin_size      = bin_size[5];

	return 0;
}