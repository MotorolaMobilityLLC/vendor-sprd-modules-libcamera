/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V1.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#define LOG_TAG "gc5035_arb_qogirl6"  

#include "sensor_gc5035_arb_mipi_raw.h"
pthread_mutex_t gc5035_sensor_mutex;


/*==============================================================================
 * Description:
 * write register value to sensor
 * please modify this function acording your spec
 *============================================================================*/

static void gc5035_arb_drv_write_reg2sensor(cmr_handle handle, struct sensor_i2c_reg_tab *reg_info)
{
	SENSOR_IC_CHECK_PTR_VOID(reg_info);
	SENSOR_IC_CHECK_HANDLE_VOID(handle);
	
	struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_int i = 0;

	for (i = 0; i < reg_info->size; i++) {
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, reg_info->settings[i].reg_addr, reg_info->settings[i].reg_value);
	}
}

/*==============================================================================
 * Description:
 * write gain to sensor registers buffer
 * please modify this function acording your spec
 *============================================================================*/

static void gc5035_arb_drv_write_gain(cmr_handle handle, struct sensor_aec_i2c_tag *aec_info , cmr_u32 gain)
{
	SENSOR_IC_CHECK_PTR_VOID(aec_info);
	SENSOR_IC_CHECK_HANDLE_VOID(handle);
	struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u32 temp_gain;
	cmr_int gain_index;
	cmr_u16 GC5035_ARB_AGC_Param[MAX_GAIN_INDEX][2] = {
		{  256,  0 },
		{  302,  1 },
		{  358,  2 },
		{  425,  3 },
		{  502,  8 },
		{  599,  9 },
		{  717, 10 },
		{  845, 11 },
		{ 998,  12 },
		{ 1203, 13 },
		{ 1434, 14 },
		{ 1710, 15 },
		{ 1997, 16 },
		{ 2355, 17 },
		{ 2816, 18 },
		{ 3318, 19 },
		{ 3994, 20 },
	};

	for (gain_index = E_GAIN_INDEX - 1; gain_index >= 0; gain_index--)
		if (gain >= GC5035_ARB_AGC_Param[gain_index][0])
			break;

	aec_info->again->settings[0].reg_value = 0x00;
	aec_info->again->settings[1].reg_value = GC5035_ARB_AGC_Param[gain_index][1];
	temp_gain = gain * Dgain_ratio / GC5035_ARB_AGC_Param[gain_index][0];
	aec_info->again->settings[2].reg_value = (temp_gain >> 8) & 0x0f;
	aec_info->again->settings[3].reg_value = temp_gain & 0xfc;

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_u16 gc5035_arb_drv_read_frame_length(cmr_handle handle)
{
	SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u16 frame_len_h=0;
	cmr_u16	frame_len_l=0;

	frame_len_h = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x41) & 0x3f;
	frame_len_l = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x42) & 0xff;

    return ((frame_len_h << 8) | frame_len_l);

}
/*==============================================================================
 * Description:
 * write frame length to sensor registers buffer
 * please modify this function acording your spec
 *============================================================================*/
static void gc5035_arb_drv_write_frame_length(cmr_handle handle, struct sensor_aec_i2c_tag *aec_info, cmr_u32 frame_len)
{
    SENSOR_IC_CHECK_PTR_VOID(aec_info);
	SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u32 frame_length = 0;

	if(aec_info->frame_length->size){
		/*TODO*/
		frame_length = frame_len >> 2;
		frame_length = frame_length << 2;

		aec_info->frame_length->settings[0].reg_value = 0;
		aec_info->frame_length->settings[1].reg_value = (frame_length >> 8) & 0x3f;
		aec_info->frame_length->settings[2].reg_value = frame_length & 0xff;

		/*END*/
	}

}


/*==============================================================================
 * Description:
 * write shutter to sensor registers buffer
 * please pay attention to the frame length 
 * please modify this function acording your spec
 *============================================================================*/
