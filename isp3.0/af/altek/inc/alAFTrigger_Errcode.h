#ifndef _ALAFTRIGGER_ERRCODE_HEADER_
#define _ALAFTRIGGER_ERRCODE_HEADER_

/*
PROPRIETARY STATEMENT:
THIS FILE (OR DOCUMENT) CONTAINS INFORMATION CONFIDENTIAL AND
PROPRIETARY TO ALTEK CORPORATION AND SHALL NOT BE REPRODUCED
OR TRANSFERRED TO OTHER FORMATS OR DISCLOSED TO OTHERS.
ALTEK DOES NOT EXPRESSLY OR IMPLIEDLY GRANT THE RECEIVER ANY
RIGHT INCLUDING INTELLECTUAL PROPERTY RIGHT TO USE FOR ANY
SPECIFIC PURPOSE, AND THIS FILE (OR DOCUMENT) DOES NOT
CONSTITUTE A FEEDBACK TO ANY KIND.
*/

#define alAFTrigger_ERR_MODULE                   0x78000
#define alAFTrigger_ERR_CODE                     int
#define alAFTrigger_ERR_SUCCESS                  0x00
#define alAFTrigger_ERR_BUFFER_SIZE_TOO_SMALL    (alAFTrigger_ERR_MODULE+0x00000001)
#define alAFTrigger_ERR_BUFFER_IS_NULL           (alAFTrigger_ERR_MODULE+0x00000002)

//AFTrigger error code

//#define alAFTrigger_ERR_MEMORY_ALLOCATE_FAIL_S1   ( (alAFTrigger_ERR_MODULE+0x0000003))
//#define alAFTrigger_ERR_MEMORY_ALLOCATE_FAIL_S2   ( (alAFTrigger_ERR_MODULE+0x0000004))
//#define alAFTrigger_ERR_PDPACKDATA_SETTING_ERROR  ( (alAFTrigger_ERR_MODULE+0x0000005))
//#define alAFTrigger_ERR_SCENEDETECTION_ERROR      ( (alAFTrigger_ERR_MODULE+0x0000006))
//#define alAFTrigger_ERR_WITHOUT_ALPDAF_INITIAL	 ( (alAFTrigger_ERR_MODULE+0x0000007))
//#define alAFTrigger_ERR_AlREADY_ALPDAF_INITIAL	 ( (alAFTrigger_ERR_MODULE+0x0000008))

#endif	//_ALCM_BINNING_ERRCODE_HEADER_
