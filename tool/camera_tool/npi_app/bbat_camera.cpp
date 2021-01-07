#define LOG_TAG "bbat_camera"

#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#include "sprd_ion.h"
#include <linux/fb.h>
#include "sprd_cpp.h"
#include <dlfcn.h>
#include <utils/Mutex.h>
#include "MemIon.h"
#include "sprd_fts_type.h"

#include "../cpat_sdk/cpat_flash.h"
#include "../cpat_sdk/cpat_camera.h"

using namespace android;

#define SBUFFER_SIZE (600 * 1024)

#define BBAT_CMD_CLOSE_BACK_FLASH 0x00
#define BBAT_CMD_OPEN_BACK_WHITE_FLASH 0x01
#define BBAT_CMD_OPEN_BACK_COLOR_FLASH 0x02

#define BBAT_CMD_CLOSE_FRONT_FLASH 0x00
#define BBAT_CMD_OPEN_FRONT_FLASH 0x01

#define BBAT_CMD_MIPICAM_OPEN 0x01
#define BBAT_CMD_MIPICAM_READ_BUF 0x02
#define BBAT_CMD_MIPICAM_CLOSE 0x03
#define BBAT_CMD_MIPICAM_SETID 0x04

int bbat_back_flash(char *buf, int buf_len, char *rsp, int rsp_size) {
    int ret = 0;
    int cmd = buf[10];

    if (BBAT_CMD_OPEN_BACK_WHITE_FLASH == cmd) {
        CMR_LOGI("open back white flash");
        ret = cpat_open_back_white_flash();
    } else if (BBAT_CMD_OPEN_BACK_COLOR_FLASH == cmd) {
        CMR_LOGI("open back color flash");
        ret = cpat_open_back_color_flash();
    } else if (BBAT_CMD_CLOSE_BACK_FLASH == cmd) {
        CMR_LOGI("close back flash");
        ret = cpat_close_back_flash();
    } else {
        CMR_LOGE("undefined cmd!");
    }

    /*--------------------------------- generic code ----------------------------*/
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    p_msg_head->len = 8;

    CMR_LOGI("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    }
    CMR_LOGI("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
             rsp[sizeof(MSG_HEAD_T)]);
    rsp[p_msg_head->len + 2 - 1] = 0x7E;
    CMR_LOGI("dylib test :return len:%d", p_msg_head->len + 2);
    CMR_LOGI("engpc->pc flash:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1],
             rsp[2], rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

    return p_msg_head->len + 2;
    /*----------------------- generic code,Direct assignment --------------------*/
}

int bbat_front_flash(char *buf, int buf_len, char *rsp, int rsp_size) {
    int ret = 0;
    int cmd = buf[10];

    if (BBAT_CMD_OPEN_FRONT_FLASH == cmd) {
        CMR_LOGI("open front flash");
        ret = cpat_open_front_flash();
    } else if (BBAT_CMD_CLOSE_FRONT_FLASH == cmd) {
        CMR_LOGI("close front flash");
        ret = cpat_close_front_flash();
    } else {
        CMR_LOGE("undefined cmd!");
    }

    /*--------------------------------- generic code ----------------------------*/
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    p_msg_head->len = 8;

    CMR_LOGI("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    }
    CMR_LOGI("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
             rsp[sizeof(MSG_HEAD_T)]);
    rsp[p_msg_head->len + 2 - 1] = 0x7E;
    CMR_LOGI("dylib test :return len:%d", p_msg_head->len + 2);
    CMR_LOGI("engpc->pc flash:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1],
             rsp[2], rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

    return p_msg_head->len + 2;
    /*----------------------- generic code,Direct assignment --------------------*/
}

int bbat_mipicam(char *buf, int buf_len, char *rsp, int rsp_size) {

    int ret = 0;
    int fun_ret = 0;
    int rec_image_size = 0;
    cmr_u32 *p_buf = NULL;
    p_buf = new cmr_u32[SBUFFER_SIZE];
    static int cam_id = 0;
    int cmd = buf[9];

    switch (cmd) {
    case BBAT_CMD_MIPICAM_OPEN:
        fun_ret = cpat_camera_init(cam_id);
        if (fun_ret < 0) {
            ret = -1;
            break;
        }
/* camera ID2 is for occlusion, yuv sensor do not need to output picture*/
#if defined(ID2_FOR_OCCLUSION)
        if (2 == cam_id)
            break;
#endif

        fun_ret = cpat_camera_startpreview();
        if (fun_ret < 0) {
            ret = -1;
            break;
        }
        break;

    case BBAT_CMD_MIPICAM_READ_BUF:

/* camera ID2 is for occlusion, yuv sensor do not need to output picture*/
#if defined(ID2_FOR_OCCLUSION)
        if (2 == cam_id) {
            fun_ret = cpat_read_yuv_sensor_luma();
            CMR_LOGI("fun_ret: %d", fun_ret);
            if (fun_ret < 0)
                ret = -1;
            break;
        }
#endif

        CMR_LOGI("rsp_size: %d", rsp_size);
        fun_ret = cpat_read_cam_buf((void **)&p_buf, SBUFFER_SIZE,
                                        &rec_image_size);
        if (fun_ret < 0) {
            CMR_LOGI("rec_image_size: %d", rec_image_size);
            ret = -1;
            break;
        }
        CMR_LOGI("rec_image_size: %d", rec_image_size);
        if (rec_image_size > rsp_size - 1) {
            memcpy(rsp, p_buf, 768);
            ret = 768;
        } else {
            memcpy(rsp, p_buf, rec_image_size);
            ret = rec_image_size;
        }
        break;

    case BBAT_CMD_MIPICAM_CLOSE:
        fun_ret = cpat_camera_stoppreview();
        if (fun_ret < 0) {
            ret = -1;
            break;
        }
        fun_ret = cpat_camera_deinit();
        if (fun_ret < 0) {
            ret = -1;
            break;
        }
        break;

    case BBAT_CMD_MIPICAM_SETID:
        cam_id = buf[10];
        CMR_LOGI("set camera id: %d", cam_id);
        break;

    default:
        break;
    }

    /*--------------------------------- generic code ----------------------------*/
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);
    p_msg_head->len = 8;

    CMR_LOGI("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    } else if (ret > 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), p_buf, ret);
        p_msg_head->len += ret;
    }
    CMR_LOGI("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
             rsp[sizeof(MSG_HEAD_T)]);
    rsp[p_msg_head->len + 2 - 1] = 0x7E;
    CMR_LOGI("dylib test :return len:%d", p_msg_head->len + 2);
    CMR_LOGI("engpc->pc:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1], rsp[2],
             rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

    delete[] p_buf;
    return p_msg_head->len + 2;
    /*----------------------- generic code,Direct assignment --------------------*/
}