static void gc5035_arb_drv_write_shutter(cmr_handle handle, struct sensor_aec_i2c_tag *aec_info , cmr_u32 shutter)
{
	SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u16 cal_shutter = 0;

	if(shutter < 4) shutter = 4;
	if(shutter > 16383) shutter = 16383;

	cal_shutter = shutter >> 2;	
	cal_shutter = cal_shutter << 2;
	Dgain_ratio = 256 * shutter / cal_shutter;
	
	if(aec_info->shutter->size){
		/*TODO*/
		aec_info->shutter->settings[0].reg_value = 0x00;
		aec_info->shutter->settings[1].reg_value = cal_shutter & 0xff;
		aec_info->shutter->settings[2].reg_value = (cal_shutter >> 8) & 0x3f;
		
		/*END*/
	}
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static void gc5035_arb_drv_calc_exposure(cmr_handle handle, cmr_u32 shutter,cmr_u32 dummy_line, 
                                                  cmr_u16 mode, struct sensor_aec_i2c_tag *aec_info)
{
	cmr_u32 dest_fr_len = 0;
	cmr_u32 cur_fr_len = 0;
	cmr_u32 fr_len = 0;
    float fps = 0.0;
    cmr_u16 frame_interval = 0x00;

    SENSOR_IC_CHECK_PTR_VOID(aec_info);
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
	struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    
	sns_drv_cxt->frame_length_def = sns_drv_cxt->trim_tab_info[mode].frame_line;
    sns_drv_cxt->line_time_def = sns_drv_cxt->trim_tab_info[mode].line_time;
	cur_fr_len = sns_drv_cxt->sensor_ev_info.preview_framelength;
    fr_len = sns_drv_cxt->frame_length_def;
	
	dummy_line = dummy_line > FRAME_OFFSET ? dummy_line : FRAME_OFFSET;
	dest_fr_len = ((shutter + dummy_line) > fr_len) ? (shutter +dummy_line) : fr_len;
    sns_drv_cxt->frame_length = dest_fr_len;

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (cur_fr_len > shutter) {
	 fps = 1000000000.0 / (cur_fr_len * sns_drv_cxt->trim_tab_info[mode].line_time);
	} else {
	 fps = 1000000000.0 / ((shutter + dummy_line) * sns_drv_cxt->trim_tab_info[mode].line_time);
	}
	SENSOR_LOGI("fps = %f", fps);

    frame_interval = (cmr_u16)(((shutter + dummy_line) *
               sns_drv_cxt->line_time_def) / 1000000);
	SENSOR_LOGI("mode = %d, exposure_line = %d, dummy_line= %d, frame_interval= %d ms",
					mode, shutter, dummy_line, frame_interval);

	if (dest_fr_len != cur_fr_len){
		sns_drv_cxt->sensor_ev_info.preview_framelength = dest_fr_len;
		gc5035_arb_drv_write_frame_length(handle, aec_info, sns_drv_cxt->sensor_ev_info.preview_framelength);
	}
    sns_drv_cxt->sensor_ev_info.preview_shutter = shutter;
	gc5035_arb_drv_write_shutter(handle, aec_info, shutter);

    if(sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle,
                          SENSOR_EXIF_CTRL_EXPOSURETIME, shutter);
    }
}


static void gc5035_arb_drv_calc_gain(cmr_handle handle,cmr_uint isp_gain, struct sensor_aec_i2c_tag *aec_info) 
{
	SENSOR_IC_CHECK_HANDLE_VOID(handle);
	struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u32 sensor_gain = 0;

	sensor_gain = isp_gain < ISP_BASE_GAIN ? ISP_BASE_GAIN : isp_gain;
	sensor_gain = sensor_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;
	
	if (SENSOR_MAX_GAIN < sensor_gain)
			sensor_gain = SENSOR_MAX_GAIN;

	SENSOR_LOGI("isp_gain = 0x%x,sensor_gain=0x%x", (unsigned int)isp_gain,sensor_gain);

	sns_drv_cxt->sensor_ev_info.preview_gain=sensor_gain;
	gc5035_arb_drv_write_gain(handle, aec_info,sensor_gain);
	
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_arb_drv_power_on(cmr_handle handle, cmr_uint power_on)
{
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    SENSOR_AVDD_VAL_E dvdd_val = module_info->dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = module_info->avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = module_info->iovdd_val;
    BOOLEAN power_down = g_gc5035_arb_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_gc5035_arb_mipi_raw_info.reset_pulse_level;	
	
    if (SENSOR_TRUE == power_on) 
	{
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        usleep(12 * 1000);		
		hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, iovdd_val);
		usleep(1 * 1000);		
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, dvdd_val);
        usleep(1 * 1000);		
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, avdd_val);
        usleep(1 * 1000);		
        hw_sensor_set_mclk(sns_drv_cxt->hw_handle, EX_MCLK);
        usleep(10 * 1000);		
        hw_sensor_power_down(sns_drv_cxt->hw_handle, !power_down);
        usleep(1 * 1000);		
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, !reset_level);
        usleep(6 * 1000);
		SENSOR_LOGE("[simon]test3");
	    sns_drv_cxt->current_state_machine = SENSOR_STATE_POWER_ON;
    }
	else 
	{
        hw_sensor_power_down(sns_drv_cxt->hw_handle, power_down);
        usleep(200);		
        hw_sensor_set_reset_level(sns_drv_cxt->hw_handle, reset_level);
		hw_sensor_set_mclk(sns_drv_cxt->hw_handle, SENSOR_DISABLE_MCLK);
        usleep(500);
        hw_sensor_set_avdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_dvdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
        hw_sensor_set_iovdd_val(sns_drv_cxt->hw_handle, SENSOR_AVDD_CLOSED);
		sns_drv_cxt->current_state_machine = SENSOR_STATE_POWER_OFF;
    }
	
    SENSOR_LOGI("(1:on, 0:off): %lu", power_on);
    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * calculate fps for every sensor mode according to frame_line and line_time
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_arb_drv_init_fps_info(cmr_handle handle) 
{
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_trim_tag *trim_info = sns_drv_cxt->trim_tab_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;

    SENSOR_LOGI("E");
    if (!fps_info->is_init) {
        cmr_u32 i, modn, tempfps = 0;
        SENSOR_LOGI("start init");
        for (i = 0; i < SENSOR_MODE_MAX; i++) {
            // max fps should be multiple of 30,it calulated from line_time and
            // frame_line
            tempfps = trim_info[i].line_time * trim_info[i].frame_line;
            if (0 != tempfps) {
                tempfps = 1000000000 / tempfps;
                modn = tempfps / 30;
                if (tempfps > modn * 30)
                    modn++;
                fps_info->sensor_mode_fps[i].max_fps = modn * 30;
                if (fps_info->sensor_mode_fps[i].max_fps > 30) {
                    fps_info->sensor_mode_fps[i].is_high_fps = 1;
                    fps_info->sensor_mode_fps[i].high_fps_skip_num =
                        fps_info->sensor_mode_fps[i].max_fps / 30;
                }
                if (fps_info->sensor_mode_fps[i].max_fps >
                    static_info->max_fps) {
                    static_info->max_fps = fps_info->sensor_mode_fps[i].max_fps;
                }
            }
            SENSOR_LOGI("mode %d,tempfps %d,frame_len %d,line_time: %d ", i,
                         tempfps, trim_info[i].frame_line,
                         trim_info[i].line_time);
            SENSOR_LOGI("mode %d,max_fps: %d ", i,
                         fps_info->sensor_mode_fps[i].max_fps);
            SENSOR_LOGI("is_high_fps: %d,highfps_skip_num %d",
                fps_info->sensor_mode_fps[i].is_high_fps,
                fps_info->sensor_mode_fps[i].high_fps_skip_num);
        }
        fps_info->is_init = 1;
    }
    SENSOR_LOGI("X");
    return rtn;
}

