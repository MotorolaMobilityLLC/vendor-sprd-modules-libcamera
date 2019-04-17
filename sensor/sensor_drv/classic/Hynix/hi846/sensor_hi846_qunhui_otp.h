#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include <cutils/properties.h>

#define MODULE_NAME "qunhui" // module vendor name
#define MODULE_ID_hi846_qunhui                                                 \
    0x0057 // hi846: sensor P/N;  qunhui: module vendor
#define LSC_PARAM_QTY 240
#define I2C_SLAVE_ADDR 0x42 /* 8bit slave address*/
#define AF_OTP_SUPPORT 0

struct otp_info_t {
    uint16_t flag;
    uint16_t module_id;
    uint16_t lens_id;
    uint16_t vcm_id;
    uint16_t vcm_driver_id;
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t rg_ratio_current;
    uint16_t bg_ratio_current;
    uint16_t gbgr_ratio_current;
    uint16_t rg_ratio_typical;
    uint16_t bg_ratio_typical;
    uint16_t gbgr_ratio_typical;
    uint16_t r_current;
    uint16_t gr_current;
    uint16_t gb_current;
    uint16_t b_current;
    uint16_t r_typical;
    uint16_t b_typical;
    uint16_t gr_typical;
    uint16_t gb_typical;
    uint16_t g_typical;
    uint16_t vcm_dac_start;
    uint16_t vcm_dac_inifity;
    uint16_t vcm_dac_macro;
    uint16_t lsc_param[LSC_PARAM_QTY];
};
static struct otp_info_t s_hi846_qunhui_otp_info;

#define RG_RATIO_TYPICAL_hi846_qunhui 0x104 // 0x108
#define BG_RATIO_TYPICAL_hi846_qunhui 0x140 // 0x142
#define GBGR_RATIO_TYPICAL_hi846_qunhui 0x200
#define R_TYPICAL_hi846_qunhui 0x0000
#define G_TYPICAL_hi846_qunhui 0x0000
#define B_TYPICAL_hi846_qunhui 0x0000

static uint16_t Sensor_readreg8bits(cmr_handle handle, uint16_t addr) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    uint8_t cmd_val[5] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;

    slave_addr = I2C_SLAVE_ADDR >> 1;

    uint16_t reg_value = 0;
    cmd_val[0] = addr >> 8;
    cmd_val[1] = addr & 0xff;
    cmd_len = 2;
    ret_value = hw_sensor_read_i2c(sns_drv_cxt->hw_handle, slave_addr,
                                   (uint8_t *)&cmd_val[0], cmd_len);
    if (SENSOR_SUCCESS == ret_value) {
        reg_value = cmd_val[0];
    }
    return reg_value;
}
static uint32_t Sensor_writereg8bits(cmr_handle handle, uint16_t addr,
                                     uint8_t val) {
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    uint8_t cmd_val[5] = {0x00};
    uint16_t slave_addr = 0;
    uint16_t cmd_len = 0;
    uint32_t ret_value = SENSOR_SUCCESS;

    slave_addr = I2C_SLAVE_ADDR >> 1;

    cmd_val[0] = addr >> 8;
    cmd_val[1] = addr & 0xff;
    cmd_val[2] = val;
    cmd_len = 3;
    ret_value = hw_sensor_write_i2c(sns_drv_cxt->hw_handle, slave_addr,
                                    (uint8_t *)&cmd_val[0], cmd_len);

    return ret_value;
}

cmr_u8 hi846_Sensor_OTP_read(cmr_handle handle, uint16_t otp_addr) {
    // Sensor_WriteReg(0x070a, otp_addr); //start address
    // Sensor_WriteReg(0x0702, 0x0100); //single read
    // return Sensor_ReadReg(0x0708)>>8; //OTP data read
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    Sensor_writereg8bits(handle, 0x070a,
                         (otp_addr >> 8) & 0xff);            // start address
    Sensor_writereg8bits(handle, 0x070b, (otp_addr & 0xff)); // start address L
    Sensor_writereg8bits(handle, 0x0702, 0x01);              // single read

    return Sensor_readreg8bits(handle, 0x0708); // OTP data read
}
static void hi846_qunhui_enable_awb_otp(void) {
    /*TODO enable awb otp update*/
    // Sensor_writereg8bits(0x021c, 0x04 | Sensor_readreg8bits(0x021c));
}

