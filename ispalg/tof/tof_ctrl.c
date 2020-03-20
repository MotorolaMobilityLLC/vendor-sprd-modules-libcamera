/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tof_ctrl.h"

static int tof_fd;

cmr_int vl53l0_open_dev()
{
	tof_fd = open("/dev/stmvl53l0_ranging",O_RDWR | O_SYNC);
	if (tof_fd <= 0) {
		ISP_LOGE("Error open stmvl53l0_ranging device,error num:%s",strerror(errno));
		return ISP_ERROR;
	}
	return ISP_SUCCESS;
}

cmr_int vl53l0_init()
{
	cmr_int rtn = ISP_SUCCESS;
	//read xtak calibration data
	FILE *fp_xtak = NULL;
	int XtalkInt = 0;
	int XtalkEnable = 0;
	int Xtalk_rtn[2] = {0};

	//fp_xtak = fopen("/productinfo/vl53l0_xtak_calibration.file","rb");
	fp_xtak = fopen("/mnt/vendor/vl53l0_xtak_calibration.file","rb");
	if(NULL!=fp_xtak){
		Xtalk_rtn[0] = fscanf(fp_xtak, "%d\n", &XtalkInt);
		Xtalk_rtn[1] = fscanf(fp_xtak, "%d\n", &XtalkEnable);
		if(Xtalk_rtn[0] == EOF || Xtalk_rtn[1] == EOF){
			fclose(fp_xtak);
			ISP_LOGE("Error: Could not read Xtalk Values from vl53l0_xtak_calibration.file");
			rtn = ISP_ERROR;
			goto exit;
		}
		fclose(fp_xtak);
		ISP_LOGI("XtalkInt=%d, XtalkEnable=%d", XtalkInt, XtalkEnable);
	}

	// read offset calibraion data
	FILE *fp_offset = NULL;
	int offset = 0;
	int VhvSettings = 0;
	int PhaseCal = 0;
	int SpadCount = 0;
	int IsApertureSpads = 0;
	int offset_rtn[5] = {0};

	//fp_offset = fopen("/productinfo/vl53l0_offset_calibration.file", "rb");
	fp_offset = fopen("/mnt/vendor/vl53l0_offset_calibration.file", "rb");
	if(NULL!=fp_offset){
		offset_rtn[0] = fscanf(fp_offset, "%d\n", &offset);
		offset_rtn[1] = fscanf(fp_offset, "%d\n", &VhvSettings);
		offset_rtn[2] = fscanf(fp_offset, "%d\n", &PhaseCal);
		offset_rtn[3] = fscanf(fp_offset, "%d\n", &SpadCount);
		offset_rtn[4] = fscanf(fp_offset, "%d\n", &IsApertureSpads);
		if(offset_rtn[0] == EOF || offset_rtn[1] == EOF || offset_rtn[2] == EOF || offset_rtn[3] == EOF || offset_rtn[4] == EOF){
			fclose(fp_offset);
			ISP_LOGE("Error: Could not read offset Values from vl53l0_offset_calibration.file");
			rtn = ISP_ERROR;
			goto exit;
		}
		fclose(fp_offset);
		ISP_LOGI("offset=%d, VhvSettings=%d, PhaseCal=%d, SpadCount=%d, IsApertureSpads=%d", offset, VhvSettings,PhaseCal,SpadCount,IsApertureSpads);
	}

	struct stmvl53l0_parameter parameter;
	parameter.is_read = 0;
	parameter.value = SpadCount;
	parameter.value2 = IsApertureSpads;
	parameter.name =REFERENCESPADS_PAR;
	if (ioctl(tof_fd, VL53L0_IOCTL_PARAMETER, &parameter) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_PARAMETER: %s", strerror(errno));
		rtn = ISP_ERROR;
		goto exit;
	}

	parameter.is_read = 0;
	parameter.name  = OFFSET_PAR;
	parameter.value= offset;
	if (ioctl(tof_fd, VL53L0_IOCTL_PARAMETER, &parameter) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_PARAMETER");
		rtn = ISP_ERROR;
		goto exit;
	}

	parameter.is_read = 0;
	parameter.name = XTALKENABLE_PAR;
	parameter.value = XtalkEnable;
	if (ioctl(tof_fd, VL53L0_IOCTL_PARAMETER , &parameter) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_PARAMETER");
		rtn = ISP_ERROR;
		goto exit;
	}

	parameter.is_read = 0;
	parameter.name  = XTALKRATE_PAR;
	parameter.value = XtalkInt;
	if (ioctl(tof_fd, VL53L0_IOCTL_PARAMETER, &parameter) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_PARAMETER");
		rtn = ISP_ERROR;
		goto exit;
	}

	parameter.is_read = 0;
	parameter.name = DEVICEMODE_PAR;
	parameter.value = VL53L0_DEVICEMODE_CONTINUOUS_RANGING;
	if (ioctl(tof_fd, VL53L0_IOCTL_PARAMETER, &parameter) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_PARAMETER(CONTINOUS_TIMED_RANGING)");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (ioctl(tof_fd, VL53L0_IOCTL_INIT , NULL) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_INIT");
		rtn = ISP_ERROR;
		goto exit;
	}
	return rtn;

  exit:
	close(tof_fd);
	return rtn;
}

