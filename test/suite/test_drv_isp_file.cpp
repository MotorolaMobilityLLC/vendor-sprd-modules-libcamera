#include "test_drv_isp_parameter.h"
#include "../framework/test_common_header.h"

extern struct host_info_t g_host_info[ISP_PATH_NUM];

/* -----------------------------------------------------------------------
	raw
------------------------------------------------------------------------*/

int isp_read_nlm_param(struct host_info_t *host_info);
int isp_read_vst_param(struct host_info_t *host_info);
/* -----------------------------------------------------------------------
	rgb
------------------------------------------------------------------------*/
int isp_read_rgb_se_param(struct host_info_t *host_info);
int isp_read_fGamma_param(struct host_info_t *host_info);
int isp_read_hsv_param(struct host_info_t *host_info);
/* -----------------------------------------------------------------------
	yuv
------------------------------------------------------------------------*/
int isp_read_ltm_map_param(struct host_info_t *host_info);
int isp_read_yGamma_param(struct host_info_t *host_info);
int isp_read_3dnr_mv_param(struct host_info_t *host_info);
/* -----------------------------------------------------------------------
	dcam
------------------------------------------------------------------------*/
int dcam_read_lens_param(struct host_info_t *host_info);
int dcam_read_bpc_param(struct host_info_t *host_info);
int dcam_read_bpc_noise_param(struct host_info_t *host_info);
int dcam_read_bpc_ppe_param(struct host_info_t *host_info);
int dcam_read_ppe_param(struct host_info_t *host_info);
int dcam_read_raw_af_param(struct host_info_t *host_info);
int dcam_read_raw_gtm_param(struct host_info_t *host_info);

int dcam_read_param_from_file(struct host_info_t *host_info)
{
	if (0 != dcam_read_ppe_param(host_info)) {
		return 1;
	}

	if (0 != dcam_read_lens_param(host_info)) {
		return 1;
	}

	if (0 != dcam_read_bpc_param(host_info)) {
		return 1;
	}

	if (0 != dcam_read_raw_af_param(host_info)) {
		return 1;
	}

	if (0 != dcam_read_bpc_noise_param(host_info)) {
		return 1;
	}

	if (0 != dcam_read_bpc_ppe_param(host_info)) {
		return 1;
	}

	if (0 != dcam_read_raw_gtm_param(host_info)) {
		return 1;
	}

	return 0;
}

int isp_read_param_from_file(struct host_info_t *host_info)
{
	if	(0 == (host_info->isp_param.storecrop1_info.store_bypass & host_info->isp_param.storecrop2_info.store_bypass)) {
		return dcam_read_param_from_file(host_info);
	}

	if (0 != isp_read_nlm_param(host_info)) {
		return 1;
	}

	if (0 != isp_read_vst_param(host_info)) {
		return 1;
	}

	if (0 != isp_read_rgb_se_param(host_info)) {
		return 1;
	}

	if (0 != isp_read_fGamma_param(host_info)) {
		return 1;
	}

	if (0 != isp_read_hsv_param(host_info)) {
		return 1;
	}

	if (0 != isp_read_ltm_map_param(host_info)) {
		return 1;
	}
	if (0 != isp_read_yGamma_param(host_info)) {
		return 1;
	}
	if (0 != isp_read_3dnr_mv_param(host_info)) {
		return 1;
	}

	return 0;
}

int isp_read_3dnr_mv_param(struct host_info_t *host_info)
{

	if (1 == host_info->isp_param.yuv_3dnr_info.bypass)
		return 0;

	FILE *fp = fopen(host_info->isp_param.yuv_3dnr_mem_ctrl_info.mv_file_preview, "r");
	if (fp) {
		unsigned int count = 0;
		signed int mv_x, mv_y;
		while (!feof(fp)) {
			int ret = fscanf(fp, "%d %d", &mv_x, &mv_y);
			if (ret == -1)
				break;

			host_info->isp_param.yuv_3dnr_mem_ctrl_info.mv_capture[count][0] = mv_x;
			host_info->isp_param.yuv_3dnr_mem_ctrl_info.mv_capture[count][1] = mv_y;
			count++;
		}
		fclose(fp);
		fp = NULL;
	} else {
		printf("ISP_FW: open %s fail\n", host_info->isp_param.yuv_3dnr_mem_ctrl_info.mv_file_preview);
		return 1;
	}

	return 0;
}

int dcam_read_bpc_noise_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.bpc_info.bpc_bypass)
		return 0;

	FILE *pf = fopen(host_info->isp_param.bpc_info.bpc_noise_curve_file, "r");

	int count = 0;
	int item_value1 = 0;
	int	item_value2 = 0;
	int	item_value3 = 0;

	if (NULL == pf) {
		printf("ISP_FW: [ERROR] fail to open the file %s.\n",host_info->isp_param.bpc_info.bpc_noise_curve_file);
		return 1;
	}
	while (!feof(pf) && count < 8) {
		fscanf(pf, "%d %d %d", &item_value1, &item_value2, &item_value3);
		host_info->isp_param.bpc_info.bpc_noise_i[count] = item_value1;
		host_info->isp_param.bpc_info.bpc_noise_k[count] = item_value2;
		host_info->isp_param.bpc_info.bpc_noise_b[count] = item_value3;
		count++;
	}

	fclose(pf);

	return 0;
}

