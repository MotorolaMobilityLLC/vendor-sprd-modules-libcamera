#include "ae_sprd_flash_calibration.h"
#include "cmr_log.h"
#include <cutils/properties.h>
/*
 * for flash calibration
*/
#define CALI_VERSION 6
#define CALI_VERSION_SUB 20190320
struct flash_led_brightness {
	uint16 levelNumber_pf1;
	uint16 levelNumber_pf2;
	uint16 levelNumber_mf1;
	uint16 levelNumber_mf2;
	uint16 ledBrightness_pf1[32];
	uint16 ledBrightness_pf2[32];
	uint16 ledBrightness_mf1[32];
	uint16 ledBrightness_mf2[32];
	uint16 index_pf1[32];
	uint16 index_pf2[32];
	uint16 index_mf1[32];
	uint16 index_mf2[32];

};

struct flash_g_frames {
	uint16 levelNumber_pf1;
	uint16 levelNumber_pf2;
	uint16 levelNumber_mf1;
	uint16 levelNumber_mf2;
	float shutter_off;
	float shutter_pf1[32];
	float shutter_pf2[32];
	float shutter_mf1[32];
	float shutter_mf2[32];
	float gain_off;
	uint32 gain_pf1[32];
	uint32 gain_pf2[32];
	uint32 gain_mf1[32];
	uint32 gain_mf2[32];
	float g_off[15];
	float g_pf1[32][15];
	float g_pf2[32][15];
	float g_mf1[32][15];
	float g_mf2[32][15];
	uint16 index_pf1[32];
	uint16 index_pf2[32];
	uint16 index_mf1[32];
	uint16 index_mf2[32];
};
struct flash_calibration_data {
	uint32 version;
	uint32 version_sub;
	char name[32];
	int32 error;
	uint8 preflashLevelNum1;
	uint8 preflashLevelNum2;
	uint8 flashLevelNum1;
	uint8 flashLevelNum2;
	bool flashMask[1024];
	uint16 brightnessTable[1024];
	uint16 rTable[1024];		//g: 1024
	uint16 bTable[1024];
	uint16 preflashBrightness[1024];
	uint16 rTable_preflash[1024];
	int16 driverIndexP1[32];
	int16 driverIndexP2[32];
	int16 driverIndexM1[32];
	int16 driverIndexM2[32];

	uint16 maP1[32];
	uint16 maP2[32];
	uint16 maM1[32];
	uint16 maM2[32];
	uint16 numP1_hwSample;
	uint16 numP2_hwSample;
	uint16 numM1_hwSample;
	uint16 numM2_hwSample;
	uint16 mAMaxP1;
	uint16 mAMaxP2;
	uint16 mAMaxP12;
	uint16 mAMaxM1;
	uint16 mAMaxM2;
	uint16 mAMaxM12;
	uint16 null_data[2];

	float rgRatPf;
	float null_data2;
	uint16 rPf1[32];
	uint16 gPf1[32];
	uint16 rPf2[32];
	uint16 gPf2[32];
	float rgTab[20];
	float ctTab[20];
	uint16 otp_random_r;
	uint16 otp_random_g;
	uint16 otp_random_b;
	uint16 otp_golden_r;
	uint16 otp_golden_g;
	uint16 otp_golden_b;
	uint16 null_data3[2];

};

enum FlashCaliError {
	FlashCali_too_close = -10000,
	FlashCali_too_far,
};

enum FlashCaliState {
	FlashCali_start,
	FlashCali_ae,
	FlashCali_cali,
	FlashCali_end,
	FlashCali_none,
};

struct FCData {
	int numP1_hw;
	int numP2_hw;
	int numM1_hw;
	int numM2_hw;

	int numP1_hwSample;
	int indP1_hwSample[256];
	float maP1_hwSample[256];
	int numP2_hwSample;
	int indP2_hwSample[256];
	float maP2_hwSample[256];
	int numM1_hwSample;
	int indM1_hwSample[256];
	float maM1_hwSample[256];
	int numM2_hwSample;
	int indM2_hwSample[256];
	float maM2_hwSample[256];

	float mAMaxP1;
	float mAMaxP2;
	float mAMaxP12;
	float mAMaxM1;
	float mAMaxM2;
	float mAMaxM12;

	int pf_st;
	int pf_ed;
	int mf_st;
	int mf_ed;
	int pf_total;
	int mf_total;

	int numP1_alg;
	int numP2_alg;
	int numM1_alg;
	int numM2_alg;
	int indHwP1_alg[32];
	int indHwP2_alg[32];
	int indHwM1_alg[32];
	int indHwM2_alg[32];
	float maHwP1_alg[32];
	float maHwP2_alg[32];
	float maHwM1_alg[32];
	float maHwM2_alg[32];

	int mask[1024];

	float rBuf[1024];			//for sta
	float gBuf[1024];			//for sta
	float bBuf[1024];			//for sta
	float expTime;
	int gain;
	float expTimeBase;
	int gainBase;
	float nextexpTime;
	int nextgain;


	int testIndAll;
	uint32 testInd;
	int isMainTab[200];
	int ind1Tab[200];
	int ind2Tab[200];
	int testMinFrm[200];
	int expReset[200];

	float expTab[200];
	int gainTab[200];

	float rData[200];
	float gData[200];
	float bData[200];

	int stateAeFrameCntSt;
	int stateCaliFrameCntSt;
	int stateCaliFrameCntStSub;

	float rFrame[200][15];
	float gFrame[200][15];
	float bFrame[200][15];

	struct flash_calibration_data out;

};
#include <math.h>
#define FLOAT_PRECISION 0.0005
#define FLOAT_EQUAL(_x, _y) ((fabs(_x - _y) < FLOAT_PRECISION) ? 1 : 0)

//============================================================
//input and output
//name: property name
//n: contains n int in property values string
//data: property values string to int data
//return: number of valid data, max: 10
static int get_prop_multi(const char *name, int n, int *data)
{
#define MAX_PROP_DATA_NUM 10
	int i;
	char str[300] = "";
	char strTemp[300] = "";
	for (i = 0; i < n; i++)
		data[i] = 0;

	property_get(name, str, "");

	int ret = -1;
	if (strlen(str) == 0)
		return 0;

#define FIND_NUM 1
#define FIND_ZERO 0
	int state = FIND_NUM;
	int charCnt = 0;
	int dataCnt = 0;
	ret = 0;
	int len;
	len = (int)strlen(str);
	for (i = 0; i < len; i++) {
		if (state == FIND_NUM) {
			if (str[i] == ' ' || str[i] == '\t') {

			} else {
				strTemp[charCnt] = str[i];
				charCnt++;
				state = FIND_ZERO;
			}
		} else {
			if (str[i] == ' ' || str[i] == '\t') {
				state = FIND_NUM;
				if (dataCnt < MAX_PROP_DATA_NUM) {
					strTemp[charCnt] = 0;
					data[dataCnt] = atoi(strTemp);
					dataCnt++;
					charCnt = 0;
				}
			} else {
				strTemp[charCnt] = str[i];
				charCnt++;
			}
		}
	}
	if (charCnt != 0) {
		if (dataCnt < MAX_PROP_DATA_NUM) {
			strTemp[charCnt] = 0;
			data[dataCnt] = atoi(strTemp);
			dataCnt++;
		}
	}
	ret = dataCnt;
	return ret;
}

static int _aem_stat_preprocess2(cmr_u32 * src_aem_stat, float *dst_r, float *dst_g, float *dst_b, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u64 bayer_pixels = (cmr_u64)aem_blk_size.w * (cmr_u64)aem_blk_size.h / 4;
	cmr_u32 stat_blocks = aem_blk_num.w * aem_blk_num.h;
	cmr_u32 *src_r_stat = (cmr_u32 *) src_aem_stat;
	cmr_u32 *src_g_stat = (cmr_u32 *) src_aem_stat + stat_blocks;
	cmr_u32 *src_b_stat = (cmr_u32 *) src_aem_stat + (2 * stat_blocks);
	//cmr_u16 *dst_r = (float*)dst_aem_stat;
	//cmr_u16 *dst_g = (float*)dst_aem_stat + stat_blocks;
	//cmr_u16 *dst_b = (float*)dst_aem_stat + stat_blocks * 2;
	double max_value = 1023;
	double avg = 0;
	cmr_u32 i = 0;
	double r = 0, g = 0, b = 0;

	UNUSED(aem_shift);
	if (bayer_pixels < 1)
		return AE_ERROR;

	for (i = 0; i < stat_blocks; i++) {
/*for r channel */
		avg = *src_r_stat++;
		r = avg > max_value ? max_value : avg;

/*for g channel */
		avg = *src_g_stat++;
		g = avg > max_value ? max_value : avg;

/*for b channel */
		avg = *src_b_stat++;
		b = avg > max_value ? max_value : avg;

		dst_r[i] = r;
		dst_g[i] = g;
		dst_b[i] = b;
	}

	return rtn;
}

static cmr_s32 _aem_stat_preprocess1(cmr_u32 * src_aem_stat, float *dst_rgb0, float *dst_rgb1, float *dst_rgb2, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift)
{
	cmr_s32 rtn = AE_SUCCESS;
	cmr_u64 bayer_pixels = (cmr_u64)aem_blk_size.w * (cmr_u64)aem_blk_size.h / 4;
	cmr_u32 stat_blocks = aem_blk_num.w * aem_blk_num.h;
	cmr_u32 *src_r_stat = (cmr_u32 *) src_aem_stat;
	cmr_u32 *src_g_stat = (cmr_u32 *) src_aem_stat + stat_blocks;
	cmr_u32 *src_b_stat = (cmr_u32 *) src_aem_stat + (2 * stat_blocks);
	//cmr_u16 *dst_r = (float*)dst_aem_stat;
	//cmr_u16 *dst_g = (float*)dst_aem_stat + stat_blocks;
	//cmr_u16 *dst_b = (float*)dst_aem_stat + stat_blocks * 2;
	double max_value = 1023;
	double avg = 0;
	cmr_u32 i = 0;

	UNUSED(aem_shift);
	if (bayer_pixels < 1)
		return AE_ERROR;

	for (i = 0; i < stat_blocks; i++) {
/*for r channel */
		avg = *src_r_stat++;
		dst_rgb0[i] = avg > max_value ? max_value : avg;

/*for g channel */
		avg = *src_g_stat++;
		dst_rgb1[i] = avg > max_value ? max_value : avg;

/*for b channel */
		avg = *src_b_stat++;
		dst_rgb2[i] = avg > max_value ? max_value : avg;
	}

	return rtn;
}


static int getCenterMean(cmr_u32 * src_aem_stat, float *dst_r, float *dst_g, float *dst_b, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift, float *rmean, float *gmean, float *bmean)
{
	int ret;
	ret = _aem_stat_preprocess2(src_aem_stat, dst_r, dst_g, dst_b, aem_blk_size, aem_blk_num, aem_shift);
	int i;
	int j;
	float rsum = 0;
	float gsum = 0;
	float bsum = 0;
	for (i = 13; i < 19; i++)
		for (j = 13; j < 19; j++) {
			int ind;
			ind = j * 32 + i;
			rsum += dst_r[ind];
			gsum += dst_g[ind];
			bsum += dst_b[ind];
		}
	*rmean = rsum / 36;
	*gmean = gsum / 36;
	*bmean = bsum / 36;
	return ret;
}

static int32 getBlockMean(cmr_u32 *src_aem_stat, uint32 *blkpoints, struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift, float *rgbmean)
{
	int32 ret;
	float dst_rgb[3][1024] = {0};
	ret = _aem_stat_preprocess1(src_aem_stat, &dst_rgb[0][0], &dst_rgb[1][0], &dst_rgb[2][0], aem_blk_size, aem_blk_num, aem_shift);
	uint32 i = 0;
	uint32 j = 0;
	float rsum = 0;
	float gsum = 0;
	float bsum = 0;
	uint32 stx = blkpoints[0];
	uint32 sty = blkpoints[1];
	uint32 endx = blkpoints[2];
	uint32 endy = blkpoints[3];
	ISP_LOGD("FLASH_CALI>P1[%d, %d] P2[%d, %d]\n", stx, sty, endx, endy);
	float radio = 1.0 / ((endy - sty) * (endx - stx));
	for (i = stx; i < endx; i++) {		//13	19
		for (j = sty; j < endy; j++) {
			int ind;
			ind = j * 32 + i;
			rsum += dst_rgb[0][ind];
			gsum += dst_rgb[1][ind];
			bsum += dst_rgb[2][ind];
			//ISP_LOGD("FLASH_CALI>[%f,%f,%f]\n", rsum, gsum, bsum);
		}
	}
	rgbmean[0] = rsum * radio;
	rgbmean[1] = gsum * radio;
	rgbmean[2] = bsum * radio;
	ISP_LOGD("FLASH_CALI>[%f,%f,%f]\n", rgbmean[0], rgbmean[1], rgbmean[2]);
	return ret;
}

static void control_led(struct ae_ctrl_cxt *cxt, int onoff, int isMainflash, int led1, int led2)	//0~31
{
	//UNUSED(cxt);
	UNUSED(onoff);
	UNUSED(isMainflash);
	UNUSED(led1);
	UNUSED(led2);

	ISP_LOGD("control_led %d %d %d %d", onoff, isMainflash, led1, led2);
	int type;
	struct ae_flash_cfg cfg;
	struct ae_flash_element element;
	if (isMainflash == 0)
		type = AE_FLASH_TYPE_PREFLASH;
	else
		type = AE_FLASH_TYPE_MAIN;

	if (onoff == 0) {
		cfg.led_idx = 0;
		cfg.type = type;
		cfg.led0_enable = 0;
		cfg.led1_enable = 0;
		cfg.multiColorLcdEn = cxt->multiColorLcdEn;
		cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
	} else {
		int led1_driver_ind = led1;
		int led2_driver_ind = led2;
		if (led1_driver_ind < 0)
			led1_driver_ind = 0;
		if (led2_driver_ind < 0)
			led2_driver_ind = 0;

		cfg.led_idx = 1;
		cfg.type = type;
		cfg.multiColorLcdEn = cxt->multiColorLcdEn;
		element.index = led1_driver_ind;
		ISP_LOGD("control_led, flash_set_charge1, cfg_type:%d, element_index:%d", cfg.type, element.index);
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);

		if (led2 == -1)
		{
		}
		else
		{
		cfg.led_idx = 2;
		cfg.type = type;
		cfg.multiColorLcdEn = cxt->multiColorLcdEn;
		element.index = led2_driver_ind;
		ISP_LOGD("control_led, flash_set_charge2, cfg_type:%d, element_index:%d", cfg.type, element.index);
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, &cfg, &element);
		}

		if (led1 == -1)
			cfg.led0_enable = 0;
		else
			cfg.led0_enable = 1;

		if (led2 == -1)
			cfg.led1_enable = 0;
		else
			cfg.led1_enable = 1;

		cfg.type = type;
		ISP_LOGD("control_led, flash_ctrl, cfg_type:%d, led0_enable:%d, led1_enable:%d", cfg.type, cfg.led0_enable, cfg.led1_enable);
		usleep(1000*50);
		cxt->isp_ops.flash_ctrl(cxt->isp_ops.isp_handler, &cfg, NULL);
	}

}

static cmr_s32 ae_round(float a)
{
	if (a > 0)
		return (cmr_s32) (a + 0.5);
	else
		return (cmr_s32) (a - 0.5);
}

static void calRgbFrameData(int isMainFlash, float *rRaw, float *gRaw, float *bRaw, float *r, float *g, float *b)
{
	float rs = 0;
	float gs = 0;
	float bs = 0;
	int i;
	if (isMainFlash == 0) {
		for (i = 5; i < 15; i++) {
			rs += rRaw[i];
			gs += gRaw[i];
			bs += bRaw[i];
		}
		rs /= 10;
		gs /= 10;
		bs /= 10;
	} else {
		rs = (rRaw[5] + rRaw[6]) / 2;
		gs = (gRaw[5] + gRaw[6]) / 2;
		bs = (bRaw[5] + bRaw[6]) / 2;
	}
	*r = rs;
	*g = gs;
	*b = bs;

}

static void CalcRgbFrmsData(int frm_st, int frm_ed, float *rRaw, float *gRaw, float *bRaw, float *r, float *g, float *b)
{
	int i= 0;
	float rs = 0.0;
	float gs = 0.0;
	float bs = 0.0;
	float rat_frm_cnts = 1.0 / (frm_ed - frm_st + 1);
	
	ISP_LOGD("frm_ed: %d, frm_st: %d\n", frm_ed, frm_st);
	
	for (i = frm_st; i <= frm_ed; i++) {
		rs += rRaw[i];
		gs += gRaw[i];
		bs += bRaw[i];
	}

	rs *= rat_frm_cnts;
	gs *= rat_frm_cnts;
	bs *= rat_frm_cnts;

	*r = rs;
	*g = gs;
	*b = bs;
}