static EXIF_SPEC_PIC_TAKING_COND_T s_gc5035_arb_exif_info;
static cmr_int gc5035_arb_drv_init_exif_info(cmr_handle handle) 
{
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_gc5035_arb_exif_info;
	memset(&s_gc5035_arb_exif_info, 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
	SENSOR_LOGI("Start");

	exif_ptr->valid.FNumber = 1;
	exif_ptr->FNumber.numerator = 11;
	exif_ptr->FNumber.denominator = 5;
	exif_ptr->valid.ApertureValue = 1;
	exif_ptr->ApertureValue.numerator = 11;
	exif_ptr->ApertureValue.denominator = 5;
	exif_ptr->valid.MaxApertureValue = 1;
	exif_ptr->MaxApertureValue.numerator = 11;
	exif_ptr->MaxApertureValue.denominator = 5;
	exif_ptr->valid.FocalLength = 1;
	exif_ptr->FocalLength.numerator = 354;
	exif_ptr->FocalLength.denominator = 100;

	return rtn;
}
static cmr_int gc5035_arb_drv_get_static_info(cmr_handle handle, cmr_u32 *param) 
{
    cmr_int rtn = SENSOR_SUCCESS;
    struct sensor_ex_info *ex_info = (struct sensor_ex_info *)param;
    cmr_u32 up = 0;
    cmr_u32 down = 0;
    
	SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(ex_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    struct sensor_fps_info *fps_info = sns_drv_cxt->fps_info;
    struct sensor_static_info *static_info = sns_drv_cxt->static_info;
    struct module_cfg_info *module_info = sns_drv_cxt->module_info;

    // make sure we have get max fps of all settings.
    if (!fps_info->is_init) {
        gc5035_arb_drv_init_fps_info(handle);
    }
    ex_info->f_num = static_info->f_num;
    ex_info->focal_length = static_info->focal_length;
    ex_info->max_fps = static_info->max_fps;
    ex_info->max_adgain = static_info->max_adgain;
    ex_info->ois_supported = static_info->ois_supported;
    ex_info->pdaf_supported = static_info->pdaf_supported;
    ex_info->exp_valid_frame_num = static_info->exp_valid_frame_num;
    ex_info->clamp_level = static_info->clamp_level;
    ex_info->adgain_valid_frame_num = static_info->adgain_valid_frame_num;
    ex_info->preview_skip_num = module_info->preview_skip_num;
    ex_info->capture_skip_num = module_info->capture_skip_num;
    ex_info->name = (cmr_s8 *)g_gc5035_arb_mipi_raw_info.name;
    ex_info->sensor_version_info = (cmr_s8 *)g_gc5035_arb_mipi_raw_info.sensor_version_info;
    memcpy(&ex_info->fov_info, &static_info->fov_info, sizeof(static_info->fov_info));
    ex_info->pos_dis.up2hori = up;
    ex_info->pos_dis.hori2down = down;
    sensor_ic_print_static_info((cmr_s8 *)SENSOR_NAME, ex_info);

    return rtn;
}

static cmr_int gc5035_arb_drv_get_fps_info(cmr_handle handle, cmr_u32 *param) 
{
    cmr_int rtn = SENSOR_SUCCESS;
    SENSOR_MODE_FPS_T *fps_info = (SENSOR_MODE_FPS_T *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(fps_info);
    SENSOR_IC_CHECK_PTR(param);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    struct sensor_fps_info *fps_data = sns_drv_cxt->fps_info;

    // make sure have inited fps of every sensor mode.
    if (!fps_data->is_init) {
        gc5035_arb_drv_init_fps_info(handle);
    }
    cmr_u32 sensor_mode = fps_info->mode;
    fps_info->max_fps = fps_data->sensor_mode_fps[sensor_mode].max_fps;
    fps_info->min_fps = fps_data->sensor_mode_fps[sensor_mode].min_fps;
    fps_info->is_high_fps = fps_data->sensor_mode_fps[sensor_mode].is_high_fps;
    fps_info->high_fps_skip_num =
        fps_data->sensor_mode_fps[sensor_mode].high_fps_skip_num;
    SENSOR_LOGI("mode %d, max_fps: %d", fps_info->mode, fps_info->max_fps);
    SENSOR_LOGI("min_fps: %d", fps_info->min_fps);
    SENSOR_LOGI("is_high_fps: %d", fps_info->is_high_fps);
    SENSOR_LOGI("high_fps_skip_num: %d", fps_info->high_fps_skip_num);

    return rtn;
}


static cmr_u8 gc5035_arb_read_otp(cmr_handle handle, cmr_u8 addr)
{
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	
	SENSOR_LOGI("E!\n");
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x02);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x69, (addr >> 8) & 0x1f);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x6a, addr & 0xff);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf3, 0x20);
	return  hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x6c);
}

