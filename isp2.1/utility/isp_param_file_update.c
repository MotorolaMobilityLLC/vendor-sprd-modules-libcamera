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
/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "isp_type.h"

#include "isp_param_file_update.h"

/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

#define LNC_MAP_NUM 9
#define SCENE_INFO_NUM 10
#define ISP_PARAM_VERSION_V21 0x0005

static int32_t count = 8;
extern isp_s8 nr_mode_name[MAX_MODE_NUM][8];

int update_param_v1(struct sensor_raw_info* sensor_raw_info_ptr, const char *sensor_name)
{
	int rtn = 0x00;
	UNUSED(sensor_raw_info_ptr);
	UNUSED(sensor_name);
	return rtn;
}

int read_nr_level_number_info(FILE *fp, uint8_t *data_ptr)
{
	int rtn =0x00;

	char line_buff[256] = {0};
	int i = 0;
	uint8_t *param_buf = data_ptr;

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;
		if (fgets(line_buff, 256, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "}};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint8_t)c[i];
		}
	}
	return rtn;
}

int read_nr_scene_map_info(FILE *fp, uint32_t *data_ptr)
{
	int rtn = 0x00;

	int i = 0;
	char line_buf[256] = {0};
	uint32_t *param_buf = data_ptr;
	while (!feof(fp)) {
		uint32_t c1[16];
		int n = 0;

		if (fgets(line_buf, 256, fp) == NULL) {
			break;
		}

		if (strstr(line_buf, "{") != NULL) {
			continue;
		}

		if (strstr(line_buf, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buf, "}};") != NULL) {
			break;
		}

		n = sscanf(line_buf, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c1[0], &c1[1], &c1[2], &c1[3], &c1[4], &c1[5], &c1[6], &c1[7], &c1[8], &c1[9], &c1[10], &c1[11], &c1[12], &c1[13], &c1[14], &c1[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = c1[i];
		}
	}

	return rtn;
}

int read_tune_info(FILE *fp,uint8_t *data_ptr,uint32_t *data_len)
{
	int rtn =0x00;

	uint8_t *param_buf = data_ptr;
	char line_buff[256];
	int i;

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;
		if (fgets(line_buff, 256, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint8_t)c[i];
		}
	}
	*data_len = (long)param_buf - (long)(data_ptr);
	return rtn;
}


int read_ae_table_32bit(FILE *fp,uint32_t *data_ptr,uint32_t *data_len)
{
	int rtn = 0x00;

	int i;
	uint32_t *param_buf = data_ptr;
	char line_buf[512];

	while (!feof(fp)) {
		uint32_t c1[16];
		int n = 0;

		if (fgets(line_buf, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buf, "{") != NULL) {
			continue;
		}

		if (strstr(line_buf, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buf, "},") != NULL) {
			break;
		}

		n = sscanf(line_buf, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c1[0], &c1[1], &c1[2], &c1[3], &c1[4], &c1[5], &c1[6], &c1[7], &c1[8], &c1[9], &c1[10], &c1[11], &c1[12], &c1[13], &c1[14], &c1[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = c1[i];
		}
	}

	*data_len = (long)param_buf - (long)data_ptr;

	return rtn;
}



int read_ae_table_16bit(FILE *fp,uint16_t *data_ptr,uint32_t *data_len)
{
	int rtn = 0x00;

	int i;
	uint16_t *param_buf = data_ptr;
	char line_buf[512];

	while (!feof(fp)) {
		uint32_t c2[16];
		int n = 0;

		if (fgets(line_buf, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buf, "{") != NULL) {
			continue;
		}

		if (strstr(line_buf, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buf, "},") != NULL) {
			break;
		}

		n = sscanf(line_buf, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c2[0], &c2[1], &c2[2], &c2[3], &c2[4], &c2[5], &c2[6], &c2[7], &c2[8], &c2[9], &c2[10], &c2[11], &c2[12], &c2[13], &c2[14], &c2[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = (uint16_t)c2[i];
		}
	}

	*data_len = (long)param_buf - (long)data_ptr;

	return rtn;
}

int read_ae_weight(FILE *fp,struct ae_weight_tab *weight_ptr)
{
	int rtn = 0x00;

	uint8_t *param_buf = weight_ptr->weight_table;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}
		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}
		if (strstr(line_buff, "},") != NULL) {
			break;
		}
		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint8_t)c[i];
		}
	}
	weight_ptr->len = (long)param_buf - (long)(weight_ptr->weight_table);

	return 0;
}

