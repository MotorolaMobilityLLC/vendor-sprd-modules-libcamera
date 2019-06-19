#include <utils/Log.h>
#include "sensor.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include <cutils/properties.h>

#define MODULE_NAME "qtech" // module vendor name
#define MODULE_ID_s5k4h7_qtech                                                 \
    0x0058 // s5k4h7: sensor P/N;  qtech: module vendor
#define I2C_SLAVE_ADDR 0x5A

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
};
static struct otp_info_t s_s5k4h7_qtech_otp_info;

#define RG_RATIO_TYPICAL_s5k4h7_qtech 0x29c // 0x108
#define BG_RATIO_TYPICAL_s5k4h7_qtech 0x26e // 0x142
#define GBGR_RATIO_TYPICAL_s5k4h7_qtech 0x3fd

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

cmr_u8 s5k4h7_Sensor_OTP_read(cmr_handle handle, uint16_t otp_addr) {
    SENSOR_IC_CHECK_HANDLE(handle);
    return Sensor_readreg8bits(handle, otp_addr); // OTP data read
}
static void s5k4h7_qtech_enable_awb_otp(void) {
    /*TODO enable awb otp update*/
    // Sensor_writereg8bits(0x021c, 0x04 | Sensor_readreg8bits(0x021c));
}

static uint32_t s5k4h7_qtech_update_awb(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    s5k4h7_qtech_enable_awb_otp();

    /*TODO*/
    uint32_t R_gain, B_gain, G_gain, G_gain_R, G_gain_B, r_ratio_4h8,
        b_ratio_4h8;
    // apply OTP WB Calibration
    if (otp_info->flag & 0x40) {

        R_gain =
            (otp_info->rg_ratio_typical * 0x0100) / otp_info->rg_ratio_current;
        B_gain =
            (otp_info->bg_ratio_typical * 0x0100) / otp_info->bg_ratio_current;
        G_gain = 0x0100;

        if (R_gain < B_gain) {
            if (R_gain < 0x0100) {
                B_gain = (0x0100 * B_gain) / R_gain;
                G_gain = (0x0100 * G_gain) / R_gain;
                R_gain = 0x0100;
            }
        } else {
            if (B_gain < 0x0100) {
                R_gain = (0x0100 * R_gain) / B_gain;
                G_gain = (0x0100 * G_gain) / B_gain;
                B_gain = 0x0100;
            }
        }

        SENSOR_PRINT("Set G_gain=%d,R_gain=%d,B_gain=%d\n", G_gain, R_gain,
                     B_gain);

        Sensor_writereg8bits(handle, 0x3C0F, 0x00);
        Sensor_writereg8bits(handle, 0x020E, G_gain >> 8);
        Sensor_writereg8bits(handle, 0x020F, G_gain & 0xff);
        Sensor_writereg8bits(handle, 0x0210, R_gain >> 8);
        Sensor_writereg8bits(handle, 0x0211, R_gain & 0xff);
        Sensor_writereg8bits(handle, 0x0212, B_gain >> 8);
        Sensor_writereg8bits(handle, 0x0213, B_gain & 0xff);
        Sensor_writereg8bits(handle, 0x0214, G_gain >> 8);
        Sensor_writereg8bits(handle, 0x0215, G_gain & 0xff);
    } else {
        SENSOR_PRINT("X");
    }

    return rtn;
}