static uint32_t hi846_qunhui_update_awb(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    hi846_qunhui_enable_awb_otp();

    /*TODO*/
    uint32_t R_gain, B_gain, G_gain;
    // apply OTP WB Calibration
    if (otp_info->flag & 0x40) {
        R_gain =
            (otp_info->rg_ratio_typical * 0x0200) / otp_info->rg_ratio_current;
        B_gain =
            (otp_info->bg_ratio_typical * 0x0200) / otp_info->bg_ratio_current;
        G_gain = 0x0200;

        if (R_gain < B_gain) {
            if (R_gain < 0x0200) {
                B_gain = (0x0200 * B_gain) / R_gain;
                G_gain = (0x0200 * G_gain) / R_gain;
                R_gain = 0x0200;
            }
        } else {
            if (B_gain < 0x0200) {
                R_gain = (0x0200 * R_gain) / B_gain;
                G_gain = (0x0200 * G_gain) / B_gain;
                B_gain = 0x0200;
            }
        }

        SENSOR_PRINT("before G_gain=0x%x,R_gain=0x%x,B_gain=0x%x\n", G_gain,
                     R_gain, B_gain);

        Sensor_writereg8bits(handle, 0x0078, G_gain >> 8);
        Sensor_writereg8bits(handle, 0x0079, G_gain & 0xff);
        Sensor_writereg8bits(handle, 0x007a, G_gain >> 8);
        Sensor_writereg8bits(handle, 0x007b, G_gain & 0xff);
        Sensor_writereg8bits(handle, 0x007c, R_gain >> 8);
        Sensor_writereg8bits(handle, 0x007d, R_gain & 0xff);
        Sensor_writereg8bits(handle, 0x007e, B_gain >> 8);
        Sensor_writereg8bits(handle, 0x007f, B_gain & 0xff);
    } else {
        SENSOR_PRINT("E");
    }
    SENSOR_PRINT("after R_gain=0x%x,B_gain=0x%x,Gr_gain=0x%x,Gb_gain=0x%x",
                 hi846_Sensor_OTP_read(handle, 0x007c),
                 hi846_Sensor_OTP_read(handle, 0x007e),
                 hi846_Sensor_OTP_read(handle, 0x0078),
                 hi846_Sensor_OTP_read(handle, 0x007a));

    return rtn;
}

static void hi846_qunhui_enable_lsc_otp(void) { /*TODO enable lsc otp update*/
}

static uint32_t hi846_qunhui_update_lsc(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);

    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    hi846_qunhui_enable_lsc_otp();

    /*TODO
    if (otp_info->flag & 0x10) {
        Sensor_writereg8bits(handle, 0x0a05,
                             (0x10) | Sensor_readreg8bits(handle, 0x0a05));
        Sensor_writereg8bits(handle, 0x021c,
                             (0x02) | Sensor_readreg8bits(handle, 0x021c));
    } else {
        Sensor_writereg8bits(handle, 0x0a05,
                             (0xef) & Sensor_readreg8bits(handle, 0x0a05));
        Sensor_writereg8bits(handle, 0x021c,
                             (0xfd) & Sensor_readreg8bits(handle, 0x021c));
    }
*/
    return rtn;
}