static void calRgbFrameData1(int frm_st, int frm_ed, float *rRaw, float *gRaw, float *bRaw, float *rgb)
{
	int i = 0;
	rgb[0] = 0.0;
	rgb[1] = 0.0;
	rgb[2] = 0.0;
	for (i = frm_st; i < frm_ed; i++) {
		rgb[0] += rRaw[i];
		rgb[1] += gRaw[i];
		rgb[2] += bRaw[i];
		ISP_LOGD("FLASH_CALI>RGB[%f,%f,%f]\n", rRaw[i],gRaw[i],bRaw[i]);
		ISP_LOGD("FLASH_CALI>[%f,%f,%f]\n", rgb[0], rgb[1], rgb[2]);
	}
	rgb[0] /= (frm_ed - frm_st);
	rgb[1] /= (frm_ed - frm_st);
	rgb[2] /= (frm_ed - frm_st);

	ISP_LOGD("FLASH_CALI>[%d, %d] [%f,%f,%f]\n", frm_st, frm_ed, rgb[0], rgb[1], rgb[2]);
}
static double interp(double x1, double y1, double x2, double y2, double x)
{
	double y;
	if (FLOAT_EQUAL(x1, x2))
		return (y2 + y1) / 2;
	y = y1 + (y2 - y1) / (x2 - x1) * (x - x1);
	return y;
}

//rgTab increasing
static float interpCt(float *rgTab, float *ctTab, int numTab, float rg)
{
	int i;
	if (rg < rgTab[0]) {
		return ctTab[0];
	} else if (rg > rgTab[numTab - 1]) {
		return ctTab[numTab - 1];
	}

	for (i = 1; i < numTab; i++) {
		if (rgTab[i] > rg) {
			return interp(rgTab[i - 1], ctTab[i - 1], rgTab[i], ctTab[i], rg);
		}
	}

	return 10000.0;
}

static void ShellSortFloatIncEx(float *data, int *data_dep, int len)
{
	int i, j;
	double tmp;
	int tmp2;
	int gap = len / 2;
	for (; gap > 0; gap = gap / 2) {
		for (i = gap; i < len; i++) {
			tmp = data[i];
			tmp2 = data_dep[i];
			for (j = i; j >= gap && tmp < data[j - gap]; j -= gap) {
				data[j] = data[j - gap];
				data_dep[j] = data_dep[j - gap];
			}
			data[j] = tmp;
			data_dep[j] = tmp2;
		}
	}
};

static void removeIntArrayElementByInd(int n, int *arr, int ind)
{
	int i;
	for (i = ind; i < n - 1; i++) {
		arr[i] = arr[i + 1];
	}
}

static void removeFloatArrayElementByInd(int n, float *arr, int ind)
{
	int i;
	for (i = ind; i < n - 1; i++) {
		arr[i] = arr[i + 1];
	}
}

static void reduceFlashIndexTab(int n, int *indTab, float *maTab, float maMax, int nMax, int *outIndTab, float *outMaTab, int *outIndTabLen)
{
	nMax -= 1;
	int tempIndTab[256] = {0};
	float tempmADifTab[256] = {0.0};
	float tempmATab[256] = {0.0};
	int i;
	int nCur = 0;
	//memset((void *)&tempIndTab[0], 0, sizeof(tempIndTab));
	for (i = 0; i < n; i++) {
		if (maTab[i] <= maMax) {
			tempIndTab[i] = indTab[i];
			tempmATab[i] = maTab[i];
			nCur = i + 1;
		}
	}
	while (nCur > nMax) {
		int indSort[256] = { 0 };
		for (i = 0; i < nCur - 2; i++) {
			int ind_1;
			int ind_p1;
			ind_1 = tempIndTab[i];
			ind_p1 = tempIndTab[i + 2];
			tempmADifTab[i] = -maTab[ind_1] + maTab[ind_p1];
			indSort[i] = i + 1;
		}
		ShellSortFloatIncEx(tempmADifTab, indSort, nCur - 2);
		removeIntArrayElementByInd(nCur, tempIndTab, indSort[0]);
		removeFloatArrayElementByInd(nCur, tempmATab, indSort[0]);
		nCur -= 1;
	}
	outIndTab[0] = -1;
	outMaTab[0] = 0;
	for (i = 0; i < nCur; i++) {
		outIndTab[i + 1] = tempIndTab[i];
		outMaTab[i + 1] = tempmATab[i];
	}
	*outIndTabLen = nCur + 1;
}

static void readDebugBin2(const char *f, struct FCData *d)
{
	FILE *fp;
	fp = fopen(f, "rb");
	if(fp){
		int sz = (int)fread(d, 1, sizeof(struct FCData), fp);
		if(sz != sizeof(struct FCData))
		        ISP_LOGE("readDebugBin2 fread faild!");
		fclose(fp);
		fp = NULL;
	}
}

static void readFCConfig(char *f, struct FCData *d, char *fout)
{
	int i;
	FILE *fp = NULL;
	if(f && d){
        fp = fopen(f, "r+");
        if (fp) {
            fscanf(fp, "%d", &d->numP1_hw);
            fscanf(fp, "%d", &d->numP2_hw);
            fscanf(fp, "%d", &d->numM1_hw);
            fscanf(fp, "%d", &d->numM2_hw);

            fscanf(fp, "%d", &d->numP1_hwSample);
            for (i = 0; i < d->numP1_hwSample; i++)
                fscanf(fp, "%d", &d->indP1_hwSample[i]);
            for (i = 0; i < d->numP1_hwSample; i++)
                fscanf(fp, "%f", &d->maP1_hwSample[i]);

            fscanf(fp, "%d", &d->numP2_hwSample);
            for (i = 0; i < d->numP2_hwSample; i++)
                fscanf(fp, "%d", &d->indP2_hwSample[i]);
            for (i = 0; i < d->numP2_hwSample; i++)
                fscanf(fp, "%f", &d->maP2_hwSample[i]);

            fscanf(fp, "%d", &d->numM1_hwSample);
            for (i = 0; i < d->numM1_hwSample; i++)
                fscanf(fp, "%d", &d->indM1_hwSample[i]);
            for (i = 0; i < d->numM1_hwSample; i++)
                fscanf(fp, "%f", &d->maM1_hwSample[i]);

            fscanf(fp, "%d", &d->numM2_hwSample);
            for (i = 0; i < d->numM2_hwSample; i++)
                fscanf(fp, "%d", &d->indM2_hwSample[i]);
            for (i = 0; i < d->numM2_hwSample; i++)
                fscanf(fp, "%f", &d->maM2_hwSample[i]);

            fscanf(fp, "%f", &d->mAMaxP1);
            fscanf(fp, "%f", &d->mAMaxP2);
            fscanf(fp, "%f", &d->mAMaxP12);
            fscanf(fp, "%f", &d->mAMaxM1);
            fscanf(fp, "%f", &d->mAMaxM2);
            fscanf(fp, "%f", &d->mAMaxM12);
  			fscanf(fp, "%d", &d->pf_st);
			fscanf(fp, "%d", &d->pf_ed);
			fscanf(fp, "%d", &d->pf_total);
			fscanf(fp, "%d", &d->mf_st);
			fscanf(fp, "%d", &d->mf_ed);
			fscanf(fp, "%d", &d->mf_total);
            fclose(fp);
            fp = NULL;
        }
		ISP_LOGD("PENG>pre_flash start: %d, end: %d, total: %d\n", d->pf_st, d->pf_ed, d->pf_total);
		ISP_LOGD("PENG>main_flash start: %d, end: %d, total: %d\n", d->mf_st, d->mf_ed, d->mf_total);
    }

	if (fout != 0 && d) {
        fp = fopen(fout, "w+");
        if (NULL != fp) {
            fprintf(fp, "%d\n", d->numP1_hw);
            fprintf(fp, "%d\n", d->numP2_hw);
            fprintf(fp, "%d\n", d->numM1_hw);
            fprintf(fp, "%d\n", d->numM2_hw);

            fprintf(fp, "%d\n", d->numP1_hwSample);
            for (i = 0; i < d->numP1_hwSample; i++)
                fprintf(fp, "%d\t", d->indP1_hwSample[i]);
            fprintf(fp, "\n");
            for (i = 0; i < d->numP1_hwSample; i++)
                fprintf(fp, "%f\t", d->maP1_hwSample[i]);
            fprintf(fp, "\n");

            fprintf(fp, "%d\n", d->numP2_hwSample);
            for (i = 0; i < d->numP2_hwSample; i++)
                fprintf(fp, "%d\t", d->indP2_hwSample[i]);
            fprintf(fp, "\n");
            for (i = 0; i < d->numP2_hwSample; i++)
                fprintf(fp, "%f\t", d->maP2_hwSample[i]);
            fprintf(fp, "\n");

            fprintf(fp, "%d\n", d->numM1_hwSample);
            for (i = 0; i < d->numM1_hwSample; i++)
                fprintf(fp, "%d\t", d->indM1_hwSample[i]);
            fprintf(fp, "\n");
            for (i = 0; i < d->numM1_hwSample; i++)
                fprintf(fp, "%f\t", d->maM1_hwSample[i]);
            fprintf(fp, "\n");

            fprintf(fp, "%d\n", d->numM2_hwSample);
            for (i = 0; i < d->numM2_hwSample; i++)
                fprintf(fp, "%d\t", d->indM2_hwSample[i]);
            fprintf(fp, "\n");
            for (i = 0; i < d->numM2_hwSample; i++)
                fprintf(fp, "%f\t", d->maM2_hwSample[i]);
            fprintf(fp, "\n");

            fprintf(fp, "%f\n", d->mAMaxP1);
            fprintf(fp, "%f\n", d->mAMaxP2);
            fprintf(fp, "%f\n", d->mAMaxP12);
            fprintf(fp, "%f\n", d->mAMaxM1);
            fprintf(fp, "%f\n", d->mAMaxM2);
            fprintf(fp, "%f\n", d->mAMaxM12);
			fprintf(fp, "%d\n", d->pf_st);
			fprintf(fp, "%d\n", d->pf_ed);
			fprintf(fp, "%d\n", d->pf_total);
			fprintf(fp, "%d\n", d->mf_st);
			fprintf(fp, "%d\n", d->mf_ed);
			fprintf(fp, "%d\n", d->pf_total);
            fclose(fp);
            fp = NULL;
        }

	}

}

struct flash_led_brightness1 {
	int32 LED_enable;	
	uint32 levelNumber_pf1;
	uint32 levelNumber_pf2;
	uint32 levelNumber_mf1;
	uint32 levelNumber_mf2;
	uint32 ledBrightness_pf1[32];
	uint32 ledBrightness_pf2[32];
	uint32 ledBrightness_mf1[32];
	uint32 ledBrightness_mf2[32];
	uint32 index_pf1[32];
	uint32 index_pf2[32];
	uint32 index_mf1[32];
	uint32 index_mf2[32];
};

struct flash_calibration_data1 {
	uint32 flashMask[9];
	uint32 brightnessTable[9];
	uint32 rTable[9];		//g: 1024
	uint32 bTable[9];
	uint32 preflashBrightness[9];
	uint32 rTable_preflash[9];

	float rgRatPf;

	uint32 rPf1[3];
	uint32 gPf1[3];
	uint32 rPf2[3];
	uint32 gPf2[3];
};

struct FCData1 {
	//config
	uint32 startID;
	uint32 LED_enable;	//0:LED1+LED2	1:LED1	2:LED2
	uint32 numP1_hw;
	uint32 numP2_hw;
	uint32 numM1_hw;
	uint32 numM2_hw;	//32
	
	uint32 numP1_hwSample;			//num:3	low/mid/high
	uint32 indP1_hwSample[3];	//LED1 sample index: 3
	float maP1_hwSample[3];	//LED1 sample value:

	uint32 numP2_hwSample;
	uint32 indP2_hwSample[3];
	float maP2_hwSample[3];

	uint32 numM1_hwSample;
	uint32 indM1_hwSample[3];
	float maM1_hwSample[3];

	uint32 numM2_hwSample;
	uint32 indM2_hwSample[3];
	float maM2_hwSample[3];

	float mAMaxP1;		//200
	float mAMaxP2;
	float mAMaxP12;
	float mAMaxM1;		//1000
	float mAMaxM2;
	float mAMaxM12;
	

	//catch frame
	uint32 prefm[3];
	uint32 mainfm[3];

	//precision of light/color
	float fourcorners[2];					//flash on [0]:brightness	[1]:color
	float around2center[2];				//
	uint32 endID;

	uint32 expTime;
	uint32 gain;
	uint32 expTimeBase;
	uint32 gainBase;
	/*uint32 nextexpTime;
	uint32 nextgain;
	*/
	
	uint32 testIndAll;
	uint32 testInd;

	uint32 isMainTab[16];	//¡ê¡§3+1¡ê?*4
	int32 ind1Tab[16];
	int32 ind2Tab[16];
	uint32 testMinFrm[16];
	uint32 expReset[16];

	uint32 expTab[16];
	uint32 gainTab[16];

	float rData[16];
	float gData[16];
	float bData[16];

	uint32 stateAeFrameCntSt;
	uint32 stateCaliFrameCntSt;
	uint32 stateCaliFrameCntStSub;

	float rFrame[15];
	float gFrame[15];
	float bFrame[15];
	float rgb[3];

	struct flash_calibration_data1 out;
};

static void write2flashcali(cmr_s8 camID, struct flash_correct_info *result)
{
	FILE *fp = NULL;
	char filename[128] = {0};
	sprintf(filename, "%s%d%s", "/data/vendor/cameraserver/flash_cali_out_", camID, ".txt");
	ISP_LOGD("FLASH_CALI>%s\n", filename);
	fp = fopen(filename, "wt");
	if(fp){
		uint32 i = 0;
		uint32 j = 0;
		//flash config
		//LED1 pre
		fprintf(fp, "%d\n", result->numP1_hwSample);
		for (i = 0; i < result->numP1_hwSample; i++)
			fprintf(fp, "%d\t", result->indP1_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < result->numP1_hwSample; i++)
			fprintf(fp, "%f\t", result->maP1_hwSample[i]);
		fprintf(fp, "\n");
		//LED2 pre
		fprintf(fp, "%d\n", result->numP2_hwSample);
		for (i = 0; i < result->numP2_hwSample; i++)
			fprintf(fp, "%d\t", result->indP2_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < result->numP2_hwSample; i++)
			fprintf(fp, "%f\t", result->maP2_hwSample[i]);
		fprintf(fp, "\n");
		//LED1 main
		fprintf(fp, "%d\n", result->numM1_hwSample);
		for (i = 0; i < result->numM1_hwSample; i++)
			fprintf(fp, "%d\t", result->indM1_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < result->numM1_hwSample; i++)
			fprintf(fp, "%f\t", result->maM1_hwSample[i]);
		fprintf(fp, "\n");
		//LED2 main
		fprintf(fp, "%d\n", result->numM2_hwSample);
		for (i = 0; i < result->numM2_hwSample; i++)
			fprintf(fp, "%d\t", result->indM2_hwSample[i]);
		fprintf(fp, "\n");
		for (i = 0; i < result->numM2_hwSample; i++)
			fprintf(fp, "%f\t", result->maM2_hwSample[i]);
		fprintf(fp, "\n");
		
		//flash cali out
		//LED1+pre:
		//fprintf(fp, "%d\n", result->numP1_hwSample);
		for (i = 0; i < result->numP1_hwSample; i++){
			//fprintf(fp, "ID:%d\t", i);
			for(j = 0; j < 3; j++)
				fprintf(fp, "%f\t", result->P1rgbData[i][j]);
			fprintf(fp, "%d\t", result->P1expgain[i][0]);
			fprintf(fp, "%d\n", result->P1expgain[i][1]);
		}

		//LED2+pre:
		//fprintf(fp, "%d\n", result->numP2_hwSample);
		for (i = 0; i < result->numP2_hwSample; i++){
			//fprintf(fp, "ID:%d\t", i);
			for(j = 0; j < 3; j++)
				fprintf(fp, "%f\t", result->P2rgbData[i][j]);
			fprintf(fp, "%d\t", result->P2expgain[i][0]);
			fprintf(fp, "%d\n", result->P2expgain[i][1]);
		}
		fprintf(fp, "\n");

		//LED1+main:
		//fprintf(fp, "%d\n", result->numM1_hwSample);
		for (i = 0; i < result->numM1_hwSample; i++){
			//fprintf(fp, "ID:%d\t", i);
			for(j = 0; j < 3; j++)
				fprintf(fp, "%f\t", result->M1rgbData[i][j]);
			fprintf(fp, "%d\t", result->M1expgain[i][0]);
			fprintf(fp, "%d\n", result->M1expgain[i][1]);
		}
		//LED2+main:
		//fprintf(fp, "%d\n", result->numM2_hwSample);
		for (i = 0; i < result->numM2_hwSample; i++){
			//fprintf(fp, "ID:%d\t", i);
			for(j = 0; j < 3; j++)
				fprintf(fp, "%f\t", result->M2rgbData[i][j]);
			fprintf(fp, "%d\t", result->M2expgain[i][0]);
			fprintf(fp, "%d\n", result->M2expgain[i][1]);
		}
		fclose(fp);
	}
	fp = NULL;
}