int read_ae_scene_info(FILE *fp,struct sensor_ae_tab *ae_ptr,int scene_mode)
{
	int rtn = 0x00;

	uint32_t *param_buf = ae_ptr->scene_tab[scene_mode][0].scene_info;
	uint32_t *param_buf1 = ae_ptr->scene_tab[scene_mode][1].scene_info;
	int i = 0;
	char line_buff[512] = {0};

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}
		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}
		if (strstr(line_buff, "},") != NULL) {
			break;
		}
		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
			*param_buf1 ++ = c[i];
		}
	}
	ae_ptr->scene_tab[scene_mode][0].scene_info_len = (long)param_buf - (long)(ae_ptr->scene_tab[scene_mode][0].scene_info);
	ae_ptr->scene_tab[scene_mode][1].scene_info_len = (long)param_buf - (long)(ae_ptr->scene_tab[scene_mode][1].scene_info);

	return rtn;
}

int read_ae_auto_iso(FILE *fp,struct ae_auto_iso_tab_v1 *auto_iso_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = auto_iso_ptr->auto_iso_tab;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "},") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint16_t)c[i];
		}
	}

	auto_iso_ptr->len = (long)param_buf - (long)(auto_iso_ptr->auto_iso_tab);

	return 0;
}
int read_fix_ae_info(FILE *fp,struct sensor_ae_tab *ae_ptr)
{
	int rtn = 0x00;

	int flag_end = 0;
	int i,j;
	char *ae_tab_info[5] = {NULL};
	char *ae_scene_info[6] = {NULL};
	char ae_auto_iso_info[50];
	char ae_weight_info[50];

	char *line_buf = (char *)malloc(512*sizeof(char));
	if ( NULL == line_buf) {
		ISP_LOGE("malloc mem was error!");
		rtn = 0x01;
		return rtn;
	}

	for(i = 0;i < 5;i++) {
		ae_tab_info[i] = (char *)malloc(50*sizeof(char));
		if ( NULL == ae_tab_info[i]) {
			ISP_LOGE("malloc mem was error!");
			rtn = 0x01;
			goto exit;
		}
	}

	for (i = 0;i < 6;i++) {
		ae_scene_info[i] = (char *)malloc(50*sizeof(char));
		if ( NULL == ae_scene_info[i]) {
			ISP_LOGE("malloc mem was error!");
			rtn = 0x01;
			goto exit;
		}
	}
	while (!feof(fp)) {
		if (NULL == fgets(line_buf,512,fp)) {
			break;
		}
		if (strstr(line_buf,"_common_ae_tab_") != NULL ) {
			for (i = 0;i < AE_FLICKER_NUM;i++) {
				int break_flag = 0;
				for (j = 0;j < AE_ISO_NUM_NEW;j++) {
					sprintf(ae_tab_info[0],"_common_ae_tab_index_%d%d",i,j);
					sprintf(ae_tab_info[1],"_common_ae_tab_exposure_%d%d",i,j);
					sprintf(ae_tab_info[2],"_common_ae_tab_dummy_%d%d",i,j);
					sprintf(ae_tab_info[3],"_common_ae_tab_again_%d%d",i,j);
					sprintf(ae_tab_info[4],"_common_ae_tab_dgain_%d%d",i,j);

					if (strstr(line_buf,ae_tab_info[0]) != NULL) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_tab[i][j].index,&ae_ptr->ae_tab[i][j].index_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[1]) != NULL) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_tab[i][j].exposure,&ae_ptr->ae_tab[i][j].exposure_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[2]) != NULL) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_tab[i][j].dummy,&ae_ptr->ae_tab[i][j].dummy_len);
						if(0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[3]) != NULL) {
						rtn = read_ae_table_16bit(fp,ae_ptr->ae_tab[i][j].again,&ae_ptr->ae_tab[i][j].again_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[4]) != NULL) {
						rtn = read_ae_table_16bit(fp,ae_ptr->ae_tab[i][j].dgain,&ae_ptr->ae_tab[i][j].dgain_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
				}
				if (0 != break_flag)
					break;
			}
		}

		if (strstr(line_buf,"_ae_flash_tab_") != NULL ) {
			for (i = 0;i < AE_FLICKER_NUM;i++) {
				int break_flag = 0;
				for (j = 0;j < AE_ISO_NUM_NEW;j++) {
					sprintf(ae_tab_info[0],"_ae_flash_tab_index_%d%d",i,j);
					sprintf(ae_tab_info[1],"_ae_flash_tab_exposure_%d%d",i,j);
					sprintf(ae_tab_info[2],"_ae_flash_tab_dummy_%d%d",i,j);
					sprintf(ae_tab_info[3],"_ae_flash_tab_again_%d%d",i,j);
					sprintf(ae_tab_info[4],"_ae_flash_tab_dgain_%d%d",i,j);

					if (strstr(line_buf,ae_tab_info[0]) != NULL) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_flash_tab[i][j].index,&ae_ptr->ae_flash_tab[i][j].index_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[1]) != NULL) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_flash_tab[i][j].exposure,&ae_ptr->ae_flash_tab[i][j].exposure_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[2]) != NULL) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_flash_tab[i][j].dummy,&ae_ptr->ae_flash_tab[i][j].dummy_len);
						if(0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[3]) != NULL) {
						rtn = read_ae_table_16bit(fp,ae_ptr->ae_flash_tab[i][j].again,&ae_ptr->ae_flash_tab[i][j].again_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if (strstr(line_buf,ae_tab_info[4]) != NULL) {
						rtn = read_ae_table_16bit(fp,ae_ptr->ae_flash_tab[i][j].dgain,&ae_ptr->ae_flash_tab[i][j].dgain_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
				}
				if (0 != break_flag)
					break;
			}
		}
		if (strstr(line_buf,"_ae_weight_") != NULL ) {
			for (i = 0;i < AE_WEIGHT_TABLE_NUM;i++) {
				sprintf(ae_weight_info,"_ae_weight_%d",i);
				if (strstr(line_buf,ae_weight_info) != NULL) {

					rtn = read_ae_weight(fp,&ae_ptr->weight_tab[i]);
					break;
				}
			}
		}
		if ((strstr(line_buf,"_scene_") != NULL) && (strstr(line_buf,"_ae_") != NULL) ) {
			for (i = 0;i < AE_SCENE_NUM;i++) {
				int break_flag = 0;
				sprintf(ae_scene_info[0],"_ae_scene_info_%d",i);
				if (strstr(line_buf,ae_scene_info[0]) != NULL) {

					rtn = read_ae_scene_info(fp,ae_ptr,i);
					if (rtn != 0x00){
						rtn = 0x01;
						goto exit;
					}
					break;
				}
				for (j = 0;j < AE_FLICKER_NUM;j++) {
					sprintf(ae_scene_info[1],"_scene_ae_tab_index_%d%d",i,j);
					sprintf(ae_scene_info[2],"_scene_ae_tab_exposure_%d%d",i,j);
					sprintf(ae_scene_info[3],"_scene_ae_tab_dummy_%d%d",i,j);
					sprintf(ae_scene_info[4],"_scene_ae_tab_again_%d%d",i,j);
					sprintf(ae_scene_info[5],"_scene_ae_tab_dgain_%d%d",i,j);
					if (strstr(line_buf,ae_scene_info[1]) != NULL) {

						rtn = read_ae_table_32bit(fp,ae_ptr->scene_tab[i][j].index,&ae_ptr->scene_tab[i][j].index_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if (strstr(line_buf,ae_scene_info[2]) != NULL) {

						rtn = read_ae_table_32bit(fp,ae_ptr->scene_tab[i][j].exposure,&ae_ptr->scene_tab[i][j].exposure_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if (strstr(line_buf,ae_scene_info[3]) != NULL) {

						rtn = read_ae_table_32bit(fp,ae_ptr->scene_tab[i][j].dummy,&ae_ptr->scene_tab[i][j].dummy_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if (strstr(line_buf,ae_scene_info[4]) != NULL) {

						rtn = read_ae_table_16bit(fp,ae_ptr->scene_tab[i][j].again,&ae_ptr->scene_tab[i][j].again_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if (strstr(line_buf,ae_scene_info[5]) != NULL) {

						rtn = read_ae_table_16bit(fp,ae_ptr->scene_tab[i][j].dgain,&ae_ptr->scene_tab[i][j].dgain_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
				}
				if (0 != break_flag)
					break;
			}
		}
		if (strstr(line_buf,"_ae_auto_iso_") != NULL ) {
			for (i = 0;i < AE_FLICKER_NUM;i++) {
				sprintf(ae_auto_iso_info,"_ae_auto_iso_%d",i);
				if (strstr(line_buf,ae_auto_iso_info) != NULL) {

					rtn = read_ae_auto_iso(fp,&ae_ptr->auto_iso_tab[i]);
					if (0x00 != rtn) {
						goto exit;
					}
					if (1 == i)
						flag_end = 1;
					break;
				}
			}
		}
		if (0 != flag_end)
			break;
	}

exit:
	if (NULL != line_buf){
		free(line_buf);
	}
	for (i = 0; i < 5; i++) {
		if (NULL != ae_tab_info[i]) {
			free(ae_tab_info[i]);
		}
	}
	for (i = 0; i < 6; i++) {
		if (NULL != ae_scene_info[i]) {
			free(ae_scene_info[i]);
		}
	}
	return rtn;
}

int read_awb_win_map(FILE *fp,struct sensor_awb_map *awb_map_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = awb_map_ptr->addr;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "},") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i = 0; i < n; i++) {
			*param_buf ++ = (uint16_t)c[i];
		}
	}

	awb_map_ptr->len = (long)param_buf - (long)(awb_map_ptr->addr);

	return 0;
}

int read_awb_pos_weight(FILE *fp,struct sensor_awb_weight *awb_weight_ptr)
{
	int rtn = 0x00;

	uint8_t *param_buf = awb_weight_ptr->addr;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}
//		ISP_LOGI("read_awb_width line_buff = %s", line_buff);

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "},") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint8_t)c[i];
		}
	}

	awb_weight_ptr->weight_len = (long)param_buf - (long)(awb_weight_ptr->addr);

	return rtn;
}

int	read_awb_width_height(FILE *fp,struct sensor_awb_weight *awb_weight_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = awb_weight_ptr->size;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "},") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint16_t)c[i];
		}
	}

	awb_weight_ptr->size_param_len = (long)param_buf - (long)(awb_weight_ptr->size);

	return 0;
}

