/******************************************************************************
*  File name: alwrapper_common.h
*  Create Date:
*
*  Comment:
*  Common wrapper, handle outside 3A behavior, such ae seperate bin file
 ******************************************************************************/

#include "mtype.h"  /* same as altek 3A lib released */

#define _WRAPPER_COMM_VER 0.8040

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
 * API name: al3awrapper_com_getversion
 * This API would return labeled version of wrapper
 * fWrapVersion[out], return current wapper version
 * return: error code
 */
UINT32 al3awrapper_com_getversion( float *fwrapversion );