static void write2FCConfig(struct FCData1 *cali, struct flash_correct_info *result)
{
	int i = 0;
	result->numP1_hw = cali->numP1_hw;
	result->numP2_hw = cali->numP2_hw;
	result->numM1_hw = cali->numM1_hw;
	result->numM2_hw = cali->numM2_hw;
	
	result->numP1_hwSample = cali->numP1_hwSample;
	result->numP2_hwSample = cali->numP2_hwSample;
	result->numM1_hwSample = cali->numM1_hwSample;
	result->numM2_hwSample = cali->numM2_hwSample;

	result->mAMaxP1 = cali->mAMaxP1;
	result->mAMaxP2 = cali->mAMaxP2;
	result->mAMaxP12 = cali->mAMaxP12;
	result->mAMaxM1 = cali->mAMaxM1;
	result->mAMaxM2 = cali->mAMaxM2;
	result->mAMaxM12 = cali->mAMaxM12;

	for(i = 0; i < cali->numP1_hwSample; i++){
		result->indP1_hwSample[i] = cali->indP1_hwSample[i];	//LED1 sample index: 3
		result->maP1_hwSample[i] = cali->maP1_hwSample[i];
	}
	ISP_LOGD("FLASH_CALI>LED1+pre:[%d: %f,%f,%f]\n", cali->numP1_hwSample, result->maP1_hwSample[0], result->maP1_hwSample[1], result->maP1_hwSample[2]);
	for(i = 0; i < cali->numP2_hwSample; i++){
		result->indP2_hwSample[i] = cali->indP2_hwSample[i];	//LED1 sample index: 3
		result->maP2_hwSample[i] = cali->maP2_hwSample[i];
	}
	ISP_LOGD("FLASH_CALI>LED2+pre:[%d: %f,%f,%f]\n", cali->numP2_hwSample, result->maP2_hwSample[0], result->maP2_hwSample[1], result->maP2_hwSample[2]);
	for(i = 0; i < cali->numM1_hwSample; i++){
		result->indM1_hwSample[i] = cali->indM1_hwSample[i];	//LED1 sample index: 3
		result->maM1_hwSample[i] = cali->maM1_hwSample[i];
	}
	ISP_LOGD("FLASH_CALI>LED1+main:[%d: %f,%f,%f]\n", cali->numM1_hwSample, result->maM1_hwSample[0], result->maM1_hwSample[1], result->maM1_hwSample[2]);
	for(i = 0; i < cali->numM2_hwSample; i++){
		result->indM2_hwSample[i] = cali->indM2_hwSample[i];	//LED1 sample index: 3
		result->maM2_hwSample[i] = cali->maM2_hwSample[i];
	}
	ISP_LOGD("FLASH_CALI>LED2+main:[%d: %f,%f,%f]\n", cali->numM2_hwSample, result->maM2_hwSample[0], result->maM2_hwSample[1], result->maM2_hwSample[2]);
	for(i = 0; i < 3; i++){
		result->prefm[i] = cali->prefm[i];
		result->mainfm[i] = cali->mainfm[i];
	}
	for(i = 0; i < 2; i++){
		result->fourcorners[i] = cali->fourcorners[i];
		result->around2center[i] = cali->around2center[i];
	}
}

static void readFCConfig1(char *f, struct FCData1 *d)
{
	if(d == NULL){
		ISP_LOGI("FCData1 malloc is fail");
		return;
	}
	int32 i = 0;
	FILE *fp = NULL;
	if(f){
        fp = fopen(f, "rt");
        if (fp) {
			fscanf(fp, "%d", &d->LED_enable);
	      	fscanf(fp, "%d", &d->numP1_hw);		//hardware hold up num of LED
	        fscanf(fp, "%d", &d->numP2_hw);
	        fscanf(fp, "%d", &d->numM1_hw);
	        fscanf(fp, "%d", &d->numM2_hw);

	        fscanf(fp, "%d", &d->numP1_hwSample);	//LED1 pre_sample index num
	        for (i = 0; i < d->numP1_hwSample; i++)
				fscanf(fp, "%d", &d->indP1_hwSample[i]);	//index 0->3
	        for (i = 0; i < d->numP1_hwSample; i++)
				fscanf(fp, "%f", &d->maP1_hwSample[i]);		//pre led level

	        fscanf(fp, "%d", &d->numP2_hwSample);	//
	        for (i = 0; i < d->numP2_hwSample; i++)
				fscanf(fp, "%d", &d->indP2_hwSample[i]);
	        for (i = 0; i < d->numP2_hwSample; i++)
				fscanf(fp, "%f", &d->maP2_hwSample[i]);

	        fscanf(fp, "%d", &d->numM1_hwSample);
	        for (i = 0; i < d->numM1_hwSample; i++)
				fscanf(fp, "%d", &d->indM1_hwSample[i]);
	        for (i = 0; i < d->numM1_hwSample; i++)
				fscanf(fp, "%f", &d->maM1_hwSample[i]);		//main led level

	        fscanf(fp, "%d", &d->numM2_hwSample);
	        for (i = 0; i < d->numM2_hwSample; i++)
				fscanf(fp, "%d", &d->indM2_hwSample[i]);
	        for (i = 0; i < d->numM2_hwSample; i++)
				fscanf(fp, "%f", &d->maM2_hwSample[i]);

	        fscanf(fp, "%f", &d->mAMaxP1);		//pre:	LED1 MAX
	        fscanf(fp, "%f", &d->mAMaxP2);		//		LED2 MAX
	        fscanf(fp, "%f", &d->mAMaxP12);		//		LED1+LED2 MAX
	        fscanf(fp, "%f", &d->mAMaxM1);		//main:	LED1 MAX
	        fscanf(fp, "%f", &d->mAMaxM2);		//		LED2 MAX
	        fscanf(fp, "%f", &d->mAMaxM12);		//		LED1+LED2 MAX
			
			//catch frame
			fscanf(fp, "%d", &d->prefm[0]);
			fscanf(fp, "%d", &d->prefm[1]);
			fscanf(fp, "%d", &d->prefm[2]);
			fscanf(fp, "%d", &d->mainfm[0]);
			fscanf(fp, "%d", &d->mainfm[1]);
			fscanf(fp, "%d", &d->mainfm[2]);
			//
			fscanf(fp, "%f", &d->fourcorners[0]);
			fscanf(fp, "%f", &d->fourcorners[1]);
			fscanf(fp, "%f", &d->around2center[0]);
			fscanf(fp, "%f", &d->around2center[1]);

	        fclose(fp);
	        fp = NULL;
        }
	}
}

static void readFCtuneparam(struct flash_correct_info *cf_info, struct FCData1 *cali){
	uint32 i = 0;
	cali->LED_enable = cf_info->LED_enable;

	cali->numP1_hw = cf_info->numP1_hw;
	cali->numP2_hw = cf_info->numP2_hw;
	cali->numM1_hw = cf_info->numM1_hw;
	cali->numM2_hw = cf_info->numM2_hw;
	ISP_LOGD("FLASH_CALI>[%d,%d,%d,%d]", cali->numP1_hw, cali->numP2_hw, cali->numM1_hw, cali->numM2_hw);
	cali->numP1_hwSample = cf_info->numP1_hwSample;
	for(i = 0; i < cali->numP1_hwSample; i++){
		cali->indP1_hwSample[i] = cf_info->indP1_hwSample[i];
		cali->maP1_hwSample[i] = cf_info->maP1_hwSample[i];
		ISP_LOGD("FLASH_CALI>[%d,%f]", cali->indP1_hwSample[i], cali->maP1_hwSample[i]);
	}
	ISP_LOGD("FLASH_CALI>[%d,%d,%d,%d]", cf_info->numP1_hwSample, cf_info->numP2_hwSample, cf_info->numM1_hwSample, cf_info->numM2_hwSample);
	cali->numP2_hwSample = cf_info->numP2_hwSample;
	for(i = 0; i < cali->numP2_hwSample; i++){
		cali->indP2_hwSample[i] = cf_info->indP2_hwSample[i];
		cali->maP2_hwSample[i] = cf_info->maP2_hwSample[i];
		ISP_LOGD("FLASH_CALI>[%d,%f]", cali->indP2_hwSample[i], cali->maP2_hwSample[i]);
	}
	
	cali->numM1_hwSample = cf_info->numM1_hwSample;
	for(i = 0; i < cali->numM1_hwSample; i++){
		cali->indM1_hwSample[i] = cf_info->indM1_hwSample[i];
		cali->maM1_hwSample[i] = cf_info->maM1_hwSample[i];
		ISP_LOGD("FLASH_CALI>[%d,%f]", cali->indM1_hwSample[i], cali->maM1_hwSample[i]);
	}
	
	cali->numM2_hwSample = cf_info->numM2_hwSample;
	for(i = 0; i < cali->numM2_hwSample; i++){
		cali->indM2_hwSample[i] = cf_info->indM2_hwSample[i];
		cali->maM2_hwSample[i] = cf_info->maM2_hwSample[i];
		ISP_LOGD("FLASH_CALI>[%d,%f]", cali->indM2_hwSample[i], cali->maM2_hwSample[i]);
	}

	cali->mAMaxP1 = cf_info->mAMaxP1;
	cali->mAMaxP2 = cf_info->mAMaxP2;
	cali->mAMaxP12 = cf_info->mAMaxP12;
	cali->mAMaxM1 = cf_info->mAMaxM1;
	cali->mAMaxM2 = cf_info->mAMaxM2;
	cali->mAMaxM12 = cf_info->mAMaxM12;

	memcpy(&cali->prefm[0], &cf_info->prefm[0], 3 * sizeof(cali->prefm[0]));
	memcpy(&cali->mainfm[0], &cf_info->mainfm[0], 3 * sizeof(cali->mainfm[0]));
	ISP_LOGD("FLASH_CALI>pre[%d,%d,%d]", cali->prefm[0], cali->prefm[1], cali->prefm[2]);
	ISP_LOGD("FLASH_CALI>main[%d,%d,%d]", cali->mainfm[0], cali->mainfm[1], cali->mainfm[2]);
	memcpy(&cali->fourcorners[0], &cf_info->fourcorners[0], 2 * sizeof(cali->fourcorners[0]));
	memcpy(&cali->around2center[0], &cf_info->around2center[0], 2 * sizeof(cali->around2center[0]));
	ISP_LOGD("FLASH_CALI>fourcornes[%f,%f]", cali->fourcorners[0], cali->fourcorners[1]);
	ISP_LOGD("FLASH_CALI>around2center[%f,%f]", cali->around2center[0], cali->around2center[1]);
}

static void writedata(float *y, float *deta, float *f2c, float *a2c){
	FILE *fp = NULL;
	char filename[128] = {"\0"};
	sprintf(filename, "%s%s", "/data/vendor/cameraserver/flash_varify", ".txt");	//result->camID
	ISP_LOGD("FLASH_CALI>%s\n", filename);
	fp = fopen(filename, "wt");
	int i = 0;
	if(fp){
		fprintf(fp, "%s\n", "brighness!");
		for (i = 0; i < 5; i++)
			fprintf(fp, "%f\t", y[i]);
		fprintf(fp, "\n");
		fprintf(fp, "%s\n", "color deta!");
		for (i = 0; i < 5; i++)
			fprintf(fp, "%f\t", deta[i]);
		fprintf(fp, "\n");
		fprintf(fp, "%s\n", "brighness precision!");
		for (i = 0; i < 2; i++)
			fprintf(fp, "%f\t", f2c[i]);
		fprintf(fp, "\n");
		fprintf(fp, "%s\n", "color precision!");
		for (i = 0; i < 2; i++)
			fprintf(fp, "%f\t", a2c[i]);
		fprintf(fp, "\n");
		fclose(fp);
	}
	fp = NULL;
}

// RGB to YUV
#define RGB2Y(_r, _g, _b)	(0.257 * (_r) + 0.504 * (_g) + 0.098 * (_b))		//0.257R+0.504G+0.098B
#define MAX_F(_x, _y)	(((_x) > (_y)) ? (_x) : (_y))

static uint8 flash_varify(cmr_u32 *src_aem_stat, float *fourcorners, float *around2center, char *error_info,
		struct ae_size aem_blk_size, struct ae_size aem_blk_num, cmr_u8 aem_shift)
{
	float rgbmean[5][3];	//4 corner & center r/g/b
	float y[5] = {0.0};
	float r2g[5] = {0.0};
	float b2g[5] = {0.0};
	float detarb2g[5] = {0.0};
	uint32 blkpoint[5][4] = {{0, 0, 6, 6},{26, 0, 32, 6},{0, 26, 6, 32},{26, 26, 32, 32},{13, 13, 19, 19}};

	//left2top
	getBlockMean(src_aem_stat, &blkpoint[0][0], aem_blk_size, aem_blk_num, aem_shift, &rgbmean[0][0]);
	y[0] = RGB2Y(rgbmean[0][0], rgbmean[0][1], rgbmean[0][2]);
	if(!FLOAT_EQUAL(rgbmean[0][1], 0.0)){
		r2g[0] = rgbmean[0][0] / rgbmean[0][1];
		b2g[0] = rgbmean[0][2] / rgbmean[0][1];
		detarb2g[0] = sqrt(r2g[0] * r2g[0] + b2g[0] * b2g[0]);
	} else {
		sprintf(error_info, "%s", "Too far away, too little reflected light");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_LONG_DISTANCE;
	}
	//right2top
	getBlockMean(src_aem_stat, &blkpoint[1][0], aem_blk_size, aem_blk_num, aem_shift, &rgbmean[1][0]);
	y[1] = RGB2Y(rgbmean[1][0], rgbmean[1][1], rgbmean[1][2]);
	if(!FLOAT_EQUAL(rgbmean[1][1], 0.0)){
		r2g[1] = rgbmean[1][0] / rgbmean[1][1];
		b2g[1] = rgbmean[1][2] / rgbmean[1][1];
		detarb2g[1] = sqrt(r2g[1] * r2g[1] + b2g[1] * b2g[1]);
	} else {
		sprintf(error_info, "%s", "Too far away, too little reflected light");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_LONG_DISTANCE;
	}
	//left2bottom
	getBlockMean(src_aem_stat, &blkpoint[2][0], aem_blk_size, aem_blk_num, aem_shift, &rgbmean[2][0]);
	y[2] = RGB2Y(rgbmean[2][0], rgbmean[2][1], rgbmean[2][2]);
	if(!FLOAT_EQUAL(rgbmean[2][1], 0.0)){
		r2g[2] = rgbmean[2][0] / rgbmean[2][1];
		b2g[2] = rgbmean[2][2] / rgbmean[2][1];
		detarb2g[2] = sqrt(r2g[2] * r2g[2] + b2g[2] * b2g[2]);
	} else {
		sprintf(error_info, "%s", "Too far away, too little reflected light");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_LONG_DISTANCE;
	}

	//right2bottom
	getBlockMean(src_aem_stat, &blkpoint[3][0], aem_blk_size, aem_blk_num, aem_shift, &rgbmean[3][0]);
	y[3] = RGB2Y(rgbmean[3][0], rgbmean[3][1], rgbmean[3][2]);
	if(!FLOAT_EQUAL(rgbmean[3][1], 0.0)){
		r2g[3] = rgbmean[3][0] / rgbmean[3][1];
		b2g[3] = rgbmean[3][2] / rgbmean[3][1];
		detarb2g[3] = sqrt(r2g[3] * r2g[3] + b2g[3] * b2g[3]);
	} else {
		sprintf(error_info, "%s", "Too far away, too little reflected light");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_LONG_DISTANCE;
	}
	//center
	getBlockMean(src_aem_stat, &blkpoint[4][0], aem_blk_size, aem_blk_num, aem_shift, &rgbmean[4][0]);
	y[4] = RGB2Y(rgbmean[4][0], rgbmean[4][1], rgbmean[4][2]);
	if(!FLOAT_EQUAL(rgbmean[4][1], 0.0)){
		r2g[4] = rgbmean[4][0] / rgbmean[4][1];
		b2g[4] = rgbmean[4][2] / rgbmean[4][1];
		detarb2g[4] = sqrt(r2g[4] * r2g[4] + b2g[4] * b2g[4]);
	} else {
		sprintf(error_info, "%s", "Too far away, too little reflected light");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_LONG_DISTANCE;
	}	
	ISP_LOGD("FLASH_CALI>rgb[%f %f %f] [%f %f]",rgbmean[4][0],rgbmean[4][1],rgbmean[4][2], r2g[4],b2g[4]);
	writedata(&y[0], &detarb2g[0], &fourcorners[0], &around2center[0]);
	//four corners diff of brightness
	float cnrMax = 0.0;
	float tmpMax1 = 0.0;
	float tmpMax2 = 0.0;
	float cnr2ctrMax = 0.0;
	if ((((int)(y[0] - y[3])) ^ ((int)(y[1] - y[2]))) < 0){		//x^y	is diff sign
		cnrMax = MAX_F(fabs(y[0] - y[1]), fabs(y[0] - y[1]));
	} else {
		cnrMax = MAX_F(fabs(y[0] - y[3]), fabs(y[1] - y[2]));
	}
	//corner & center diff of brightness
	tmpMax1 = MAX_F(fabs(y[4] - y[0]), fabs(y[4] - y[1]));
	tmpMax2 = MAX_F(fabs(y[4] - y[2]), fabs(y[4] - y[3]));
	cnr2ctrMax = MAX_F(tmpMax1, tmpMax2);
	ISP_LOGD("FLASH_CALI>[%f,%f,%f,%f  %f][%f,%f]\n",y[0],y[1],y[2],y[3],y[4], tmpMax1,tmpMax2);
	ISP_LOGD("FLASH_CALI>BRIGHTNESS:COR[%f, %f] CTR[%f, %f]\n", cnrMax, fourcorners[0], cnr2ctrMax, around2center[0]);