static uint32_t hi846_qunhui_test_awb(void *param_ptr) {
    uint32_t flag = 1;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.otp.awb", value, "on");

    if (!strcmp(value, "on")) {
        SENSOR_PRINT("apply awb otp normally!");
        otp_info->rg_ratio_typical = RG_RATIO_TYPICAL_hi846_qunhui;
        otp_info->bg_ratio_typical = BG_RATIO_TYPICAL_hi846_qunhui;
        otp_info->gbgr_ratio_typical = GBGR_RATIO_TYPICAL_hi846_qunhui;
        otp_info->r_typical = otp_info->r_typical;
        otp_info->g_typical = otp_info->g_typical;
        otp_info->b_typical = otp_info->b_typical;

    } else if (!strcmp(value, "test")) {
        SENSOR_PRINT("apply awb otp on test mode!");
        otp_info->rg_ratio_typical = RG_RATIO_TYPICAL_hi846_qunhui * 1.5;
        otp_info->bg_ratio_typical = BG_RATIO_TYPICAL_hi846_qunhui * 1.5;
        otp_info->gbgr_ratio_typical = GBGR_RATIO_TYPICAL_hi846_qunhui * 1.5;
        otp_info->r_typical = otp_info->g_typical;
        otp_info->g_typical = otp_info->g_typical;
        otp_info->b_typical = otp_info->g_typical;

    } else {
        SENSOR_PRINT("without apply awb otp!");
        flag = 0;
    }
    return flag;
}

static uint32_t hi846_qunhui_test_lsc(void) {
    uint32_t flag = 1;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.otp.lsc", value, "on");

    if (!strcmp(value, "on")) {
        SENSOR_PRINT("apply lsc otp normally!");
        flag = 1;
    } else {
        SENSOR_PRINT("without apply lsc otp!");
        flag = 0;
    }
    return flag;
}

static uint32_t hi846_qunhui_read_otp_info(cmr_handle handle, void *param_ptr,
                                           void *p_data) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;

#if AF_OTP_SUPPORT
    /*****************************covert af data**************************/
    cmr_u8 af_check_flag = 0;
    SENSOR_VAL_T *p_val = (SENSOR_VAL_T *)p_data;
    SENSOR_IC_CHECK_PTR(p_val);
    SENSOR_IC_CHECK_PTR(p_data);

    struct sensor_otp_section_info *module_info_data = NULL;
    module_info_data = malloc(sizeof(struct sensor_otp_section_info));
    if (NULL == module_info_data) {
        SENSOR_PRINT("malloc otp module_info_data failed.\n");
        return CMR_CAMERA_FAIL;
    }
    cmr_bzero(module_info_data, sizeof(*module_info_data));

    struct sensor_otp_section_info *af_rdm_otp_data = NULL;
    af_rdm_otp_data = malloc(sizeof(struct sensor_otp_section_info));
    if (NULL == af_rdm_otp_data) {
        SENSOR_PRINT("malloc otp af_rdm_otp_data failed.\n");
        return CMR_CAMERA_FAIL;
    }
    cmr_bzero(af_rdm_otp_data, sizeof(*af_rdm_otp_data));

    struct sensor_single_otp_info *single_otp = NULL;
    struct sensor_otp_cust_info *convert_data = NULL;
    convert_data = malloc(sizeof(struct sensor_otp_cust_info));
    if (NULL == convert_data) {
        SENSOR_PRINT("malloc otp convert_data failed.\n");
        return CMR_CAMERA_FAIL;
    }
    cmr_bzero(convert_data, sizeof(*convert_data));