int dcam_read_bpc_ppe_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.bpc_info.bpc_bypass || 0 == host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_en )
		return 0;

	FILE *pf = fopen(host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_map_file,"r");
	if (!pf) {
		printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.ppe_info.ppe_pattern_map_file);
		return 1;
	}

	int count = 0;
	int item_value1, item_value2, item_value3, item_value4;
	while (!feof(pf) && count < 64) {
		char linebuf[512];
		char *str_p = fgets(linebuf, sizeof(linebuf), pf);
		if (!str_p || '#' == linebuf[0]) {
			continue;
		}
		int item_num = sscanf(linebuf, "%d\t%d\t%d\t%d", &item_value1, &item_value2, &item_value3, &item_value4);
		if (4 == item_num)
		{
			host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[count] = item_value1;
			host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[count] = item_value2;
			host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_pos[count] = item_value4;
			count++;
			linebuf[0] = 0;
		}
	}

	host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_count = count;

	for (int i = 0; i < count; i++)
	{
		for (int j = i + 1; j < count; j++)
		{
			if (host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[i] + 64 * host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[i] > host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[j] + 64 * host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[j])
			{
				uint32 col = host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[i];
				host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[i] = host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[j];
				host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_col[j] = col;

				uint32 row = host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[i];
				host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[i] = host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[j];
				host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_row[j] = row;

				uint32 pos = host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_pos[i];
				host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_pos[i] = host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_pos[j];
				host_info->isp_param.bpc_info.bpc_ppe_info.bpc_ppi_pattern_pos[j] = pos;
			}
		}
	}

	fclose(pf);
	pf = NULL;

	return 0;
}

int dcam_read_ppe_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.ppe_info.bypass)
		return 0;

	FILE *pf = fopen(host_info->isp_param.ppe_info.ppe_pattern_map_file,"r");
	if (!pf) {
		printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.ppe_info.ppe_pattern_map_file);
		return 1;
	}

	int count = 0;
	int item_value1, item_value2, item_value3, item_value4;
	while (!feof(pf) && count < 64) {
		char linebuf[512];
		char *str_p = fgets(linebuf, sizeof(linebuf), pf);
		if (!str_p || '#' == linebuf[0]) {
			continue;
		}
		int item_num = sscanf(linebuf, "%d\t%d\t%d\t%d", &item_value1, &item_value2, &item_value3, &item_value4);
		if (4 == item_num)
		{
			host_info->isp_param.ppe_info.pattern_col[count] = item_value1;
			host_info->isp_param.ppe_info.pattern_row[count] = item_value2;
			host_info->isp_param.ppe_info.pattern_pos[count] = item_value4;
			count++;
			linebuf[0] = 0;
		}
	}

	host_info->isp_param.ppe_info.pattern_count = count;
	uint32 block_width = (8<<host_info->isp_param.ppe_info.ppe_block_width);
	for (int i = 0; i < count; i++)
	{
		for (int j = i + 1; j < count; j++)
		{
			if (host_info->isp_param.ppe_info.pattern_col[i] + block_width * host_info->isp_param.ppe_info.pattern_row[i] > host_info->isp_param.ppe_info.pattern_col[j] + block_width * host_info->isp_param.ppe_info.pattern_row[j])
			{
				uint32 col = host_info->isp_param.ppe_info.pattern_col[i];
				host_info->isp_param.ppe_info.pattern_col[i] = host_info->isp_param.ppe_info.pattern_col[j];
				host_info->isp_param.ppe_info.pattern_col[j] = col;

				uint32 row = host_info->isp_param.ppe_info.pattern_row[i];
				host_info->isp_param.ppe_info.pattern_row[i] = host_info->isp_param.ppe_info.pattern_row[j];
				host_info->isp_param.ppe_info.pattern_row[j] = row;

				uint32 pos = host_info->isp_param.ppe_info.pattern_pos[i];
				host_info->isp_param.ppe_info.pattern_pos[i] = host_info->isp_param.ppe_info.pattern_pos[j];
				host_info->isp_param.ppe_info.pattern_pos[j] = pos;
			}
		}
	}

	fclose(pf);
	pf = NULL;

	if (1 == host_info->isp_param.ppe_info.ppe_phase_map_corr_en) {
		int block_width = host_info->isp_param.ppe_info.ppe_block_end_col - host_info->isp_param.ppe_info.ppe_block_start_col;
		int ppe_grid = host_info->isp_param.ppe_info.ppe_gain_map_grid ? 64 : 32;
		int grid = 128;//(block_width + ppe_grid - 1) / ppe_grid + 1;
		int *table = new int[grid];
		int	itemvalue;
		char *gain_map[2];
		gain_map[0] = host_info->isp_param.ppe_info.ppe_phase_l_gain_map;
		gain_map[1] = host_info->isp_param.ppe_info.ppe_phase_r_gain_map;
		for (int map_id = 0; map_id < 2; map_id++) {
			pf = fopen(gain_map[map_id], "r");
			if (!pf) {
				printf("ISP_FW: open %s failed\n", gain_map[map_id]);
				return 1;
			}
			uint32 grid_size;
			for (int i = 0; i < grid && !feof(pf); i++) {
				fscanf(pf, "%d", &itemvalue);
				*(table + i) = itemvalue;
				grid_size = i;
			}
			fclose(pf);
			pf = NULL;

			host_info->isp_param.ppe_info.grid = grid;
			if (map_id == 0) {
				for (int i = 0; i < grid_size; i++)
					host_info->isp_param.ppe_info.l_gain[i] = table[i];
			} else {
				for (int i = 0; i < grid_size; i++)
					host_info->isp_param.ppe_info.r_gain[i] = table[i];
			}
		}
		delete table;
	}

	return 0;
}