	if (fourcorners[0] < cnrMax){
		sprintf(error_info, "%s", "The brightness difference between the four corners is too large");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_BRI_FOUR_COR;
	} else if (around2center[0] < cnr2ctrMax) {
		sprintf(error_info, "%s", "The brightness difference between the four corners and the center is too large");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_BRI_COR2CTR;
	}
	//four corners diff of colorness
	if ((((int)(detarb2g[0] - detarb2g[3])) ^ ((int)(detarb2g[1] - detarb2g[2]))) < 0){		//x^y	is diff sign
		cnrMax = MAX_F(fabs(detarb2g[0] - detarb2g[1]), fabs(detarb2g[0] - detarb2g[1]));
	} else {
		cnrMax = MAX_F(fabs(detarb2g[0] - detarb2g[3]), fabs(detarb2g[1] - detarb2g[2]));
	}
	//corner & center diff of colorness
	tmpMax1 = MAX_F(fabs(detarb2g[4] - detarb2g[0]), fabs(detarb2g[4] - detarb2g[1]));
	tmpMax2 = MAX_F(fabs(detarb2g[4] - detarb2g[2]), fabs(detarb2g[4] - detarb2g[3]));
	cnr2ctrMax = MAX_F(tmpMax1, tmpMax2);
	ISP_LOGD("FLASH_CALI>[%f,%f,%f,%f  %f][%f,%f]\n", detarb2g[0],detarb2g[1],detarb2g[2],detarb2g[3], detarb2g[4], tmpMax1,tmpMax2);
	ISP_LOGD("FLASH_CALI>COLOR:COR[%f, %f] CTR[%f, %f]\n", cnrMax, fourcorners[1], cnr2ctrMax, around2center[1]);

	if (fourcorners[1] < cnrMax){
		sprintf(error_info, "%s", "The color difference between the four corners is too large");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_CLR_FOUR_COR;
	} else if (around2center[1] < cnr2ctrMax) {
		sprintf(error_info, "%s", "The color difference between the four corners and the center is too large");
		ISP_LOGD("FLASH_CALI>%s\n", error_info);
		return FLASH_CALI_CLR_COR2CTR;
	}
	return FLASH_CALI_DOING;
}
		
static void write2brigthness(struct FCData1 *caliData){

	uint32 i = 0;
	uint32 j = 0;
	float rTab1[3] = {0.0};	//LED1+pre	//id:0/9/19
	float gTab1[3] = {0.0};
	float bTab1[3] = {0.0};
	float rTab2[3] = {0.0};	//LED2+pre
	float gTab2[3] = {0.0};
	float bTab2[3] = {0.0};
	
	float rTab1Main[3] = {0.0};	//LED1+main
	float gTab1Main[3] = {0.0};
	float bTab1Main[3] = {0.0};
	float rTab2Main[3] = {0.0};	//LED2+main
	float gTab2Main[3] = {0.0};
	float bTab2Main[3] = {0.0};
	float radio = 1.0 / 65535;
		
	float maxV = 0.0;
	uint32 LED1_pre_id = 0;
	uint32 LED2_pre_id = 0;
	uint32 LED1_main_id = 0;
	uint32 LED2_main_id = 0;
	for (i = 0; i < caliData->testIndAll; i++) {
		double rat = (double)caliData->expTimeBase * caliData->gainBase / caliData->gainTab[i] / caliData->expTab[i];

		//int32 id1 = caliData->ind1Tab[i];
		//int32 id2 = caliData->ind2Tab[i];

		float r = caliData->rData[i] * rat - caliData->rData[0];
		float g = caliData->gData[i] * rat - caliData->gData[0];
		float b = caliData->bData[i] * rat - caliData->bData[0];

		if (r < 0.0 || g < 0.0 || b < 0.0) {
			r = 0.0;
			g = 0.0;
			b = 0.0;
		}

		if (maxV < g)
			maxV = g;
		
		if (caliData->ind1Tab[i] != -1 && caliData->isMainTab[i] 
			&& i < caliData->numP1_hwSample + caliData->numP2_hwSample + caliData->numM1_hwSample + 3
			&& i > caliData->numP1_hwSample + caliData->numP2_hwSample + 2) {
			rTab1Main[LED1_main_id] = r;
			gTab1Main[LED1_main_id] = g;
			bTab1Main[LED1_main_id] = b;
			LED1_main_id++;
			ISP_LOGD("PENG_END:LED1_main %d\n", LED1_main_id);
		} else if (caliData->ind1Tab[i] != -1 && caliData->isMainTab[i] == 0 
			&& i < caliData->numP1_hwSample + 1 && i > 0) {
			rTab1[LED1_pre_id] = r;
			gTab1[LED1_pre_id] = g;
			bTab1[LED1_pre_id] = b;
			LED1_pre_id++;
			ISP_LOGD("PENG_END:LED1_pre: %d\n", LED1_pre_id);
		} else if (caliData->ind2Tab[i] != -1 && caliData->isMainTab[i] 
			&& i < caliData->numP1_hwSample + caliData->numP2_hwSample + caliData->numM1_hwSample + caliData->numM2_hwSample + 4
			&& i > caliData->numP1_hwSample + caliData->numP2_hwSample + caliData->numM1_hwSample + 3) {
			rTab2Main[LED2_main_id] = r;
			gTab2Main[LED2_main_id] = g;
			bTab2Main[LED2_main_id] = b;
			LED2_main_id++;
			ISP_LOGD("PENG_END:LED2_main: %d\n", LED2_main_id);
		} else if (caliData->ind2Tab[i] != -1 && caliData->isMainTab[i] == 0 
			&& i < caliData->numP1_hwSample + caliData->numP2_hwSample + 2
			&& i > caliData->numP1_hwSample + 1) {
			rTab2[LED2_pre_id] = r;
			gTab2[LED2_pre_id] = g;
			bTab2[LED2_pre_id] = b;
			LED2_pre_id++;
			ISP_LOGD("PENG_END:LED2_pre: %d\n", LED2_pre_id);
		} else {
			ISP_LOGD("PENG_END>MASK for LED1 to LED2 ID: %d\n", i);
		}
		ISP_LOGD("PENG_END>%f [%d,%d]", rat, caliData->expTab[i], caliData->gainTab[i]);
		ISP_LOGD("PENG_END>ID:%d[%f,%f,%f]\n", i, r, g, b);
	}
	double sc = 0.0;
	if (maxV > 0.0) {
		sc = 65536.0 / maxV / 2;
	} else {
		ISP_LOGW("maxV is invalidated, %f", maxV);
	}
	for (i = 0; i < 3; i++) {
		rTab1[i] *= sc;		//LED1+pre
		gTab1[i] *= sc;
		bTab1[i] *= sc;
		rTab2[i] *= sc;
		gTab2[i] *= sc;
		bTab2[i] *= sc;
		rTab1Main[i] *= sc;		//LED1+main
		gTab1Main[i] *= sc;
		bTab1Main[i] *= sc;
		rTab2Main[i] *= sc;
		gTab2Main[i] *= sc;
		bTab2Main[i] *= sc;
	}
	
	//Preflash ct
	//preflash brightness
	float maxRg = 0;
	for (i = 0; i < caliData->numP1_hwSample; i++) {
		if (maxRg < gTab1[i])
			maxRg = gTab1[i];
		if (maxRg < rTab1[i])
			maxRg = rTab1[i];
	}
	for (i = 0; i < caliData->numP2_hwSample; i++) {
		if (maxRg < gTab2[i])
			maxRg = gTab2[i];
		if (maxRg < rTab2[i])
			maxRg = rTab2[i];
	}
	caliData->out.rgRatPf = maxRg * radio;	//256*256-1
	ISP_LOGD("PENG_END>%f\n", caliData->out.rgRatPf);
	for (i = 0; i < caliData->numP1_hwSample; i++) {
		caliData->out.rPf1[i] = ae_round(rTab1[i] * 65535.0 / maxRg);
		caliData->out.gPf1[i] = ae_round(gTab1[i] * 65535.0 / maxRg);
	}
	for (i = 0; i < caliData->numP2_hwSample; i++) {
		caliData->out.rPf2[i] = ae_round(rTab2[i] * 65535.0 / maxRg);
		caliData->out.gPf2[i] = ae_round(gTab2[i] * 65535.0 / maxRg);
	}
	
	//preflash brightness
	for (j = 0; j == 0 || j < caliData->numP2_hwSample; j++)	{		//LED2
		for (i = 0; i == 0 || i < caliData->numP1_hwSample; i++) {	//LED1
			uint32 ind = j * 3 + i;
			if (ae_round(caliData->maP1_hwSample[i] + caliData->maP2_hwSample[j]) <= ae_round(caliData->mAMaxP12)) {
				//double rg;

				int32 r = caliData->out.rPf1[i] + caliData->out.rPf2[j];	//pre&R	LED1+LED2
				int32 g = caliData->out.gPf1[i] + caliData->out.gPf2[j];	//pre&G	LED1+LED2
				//double b;
				ISP_LOGD("PENG_END:R [%d,%d]\n", caliData->out.rPf1[i], caliData->out.rPf2[j]);
				ISP_LOGD("PENG_END:G [%d,%d]\n", caliData->out.gPf1[i], caliData->out.gPf2[j]);
				if (r > 0 && g > 0) {
					caliData->out.rTable_preflash[ind] = (uint32)ae_round(1024 * g / r);
				} else {
					caliData->out.rTable_preflash[ind] = 0;
				}
				caliData->out.preflashBrightness[ind] = (uint32)ae_round(g * caliData->out.rgRatPf);
				ISP_LOGD("PENG_END>pre_brightnessTable:[%d,%d] %d\n", i, j, caliData->out.preflashBrightness[ind]);
			} else {
				caliData->out.rTable_preflash[ind] = 0;
				caliData->out.preflashBrightness[ind] = 0;
				ISP_LOGD("FLASH_CALI>LED1+LED2 ELE stream value is error\n");
			}
		}
	}

	//mainflash brightness
	//mainflash r/b table
	for (i = 0;  i == 0 || i < caliData->numM2_hwSample; i++) {
		for (j = 0; j == 0 || j < caliData->numM1_hwSample; j++) {
			uint32 ind = i * 3 + j;
			if (gTab1Main[j] != -1 && gTab2Main[i] != -1) {
				caliData->out.brightnessTable[ind] = (uint32)ae_round(gTab1Main[j] + gTab2Main[i]);
				ISP_LOGD("PENG_END>ID:[%d,%d] main_brightnessTable: %d\n", i, j, caliData->out.brightnessTable[ind]);
				float r = rTab1Main[j] + rTab2Main[i];
				float g = gTab1Main[j] + gTab2Main[i];
				float b = bTab1Main[j] + bTab2Main[i];
				ISP_LOGD("PENG_END:R(%f, %f)\n", rTab1Main[j], rTab2Main[i]);
				ISP_LOGD("PENG_END:G(%f, %f)\n", gTab1Main[j], gTab2Main[i]);
				ISP_LOGD("PENG_END:B(%f, %f)\n", bTab1Main[j], bTab2Main[i]);
				if (r > 0 && g > 0 && b > 0) {
					caliData->out.rTable[ind] = (uint32)ae_round(1024 * g / r);
					caliData->out.bTable[ind] = (uint32)ae_round(1024 * g / b);
				} else {
					caliData->out.brightnessTable[ind] = 0;
					caliData->out.rTable[ind] = 0;
					caliData->out.bTable[ind] = 0;
				}
			} else {
				caliData->out.brightnessTable[ind] = 0;
				caliData->out.rTable[ind] = 0;
				caliData->out.bTable[ind] = 0;
			}
			
			//for mask of main
			if (ae_round(caliData->maM1_hwSample[j] + caliData->maM2_hwSample[i]) < ae_round(caliData->mAMaxM12)) {
				caliData->out.flashMask[ind] = 1;
			} else {
				caliData->out.flashMask[ind] = 0;
			}
		}
	}
	
	{
		FILE *fp = NULL;
#ifdef WIN32
		fp = fopen("d:\\temp\\flash_led_brightness1.bin", "wb");
#else
		fp = fopen("/data/vendor/cameraserver/flash_led_brightness1.bin", "wb");
#endif
		struct flash_led_brightness1 led_bri;
		memset(&led_bri, 0, sizeof(struct flash_led_brightness1));
		led_bri.LED_enable = caliData->LED_enable;
		led_bri.levelNumber_pf1 = caliData->numP1_hwSample;
		led_bri.levelNumber_pf2 = caliData->numP2_hwSample;
		led_bri.levelNumber_mf1 = caliData->numM1_hwSample;
		led_bri.levelNumber_mf2 = caliData->numM2_hwSample;
		for (i = 0; i < led_bri.levelNumber_pf1; i++)
			led_bri.ledBrightness_pf1[i] = caliData->out.preflashBrightness[i];
		for (i = 0; i < led_bri.levelNumber_pf2; i++)
			led_bri.ledBrightness_pf2[i] = caliData->out.preflashBrightness[i * 3];
		for (i = 0; i < led_bri.levelNumber_mf1; i++)
			led_bri.ledBrightness_mf1[i] = caliData->out.brightnessTable[i];
		for (i = 0; i < led_bri.levelNumber_mf2; i++)
			led_bri.ledBrightness_mf2[i] = caliData->out.brightnessTable[i * 3];

		for (i = 0; i < led_bri.levelNumber_pf1; i++)
			led_bri.index_pf1[i] = caliData->indP1_hwSample[i];
		for (i = 0; i < led_bri.levelNumber_pf2; i++)
			led_bri.index_pf2[i] = caliData->indP2_hwSample[i];
		for (i = 0; i < led_bri.levelNumber_mf1; i++)
			led_bri.index_mf1[i] = caliData->indM1_hwSample[i];
		for (i = 0; i < led_bri.levelNumber_mf2; i++)
			led_bri.index_mf2[i] = caliData->indM2_hwSample[i];
		if(fp){
			fwrite(&led_bri, 1, sizeof(struct flash_led_brightness1), fp);
			fclose(fp);
			fp = NULL;
		}
	}
}


void writeDflashbin(struct ae_ctrl_cxt *cxt)
{
	static uint32 flg = 1;
	if ( flg ) {
		FILE *fp1 = fopen("/data/vendor/cameraserver/Dflash_cxt1.bin", "wb");
		if(fp1){
			fwrite(&cxt->dflash_param[0], 1, sizeof(struct flash_tune_param), fp1);
			fclose(fp1);
			fp1 = NULL;
		}
		flg = 0;
		ISP_LOGD("FLASH_CALI>save_flash_tune_param");
	}
}