cmr_int vl53l0_deinit()
{
	if (ioctl(tof_fd, VL53L0_IOCTL_STOP , NULL) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform VL53L0_IOCTL_DEINIT");
		close(tof_fd);
		return ISP_ERROR;
	}
	close(tof_fd);
	return ISP_SUCCESS;
}

cmr_int vl53l0_getdata(void * data)
{
	char value[PROPERTY_VALUE_MAX] = { '\0' };
	VL53L0_RangingMeasurementData_t *range_datas_call =  (VL53L0_RangingMeasurementData_t *)data;
	VL53L0_RangingMeasurementData_t range_datas = {0};

	//to get datas
	if (ioctl(tof_fd, VL53L0_IOCTL_GETDATAS,&range_datas) != ISP_SUCCESS) {
		ISP_LOGE("Error: Could not perform vl53l0_getdata");
		return ISP_ERROR;
	}

	memcpy(range_datas_call, &range_datas, sizeof(VL53L0_RangingMeasurementData_t));

	ISP_LOGV("VL53L0 Range MilliMeter:%4d, status:0x%x, DMax MilliMeter:%4d\n",
		range_datas.RangeMilliMeter, range_datas.RangeStatus, range_datas.RangeDMaxMilliMeter);

	property_get("persist.vendor.cam.tofmlog.enable", value, "1");

	//Mlog
	if(atoi(value) == 1){

	        FILE *pf = NULL;
	        const char saveLogFile[50] = "/data/mlog/tof.txt";
			pf = fopen(saveLogFile, "wb");
	        if (NULL != pf){

	            fprintf(pf, "VL53L0 Range MilliMeter:%4d \n",range_datas.RangeMilliMeter);
				fprintf(pf, "status:0x%x \n",range_datas.RangeStatus);
				fprintf(pf, "DMax MilliMeter:%4d \n",range_datas.RangeDMaxMilliMeter);

				fprintf(pf, "SignalRate:%4d \n",range_datas.SignalRateRtnMegaCps);
				fprintf(pf, "AmbientRate:%4d \n",range_datas.AmbientRateRtnMegaCps);
				fprintf(pf, "EffectiveSpad:%4d \n",range_datas.EffectiveSpadRtnCount);
				fprintf(pf, "RangeFractionalPart:%4d \n",range_datas.RangeFractionalPart);
				//fprintf(pf, "ZoneId:%4d \n",range_datas.ZoneId);
				//fprintf(pf, "TimeStamp:%4d \n",range_datas.TimeStamp);
	            fclose(pf);
			}
	}

	return ISP_SUCCESS;
}


static cmr_u32 _set_tof_info_to_af(void *handle, VL53L0_RangingMeasurementData_t *in_param)
{
	struct tof_ctrl_cxt *cxt_ptr = (struct tof_ctrl_cxt *)handle;

	if (cxt_ptr->tof_set_cb) {
		cxt_ptr->tof_set_cb(cxt_ptr->caller_handle, ISP_AF_SET_TOF_INFO, (void *)in_param, NULL);
	}

	return ISP_SUCCESS;
}

#if 0

static cmr_int  _tof_ctrl_thr_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int rtn = ISP_SUCCESS;
	struct tof_ctrl_cxt *cxt_ptr = (struct tof_ctrl_cxt *)p_data;
	VL53L0_RangingMeasurementData_t data;

	UNUSED(message);

	if (NULL == cxt_ptr) {
		ISP_LOGE("Error: handle is NULL");
		return rtn;
	}

	while(1){

		vl53l0_getdata(&data);
		_set_tof_info_to_af(cxt_ptr, &data);

		usleep(30000);
	}

	return rtn;
}

static cmr_s32 _tof_create_thread(struct tof_ctrl_cxt *cxt_ptr)
{
	  cmr_int rtn = ISP_SUCCESS;

	  rtn = cmr_thread_create(&cxt_ptr->thr_handle, ISP_THREAD_QUEUE_NUM, _tof_ctrl_thr_proc, (void *)cxt_ptr);
	  if (rtn) {
		  ISP_LOGE("fail to create ctrl thread");
		  rtn = ISP_ERROR;
		  goto exit;
	  }
	  rtn = cmr_thread_set_name(cxt_ptr->thr_handle, "tof");
	  if (CMR_MSG_SUCCESS != rtn) {
		  ISP_LOGE("fail to set tof ctrl name");
		  rtn = CMR_MSG_SUCCESS;
	  }
	exit:
	  ISP_LOGI("tof_ctrl thread rtn %ld", rtn);
	  return rtn;
}
#endif