#endif

    otp_info->rg_ratio_typical = RG_RATIO_TYPICAL_hi846_qunhui;
    otp_info->bg_ratio_typical = BG_RATIO_TYPICAL_hi846_qunhui;
    otp_info->gbgr_ratio_typical = GBGR_RATIO_TYPICAL_hi846_qunhui;
    otp_info->r_typical = R_TYPICAL_hi846_qunhui;
    otp_info->g_typical = G_TYPICAL_hi846_qunhui;
    otp_info->b_typical = B_TYPICAL_hi846_qunhui;
    /*TODO*/
    uint16_t flag, start_address, wb_rg_golden, wb_bg_golden, wb_gg_golden,
        status;
    uint32_t checksum, i, checksum_reg;

    /*OTP Initial Setting*/
    Sensor_writereg8bits(handle, 0x0a02, 0x01); // fast sleep On
    Sensor_writereg8bits(handle, 0x0a00, 0x00); // stand by On
    usleep(10 * 1000);
    Sensor_writereg8bits(handle, 0x0f02, 0x00); // pll disable
    Sensor_writereg8bits(handle, 0x071a, 0x01); // CP TRI_H;
    Sensor_writereg8bits(handle, 0x071b, 0x09); // IPGM TRIM_H
    Sensor_writereg8bits(handle, 0x0d04, 0x01); // Fsync Output enable
    Sensor_writereg8bits(handle, 0x0d00, 0x07); // Fsync Output Drivability
    Sensor_writereg8bits(handle, 0x003e, 0x10); // OTP R/W
    Sensor_writereg8bits(handle, 0x070f, 0x05); // OTP data rewrite
    usleep(10 * 1000);
    Sensor_writereg8bits(handle, 0x0a00, 0x01); ////stand by Off

    /////////////////////* module information*///////////////////////
    /*flag check*/
    otp_info->flag = 0x00;
    flag = 0x00;
    status = 0x00;
    flag = hi846_Sensor_OTP_read(handle, 0x0201);
    SENSOR_PRINT("module info flag=0x%x\n", flag);
    if (0 == flag) {
        status = hi846_Sensor_OTP_read(handle, 0x0a00);
        SENSOR_PRINT("0x0a00 = 0x%x\n", status);
        if (0x01 != status) {
            Sensor_writereg8bits(handle, 0x0a00, 0x01);
            usleep(1000);
            status = hi846_Sensor_OTP_read(handle, 0x0a00);
            SENSOR_PRINT("0x0a00 = 0x%x\n", status);
        }
    }

    if (flag == 0x01) {
        start_address = 0x0202; // group1
        checksum_reg = 0x0212;
    } else if (flag == 0x13) {
        start_address = 0x0213; // group2
        checksum_reg = 0x0223;
    } else if (flag == 0x37) {
        start_address = 0x0224; // group3
        checksum_reg = 0x0234;
    } else {
        start_address = 0x00;
        SENSOR_PRINT("no module information in otp\n");
    }

    /*checksum*/
    checksum = 0;
    for (i = 0; i < 10; i++) {
        checksum = checksum + hi846_Sensor_OTP_read(handle, start_address + i);
    }
    checksum = (checksum % 0xff) + 1;
    SENSOR_PRINT("module information checksum=0x%x\n", checksum);

    if (checksum == hi846_Sensor_OTP_read(handle, checksum_reg)) {
        SENSOR_PRINT("module information checksum success\n");
        otp_info->flag |= 0x80;
    } else {
        SENSOR_PRINT("module information checksum error\n");
        otp_info->flag &= 0x7f;
    }
    /*read module information*/
    otp_info->module_id = hi846_Sensor_OTP_read(handle, start_address);
    otp_info->year = hi846_Sensor_OTP_read(handle, start_address + 2);
    otp_info->month = hi846_Sensor_OTP_read(handle, start_address + 3);
    otp_info->day = hi846_Sensor_OTP_read(handle, start_address + 4);
    otp_info->lens_id = hi846_Sensor_OTP_read(handle, start_address + 6);
    /////////////////////* module information end*/////////////////
    /////////////////////* lsc information*////////////////////////
    /*flag check*/
    flag = hi846_Sensor_OTP_read(handle, 0x0235);
    SENSOR_PRINT("lsc flag=0x%x\n", flag);

    if (flag == 0x01) {
        start_address = 0x0236; // group1
        checksum_reg = 0x0598;
    } else if (flag == 0x13) {
        start_address = 0x0599; // group2
        checksum_reg = 0x08fb;
    } else if (flag == 0x37) {
        start_address = 0x08fc; // group3
        checksum_reg = 0x0c5e;
    } else {
        start_address = 0x00;
        SENSOR_PRINT("no lsc information in otp\n");
    }

    /*checksum*/
    checksum = 0;
    for (i = 0; i < 858; i++) {
        checksum = checksum + hi846_Sensor_OTP_read(handle, start_address + i);
    }
    checksum = (checksum % 0xff) + 1;
    SENSOR_PRINT("lsc information checksum=0x%x\n", checksum);
    if (checksum == hi846_Sensor_OTP_read(handle, checksum_reg)) {
        SENSOR_PRINT("lsc information checksum success\n");
        otp_info->flag |= 0x10;
    } else {
        SENSOR_PRINT("lsc information checksum error.\n");
        otp_info->flag &= 0xef;
    }
    /*read lsc information*/

    /////////////////////* lsc information end*/////////////////////
    /////////////////////* awb information*///////////////////////
    /*flag check*/
    flag = hi846_Sensor_OTP_read(handle, 0x0c5f);
    SENSOR_PRINT("awb flag=0x%x\n", flag);

    if (flag == 0x01) {
        start_address = 0x0c60; // group1
        checksum_reg = 0x0c7d;
    } else if (flag == 0x13) {
        start_address = 0x0c7e; // group2
        checksum_reg = 0x0c9b;
    } else if (flag == 0x37) {
        start_address = 0x0c9c; // group3
        checksum_reg = 0x0cb9;
    } else {
        start_address = 0x00;
        SENSOR_PRINT("no awb information in otp\n");
    }

    /*checksum*/
    checksum = 0;
    for (i = 0; i < 28; i++) {
        checksum = checksum + hi846_Sensor_OTP_read(handle, start_address + i);
    }
    checksum = (checksum % 0xff) + 1;
    SENSOR_PRINT("awb information checksum=0x%x\n", checksum);

    if (checksum == hi846_Sensor_OTP_read(handle, checksum_reg)) {
        SENSOR_PRINT("awb information checksum success\n");
        otp_info->flag |= 0x40;
    } else {
        SENSOR_PRINT("awb information checksum error\n");
        otp_info->flag |= 0xbf;
    }
    /*read awb information*/
    otp_info->rg_ratio_current =
        (hi846_Sensor_OTP_read(handle, start_address) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 1);
    otp_info->bg_ratio_current =
        (hi846_Sensor_OTP_read(handle, start_address + 2) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 3);
    otp_info->gbgr_ratio_current =
        (hi846_Sensor_OTP_read(handle, start_address + 4) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 5);
    wb_rg_golden = (hi846_Sensor_OTP_read(handle, start_address + 6) << 8) +
                   hi846_Sensor_OTP_read(handle, start_address + 7);
    wb_bg_golden = (hi846_Sensor_OTP_read(handle, start_address + 8) << 8) +
                   hi846_Sensor_OTP_read(handle, start_address + 9);
    wb_gg_golden = (hi846_Sensor_OTP_read(handle, start_address + 10) << 8) +
                   hi846_Sensor_OTP_read(handle, start_address + 11);

    otp_info->r_current =
        (hi846_Sensor_OTP_read(handle, start_address + 12) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 13);
    otp_info->b_current =
        (hi846_Sensor_OTP_read(handle, start_address + 14) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 15);
    otp_info->gr_current =
        (hi846_Sensor_OTP_read(handle, start_address + 16) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 17);
    otp_info->gb_current =
        (hi846_Sensor_OTP_read(handle, start_address + 18) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 19);
    otp_info->r_typical =
        (hi846_Sensor_OTP_read(handle, start_address + 20) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 21);
    otp_info->b_typical =
        (hi846_Sensor_OTP_read(handle, start_address + 22) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 23);
    otp_info->gr_typical =
        (hi846_Sensor_OTP_read(handle, start_address + 24) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 25);
    otp_info->gb_typical =
        (hi846_Sensor_OTP_read(handle, start_address + 26) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 27);

    SENSOR_PRINT("wb_rg_golden=0x%x", wb_rg_golden);
    SENSOR_PRINT("wb_bg_golden=0x%x", wb_bg_golden);
    SENSOR_PRINT("wb_gg_golden=0x%x", wb_gg_golden);
    if (wb_rg_golden && wb_bg_golden & wb_gg_golden) {
        otp_info->rg_ratio_typical = wb_rg_golden;
        otp_info->bg_ratio_typical = wb_bg_golden;
        otp_info->gbgr_ratio_typical = wb_gg_golden;
    }
