/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _PDAF_CTRL_H_
#define _PDAF_CTRL_H_

#include <sys/types.h>
#include "pdaf_sprd_adpt.h"
#include "isp_adpt.h"
#include "sensor_raw.h"  //because third_lib_info


#define MAX_MULTIZONE_NUM 45
#define AREA_LOOP 4		//PASSIVE and ACTIVE mode default

enum pdaf_ctrl_PDAF_TYPE {
	PDAF_PASSIVE = 0,	//caf type
	PDAF_ACTIVE, 	//touch type
	PDAF_MULTIZONE,
};


enum pdaf_ctrl_data_type {
	PDAF_DATA_TYPE_RAW = 0,
	PDAF_DATA_TYPE_LEFT,
	PDAF_DATA_TYPE_RIGHT,
	PDAF_DATA_TYPE_OUT,
	PDAF_DATA_TYPE_MAX
};

enum pdaf_mode {
	PDAF_MODE_NORMAL = 0x00,
	PDAF_MODE_MACRO,
	PDAF_MODE_CONTINUE,
	PDAF_MODE_VIDEO,
	PDAF_MODE_MANUAL,
	PDAF_MODE_PICTURE,
	PDAF_MODE_FULLSCAN,
	PDAF_MODE_MAX
};

enum pdaf_ctrl_cmd_type {
	PDAF_CTRL_CMD_SET_OPEN,
	PDAF_CTRL_CMD_SET_CLOSE,
	PDAF_CTRL_CMD_SET_COOR,
	PDAF_CTRL_CMD_SET_MODE,
	PDAF_CTRL_CMD_SET_AFMFV,
	PDAF_CTRL_CMD_SET_MULTIZONE,
	PDAF_CTRL_CMD_SET_OTSWITCH,
	PDAF_CTRL_CMD_SET_OTINFO,
	/*
	 * warning if you wanna set ioctrl directly
	 * please add msg id below here
	 * */
	PDAF_CTRL_CMD_DIRECT_BEGIN,
	PDAF_CTRL_CMD_GET_BUSY,
	PDAF_CTRL_CMD_SET_CONFIG,
	PDAF_CTRL_CMD_SET_PARAM,
	PDAF_CTRL_CMD_DISABLE_PDAF,
	PDAF_CTRL_CMD_DIRECT_END,
};

enum pdaf_cb_cmd {
	PDAF_CB_CMD_AF_SET_PD_INFO,
	PDAF_CB_CMD_SET_CFG_PARAM,
	PDAF_CB_CMD_BLOCK_CFG,
	PDAF_CB_CMD_SET_BYPASS,
	PDAF_CB_CMD_SET_WORK_MODE,
	PDAF_CB_CMD_SET_SKIP_NUM,
	PDAF_CB_CMD_SET_PPI_INFO,
	PDAF_CB_CMD_SET_ROI,
	PDAF_CB_CMD_SET_EXTRACTOR_BYPASS
};


enum pdaf_ctrl_lib_product_id {
	ALTEC_PDAF_LIB,
	SPRD_PDAF_LIB,
	SFT_PDAF_LIB,
	ALC_PDAF_LIB,
};

enum pdaf_ctrl_data_bit {
	PD_DATA_BIT_8 = 8,
	PD_DATA_BIT_10 = 10,
	PD_DATA_BIT_12 = 12,
};

struct pdaf_ctrl_otp_info_t {
	void *otp_data;
	cmr_int size;
};

struct pdaf_ctrl_process_in {
	cmr_uint u_addr;
	cmr_uint u_addr_right;
	cmr_uint datasize;
};

struct pdaf_ctrl_process_out {
	struct sprd_pdaf_report_t pd_report_data;
};

struct pdaf_ctrl_callback_in {
	union {
		struct pdaf_ctrl_process_out proc_out;
	};
};

struct pdaf_ctrl_cb_ops_type {
	cmr_int(*call_back) (cmr_handle caller_handle, struct pdaf_ctrl_callback_in * in);
};