int isp_read_rgb_se_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.pstrz_info.bypass)
		return 0;

	int file_num = 1;
	FILE *file[2];
	file[0] = fopen(host_info->isp_param.pstrz_info.posterize_filename[0], "r");
	file[1] = NULL;

	if(strlen(host_info->isp_param.pstrz_info.posterize_filename[1])){
		file[1] = fopen(host_info->isp_param.pstrz_info.posterize_filename[1], "r");
		file_num = 2;
	}

	int count = 0;
	int item_value1, item_value2, item_value3;
	host_info->isp_param.pstrz_info.pingpong = file_num - 1;

	for(int i = 0 ; i< file_num; i++)
	{
		if (file[i]) {
			while (!feof(file[i])) {
				fscanf(file[i], "%d\t%d\t%d", &item_value1, &item_value2, &item_value3);
				host_info->isp_param.pstrz_info.posterize_r_data[i][count] = item_value1;
				host_info->isp_param.pstrz_info.posterize_g_data[i][count] = item_value2;
				host_info->isp_param.pstrz_info.posterize_b_data[i][count] = item_value3;
				count++;
				if(129 == count) break;
			}
			fclose(file[i]);
		} else {
			printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.pstrz_info.posterize_filename[i]);
			return 1;
		}
		count = 0;
	}
	return 0;
}

int isp_read_vst_param(struct host_info_t *host_info)
{
	if (0 == host_info->isp_param.nlm_info.vst_en)
		return 0;

	uint32 path_num = host_info->path_num;
	int i = 0;
	FILE *file = NULL;
	char OutputFileName[STRING_SIZE];
	int ping_pong = host_info->img_num <= PINGPONG ? 1 : 2;

	for (int pp_i = 0; pp_i < ping_pong; pp_i++) {
		//vst
		if (!host_info->isp_param.nlm_info.vst_filename[pp_i][0])
			strcpy(host_info->isp_param.nlm_info.vst_filename[pp_i],host_info->isp_param.nlm_info.vst_filename[1-pp_i]);
		file = fopen(host_info->isp_param.nlm_info.vst_filename[pp_i], "r");

		sprintf(OutputFileName, "%s%d%s", host_info->output_vector_fpga,pp_i,"__");
		strcat(OutputFileName,"nlm_vst.txt");

		if (file) {
			for (i = 0; i < 1025; i++) {
				int tmp;
				fscanf(file, "%d", &tmp);
				host_info->isp_param.nlm_info.vst_table[pp_i][i] = tmp;
			}
			fclose(file);
			file = NULL;
		} else {
			printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.nlm_info.vst_filename[pp_i]);
			return 1;
		}

		//ivst
		if (!host_info->isp_param.nlm_info.vst_inv_filename[pp_i][0])
			strcpy(host_info->isp_param.nlm_info.vst_inv_filename[pp_i],host_info->isp_param.nlm_info.vst_inv_filename[1-pp_i]);
		file = fopen(host_info->isp_param.nlm_info.vst_inv_filename[pp_i],"r");

		sprintf(OutputFileName, "%s%d%s", host_info->output_vector_fpga,pp_i,"__");
		strcat(OutputFileName,"nlm_ivst.txt");

		if (file) {
			for (i=0; i<1025; i++) {
				int tmp;
				fscanf(file, "%d", &tmp);
				host_info->isp_param.nlm_info.vst_inv_table[pp_i][i] = tmp;
			}
			fclose(file);
			file = NULL;
		} else {
			printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.nlm_info.vst_inv_filename[pp_i]);
			return 1;
		}
	}

	return 0;
}

int isp_read_nlm_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.nlm_info.NLM_bypass)
		return 0;

	uint32 path_num = host_info->path_num;
	int i, value;
	int weight_num;
	FILE *pf = NULL;
	char OutputFileName[STRING_SIZE];

	weight_num = host_info->isp_param.nlm_info.nlm_argument_number;
	if(weight_num>2){
		printf("ISP_FW: Failed! the weight parameter number > 2 ,while the nlm_weight_file number is <=2 !\n");
		return 0;
	}

	for(int j=0;j<2;j++)
	{
		pf = fopen(host_info->isp_param.nlm_info.nlm_weight_filename[j], "r");

		if (pf)
		{
			for (i=0; i<72; i++) {
					fscanf(pf, "%d", &value);
					host_info->isp_param.nlm_info.NLM_w[j][i] = value;
				}
			fclose(pf);
			pf = NULL;
		}
		else
		{
			printf("ISP_FW : fail to open %s\n", host_info->isp_param.nlm_info.nlm_weight_filename);
			return 1;
		}

		if(weight_num==1)
			break;

	}
	return 0;
}

