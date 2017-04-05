#include <camera/Camera.h>
#include <utils/Log.h>
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
#include "hal3_2v1/SprdCamera3OEMIf.h"
#include "hal3_2v1/SprdCamera3Setting.h"
#else
#include "hal3_3v0/SprdCamera3OEMIf.h"
#include "hal3_3v0/SprdCamera3Setting.h"
#endif
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
//#include "graphics.h"
#include <linux/fb.h>
//#include "sprd_rot_k.h"
#include "sprd_cpp.h"
using namespace sprdcamera;

#include <dlfcn.h>

typedef enum enumYUVFormat {
    FMT_NV21 = 0,
    FMT_NV12,
} YUVFormat;

typedef struct {
    int width;
    int height;
    int row_bytes;
    int pixel_bytes;
    unsigned char *data;
} GRSurface;

typedef GRSurface *gr_surface;

typedef struct minui_backend {
    // Initializes the backend and returns a gr_surface to draw into.
    gr_surface (*init)(struct minui_backend *);

    // Causes the current drawing surface (returned by the most recent
    // call to flip() or init()) to be displayed, and returns a new
    // drawing surface.
    gr_surface (*flip)(struct minui_backend *);

    // Blank (or unblank) the screen.
    void (*blank)(struct minui_backend *, bool);

    // Device cleanup when drawing is done.
    void (*exit)(struct minui_backend *);
} minui_backend;

static minui_backend *gr_backend = NULL;
static GRSurface *gr_draw = NULL;

/*yuv->rgb*/
#define RGB565(r, g, b)                                                        \
    ((unsigned short)((((unsigned char)(r) >> 3) |                             \
                       ((unsigned short)(((unsigned char)(g) >> 2)) << 5)) |   \
                      (((unsigned short)((unsigned char)(b >> 3))) << 11)))

/*rotation device nod*/
static int rot_fd = -1;
#define ROT_DEV "/dev/sprd_rotation"

/*process control*/
Mutex previewLock;                  /*preview lock*/
int previewvalid = 0;               /*preview flag*/
static int s_mem_method = 0;        /*0: physical address, 1: iommu  address*/
static unsigned char camera_id = 0; /*camera id: fore=1,back=0*/

/*data processing useful*/
#define PREVIEW_WIDTH 960  // 1280//640//
#define PREVIEW_HIGHT 720  // 960//480//
#define PREVIEW_BUFF_NUM 4 /*preview buffer*/
#define SPRD_MAX_PREVIEW_BUF PREVIEW_BUFF_NUM
struct frame_buffer_t {
    cmr_uint phys_addr;
    cmr_uint virt_addr;
    cmr_uint length; // buffer's length is different from cap_image_size
};
static char af_tuning_path[] = "/data/misc/cameraserver/af_tuning_default.bin";
static char sensor_para_path[] = "/data/misc/cameraserver/sensor.file";
static struct frame_buffer_t fb_buf[SPRD_MAX_PREVIEW_BUF + 1];
static uint8_t *tmpbuf2, *tmpbuf3; //*tmpbuf1, *tmpbuf,
static uint32_t post_preview_buf[PREVIEW_WIDTH * PREVIEW_HIGHT];
static struct fb_var_screeninfo var;
static uint32_t frame_num = 0;           /*record frame number*/
static unsigned int mPreviewHeapNum = 0; /*allocated preview buffer number*/
#if defined(CONFIG_CAMERA_ISP_DIR_3)
static sprd_camera_memory_t *mPreviewHeapReserved[PREV_RESERVED_FRM_CNT] = {0};
static sprd_camera_memory_t *mVideoHeapReserved[VIDEO_RESERVED_FRM_CNT] = {0};
static sprd_camera_memory_t *mZslHeapReserved[CAP_ZSL_RESERVED_FRM_CNT] = {0};
static sprd_camera_memory_t *mIspAntiFlickerHeapReserved = NULL;
static sprd_camera_memory_t *mPdafRawHeapReserved = NULL;
static sprd_camera_memory_t *mHighIsoSnapshotHeapReserved = NULL;
static sprd_camera_memory_t *mIspPreviewYReserved[2];
static sprd_camera_memory_t *mIspYUVReserved;
static sprd_camera_memory_t *mIspRawDataReserved[ISP_RAWBUF_NUM];
static sprd_camera_memory_t *mDepthHeapReserved = NULL;
#else
static sprd_camera_memory_t *mPreviewHeapReserved = NULL;
#endif
static sprd_camera_memory_t *mIspLscHeapReserved = NULL;
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
static sprd_camera_memory_t *mIspStatisHeapReserved = NULL;
#endif
static sprd_camera_memory_t *mIspAFLHeapReserved = NULL;
static sprd_camera_memory_t *mIspFirmwareReserved = NULL;
static uint32_t mIspFirmwareReserved_cnt = 0;
static const int kISPB4awbCount = 16;
sprd_camera_memory_t *mIspB4awbHeapReserved[kISPB4awbCount];
sprd_camera_memory_t *mIspRawAemHeapReserved[kISPB4awbCount];

static sprd_camera_memory_t
    *previewHeapArray[PREVIEW_BUFF_NUM]; /*preview heap arrary*/

static oem_module_t *mHalOem;

struct client_t {
    int reserved;
};
static struct client_t client_data;
static cmr_handle oem_handle = 0;

uint32_t lcd_w = 0, lcd_h = 0;

int IommuIsEnabled(void) {
    int ret;
    int iommuIsEnabled = 0;

    if (NULL == oem_handle || NULL == mHalOem || NULL == mHalOem->ops) {
        HAL_LOGE("oem is null or oem ops is null");
        return 0;
    }

    ret = mHalOem->ops->camera_get_iommu_status(oem_handle);
    if (ret) {
        iommuIsEnabled = 0;
        return iommuIsEnabled;
    }

    iommuIsEnabled = 1;
    return iommuIsEnabled;
}

bool getLcdSize(uint32_t *width, uint32_t *height) {
    *width = (uint32_t)gr_draw->width;
    *height = (uint32_t)gr_draw->height;
    return true;
}