void flash_calibration(cmr_handle ae_cxt, struct flash_cali_stat *cali_stat)
{
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)ae_cxt;
	//writeDflashbin(cxt);
	/*int32 prop2[10] = {0};
	int32 prop1 = get_prop_multi("persist.vendor.cam.isp.ae.DFlash_bin", 1, prop2);
	if (prop2[0] == 1 && prop1 > 0){
		FILE *fp1 = fopen("/data/vendor/cameraserver/DFlash.bin", "rb");
		if(fp1){
			fread(&cxt->dflash_param[0], 1, sizeof(struct flash_tune_param), fp1);
			fclose(fp1);
			fp1 = NULL;
		}
		ISP_LOGD("FLASH_CALI>flash config from DFlash.bin\n");
	}*/
	struct flash_tune_param *flash_tune = &cxt->dflash_param[0];
	struct flash_correct_info *result = &cxt->flash_cali_data;
	struct ae_lib_calc_in *cur_status = &cxt->cur_status;
	static struct FCData1 *caliData = NULL;
	static uint32 caliStat = FlashCali_start;
	static uint32 frameCount = 0;
	uint32 blkcnt[4] = {13, 13, 19, 19};	//block center 13-19:36
	static float rgbmean[3] = {0.0};
	static float G_AE_mean = 0.0;
	
	if(cali_stat->flag){
		caliData = NULL;
		caliStat = FlashCali_start;
		frameCount = 0;
	}

	int32 prop_fc[10] = {0};
	int32 propRet_fc = 0;
	propRet_fc = get_prop_multi("persist.vendor.cam.isp.ae.fc_cali_config", 1, prop_fc);
	int32 propval[10] = {0};
	int32 propRet = 0;
	propRet = get_prop_multi("persist.vendor.cam.isp.ae.flash_cali_debug", 1, propval);

	if (caliStat == FlashCali_start) {
		ISP_LOGD("FLASH_CALI>FlashCali_start\n");
		cali_stat->result = 1;	//flash calibration doing
		caliData = (struct FCData1 *)malloc(sizeof(struct FCData1));
		if(caliData == NULL){
			ISP_LOGE("FLASH_CALI>malloc is fail");
			return;
		}
		memset(caliData, 0, sizeof(struct FCData1));
		cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;

		if (prop_fc[0] == 1 && propRet_fc > 0){
			readFCConfig1("/data/vendor/cameraserver/fc_cali_config.txt", caliData);
		} else if(propRet_fc <= 0){
			//flash config from tuning param
			readFCtuneparam(&flash_tune->flash_correct_base, caliData);
		}

		write2FCConfig(caliData, result);
		//ISP_LOGD("PENG_NUM%d\n", result->numP1_hwSample);
		uint32 i = 0;
		uint32 id = 0;
		//preflash1
		caliData->expReset[id] = 1;
		caliData->ind1Tab[id] = 0;
		caliData->ind2Tab[id] = 0;
		caliData->testMinFrm[id] = 30;
		caliData->isMainTab[id] = 0;
		id++;
		for (i = 0; i < caliData->numP1_hwSample; i++, id++) {		//20
			caliData->expReset[id] = 0;
			if (caliData->LED_enable != 2)
				caliData->ind1Tab[id] = caliData->indP1_hwSample[i];			//LED1 index	0->2
			else
				caliData->ind1Tab[id] = -1;
			caliData->ind2Tab[id] = -1;
			caliData->testMinFrm[id] = 30;
			caliData->isMainTab[id] = 0;
		}
		//preflash2
		caliData->expReset[id] = 1;
		caliData->ind1Tab[id] = 0;
		caliData->ind2Tab[id] = 0;
		caliData->testMinFrm[id] = 30;
		caliData->isMainTab[id] = 0;
		id++;
		for (i = 0; i < caliData->numP2_hwSample; i++, id++) {
			caliData->expReset[id] = 0;
			caliData->ind1Tab[id] = -1;
			if (caliData->LED_enable != 1)
				caliData->ind2Tab[id] = caliData->indP2_hwSample[i];			//LED2 index 0->2
			else 
				caliData->ind2Tab[id] = -1;
			caliData->testMinFrm[id] = 30;
			caliData->isMainTab[id] = 0;		//0:pre	1:main
		}
		//m1
		caliData->expReset[id] = 1;
		caliData->ind1Tab[id] = 0;
		caliData->ind2Tab[id] = 0;
		caliData->testMinFrm[id] = 60;
		caliData->isMainTab[id] = 1;
		id++;
		for (i = 0; i < caliData->numM1_hwSample; i++, id++) {
			caliData->expReset[id] = 0;
			if (caliData->LED_enable != 2)
				caliData->ind1Tab[id] = caliData->indM1_hwSample[i];
			else
				caliData->ind1Tab[id] = -1;
			caliData->ind2Tab[id] = -1;
			caliData->testMinFrm[id] = 60;
			caliData->isMainTab[id] = 1;
		}
		//m2
		caliData->expReset[id] = 1;
		caliData->ind1Tab[id] = 0;
		caliData->ind2Tab[id] = 0;
		caliData->testMinFrm[id] = 60;
		caliData->isMainTab[id] = 1;
		id++;
		for (i = 0; i < caliData->numM2_hwSample; i++, id++) {
			caliData->expReset[id] = 0;
			caliData->ind1Tab[id] = -1;
			if (caliData->LED_enable != 1)
				caliData->ind2Tab[id] = caliData->indM2_hwSample[i];
			else
				caliData->ind2Tab[id] = -1;
			caliData->testMinFrm[id] = 60;
			caliData->isMainTab[id] = 1;
		}
		caliData->testIndAll = id;				//16
		ISP_LOGD("FLASH_CALI>TESTINDALL[%d]\n", caliData->testIndAll);

		caliData->expTimeBase = 0.05 * AEC_LINETIME_PRECESION;
		caliData->gainBase = 8 * 128;
		caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
		caliData->gain = 8 * 128;

		caliData->stateAeFrameCntSt = 1;
		caliStat = FlashCali_ae;
	}else if(caliStat == FlashCali_ae) {
		cali_stat->result = 1;
		int32 frm = frameCount - caliData->stateAeFrameCntSt;

		ISP_LOGD("FLASH_CALI>FlashCali_ae!	frm:%d\n", frm);
		if ((frm % 3 == 0) && (frm != 0)) {
			//struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;
			getBlockMean((cmr_u32 *)&cxt->sync_aem[0], &blkcnt[0], cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0, &rgbmean[0]);
			ISP_LOGD("FLASH_CALI>qqfc AE frmCnt=%d exp,gain=%d %d, gmean=%f", (uint32)frameCount, (uint32)caliData->expTime, (uint32)caliData->gain, rgbmean[1]);
			G_AE_mean = rgbmean[1];
			if ((rgbmean[1] > 200 && rgbmean[1] < 400) || (FLOAT_EQUAL(caliData->expTime, (0.05 * AEC_LINETIME_PRECESION)) && caliData->gain == (8 * 128))) {	
				caliData->stateCaliFrameCntSt = frameCount + 1;
				caliData->expTimeBase = caliData->expTime;
				caliData->gainBase = caliData->gain;
				caliStat = FlashCali_cali;
			} else {
				//ISP_LOGD("FLASH_CALI>FlashCali_ae_ELSE!\n");
				if (rgbmean[1] < 10) {
					caliData->expTime *= 25;
					//caliData->gain = caliData->gain;
				} else {
					caliData->expTime = (uint32)caliData->expTime * (300 / rgbmean[1]);
					//caliData->gain = caliData->gain;
				}
				float ratio = 0.0;
				if (caliData->expTime > 0.05 * AEC_LINETIME_PRECESION) {
					ratio = caliData->expTime / (0.05 * AEC_LINETIME_PRECESION);
					caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
					caliData->gain *= ratio;
				} else if (caliData->expTime < 0.001 * AEC_LINETIME_PRECESION) {
					ratio = caliData->expTime / (0.001 * AEC_LINETIME_PRECESION);
					caliData->expTime = 0.001 * AEC_LINETIME_PRECESION;
					caliData->gain *= ratio;
				}
				if (caliData->gain < 128)
					caliData->gain = 128;
				else if (caliData->gain > 8 * 128)
					caliData->gain = 8 * 128;
			}
		}
	}else if(caliStat == FlashCali_cali) {
		cali_stat->result = 1;
		ISP_LOGD("FLASH_CALI>FlashCali_cali\n");
		if ((frameCount - caliData->stateCaliFrameCntSt) == 0) {
			caliData->testInd = 0;
			caliData->stateCaliFrameCntStSub = caliData->stateCaliFrameCntSt;
		}

		uint32 frmCnt = frameCount - caliData->stateCaliFrameCntStSub;
		ISP_LOGD("qqfc caliData->testInd=%d %d %d", caliData->testInd, frmCnt,caliData->expReset[caliData->testInd]);

		if (frmCnt == 0) {
			//led_id = caliData->testInd;
			int32 led1 = caliData->ind1Tab[caliData->testInd];		//id:0-3	0:mask	1-3:lew/mid/high
			int32 led2 = caliData->ind2Tab[caliData->testInd];
			ISP_LOGD("FLASH_CALI>LEVEL_ID:%d LED[%d,%d]\n", caliData->testInd, led1, led2);

			if (caliData->expReset[caliData->testInd] == 1) {		//mask
				caliData->expTime = caliData->expTimeBase;
				caliData->gain = caliData->gainBase;
			}

			caliData->expTab[caliData->testInd] = caliData->expTime;
			caliData->gainTab[caliData->testInd] = caliData->gain;
			if (led1 == 0 && led2 == 0)		//mask
				control_led(cxt, 0, 0, 0, 0);
			else{
				control_led(cxt, 1, caliData->isMainTab[caliData->testInd], led1, led2);		//flash on
				ISP_LOGD("FLASH_CALI>flash on!\n");
			}
		} else if (frmCnt == 4) {
			if (caliData->testInd == 1){
				cali_stat->result = flash_varify((cmr_u32 *)&cxt->sync_aem[0], &caliData->fourcorners[0], &caliData->around2center[0], &cali_stat->error_info[0],
					cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0);
				if(cali_stat->result != 0 && cali_stat->result != 1){
					cali_stat->flag = 1;
					control_led(cxt, 0, 0, 0, 0);
					caliStat = FlashCali_none;
				}
				ISP_LOGD("FLASH_CALI>varify check result:%d,%s\n", cali_stat->result, &cali_stat->error_info[0]);
			}

			ISP_LOGD("FLASH_CALI>FlashCali_CALI_AE! frmCnt:%d\n", frmCnt);
			
			//struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;
			getBlockMean((cmr_u32 *)&cxt->sync_aem[0], &blkcnt[0], cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0, &rgbmean[0]);
			ISP_LOGD("FLASH_CALI>frm: %d RGBmean[%f,%f,%f]\n", frmCnt,rgbmean[0], rgbmean[1], rgbmean[2]);
			ISP_LOGD("FLASH_CALI>expTime gain %d %d \n", caliData->expTime, caliData->gain);
			
			if((frmCnt != 0) && ((caliData->testInd >= 1) && (caliData->expReset[caliData->testInd - 1] == 1) && (caliData->expReset[caliData->testInd] == 0))){
				if ((rgbmean[1] > G_AE_mean && rgbmean[1] < 400)) {
					//caliData->stateCaliFrameCntSt = frameCount + 1;
					caliData->expTimeBase = caliData->expTime;
					caliData->gainBase = caliData->gain;
					//caliStat = FlashCali_cali;
				} else {
					float rat = 0.0;
					if(rgbmean[1] < 10){
						rat = rgbmean[1] / 75.0;
					} else {
					 	rat = rgbmean[1] / 300.0; //75
					}
					float rat1 = 0.0;
					float rat2 = 0.0;
					float rat3 = 0.0;
					if (caliData->expTime > 0.03 * AEC_LINETIME_PRECESION) {	//20-33
						rat1 = rat;
						float expTest = caliData->expTime / rat1;
						if (expTest < 0.03 * AEC_LINETIME_PRECESION )
							expTest = 0.03 * AEC_LINETIME_PRECESION;
						rat1 = caliData->expTime / expTest;
						caliData->expTime = (uint32)expTest;
					} else
						rat1 = 1;
	
					if (caliData->gain > 128) {
						rat2 = rat / rat1;
						uint32 gainTest = caliData->gain;
						gainTest /= rat2;
						if (gainTest < 128)
							gainTest = 128;
						rat2 = (float)caliData->gain / gainTest;
						caliData->gain = gainTest;
					} else {
						rat2 = 1;
					}
					{
						rat3 = rat / rat1 / rat2;
						float expTest = caliData->expTime / rat3;
						if (expTest < 0.001 * AEC_LINETIME_PRECESION)
							expTest = 0.001 * AEC_LINETIME_PRECESION;
						rat3 = (double)caliData->expTime / expTest;
						caliData->expTime = (uint32)expTest;
						ISP_LOGD("FLASH_CALI>2:[%d, %d]\n", caliData->expTime, caliData->gain);
					}
					//for reset cali exp and gain
					//caliData->expReset[caliData->testInd] = 0; // reset value is not enough for calibration
					control_led(cxt, 0, 0, 0, 0);
					ISP_LOGD("FLASH_CALI>LED_OFF0\n");
					caliData->stateCaliFrameCntStSub = frameCount + 1;

#if 0
					ISP_LOGD("FLASH_CALI>FlashCali_ae_ELSE!\n");
					if (rgbmean[1] < 10) {
						caliData->expTime *= 25;
						//caliData->gain = caliData->gain;
					} else {
						caliData->expTime *= 300 / rgbmean[1];
						//caliData->gain = caliData->gain;
					}
					float ratio = 0.0;
					if (caliData->expTime > 0.05 * AEC_LINETIME_PRECESION) {
						ratio = caliData->expTime / (0.05 * AEC_LINETIME_PRECESION);
						caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
						caliData->gain *= ratio;
					} else if (caliData->expTime < 0.001 * AEC_LINETIME_PRECESION) {
						ratio = caliData->expTime / (0.001 * AEC_LINETIME_PRECESION);
						caliData->expTime = 0.001 * AEC_LINETIME_PRECESION;
						caliData->gain *= ratio;
					}
					if (caliData->gain < 128)
						caliData->gain = 128;
					else if (caliData->gain > 8 * 128)
						caliData->gain = 8 * 128;
					caliData->expReset[caliData->testInd] = 0; // reset value is not enough for calibration
					caliData->stateCaliFrameCntStSub = frameCount + 1;
#endif 
				}


			}else if ((rgbmean[1] > 600) && (caliData->testInd >= 1) && !((caliData->expReset[caliData->testInd - 1] == 1) && (caliData->expReset[caliData->testInd] == 0))){
				if ((rgbmean[1] > 200 && rgbmean[1] < 600)) {
					//caliData->stateCaliFrameCntSt = frameCount + 1;
					caliData->expTimeBase = caliData->expTime;
					caliData->gainBase = caliData->gain;
					//caliStat = FlashCali_cali;
				} else {
					float rat = rgbmean[1] / 300.0; //75
					float rat1 = 0.0;
					float rat2 = 0.0;
					float rat3 = 0.0;
					if (caliData->expTime > 0.03 * AEC_LINETIME_PRECESION) {	//20-33
						rat1 = rat;
						float expTest = caliData->expTime / rat1;
						if (expTest < 0.03 * AEC_LINETIME_PRECESION )
							expTest = 0.03 * AEC_LINETIME_PRECESION;
						rat1 = caliData->expTime / expTest;
						caliData->expTime = (uint32)expTest;
					} else
						rat1 = 1;

					if (caliData->gain > 128) {
						rat2 = rat / rat1;
						uint32 gainTest = caliData->gain;
						gainTest /= rat2;
						if (gainTest < 128)
							gainTest = 128;
						rat2 = (float)caliData->gain / gainTest;
						caliData->gain = gainTest;
					} else {
						rat2 = 1;
					}
					ISP_LOGD("FLASH_CALI>1:[%d, %d]\n", caliData->expTime, caliData->gain);
					{
						rat3 = rat / rat1 / rat2;
						float expTest = caliData->expTime / rat3;
						if (expTest < 0.001 * AEC_LINETIME_PRECESION)
							expTest = 0.001 * AEC_LINETIME_PRECESION;
						rat3 = (double)caliData->expTime / expTest;
						caliData->expTime = (uint32)expTest;

						/*
						caliData->expTab[caliData->testInd] = caliData->expTime;
						caliData->gainTab[caliData->testInd] = caliData->gain;
						*/
						ISP_LOGD("FLASH_CALI>2:[%d, %d]\n", caliData->expTime, caliData->gain);
						if (FLOAT_EQUAL(expTest, 0.001 * AEC_LINETIME_PRECESION) && caliData->gain == 128) {
							char err[] = "error: the exposure is min\n";
#ifdef WIN32
							printf(err);
#endif
							ISP_LOGE("%s", err);
							cali_stat->result = FLASH_CALI_EXPGAIN_MIN; 	//error manager
							sprintf(cali_stat->error_info, "%s", "exp&gain value is min\n");
						} else {
							if(caliData->expReset[caliData->testInd + 1] != 1){
								//caliData->expReset[caliData->testInd + 1] = 2; // reset value is not enough for calibration
								ISP_LOGD("FLASH_CALI>OVER_EXP%d\n", caliData->testInd);
							}
						} /*else {
							//caliData->expReset[caliData->testInd] = 0; // reset value is not enough for calibration
							ISP_LOGD("FLASH_CALI>%d\n", caliData->testInd);
							caliData->stateCaliFrameCntStSub = frameCount + 1;
						}*/
					}
					//for reset cali exp and gain
					//caliData->expReset[caliData->testInd] = 0; // reset value is not enough for calibration
					control_led(cxt, 0, 0, 0, 0);
					ISP_LOGD("FLASH_CALI>LED_OFF1");
					caliData->stateCaliFrameCntStSub = frameCount + 1;

				}
			}
		}

		if (caliData->expReset[caliData->testInd] == 1) {
			ISP_LOGD("FLASH_CALI>TESTIND:%d", caliData->testInd);
			caliData->testInd++;
			caliData->stateCaliFrameCntStSub = frameCount + 1;
		}

		if (frmCnt < 15) {
			//struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;
			getBlockMean((cmr_u32 *)&cxt->sync_aem[0], &blkcnt[0], cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0, &rgbmean[0]);
			caliData->rFrame[frmCnt] = rgbmean[0];
			caliData->gFrame[frmCnt] = rgbmean[1];
			caliData->bFrame[frmCnt] = rgbmean[2];
			ISP_LOGD("FLASH_CALI>[%d: %f,%f,%f]\n", frmCnt, rgbmean[0], rgbmean[1], rgbmean[2]);
		} else if (frmCnt > caliData->testMinFrm[caliData->testInd]) {
			control_led(cxt, 0, 0, 0, 0);	//flash off
			ISP_LOGD("FLASH_CALI>flash off!\n");
			//float rgb[3] = {0.0};

			uint32 frm_st = 0;
			uint32 frm_ed = 0;
			if (caliData->isMainTab[caliData->testInd] == 0) {
				frm_st = caliData->prefm[0];
				frm_ed = caliData->prefm[1];
			} else if (caliData->isMainTab[caliData->testInd] == 1){
				frm_st = caliData->mainfm[0];
				frm_ed = caliData->mainfm[1];
			}
			ISP_LOGD("FLASH_CALI>[%d, %d]\n", frm_st, frm_ed);
			calRgbFrameData1(frm_st, frm_ed, &caliData->rFrame[0], &caliData->gFrame[0], &caliData->bFrame[0], &caliData->rgb[0]);
			caliData->rData[caliData->testInd] = caliData->rgb[0];
			caliData->gData[caliData->testInd] = caliData->rgb[1];
			caliData->bData[caliData->testInd] = caliData->rgb[2];
			caliData->testInd++;
			ISP_LOGD("FLASH_CALI>TESTIND1:%d", caliData->testInd);
			caliData->stateCaliFrameCntStSub = frameCount + 1;
			if (caliData->testInd == caliData->testIndAll){
				caliStat = FlashCali_end;
			}
		}
		if (caliData->testInd == caliData->testIndAll && caliData->numM2_hwSample == 0) {
			caliStat = FlashCali_end;
			ISP_LOGD("FLASH_CALI> testInd == testIndALL && numM2_hwSample == 0");
		}
	}else if(caliStat == FlashCali_end){	//cali finish
		ISP_LOGD("FLASH_CALI>FlashCali_end\n");
		uint32 i = 0;
		for (i = 0; i < caliData->testIndAll; i++) {
			if (caliData->rData[i] < 0 || caliData->gData[i] < 0 || caliData->bData[i] < 0){
				caliData->rData[i] = 0;
				caliData->gData[i] = 0;
				caliData->bData[i] = 0;
			}
			uint32 j = 0;
			if ( caliData->expReset[i] != 1 ) {
				if((i >= 1) && (i < caliData->numP1_hwSample + 1)){
					j = i - 1;
					result->P1rgbData[j][0] = caliData->rData[i];
					result->P1rgbData[j][1] = caliData->gData[i];
					result->P1rgbData[j][2] = caliData->bData[i];

					result->P1expgain[j][0] = caliData->expTab[i];
					result->P1expgain[j][1] = caliData->gainTab[i];
				} else if((i >= caliData->numP1_hwSample + 2) && (i < caliData->numP1_hwSample + caliData->numP2_hwSample + 2)){
					j = i - caliData->numP1_hwSample - 2;
					result->P2rgbData[j][0] = caliData->rData[i];
					result->P2rgbData[j][1] = caliData->gData[i];
					result->P2rgbData[j][2] = caliData->bData[i];

					result->P2expgain[j][0] = caliData->expTab[i];
					result->P2expgain[j][1] = caliData->gainTab[i];
				} else if((i >= caliData->numP1_hwSample + caliData->numP2_hwSample + 3) && (i < caliData->numP1_hwSample + caliData->numP2_hwSample + caliData->numM1_hwSample + 3)){
					j = i - caliData->numP1_hwSample - caliData->numP2_hwSample - 3;
					result->M1rgbData[j][0] = caliData->rData[i];
					result->M1rgbData[j][1] = caliData->gData[i];
					result->M1rgbData[j][2] = caliData->bData[i];

					result->M1expgain[j][0] = caliData->expTab[i];
					result->M1expgain[j][1] = caliData->gainTab[i];
				} else if(i >= caliData->numP1_hwSample + caliData->numP2_hwSample + caliData->numM1_hwSample + 4){
					j = i - caliData->numP1_hwSample - caliData->numP2_hwSample - caliData->numM1_hwSample - 4;
					result->M2rgbData[j][0] = caliData->rData[i];
					result->M2rgbData[j][1] = caliData->gData[i];
					result->M2rgbData[j][2] = caliData->bData[i];

					result->M2expgain[j][0] = caliData->expTab[i];
					result->M2expgain[j][1] = caliData->gainTab[i];
				}
			}
			ISP_LOGD("FLASH_CALI>ID:%d[%f,%f,%f][%d,%d]\n", i, caliData->rData[i], caliData->gData[i], caliData->bData[i], caliData->expTab[i], caliData->gainTab[i]);
		}
		cali_stat->flag = 0;
		cali_stat->result = 0;	//successful
		cali_stat->enable = 1;	//flash cali has been completed
		sprintf(cali_stat->error_info, "%s", "Flash calibration is success!\n");
		caliStat = FlashCali_none;
		if (propval[0] == 1 && propRet > 0){
			write2flashcali(cxt->camera_id, result);
			write2brigthness(caliData);
		}
	}

	if (caliStat < FlashCali_end) {
		ISP_LOGD("qqfc ID[%d], exp=[%d %d]", frameCount, (int)caliData->expTime, (int)caliData->gain);
		//int lineTime = cxt->cur_status.adv_param.cur_ev_setting.line_time;
		cur_status->adv_param.mode_param.mode = 3;
		cxt->cur_status.adv_param.mode_param.value.ae_idx = 0;
		//cxt->cur_status.adv_param.mode_param.exp_line = caliData->expTime / lineTime;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = caliData->expTime;
		cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = caliData->gain;
	}
	frameCount++;
}