int isp_read_fGamma_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.gamma_info.bypass)
		return 0;

	uint32 path_num = host_info->path_num;
	FILE *pf = NULL;
	int ping_pong = host_info->img_num <= PINGPONG ? 1 : 2;
	int gmc_red[257];
	int gmc_green[257];
	int gmc_blue[257];

	for (int pp_i = 0; pp_i < ping_pong; pp_i++) {
		if (!host_info->isp_param.gamma_info.gmc_curve_file[pp_i][0])
			strcpy(host_info->isp_param.gamma_info.gmc_curve_file[pp_i],host_info->isp_param.gamma_info.gmc_curve_file[1-pp_i]);
		pf = fopen(host_info->isp_param.gamma_info.gmc_curve_file[pp_i], "r");
		if (pf) {
			int	item_value1, item_value2, item_value3;
			int count = 0;
			while (!feof(pf)) {
				fscanf(pf, "%d\t%d\t%d", &item_value1, &item_value2, &item_value3);
				gmc_red[count] = item_value1;
				gmc_green[count] = item_value2;
				gmc_blue[count] = item_value3;
				count++;
				if(257 == count) break;
			}
			fclose(pf);
			pf = NULL;
		} else {
			printf("\nISP_FW : fail to open %s\n", host_info->isp_param.gamma_info.gmc_curve_file[pp_i]);
			return 1;
		}

		for (int i = 0; i < 256; ++i) {
			host_info->isp_param.gamma_info.gamma_r[pp_i][i] = ((gmc_red[i]   & 0xFF) << 8) | (gmc_red[i+1]   & 0xFF);
			host_info->isp_param.gamma_info.gamma_g[pp_i][i] = ((gmc_green[i] & 0xFF) << 8) | (gmc_green[i+1] & 0xFF);
			host_info->isp_param.gamma_info.gamma_b[pp_i][i] = ((gmc_blue[i]  & 0xFF) << 8) | (gmc_blue[i+1]  & 0xFF);
		}
	}

	return 0;
}

int isp_read_yGamma_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.ygamma_info.bypass)
		return 0;

	uint32 path_num = host_info->path_num;
	FILE *pf = NULL;
	int ping_pong = host_info->img_num <= PINGPONG ? 1 : 2;

	for (int pp_i = 0; pp_i < ping_pong; pp_i++) {
		if (!host_info->isp_param.ygamma_info.solarize_curve_file[pp_i][0])
			strcpy(host_info->isp_param.ygamma_info.solarize_curve_file[pp_i],host_info->isp_param.ygamma_info.solarize_curve_file[1-pp_i]);
		pf = fopen(host_info->isp_param.ygamma_info.solarize_curve_file[pp_i], "r");
		if (pf) {
			int	item_value = 0 ;
			int count = 0;
			while (!feof(pf)) {
				fscanf(pf, "%d", &item_value);
				host_info->isp_param.ygamma_info.solarize_y[pp_i][count] = item_value;
				count++;
				if(129 == count) break;
			}
			fclose(pf);
			pf = NULL;
		} else {
			printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.ygamma_info.solarize_curve_file[pp_i]);
			return 1;
		}
	}

	return 0;
}

