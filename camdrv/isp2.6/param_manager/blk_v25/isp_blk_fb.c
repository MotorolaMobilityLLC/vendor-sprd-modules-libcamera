/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "isp_blk_fb"
#include "isp_blocks_cfg.h"

cmr_s32 _pm_fb_init(void * dst_fb_param, void * src_fb_param,
			void * param1, void * param2)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_facebeauty_param_info *dst_ptr =
		(struct isp_facebeauty_param_info *)dst_fb_param;
	struct sensor_facebeauty_param *src_ptr =
		(struct sensor_facebeauty_param *)src_fb_param;
	struct isp_pm_block_header *header_ptr =
		(struct isp_pm_block_header *)param1;
	UNUSED(param2);
	cmr_s32 i = 0, j = 0;
	for(i = 0; i < ISP_PM_FB_SKINTONE_NUM; i++) {
		dst_ptr->cur.fb_param[i].blemishSizeThrCoeff =
			src_ptr->fb_param[i].blemishSizeThrCoeff;
		dst_ptr->cur.fb_param[i].lipColorType =
			src_ptr->fb_param[i].lipColorType;
		dst_ptr->cur.fb_param[i].removeBlemishFlag =
			src_ptr->fb_param[i].removeBlemishFlag;
		dst_ptr->cur.fb_param[i].skinColorType =
			src_ptr->fb_param[i].skinColorType;
		ISP_LOGV("i %d blemishSizeThrCoeff %d lipColorType %d "
			"removeBlemishFlag %d skinColorType %d", i,
			dst_ptr->cur.fb_param[i].blemishSizeThrCoeff,
			dst_ptr->cur.fb_param[i].lipColorType,
			dst_ptr->cur.fb_param[i].removeBlemishFlag,
			dst_ptr->cur.fb_param[i].skinColorType);
		dst_ptr->cur.fb_param[i].fb_layer.largeEyeDefaultLevel =
			src_ptr->fb_param[i].fb_layer.largeEyeDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.lipColorDefaultLevel =
			src_ptr->fb_param[i].fb_layer.lipColorDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.skinBrightDefaultLevel =
			src_ptr->fb_param[i].fb_layer.skinBrightDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.skinColorDefaultLevel =
			src_ptr->fb_param[i].fb_layer.skinColorDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.skinSmoothDefaultLevel =
			src_ptr->fb_param[i].fb_layer.skinSmoothDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.skinTextureHiFreqDefaultLevel =
			src_ptr->fb_param[i].fb_layer.skinTextureHiFreqDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.skinTextureLoFreqDefaultLevel =
			src_ptr->fb_param[i].fb_layer.skinTextureLoFreqDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.slimFaceDefaultLevel =
			src_ptr->fb_param[i].fb_layer.slimFaceDefaultLevel;
		dst_ptr->cur.fb_param[i].fb_layer.skinSmoothRadiusCoeffDefaultLevel =
			src_ptr->fb_param[i].fb_layer.skinSmoothRadiusCoeffDefaultLevel;
		ISP_LOGV("largeEyeDefaultLevel %d skinSmoothDefaultLevel %d "
			"skinSmoothRadiusCoeffDefaultLevel %d",
			dst_ptr->cur.fb_param[i].fb_layer.largeEyeDefaultLevel,
			dst_ptr->cur.fb_param[i].fb_layer.skinSmoothDefaultLevel,
			dst_ptr->cur.fb_param[i].fb_layer.skinSmoothRadiusCoeffDefaultLevel);
		for(j = 0; j < 11; j++) {
			dst_ptr->cur.fb_param[i].fb_layer.skinSmoothRadiusCoeff[j] =
				src_ptr->fb_param[i].fb_layer.skinSmoothRadiusCoeff[j];
			dst_ptr->cur.fb_param[i].fb_layer.largeEyeLevel[j] =
				src_ptr->fb_param[i].fb_layer.largeEyeLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.lipColorLevel[j] =
				src_ptr->fb_param[i].fb_layer.lipColorLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.skinBrightLevel[j] =
				src_ptr->fb_param[i].fb_layer.skinBrightLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.skinColorLevel[j] =
				src_ptr->fb_param[i].fb_layer.skinColorLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.skinSmoothLevel[j] =
				src_ptr->fb_param[i].fb_layer.skinSmoothLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.skinTextureHiFreqLevel[j] =
				src_ptr->fb_param[i].fb_layer.skinTextureHiFreqLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.skinTextureLoFreqLevel[j] =
				src_ptr->fb_param[i].fb_layer.skinTextureLoFreqLevel[j];
			dst_ptr->cur.fb_param[i].fb_layer.slimFaceLevel[j] =
				src_ptr->fb_param[i].fb_layer.slimFaceLevel[j];
			ISP_LOGV("i %d, j %d largeEyeLevel %d skinBrightLevel %d "
				"skinSmoothRadiusCoeff %d", i, j,
				dst_ptr->cur.fb_param[i].fb_layer.largeEyeLevel[j],
				dst_ptr->cur.fb_param[i].fb_layer.skinBrightLevel[j],
				dst_ptr->cur.fb_param[i].fb_layer.skinSmoothRadiusCoeff[j]);
			}
		}
	header_ptr->is_update = ISP_ONE;
	return rtn;
}

cmr_s32 _pm_fb_set_param(void * fb_param, cmr_u32 cmd, void * param_ptr0,
	void * param_ptr1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	UNUSED(fb_param);
	UNUSED(cmd);
	UNUSED(param_ptr0);
	UNUSED(param_ptr1);
	return rtn;
}

cmr_s32 _pm_fb_get_param(void *fb_param, cmr_u32 cmd, void *rtn_param0,
	void *rtn_param1)
{
	cmr_s32 rtn = ISP_SUCCESS;
	struct isp_facebeauty_param_info *fb_ptr =
		(struct isp_facebeauty_param_info *)fb_param;
	struct isp_pm_param_data *param_data_ptr =
		(struct isp_pm_param_data *)rtn_param0;
	cmr_u32 *update_flag = (cmr_u32 *) rtn_param1;

	param_data_ptr->id = ISP_BLK_FB;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &fb_ptr->cur;
		param_data_ptr->data_size = sizeof(fb_ptr->cur);
		*update_flag = ISP_ZERO;
		break;

	default:
		break;
	}

	return rtn;
}