static void RGBRotate90_anticlockwise(void *pDest, int nDestWidth,
                                      int nDestHeight, int nDestBits,
                                      void *pSrc, int nSrcWidth, int nSrcHeight,
                                      int nSrcBits) {

    int i, j;
    int m = nSrcBits >> 3;

    unsigned int *des32 = (unsigned int *)pDest;
    unsigned int *src32 = (unsigned int *)pSrc;
    unsigned short *des16 = (unsigned short *)pDest;
    unsigned short *src16 = (unsigned short *)pSrc;

    if ((nDestWidth < nSrcHeight) || (nDestHeight < nSrcWidth)) {
        int s_offsetX = 0;
        int s_offsetY = 0;
        int d_offsetX = s_offsetY;
        int d_offsetY = s_offsetX;

        if ((!pDest) || (!pSrc))
            return;
        if (m == 4) {
            for (j = 0; j < nDestHeight; j++) {
                for (i = 0; i < nDestWidth; i++) {
                    *(des32 + (j + d_offsetY) * nDestWidth + nDestWidth -
                      (j + d_offsetX)) =
                        *(src32 + (j + s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        } else {
            for (j = 0; j < nDestHeight; j++) {
                for (i = 0; i < nDestWidth; i++) {
                    *(des16 + (j + d_offsetY) * nDestWidth + nDestWidth -
                      (j + d_offsetX)) =
                        *(src16 + (j + s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        }
    } else {
        int d_offsetX = ((int)((nDestWidth - nSrcHeight) / 2));
        int d_offsetY = ((int)((nDestHeight - nSrcWidth) / 2));
        int s_offsetX = d_offsetY;
        int s_offsetY = d_offsetX;
        if ((!pDest) || (!pSrc))
            return;
        if (m == 4) {
            for (j = 0; j < nSrcHeight; j++) {
                for (i = 0; i < nSrcWidth; i++) {
                    *(des32 + (i + d_offsetY) * nDestWidth + nDestWidth -
                      (j + d_offsetX)) =
                        *(src32 + (j + s_offsetY) * nDestHeight + i +
                          s_offsetX);
                }
            }
        } else {
            for (j = 0; j < nSrcHeight; j++) {
                for (i = 0; i < nSrcWidth; i++) {
                    *(des16 + (i + d_offsetY) * nDestWidth + nDestWidth -
                      (j + d_offsetX)) =
                        *(src16 + (j + s_offsetY) * nDestHeight + i +
                          s_offsetX);
                }
            }
        }
    }
}

static void RGBRotate90_clockwise(void *pDest, int nDestWidth, int nDestHeight,
                                  int nDestBits, void *pSrc, int nSrcWidth,
                                  int nSrcHeight, int nSrcBits) {

    int i, j;
    int m = nSrcBits >> 3;

    unsigned int *des32 = (unsigned int *)pDest;
    unsigned int *src32 = (unsigned int *)pSrc;
    unsigned short *des16 = (unsigned short *)pDest;
    unsigned short *src16 = (unsigned short *)pSrc;

    if ((nDestWidth < nSrcHeight) || (nDestHeight < nSrcWidth)) {
        int s_offsetX = 0;
        int s_offsetY = 0;
        int d_offsetX = s_offsetY;
        int d_offsetY = s_offsetX;

        if ((!pDest) || (!pSrc))
            return;
        if (m == 4) {
            for (j = 0; j < nDestHeight; j++) {
                for (i = 0; i < nDestWidth; i++) {
                    *(des32 + (nDestHeight - i - d_offsetY) * nDestWidth +
                      nDestWidth - (j + d_offsetX)) =
                        *(src32 + (j + s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        } else {
            for (j = 0; j < nDestHeight; j++) {
                for (i = 0; i < nDestWidth; i++) {
                    *(des16 + (nDestHeight - i - d_offsetY) * nDestWidth +
                      nDestWidth - (j + d_offsetX)) =
                        *(src16 + (j + s_offsetY) * nDestWidth + i + s_offsetX);
                }
            }
        }
    } else {
        int d_offsetX = ((int)((nDestWidth - nSrcHeight) / 2));
        int d_offsetY = ((int)((nDestHeight - nSrcWidth) / 2));
        int s_offsetX = d_offsetY;
        int s_offsetY = d_offsetX;
        if ((!pDest) || (!pSrc))
            return;
        if (m == 4) {
            for (j = 0; j < nSrcHeight; j++) {
                for (i = 0; i < nSrcWidth; i++) {
                    *(des32 + (nDestHeight - i - d_offsetY) * nDestWidth +
                      nDestWidth - (j + d_offsetX)) =
                        *(src32 + (j + s_offsetY) * nDestHeight + i +
                          s_offsetX);
                }
            }
        } else {
            for (j = 0; j < nSrcHeight; j++) {
                for (i = 0; i < nSrcWidth; i++) {
                    *(des16 + (nDestHeight - i - d_offsetY) * nDestWidth +
                      nDestWidth - (j + d_offsetX)) =
                        *(src16 + (j + s_offsetY) * nDestHeight + i +
                          s_offsetX);
                }
            }
        }
    }
}

static void YUVRotate90(uint8_t *des, uint8_t *src, int width, int height) {
    int i = 0, j = 0, n = 0;
    int hw = width / 2, hh = height / 2;

    for (j = width; j > 0; j--) {
        for (i = 0; i < height; i++) {
            des[n++] = src[width * i + j];
        }
    }

    unsigned char *ptmp = src + width * height;
    for (j = hw; j > 0; j--) {
        for (i = 0; i < hh; i++) {
            des[n++] = ptmp[hw * i + j];
        }
    }

    ptmp = src + width * height * 5 / 4;
    for (j = hw; j > 0; j--) {
        for (i = 0; i < hh; i++) {
            des[n++] = ptmp[hw * i + j];
        }
    }
}

static void StretchColors(void *pDest, int nDestWidth, int nDestHeight,
                          int nDestBits, void *pSrc, int nSrcWidth,
                          int nSrcHeight, int nSrcBits) {
    if ((nDestWidth < nSrcWidth) || (nDestHeight < nSrcHeight)) {
        double dfAmplificationX = ((double)nDestWidth) / nSrcWidth;
        double dfAmplificationY = ((double)nDestHeight) / nSrcHeight;
        double stretch =
            1 / (dfAmplificationY > dfAmplificationX ? dfAmplificationY
                                                     : dfAmplificationX);
        int offsetX =
            (dfAmplificationY > dfAmplificationX
                 ? (int)(nSrcWidth - nDestWidth / dfAmplificationY) >> 1
                 : 0);
        int offsetY =
            (dfAmplificationX > dfAmplificationY
                 ? (int)(nSrcHeight - nDestHeight / dfAmplificationX) >> 1
                 : 0);
        int i = 0;
        int j = 0;
        double tmpY = 0;
        unsigned int *pSrcPos = (unsigned int *)pSrc;
        unsigned int *pDestPos = (unsigned int *)pDest;
        int linesize;
        ALOGI("Native MMI Test: nDestWidth = %d, nDestHeight = %d, nSrcWidth = "
              "%d, nSrcHeight = %d, offsetX = %d, offsetY= %d \n",
              nDestWidth, nDestHeight, nSrcWidth, nSrcHeight, offsetX, offsetY);

        for (i = 0; i < nDestHeight; i++) {
            double tmpX = 0;

            int nLine = (int)(tmpY + 0.5) + offsetY;

            linesize = nLine * nSrcWidth;

            for (j = 0; j < nDestWidth; j++) {

                int nRow = (int)(tmpX + 0.5) + offsetX;

                *pDestPos++ = *(pSrcPos + linesize + nRow);

                tmpX += stretch;
            }
            tmpY += stretch;
        }
    } else {
        int offsetX = (int)((nDestHeight - nSrcHeight) / 2);
        int offsetY = (int)((nDestWidth - nSrcWidth) / 2);
        int i = 0, j = 0;
        unsigned int *pSrcPos = (unsigned int *)pSrc;
        unsigned int *pDestPos = (unsigned int *)pDest;
        for (j = 0; j < nSrcHeight; j++) {
            for (i = 0; i < nSrcWidth; i++) {
                *(pDestPos + (j + offsetX) * nDestWidth + i + offsetY) =
                    *(pSrcPos + j * nSrcWidth + i);
            }
        }
    }
}

static void yuv420_to_rgb(int width, int height, unsigned char *src,
                          unsigned int *dst, unsigned int format) {
    int frameSize = width * height;
    int j = 0, yp = 0, i = 0;
    unsigned short *dst16 = (unsigned short *)dst;
    unsigned char *yuv420sp = src;

    for (j = 0, yp = 0; j < height; j++) {
        int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;

        for (i = 0; i < width; i++, yp++) {
            int y = (0xff & ((int)yuv420sp[yp])) - 16;

            if (y < 0)
                y = 0;
            if (format == FMT_NV21) {
                if ((i & 1) == 0) {
                    u = (0xff & yuv420sp[uvp++]) - 128;
                    v = (0xff & yuv420sp[uvp++]) - 128;
                }
            } else {
                if ((i & 1) == 0) {
                    v = (0xff & yuv420sp[uvp++]) - 128;
                    u = (0xff & yuv420sp[uvp++]) - 128;
                }
            }

            int y1192 = 1192 * y;
            int r = (y1192 + 1634 * v);
            int g = (y1192 - 833 * v - 400 * u);
            int b = (y1192 + 2066 * u);

            if (r < 0)
                r = 0;
            else if (r > 262143)
                r = 262143;
            if (g < 0)
                g = 0;
            else if (g > 262143)
                g = 262143;
            if (b < 0)
                b = 0;
            else if (b > 262143)
                b = 262143;

            if (var.bits_per_pixel == 32) {
                dst[yp] = ((((r << 6) & 0xff0000) >> 16) << 16) |
                          (((((g >> 2) & 0xff00) >> 8)) << 8) |
                          ((((b >> 10) & 0xff)) << 0);
            } else {
                dst16[yp] =
                    RGB565((((r << 6) & 0xff0000) >> 16),
                           (((g >> 2) & 0xff00) >> 8), (((b >> 10) & 0xff)));
            }
        }
    }
}

static void eng_dcamtest_switchTB(uint8_t *buffer, uint16_t width,
                                  uint16_t height, uint8_t pixel) {
    uint32_t j;
    uint32_t linesize;
    uint8_t *dst = NULL;
    uint8_t *src = NULL;
    uint8_t *tmpBuf = NULL;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
    linesize = width * (pixel / 8);

    tmpBuf = (uint8_t *)malloc(linesize);
    if (!tmpBuf) {
        ALOGE("Native MMI Test: %s,%d Fail to alloc temp buffer\n", __func__,
              __LINE__);
        return;
    }

    /*  */
    for (j = 0; j < height / 2; j++) {
        src = buffer + j * linesize;
        dst = buffer + height * linesize - (j + 1) * linesize;
        memcpy(tmpBuf, src, linesize);

        for (j = 0; j < height / 2; j++) {
            src = buffer + j * linesize;
            dst = buffer + height * linesize - (j + 1) * linesize;
            memcpy(tmpBuf, src, linesize);
            memcpy(src, dst, linesize);
            memcpy(dst, tmpBuf, linesize);
        }

        free(tmpBuf);
    }
}

static int eng_test_rotation(uint32_t agree, uint32_t width, uint32_t height,
                             uint32_t in_addr, uint32_t out_addr) {
    struct sprd_cpp_rot_cfg_parm rot_params;

    /* set rotation params */
    rot_params.format = ROT_YUV420;
    switch (agree) {
    case 90:
        rot_params.angle = ROT_90;
        break;
    case 180:
        rot_params.angle = ROT_180;
        break;
    case 270:
        rot_params.angle = ROT_270;
        break;
    default:
        rot_params.angle = ROT_ANGLE_MAX;
        break;
    }
    rot_params.size.w = width;
    rot_params.size.h = height;
    rot_params.src_addr.y = in_addr;
    rot_params.src_addr.u =
        rot_params.src_addr.y + rot_params.size.w * rot_params.size.h;
    rot_params.src_addr.v =
        rot_params.src_addr.u + rot_params.size.w * rot_params.size.h / 4;
    rot_params.dst_addr.y = out_addr;
    rot_params.dst_addr.u =
        rot_params.dst_addr.y + rot_params.size.w * rot_params.size.h;
    rot_params.dst_addr.v =
        rot_params.dst_addr.u + rot_params.size.w * rot_params.size.h / 4;

    /* open rotation device  */
    rot_fd = open(ROT_DEV, O_RDWR, 0);
    if (-1 == rot_fd) {
        ALOGE("Native MMI Test: %s,%d Fail to open rotation device\n", __func__,
              __LINE__);
        return -1;
    }

    /* call ioctl */
    if (-1 == ioctl(rot_fd, SPRD_CPP_IO_START_ROT, &rot_params)) {
        ALOGE("Native MMI Test: %s,%d Fail to SC8800G_ROTATION_DONE\n",
              __func__, __LINE__);
        return -1;
    }

    return 0;
}

static int eng_test_fb_open(void) {
    int i;
    void *bits;
    int offset_page_align;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    var.yres = lcd_h;
    var.xres = lcd_w;
    var.bits_per_pixel = (8 * gr_draw->pixel_bytes);
    ALOGI("Native MMI Test: var.yres = %d, var.xres = %d, var.bits_per_pixel = "
          "%d\n",
          var.yres, var.xres, var.bits_per_pixel);
    /* set framebuffer address */
    memset(&fb_buf, 0, sizeof(fb_buf));

    fb_buf[0].virt_addr = (size_t)gr_draw->data;

    fb_buf[1].virt_addr =
        (size_t)var.yres * var.xres * (var.bits_per_pixel / 8);

    fb_buf[2].virt_addr = (size_t)tmpbuf2;

    fb_buf[3].virt_addr = (size_t)tmpbuf3;

    /*
            for(i = 0; i < 6; i++){
                    ALOGD("DCAM: buf[%d] virt_addr=0x%x, phys_addr=0x%x,
       length=%d", \
                            i,
       fb_buf[i].virt_addr,fb_buf[i].phys_addr,fb_buf[i].length);
            }
    */

    return 0;
}

static unsigned int getPreviewBufferIDForFd(cmr_s32 fd) {
    unsigned int i = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!previewHeapArray[i])
            continue;

        if (!(cmr_uint)previewHeapArray[i]->fd)
            continue;

        if (previewHeapArray[i]->fd == fd)
            return i;
    }

    return 0xFFFFFFFF;
}

static void eng_test_fb_update(const camera_frame_type *frame) {
    // int width, height;
    int crtc = 0;
    unsigned int buffer_id;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (!frame)
        return;

    buffer_id = getPreviewBufferIDForFd(frame->fd);

    //	memcpy(gr_draw->data, (uint8_t *)(fb_buf[!(frame_num % 2)].virt_addr),
    // lcd_w*lcd_h*4);

    gr_draw = gr_backend->flip(gr_backend);
    fb_buf[0].virt_addr = (size_t)gr_draw->data;

    /*  */
    if (!previewHeapArray[buffer_id]) {
        ALOGI("Native MMI Test: %s,%d preview heap array empty, do nothing\n",
              __func__, __LINE__);
        return;
    }

    /*  */
    if (NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGI("Native MMI Test: oem is null or oem ops is null, do nothing\n");
        return;
    }

    mHalOem->ops->camera_set_preview_buffer(
        oem_handle, (cmr_uint)previewHeapArray[buffer_id]->phys_addr,
        (cmr_uint)previewHeapArray[buffer_id]->data,
        (cmr_s32)previewHeapArray[buffer_id]->fd);
}

void eng_test_camera_close(void) {
    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);
    return;
}

int eng_test_flashlight_ctrl(uint32_t flash_status) {
    int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    return ret;
}

static void data_mirror(uint8_t *des, uint8_t *src, int width, int height,
                        int bits) {
    int size = 0;
    int i, j;
    int lineunm;
    int m = bits >> 3;
    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    /*  */
    if ((!des) || (!src))
        return;

    /*  */

    if (m == 4) {
        for (j = 0; j < height; j++) {
            size += width;
            for (i = 0; i < width; i++) {
                *(des32 + size - i - 1) = *src32++;
            }
        }
    } else {
        for (j = 0; j < height; j++) {
            size += width;
            for (i = 0; i < width; i++) {
                *(des16 + size - i - 1) = *src16++;
            }
        }
    }
    return;
}

void eng_tst_camera_cb(enum camera_cb_type cb, const void *client_data,
                       enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;
    int num;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (!frame) {
        ALOGI("Native MMI Test: %s,%d, camera call back parm4 error: NULL, do "
              "nothing\n",
              __func__, __LINE__);
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        ALOGI(
            "Native MMI Test: %s,%d, camera func type error: %d, do nothing\n",
            __func__, __LINE__, func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        ALOGI("Native MMI Test: %s,%d, camera cb type error: %d, do nothing\n",
              __func__, __LINE__, cb);
        return;
    }

    /*lock*/
    previewLock.lock();

    /*empty preview arry, do nothing*/
    if (!previewHeapArray[frame->buf_id]) {
        ALOGI("Native MMI Test: %s,%d, preview heap array empty, do nothine\n",
              __func__, __LINE__);
        previewLock.unlock();
        return;
    }

    /*get frame buffer id*/
    frame->buf_id = getPreviewBufferIDForFd(frame->fd);

    /*preview enable or disable?*/
    if (!previewvalid) {
        ALOGI("Native MMI Test: %s,%d, preview disabled, do nothing\n",
              __func__, __LINE__);
        previewLock.unlock();
        return;
    }

#ifdef CONFIG_CAMERA_DCAM_SUPPORT_FORMAT_NV12
    unsigned int format = FMT_NV12;
#else
    unsigned int format = FMT_NV21;
#endif
    // 1.yuv -> rgb
    yuv420_to_rgb(PREVIEW_WIDTH, PREVIEW_HIGHT,
                  (unsigned char *)previewHeapArray[frame->buf_id]->data,
                  post_preview_buf, format);

    /*unlock*/
    previewLock.unlock();

    /* fore && back camera: istrech,mirror,rotate*/
    if (0 == camera_id) {
        // 2. stretch
        StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                      var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                      PREVIEW_HIGHT, var.bits_per_pixel);
        /*FIXME: here need 2 or 3 framebuffer pingpang ??*/
        if (!(frame_num % 2)) {

            // 3. rotation
            RGBRotate90_anticlockwise(
                (void *)(fb_buf[0].virt_addr), var.xres, var.yres,
                var.bits_per_pixel, (void *)fb_buf[2].virt_addr, PREVIEW_WIDTH,
                PREVIEW_HIGHT, var.bits_per_pixel);
        } else {

            // 3. rotation
            RGBRotate90_anticlockwise(
                (void *)(fb_buf[0].virt_addr), var.xres, var.yres,
                var.bits_per_pixel, (void *)fb_buf[2].virt_addr, PREVIEW_WIDTH,
                PREVIEW_HIGHT, var.bits_per_pixel);
            // RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr),
            // (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres,
            // var.bits_per_pixel);
        }
    } else if (1 == camera_id) {
        // 2. stretch
        StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                      var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                      PREVIEW_HIGHT, var.bits_per_pixel);
        if (!(frame_num % 2)) {
            RGBRotate90_clockwise((void *)(fb_buf[0].virt_addr), var.xres,
                                  var.yres, var.bits_per_pixel,
                                  (void *)fb_buf[2].virt_addr, PREVIEW_WIDTH,
                                  PREVIEW_HIGHT, var.bits_per_pixel);
        } else {
            RGBRotate90_clockwise((void *)(fb_buf[0].virt_addr), var.xres,
                                  var.yres, var.bits_per_pixel,
                                  (void *)fb_buf[2].virt_addr, PREVIEW_WIDTH,
                                  PREVIEW_HIGHT, var.bits_per_pixel);
            // RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr),
            // (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres,
            // var.bits_per_pixel);
        }
    } else if (2 == camera_id) {
        // 2. stretch
        StretchColors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                      var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                      PREVIEW_HIGHT, var.bits_per_pixel);

        /*FIXME: here need 2 or 3 framebuffer pingpang ??*/
        if (!(frame_num % 2)) {

            // 3. rotation
            RGBRotate90_anticlockwise(
                (void *)(fb_buf[0].virt_addr), var.xres, var.yres,
                var.bits_per_pixel, (void *)fb_buf[2].virt_addr, PREVIEW_WIDTH,
                PREVIEW_HIGHT, var.bits_per_pixel);
        } else {

            // 3. rotation
            RGBRotate90_anticlockwise(
                (void *)(fb_buf[0].virt_addr), var.xres, var.yres,
                var.bits_per_pixel, (void *)fb_buf[2].virt_addr, PREVIEW_WIDTH,
                PREVIEW_HIGHT, var.bits_per_pixel);
            // RGBRotate90_anticlockwise((uint8_t *)(fb_buf[1].virt_addr),
            // (uint8_t*)fb_buf[2].virt_addr, var.yres, var.xres,
            // var.bits_per_pixel);
        }
    }

    /*lock*/
    previewLock.lock();
    ALOGI("Native MMI Test: "
          "fb_buf[0].virt_addr=0x%lx,var.xres=%d,var.yres=%d,var.bits_per_"
          "pixel=%d\n",
          fb_buf[0].virt_addr, var.xres, var.yres, var.bits_per_pixel);

    // 4. update
    eng_test_fb_update(frame);

    /*unlock*/
    previewLock.unlock();

    frame_num++;
}

static void freeCameraMem(sprd_camera_memory_t *memory) {
    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (!memory)
        return;

    if (memory->ion_heap) {
        if (s_mem_method)
            ; // memory->ion_heap->free_iova(ION_MM,memory->phys_addr,
              // memory->phys_size);

        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

#if defined(CONFIG_CAMERA_ISP_DIR_3)
static int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    HAL_LOGD("sum %d", sum);

    if (type == CAMERA_PREVIEW_RESERVED) {
        for (i = 0; i < PREV_RESERVED_FRM_CNT; i++) {
            if (NULL != mPreviewHeapReserved[i]) {
                freeCameraMem(mPreviewHeapReserved[i]);
                mPreviewHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_VIDEO_RESERVED) {
        for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
            if (NULL != mVideoHeapReserved[i]) {
                freeCameraMem(mVideoHeapReserved[i]);
                mVideoHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
            if (NULL != mZslHeapReserved[i]) {
                freeCameraMem(mZslHeapReserved[i]);
                mZslHeapReserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            freeCameraMem(mIspLscHeapReserved);
        }
        mIspLscHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAntiFlickerHeapReserved) {
            freeCameraMem(mIspAntiFlickerHeapReserved);
        }
        mIspAntiFlickerHeapReserved = NULL;
    }

    if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (NULL != mPdafRawHeapReserved) {
            freeCameraMem(mPdafRawHeapReserved);
        }
        mPdafRawHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                freeCameraMem(mIspB4awbHeapReserved[i]);
            }
            mIspB4awbHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            freeCameraMem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_SNAPSHOT_HIGHISO) {
        if (NULL != mHighIsoSnapshotHeapReserved) {
            freeCameraMem(mHighIsoSnapshotHeapReserved);
        }
        mHighIsoSnapshotHeapReserved = NULL;
    }

    if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspPreviewYReserved[i]) {
                freeCameraMem(mIspPreviewYReserved[i]);
            }
            mIspPreviewYReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (NULL != mIspYUVReserved) {
            freeCameraMem(mIspYUVReserved);
        }
        mIspYUVReserved = NULL;
    }

    if (type == CAMERA_ISP_RAW_DATA) {
        for (i = 0; i < sum; i++) {
            if (NULL != mIspRawDataReserved[i]) {
                freeCameraMem(mIspRawDataReserved[i]);
            }
            mIspRawDataReserved[i] = NULL;
        }
    }

    return 0;
}
#else
static int Callback_OtherFree(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                              cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    unsigned int i;
    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != mPreviewHeapReserved) {
            freeCameraMem(mPreviewHeapReserved);
            mPreviewHeapReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != mIspLscHeapReserved) {
            freeCameraMem(mIspLscHeapReserved);
            mIspLscHeapReserved = NULL;
        }
    }
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    if (type == CAMERA_ISP_STATIS) {
        if (NULL != mIspStatisHeapReserved) {
            mIspStatisHeapReserved->ion_heap->free_kaddr();
            freeCameraMem(mIspStatisHeapReserved);
            mIspStatisHeapReserved = NULL;
        }
    }
#endif
    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspB4awbHeapReserved[i]) {
                freeCameraMem(mIspB4awbHeapReserved[i]);
                mIspB4awbHeapReserved[i] = NULL;
            }
        }
    }
    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != mIspFirmwareReserved && !(--mIspFirmwareReserved_cnt)) {
            freeCameraMem(mIspFirmwareReserved);
            mIspFirmwareReserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < kISPB4awbCount; i++) {
            if (NULL != mIspRawAemHeapReserved[i]) {
                mIspRawAemHeapReserved[i]->ion_heap->free_kaddr();
                freeCameraMem(mIspRawAemHeapReserved[i]);
            }
            mIspRawAemHeapReserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != mIspAFLHeapReserved) {
            freeCameraMem(mIspAFLHeapReserved);
            mIspAFLHeapReserved = NULL;
        }
    }

    return 0;
}
#endif

