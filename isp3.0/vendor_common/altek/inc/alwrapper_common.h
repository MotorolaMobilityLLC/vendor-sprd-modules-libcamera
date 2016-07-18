/******************************************************************************
*  File name: alwrapper_common.h
*  Create Date:
*
*  Comment:
*  Common wrapper, handle outside 3A behavior, such ae seperate bin file
 ******************************************************************************/

#include "mtype.h"  /* same as altek 3A lib released */

#define _WRAPPER_COMM_VER 0.8060

#define WRAPPER_COMM_IDX_NUMBER                 (15)
#define WRAPPER_COMM_BIN_NUMBER                 (6)
#define WRAPPER_COMM_BIN_PROJECT_STRLENGTH      (32)

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct header_info {
	int8 cbintag[20];
	uint32 uwtotalsize;
	uint32 uwversion1;
	uint32 uwversion2;
	uint32 uwlocation1;
	uint32 uwlocation2;
	uint32 uwlocation3;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct bin1_sep_info {
	uint32 uw_ae_bin_size;
	uint8* puc_ae_bin_addr;
	uint32 uw_awb_bin_size;
	uint8* puc_awb_bin_addr;
	uint32 uw_af_bin_size;
	uint8* puc_af_bin_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct bin2_sep_info {
	uint32 uw_shading_bin_size;
	uint8* puc_shading_bin_addr;
	uint32 uw_irp_bin_size;
	uint8* puc_irp_bin_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
struct bin3_sep_info {
	uint32 uw_caf_bin_size;
	uint8* puc_caf_bin_addr;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(1)    /* new alignment setting */
struct functionbin_header_info {
	uint32 Verifier;
	uint32 HeaderSize;
	uint32 TotalSize;
	uint32 Index_Table_Number;
	uint32 Index_Table_Offset;
	uint32 Bin_Number;
	uint32 Bin_List_Offset;
	uint32 Bin_Size_Table_Offset;
	uint32 Bin_FileName_Table_Offset;
	uint32 Bin_FileName_Array_Length;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(1)    /* new alignment setting */
struct onebin_header_info {
	uint16 HeaderSize;
	uint16 IDX_Application_Number;
	uint16 IDX_ApplicationEnable_Offset;
	uint16 IDX_ApplicationName_Offset;
	uint16 FunctionBin_Table_Number;
	uint16 FunctionBin_Table_Offset;

	uint16 Prj_StrLength;
	int8 Prj_ProjectName[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_SensorName[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_MegaPixel[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_Assembly[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_Date[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	int8 Prj_Tuner[WRAPPER_COMM_BIN_PROJECT_STRLENGTH];
	uint16 Prj_CameraDirection;

	uint16 Raw_Width;
	uint16 Raw_Height;
	uint16 Raw_BayerPatten;
	uint16 Raw_HeaderSize;
	uint16 Raw_PackType;
	uint16 Raw_AppliedSHD;
	uint16 Raw_BlackOffset_R;
	uint16 Raw_BlackOffset_Gr;
	uint16 Raw_BlackOffset_Gb;
	uint16 Raw_BlackOffset_B;

	uint32 Version;
};
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * brief Separate 3ABin
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param a_pcAEAdd     [OUT], location pointer of AE bin
 * param a_pcAFAdd     [OUT], location pointer of AF bin
 * param a_pcAWBAdd    [OUT], location pointer of AWB bin
 * return None
 */
void al3awrapper_com_separate_3a_bin(uint8* a_pc3abinadd, uint8** a_pcaeadd, uint8** a_pcafadd, uint8** a_pcawbadd);

/*
 * brief Separate ShadongIRPBin
 * param a_pcShadingIRPBinAdd [IN], pointer of ShadingIRP Bin
 * param a_pcShadingAdd      [OUT], location pointer of Shading bin
 * param a_pcIRPAdd          [OUT], location pointer of IRP bin
 * return None
 */
void al3awrapper_com_separate_shadingirp_bin(uint8* a_pcshadingirpbinadd, uint8** a_pcshadingadd, uint8** a_pcirpadd);


/*
 * brief Separate 3ABin type2 API, add return size info
 * param a_pc3ABinAdd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_3a_bin_type2(uint8* a_pc3abinadd, uint32 uw_bin_size , struct bin1_sep_info* a_bin1_info );

/*
 * brief Separate ShadongIRPBin type2 API, add return size info
 * param a_pcshadingirpbinadd [IN], pointer of 3A Bin
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32 al3awrapper_com_separate_shadingirp_bin_type2(uint8* a_pcshadingirpbinadd, uint32 uw_bin_size , struct bin2_sep_info* a_bin2_info );

/*
 * API name: al3awrapper_com_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
UINT32 al3awrapper_com_getversion( float *fwrapversion );

/*
 * brief Separate one bin API to structure bin1_sep_info & bin2_sep_info
 * param a_pcbinadd [IN], pointer of bin addr
 * param uw_bin_size  [IN], total bin size of 3A Bin file
 * param uw_IDX_number [IN], current IDX
 * param a_bin1_info [OUT], seperated bin information (including addr, size)
 * param a_bin2_info [OUT], seperated bin information (including addr, size)
 * return None
 */
uint32   al3awrapper_com_separate_one_bin(uint8* a_pcbinadd, uint32 uw_bin_size, uint32 uw_IDX_number, struct bin1_sep_info* a_bin1_info, struct bin2_sep_info* a_bin2_info, struct bin3_sep_info* a_bin3_info );