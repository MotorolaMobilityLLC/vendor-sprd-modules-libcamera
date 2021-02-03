#ifndef ATM_H_
#define ATM_H_

#ifdef WIN32
#include "sci_types.h"
#endif

#ifdef CONFIG_LINUX_LIKE
#include <linux/types.h>
#include <sys/types.h>
#include <cmr_property.h>
#include "stdint.h"
//#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <android/log.h>
#include <sys/system_properties.h>
#endif
#include "CurveUtility.h"
/*------------------------------------------------------------------------------*
 *				Compiler Flag					*
 *-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    ATM_RET_OK,
    ATM_RET_NO_PIXEL,
    ATM_RET_HIST_INVALID,
    ATM_RET_NULLPTR,
} ATM_RET;

typedef enum {
    ATM_MODE_NORMAL = 0,
    ATM_MODE_GLOBAL_ONLY,
    ATM_MODE_LOCAL_ONLY
} ATM_MODE;

struct ATMCalcParam
{
    int magic;
    int version;
    int date;//hardcode for algo matching
    int time;//hardcode for algo matching
    int check;

    // control
    ATM_MODE eMode;  // algo strategy

    int i4BV;
    int i4LowPT;
    int i4LowPcentThd;
    int i4LowRightThd;
    int i4LowLeftThd;
    int i4HighPT;
    int i4HighPcentThd;
    int i4HighRightThd;
    int i4HighLeftThd;
    GEN_LUT strBVLut;
};

#define MAX_SYNC_SENSORS (4)
#define AEC_CFG_NUM 8
#define U4BIT8BIN 256
#define ATM_TUNING_VERSION_V0 78 * 4
#define ATM_TUNING_VERSION_V1 738 * 4

struct _atm_init_param
{
    uint32_t u4Magic;
    uint8_t  uOrigGamma[256];
};

struct atm_table{
	uint8_t i4PT;
	uint8_t i4PcentThd;
	uint8_t i4RightThd;
	uint8_t i4LeftThd;
};/*1 * 4 bytes*/

struct atm_evd_table{
	int16_t evd_idx;
	uint16_t reserved;
	struct atm_table table[8];
};/*9 * 4 bytes*/

struct atm_bvt_table{
	int16_t bv_idx;
	uint16_t evd_num;
	struct atm_evd_table evd_table[8];	//72
};/*73 * 4 bytes*/

struct atm_tune_param_v1{
	uint16_t bv_num;
	uint16_t reserved;
	struct atm_bvt_table bv_table[8];	/*8 * 73 * 4 bytes*/
};/*585 * 4 bytes*/

/*for ATM algorithm*/
struct ae_atm_tune_param_v1{
	uint32_t version;
	uint32_t enable;					/*2 * 4 bytes*/
	uint8_t input_type;
	uint8_t output_type;
	uint8_t point_num;
	uint8_t reserved_0; 				/*1 * 4 bytes*/
	struct atm_tune_param_v1 tune;		/*585 * 4 bytes*/
	uint32_t reserved_1[150];			/*150 * 4 bytes*/
};/*738 * 4 bytes*/

struct atm_calc_param
{
    struct ATMCalcParam stAlgoParams;    // control parameters

    int date;				//ex: 20160331, hardcode for algo matching
    int time;				//ex: 221059, , hardcode for algo matching

    unsigned long long *pHist;
    uint32_t u4Bins;
    uint8_t *uBaseGamma;
    uint8_t *uModGamma;
    uint8_t bHistB4Gamma;
	uint32_t atm_version;
	struct ae_atm_tune_param_v1 atm_tune;
};

struct atm_calc_result
{
    ATM_RET eStatus;  // should be 0

    uint8_t *uGamma;
    int32_t i4RespCurve[256];
    uint8_t uLowPT;
    uint8_t uHighPT;
    uint8_t uFinalLowBin;
    uint8_t uFinalHighBin;
//    uint8_t *log_buffer;
//    uint32_t log_size;
};

typedef enum
{
    ATM_IOCTRL_SET_MODE = 1,
    ATM_IOCTRL_SET_STATE = 2,
    ATM_IOCTRL_CMD_MAX,
} ATM_IOCTL;


/*------------------------------------------------------------------------------*
 *				Function Prototype					*
 *-------------------------------------------------------------------------------*/

void *atm_init(struct _atm_init_param *init_param);
int atm_calc(void* atm_handle, struct atm_calc_param* calc_param, struct atm_calc_result* calc_result);
int atm_ioctrl(void *atm_handle, int cmd, void *param);
int atm_deinit(void *atm_handle);


/*------------------------------------------------------------------------------*
 *				Compiler Flag					*
 *-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