int read_fix_awb_info(FILE *fp,struct sensor_awb_map_weight_param *awb_ptr)
{
	int rtn = 0x00;

	int flag_end = 0;
	char line_buf[512];

	while (!feof(fp)) {
		if (NULL == fgets(line_buf,512,fp)) {
			break;
		}
		if (strstr(line_buf,"_awb_win_map") != NULL) {
			rtn = read_awb_win_map(fp,&awb_ptr->awb_map);
			if (0x00 != rtn) {
				return rtn;
			}
		}
		if ((strstr(line_buf,"_awb_pos_weight") != NULL) && (strstr(line_buf, "height") != NULL)) {
			rtn = read_awb_pos_weight(fp,&awb_ptr->awb_weight);
			if (0x00 != rtn) {
				return rtn;
			}
		}
		if (strstr(line_buf,"_awb_pos_weight_width_height") != NULL) {
			rtn = read_awb_width_height(fp,&awb_ptr->awb_weight);
			if (0x00 != rtn) {
				return rtn;
			}
			flag_end = 1;
		}
		if (0 != flag_end)
			break;
	}

	return rtn;
}

int read_lnc_map_info(FILE *fp,struct sensor_lens_map *lnc_map_ptr)
{
	int rtn = 0x00;

	uint32_t *param_buf = lnc_map_ptr->map_info;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL)
		{
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "},") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	lnc_map_ptr->map_info_len = (long)param_buf - (long)(lnc_map_ptr->map_info);

	return rtn;
}