static cmr_u8 gc5035_arb_otp_read_group(cmr_handle handle, cmr_u16 addr, cmr_u8 *data, cmr_u16 length)
{
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u16 i = 0;

	if ((((addr & 0x1fff) >> 3) + length) > GC5035_ARB_OTP_DATA_LENGTH) {
		SENSOR_PRINT("out of range, start addr: 0x%.4x, length = %d\n", addr & 0x1fff, length);
		return CMR_CAMERA_FAIL;
	}

	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x02);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x69, (addr >> 8) & 0x1f);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x6a, addr & 0xff);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf3, 0x20);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf3, 0x12);

	for (i = 0; i < length; i++)
		data[i] = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x6c);

	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf3, 0x00);
	return CMR_CAMERA_SUCCESS;
}

static void gc5035_arb_gcore_read_dpc(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u32 dpcFlag = 0;
	struct gc5035_arb_dpc_t *pDPC = &gc5035_arb_otp_info.dpc;
	/* Static Defective Pixel */
	dpcFlag = gc5035_arb_read_otp(handle, GC5035_ARB_OTP_DPC_FLAG_OFFSET);
	SENSOR_PRINT("dpc flag = 0x%x\n", dpcFlag);
	switch (GC5035_ARB_OTP_GET_2BIT_FLAG(dpcFlag, 0)) {
	case GC5035_ARB_OTP_FLAG_EMPTY: {
		SENSOR_PRINT("dpc info is empty!\n");
		pDPC->flag = GC5035_ARB_OTP_FLAG_EMPTY;
		break;
	}
	case GC5035_ARB_OTP_FLAG_VALID: {
		SENSOR_PRINT("dpc info is valid!\n");
		pDPC->total_num = gc5035_arb_read_otp(handle, GC5035_ARB_OTP_DPC_TOTAL_NUMBER_OFFSET)
			+ gc5035_arb_read_otp(handle, GC5035_ARB_OTP_DPC_ERROR_NUMBER_OFFSET);
		pDPC->flag = GC5035_ARB_OTP_FLAG_VALID;
		SENSOR_PRINT("total_num = %d\n", pDPC->total_num);
		break;
	}
	default:
		pDPC->flag = GC5035_ARB_OTP_FLAG_INVALID;
		break;
	}
}