static void flashCalibration(struct ae_ctrl_cxt *cxt)
{
	struct ae_lib_calc_in *cur_status = &cxt->cur_status;
	static int frameCount = 0;
	static int caliState = FlashCali_start;
	static struct FCData *caliData = 0;
	int propValue[10];
	int propRet;
	propRet = get_prop_multi("persist.vendor.cam.isp.ae.flash_cali", 1, propValue);
	if (propRet <= 0)
		return;

	if (propValue[0] == 0) {
		control_led(cxt, 0, 0, 0, 0);
		caliState = FlashCali_start;
	}
	if (propValue[0] == 1) {

		ISP_LOGD("qqfc state=%d", caliState);
		if (caliState == FlashCali_start) {
			ISP_LOGD("qqfc FlashCali_start");
			caliData = (struct FCData *)malloc(sizeof(struct FCData));
			caliData->out.error = 0;

#ifdef WIN32
			readFCConfig("fc_config.txt", caliData, "fc_config_check.txt");
#else
			readFCConfig("/data/vendor/cameraserver/fc_config.txt", caliData, "/data/vendor/cameraserver/fc_config_check.txt");
#endif
			frameCount = 0;
			cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;	//lock ae

			reduceFlashIndexTab(caliData->numP1_hwSample, caliData->indP1_hwSample, caliData->maP1_hwSample, caliData->mAMaxP1, 32, caliData->indHwP1_alg, caliData->maHwP1_alg, &caliData->numP1_alg);
			reduceFlashIndexTab(caliData->numP2_hwSample, caliData->indP2_hwSample, caliData->maP2_hwSample, caliData->mAMaxP2, 32, caliData->indHwP2_alg, caliData->maHwP2_alg, &caliData->numP2_alg);
			reduceFlashIndexTab(caliData->numM1_hwSample, caliData->indM1_hwSample, caliData->maM1_hwSample, caliData->mAMaxM1, 32, caliData->indHwM1_alg, caliData->maHwM1_alg, &caliData->numM1_alg);
			reduceFlashIndexTab(caliData->numM2_hwSample, caliData->indM2_hwSample, caliData->maM2_hwSample, caliData->mAMaxM2, 32, caliData->indHwM2_alg, caliData->maHwM2_alg, &caliData->numM2_alg);

			//gen test
			int i;
			int id = 0;
			//preflash1
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numP1_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = i;
				caliData->ind2Tab[id] = 0;
				caliData->testMinFrm[id] = 0;
				caliData->isMainTab[id] = 0;
				id++;
			}
			//preflash2
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numP2_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = 0;
				caliData->ind2Tab[id] = i;
				caliData->testMinFrm[id] = 0;
				caliData->isMainTab[id] = 0;
				id++;
			}
			//m1
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numM1_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = i;
				caliData->ind2Tab[id] = 0;
				caliData->testMinFrm[id] = 120;
				caliData->isMainTab[id] = 1;
				id++;
			}
			//m2
			caliData->expReset[id] = 1;
			caliData->ind1Tab[id] = 0;
			caliData->ind2Tab[id] = 0;
			caliData->testMinFrm[id] = 0;
			caliData->isMainTab[id] = 0;
			id++;
			for (i = 1; i < caliData->numM2_alg; i++) {
				caliData->expReset[id] = 0;
				caliData->ind1Tab[id] = 0;
				caliData->ind2Tab[id] = i;
				caliData->testMinFrm[id] = 120;
				caliData->isMainTab[id] = 1;
				id++;
			}
			caliData->testIndAll = id;

			caliData->expTimeBase = 0.05 * AEC_LINETIME_PRECESION;
			caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
			caliData->gainBase = 8 * 128;
			caliData->gain = 8 * 128;

			//caliData->stateCaliFrameCntSt = frameCount + 1;
			//caliState = FlashCali_cali;

			caliData->stateAeFrameCntSt = 1;
			caliState = FlashCali_ae;

		} else if (caliState == FlashCali_ae) {
			int frm = frameCount - caliData->stateAeFrameCntSt;
			if ((frm % 3 == 0) && (frm != 0)) {
				float rmean;
				float gmean;
				float bmean;
				//struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32 *) & cxt->sync_aem[0],
							  caliData->rBuf, caliData->gBuf, caliData->bBuf, cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0, &rmean, &gmean, &bmean);
				ISP_LOGD("qqfc AE frmCnt=%d sh,gain=%d %d, gmean=%f", (int)frameCount, (int)caliData->expTime, (int)caliData->gain, gmean);

				bool flagcaliData;
				flagcaliData=(gmean > 200 && gmean < 400) || (FLOAT_EQUAL(caliData->expTime, (0.05 * AEC_LINETIME_PRECESION)) && caliData->gain == (8 * 128));



				if (flagcaliData) {
					caliData->stateCaliFrameCntSt = frameCount + 1;
					caliData->expTimeBase = caliData->expTime;
					caliData->gainBase = caliData->gain;
					caliState = FlashCali_cali;
				} else {
				       //caliData->gain = caliData->gain;
				      	caliData->expTime=(gmean < 10)?(caliData->expTime*25):(caliData->expTime*300 / gmean);

					  float gain_tmp = (float)caliData->gain;
					if (caliData->expTime > 0.05 * AEC_LINETIME_PRECESION) {
						float ratio = caliData->expTime / (0.05 * AEC_LINETIME_PRECESION);
						caliData->expTime = 0.05 * AEC_LINETIME_PRECESION;
						gain_tmp *= ratio;
					} else if (caliData->expTime < 0.001 * AEC_LINETIME_PRECESION) {
						float ratio = caliData->expTime / (0.001 * AEC_LINETIME_PRECESION);
						caliData->expTime = 0.001 * AEC_LINETIME_PRECESION;
						gain_tmp *= ratio;
					}
					caliData->gain = (int)(gain_tmp + 0.5);
					if (caliData->gain < 128)
						caliData->gain = 128;
					else if (caliData->gain > 8 * 128)
						caliData->gain = 8 * 128;
				}
			}


		} else if (caliState == FlashCali_cali) {
			int frmCntState;
			uint32 frmCnt;
			frmCntState = frameCount - caliData->stateCaliFrameCntSt;
			if (frmCntState == 0) {
				caliData->testInd = 0;
				caliData->stateCaliFrameCntStSub = caliData->stateCaliFrameCntSt;
			}

			frmCnt = frameCount - caliData->stateCaliFrameCntStSub;
			ISP_LOGD("qqfc caliData->testInd=%d", caliData->testInd);
			if (frmCnt == 0) {
				int id;
				id = caliData->testInd;
				int led1;
				int led2;
				int led1_hw;
				int led2_hw;
				led1 = caliData->ind1Tab[id];
				led2 = caliData->ind2Tab[id];

				if (caliData->isMainTab[id] == 0) {
					led1_hw = caliData->indHwP1_alg[led1];
					led2_hw = caliData->indHwP2_alg[led2];
				} else {
					led1_hw = caliData->indHwM1_alg[led1];
					led2_hw = caliData->indHwM1_alg[led2];
				}



				if (caliData->expReset[id] == 1) {
					caliData->expTime = caliData->expTimeBase;
					caliData->gain = caliData->gainBase;
				}
				if(caliData->expReset[id] == 2){
					caliData->expTime = caliData->nextexpTime;
					caliData->gain = caliData->nextgain;
				}
				caliData->expTab[id] = caliData->expTime;
				caliData->gainTab[id] = caliData->gain;
				if (led1 == 0 && led2 == 0)
					control_led(cxt, 0, 0, 0, 0);
				else
					control_led(cxt, 1, caliData->isMainTab[id], led1_hw, led2_hw);
			} else if (frmCnt == 3) {
				float rmean;
				float gmean;
				float bmean;
				//struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32 *) & cxt->sync_aem[0],
							  caliData->rBuf, caliData->gBuf, caliData->bBuf,cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0, &rmean, &gmean, &bmean);
				if (gmean > 500) {

					float rat = gmean / 300.0;
					float rat1;
					float rat2;
					float rat3;

					if (caliData->expTime > 0.03 * AEC_LINETIME_PRECESION) {
						rat1 = rat;
						float expTest = caliData->expTime / rat1;
						if (expTest < 0.03 * AEC_LINETIME_PRECESION)
							expTest = 0.03 * AEC_LINETIME_PRECESION;
						rat1 = caliData->expTime / expTest;
						caliData->nextexpTime = expTest;
					} else
						rat1 = 1;

					if (caliData->gain > 128) {
						rat2 = rat / rat1;
						int gainTest = (int)((float)caliData->gain / rat2);
						if (gainTest < 128)
							gainTest = 128;
						rat2 = (float)caliData->gain / (float)gainTest;
						caliData->nextgain = gainTest;
					} else {
						rat2 = 1;
					}
					{
						rat3 = rat / rat1 / rat2;
						float expTest = caliData->nextexpTime / rat3;
						if (expTest < 0.001 * AEC_LINETIME_PRECESION)
							expTest = 0.001 * AEC_LINETIME_PRECESION;
						rat3 = caliData->nextexpTime / expTest;
						caliData->nextexpTime = expTest;

						if (FLOAT_EQUAL(expTest, 0.001 * AEC_LINETIME_PRECESION) && caliData->nextgain == 128) {
							char err[] = "error: the exposure is min\n";
#ifdef WIN32
							printf(err);
#endif
							ISP_LOGE("%s", err);
							caliData->out.error = FlashCali_too_close;

						} else {
							caliData->expReset[caliData->testInd + 1] = 2; // reset value is not enough for calibration
							//caliData->stateCaliFrameCntStSub = frameCount + 1;
						}
					}
					caliData->rFrame[caliData->testInd][frmCnt] = rmean;
					caliData->gFrame[caliData->testInd][frmCnt] = gmean;
					caliData->bFrame[caliData->testInd][frmCnt] = bmean;
				} else {
					caliData->rFrame[caliData->testInd][frmCnt] = rmean;
					caliData->gFrame[caliData->testInd][frmCnt] = gmean;
					caliData->bFrame[caliData->testInd][frmCnt] = bmean;
				}
			}
			if (frmCnt < 15) {
				float rmean;
				float gmean;
				float bmean;
				//struct ae_lib_calc_in *current_status = &cxt->sync_cur_status;
				getCenterMean((cmr_u32 *) & cxt->sync_aem[0],
							  caliData->rBuf, caliData->gBuf, caliData->bBuf, cxt->cur_status.stats_data_basic.blk_size, cxt->cur_status.stats_data_basic.size, 0, &rmean, &gmean, &bmean);
				caliData->rFrame[caliData->testInd][frmCnt] = rmean;
				caliData->gFrame[caliData->testInd][frmCnt] = gmean;
				caliData->bFrame[caliData->testInd][frmCnt] = bmean;

			} else if (frmCnt > caliData->testMinFrm[caliData->testInd]) {
				control_led(cxt, 0, 0, 0, 0);
				float r = 0.0;
				float g = 0.0;
				float b = 0.0;
				int frm_st = 0, frm_ed = 0;
				if (caliData->isMainTab[caliData->testInd]) {
					frm_st = caliData->mf_st;
					frm_ed = caliData->mf_ed;
				} else {
					frm_st = caliData->pf_st;
					frm_ed = caliData->pf_ed;
				}
				CalcRgbFrmsData(frm_st, frm_ed,
								caliData->rFrame[caliData->testInd],
								caliData->gFrame[caliData->testInd],
								caliData->bFrame[caliData->testInd],
								&r, &g, &b);
				caliData->rData[caliData->testInd] = r;
				caliData->gData[caliData->testInd] = g;
				caliData->bData[caliData->testInd] = b;
				caliData->testInd++;
				caliData->stateCaliFrameCntStSub = frameCount + 1;
				if (caliData->testInd == caliData->testIndAll)
					caliState = FlashCali_end;
			}

		} else if (caliState == FlashCali_end) {
			//readDebugBin2("d:\\temp\\fc_debug.bin", caliData);
			int i;
			int j;
			float rTab1[32];
			float gTab1[32];
			float bTab1[32];
			float rTab2[32];
			float gTab2[32];
			float bTab2[32];
			float rTab1Main[32];
			float gTab1Main[32];
			float bTab1Main[32];
			float rTab2Main[32];
			float gTab2Main[32];
			float bTab2Main[32];

			double rbase;
			double gbase;
			double bbase;
			rbase = caliData->rData[0];
			gbase = caliData->gData[0];
			bbase = caliData->bData[0];
			for (i = 0; i < 32; i++) {
				gTab1[i] = -1;
				gTab2[i] = -1;
				gTab1Main[i] = -1;
				gTab2Main[i] = -1;
			}
			rTab1[0] = 0;
			gTab1[0] = 0;
			bTab1[0] = 0;
			rTab2[0] = 0;
			gTab2[0] = 0;
			bTab2[0] = 0;
			rTab1Main[0] = 0;
			gTab1Main[0] = 0;
			bTab1Main[0] = 0;
			rTab2Main[0] = 0;
			gTab2Main[0] = 0;
			bTab2Main[0] = 0;
			double maxV = 0;
			for (i = 0; i < caliData->testIndAll; i++) {
				double rat;
				rat = (double)caliData->expTimeBase * caliData->gainBase / caliData->gainTab[i] / caliData->expTab[i];

				int id1;
				int id2;
				id1 = caliData->ind1Tab[i];
				id2 = caliData->ind2Tab[i];

				float r;
				float g;
				float b;

				r = caliData->rData[i] * rat - rbase;
				g = caliData->gData[i] * rat - gbase;
				b = caliData->bData[i] * rat - bbase;
				bool flagrgb;
				flagrgb=(r < 0 || g < 0 || b < 0);
				if (flagrgb) {
					r = 0;
					g = 0;
					b = 0;
				}

				if (maxV < g)
					maxV = g;
				if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i]) {
					rTab1Main[id1] = r;
					gTab1Main[id1] = g;
					bTab1Main[id1] = b;
				} else if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i] == 0) {
					rTab1[id1] = r;
					gTab1[id1] = g;
					bTab1[id1] = b;
				} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i]) {
					rTab2Main[id2] = r;
					gTab2Main[id2] = g;
					bTab2Main[id2] = b;
				} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i] == 0) {
					rTab2[id2] = r;
					gTab2[id2] = g;
					bTab2[id2] = b;
				}
			}
			double sc = 0;
			if (maxV > 0.0) {
				sc = 65536.0 / maxV / 2;
			} else {
				ISP_LOGW("maxV is invalidated, %f", maxV);
			}
			for (i = 0; i < 32; i++) {
				rTab1[i] *= sc;
				gTab1[i] *= sc;
				bTab1[i] *= sc;
				rTab2[i] *= sc;
				gTab2[i] *= sc;
				bTab2[i] *= sc;
				rTab1Main[i] *= sc;
				gTab1Main[i] *= sc;
				bTab1Main[i] *= sc;
				rTab2Main[i] *= sc;
				gTab2Main[i] *= sc;
				bTab2Main[i] *= sc;
			}

			caliData->out.version = CALI_VERSION;
			caliData->out.version_sub = CALI_VERSION_SUB;
			strcpy(caliData->out.name, "flash_calibration_data");
			caliData->out.otp_random_r = cxt->awb_otp_info.rdm_stat_info.r;
			caliData->out.otp_random_g = cxt->awb_otp_info.rdm_stat_info.g;
			caliData->out.otp_random_b = cxt->awb_otp_info.rdm_stat_info.b;
			caliData->out.otp_golden_r = cxt->awb_otp_info.gldn_stat_info.r;
			caliData->out.otp_golden_g = cxt->awb_otp_info.gldn_stat_info.g;
			caliData->out.otp_golden_b = cxt->awb_otp_info.gldn_stat_info.b;


			for (i = 0; i < 32; i++) {
				caliData->out.driverIndexP1[i] = -1;
				caliData->out.driverIndexP2[i] = -1;
				caliData->out.driverIndexM1[i] = -1;
				caliData->out.driverIndexM2[i] = -1;
			}
			for (i = 0; i < caliData->numP1_alg; i++)
				caliData->out.driverIndexP1[i] = caliData->indHwP1_alg[i];
			for (i = 0; i < caliData->numP2_alg; i++)
				caliData->out.driverIndexP2[i] = caliData->indHwP2_alg[i];
			for (i = 0; i < caliData->numM1_alg; i++)
				caliData->out.driverIndexM1[i] = caliData->indHwM1_alg[i];
			for (i = 0; i < caliData->numM2_alg; i++)
				caliData->out.driverIndexM2[i] = caliData->indHwM2_alg[i];
			for (i = 0; i < caliData->numP1_alg; i++)
				caliData->out.maP1[i] = ae_round(caliData->maHwP1_alg[i]);
			for (i = 0; i < caliData->numP2_alg; i++)
				caliData->out.maP2[i] = ae_round(caliData->maHwP2_alg[i]);
			for (i = 0; i < caliData->numM1_alg; i++)
				caliData->out.maM1[i] = ae_round(caliData->maHwM1_alg[i]);
			for (i = 0; i < caliData->numM2_alg; i++)
				caliData->out.maM2[i] = ae_round(caliData->maHwM2_alg[i]);

			caliData->out.numP1_hwSample = caliData->numP1_hwSample;
			caliData->out.numP2_hwSample = caliData->numP2_hwSample;
			caliData->out.numM1_hwSample = caliData->numM1_hwSample;
			caliData->out.numM2_hwSample = caliData->numM2_hwSample;
			caliData->out.mAMaxP1 = ae_round(caliData->mAMaxP1);
			caliData->out.mAMaxP2 = ae_round(caliData->mAMaxP2);
			caliData->out.mAMaxP12 = ae_round(caliData->mAMaxP12);
			caliData->out.mAMaxM1 = ae_round(caliData->mAMaxM1);
			caliData->out.mAMaxM2 = ae_round(caliData->mAMaxM2);
			caliData->out.mAMaxM12 = ae_round(caliData->mAMaxM12);

			//--------------------------
			//--------------------------
			//@@

			//float rgtab[20];
			//float cttab[20];
			//for (i = 0; i < 20; i++) {
				//rgtab[i] = cxt->ctTabRg[i];
				//cttab[i] = cxt->ctTab[i];
			//}

			//--------------------------
			//--------------------------
			//Preflash ct
			//preflash brightness
			for (i = 0; i < 1024; i++) {
				caliData->out.rTable_preflash[i] = 0;
				caliData->out.preflashBrightness[i] = 0;
			}

			float maxRg = 0;
			for (i = 0; i < caliData->numP1_alg; i++) {
				if (maxRg < gTab1[i])
					maxRg = gTab1[i];
				if (maxRg < rTab1[i])
					maxRg = rTab1[i];
			}
			for (i = 0; i < caliData->numP2_alg; i++) {
				if (maxRg < gTab2[i])
					maxRg = gTab2[i];
				if (maxRg < rTab2[i])
					maxRg = rTab2[i];
			}
			float ratMul;
			ratMul = maxRg / 65535.0;

			caliData->out.rgRatPf = ratMul;

			for (i = 0; i < caliData->numP1_alg; i++) {
				caliData->out.rPf1[i] = ae_round(rTab1[i] * 65535.0 / maxRg);
				caliData->out.gPf1[i] = ae_round(gTab1[i] * 65535.0 / maxRg);
			}
			for (i = 0; i < caliData->numP2_alg; i++) {
				caliData->out.rPf2[i] = ae_round(rTab2[i] * 65535.0 / maxRg);
				caliData->out.gPf2[i] = ae_round(gTab2[i] * 65535.0 / maxRg);
			}
			for (i = 0; i < 20; i++) {
				caliData->out.rgTab[i] = cxt->ctTabRg[i];
				caliData->out.ctTab[i] = cxt->ctTab[i];
				ISP_LOGD("cali out data 1, i:%d, cxt_ctTab:%f, out_ctTab:%f", i, cxt->ctTab[i], caliData->out.ctTab[i]);

			}