int isp_read_hsv_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.hsv_info.bypass)
		return 0;

	uint32 path_num = host_info->path_num;
	FILE *fp = NULL;
	int i;
	uint16 hue_table[360];
	uint16 sat_table[360];
	int ping_pong = host_info->img_num <= PINGPONG ? 1 : 2;

	for (int pp_i = 0; pp_i < ping_pong; pp_i++) {
		//hsv_color_tuning_filename
		if (!host_info->isp_param.hsv_info.hsv_color_tuning_filename[pp_i][0])
			strcpy(host_info->isp_param.hsv_info.hsv_color_tuning_filename[pp_i],host_info->isp_param.hsv_info.hsv_color_tuning_filename[1-pp_i]);
		fp = fopen(host_info->isp_param.hsv_info.hsv_color_tuning_filename[pp_i], "r");
		if (fp) {
			uint16 hue, sat;
			for (i = 0; i < 360; i++) {
				fscanf(fp, "%hu %hu", &hue, &sat);
				hue_table[i] = hue;
				sat_table[i] = sat;
			}
			fclose(fp);
			fp = NULL;
		} else {
			printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.hsv_info.hsv_color_tuning_filename[pp_i]);
			return 1;
		}
		for (i = 0; i < 360; i++)
			host_info->isp_param.hsv_info.hsv_table[pp_i][i] = (hue_table[i] & 0x1FF) | ((sat_table[i] & 0x7FF) << 9);

		//hsv_sub_region_filename
		if (!host_info->isp_param.hsv_info.hsv_sub_region_filename[pp_i][0])
			strcpy(host_info->isp_param.hsv_info.hsv_sub_region_filename[pp_i],host_info->isp_param.hsv_info.hsv_sub_region_filename[1-pp_i]);
		fp = fopen(host_info->isp_param.hsv_info.hsv_sub_region_filename[pp_i], "r");
		if (fp) {
			for (i = 0; i < 5; i++) {
				fscanf(fp,"%d %d %d %d %d %d %d %d %d %d %d %d %d %d", &host_info->isp_param.hsv_info.h[pp_i][i][0], &host_info->isp_param.hsv_info.h[pp_i][i][1],
					&host_info->isp_param.hsv_info.s[pp_i][i][0], &host_info->isp_param.hsv_info.s[pp_i][i][1],
					&host_info->isp_param.hsv_info.s[pp_i][i][2], &host_info->isp_param.hsv_info.s[pp_i][i][3],
					&host_info->isp_param.hsv_info.v[pp_i][i][0], &host_info->isp_param.hsv_info.v[pp_i][i][1],
					&host_info->isp_param.hsv_info.v[pp_i][i][2], &host_info->isp_param.hsv_info.v[pp_i][i][3],
					&host_info->isp_param.hsv_info.r_s[pp_i][i][0], &host_info->isp_param.hsv_info.r_s[pp_i][i][1],
					&host_info->isp_param.hsv_info.r_v[pp_i][i][0], &host_info->isp_param.hsv_info.r_v[pp_i][i][1]);
			}
			fclose(fp);
			fp = NULL;
		} else {
			printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.hsv_info.hsv_sub_region_filename[pp_i]);
			return 1;
		}
	}

	return 0;
}

int dcam_read_raw_af_param(struct host_info_t *host_info)
{
	if (1 == host_info->isp_param.afm_info.bypass) {
		return 0;
	}

	FILE *file = fopen(host_info->isp_param.afm_info.fv1_coeff, "r");
	int i,j,k;

	if (NULL == file) {
		printf("ISP_FW: [ERROR] open %s failed\n", host_info->isp_param.afm_info.fv1_coeff);
		return -1;
	}

	int value;
	for (i=0; i<4; i++) {
		for(j = 0;j<3;j++) {
			for(k=0;k<3;k++) {
				fscanf(file, "%d", &value);
				if(i == 0) {
					host_info->isp_param.afm_info.mask_fv11[j][k] = value;
				}
				else if(i==1) {
					host_info->isp_param.afm_info.mask_fv12[j][k] = value;
				}
				else if(i==2) {
					host_info->isp_param.afm_info.mask_fv13[j][k] = value;
				}
				else if(i==3) {
					host_info->isp_param.afm_info.mask_fv14[j][k] = value;
				}
			}
		}
	}

	fclose(file);

	return 0;
}