static int Callback_PreviewFree(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*lock*/
    previewLock.lock();

    /*  */
    for (i = 0; i < mPreviewHeapNum; i++) {
        if (!previewHeapArray[i])
            continue;

        freeCameraMem(previewHeapArray[i]);
        previewHeapArray[i] = NULL;
    }

    mPreviewHeapNum = 0;

    /*unlock*/
    previewLock.unlock();

    return 0;
}

static cmr_int Callback_Free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data) {
    cmr_int ret = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
    if (!private_data || !vir_addr || !fd) {
        ALOGE("Native MMI Test: %s,%d, error param 0x%lx 0x%lx 0x%x 0x%lx\n",
              __func__, __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr, *fd,
              (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        ALOGE("Native MMI Test: %s,%d, mem type error %d\n", __func__, __LINE__,
              type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = Callback_PreviewFree(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
               || type == CAMERA_ISP_STATIS
#endif
               || type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = Callback_OtherFree(type, phy_addr, vir_addr, fd, sum);
    } else {
        ALOGE("Native MMI Test: %s,%s,%d, type ignore: %d, do nothing.\n",
              __FILE__, __func__, __LINE__, type);
    }

    /* disable preview flag */
    previewvalid = 0;

    return ret;
}

static sprd_camera_memory_t *allocCameraMem(int buf_size, int num_bufs,
                                            uint32_t is_cache) {
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    unsigned long paddr = 0;
    MemIon *pHeapIon = NULL;

    ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
    ALOGI("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        ALOGE("Native MMI Test: %s,%d, failed: fatal error! memory pointer is "
              "null.\n",
              __func__, __LINE__);
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        ALOGE("Native MMI Test: %s,%d, failed: mem size err.\n", __func__,
              __LINE__);
        goto getpmem_fail;
    }

    if (0 == s_mem_method) {
        ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_MM);
        }
    } else {
        ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        if (is_cache) {
            pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        ALOGE("Native MMI Test: %s,%s,%d, failed: pHeapIon is null or "
              "getHeapID failed.\n",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        ALOGE("Native MMI Test: error getBase is null. %s,%s,%d, failed: ion "
              "get base err.\n",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }
    ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    if (0 == s_mem_method) {
        ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              pHeapIon);
    } else {
        ALOGD("iommu: fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, "
              "heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              pHeapIon);
    }

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

static int Callback_PreviewMalloc(cmr_u32 size, cmr_u32 sum, cmr_uint *phy_addr,
                                  cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;

    ALOGI("size %d sum %d mPreviewHeapNum %d", size, sum, mPreviewHeapNum);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = allocCameraMem(size, 1, true);
        if (!memory) {
            ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n",
                  __func__, __LINE__);
            goto mem_fail;
        }

        previewHeapArray[i] = memory;
        mPreviewHeapNum++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    Callback_PreviewFree(0, 0, 0, 0);
    return -1;
}

#if defined(CONFIG_CAMERA_ISP_DIR_3)
static sprd_camera_memory_t *allocReservedMem(int buf_size, int num_bufs,
                                              uint32_t is_cache) {
    unsigned long paddr = 0;
    size_t psize = 0;
    int result = 0;
    size_t mem_size = 0;
    MemIon *pHeapIon = NULL;

    HAL_LOGD("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        HAL_LOGE("fatal error! memory pointer is null.");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (is_cache) {
        pHeapIon = new MemIon("/dev/ion", mem_size, 0,
                              (1 << 31) | ION_HEAP_ID_MASK_CAM);
    } else {
        pHeapIon = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                              ION_HEAP_ID_MASK_CAM);
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        HAL_LOGE("pHeapIon is null or getHeapID failed");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        HAL_LOGE("error getBase is null.");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    HAL_LOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,
             pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

static int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                                cmr_u32 sum, cmr_uint *phy_addr,
                                cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 mem_sum;
    int buffer_id;

    // cmr_u32 sum = *sum_ptr;

    HAL_LOGD("size=%d, sum=%d, mem_type=%d", size, sum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    if (type == CAMERA_PREVIEW_RESERVED) {
        for (i = 0; i < PREV_RESERVED_FRM_CNT; i++) {
            if (mPreviewHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mPreviewHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved[i]->data;
            *fd++ = mPreviewHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_VIDEO_RESERVED) {
        for (i = 0; i < VIDEO_RESERVED_FRM_CNT; i++) {
            if (mVideoHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mVideoHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mVideoHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mVideoHeapReserved[i]->data;
            *fd++ = mVideoHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_SNAPSHOT_ZSL_RESERVED) {
        for (i = 0; i < CAP_ZSL_RESERVED_FRM_CNT; i++) {
            if (mZslHeapReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    goto mem_fail;
                }
                mZslHeapReserved[i] = memory;
            }
            *phy_addr++ = (cmr_uint)mZslHeapReserved[i]->phys_addr;
            *vir_addr++ = (cmr_uint)mZslHeapReserved[i]->data;
            *fd++ = mZslHeapReserved[i]->fd;
        }
    } else if (type == CAMERA_SENSOR_DATATYPE_MAP_RESERVED) {
        if (mDepthHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mDepthHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mDepthHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mDepthHeapReserved->data;
        *fd++ = mDepthHeapReserved->fd;
    } else if (type == CAMERA_PDAF_RAW_RESERVED) {
        if (mPdafRawHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mPdafRawHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mPdafRawHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mPdafRawHeapReserved->data;
        *fd++ = mPdafRawHeapReserved->fd;
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null");
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
        *fd++ = mIspLscHeapReserved->fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAntiFlickerHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspAntiFlickerHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mIspAntiFlickerHeapReserved->data;
        *fd++ = mIspAntiFlickerHeapReserved->fd;
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else if (type == CAMERA_SNAPSHOT_HIGHISO) {
        if (mHighIsoSnapshotHeapReserved == NULL) {
            memory = allocReservedMem(size, 1, true);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mHighIsoSnapshotHeapReserved = memory;
        }
        *phy_addr++ = (cmr_uint)mHighIsoSnapshotHeapReserved->phys_addr;
        *vir_addr++ = (cmr_uint)mHighIsoSnapshotHeapReserved->data;
        *fd++ = mHighIsoSnapshotHeapReserved->fd;
    } else if (type == CAMERA_ISP_PREVIEW_Y) {
        for (i = 0; i < sum; i++) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspPreviewYReserved[i] = memory;
            *phy_addr++ = 0;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_PREVIEW_YUV) {
        if (mIspYUVReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                HAL_LOGE("memory is null.");
                goto mem_fail;
            }
            mIspYUVReserved = memory;
        }
        *phy_addr++ = 0;
        *vir_addr++ = (cmr_uint)mIspYUVReserved->data;
        *fd++ = mIspYUVReserved->fd;
    } else if (type == CAMERA_ISP_RAW_DATA) {
        cmr_u64 *kaddr = (cmr_u64 *)vir_addr;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            if (mIspRawDataReserved[i] == NULL) {
                memory = allocCameraMem(size, 1, true);
                if (NULL == memory) {
                    HAL_LOGE("memory is null.");
                    break;
                }
                mIspRawDataReserved[i] = memory;
            }
            //			*phy_addr++ = 0;
            *phy_addr++ = (cmr_uint)mIspRawDataReserved[i]->data;
            mIspRawDataReserved[i]->ion_heap->get_kaddr(kaddr, &ksize);
            kaddr++;
            *fd++ = mIspRawDataReserved[i]->fd;
            HAL_LOGD("isp raw data fd=0x%0x, vir_addr=%p",
                     mIspRawDataReserved[i]->fd, mIspRawDataReserved[i]->data);
        }
        //		*sum_ptr = i;
    }

    return 0;

mem_fail:
    Callback_OtherFree(type, 0, 0, 0, 0);
    return BAD_VALUE;
}
#else
static int Callback_OtherMalloc(enum camera_mem_cb_type type, cmr_u32 size,
                                cmr_u32 sum, cmr_uint *phy_addr,
                                cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;
    cmr_u32 mem_size;
    cmr_u32 mem_sum;
    int buffer_id;
    ALOGD("size %d sum %d mPreviewHeapNum %d, type %d", size, sum,
          mPreviewHeapNum, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == mPreviewHeapReserved) {
            ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__,
                  __LINE__);
            memory = allocCameraMem(size, 1, true);
            if (NULL == memory) {
                ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                LOGE("error memory is null.");
                goto mem_fail;
            }
            mPreviewHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__,
                  __LINE__);
            ALOGI("malloc Common memory for preview, video, and zsl, malloced "
                  "type %d,request num %d, request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)mPreviewHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mPreviewHeapReserved->data;
            *fd++ = mPreviewHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_LSC) {
        if (mIspLscHeapReserved == NULL) {
            ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__,
                  __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                ALOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            mIspLscHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp lsc memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)mIspLscHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspLscHeapReserved->data;
            *fd++ = mIspLscHeapReserved->fd;
        }
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
    } else if (type == CAMERA_ISP_STATIS) {

        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (mIspStatisHeapReserved == NULL) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("memory is null");
                goto mem_fail;
            }
            mIspStatisHeapReserved = memory;
        }
        memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr = kaddr;
        *vir_addr++ = (cmr_uint)mIspStatisHeapReserved->data;
        *fd++ = mIspStatisHeapReserved->fd;
        *fd++ = mIspStatisHeapReserved->dev_fd;
#endif
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__,
                  __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            mIspB4awbHeapReserved[i] = memory;
            *phy_addr_64++ = (cmr_u64)memory->phys_addr;
            *vir_addr_64++ = (cmr_u64)memory->data;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr++ = kaddr;
            *phy_addr = kaddr >> 32;
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_RAWAE) {
        cmr_u64 kaddr = 0;
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        size_t ksize = 0;
        for (i = 0; i < sum; i++) {
            ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__,
                  __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("Native MMI Test: error memory is null,malloced type %d",
                      type);
                goto mem_fail;
            }
            mIspRawAemHeapReserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (mIspAFLHeapReserved == NULL) {
            ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__,
                  __LINE__);
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            mIspAFLHeapReserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp afl memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)mIspAFLHeapReserved->phys_addr;
            *vir_addr++ = (cmr_uint)mIspAFLHeapReserved->data;
            *fd++ = mIspAFLHeapReserved->fd;
        }
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        if (++mIspFirmwareReserved_cnt == 1) {
            memory = allocCameraMem(size, 1, false);
            if (NULL == memory) {
                ALOGE("Native MMI Test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            mIspFirmwareReserved = memory;
        } else {
            memory = mIspFirmwareReserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else {
        ALOGE("Native MMI Test: %s,%s,%d, type ignore: %d, do nothing.\n",
              __FILE__, __func__, __LINE__, type);
    }

    return 0;

mem_fail:
    Callback_OtherFree(type, 0, 0, 0, 0);
    return -1;
}
#endif
static cmr_int Callback_Malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;

    ALOGI("Native MMI Test: %s,%s,%d, type %d IN\n", __FILE__, __func__,
          __LINE__, type);

    /*lock*/
    previewLock.lock();

    if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) ||
        (0 == *sum_ptr)) {
        ALOGE("Native MMI Test: %s,%d, alloc param error 0x%lx 0x%lx 0x%lx\n",
              __func__, __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr,
              (cmr_uint)size_ptr);
        /*unlock*/
        previewLock.unlock();
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        /* preview buffer */
        ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        ret = Callback_PreviewMalloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE
#if defined(CONFIG_CAMERA_ISP_DIR_2_1)
               || type == CAMERA_ISP_STATIS
#endif
               || type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        ret = Callback_OtherMalloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        ALOGE("Native MMI Test: %s,%s,%d, type ignore: %d, do nothing.\n",
              __FILE__, __func__, __LINE__, type);
    }

    /* enable preview flag */
    previewvalid = 1;

    /*unlock*/
    previewLock.unlock();

    return ret;
}

static void eng_tst_camera_startpreview(void) {
    cmr_int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;
    struct img_size capture_size;
    struct cmr_preview_fps_param fps_param;

    if (!oem_handle || NULL == mHalOem || NULL == mHalOem->ops)
        return;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    /*  */
    preview_size.width = PREVIEW_WIDTH;
    preview_size.height = PREVIEW_HIGHT;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio =
        (float)PREVIEW_WIDTH / PREVIEW_HIGHT;

    fps_param.frame_rate = 25;
    fps_param.video_mode = 0;

    /*  */
    mHalOem->ops->camera_fast_ctrl(oem_handle, CAMERA_FAST_MODE_FD, 0);

    /*  */
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);
#if defined(CONFIG_CAMERA_ISP_DIR_3)
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_AF_MODE, CAMERA_FOCUS_MODE_CAF);
#endif
    // SET_PARM(oem_handle , CAMERA_PARAM_VIDEO_SIZE     ,
    // (cmr_uint)&video_size);
    // SET_PARM(oem_handle , CAMERA_PARAM_CAPTURE_SIZE   ,
    // (cmr_uint)&capture_size);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
             CAMERA_DATA_FORMAT_YUV420);
    // SET_PARM(oem_handle , CAMERA_PARAM_CAPTURE_FORMAT ,
    // CAMERA_DATA_FORMAT_YUV420);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    SET_PARM(mHalOem, oem_handle, CAMERA_PARAM_PREVIEW_FPS,
             (cmr_uint)&fps_param);

    /* set malloc && free callback*/
    ret = mHalOem->ops->camera_set_mem_func(oem_handle, (void *)Callback_Malloc,
                                            (void *)Callback_Free, NULL);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("Native MMI Test: %s,%d, failed: camera set mem func error.\n",
              __func__, __LINE__);
        return;
    }

    /*start preview*/
    ret = mHalOem->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("Native MMI Test: %s,%d, failed: camera start preview error.\n",
              __func__, __LINE__);
        return;
    }
}