/////////////////////* awb information end*//////////////////
#if AF_OTP_SUPPORT
    /////////////////////* af information*///////////////////////
    /*flag check*/
    flag = hi846_Sensor_OTP_read(handle, 0x0cba);
    SENSOR_PRINT("af flag=0x%x\n", flag);

    if (flag == 0x01) {
        start_address = 0x0cbb; // group1
        checksum_reg = 0x0cc0;
    } else if (flag == 0x13) {
        start_address = 0x0cc1; // group2
        checksum_reg = 0x0cc6;
    } else if (flag == 0x37) {
        start_address = 0x0cc7; // group3
        checksum_reg = 0x0ccc;
    } else {
        start_address = 0x00;
        SENSOR_PRINT("no af information in otp\n");
    }

    /*checksum*/
    checksum = 0;
    for (i = 0; i < 5; i++) {
        checksum = checksum + hi846_Sensor_OTP_read(handle, start_address + i);
    }
    checksum = (checksum % 0xff) + 1;
    SENSOR_PRINT("af information checksum=0x%x\n", checksum);

    if (checksum == hi846_Sensor_OTP_read(handle, checksum_reg)) {
        af_check_flag = 1;
        SENSOR_PRINT("af information checksum success\n");
        otp_info->flag |= 0x20;
    } else {
        af_check_flag = 0;
        SENSOR_PRINT("af information checksum error\n");
        otp_info->flag &= 0xdf;
    }

    otp_info->flag = otp_info->flag | 0x04;
    /*read af information*/
    // otp read af code value

    otp_info->vcm_dac_inifity =
        (hi846_Sensor_OTP_read(handle, start_address + 1) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 2);
    otp_info->vcm_dac_macro =
        (hi846_Sensor_OTP_read(handle, start_address + 3) << 8) +
        hi846_Sensor_OTP_read(handle, start_address + 4);

    /*****************************covert af data**************************/

    cmr_u8 *hi846_af_otp_data = malloc((sizeof(cmr_u8) * 4));
    cmr_u8 *hi846_module_info =
        malloc((sizeof(cmr_u8) * 6)); // spread otp version,not change
    // macro position(High)
    cmr_bzero(hi846_module_info, (sizeof(cmr_u8) * 6));
    hi846_module_info[5] = 1;

    hi846_af_otp_data[0] = hi846_Sensor_OTP_read(handle, start_address + 2);
    hi846_af_otp_data[1] = hi846_Sensor_OTP_read(handle, start_address + 1);

    hi846_af_otp_data[2] = hi846_Sensor_OTP_read(handle, start_address + 4);
    hi846_af_otp_data[3] = hi846_Sensor_OTP_read(handle, start_address + 3);

    SENSOR_PRINT("cover_af_data[0]=0x%x,[1]=0x%x,[2]=0x%x,[3]=0x%x\n",
                 hi846_af_otp_data[0], hi846_af_otp_data[1],
                 hi846_af_otp_data[2], hi846_af_otp_data[3]);

    if (1 == af_check_flag) {
        af_rdm_otp_data->rdm_info.data_addr = hi846_af_otp_data;
        af_rdm_otp_data->rdm_info.data_size = 4;
        module_info_data->rdm_info.data_addr = hi846_module_info;
        module_info_data->rdm_info.data_size = 6;

        single_otp = &convert_data->single_otp;
        convert_data->otp_vendor = OTP_VENDOR_SINGLE;

        single_otp->af_info = af_rdm_otp_data;
        single_otp->module_info = module_info_data;

        p_val->pval = convert_data;
        p_val->type = SENSOR_VAL_TYPE_PARSE_OTP;
        SENSOR_PRINT("out");
    } else {
        SENSOR_PRINT("AF checksum err ,can't cover af data!!!\n");
    }