int read_lnc_weight_info(FILE *fp,struct sensor_lens_map *lnc_map_ptr)
{
	int rtn = 0x00;
	int i;
	char line_buff[512];
	uint16_t *param_buf = PNULL;
	param_buf = lnc_map_ptr->weight_info;
	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL)
		{
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "},") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint16_t)c[i];
		}
	}

	lnc_map_ptr->weight_info_len= (long)param_buf - (long)(lnc_map_ptr->weight_info);

	return rtn;
}

 int find_digit(char *dst, const char *src, int out_buff_size)
 {
	int num = 0, i = 0, cur = 0;
	char *dst_ptr = dst;
	num = strlen(src);
	for(i = 0; i < num; i++) {
		//if(isdigit(src[i])) {
		if( src[i] >= '0' && src[i] <= '9') {
			*dst_ptr++ = src[i];
			cur++;
		}
		if(cur == out_buff_size) {
			break;
		}
	}
 	*dst_ptr = '\0';
	if(cur == 0)
		return -1;
 	return cur;
}

int read_lnc_tab_size_offset_info(FILE *fp,struct sensor_lsc_map *lsc_ptr)
{
	int rtn = 0x00;

	uint32_t map_tab_offset = 0;
	int i = 0;
	char line_buff[512];
	char digit_buff[8];
	char lsc_2d_macro[64] = {0};
	int lnc_flag_len = 0;
	uint32_t c = 0;

	struct sensor_lens_map  *lnc_map_ptr = PNULL;
	uint8_t *lnc_start_addr = PNULL;
	lnc_map_ptr = (struct sensor_lens_map *)&(lsc_ptr->map[0]);
	lnc_start_addr = (uint8_t *)lsc_ptr->map[0].lnc_addr;
	for (i = 0; i < LNC_MAP_COUNT; i++) {
		lnc_flag_len = sprintf(lsc_2d_macro,"#define LSC_2D_MAP_%d",i);
		while(1) {
			if(feof(fp)) {
				return -1;
			}
			if (fgets(line_buff, 512, fp) == NULL)
			{
				return -1;
			}

			if (strstr(line_buff, "/*") != NULL) {
				continue;
			}

			if (strstr(line_buff, "#undef") != NULL) {
				continue;
			}

			if (strstr(line_buff, lsc_2d_macro) != NULL) {
				if(find_digit(digit_buff, &line_buff[lnc_flag_len], sizeof(digit_buff)) < 0) {
					return -1;
				}
				sscanf(digit_buff, "%d", &c);

				lnc_map_ptr[i].lnc_addr  = (uint16_t *)(lnc_start_addr + map_tab_offset);
				//lnc_map_ptr[i].lnc_len = c;
				*(lnc_map_ptr[i].lnc_map_tab_len) = c;
				map_tab_offset += c;
				*(lnc_map_ptr[i].lnc_map_tab_offset)  = map_tab_offset;
				#if 0
				ISP_LOGV("offset calc:%d,%d,%d,%d,%p,%p\n",i,map_tab_offset,
					*(lnc_map_ptr[i].lnc_map_tab_len),*(lnc_map_ptr[i].lnc_map_tab_offset),
					lnc_map_ptr[i].lnc_addr,lnc_map_ptr[i].lnc_len);
				#endif
				break;
			}
		}
	}

	return rtn;
}