int dcam_read_lens_param(struct host_info_t *host_info)
{
	uint32 path_num = host_info->path_num;
	int ping_pang = host_info->img_num <= PINGPONG ? 1 : 2;
	int pp_i = 0;
	FILE *pf = NULL;
#if _CMODEL_
	char file_name[STRING_SIZE];
#endif
	if (1 == host_info->isp_param.dcam_lsc_info.bypass) {
		return 0;
	}

	uint16 len_grid = host_info->isp_param.dcam_lsc_info.LNC_GRID;
	uint16 image_width = host_info->isp_param.general_info.general_image_width;
	uint16 image_height = host_info->isp_param.general_info.general_image_height;
	uint16 grid_x = (image_width / 2  % len_grid) ? (image_width / 2 / len_grid + 2) : (image_width / 2  / len_grid + 1);
	uint16 grid_y = (image_height / 2 % len_grid) ? (image_height / 2 / len_grid + 2) : (image_height / 2 / len_grid + 1);
	grid_x += 2;
	grid_y += 2;
	uint32 buffer_size = 4 * grid_x * grid_y * sizeof(uint16);
	host_info->isp_param.dcam_lsc_info.buffer_size_bytes = buffer_size;

	uint32 i_current = host_info->current_img;
	FILE *pR0C0 = NULL;
	FILE *pR0C1 = NULL;
	FILE *pR1C0 = NULL;
	FILE *pR1C1 = NULL;

	uint16 i, j;
	char buffer_name[STRING_SIZE];
	for (pp_i = 0; pp_i < ping_pang; pp_i++) {
		if (!host_info->isp_param.dcam_lsc_file.full_r0c0_file[pp_i][0])
			strcpy(host_info->isp_param.dcam_lsc_file.full_r0c0_file[pp_i],host_info->isp_param.dcam_lsc_file.full_r0c0_file[1-pp_i]);
		if (NULL == (pR0C0 = fopen(host_info->isp_param.dcam_lsc_file.full_r0c0_file[pp_i], "r"))) {
			printf("ISP_FW: [ERROR] fail to open the len param file: %s.\n", host_info->isp_param.dcam_lsc_file.full_r0c0_file[pp_i]);
			return 1;
		}

		if (!host_info->isp_param.dcam_lsc_file.full_r0c1_file[pp_i][0])
			strcpy(host_info->isp_param.dcam_lsc_file.full_r0c1_file[pp_i],host_info->isp_param.dcam_lsc_file.full_r0c1_file[1-pp_i]);
		if (NULL == (pR0C1 = fopen(host_info->isp_param.dcam_lsc_file.full_r0c1_file[pp_i], "r"))) {
			printf("ISP_FW: [ERROR] fail to open the len param file: %s.\n", host_info->isp_param.dcam_lsc_file.full_r0c1_file[pp_i]);
			return 1;
		}

		if (!host_info->isp_param.dcam_lsc_file.full_r1c0_file[pp_i][0])
			strcpy(host_info->isp_param.dcam_lsc_file.full_r1c0_file[pp_i],host_info->isp_param.dcam_lsc_file.full_r1c0_file[1-pp_i]);
		if (NULL == (pR1C0 = fopen(host_info->isp_param.dcam_lsc_file.full_r1c0_file[pp_i], "r"))) {
			printf("ISP_FW: [ERROR] fail to open the len param file: %s.\n", host_info->isp_param.dcam_lsc_file.full_r1c0_file[pp_i]);
			return 1;
		}

		if (!host_info->isp_param.dcam_lsc_file.full_r1c1_file[pp_i][0])
			strcpy(host_info->isp_param.dcam_lsc_file.full_r1c1_file[pp_i],host_info->isp_param.dcam_lsc_file.full_r1c1_file[1-pp_i]);
		if (NULL == (pR1C1 = fopen(host_info->isp_param.dcam_lsc_file.full_r1c1_file[pp_i], "r"))) {
			printf("ISP_FW: [ERROR] fail to open the len param file: %s.\n", host_info->isp_param.dcam_lsc_file.full_r1c1_file[pp_i]);
			return 1;
		}

		uint16 *pBuf16_r0c0 = (uint16 *)malloc(buffer_size/4);
		uint16 *pBuf16_r0c1 = (uint16 *)malloc(buffer_size/4);
		uint16 *pBuf16_r1c0 = (uint16 *)malloc(buffer_size/4);
		uint16 *pBuf16_r1c1 = (uint16 *)malloc(buffer_size/4);
		uint16 *pBuf16_tmp_r0c0 = pBuf16_r0c0;
		uint16 *pBuf16_tmp_r0c1 = pBuf16_r0c1;
		uint16 *pBuf16_tmp_r1c0 = pBuf16_r1c0;
		uint16 *pBuf16_tmp_r1c1 = pBuf16_r1c1;

		uint16 *pBuf16 = (uint16 *)malloc(buffer_size);
		uint16 *pBuf16_tmp = pBuf16;

		uint16 tmp16;
		for (i = 0; i < grid_y; i++) {
			for (j = 0; j < grid_x; j++) {
				fscanf(pR0C0, "%hd", &tmp16);
				*pBuf16_tmp++ = tmp16;
				*pBuf16_tmp_r0c0++ = tmp16;
				fscanf(pR0C1, "%hd", &tmp16);
				*pBuf16_tmp++ = tmp16;
				*pBuf16_tmp_r0c1++ = tmp16;
				fscanf(pR1C0, "%hd", &tmp16);
				*pBuf16_tmp++ = tmp16;
				*pBuf16_tmp_r1c0++ = tmp16;
				fscanf(pR1C1, "%hd", &tmp16);
				*pBuf16_tmp++ = tmp16;
				*pBuf16_tmp_r1c1++ = tmp16;
			}
		}
		uint32 bayer_mode = host_info->isp_param.general_info.bayer_pattern;
		uint16 *pBuf16b = NULL;
		uint16 *pBuf16gb = NULL;
		uint16 *pBuf16gr = NULL;
		uint16 *pBuf16r = NULL;

		switch (bayer_mode){
		case 0://gr r b gb
			pBuf16b = pBuf16_r1c0;
			pBuf16gb = pBuf16_r1c1;
			pBuf16gr = pBuf16_r0c0;
			pBuf16r = pBuf16_r0c1;
			break;
		case 1://r gr gb b
			pBuf16b = pBuf16_r1c1;
			pBuf16gb = pBuf16_r1c0;
			pBuf16gr = pBuf16_r0c1;
			pBuf16r = pBuf16_r0c0;
			break;
		case 2://b gb gr r
			pBuf16b = pBuf16_r0c0;
			pBuf16gb = pBuf16_r0c1;
			pBuf16gr = pBuf16_r1c0;
			pBuf16r = pBuf16_r1c1;
			break;
		case 3://gb b r gr
			pBuf16b = pBuf16_r0c1;
			pBuf16gb = pBuf16_r0c0;
			pBuf16gr = pBuf16_r1c1;
			pBuf16r = pBuf16_r1c0;
			break;
		}

		uint16 *pBuf_grid_table = (uint16 *)malloc(buffer_size);
		for(int i = 0; i < grid_x * grid_y ;i++){
			*(pBuf_grid_table+4*i) = *(pBuf16b+i) ;
			*(pBuf_grid_table+4*i+1) = *(pBuf16gb +i) ;
			*(pBuf_grid_table+4*i+2) = *(pBuf16gr +i);
			*(pBuf_grid_table+4*i+3) = *(pBuf16r +i);
		}

		free(pBuf16);
		fclose(pR0C0);
		fclose(pR0C1);
		fclose(pR1C0);
		fclose(pR1C1);
		pR0C0 = NULL;
		pR0C1 = NULL;
		pR1C0 = NULL;
		pR1C1 = NULL;
	}

	return 0;
}