#if 0
			for (j = 0; j < caliData->numP2_alg; j++)
				for (i = 0; i < caliData->numP1_alg; i++) {
					int ind;
					ind = j * 32 + i;
					float ma1;
					float ma2;
					ma1 = caliData->maHwP1_alg[i];
					ma2 = caliData->maHwP1_alg[j];
					if (ma1 + ma2 <= caliData->mAMaxP12) {
						double rg;
						caliData->out.preflashCt[ind] = 0;
						if (gTab1[i] + gTab2[j] != 0) {
							rg = (rTab1[i] + rTab2[j]) / (gTab1[i] + gTab2[j]);
							caliData->out.preflashCt[ind] = interpCt(rgtab, cttab, 20, rg);
						}
						caliData->out.preflashBrightness[ind] = ae_round(gTab1[i] + gTab2[j]);
					} else {
						caliData->out.preflashCt[ind] = 0;
						caliData->out.preflashBrightness[ind] = 0;
					}
				}
#else
			for (j = 0; j < caliData->numP2_alg; j++)
				for (i = 0; i < caliData->numP1_alg; i++) {
					int ind;
					ind = j * 32 + i;
					float ma1;
					float ma2;
					ma1 = ae_round(caliData->maHwP1_alg[i]);
					ma2 = ae_round(caliData->maHwP1_alg[j]);
					if (ma1 + ma2 <= ae_round(caliData->mAMaxP12)) {
						//double rg;
						caliData->out.rTable_preflash[ind] = 0;

						int rPf1;
						int gPf1;
						int rPf2;
						int gPf2;
						rPf1 = caliData->out.rPf1[i];
						gPf1 = caliData->out.gPf1[i];
						rPf2 = caliData->out.rPf2[j];
						gPf2 = caliData->out.gPf2[j];
						double r;
						double g;
						//double b;
						r = rPf1+rPf2;
						g = gPf1 + gPf2;


						if (r > 0 && g > 0) {
							caliData->out.rTable_preflash[ind] = ae_round(1024 * g / r);
						}
						else {
							caliData->out.rTable_preflash[ind] = 0;
						}

						caliData->out.preflashBrightness[ind] = ae_round((gPf1 + gPf2) * caliData->out.rgRatPf);
					} else {
						caliData->out.rTable_preflash[ind] = 0;
						caliData->out.preflashBrightness[ind] = 0;
					}
				}
#endif
			for (i = 0; i < 1024; i++) {
				caliData->out.brightnessTable[i] = 0;
				caliData->out.rTable[i] = 0;
				caliData->out.bTable[i] = 0;
			}
			//mainflash brightness
			//mainflash r/b table
			for (i = 0; i < caliData->numM1_alg; i++)
				for (j = 0; j < caliData->numM2_alg; j++) {
					int ind;
					ind = j * 32 + i;
					caliData->out.brightnessTable[ind] = 0;
					if (gTab1Main[i] != -1 && gTab2Main[j] != -1) {
						caliData->out.brightnessTable[ind] = ae_round(gTab1Main[i] + gTab2Main[j]);
						double r;
						double g;
						double b;
						r = rTab1Main[i] + rTab2Main[j];
						g = gTab1Main[i] + gTab2Main[j];
						b = bTab1Main[i] + bTab2Main[j];

						bool rgbflag;
						rgbflag=(r > 0 && g > 0 && b > 0);

						if (rgbflag) {
							caliData->out.rTable[ind] = ae_round(1024 * g / r);
							caliData->out.bTable[ind] = ae_round(1024 * g / b);
						} else {
							caliData->out.brightnessTable[ind] = 0;
							caliData->out.rTable[ind] = 0;
							caliData->out.bTable[ind] = 0;
						}
					} else {
						caliData->out.brightnessTable[ind] = 0;
						caliData->out.rTable[ind] = 0;
						caliData->out.bTable[ind] = 0;
					}
				}
			//flash mask
			for (i = 0; i < 1024; i++) {
				caliData->out.flashMask[i] = 0;
			}
			for (j = 0; j < caliData->numM2_alg; j++)
				for (i = 0; i < caliData->numM1_alg; i++) {
					int ind;
					ind = j * 32 + i;
					float ma1;
					float ma2;
					ma1 = ae_round(caliData->maHwM1_alg[i]);
					ma2 = ae_round(caliData->maHwM2_alg[j]);
					if (ma1 + ma2 < ae_round(caliData->mAMaxM12))
						caliData->out.flashMask[ind] = 1;
					else
						caliData->out.flashMask[ind] = 0;
				}
			caliData->out.flashMask[0] = 0;

			{
				FILE *fp = NULL;
#ifdef WIN32
				fp = fopen("d:\\temp\\flash_led_brightness.bin", "wb");
#else
				fp = fopen("/data/vendor/cameraserver/flash_led_brightness.bin", "wb");
#endif
				struct flash_led_brightness led_bri;
				memset(&led_bri, 0, sizeof(struct flash_led_brightness));
				led_bri.levelNumber_pf1 = caliData->numP1_alg - 1;
				led_bri.levelNumber_pf2 = caliData->numP2_alg - 1;
				led_bri.levelNumber_mf1 = caliData->numM1_alg - 1;
				led_bri.levelNumber_mf2 = caliData->numM2_alg - 1;
				for (i = 0; i < led_bri.levelNumber_pf1; i++)
					led_bri.ledBrightness_pf1[i] = caliData->out.preflashBrightness[i + 1];
				for (i = 0; i < led_bri.levelNumber_pf2; i++)
					led_bri.ledBrightness_pf2[i] = caliData->out.preflashBrightness[(i + 1) * 32];
				for (i = 0; i < led_bri.levelNumber_mf1; i++)
					led_bri.ledBrightness_mf1[i] = caliData->out.brightnessTable[i + 1];
				for (i = 0; i < led_bri.levelNumber_mf2; i++)
					led_bri.ledBrightness_mf2[i] = caliData->out.brightnessTable[(i + 1) * 32];

				for (i = 0; i < led_bri.levelNumber_pf1; i++)
					led_bri.index_pf1[i] = caliData->indHwP1_alg[i + 1];
				for (i = 0; i < led_bri.levelNumber_pf2; i++)
					led_bri.index_pf2[i] = caliData->indHwP2_alg[i + 1];
				for (i = 0; i < led_bri.levelNumber_mf1; i++)
					led_bri.index_mf1[i] = caliData->indHwM1_alg[i + 1];
				for (i = 0; i < led_bri.levelNumber_mf2; i++)
					led_bri.index_mf2[i] = caliData->indHwM2_alg[i + 1];
				if(fp){
					fwrite(&led_bri, 1, sizeof(struct flash_led_brightness), fp);
					fclose(fp);
					fp = NULL;
				}
				struct flash_g_frames gf;
				memset(&gf, 0, sizeof(struct flash_g_frames));
				gf.levelNumber_pf1 = led_bri.levelNumber_pf1;
				gf.levelNumber_pf2 = led_bri.levelNumber_pf2;
				gf.levelNumber_mf1 = led_bri.levelNumber_mf1;
				gf.levelNumber_mf2 = led_bri.levelNumber_mf2;

				for (i = 0; i < 32; i++) {
					gf.index_pf1[i] = led_bri.index_pf1[i];
					gf.index_pf2[i] = led_bri.index_pf2[i];
					gf.index_mf1[i] = led_bri.index_mf1[i];
					gf.index_mf2[i] = led_bri.index_mf2[i];
				}
				for (j = 0; j < 15; j++)
					gf.g_off[j] = caliData->gFrame[0][j];
				gf.gain_off = caliData->gainTab[0];
				gf.shutter_off = caliData->expTab[0];
				for (i = 0; i < caliData->testIndAll; i++) {
					int ind;
					if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i]) {
						ind = caliData->ind1Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_mf1[ind][j] = caliData->gFrame[i][j];
						gf.gain_mf1[ind] = caliData->gainTab[i];
						gf.shutter_mf1[ind] = caliData->expTab[i];
					} else if (caliData->ind1Tab[i] != 0 && caliData->isMainTab[i] == 0) {
						ind = caliData->ind1Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_pf1[ind][j] = caliData->gFrame[i][j];
						gf.gain_pf1[ind] = caliData->gainTab[i];
						gf.shutter_pf1[ind] = caliData->expTab[i];
					} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i]) {
						ind = caliData->ind2Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_mf2[ind][j] = caliData->gFrame[i][j];
						gf.gain_mf2[ind] = caliData->gainTab[i];
						gf.shutter_mf2[ind] = caliData->expTab[i];
					} else if (caliData->ind2Tab[i] != 0 && caliData->isMainTab[i] == 0) {
						ind = caliData->ind2Tab[i] - 1;
						for (j = 0; j < 15; j++)
							gf.g_pf2[ind][j] = caliData->gFrame[i][j];
						gf.gain_pf2[ind] = caliData->gainTab[i];
						gf.shutter_pf2[ind] = caliData->expTab[i];
					}
				}