static void gc5035_arb_gcore_read_reg(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u8 i = 0;
	cmr_u8 j = 0;
	cmr_u16 base_group = 0;
	cmr_u8 reg[GC5035_ARB_OTP_REG_DATA_SIZE];
	struct gc5035_arb_reg_update_t *pRegs = &gc5035_arb_otp_info.regs;

	memset(&reg, 0, GC5035_ARB_OTP_REG_DATA_SIZE);
	pRegs->flag = gc5035_arb_read_otp(handle, GC5035_ARB_OTP_REG_FLAG_OFFSET);
	SENSOR_PRINT("register update flag = 0x%x\n", pRegs->flag);
	if (pRegs->flag == GC5035_ARB_OTP_FLAG_VALID) {
		gc5035_arb_otp_read_group(handle, GC5035_ARB_OTP_REG_DATA_OFFSET, &reg[0], GC5035_ARB_OTP_REG_DATA_SIZE);

		for (i = 0; i < GC5035_ARB_OTP_REG_MAX_GROUP; i++) {
			base_group = i * GC5035_ARB_OTP_REG_BYTE_PER_GROUP;
			for (j = 0; j < GC5035_ARB_OTP_REG_REG_PER_GROUP; j++)
				if (GC5035_ARB_OTP_CHECK_1BIT_FLAG(reg[base_group], (4 * j + 3))) {
					pRegs->reg[pRegs->cnt].page =
						(reg[base_group] >> (4 * j)) & 0x07;
					pRegs->reg[pRegs->cnt].addr =
						reg[base_group + j * GC5035_ARB_OTP_REG_BYTE_PER_REG + 1];
					pRegs->reg[pRegs->cnt].value =
						reg[base_group + j * GC5035_ARB_OTP_REG_BYTE_PER_REG + 2];
					SENSOR_PRINT("register[%d] P%d:0x%x->0x%x\n",
						pRegs->cnt, pRegs->reg[pRegs->cnt].page,
						pRegs->reg[pRegs->cnt].addr, pRegs->reg[pRegs->cnt].value);
					pRegs->cnt++;
				}
		}

	}
}

static void gc5035_arb_otp_read_sensor_info(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

#if GC5035_ARB_OTP_DEBUG
	cmr_u16 i = 0;
	cmr_u8 debug[GC5035_ARB_OTP_DATA_LENGTH];
#endif

	gc5035_arb_gcore_read_dpc(handle);
	gc5035_arb_gcore_read_reg(handle);

#if GC5035_ARB_OTP_DEBUG
	memset(&debug[0], 0, GC5035_ARB_OTP_DATA_LENGTH);
	gc5035_arb_otp_read_group(handle, GC5035_ARB_OTP_START_ADDR, &debug[0], GC5035_ARB_OTP_DATA_LENGTH);
	for (i = 0; i < GC5035_ARB_OTP_DATA_LENGTH; i++)
		SENSOR_PRINT("addr = 0x%x, data = 0x%x\n", GC5035_ARB_OTP_START_ADDR + i * 8, debug[i]);
#endif
}

static void gc5035_arb_otp_update_dd(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u8 state = 0;
	cmr_u8 n = 0;
	struct gc5035_arb_dpc_t *pDPC = &gc5035_arb_otp_info.dpc;

	if (GC5035_ARB_OTP_FLAG_VALID == pDPC->flag) {
		SENSOR_PRINT("DD auto load start!\n");
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x17, GC5035_ARB_MIRROR);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x88);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x10);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x8e);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x88);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x10);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x8e);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x02);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xbe, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xa9, 0x01);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x09, 0x33);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x01, (pDPC->total_num >> 8) & 0x07);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x02, pDPC->total_num & 0xff);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x03, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x04, 0x80);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x95, 0x0a);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x96, 0x30);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x97, 0x0a);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x98, 0x32);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x99, 0x07);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x9a, 0xa9);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf3, 0x80);
		while (n < 3) {
			state = hw_sensor_read_reg(sns_drv_cxt->hw_handle, 0x06);
			if ((state | 0xfe) == 0xff)
				usleep(10 *1000);
			else
				n = 3;
			n++;
		}
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xbe, 0x01);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x09, 0x00);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x01);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x80, 0x02);
		hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	}
}

static void gc5035_arb_otp_update_reg(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u8 i = 0;

	SENSOR_PRINT("reg count = %d\n", gc5035_arb_otp_info.regs.cnt);

	if (GC5035_ARB_OTP_CHECK_1BIT_FLAG(gc5035_arb_otp_info.regs.flag, 0))
		for (i = 0; i < gc5035_arb_otp_info.regs.cnt; i++) {
			hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, gc5035_arb_otp_info.regs.reg[i].page);
			hw_sensor_write_reg(sns_drv_cxt->hw_handle, gc5035_arb_otp_info.regs.reg[i].addr, gc5035_arb_otp_info.regs.reg[i].value);
			SENSOR_PRINT("reg[%d] P%d:0x%x -> 0x%x\n", i, gc5035_arb_otp_info.regs.reg[i].page,
				gc5035_arb_otp_info.regs.reg[i].addr, gc5035_arb_otp_info.regs.reg[i].value);
		}
}

static void gc5035_arb_otp_update(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

	SENSOR_PRINT("in\n");
	gc5035_arb_otp_update_dd(handle);
	gc5035_arb_otp_update_reg(handle);
}