struct pdaf_ctrl_win_rect {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct pdaf_ctrl_PD_Multi_zone_param {
	cmr_u16 Center_X;
	cmr_u16 Center_Y;
	cmr_u16 sWidth;
	cmr_u16 sHeight;
};

struct pdaf_ctrl_SetPD_ROI_param {
	cmr_u16 ROI_Size;
	struct pdaf_ctrl_PD_Multi_zone_param ROI_info[MAX_MULTIZONE_NUM];
};

struct pdaf_ctrl_ot_info {
	cmr_u32 centorX;	//Center X Coordinate
	cmr_u32 centorY;	//Center Y Coordinate
	cmr_u32 sizeX;	//Object Size Width
	cmr_u32 sizeY;	//Object Size Height
	cmr_u32 axis1;	//Object Axis Width
	cmr_u32 axis2;	//Object Axis Height
	cmr_u32 otdiff;
	cmr_u32 otstatus;
	cmr_u32 otframeid;
	cmr_u32 imageW; //pdaf plane width
	cmr_u32 imageH; //pdaf plane height
	cmr_u32 reserved[20];
};

struct pdaf_ctrl_param_in {
	union {
		struct isp3a_pd_config_t *pd_config;
		struct pdaf_ctrl_win_rect touch_area;
		cmr_u32 af_mode;
		struct pdaf_ctrl_SetPD_ROI_param af_roi;
		cmr_u32 ot_switch;
		struct pdaf_ctrl_ot_info ot_info;
	};
	void *af_addr;// afm statis buffer
	cmr_u32 af_addr_len;// in bytes
};

struct pdaf_ctrl_param_out {
	union {
		cmr_u8 is_busy;
	};
};

#define ISP_PRODUCT_NAME_LEN				20

struct isp_lib_config {
	cmr_u32 product_id;			//lib owner
	cmr_u32 version_id;			//lib version
	cmr_s8 product_name_low[ISP_PRODUCT_NAME_LEN];
	cmr_s8 product_name_high[ISP_PRODUCT_NAME_LEN];
};

struct pdaf_ctrl_pd_result{
	cmr_s32 pdConf[MAX_MULTIZONE_NUM + 1];
	double pdPhaseDiff[MAX_MULTIZONE_NUM + 1];
	cmr_s32 pdGetFrameID;
	cmr_s32 pdDCCGain[MAX_MULTIZONE_NUM + 1];	// be sure MAX_MULTIZONE_NUM bigger than AREA_LOOP
	cmr_u32 pd_roi_num;
	cmr_u32 af_type;	// notify to AF that PDAF is in passive mode or active mode
};

typedef cmr_int(*pdaf_ctrl_cb) (cmr_handle handle, cmr_int type, void *param0, void *param1);

struct pdaf_ctrl_init_in {
	cmr_u32 camera_id;
	void *caller;
	cmr_handle caller_handle;
	cmr_u8 pdaf_support;
	cmr_s8 *name;
	struct isp_size sensor_max_size;
	struct isp_lib_config pdaf_lib_info;
	struct pdaf_ctrl_otp_info_t af_otp;
	struct pdaf_ctrl_otp_info_t pdaf_otp;
	struct sensor_pdaf_info *pd_info;
	struct pdaf_ctrl_cb_ops_type pdaf_ctrl_cb_ops;
	pdaf_ctrl_cb pdaf_set_cb;
	struct third_lib_info lib_param;
	cmr_handle handle_pm;
	cmr_u32(*pdaf_set_pdinfo_to_af) (void *handle, struct pdaf_ctrl_pd_result * in_parm);
	cmr_u32(*pdaf_set_work_mode) (void *handle, cmr_u32 in_parm);
	cmr_u32(*pdaf_set_skip_num) (void *handle, cmr_u32 in_parm);
	cmr_u32(*pdaf_set_roi) (void *handle, struct pd_roi_info * in_parm);
	cmr_u32(*pdaf_set_extractor_bypass) (void *handle, cmr_u32 in_parm);
	struct sensor_otp_cust_info *otp_info_ptr;
	cmr_u8 is_master;
};

struct pdaf_ctrl_init_out {
	union {
		cmr_u8 enable;
	};
};

struct pdafctrl_work_lib {
	cmr_handle lib_handle;
	struct adpt_ops_type *adpt_ops;
};

cmr_int pdaf_ctrl_init(struct pdaf_ctrl_init_in *in, struct pdaf_ctrl_init_out *out, cmr_handle * handle);

cmr_int pdaf_ctrl_deinit(cmr_handle * handle);

cmr_int pdaf_ctrl_process(cmr_handle handle, struct pdaf_ctrl_process_in *in, struct pdaf_ctrl_process_out *out);

cmr_int pdaf_ctrl_ioctrl(cmr_handle handle, cmr_int cmd, struct pdaf_ctrl_param_in *in, struct pdaf_ctrl_param_out *out);
#endif