int dcam_read_bpc_param(struct host_info_t *host_info)
{
	uint32 path_num = host_info->path_num;
	char file_name[STRING_SIZE] = {};
	FILE *pf = NULL;
	if ((1 == host_info->isp_param.bpc_info.bpc_bypass) || (0 == host_info->isp_param.bpc_info.bpc_mode)) {
#if _CMODEL_
		sprintf(file_name, "%sisp_firmware_full_map.bin", host_info->output_file);
		if (NULL == (pf = fopen(file_name, "wb")))
		{
			printf("ISP_FW: [ERROR] fail to open the isp_full_map.bin.\n") ;
			return 1;
		}
		fclose(pf);
		pf = NULL;
#endif
		return 0;
	}

	if(NULL == (pf = fopen(host_info->isp_param.bpc_info.bpc_map_file, "r"))) {
		printf("ISP_FW: [ERROR] fail to open the bpc parameter file: %s.\n", host_info->isp_param.bpc_info.bpc_map_file);
		return 1;
	}

	uint16 x_cor = 0;
	uint16 y_cor = 0;
	int bad_pixel_count = 0;
	while (!feof(pf)) {
		fscanf(pf, "%hu %hu\n", &x_cor, &y_cor);
		uint32 value = (y_cor << 16) + x_cor;
		host_info->bad_pixel_pos.push_back(value);
		bad_pixel_count++;
		if (bad_pixel_count > MAX_BADPIXEL_NUM) {
			printf("ISP_FW: [ERROR] too many bad pixels[%d]\n", bad_pixel_count);
			exit(EXIT_PARAM_INVALID);
		}
	}
	fclose(pf);
	pf = NULL;

	return 0;
}

int isp_read_ltm_map_param(struct host_info_t *host_info)
{
	for (int ltm_id = 0; ltm_id < LTM_NUM; ltm_id++) {
		if (1 == host_info->isp_param.ltm_map_info[ltm_id].bypass)
			continue;

		if (host_info->isp_param.ltm_stat_info[ltm_id].bypass || host_info->path_num == 2 || host_info->path_num == 3) {
			int tile_num_x, tile_num_y, tile_width_stat, tile_height_stat, frame_width_stat, frame_height_stat;
			char stat_file_name[STRING_SIZE] = {0};
			char temp[STRING_SIZE] = {0};
			char base[STRING_SIZE] = {0};
			char* p;
			int num = 0;
			int r, c, m, count;
			int val1, val2, val;
			FILE *fp_stat = NULL;
			FILE *fp = NULL;
			fp = fopen(host_info->isp_param.ltm_map_info[ltm_id].tile_file, "r");
			if (!fp) {
				printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.ltm_map_info[ltm_id].tile_file);
				return 1;
			}
			if (ltm_id == 0){
				fscanf(fp,"tile_num_row = %d\n", &tile_num_y);
				fscanf(fp,"tile_num_col = %d", &tile_num_x);
			} else {
				fscanf(fp,"tile_num_x = %d\n", &tile_num_x);
				fscanf(fp,"tile_num_y = %d", &tile_num_y);
			}
			host_info->isp_param.ltm_map_info[ltm_id].tile_num_x_stat = tile_num_x;
			host_info->isp_param.ltm_map_info[ltm_id].tile_num_y_stat = tile_num_y;

			fp = fopen(host_info->isp_param.ltm_map_info[ltm_id].stat_file, "r");
			if (!fp) {
				printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.ltm_map_info[ltm_id].stat_file);
				return 1;
			}

			while (0 == feof(fp)) {
				if (1 != fscanf(fp, "%s", temp))
					continue;
				strcpy(stat_file_name, temp);
				strcpy(base, host_info->isp_param.ltm_map_info[ltm_id].stat_file);
				if (p = strrchr(base, '\\')) {
					*(p+1) = '\0';
					strcat(base, temp);
					strcpy(stat_file_name, base);
				}
				if (NULL == (fp_stat = fopen(stat_file_name, "r"))) {
					printf("ISP_FW: [ERROR] fail to open %s\n", temp);
					fclose(fp);
					fp = NULL;
					return 1;
				}

				fscanf(fp_stat,"tile_width = %d\n",     &tile_width_stat);
				fscanf(fp_stat,"tile_height = %d\n",    &tile_height_stat);
				fscanf(fp_stat,"img_width = %d\n",      &frame_width_stat);
				fscanf(fp_stat,"img_height = %d\n",     &frame_height_stat);

				host_info->isp_param.ltm_map_info[ltm_id].tile_width_stat = tile_width_stat;
				host_info->isp_param.ltm_map_info[ltm_id].tile_height_stat = tile_height_stat;
				host_info->isp_param.ltm_map_info[ltm_id].frame_width_stat = frame_width_stat;
				host_info->isp_param.ltm_map_info[ltm_id].frame_height_stat = frame_height_stat;

				count = 0;
				for (r=0; r<tile_num_y; r++) {
					for (c=0; c<tile_num_x; c++) {
						fscanf(fp_stat,"histogram[%d][%d] = ", &r, &c);
						for (m=0; m<LTM_TILE_BIN; m+=2) {
							fscanf(fp_stat,"%d %d ", &val1, &val2);
							val = (val1 & 0xFFFF) | ((val2 & 0xFFFF) << 16);
							host_info->isp_param.ltm_map_info[ltm_id].hist[num][count++] = val;
						}
					}
				}
				num++;
			}

			fclose(fp);
			fclose(fp_stat);
			fp = NULL;
			fp_stat = NULL;
		}
	}
	return 0;
}