static void gc5035_arb_gcore_load_data(cmr_handle handle)
{
    SENSOR_IC_CHECK_HANDLE_VOID(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	cmr_u8 i = 0;
	cmr_u8 n = 0;
	cmr_u8 state = 0;

	SENSOR_PRINT("in\n");

	/*TODO*/
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x01);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf4, 0x40);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf5, 0xe9);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf6, 0x14);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf8, 0x49);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf9, 0x82);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfa, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x81);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x36, 0x01);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xd3, 0x87);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x36, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x33, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf7, 0x01);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfc, 0x8e);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xee, 0x30);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfa, 0x10);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xf5, 0xe9);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x02);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x67, 0xc0);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x59, 0x3f);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x55, 0x80);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x65, 0x80);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x66, 0x03);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);

	gc5035_arb_otp_read_sensor_info(handle);
	gc5035_arb_otp_update(handle);

	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x02);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x67, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfa, 0x00);
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_arb_drv_access_val(cmr_handle handle, cmr_uint param)
{
	cmr_int ret = SENSOR_FAIL;
    SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *)param;
    
	SENSOR_IC_CHECK_HANDLE(handle);
	SENSOR_IC_CHECK_PTR(param_ptr);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

	SENSOR_LOGI("sensor gc5035_arb: param_ptr->type=%x", param_ptr->type);
	
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_GET_STATIC_INFO:
			ret = gc5035_arb_drv_get_static_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_FPS_INFO:
			ret = gc5035_arb_drv_get_fps_info(handle, param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_SET_SENSOR_CLOSE_FLAG:
			ret = sns_drv_cxt->is_sensor_close = 1;
			break;
		case SENSOR_VAL_TYPE_GET_PDAF_INFO:
			//ret = gc5035_arb_drv_get_pdaf_info(handle, param_ptr->pval);
			break;
		default:
			break;
    }
    ret = SENSOR_SUCCESS;

    return ret;
}


/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_arb_drv_identify(cmr_handle handle, cmr_uint param)
{
	cmr_u16 pid_value = 0x00;
	cmr_u16 ver_value = 0x00;
	cmr_int ret_value = SENSOR_FAIL;
	
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	//struct gc5035_arb_otp *otp_info = &gc5035_arb_otp_info;

	SENSOR_LOGI("mipi raw identify");

	pid_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, GC5035_ARB_PID_ADDR);

	if (GC5035_ARB_PID_VALUE == pid_value) {
		ver_value = hw_sensor_read_reg(sns_drv_cxt->hw_handle, GC5035_ARB_VER_ADDR);
		SENSOR_LOGI("Identify: pid_value = %x, ver_value = %x", pid_value, ver_value);
		if (GC5035_ARB_VER_VALUE == ver_value) {
			SENSOR_LOGI("this is gc5035_arb sensor");
			//sensor_rid_save_sensor_name(1, "1_gc5035_arb");

			gc5035_arb_gcore_load_data(handle);

			ret_value = SENSOR_SUCCESS;
		} else {
			SENSOR_LOGE("sensor identify fail, pid_value = %x, ver_value = %x", pid_value, ver_value);
		}
	} else {
		SENSOR_LOGE("sensor identify fail, pid_value = %x, ver_value = %x", pid_value, ver_value);
	}

	return ret_value;
}


/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int gc5035_arb_drv_before_snapshot(cmr_handle handle, cmr_uint param)
{
	cmr_u32 cap_shutter = 0;
	cmr_u32 prv_shutter = 0;
	cmr_u32 prv_gain = 0;
	cmr_u32 cap_gain = 0;
	cmr_u32 capture_mode = param & 0xffff;
	cmr_u32 preview_mode = (param >> 0x10) & 0xffff;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	
	cmr_u32 prv_linetime = sns_drv_cxt->trim_tab_info[preview_mode].line_time;
	cmr_u32 cap_linetime = sns_drv_cxt->trim_tab_info[capture_mode].line_time;

	SENSOR_LOGI("preview_mode=%d,capture_mode = %d", preview_mode,capture_mode);
    SENSOR_LOGI("preview_shutter = 0x%x, preview_gain = 0x%x",
					sns_drv_cxt->sensor_ev_info.preview_shutter,
					(unsigned int)sns_drv_cxt->sensor_ev_info.preview_gain);


	if (preview_mode == capture_mode) {
        cap_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
        cap_gain = sns_drv_cxt->sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

    prv_shutter = sns_drv_cxt->sensor_ev_info.preview_shutter;
    prv_gain = sns_drv_cxt->sensor_ev_info.preview_gain;

    if(sns_drv_cxt->ops_cb.set_mode)
        sns_drv_cxt->ops_cb.set_mode(sns_drv_cxt->caller_handle, capture_mode);
    if(sns_drv_cxt->ops_cb.set_mode_wait_done)
        sns_drv_cxt->ops_cb.set_mode_wait_done(sns_drv_cxt->caller_handle);

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;
	cap_gain = prv_gain;

	SENSOR_LOGI("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
	
    gc5035_arb_drv_calc_exposure(handle,cap_shutter, 0 , capture_mode,&gc5035_arb_aec_info);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.frame_length);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.shutter);

	sns_drv_cxt->sensor_ev_info.preview_gain=cap_gain;
	gc5035_arb_drv_write_gain(handle, &gc5035_arb_aec_info, cap_gain);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.again);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.dgain);

