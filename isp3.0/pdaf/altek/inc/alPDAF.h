#ifndef _alPDAF_H_
#define _alPDAF_H_

#include "alPD.h"
#include "alPDAF_err.h"


typedef struct
{
	int dSensorID;//0:for SamSung 1: for Sony
	double uwInfVCM;
	double uwMacroVCM;
}SensorInfo;

typedef struct {
	/*sensor information*/
	SensorInfo tSensorInfo;
	int dcurrentVCM;
	// BV value
	float dBv;
	// black offset
	int dBlackOffset;
	unsigned char ucPrecision;
} PDInReg;

typedef struct{
	short int   m_wLeft;
	short int   m_wTop;
	short int   m_wWidth;
	short int   m_wHeight;
} alPD_RECT;

typedef enum{
	SRCIMG_8 = 0,
	SRCIMG_10 = 1,
	SRCIMG_12 = 2,
}DataBit;

#ifdef __cplusplus
extern "C"{
#endif
alPDAF_ERR_CODE alPDAF_Initial(void *a_pInPDPackData);
alPDAF_ERR_CODE alPDAF_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);
alPDAF_ERR_CODE alPDAF_Calculate(float *a_pfOutPDValue, void *a_tOutPdReg, void *a_pInImageBuf_left, void *a_pInImageBuf_right,
								unsigned short a_uwInWidth, unsigned short a_uwInHeight,alGE_RECT a_tInWOI,
								DataBit a_tInBits, PDInReg *a_tInPdReg);
alPDAF_ERR_CODE alPDAF_Close();
alPDAF_ERR_CODE alPDAF_Reset();
#ifdef __cplusplus
}
#endif

#endif