#ifdef WIN32
				fp = fopen("d:\\temp\\flash_g_frames.bin", "wb");
#else
				fp = fopen("/data/vendor/cameraserver/flash_g_frames.bin", "wb");
#endif
				if(fp){
					fwrite(&gf, 1, sizeof(struct flash_g_frames), fp);
					fclose(fp);
					fp = NULL;
				}
#ifdef WIN32
				fp = fopen("d:\\temp\\flash_g_frames.txt", "w+");

				if(fp){
					fprintf(fp, "levelNumber_pf1: %d\n", (int)gf.levelNumber_pf1);
					fprintf(fp, "levelNumber_pf2: %d\n", (int)gf.levelNumber_pf2);
					fprintf(fp, "levelNumber_mf1: %d\n", (int)gf.levelNumber_mf1);
					fprintf(fp, "levelNumber_mf2: %d\n", (int)gf.levelNumber_mf2);

					fprintf(fp, "index_pf1:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.index_pf1[i]);
					fprintf(fp, "\n");

					fprintf(fp, "index_pf2:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.index_pf2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "index_mf1:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.index_mf1[i]);
					fprintf(fp, "\n");

					fprintf(fp, "index_mf2:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.index_mf2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "shutter_off: %lf\n", (double)gf.shutter_off);
					fprintf(fp, "shutter_pf1:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%lf\t", (double)gf.shutter_pf1[i]);
					fprintf(fp, "\n");

					fprintf(fp, "shutter_pf2:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%lf\t", (double)gf.shutter_pf2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "shutter_mf1:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%lf\t", (double)gf.shutter_mf1[i]);
					fprintf(fp, "\n");

					fprintf(fp, "shutter_mf2:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%lf\t", (double)gf.shutter_mf2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "gain_off: %d\n", (int)gf.gain_off);
					fprintf(fp, "gain_pf1:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.gain_pf1[i]);
					fprintf(fp, "\n");

					fprintf(fp, "gain_pf2:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.gain_pf2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "gain_mf1:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.gain_mf1[i]);
					fprintf(fp, "\n");

					fprintf(fp, "gain_mf2:\n");
					for (i = 0; i < 32; i++)
						fprintf(fp, "%d\t", (int)gf.gain_mf2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "g_off:\n");
					for (j = 0; j < 15; j++)
						fprintf(fp, "%lf\t", (double)gf.g_off[j]);
					fprintf(fp, "\n");

					fprintf(fp, "g_pf1:\n");
					for (i = 0; i < 32; i++) {
						for (j = 0; j < 15; j++)
							fprintf(fp, "%lf\t", (double)gf.g_pf1[i][j]);
						fprintf(fp, "\n");
					}

					fprintf(fp, "g_pf2:\n");
					for (i = 0; i < 32; i++) {
						for (j = 0; j < 15; j++)
							fprintf(fp, "%lf\t", (double)gf.g_pf2[i][j]);
						fprintf(fp, "\n");
					}

					fprintf(fp, "g_mf1:\n");
					for (i = 0; i < 32; i++) {
						for (j = 0; j < 15; j++)
							fprintf(fp, "%lf\t", (double)gf.g_mf1[i][j]);
						fprintf(fp, "\n");
					}

					fprintf(fp, "g_mf2:\n");
					for (i = 0; i < 32; i++) {
						for (j = 0; j < 15; j++)
							fprintf(fp, "%lf\t", (double)gf.g_mf2[i][j]);
						fprintf(fp, "\n");
					}
					fclose(fp);
					fp = NULL;
				}
#endif
			}

			caliData->numM1_alg = 32;
			caliData->numM2_alg = 32;
			caliData->numP1_alg = 32;
			caliData->numP2_alg = 32;
			caliData->out.preflashLevelNum1 = caliData->numP1_alg;
			caliData->out.preflashLevelNum2 = caliData->numP2_alg;
			caliData->out.flashLevelNum1 = caliData->numM1_alg;
			caliData->out.flashLevelNum2 = caliData->numM2_alg;

			FILE *fp = NULL;

			int propValue[10];
			int propRet;
			propRet = get_prop_multi("persist.vendor.cam.isp.ae.fc_debug", 1, propValue);

			int debug1En = 0;
			int debug2En = 0;
			propRet = get_prop_multi("persist.vendor.cam.isp.ae.fc_debug", 1, propValue);
			if (propRet >= 1)
				debug1En = propValue[0];
			propRet = get_prop_multi("persist.vendor.cam.isp.ae.fc_debug2", 1, propValue);
			if (propRet >= 1)
				debug2En = propValue[0];

			if (debug1En == 1) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_frame_rgb.txt", "w+");
#else
				fp = fopen("/data/vendor/cameraserver/fc_frame_rgb.txt", "w+");
#endif
				if(fp){
					for (i = 0; i < caliData->testIndAll; i++) {


						char pfmf[][4]={"pf ","mf "};
						int index_pfmf;
						index_pfmf=(caliData->isMainTab[i] == 0)?0:1;
						fprintf(fp,"%s",pfmf[index_pfmf]);

						int led1;
						int led2;
						led1 = caliData->ind1Tab[i];
						led2 = caliData->ind2Tab[i];
						int led1_hw;
						int led2_hw;
						if (caliData->isMainTab[i] == 0) {
							led1_hw = caliData->indHwP1_alg[led1];
							led2_hw = caliData->indHwP2_alg[led2];
						} else {
							led1_hw = caliData->indHwM1_alg[led1];
							led2_hw = caliData->indHwM1_alg[led2];
						}
						fprintf(fp, "ind1,ind2: %d\t%d\n",
								//(int)caliData->ind1Tab[i],
								//(int)caliData->ind2Tab[i]);
								(int)led1_hw, (int)led2_hw);

						fprintf(fp, "expBase,gainBase,exp,gain: %d\t%d\t%d\t%d\n", (int)caliData->expTimeBase, (int)caliData->gainBase, (int)caliData->expTab[i], (int)caliData->gainTab[i]);
						int j;
						for (j = 0; j < 15; j++)
							fprintf(fp, "%lf\t%lf\t%lf\t\n", (double)caliData->rFrame[i][j], (double)caliData->gFrame[i][j], (double)caliData->bFrame[i][j]);
						fprintf(fp, "============\n\n");

					}
					fclose(fp);
					fp = NULL;
				}
			}
			if (debug2En == 1) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_debug.txt", "w+");
#else
				fp = fopen("/data/vendor/cameraserver/fc_debug.txt", "w+");
#endif
				if(fp){
					fprintf(fp, "\ndriver_ind\n");
					for (i = 0; i < caliData->numP1_alg; i++)
						fprintf(fp, "%d\t", caliData->out.driverIndexP1[i]);
					fprintf(fp, "\n");

					for (i = 0; i < caliData->numP2_alg; i++)
						fprintf(fp, "%d\t", caliData->out.driverIndexP2[i]);
					fprintf(fp, "\n");

					for (i = 0; i < caliData->numM1_alg; i++)
						fprintf(fp, "%d\t", caliData->out.driverIndexM1[i]);
					fprintf(fp, "\n");

					for (i = 0; i < caliData->numM2_alg; i++)
						fprintf(fp, "%d\t", caliData->out.driverIndexM2[i]);
					fprintf(fp, "\n");

					fprintf(fp, "\nmask\n");
					for (j = 0; j < caliData->numM2_alg; j++) {
						for (i = 0; i < caliData->numM1_alg; i++) {
							int ind;
							ind = j * caliData->numM1_alg + i;
							fprintf(fp, "%d\t", (int)caliData->out.flashMask[ind]);
						}
						fprintf(fp, "\n");
					}
					fprintf(fp, "\nbrightness\n");
					for (j = 0; j < caliData->numM2_alg; j++) {
						for (i = 0; i < caliData->numM1_alg; i++) {
							int ind;
							ind = j * caliData->numM1_alg + i;
							fprintf(fp, "%d\t", (int)caliData->out.brightnessTable[ind]);
						}
						fprintf(fp, "\n");
					}
					fprintf(fp, "\nr tab\n");
					for (j = 0; j < caliData->numM2_alg; j++) {
						for (i = 0; i < caliData->numM1_alg; i++) {
							int ind;
							ind = j * caliData->numM1_alg + i;
							fprintf(fp, "%d\t", (int)caliData->out.rTable[ind]);
						}
						fprintf(fp, "\n");
					}
					fprintf(fp, "\nb tab\n");
					for (j = 0; j < caliData->numM2_alg; j++) {
						for (i = 0; i < caliData->numM1_alg; i++) {
							int ind;
							ind = j * caliData->numM1_alg + i;
							fprintf(fp, "%d\t", (int)caliData->out.bTable[ind]);
						}
						fprintf(fp, "\n");
					}
					fprintf(fp, "\npre bright\n");
					for (j = 0; j < caliData->numP2_alg; j++) {
						for (i = 0; i < caliData->numP1_alg; i++) {
							int ind;
							ind = j * caliData->numP1_alg + i;
							fprintf(fp, "%d\t", (int)caliData->out.preflashBrightness[ind]);
						}
						fprintf(fp, "\n");
					}
					fprintf(fp, "\npre rtab\n");
					for (j = 0; j < caliData->numP2_alg; j++) {
						for (i = 0; i < caliData->numP1_alg; i++) {
							int ind;
							ind = j * caliData->numP1_alg + i;
							fprintf(fp, "%d\t", (int)caliData->out.rTable_preflash[ind]);
						}
						fprintf(fp, "\n");
					}
					fclose(fp);
					fp = NULL;
				}
			}

			if (debug1En == 1) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_debug.bin", "wb");
#else
				fp = fopen("/data/vendor/cameraserver/fc_debug.bin", "wb");
#endif
				if(fp){
					fwrite(caliData, 1, sizeof(struct FCData), fp);
					fclose(fp);
					fp = NULL;
				}
			}
			if (caliData->out.error != 0) {
#ifdef WIN32
				fp = fopen("d:\\temp\\fc_error.txt", "w+");
#else
				fp = fopen("/data/vendor/cameraserver/fc_error.txt", "w+");
#endif
				if (fp && caliData->out.error == FlashCali_too_close) {
					fprintf(fp, "error: chart to camera is to close!\n");
					fclose(fp);
					fp = NULL;
				}
				if(fp && caliData->out.error != FlashCali_too_close)
{
					fclose(fp);
					fp = NULL;
				}
			} else {
#ifdef WIN32
				fp = fopen("d:\\temp\\flashcalibration.bin", "wb");
#else
				fp = fopen("/data/vendor/cameraserver/flashcalibration.bin", "wb");
#endif
				if(fp){
					ISP_LOGD("cali out data 2,out_ctTab:%f", caliData->out.ctTab[0]);
					fwrite(&caliData->out, 1, sizeof(struct flash_calibration_data), fp);
					fclose(fp);
					fp = NULL;
				}
			}

			free(caliData);
			caliState = FlashCali_none;

		} else {
			//none
#ifdef WIN32
			printf("state=none\n");
#endif
			ISP_LOGD("flash calibration done!!\n");
		}

		if (caliState < FlashCali_end) {
			ISP_LOGD("qqfc exp=%d %d", (int)caliData->expTime, (int)caliData->gain);
			//int lineTime = cxt->cur_status.adv_param.cur_ev_setting.line_time;
			cur_status->adv_param.mode_param.mode = 3;
			cxt->cur_status.adv_param.mode_param.value.ae_idx = 0;
			//cxt->cur_status.adv_param.mode_param.exp_line = caliData->expTime / lineTime;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[0] = (cmr_u32)caliData->expTime;
			cxt->cur_status.adv_param.mode_param.value.exp_gain[1] = (cmr_u32)caliData->gain;
		}
	}
	frameCount++;
}

static void _set_led2(struct ae_ctrl_cxt *cxt)
{
	struct ae_lib_calc_in *cur_status = &cxt->cur_status;
	int propValue[10];
	int ret;
	static int led_onOff = 0;
	static int led_isMain = 0;
	static int led_led1 = 0;
	static int led_led2 = 0;

	ret = get_prop_multi("persist.vendor.cam.isp.ae.fc_led", 4, propValue);
	if (ret > 0) {
		if (led_onOff == propValue[0] && led_isMain == propValue[1] && led_led1 == propValue[2] && led_led2 == propValue[3]) {
		} else {
			led_onOff = propValue[0];
			led_isMain = propValue[1];
			led_led1 = propValue[2];
			led_led2 = propValue[3];
			control_led(cxt, led_onOff, led_isMain, led_led1, led_led2);
			ISP_LOGD("qqfc led control %d %d %d %d", led_onOff, led_isMain, led_led1, led_led2);
		}
	}

	static int lock_lock = 0;
	static int exp_exp_line = 0;
	static int exp_exp_time = 0;
	static int exp_dummy = 0;
	static int exp_isp_gain = 0;
	static int exp_sensor_gain = 0;
	ret = get_prop_multi("persist.vendor.cam.isp.ae.fc_exp", 5, propValue);
	if (ret > 0) {
		if (exp_exp_line == propValue[0] && exp_exp_time == propValue[1] && exp_dummy == propValue[2] && exp_isp_gain == propValue[3] && exp_sensor_gain == propValue[4]) {
		} else {
			exp_exp_line = propValue[0];
			exp_exp_time = propValue[1];
			exp_dummy = propValue[2];
			exp_isp_gain = propValue[3];
			exp_sensor_gain = propValue[4];
			cur_status->adv_param.lock = lock_lock;
			cur_status->adv_param.mode_param.mode = 3;
			cur_status->adv_param.cur_ev_setting.ae_idx = 0;
			cur_status->adv_param.cur_ev_setting.exp_line = exp_exp_line;
			cur_status->adv_param.cur_ev_setting.ae_gain = exp_isp_gain * exp_sensor_gain / 4096;
			ISP_LOGD("qqfc set exp %d %d %d %d %d", exp_exp_line, exp_exp_time, exp_dummy, exp_isp_gain, exp_sensor_gain);
		}
	}

	ret = get_prop_multi("persist.vendor.cam.isp.ae.fc_lock", 1, propValue);
	if (ret > 0) {
		if (lock_lock == propValue[0]) {
		} else {
			lock_lock = propValue[0];
			if (lock_lock == 1) {
				//_set_pause(cxt);
				cxt->cur_status.adv_param.lock = AE_STATE_LOCKED;
				ISP_LOGD("qqfc lock");
			}

		}
	}

	static int lock_unlock = 0;
	ret = get_prop_multi("persist.vendor.cam.isp.ae.fc_unlock", 1, propValue);
	if (ret > 0) {
		if (lock_unlock == propValue[0]) {
		} else {
			lock_unlock = propValue[0];
			if (lock_unlock == 1) {
				// _set_restore_cnt(cxt);
				cxt->cur_status.adv_param.lock = AE_STATE_NORMAL;
				ISP_LOGD("qqfc unlock");
			}
		}
	}

	static int exp2 = 0;
	ret = get_prop_multi("persist.vendor.cam.isp.ae.fc_exp2", 1, propValue);
	if (ret > 0) {
		if (exp2 == propValue[0]) {
		} else {
			exp2 = propValue[0];
			if (exp2 == 1) {
				cur_status->adv_param.mode_param.mode = 3;
				cur_status->adv_param.cur_ev_setting.ae_idx = 0;
				cur_status->adv_param.cur_ev_setting.exp_line = 800;
				cur_status->adv_param.cur_ev_setting.ae_gain = 256;
			}
		}
	}

	static int exp1 = 0;
	ret = get_prop_multi("persist.vendor.cam.isp.ae.fc_exp1", 1, propValue);
	if (ret > 0) {
		if (exp1 == propValue[0]) {
		} else {
			exp1 = propValue[0];
			if (exp1 == 1) {
				cur_status->adv_param.mode_param.mode = 3;
				cur_status->adv_param.cur_ev_setting.ae_idx = 0;
				cur_status->adv_param.cur_ev_setting.exp_line = 100;
				cur_status->adv_param.cur_ev_setting.ae_gain = 256;
				ISP_LOGD("qqfc exp1 set");
			}
		}
	}

}

void flash_calibration_script(cmr_handle ae_cxt)
{
	char str[PROPERTY_VALUE_MAX];
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)ae_cxt;
	memset((void *)&str[0], 0, sizeof(str));

	property_get("persist.vendor.cam.isp.ae.fc_stript", str, "");
	if (!strcmp(str, "on")) {
		flashCalibration(cxt);
		_set_led2(cxt);
	}
}