snapshot_info:
    if(sns_drv_cxt->ops_cb.set_exif_info) {
        sns_drv_cxt->ops_cb.set_exif_info(sns_drv_cxt->caller_handle, SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);
    } else {
        sns_drv_cxt->exif_info.exposure_line = cap_shutter;
    }
	
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int gc5035_arb_drv_write_exposure(cmr_handle handle, cmr_uint param)
{
	cmr_int ret_value = SENSOR_SUCCESS;
	cmr_u16 exposure_line = 0x00;
	cmr_u16 dummy_line = 0x00;
	cmr_u16 size_index = 0x00;
    
	struct sensor_ex_exposure *ex = (struct sensor_ex_exposure *)param;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_HANDLE(ex);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    exposure_line = ex->exposure;
    dummy_line = ex->dummy;
    size_index = ex->size_index;

	gc5035_arb_drv_calc_exposure(handle,exposure_line, dummy_line, size_index, &gc5035_arb_aec_info);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.frame_length);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.shutter);

    return ret_value;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_int gc5035_arb_drv_write_gain_value(cmr_handle handle, cmr_uint param)
{
	cmr_int ret_value = SENSOR_SUCCESS;
	
	pthread_mutex_lock(&gc5035_sensor_mutex);
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

	gc5035_arb_drv_calc_gain(handle,param, &gc5035_arb_aec_info);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.again);
	gc5035_arb_drv_write_reg2sensor(handle, gc5035_arb_aec_info.dgain);
	pthread_mutex_unlock(&gc5035_sensor_mutex);

	return ret_value;
}

/*==============================================================================
 * Description:
 * read ae control info
 * please don't change this function unless it's necessary
 *============================================================================*/
static cmr_int gc5035_arb_drv_read_aec_info(cmr_handle handle, cmr_uint param) 
{
    cmr_int ret_value = SENSOR_SUCCESS;
    struct sensor_aec_reg_info *info = (struct sensor_aec_reg_info *)param;
    cmr_u16 exposure_line = 0x00;
    cmr_u16 dummy_line = 0x00;
    cmr_u16 mode = 0x00;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(info);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    SENSOR_LOGI("E");

    info->aec_i2c_info_out = &gc5035_arb_aec_info;
    exposure_line = info->exp.exposure;
    dummy_line = info->exp.dummy;
    mode = info->exp.size_index;

    gc5035_arb_drv_calc_exposure(handle, exposure_line, dummy_line, mode, &gc5035_arb_aec_info);
    gc5035_arb_drv_calc_gain(handle,info->gain, &gc5035_arb_aec_info);

    return ret_value;
}

static cmr_int gc5035_arb_drv_set_master_FrameSync(cmr_handle handle, cmr_uint param) 
{
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
	
	SENSOR_LOGI("E");

	/*TODO*/

	//hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3002, 0x40);
	
	/*END*/

    return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_arb_drv_stream_on(cmr_handle handle, cmr_uint param)
{
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    if (sns_drv_cxt->current_state_machine == SENSOR_STATE_STREAM_ON) {
        return 0;
    }	
	pthread_mutex_lock(&gc5035_sensor_mutex);
	SENSOR_LOGI("E");
	
#if defined(CONFIG_DUAL_MODULE)
	//gc5035_arb_drv_set_master_FrameSync(handle,param);
#endif   
	/*TODO*/
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3e, 0x91);	
    hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	/*END*/
	
	/*delay*/
	pthread_mutex_unlock(&gc5035_sensor_mutex);
	sns_drv_cxt->current_state_machine = SENSOR_STATE_STREAM_ON;
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_int gc5035_arb_drv_stream_off(cmr_handle handle, cmr_uint param)
{
	pthread_mutex_lock(&gc5035_sensor_mutex);
	SENSOR_LOGI("E");
	
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u16 delay_ms = (sns_drv_cxt->sensor_ev_info.preview_shutter *
                        sns_drv_cxt->line_time_def / 1000000);
    if (!sns_drv_cxt->is_sensor_close) {
        usleep(5 * 1000);
    }
   	/*TODO*/
    if (sns_drv_cxt->current_state_machine == SENSOR_STATE_STREAM_ON) {
   	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0x3e, 0x00);
	hw_sensor_write_reg(sns_drv_cxt->hw_handle, 0xfe, 0x00);
	/*END*/
	SENSOR_LOGI("stream_off delay_ms %d", delay_ms);
    usleep((delay_ms + 10) * 1000);
    sns_drv_cxt->current_state_machine = SENSOR_STATE_STREAM_OFF;
    }
    sns_drv_cxt->is_sensor_close = 0;
    SENSOR_LOGI("X");
	pthread_mutex_unlock(&gc5035_sensor_mutex);
    return SENSOR_SUCCESS;
}

