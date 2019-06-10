
#ifndef _TOF_CTRL_H
#define _TOF_CTRL_H

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "isp_mw.h"
#include "af_ctrl.h"
#include "af_sprd_adpt_v1.h"


#define ISP_THREAD_QUEUE_NUM                 (100)

typedef cmr_int(*_tof_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

struct tof_ctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

struct tof_ctrl_cxt {
	cmr_s32 is_deinit;  // 0: tof not deinit , 1: tof is deinit
	pthread_t thr_handle;
	//cmr_handle thr_handle;
	cmr_handle caller_handle;
	_tof_cb tof_set_cb;
};

//******************************** IOCTL definitions
#define VL53L0_IOCTL_INIT			_IO('p', 0x01)
#define VL53L0_IOCTL_XTALKCALB		_IOW('p', 0x02, unsigned int)
#define VL53L0_IOCTL_OFFCALB		_IOW('p', 0x03, unsigned int)
#define VL53L0_IOCTL_STOP			_IO('p', 0x05)
#define VL53L0_IOCTL_SETXTALK		_IOW('p', 0x06, unsigned int)
#define VL53L0_IOCTL_SETOFFSET		_IOW('p', 0x07, int8_t)
#define VL53L0_IOCTL_GETDATAS		_IOR('p', 0x0b, VL53L0_RangingMeasurementData_t)
#define VL53L0_IOCTL_PARAMETER		_IOWR('p', 0x0d, struct stmvl53l0_parameter)

typedef unsigned char VL53L0_DeviceModes;
#define VL53L0_DEVICEMODE_CONTINUOUS_RANGING	   ((VL53L0_DeviceModes)  1)

typedef enum {
	OFFSET_PAR = 0,
	XTALKRATE_PAR = 1,
	XTALKENABLE_PAR = 2,
	GPIOFUNC_PAR = 3,
	LOWTHRESH_PAR = 4,
	HIGHTHRESH_PAR = 5,
	DEVICEMODE_PAR = 6,
	INTERMEASUREMENT_PAR = 7,
	REFERENCESPADS_PAR = 8,
	REFCALIBRATION_PAR = 9,
} parameter_name_e;

struct stmvl53l0_parameter {
	uint32_t is_read;
	parameter_name_e name;
	int32_t value;
	int32_t value2;
	int32_t status;
};

typedef struct {
	unsigned int TimeStamp;
	unsigned int MeasurementTimeUsec;
	unsigned short RangeMilliMeter;
	unsigned short RangeDMaxMilliMeter;
	unsigned int SignalRateRtnMegaCps;
	unsigned int AmbientRateRtnMegaCps;
	unsigned short EffectiveSpadRtnCount;
	unsigned char ZoneId;
	unsigned char RangeFractionalPart;
	unsigned char RangeStatus;
} VL53L0_RangingMeasurementData_t;


struct tof_ctrl_init_in {
	_tof_cb tof_set_cb;
	cmr_handle caller_handle;
};

cmr_int tof_ctrl_init(struct tof_ctrl_init_in * input_ptr, cmr_handle * handle);
cmr_int tof_ctrl_deinit(cmr_handle * handle);

#endif