int read_lnc_info(FILE *fp,struct sensor_lens_map *lnc_map_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = lnc_map_ptr->lnc_addr;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			if (strstr(line_buff, "_lnc") != NULL) {
				break;
			} else {
				continue;
			}
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = (uint16_t)c[i];
		}
	}

	lnc_map_ptr->lnc_len = (long)param_buf - (long)(lnc_map_ptr->lnc_addr);
	return rtn;
}
int read_fix_lnc_info(FILE *fp,struct sensor_lsc_map *lnc_ptr)
{
	int rtn = 0x00;

	int i;
	int flag_end = 0;

	char lnc_map_info[50];
	char lnc_weight_info[50];
	char lnc_info[20];

	char *line_buf = (char *)malloc(512*sizeof(char));
	if (NULL == line_buf) {
		ISP_LOGE("malloc mem was error!");
		rtn = 0x01;
		return rtn;
	}
	while (!feof(fp)) {
		if (NULL == fgets(line_buf,512,fp)) {
			break;
		}
		if (NULL != strstr(line_buf,"_lnc_map_info")) {
			for(i = 0;i < LNC_MAP_NUM;i++) {
				sprintf(lnc_map_info,"_lnc_map_info_0%d",i);
				if (strstr(line_buf,lnc_map_info) != NULL) {
					rtn = read_lnc_map_info(fp,&lnc_ptr->map[i]);
					if (0x00 != rtn) {
						return rtn;
					}
					break;
				}
			}
		}
		if (NULL != strstr(line_buf,"_lnc_weight_0")) {
			for(i = 0;i < LNC_MAP_NUM;i++) {
				sprintf(lnc_weight_info,"_lnc_weight_0%d",i);
				if (strstr(line_buf,lnc_weight_info) != NULL) {
					rtn = read_lnc_weight_info(fp,&lnc_ptr->map[i]);
					if (0x00 != rtn) {
						return rtn;
					}
					break;
				}
			}
		}
		if (NULL != strstr(line_buf,"_lnc_0")) {
			for (i = 0;i < LNC_MAP_NUM;i++) {
				sprintf(lnc_info,"_lnc_0%d",i);
				rtn = read_lnc_info(fp,&lnc_ptr->map[i]);
				if (0x00 != rtn) {
					return rtn;
				}
			}
			break;
		}
	}

	if (NULL != line_buf) {
		free(line_buf);
		line_buf = NULL;
	}
	return rtn;
}