static cmr_int gc5035_arb_drv_handle_create(struct sensor_ic_drv_init_para *init_param, cmr_handle* sns_ic_drv_handle) 
{
    cmr_int ret = SENSOR_SUCCESS;
    struct sensor_ic_drv_cxt * sns_drv_cxt = NULL;
    void *pri_data = NULL;

    ret = sensor_ic_drv_create(init_param,sns_ic_drv_handle);
    sns_drv_cxt = *sns_ic_drv_handle;

    sns_drv_cxt->sensor_ev_info.preview_shutter = PREVIEW_FRAME_LENGTH - FRAME_OFFSET;
    sns_drv_cxt->sensor_ev_info.preview_gain = SENSOR_BASE_GAIN;
    sns_drv_cxt->sensor_ev_info.preview_framelength = PREVIEW_FRAME_LENGTH;
    sns_drv_cxt->current_state_machine = SENSOR_STATE_IDLE;
    sns_drv_cxt->frame_length_def = PREVIEW_FRAME_LENGTH;
	sns_drv_cxt->exif_ptr = (void*)&s_gc5035_arb_exif_info;
	
	gc5035_arb_drv_write_frame_length(sns_drv_cxt, &gc5035_arb_aec_info, sns_drv_cxt->sensor_ev_info.preview_framelength);
	gc5035_arb_drv_write_gain(sns_drv_cxt, &gc5035_arb_aec_info, sns_drv_cxt->sensor_ev_info.preview_gain);
	gc5035_arb_drv_write_shutter(sns_drv_cxt, &gc5035_arb_aec_info, sns_drv_cxt->sensor_ev_info.preview_shutter);

    sensor_ic_set_match_module_info(sns_drv_cxt, ARRAY_SIZE(s_gc5035_arb_module_info_tab), s_gc5035_arb_module_info_tab);
    sensor_ic_set_match_resolution_info(sns_drv_cxt, ARRAY_SIZE(s_gc5035_arb_resolution_tab_raw), s_gc5035_arb_resolution_tab_raw);
    sensor_ic_set_match_trim_info(sns_drv_cxt, ARRAY_SIZE(s_gc5035_arb_resolution_trim_tab), s_gc5035_arb_resolution_trim_tab);
    sensor_ic_set_match_static_info(sns_drv_cxt, ARRAY_SIZE(s_gc5035_arb_static_info), s_gc5035_arb_static_info);
    sensor_ic_set_match_fps_info(sns_drv_cxt, ARRAY_SIZE(s_gc5035_arb_mode_fps_info), s_gc5035_arb_mode_fps_info);

    /*init exif info,this will be deleted in the future*/
    gc5035_arb_drv_init_fps_info(sns_drv_cxt);
	gc5035_arb_drv_init_exif_info(sns_drv_cxt);
    pthread_mutex_init(&gc5035_sensor_mutex, NULL);

    /*add private here*/
    return ret;
}

static cmr_int gc5035_arb_drv_handle_delete(cmr_handle handle, void *param) 
{
    cmr_int ret = SENSOR_SUCCESS;

    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt * sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

    pthread_mutex_destroy(&gc5035_sensor_mutex);
	sns_drv_cxt->current_state_machine = SENSOR_STATE_IDLE;
    ret = sensor_ic_drv_delete(handle,param);
    return ret;
}

static cmr_int gc5035_arb_drv_get_private_data(cmr_handle handle, cmr_uint cmd, void**param)
{
    cmr_int ret = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_IC_CHECK_PTR(param);

    ret = sensor_ic_get_private_data(handle,cmd, param);
    return ret;
}
void *sensor_ic_open_lib(void)
{
     return &g_gc5035_arb_mipi_raw_info;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = gc5035_arb_power_on,
 *============================================================================*/
static struct sensor_ic_ops s_gc5035_arb_ops_tab = {
    .create_handle = gc5035_arb_drv_handle_create,
    .delete_handle = gc5035_arb_drv_handle_delete,
    .get_data = gc5035_arb_drv_get_private_data,
/*---------------------------------------*/
	.power = gc5035_arb_drv_power_on,
	.identify = gc5035_arb_drv_identify,
	.ex_write_exp = gc5035_arb_drv_write_exposure,
	.write_gain_value = gc5035_arb_drv_write_gain_value,
	
#if defined(CONFIG_DUAL_MODULE)
	//.read_aec_info = gc5035_arb_drv_read_aec_info,
#endif

    .ext_ops = {
        [SENSOR_IOCTL_BEFORE_SNAPSHOT].ops = gc5035_arb_drv_before_snapshot,
        [SENSOR_IOCTL_STREAM_ON].ops = gc5035_arb_drv_stream_on,
        [SENSOR_IOCTL_STREAM_OFF].ops = gc5035_arb_drv_stream_off,
        /* expand interface,if you want to add your sub cmd ,
         *  you can add it in enum {@SENSOR_IOCTL_VAL_TYPE}
         */
        [SENSOR_IOCTL_ACCESS_VAL].ops = gc5035_arb_drv_access_val,
    }
};