static int eng_tst_camera_stoppreview(void) {
    int ret;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    if (!oem_handle || NULL == mHalOem || NULL == mHalOem->ops) {
        ALOGI("Native MMI Test: oem is null or oem ops is null, do nothing\n");
        return -1;
    }

    ret = mHalOem->ops->camera_stop_preview(oem_handle);

    return ret;
}

extern "C" {
int eng_tst_camera_deinit() {
    cmr_int ret;

    ALOGI("Native MMI Test: %s,%d IN\n", __func__, __LINE__);

    ret = eng_tst_camera_stoppreview();

    if (oem_handle != NULL && mHalOem != NULL && mHalOem->ops != NULL) {
        ret = mHalOem->ops->camera_deinit(oem_handle);
    }

    //	memcpy(gr_draw->data, tmpbuf1, lcd_w*lcd_h*4);
    //	gr_draw = gr_backend->flip(gr_backend);
    //	memcpy(gr_draw->data, tmpbuf1, lcd_w*lcd_h*4);

    if (NULL != mHalOem->dso) {
        dlclose(mHalOem->dso);
    }
    free((void *)mHalOem);
    mHalOem = NULL;

    if (remove(af_tuning_path) != 0) {
        ALOGE(
            "Native MMI Test: %s,%d, failed: to delete af_tuning_path file \n",
            __func__, __LINE__);
    }
    if (remove(sensor_para_path) != 0) {
        ALOGE("Native MMI Test: %s,%d, failed: to delete sensor_para_path file "
              "\n",
              __func__, __LINE__);
    }
    //	free(tmpbuf);
    //	free(tmpbuf1);
    free(tmpbuf2);
    free(tmpbuf3);
    return ret;
}
cmr_int autotest_load_hal_lib(void) {
    int ret = 0;
    if (!mHalOem) {
        void *handle;
        oem_module_t *omi;

        mHalOem = (oem_module_t *)malloc(sizeof(oem_module_t));

        handle = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);

        if (handle == NULL) {
            char const *err_str = dlerror();
            ALOGE("dlopen error%s", err_str ? err_str : "unknown");
            ret = -1;
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info. */
        const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
        omi = (oem_module_t *)dlsym(handle, sym);
        if (omi == NULL) {
            ALOGE("load: couldn't find symbol %s", sym);
            ret = -1;
            goto loaderror;
        }

        mHalOem->dso = handle;
        mHalOem->ops = omi->ops;

        ALOGV("loaded HAL libcamoem handle=%p", handle);
    }

    return ret;

loaderror:

    if (NULL != mHalOem->dso) {
        dlclose(mHalOem->dso);
    }
    free((void *)mHalOem);
    mHalOem = NULL;
    return ret;
}
int eng_tst_camera_init(int cameraId, minui_backend *backend, GRSurface *draw) {
    int ret = 0;

    ALOGI("Native MMI Test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
    //	gr_init();
    gr_backend = backend;
    gr_draw = draw;
    if (2 == cameraId)
        camera_id = 2; // auxiliary camera
    else if (1 == cameraId || 3 == cameraId)
        camera_id = 1; // fore camera
    else
        camera_id = 0; // back camera
    if (getLcdSize(&lcd_w, &lcd_h)) {
        /*update preivew size by lcd*/
        ALOGI("%s Native MMI Test: lcd_w=%d,lcd_h=%d\n", __func__, lcd_w,
              lcd_h);
    }
    //	tmpbuf = (uint8_t*)malloc(lcd_w*lcd_h*4);
    //	tmpbuf1 = (uint8_t*)malloc(lcd_w*lcd_h*4);
    tmpbuf2 = (uint8_t *)malloc(lcd_w * lcd_h * 4);
    tmpbuf3 = (uint8_t *)malloc(lcd_w * lcd_h * 4);
    //	memcpy(tmpbuf1, gr_draw->data, lcd_w*lcd_h*4);
    eng_test_fb_open();
    if (autotest_load_hal_lib()) {
        return -1;
    }

    ret = mHalOem->ops->camera_init(cameraId, eng_tst_camera_cb, &client_data,
                                    0, &oem_handle, (void *)Callback_Malloc,
                                    (void *)Callback_Free);
    s_mem_method = IommuIsEnabled();
    ALOGI("Native MMI Test: %s,%s,%d, s_mem_method %d IN\n", __FILE__, __func__,
          __LINE__, s_mem_method);

    eng_tst_camera_startpreview();

    return ret;
}
}