static uint32_t s5k4h7_qtech_test_awb(void *param_ptr) {
    uint32_t flag = 1;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.cam.otp.awb", value, "on");

    if (!strcmp(value, "on")) {
        SENSOR_PRINT("apply awb otp normally!");
#if 0
        otp_info->rg_ratio_typical = RG_RATIO_TYPICAL_s5k4h7_qtech;
        otp_info->bg_ratio_typical = BG_RATIO_TYPICAL_s5k4h7_qtech;
        otp_info->gbgr_ratio_typical = GBGR_RATIO_TYPICAL_s5k4h7_qtech;
        otp_info->r_typical = otp_info->r_typical;
        otp_info->g_typical = otp_info->g_typical;
        otp_info->b_typical = otp_info->b_typical;
#endif

    } else if (!strcmp(value, "test")) {
        SENSOR_PRINT("apply awb otp on test mode!");
        otp_info->rg_ratio_typical = RG_RATIO_TYPICAL_s5k4h7_qtech * 1.5;
        otp_info->bg_ratio_typical = BG_RATIO_TYPICAL_s5k4h7_qtech * 1.5;
        otp_info->gbgr_ratio_typical = GBGR_RATIO_TYPICAL_s5k4h7_qtech * 1.5;
        otp_info->r_typical = otp_info->g_typical;
        otp_info->g_typical = otp_info->g_typical;
        otp_info->b_typical = otp_info->g_typical;

    } else {
        SENSOR_PRINT("without apply awb otp!");
        flag = 0;
    }
    return flag;
}

static uint32_t s5k4h7_qtech_update_lsc(cmr_handle handle, void *param_ptr) {
    uint32_t rtn = SENSOR_SUCCESS;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_PRINT("s5k4h7_qtech_update_lsc");

    Sensor_writereg8bits(handle, 0x3400, 0x00);
    Sensor_writereg8bits(handle, 0x0B00, 0x01);

    return rtn;
}

