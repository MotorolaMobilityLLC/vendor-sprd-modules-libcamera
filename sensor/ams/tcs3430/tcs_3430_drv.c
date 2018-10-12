#include "tcs_3430_drv.h"

static int read_tcs3430(const char *filename) {
    int data = -1;
    FILE *fd = fopen(filename, "r");
    if (fd != NULL) {
        fscanf(fd, "%d", &data);
        fclose(fd);
    }
    return data;
}

static void calc_lux_cct(struct tcs_data *tcs3430_data) {
    double kr = 1.0;
    double ky = 1.0;
    double kz = 1.0;
    double kir1 = 1.0;

    double kH11 = +1.40836; double kH12 = +0.57942; double kH13 = -0.16628; double kH14 = -0.04627;
    double kH21 = +0.42070; double kH22 = +1.56692; double kH23 = +0.17842; double kH24 = -0.06553;
    double kH31 = -2.19037; double kH32 = +2.80822; double kH33 = +3.93525; double kH34 = -0.12764;

    double kL11 = +1.46390; double kL12 = +0.55112; double kL13 = -0.12560; double kL14 = -0.18618;
    double kL21 = -0.19919; double kL22 = +2.30077; double kL23 = +0.43753; double kL24 = -0.73700;
    double kL31 = -0.07548; double kL32 = -0.05515; double kL33 = +5.91481; double kL34 = -0.21559;

    double IR_boundary = 1.0;

    double x_cal = tcs3430_data->x_raw * kr;
    double y_cal = tcs3430_data->y_raw * ky;
    double z_cal = tcs3430_data->z_raw * kz;
    double ir1_cal = tcs3430_data->ir_raw * kir1;

    double atime = tcs3430_data->atime_data;
    double again = tcs3430_data->gain_data;
    double tmp = atime * again;

    if (0 == tmp) {
        SENSOR_LOGE("ams tcs3430 tmp value error\n");
        goto error;
    }
    double x_norm = 100.008 * 16 * x_cal / tmp;
    double y_norm = 100.008 * 16 * y_cal / tmp;
    double z_norm = 100.008 * 16 * z_cal / tmp;
    double ir1_norm = 100.008 * 16 * ir1_cal / tmp;

    double x_hi_ir = MH[0][0] * x_norm + MH[0][1] * y_norm + MH[0][2] * z_norm +
                     MH[0][3] * ir1_norm + MH[0][4];
    double y_hi_ir = MH[1][0] * x_norm + MH[1][1] * y_norm + MH[1][2] * z_norm +
                     MH[1][3] * ir1_norm + MH[1][4];
    double z_hi_ir = MH[2][0] * x_norm + MH[2][1] * y_norm + MH[2][2] * z_norm +
                     MH[2][3] * ir1_norm + MH[2][4];

    double x_lo_ir = ML[0][0] * x_norm + ML[0][1] * y_norm + ML[0][2] * z_norm +
                     ML[0][3] * ir1_norm + ML[0][4];
    double y_lo_ir = ML[1][0] * x_norm + ML[1][1] * y_norm + ML[1][2] * z_norm +
                     ML[1][3] * ir1_norm + ML[1][4];
    double z_lo_ir = ML[2][0] * x_norm + ML[2][1] * y_norm + ML[2][2] * z_norm +
                     ML[2][3] * ir1_norm + ML[2][4];

    if (0 == y_norm) {
        SENSOR_LOGE("ams tcs3430 y_norm value error\n");
        goto error;
    }
    double ir_ratio = ir1_norm / y_norm;

    if (ir_ratio > IR_boundary) {
        tcs3430_data->x_data = x_hi_ir;
        tcs3430_data->y_data = y_hi_ir;
        tcs3430_data->z_data = z_hi_ir;
    } else {
        tcs3430_data->x_data = x_lo_ir;
        tcs3430_data->y_data = y_lo_ir;
        tcs3430_data->z_data = z_lo_ir;
    }

    tcs3430_data->ir_data = tcs3430_data->ir_raw;
    tcs3430_data->lux_data = 0;
    tcs3430_data->cct_data = 0;
    return;

error:
    memset(tcs3430_data, 0, sizeof(struct tcs_data));
    return;

}

int tcs3430_read_data(void *param) {
    struct tcs_data *tcs3430_data = (struct tcs_data *)param;
    tcs3430_data->x_raw =
        read_tcs3430("/sys/devices/virtual/input/input5/tcs3430_als_x");
    tcs3430_data->y_raw =
        read_tcs3430("/sys/devices/virtual/input/input5/tcs3430_als_y");
    tcs3430_data->z_raw =
        read_tcs3430("/sys/devices/virtual/input/input5/tcs3430_als_z");
    tcs3430_data->ir_raw =
        read_tcs3430("/sys/devices/virtual/input/input5/tcs3430_als_ir1");
    tcs3430_data->gain_data =
        read_tcs3430("/sys/devices/virtual/input/input5/tcs3430_als_gain");
    tcs3430_data->atime_data =
        read_tcs3430("/sys/devices/virtual/input/input5/tcs3430_als_atime");

    if (-1 == tcs3430_data->x_raw || -1 == tcs3430_data->y_raw ||
        -1 == tcs3430_data->z_raw || -1 == tcs3430_data->ir_raw ||
        -1 == tcs3430_data->gain_data || -1 == tcs3430_data->atime_data) {
        memset(tcs3430_data, 0, sizeof(struct tcs_data));
    } else {
        calc_lux_cct(tcs3430_data);
    }
    SENSOR_LOGD("ams tcs3430 x_data:%d, y_data:%d, z_data:%d, ir_data:%d, "
                "x_raw:%d, y_raw:%d, z_raw:%d, ir_raw:%d\n",
                tcs3430_data->x_data, tcs3430_data->y_data,
                tcs3430_data->z_data, tcs3430_data->ir_data,
                tcs3430_data->x_raw, tcs3430_data->y_raw,
                tcs3430_data->z_raw, tcs3430_data->ir_raw);
    return 0;
}