#endif
    /*back to display mode*/
    Sensor_writereg8bits(handle, 0x0a00, 0x00); // stand by On
    usleep(10 * 1000);
    Sensor_writereg8bits(handle, 0x003e, 0x00); // display mode

    /*print otp information*/
    SENSOR_PRINT("flag=0x%x", otp_info->flag);
    SENSOR_PRINT("module_id=0x%x", otp_info->module_id);
    SENSOR_PRINT("lens_id=0x%x", otp_info->lens_id);
    SENSOR_PRINT("vcm_id=0x%x", otp_info->vcm_id);
    SENSOR_PRINT("vcm_id=0x%x", otp_info->vcm_id);
    SENSOR_PRINT("vcm_driver_id=0x%x", otp_info->vcm_driver_id);
    SENSOR_PRINT("data=%d-%d-%d", otp_info->year, otp_info->month,
                 otp_info->day);
    SENSOR_PRINT("rg_ratio_current=0x%x", otp_info->rg_ratio_current);
    SENSOR_PRINT("bg_ratio_current=0x%x", otp_info->bg_ratio_current);
    SENSOR_PRINT("gbgr_ratio_current=0x%x", otp_info->gbgr_ratio_current);
    SENSOR_PRINT("rg_ratio_typical=0x%x", otp_info->rg_ratio_typical);
    SENSOR_PRINT("bg_ratio_typical=0x%x", otp_info->bg_ratio_typical);
    SENSOR_PRINT("gbgr_ratio_typical=0x%x", otp_info->gbgr_ratio_typical);
    SENSOR_PRINT("r_current=0x%x", otp_info->r_current);
    SENSOR_PRINT("gr_current=0x%x", otp_info->gr_current);
    SENSOR_PRINT("gb_current=0x%x", otp_info->gb_current);
    SENSOR_PRINT("b_current=0x%x", otp_info->b_current);
    SENSOR_PRINT("r_typical=0x%x", otp_info->r_typical);
    SENSOR_PRINT("gr_typical=0x%x", otp_info->gr_typical);
    SENSOR_PRINT("gb_typical=0x%x", otp_info->gb_typical);
    SENSOR_PRINT("b_typical=0x%x", otp_info->b_typical);
    SENSOR_PRINT("vcm_dac_start=0x%x", otp_info->vcm_dac_start);
    SENSOR_PRINT("vcm_dac_inifity=0x%x", otp_info->vcm_dac_inifity);
    SENSOR_PRINT("vcm_dac_macro=0x%x", otp_info->vcm_dac_macro);

    /*update awb*/
    if (hi846_qunhui_test_awb(otp_info)) {
        rtn = hi846_qunhui_update_awb(handle, param_ptr);
        if (rtn != SENSOR_SUCCESS) {
            SENSOR_PRINT_ERR("OTP awb apply error!");
            return rtn;
        }
    } else {
        rtn = SENSOR_SUCCESS;
    }

    /*update lsc*/
    if (hi846_qunhui_test_lsc()) {
        rtn = hi846_qunhui_update_lsc(handle, param_ptr);
        if (rtn != SENSOR_SUCCESS) {
            SENSOR_PRINT_ERR("OTP lsc apply error!");
            return rtn;
        }
    } else {
        // Sensor_writereg8bits(0x0a05,0xef&Sensor_readreg8bits(0x0a05));
        // Sensor_writereg8bits(0x021c,0xfd&Sensor_readreg8bits(0x021c));
        rtn = SENSOR_SUCCESS;
    }

    return rtn;
}

static uint32_t hi846_qunhui_identify_otp(cmr_handle handle, void *param_ptr,
                                          void *p_data) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_PRINT("Mesin enter\n");
    rtn = hi846_qunhui_read_otp_info(handle, param_ptr, p_data);
    SENSOR_IC_CHECK_PTR(p_data);
    if (rtn != SENSOR_SUCCESS) {
        SENSOR_PRINT_ERR("read otp information failed\n!");
        return rtn;
    } else {
        SENSOR_PRINT("identify otp success mode_id = %d !",
                     otp_info->module_id);
    }

    return rtn;
}