static uint32_t s5k4h7_qtech_read_otp_info(cmr_handle handle, void *param_ptr,
                                           void *p_data) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    struct sensor_ic_drv_cxt *sns_drv_cxt = (struct sensor_ic_drv_cxt *)handle;
    cmr_u8 otp_buffer[64];
    cmr_bzero(otp_buffer, sizeof(otp_buffer));

    uint16_t flag, start_address, wb_rg_golden, wb_bg_golden, wb_gg_golden,
        reg_offset;
    uint32_t checksum, i, checksum_reg;

    /*OTP Initial Setting,read buffer*/
    Sensor_writereg8bits(handle, 0x0100, 0x01); // Streaming On
    usleep(50 * 1000);
    Sensor_writereg8bits(handle, 0x0A02, 0x15); // page 21
    Sensor_writereg8bits(handle, 0x0A00, 0x01); // Read command
    usleep(50 * 1000);

    // read otp
    reg_offset = 0x0A04;
    for (i = 0; i < 64; i++) {
        otp_buffer[i] = s5k4h7_Sensor_OTP_read(handle, reg_offset + i);
        // SENSOR_PRINT("read reg value %d:0x%x\n", i,otp_buffer[i]);
    }
    Sensor_writereg8bits(handle, 0x0A00, 0x00);

    /////////////////////* module information*///////////////////////
    /*flag check*/
    flag = otp_buffer[0];
    otp_info->flag = 0x00;
    SENSOR_PRINT("module info flag=0x%x\n", flag);

    if (flag == 0x40) {
        start_address = 0x0A05 - reg_offset; // group1
        checksum_reg = 0x0A17 - reg_offset;
    } else if (flag == 0x10) {
        start_address = 0x0A25 - reg_offset; // group2
        checksum_reg = 0x0A37 - reg_offset;
    } else {
        start_address = 0x00;
        checksum_reg = 0x00;
        SENSOR_PRINT("no module information in otp\n");
    }

    /*checksum*/
    checksum = 0;
    for (i = 0; i < 18; i++) {
        checksum = checksum + otp_buffer[start_address + i];
    }
    checksum = (checksum % 255) + 1;

    if (checksum == otp_buffer[checksum_reg]) {
        SENSOR_PRINT("module information checksum success\n");
        otp_info->flag |= 0xC0;
    } else {
        SENSOR_PRINT("module information checksum error\n");
        otp_info->flag &= 0x7f;
    }
    /*read module information*/
    otp_info->module_id = otp_buffer[start_address];
    otp_info->lens_id = otp_buffer[start_address + 1];
    otp_info->year = otp_buffer[start_address + 3];
    otp_info->month = otp_buffer[start_address + 4];
    otp_info->day = otp_buffer[start_address + 5];
    /*read awb information*/
    otp_info->rg_ratio_current =
        (otp_buffer[start_address + 6] << 8) + otp_buffer[start_address + 7];
    otp_info->bg_ratio_current =
        (otp_buffer[start_address + 8] << 8) + otp_buffer[start_address + 9];
    otp_info->gbgr_ratio_current =
        (otp_buffer[start_address + 10] << 8) + otp_buffer[start_address + 11];
    otp_info->rg_ratio_typical =
        (otp_buffer[start_address + 12] << 8) + otp_buffer[start_address + 13];
    otp_info->bg_ratio_typical =
        (otp_buffer[start_address + 14] << 8) + otp_buffer[start_address + 15];
    otp_info->gbgr_ratio_typical =
        (otp_buffer[start_address + 16] << 8) + otp_buffer[start_address + 17];

    if (otp_info->rg_ratio_typical == 0 || otp_info->bg_ratio_typical == 0 ||
        otp_info->gbgr_ratio_typical == 0) {
        otp_info->rg_ratio_typical = RG_RATIO_TYPICAL_s5k4h7_qtech;
        otp_info->bg_ratio_typical = BG_RATIO_TYPICAL_s5k4h7_qtech;
        otp_info->gbgr_ratio_typical = GBGR_RATIO_TYPICAL_s5k4h7_qtech;
    }

    SENSOR_PRINT("wb_rg_golden=0x%x", otp_info->rg_ratio_typical);
    SENSOR_PRINT("wb_bg_golden=0x%x", otp_info->bg_ratio_typical);
    SENSOR_PRINT("wb_gg_golden=0x%x", otp_info->gbgr_ratio_typical);

    /*print otp information*/
    SENSOR_PRINT("flag=0x%x", otp_info->flag);
    SENSOR_PRINT("module_id=0x%x", otp_info->module_id);

    SENSOR_PRINT("data=%d-%d-%d", otp_info->year, otp_info->month,
                 otp_info->day);
    SENSOR_PRINT("rg_ratio_current=0x%x", otp_info->rg_ratio_current);
    SENSOR_PRINT("bg_ratio_current=0x%x", otp_info->bg_ratio_current);
    SENSOR_PRINT("gbgr_ratio_current=0x%x", otp_info->gbgr_ratio_current);
    SENSOR_PRINT("rg_ratio_typical=0x%x", otp_info->rg_ratio_typical);
    SENSOR_PRINT("bg_ratio_typical=0x%x", otp_info->bg_ratio_typical);
    SENSOR_PRINT("gbgr_ratio_typical=0x%x", otp_info->gbgr_ratio_typical);

    /*update awb*/
    if (s5k4h7_qtech_test_awb(otp_info)) {
        rtn = s5k4h7_qtech_update_awb(handle, param_ptr);
        if (rtn != SENSOR_SUCCESS) {
            SENSOR_PRINT_ERR("OTP awb apply error!");
            return rtn;
        }
        rtn = s5k4h7_qtech_update_lsc(handle, param_ptr);
        if (rtn != SENSOR_SUCCESS) {
            SENSOR_PRINT_ERR("OTP awb apply error!");
            return rtn;
        }
    } else {
        rtn = SENSOR_SUCCESS;
    }

    Sensor_writereg8bits(handle, 0x0100, 0x00); // Streaming Off

    return rtn;
}

static uint32_t s5k4h7_qtech_identify_otp(cmr_handle handle, void *param_ptr,
                                          void *p_data) {
    uint32_t rtn = SENSOR_SUCCESS;
    struct otp_info_t *otp_info = (struct otp_info_t *)param_ptr;
    SENSOR_IC_CHECK_HANDLE(handle);
    SENSOR_PRINT("Mesin enter\n");
    rtn = s5k4h7_qtech_read_otp_info(handle, param_ptr, p_data);
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