static void *  _tof_ctrl_thr_proc(void *p_data)
{
	struct tof_ctrl_cxt *cxt_ptr = (struct tof_ctrl_cxt *)p_data;
	VL53L0_RangingMeasurementData_t data;
	int sleep_rtn = 0;

	if (NULL == cxt_ptr) {
		ISP_LOGE("Error: handle is NULL");
		return NULL;
	}

	while(1){

	if(1==cxt_ptr->is_deinit)
			break;

		if(ISP_SUCCESS == vl53l0_getdata(&data)){
			_set_tof_info_to_af(cxt_ptr, &data);
		}

		sleep_rtn = usleep(30000);
		if(sleep_rtn)
			ISP_LOGE("Error: No pause for 30ms!");
	}

	return NULL;
}

static cmr_s32 _tof_create_thread(struct tof_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	rtn = pthread_create(&cxt_ptr->thr_handle, &attr, _tof_ctrl_thr_proc, (void *)cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to create tof ctrl thread");
		rtn = ISP_ERROR;
		goto exit;
	}

	pthread_setname_np(cxt_ptr->thr_handle, "tof_ctrl");
	pthread_attr_destroy(&attr);

	cxt_ptr->is_deinit = 0;

  exit:
	ISP_LOGI("tof_ctrl thread rtn %ld", rtn);
	return rtn;
}



static cmr_s32 _tof_destroy_thread(struct tof_ctrl_cxt *cxt_ptr)
{
	cmr_int rtn = ISP_SUCCESS;
	void *dummy;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param , param is NULL");
		rtn = ISP_ERROR;
		goto exit;
	}

	if (cxt_ptr->thr_handle) {
	//	rtn = cmr_thread_destroy(cxt_ptr->thr_handle);
		if (!rtn) {
		//	cxt_ptr->thr_handle = NULL;
			cxt_ptr->is_deinit = 1;
			rtn = pthread_join(cxt_ptr->thr_handle, &dummy);
			cxt_ptr->thr_handle = 0;
		} else {
			ISP_LOGE("fail to destroy ctrl thread %ld", rtn);
		}
	}
  exit:
	ISP_LOGI("done %ld", rtn);
	return rtn;
}

cmr_int tof_ctrl_init(struct tof_ctrl_init_in * input_ptr, cmr_handle * handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct tof_ctrl_cxt *cxt_ptr = NULL;

	cxt_ptr = (struct tof_ctrl_cxt *)malloc(sizeof(struct tof_ctrl_cxt));
	if (NULL == cxt_ptr) {
		ISP_LOGE("fail to create tof ctrl context!");
		rtn = ISP_ALLOC_ERROR;
		goto exit;
	}
	memset(cxt_ptr, 0, sizeof(struct tof_ctrl_cxt));

	rtn = vl53l0_open_dev();
	if (rtn) {
		ISP_LOGE("fail to open tof dev");
		goto exit;
	} else {
		ISP_LOGI("open tof dev success");
	}

	rtn = vl53l0_init();
	if (rtn) {
		ISP_LOGE("fail to vl53l0_init %ld", rtn);
		goto exit;
	}

	rtn = _tof_create_thread(cxt_ptr);
	if (rtn) {
		goto exit;
	}

	cxt_ptr->tof_set_cb = input_ptr->tof_set_cb;
	cxt_ptr->caller_handle = input_ptr->caller_handle;

  exit:
	if (rtn) {
		if (cxt_ptr) {
			free(cxt_ptr);
		}
	} else {
		*handle = (cmr_handle) cxt_ptr;
	}
	ISP_LOGI("done %ld", rtn);

	return rtn;
}

cmr_int tof_ctrl_deinit(cmr_handle * handle)
{
	cmr_int rtn = ISP_SUCCESS;
	struct tof_ctrl_cxt *cxt_ptr = *handle;

	if (!cxt_ptr) {
		ISP_LOGE("fail to check param, param is NULL!");
		rtn = ISP_ERROR;
		goto exit;
	}

	rtn = _tof_destroy_thread(cxt_ptr);
	if (rtn) {
		ISP_LOGE("fail to destroy tof ctrl thread %ld", rtn);
		goto exit;
	}

	rtn = vl53l0_deinit();
	if (rtn) {
		ISP_LOGE("fail to vl53l0_deinit %ld", rtn);
		goto exit;
	}

  exit:
	if (cxt_ptr) {
		free((void *)cxt_ptr);
		*handle = NULL;
	}

	ISP_LOGI("done %ld", rtn);
	return rtn;
}