int read_lnc_map(FILE *fp,struct sensor_lens_map *lnc_map_ptr)
{
	int rtn = 0x00;

	uint32_t *param_buf = lnc_map_ptr->map_info;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL)
		{
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	lnc_map_ptr->map_info_len = (long)param_buf - (long)(lnc_map_ptr->map_info);

	return rtn;
}

int read_libuse_info(FILE *fp,struct sensor_raw_info *sensor_raw_ptr)
{
	int rtn = 0x00;

	uint32_t *param_buf = (uint32_t *)sensor_raw_ptr->libuse_info;
	int i;
	char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i = 0; i < n; i++) {
			*param_buf ++ = c[i];
		}
	}
	return 0;
}

int update_param_v21(struct sensor_raw_info *sensor_raw_ptr,const char *sensor_name)
{
	int rtn = 0x00;

	int i,j = 0;

	char tune_info[128];
	char note_name[128];
	char libuse_info[128];
	char ae_tab[128];
	char awb_tab[128];
	char lsc_tab[128];
	char lsc_2d_map_flag1[64];
	char lsc_2d_map_flag2[64];
	char nr_level_number[256];
	char nr_default_level[256];
	char nr_scene_map[256];
	struct sensor_nr_level_map_param *nr_level_number_ptr = PNULL;
	struct sensor_nr_level_map_param *nr_default_level_ptr = PNULL;
	struct sensor_nr_scene_map_param *nr_map_ptr = PNULL;

	char *line_buf = (char *)malloc(512*sizeof(char));
	char *filename[14] = {PNULL};
	FILE *fp = PNULL;

	if (NULL == line_buf) {
		ISP_LOGI("malloc mem was error!");
		rtn = 0x01;
		return rtn;
	}

	for (i = 0;i < 14;i++) {
		filename[i] = (char *)malloc(128*sizeof(char));
		if (NULL == filename[i]) {
			ISP_LOGI("malloc mem was error!");
			rtn = 0x01;
			goto exit;
		}
	}

	sprintf(filename[0],"%s%s",CAMERA_DUMP_PATH, "isp_nr.h");
	sprintf(nr_level_number,"static struct sensor_nr_level_map_param s_%s_nr_level_number_map_param",sensor_name);
	sprintf(nr_default_level,"static struct sensor_nr_level_map_param s_%s_default_nr_level_map_param",sensor_name);
	sprintf(nr_scene_map,"static struct sensor_nr_scene_map_param s_%s_nr_scene_map_param",sensor_name);
	sprintf(lsc_2d_map_flag1,"#undef LSC_2D_MAP_0");
	sprintf(lsc_2d_map_flag2,"#undef LSC_2D_MAP_0_OFFSET");

	nr_level_number_ptr = sensor_raw_ptr->nr_fix.nr_level_number_ptr;
	nr_default_level_ptr = sensor_raw_ptr->nr_fix.nr_default_level_ptr;
	nr_map_ptr = sensor_raw_ptr->nr_fix.nr_scene_ptr;
	fp = fopen(filename[0],"r");
	if (NULL == fp) {
		ISP_LOGI("The param file %s is not exit!\n",filename[0]);
	}

	while (!feof(fp))
	{
		if (fgets(line_buf, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buf,nr_level_number) != NULL) {
			if(nr_level_number_ptr != NULL) {
				rtn = read_nr_level_number_info(fp,&(nr_level_number_ptr->nr_level_map[0]));
				if (0x00 != rtn) {
					ISP_LOGE("nr_level_number_info was error!");
					fclose(fp);
					goto exit;
				}
			}
			continue;
		}
		if (strstr(line_buf,nr_default_level) != NULL) {
			if(nr_default_level_ptr != NULL) {
				rtn = read_nr_level_number_info(fp,&(nr_default_level_ptr->nr_level_map[0]));
				if (0x00 != rtn) {
					ISP_LOGE("nr_default_level_info was error!");
					fclose(fp);
					goto exit;
				}
			}
			continue;
		}
		if (strstr(line_buf,nr_scene_map) != NULL) {
			if(nr_default_level_ptr != NULL) {
				rtn = read_nr_scene_map_info(fp,&(nr_map_ptr->nr_scene_map[0]));
				if (0x00 != rtn) {
					ISP_LOGE("nr_scene_map_info was error!");
					fclose(fp);
					goto exit;
				}
			}
			break;
		}
	}
	fclose(fp);

	sprintf(libuse_info,"static uint32_t s_%s_libuse_info",sensor_name);
	for (i = 0;i < 14;i++) {
		sprintf(filename[i],"%ssensor_%s_raw_param_%s.c",CAMERA_DUMP_PATH, sensor_name,&nr_mode_name[i][0]);
		sprintf(tune_info,"static uint8_t s_%s_tune_info_%s",sensor_name, &nr_mode_name[i][0]);
		sprintf(note_name,"static uint8_t s_%s_%s_tool_ui_input",sensor_name, &nr_mode_name[i][0]);
		sprintf(ae_tab,"static struct ae_table_param_2 s_%s_%s_ae_table_param",sensor_name, &nr_mode_name[i][0]);
		sprintf(awb_tab,"static struct sensor_awb_table_param s_%s_%s_awb_table_param",sensor_name, &nr_mode_name[i][0]);
		sprintf(lsc_tab,"static struct sensor_lsc_2d_table_param s_%s_%s_lsc_2d_table_param",sensor_name, &nr_mode_name[i][0]);

		fp = fopen(filename[i],"r");
		if (NULL == fp) {
			ISP_LOGI("The param file %s is not exit!\n",filename[i]);
			continue;
		}

		while (!feof(fp))
		{
			if (fgets(line_buf, 512, fp) == NULL) {
				break;
			}

			if (strstr(line_buf,libuse_info) != NULL) {
				rtn = read_libuse_info(fp,sensor_raw_ptr);
				if (0x00 != rtn) {
					ISP_LOGE("libuse_info was error!");
					fclose(fp);
					goto exit;
				}
				break;
			}
			if ((strstr(line_buf,lsc_2d_map_flag1) != NULL) && (strstr(line_buf,lsc_2d_map_flag2) == NULL)) {
				if(sensor_raw_ptr->fix_ptr[i]->lnc.lnc_param.lnc != NULL) {
					rtn = read_lnc_tab_size_offset_info(fp,&sensor_raw_ptr->fix_ptr[i]->lnc);
					if (0x00 != rtn) {
						ISP_LOGE("read_lnc_size_offset was error!");
						fclose(fp);
						goto exit;
					}
				}
				continue;
			}
			if (strstr(line_buf,tune_info) != NULL) {
				rtn = read_tune_info(fp,sensor_raw_ptr->mode_ptr[i].addr,&sensor_raw_ptr->mode_ptr[i].len);
				if (0x00 != rtn) {
					ISP_LOGE("read_tune_info was error!");
					fclose(fp);
					goto exit;
				}
				continue;
			}
			if (strstr(line_buf,ae_tab) != NULL) {
				if(sensor_raw_ptr->fix_ptr[i]->ae.ae_param.ae != NULL) {
					rtn = read_fix_ae_info(fp,&sensor_raw_ptr->fix_ptr[i]->ae);
					if(0x00 != rtn){
						ISP_LOGE("read_ae_info was error!");
						fclose(fp);
						goto exit;
					}
				}
				continue;
			}
			if (strstr(line_buf,awb_tab) != NULL) {
				if(sensor_raw_ptr->fix_ptr[i]->awb.awb_param.awb != NULL) {
					rtn = read_fix_awb_info(fp,&sensor_raw_ptr->fix_ptr[i]->awb);
					if (0x00 != rtn) {
						ISP_LOGE("read_awb_info was error!");
						fclose(fp);
						goto exit;
					}
				}
				continue;
			}
			if (strstr(line_buf,lsc_tab) != NULL) {
				if(sensor_raw_ptr->fix_ptr[i]->lnc.lnc_param.lnc != NULL) {
					rtn = read_fix_lnc_info(fp,&sensor_raw_ptr->fix_ptr[i]->lnc);
					if (0x00 != rtn) {
						ISP_LOGE("read_lnc_info was error!");
						fclose(fp);
						goto exit;
					}
				}
			}
			if (strstr(line_buf,note_name) != NULL) {
				ISP_LOGI("update file _tool_ui_input");
				break;
			}
		}
		fclose(fp);
	}

exit:
	if (PNULL != line_buf) {
		free(line_buf);
	}
	for (i = 0; i < 14; i++) {
		if (PNULL != filename[i]) {
			free(filename[i]);
		}
	}
	return rtn;
}
uint32_t isp_pm_raw_para_update_from_file(struct sensor_raw_info *raw_info_ptr)
{
	int rtn = 0x00;
	const char *sensor_name = PNULL;
	struct sensor_raw_info* sensor_raw_info_ptr = raw_info_ptr;

	int version = 0;
	char *filename = NULL;
	char filename0[80];
	char filename1[80];
	sensor_name = (char*)&sensor_raw_info_ptr->version_info->sensor_ver_name.sensor_name;

	sprintf(filename0,"%ssensor_%s_raw_param.c",CAMERA_DUMP_PATH, sensor_name);
	sprintf(filename1,"%ssensor_%s_raw_param_common.c",CAMERA_DUMP_PATH, sensor_name);

	if (-1 != access(filename0,0)) {
		ISP_LOGI("%s is exit!\n",filename0);
		filename = filename0;
		version = 1;
	}
	if (NULL == filename) {
		if(-1 != access(filename1,0)) {
			ISP_LOGI("%s is exit!\n",filename1);
			filename = filename1;
			version = 2;
		}
	}
	if (NULL == filename && 0 == version) {
		ISP_LOGI("there is no param file!\n");
		return rtn;
	} else {
		ISP_LOGI("the param file is %s,version = %d",filename,version);
	}

	if ( ISP_PARAM_VERSION_V21 == (TUNE_FILE_CHIP_VER_MASK &  sensor_raw_info_ptr->version_info->version_id)) {
		rtn = update_param_v21(sensor_raw_info_ptr,sensor_name);
		if(0x00 != rtn) {
			ISP_LOGI("update param error!");
			return rtn;
		}
	} else {
		rtn = update_param_v1(sensor_raw_info_ptr,sensor_name);
		if(0x00 != rtn){
			ISP_LOGI("update param error!");
			return rtn;
		}
	}
	return rtn;
}
#ifndef WIN32
uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,SENSOR_ID_E sensor_id)
{
	int rtn = 0x00;
	struct sensor_raw_info* sensor_raw_info_ptr = *(sensor_info_ptr->raw_info_ptr);
	UNUSED(sensor_id);
	isp_pm_raw_para_update_from_file(sensor_raw_info_ptr);

	return rtn;
}
#endif
#ifdef __cplusplus
}
#endif