int dcam_read_raw_gtm_param(struct host_info_t *host_info)
{
	struct ISP_PARAM_T *isp_param_ptr = &host_info->isp_param;
	int gtm_stat_en = !isp_param_ptr->dcam_raw_gtm_stat_info.bypass;
	int gtm_map_en = !isp_param_ptr->dcam_raw_gtm_map_info.bypass;
	isp_param_ptr->dcam_raw_gtm_stat_info.mod_en = gtm_stat_en || gtm_map_en;
	if (!isp_param_ptr->dcam_raw_gtm_stat_info.mod_en)
		return 0;
	char stat_file_name[STRING_SIZE] = {0};
	char hist_file_name[STRING_SIZE] = {0};
	char temp[STRING_SIZE] = {0};
	char base[STRING_SIZE] = {0};
	char* p;
	FILE *fp_stat = NULL;
	FILE *fp = NULL;
	FILE *fp_hist = NULL;
	char param_name[STRING_SIZE];
	int param_num = 10;
	int param_val[100];
	int hist_val[GTM_HIST_BIN_NUM];
	fp = fopen(host_info->isp_param.dcam_raw_gtm_stat_info.stat_file, "r");
	if (!fp) {
		printf("ISP_FW: [ERROR] fail to open %s\n", host_info->isp_param.dcam_raw_gtm_stat_info.stat_file);
		return 1;
	}
	int img_id = 0;
	while (0 == feof(fp)) {
		char txtpath[256] = "/data/vendor/cameraserver/testdcam/";
		if (1 != fscanf(fp, "%s", temp))
			continue;
		strcpy(stat_file_name, temp);
		strcpy(base, host_info->isp_param.dcam_raw_gtm_stat_info.stat_file);
		if (p = strrchr(base, '\\')) {
			*(p+1) = '\0';
			strcat(base, temp);
			if(gtm_stat_en == 0)
				strcpy(stat_file_name, base);
		}
		if (NULL == (fp_stat = fopen(strcat(txtpath, stat_file_name), "r"))) {
			printf("ISP_FW: [ERROR] fail to open %s\n", temp);
			fclose(fp);
			fp = NULL;
			return 1;
		}

		for (int n=0;n<param_num;n++)
		{
			fscanf(fp_stat,"%s = %d",param_name,&param_val[n]);
		}
		fscanf(fp,"%*[^\n]%*c");

		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_ymin_arry[img_id] = param_val[0];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_ymax_arry[img_id] = param_val[1];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_yavg_arry[img_id] = param_val[2];
		isp_param_ptr->dcam_raw_gtm_stat_info.target_norm_arry[img_id]=param_val[3];
		isp_param_ptr->dcam_raw_gtm_stat_info.image_key_arry[img_id] = param_val[4];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_lr_int_arry[img_id] = param_val[5];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_log_min_int_arry[img_id] = param_val[6];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_log_max_int_arry[img_id] = param_val[7];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_log_diff_int_arry[img_id] =param_val[8];
		isp_param_ptr->dcam_raw_gtm_stat_info.gtm_log_diff_arry[img_id] = param_val[9];
		fclose(fp_stat);
		img_id++;
	}
	//read hist file
	if((1 == isp_param_ptr->general_info.general_vector_rtl_level)&&(gtm_stat_en)){
		sprintf(hist_file_name, "%s%s%s", host_info->output_file,"vector_output\/test_raw_gtm_stat_hist",".dat");
		fp_hist = fopen(hist_file_name, "r");
		if (!fp_hist) {
			printf("ISP_FW: [ERROR] fail to open %p\n", fp_hist);
			return 1;
		}
		img_id = 0;
		while (0 == feof(fp_hist)) {
			for (int n=0;n<GTM_HIST_BIN_NUM;n++){
				fscanf(fp_hist,"%x\n",&hist_val[n]);
				isp_param_ptr->dcam_raw_gtm_stat_info.tm_hist_xpts_arry[img_id][n] = hist_val[n];
			}
			img_id++;
		}
		fclose(fp_hist);
	}
	fclose(fp);
	fp = NULL;
	fp_stat = NULL;
	fp_hist = NULL;
	return 0;
}
