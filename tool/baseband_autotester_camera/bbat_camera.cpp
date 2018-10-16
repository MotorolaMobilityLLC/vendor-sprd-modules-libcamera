#include <utils/Log.h>
#if defined(CONFIG_ISP_2_1) || defined(CONFIG_ISP_2_2) ||                      \
    defined(CONFIG_ISP_2_3) || defined(CONFIG_ISP_2_5) || defined(CONFIG_ISP_2_6)
#if defined(CONFIG_ISP_2_1_A)
#include "hal3_2v1a/SprdCamera3OEMIf.h"
#include "hal3_2v1a/SprdCamera3Setting.h"
#else
#include "hal3_2v1/SprdCamera3OEMIf.h"
#include "hal3_2v1/SprdCamera3Setting.h"
#endif
#endif
#if defined(CONFIG_ISP_2_4)
#include "hal3_2v4/SprdCamera3OEMIf.h"
#include "hal3_2v4/SprdCamera3Setting.h"
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
#include <linux/fb.h>
#include "sprd_cpp.h"
#include <dlfcn.h>
#include "eng_modules.h"
#include "eng_diag.h"

/*yuv->rgb*/
#define RGB565(r, g, b)                                                        \
    ((unsigned short)((((unsigned char)(r) >> 3) |                             \
                       ((unsigned short)(((unsigned char)(g) >> 2)) << 5)) |   \
                      (((unsigned short)((unsigned char)(b >> 3))) << 11)))

#define ROT_DEV "/dev/sprd_rotation"

using namespace sprdcamera;
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

/*rotation device nod*/
static int rot_fd = -1;

/*process control*/
static Mutex preview_lock;          /*preview lock*/
static int preview_valid = 0;       /*preview flag*/
static int s_mem_method = 0;        /*0: physical address, 1: iommu  address*/
static unsigned char camera_id = 0; /*camera id: fore=1,back=0*/

/*data processing useful*/
#define PREVIEW_WIDTH 960  // 1600//1280//640
#define PREVIEW_HIGHT 720  // 1200//960//480
#define PREVIEW_BUFF_NUM 8 /*preview buffer*/
#define SPRD_MAX_PREVIEW_BUF PREVIEW_BUFF_NUM
struct frame_buffer_t {
    cmr_uint phys_addr;
    cmr_uint virt_addr;
    cmr_uint length; // buffer's length is different from cap_image_size
};
static struct frame_buffer_t fb_buf[SPRD_MAX_PREVIEW_BUF + 1];
static uint8_t *tmp_buf2, *tmp_buf3; //*tmpbuf1, *tmp_buf,
static uint32_t post_preview_buf[PREVIEW_WIDTH * PREVIEW_HIGHT];
static struct fb_var_screeninfo var;
static uint32_t frame_num = 0; /*record frame number*/

static unsigned int m_preview_heap_num = 0; /*allocated preview buffer number*/
static sprd_camera_memory_t *m_preview_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_lsc_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_statis_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_afl_heap_reserved = NULL;
static sprd_camera_memory_t *m_isp_firmware_reserved = NULL;
static const int k_isp_b4_awb_count = 16;
static sprd_camera_memory_t *m_isp_b4_awb_heap_reserved[k_isp_b4_awb_count];
static sprd_camera_memory_t *m_isp_raw_aem_heap_reserved[k_isp_b4_awb_count];
static sprd_camera_memory_t *preview_heap_array[PREVIEW_BUFF_NUM];
static int target_buffer_id;

static oem_module_t *m_hal_oem;

struct client_t {
    int reserved;
};
static struct client_t client_data;
static cmr_handle oem_handle = 0;

static uint32_t lcd_w = 0, lcd_h = 0;

int autotest_iommu_is_enabled(void) {
    int ret;
    int iommu_is_enabled = 0;

    if (NULL == oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        ALOGE("auto_test: %s,%d, oem is null or oem ops is null.\n", __func__,
              __LINE__);
        return 0;
    }

    ret = m_hal_oem->ops->camera_get_iommu_status(oem_handle);
    if (ret) {
        iommu_is_enabled = 0;
        return iommu_is_enabled;
    }

    iommu_is_enabled = 1;
    return iommu_is_enabled;
}

static bool get_lcd_size(uint32_t *p_width, uint32_t *p_height) {
    if (NULL != gr_draw) {
        *p_width = (uint32_t)gr_draw->width;
        *p_height = (uint32_t)gr_draw->height;
    }
    return true;
}

static void rgb_rotate90_anticlockwise(uint8_t *des, uint8_t *src, int width,
                                       int height, int bits) {

    int i, j;
    int m = bits >> 3;
    int size;

    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    if ((!des) || (!src))
        return;
    if (m == 4) {
        for (j = 0; j < height; j++) {
            size = 0;
            for (i = 0; i < width; i++) {
                size += height;
                *(des32 + size - j - 1) = *src32++;
            }
        }
    } else {
        for (j = 0; j < height; j++) {
            size = 0;
            for (i = 0; i < width; i++) {
                size += height;
                *(des16 + size - j - 1) = *src16++;
            }
        }
    }
}

static void rgb_rotate90_clockwise(uint8_t *des, uint8_t *src, int width,
                                   int height, int bits) {
    int i, j;
    int m = bits >> 3;
    int size;

    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    if ((!des) || (!src))
        return;
    if (m == 4) {
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                size = (i + 1) * height;
                *(des32 + size - j - 1) = *src32++;
            }
        }
    } else {
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                size = (i + 1) * height;
                *(des16 + size - j - 1) = *src16++;
            }
        }
    }
}

static void yuv_rotate90(uint8_t *des, uint8_t *src, int width, int height) {
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

static void stretch_colors(void *dst, int dst_width, int dst_height,
                           int dst_bits, void *p_drc, int src_width,
                           int src_height, int src_bits) {
    double df_amplification_x = ((double)dst_width) / src_width;
    double df_amplification_y = ((double)dst_height) / src_height;
    double stretch =
        1 / (df_amplification_y > df_amplification_x ? df_amplification_y
                                                     : df_amplification_x);
    int offset_x = (df_amplification_y > df_amplification_x
                        ? (int)(src_width - dst_width / df_amplification_y) >> 1
                        : 0);
    int offset_y =
        (df_amplification_x > df_amplification_y
             ? (int)(src_height - dst_height / df_amplification_x) >> 1
             : 0);
    int i = 0;
    int j = 0;
    double tmp_y = 0;
    unsigned int *src_pos = (unsigned int *)p_drc;
    unsigned int *dst_pos = (unsigned int *)dst;
    int line_size;
    ALOGI("auto_test: dst_width = %d, dst_height = %d, src_width = %d, "
          "src_height = %d, offset_x = %d, offset_y= %d \n",
          dst_width, dst_height, src_width, src_height, offset_x, offset_y);

    for (i = 0; i < dst_height; i++) {
        double tmp_x = 0;

        int n_line = (int)(tmp_y + 0.5) + offset_y;

        line_size = n_line * src_width;

        for (j = 0; j < dst_width; j++) {

            int nRow = (int)(tmp_x + 0.5) + offset_x;

            *dst_pos++ = *(src_pos + line_size + nRow);

            tmp_x += stretch;
        }
        tmp_y += stretch;
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

static void auto_dcamtest_switch_tb(uint8_t *buffer, uint16_t width,
                                    uint16_t height, uint8_t pixel) {
    uint32_t j;
    uint32_t line_size;
    uint8_t *dst = NULL;
    uint8_t *src = NULL;
    uint8_t *tmp_buf = NULL;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    line_size = width * (pixel / 8);

    tmp_buf = (uint8_t *)malloc(line_size);
    if (!tmp_buf) {
        ALOGE("auto_test: %s,%d Fail to alloc temp buffer\n", __func__,
              __LINE__);
        return;
    }

    for (j = 0; j < height / 2; j++) {
        src = buffer + j * line_size;
        dst = buffer + height * line_size - (j + 1) * line_size;
        memcpy(tmp_buf, src, line_size);

        for (j = 0; j < height / 2; j++) {
            src = buffer + j * line_size;
            dst = buffer + height * line_size - (j + 1) * line_size;
            memcpy(tmp_buf, src, line_size);
            memcpy(src, dst, line_size);
            memcpy(dst, tmp_buf, line_size);
        }
    }
    free(tmp_buf);
}

static int autotest_rotation(uint32_t agree, uint32_t width, uint32_t height,
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
        ALOGE("auto_test: %s,%d Fail to open rotation device\n", __func__,
              __LINE__);
        return -1;
    }

    /* call ioctl */
    if (-1 == ioctl(rot_fd, SPRD_CPP_IO_START_ROT, &rot_params)) {
        ALOGE("auto_test: %s,%d Fail to SC8800G_ROTATION_DONE\n", __func__,
              __LINE__);
        return -1;
    }

    return 0;
}

static int autotest_fb_open(void) {

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    var.yres = lcd_h;
    var.xres = lcd_w;
    var.bits_per_pixel = (8 * gr_draw->pixel_bytes);
    ALOGI("auto_test: var.yres = %d, var.xres = %d, var.bits_per_pixel = %d\n",
          var.yres, var.xres, var.bits_per_pixel);
    /* set framebuffer address */
    memset(&fb_buf, 0, sizeof(fb_buf));

    fb_buf[0].virt_addr = (size_t)gr_draw->data;

    fb_buf[1].virt_addr =
        (size_t)var.yres * var.xres * (var.bits_per_pixel / 8);

    fb_buf[2].virt_addr = (size_t)tmp_buf2;

    fb_buf[3].virt_addr = (size_t)tmp_buf3;

    return 0;
}

static unsigned int get_preview_buffer_id_for_fd(cmr_s32 fd) {
    unsigned int i = 0;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        if (!preview_heap_array[i])
            continue;

        if (!(cmr_uint)preview_heap_array[i]->fd)
            continue;

        if (preview_heap_array[i]->fd == fd)
            return i;
    }

    return 0xFFFFFFFF;
}

static void autotest_fb_update(const camera_frame_type *frame) {

    unsigned int buffer_id;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    if (!frame)
        return;

    buffer_id = get_preview_buffer_id_for_fd(frame->fd);
    if (0xFFFFFFFF == buffer_id) {
        ALOGI("auto_test: buffer id is invalid, do nothing\n");
        return;
    }

    if (NULL != gr_draw) {
        gr_draw = gr_backend->flip(gr_backend);
        fb_buf[0].virt_addr = (size_t)gr_draw->data;
    }

    if (!preview_heap_array[buffer_id]) {
        ALOGI("auto_test: %s,%d preview heap array empty, do nothing\n",
              __func__, __LINE__);
        return;
    }

    if (NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        ALOGI("auto_test: oem is null or oem ops is null, do nothing\n");
        return;
    }

    m_hal_oem->ops->camera_set_preview_buffer(
        oem_handle, (cmr_uint)preview_heap_array[buffer_id]->phys_addr,
        (cmr_uint)preview_heap_array[buffer_id]->data,
        (cmr_s32)preview_heap_array[buffer_id]->fd);
}

void autotest_camera_close(void) {
    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);
    return;
}

int autotest_flashlight_ctrl(uint32_t flash_status) {
    int ret = 0;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    return ret;
}

static void data_mirror(uint8_t *des, uint8_t *src, int width, int height,
                        int bits) {
    int size = 0;
    int i, j;
    int m = bits >> 3;
    unsigned int *des32 = (unsigned int *)des;
    unsigned int *src32 = (unsigned int *)src;
    unsigned short *des16 = (unsigned short *)des;
    unsigned short *src16 = (unsigned short *)src;

    if ((!des) || (!src))
        return;

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

void autotest_camera_cb(enum camera_cb_type cb, const void *client_data,
                        enum camera_func_type func, void *parm4) {
    struct camera_frame_type *frame = (struct camera_frame_type *)parm4;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    if (!frame) {
        ALOGI("auto_test: %s,%d, camera call back parm4 error: NULL, do "
              "nothing\n",
              __func__, __LINE__);
        return;
    }

    if (CAMERA_FUNC_START_PREVIEW != func) {
        ALOGI("auto_test: %s,%d, camera func type error: %d, do nothing\n",
              __func__, __LINE__, func);
        return;
    }

    if (CAMERA_EVT_CB_FRAME != cb) {
        ALOGI("auto_test: %s,%d, camera cb type error: %d, do nothing\n",
              __func__, __LINE__, cb);
        return;
    }

    /*lock*/
    preview_lock.lock();

    /*empty preview arry, do nothing*/
    if (!preview_heap_array[frame->buf_id]) {
        ALOGI("auto_test: %s,%d, preview heap array empty, do nothine\n",
              __func__, __LINE__);
        preview_lock.unlock();
        return;
    }

    /*get frame buffer id*/
    frame->buf_id = get_preview_buffer_id_for_fd(frame->fd);
    if (0xFFFFFFFF == frame->buf_id) {
        ALOGI("auto_test: buffer id is invalid, do nothing\n");
        preview_lock.unlock();
        return;
    }

    /*preview enable or disable?*/
    if (!preview_valid) {
        ALOGI("auto_test: %s,%d, preview disabled, do nothing\n", __func__,
              __LINE__);
        preview_lock.unlock();
        return;
    }

    unsigned int format = FMT_NV21;

    // 1.yuv -> rgb
    yuv420_to_rgb(PREVIEW_WIDTH, PREVIEW_HIGHT,
                  (unsigned char *)preview_heap_array[frame->buf_id]->data,
                  post_preview_buf, format);

    /*unlock*/
    preview_lock.unlock();

    /* fore && back camera: istrech,mirror,rotate*/
    if (0 == camera_id) {
        // 2. stretch
        stretch_colors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                       var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                       PREVIEW_HIGHT, var.bits_per_pixel);

        /*FIXME: here need 2 or 3 framebuffer pingpang ??*/
        if (!(frame_num % 2)) {

            // 3. rotation
            rgb_rotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                       (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                       var.xres, var.bits_per_pixel);
        } else {

            // 3. rotation
            rgb_rotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                       (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                       var.xres, var.bits_per_pixel);
        }
    } else if (1 == camera_id) {
        // 2. stretch
        stretch_colors((void *)(fb_buf[3].virt_addr), var.yres, var.xres,
                       var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                       PREVIEW_HIGHT, var.bits_per_pixel);
        // mirror fore camera
        data_mirror((uint8_t *)(fb_buf[2].virt_addr),
                    (uint8_t *)(fb_buf[3].virt_addr), var.yres, var.xres,
                    var.bits_per_pixel);
        if (!(frame_num % 2)) {
            rgb_rotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                       (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                       var.xres, var.bits_per_pixel);
        } else {
            rgb_rotate90_anticlockwise((uint8_t *)(fb_buf[0].virt_addr),
                                       (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                       var.xres, var.bits_per_pixel);
        }
    } else if (2 == camera_id) {
        // 2. stretch
        stretch_colors((void *)(fb_buf[2].virt_addr), var.yres, var.xres,
                       var.bits_per_pixel, post_preview_buf, PREVIEW_WIDTH,
                       PREVIEW_HIGHT, var.bits_per_pixel);

        /*FIXME: here need 2 or 3 framebuffer pingpang ??*/
        if (!(frame_num % 2)) {

            // 3. rotation
            rgb_rotate90_clockwise((uint8_t *)(fb_buf[0].virt_addr),
                                   (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                   var.xres, var.bits_per_pixel);
        } else {

            // 3. rotation
            rgb_rotate90_clockwise((uint8_t *)(fb_buf[0].virt_addr),
                                   (uint8_t *)fb_buf[2].virt_addr, var.yres,
                                   var.xres, var.bits_per_pixel);
        }
    }

    /*lock*/
    preview_lock.lock();
    ALOGI("auto_test: "
          "fb_buf[0].virt_addr=0x%lx,var.xres=%d,var.yres=%d,var.bits_per_"
          "pixel=%d\n",
          fb_buf[0].virt_addr, var.xres, var.yres, var.bits_per_pixel);

    // 4. update
    autotest_fb_update(frame);

    /*unlock*/
    preview_lock.unlock();

    frame_num++;
}

static void free_camera_mem(sprd_camera_memory_t *memory) {
    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    if (!memory)
        return;

    if (memory->ion_heap) {
        delete memory->ion_heap;
        memory->ion_heap = NULL;
    }

    free(memory);
}

static int callback_other_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum) {
    int i;
    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL != m_preview_heap_reserved) {
            free_camera_mem(m_preview_heap_reserved);
            m_preview_heap_reserved = NULL;
        }
    }

    if (type == CAMERA_ISP_LSC) {
        if (NULL != m_isp_lsc_heap_reserved) {
            free_camera_mem(m_isp_lsc_heap_reserved);
            m_isp_lsc_heap_reserved = NULL;
        }
    }
    if (type == CAMERA_ISP_STATIS) {
        if (NULL != m_isp_statis_heap_reserved) {
            m_isp_statis_heap_reserved->ion_heap->free_kaddr();
            free_camera_mem(m_isp_statis_heap_reserved);
            m_isp_statis_heap_reserved = NULL;
        }
    }
    if (type == CAMERA_ISP_BINGING4AWB) {
        for (i = 0; i < k_isp_b4_awb_count; i++) {
            if (NULL != m_isp_b4_awb_heap_reserved[i]) {
                free_camera_mem(m_isp_b4_awb_heap_reserved[i]);
                m_isp_b4_awb_heap_reserved[i] = NULL;
            }
        }
    }

    if (type == CAMERA_ISP_FIRMWARE) {
        if (NULL != m_isp_firmware_reserved) {
            free_camera_mem(m_isp_firmware_reserved);
            m_isp_firmware_reserved = NULL;
        }
    }

    if (type == CAMERA_ISP_RAWAE) {
        for (i = 0; i < k_isp_b4_awb_count; i++) {
            if (NULL != m_isp_raw_aem_heap_reserved[i]) {
                m_isp_raw_aem_heap_reserved[i]->ion_heap->free_kaddr();
                free_camera_mem(m_isp_raw_aem_heap_reserved[i]);
            }
            m_isp_raw_aem_heap_reserved[i] = NULL;
        }
    }

    if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (NULL != m_isp_afl_heap_reserved) {
            free_camera_mem(m_isp_afl_heap_reserved);
            m_isp_afl_heap_reserved = NULL;
        }
    }

    return 0;
}

static int callback_preview_free(cmr_uint *phy_addr, cmr_uint *vir_addr,
                                 cmr_s32 *fd, cmr_u32 sum) {
    cmr_u32 i;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    /*lock*/
    preview_lock.lock();

    for (i = 0; i < m_preview_heap_num; i++) {
        if (!preview_heap_array[i])
            continue;

        free_camera_mem(preview_heap_array[i]);
        preview_heap_array[i] = NULL;
    }

    m_preview_heap_num = 0;

    /*unlock*/
    preview_lock.unlock();

    return 0;
}

static cmr_int callback_free(enum camera_mem_cb_type type, cmr_uint *phy_addr,
                             cmr_uint *vir_addr, cmr_s32 *fd, cmr_u32 sum,
                             void *private_data) {
    cmr_int ret = 0;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    if (!private_data || !vir_addr || !fd) {
        ALOGE("auto_test: %s,%d, error param 0x%lx 0x%lx 0x%lx\n", __func__,
              __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr,
              (cmr_uint)private_data);
        return -1;
    }

    if (CAMERA_MEM_CB_TYPE_MAX <= type) {
        ALOGE("auto_test: %s,%d, mem type error %d\n", __func__, __LINE__,
              type);
        return -1;
    }

    if (CAMERA_PREVIEW == type) {
        ret = callback_preview_free(phy_addr, vir_addr, fd, sum);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ret = callback_other_free(type, phy_addr, vir_addr, fd, sum);
    } else {
        ALOGE("auto_test: %s,%s,%d, type ignore: %d, do nothing.\n", __FILE__,
              __func__, __LINE__, type);
    }

    /* disable preview flag */
    preview_valid = 0;

    return ret;
}

static sprd_camera_memory_t *alloc_camera_mem(int buf_size, int num_bufs,
                                              uint32_t is_cache) {

    size_t mem_size = 0;
    MemIon *p_heap_ion = NULL;

    ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
    ALOGI("buf_size %d, num_bufs %d", buf_size, num_bufs);

    sprd_camera_memory_t *memory =
        (sprd_camera_memory_t *)malloc(sizeof(sprd_camera_memory_t));
    if (NULL == memory) {
        ALOGE(
            "auto_test: %s,%d, failed: fatal error! memory pointer is null.\n",
            __func__, __LINE__);
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        ALOGE("auto_test: %s,%d, failed: mem size err.\n", __func__, __LINE__);
        goto getpmem_fail;
    }

    if (0 == s_mem_method) {
        ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        if (is_cache) {
            p_heap_ion = new MemIon("/dev/ion", mem_size, 0,
                                    (1 << 31) | ION_HEAP_ID_MASK_MM);
        } else {
            p_heap_ion = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                    ION_HEAP_ID_MASK_MM);
        }
    } else {
        ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        if (is_cache) {
            p_heap_ion = new MemIon("/dev/ion", mem_size, 0,
                                    (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            p_heap_ion = new MemIon("/dev/ion", mem_size, MemIon::NO_CACHING,
                                    ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (p_heap_ion == NULL || p_heap_ion->getHeapID() < 0) {
        ALOGE("auto_test: %s,%s,%d, failed: p_heap_ion is null or getHeapID "
              "failed.\n",
              __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }

    if (NULL == p_heap_ion->getBase() || MAP_FAILED == p_heap_ion->getBase()) {
        ALOGE(
            "auto_test: error getBase is null. %s,%s,%d, failed: ion get base "
            "err.\n",
            __FILE__, __func__, __LINE__);
        goto getpmem_fail;
    }
    ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);

    memory->ion_heap = p_heap_ion;
    memory->fd = p_heap_ion->getHeapID();
    memory->dev_fd = p_heap_ion->getIonDeviceFd();
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = p_heap_ion->getBase();

    if (0 == s_mem_method) {
        ALOGD("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              p_heap_ion);
    } else {
        ALOGD("iommu: fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, "
              "heap=%p",
              memory->fd, memory->phys_addr, memory->data, memory->phys_size,
              p_heap_ion);
    }

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}

static int callback_preview_malloc(cmr_u32 size, cmr_u32 sum,
                                   cmr_uint *phy_addr, cmr_uint *vir_addr,
                                   cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_uint i = 0;

    ALOGI("size %d sum %d m_preview_heap_num %d", size, sum,
          m_preview_heap_num);

    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    for (i = 0; i < PREVIEW_BUFF_NUM; i++) {
        memory = alloc_camera_mem(size, 1, true);
        if (!memory) {
            ALOGE("auto_test: %s,%d, failed: alloc camera mem err.\n", __func__,
                  __LINE__);
            goto mem_fail;
        }

        preview_heap_array[i] = memory;
        m_preview_heap_num++;

        *phy_addr++ = (cmr_uint)memory->phys_addr;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
    }

    return 0;

mem_fail:
    callback_preview_free(0, 0, 0, 0);
    return -1;
}

static int callback_other_malloc(enum camera_mem_cb_type type, cmr_u32 size,
                                 cmr_u32 sum, cmr_uint *phy_addr,
                                 cmr_uint *vir_addr, cmr_s32 *fd) {
    sprd_camera_memory_t *memory = NULL;
    cmr_u32 i;

    ALOGD("size %d sum %d m_preview_heap_num %d, type %d", size, sum,
          m_preview_heap_num, type);
    *phy_addr = 0;
    *vir_addr = 0;
    *fd = 0;
    ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);

    if (type == CAMERA_PREVIEW_RESERVED) {
        if (NULL == m_preview_heap_reserved) {
            ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, true);
            if (NULL == memory) {
                ALOGE("auto_test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                LOGE("error memory is null.");
                goto mem_fail;
            }
            m_preview_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            ALOGI("malloc Common memory for preview, video, and zsl, malloced "
                  "type %d,request num %d, request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)m_preview_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_preview_heap_reserved->data;
            *fd++ = m_preview_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_LSC) {
        if (m_isp_lsc_heap_reserved == NULL) {
            ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("auto_test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                ALOGE("error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            m_isp_lsc_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp lsc memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)m_isp_lsc_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_isp_lsc_heap_reserved->data;
            *fd++ = m_isp_lsc_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_STATIS) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (m_isp_statis_heap_reserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("memory is null");
                goto mem_fail;
            }
            m_isp_statis_heap_reserved = memory;
        }
        m_isp_statis_heap_reserved->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr = kaddr >> 32;
        *vir_addr++ = (cmr_uint)m_isp_statis_heap_reserved->data;
        *fd++ = m_isp_statis_heap_reserved->fd;
        *fd++ = m_isp_statis_heap_reserved->dev_fd;
    } else if (type == CAMERA_ISP_BINGING4AWB) {
        cmr_u64 *phy_addr_64 = (cmr_u64 *)phy_addr;
        cmr_u64 *vir_addr_64 = (cmr_u64 *)vir_addr;
        cmr_u64 kaddr = 0;
        size_t ksize = 0;

        for (i = 0; i < sum; i++) {
            ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("auto_test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            m_isp_b4_awb_heap_reserved[i] = memory;
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
            ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("auto_test: error memory is null,malloced type %d", type);
                goto mem_fail;
            }
            m_isp_raw_aem_heap_reserved[i] = memory;
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
            *phy_addr_64++ = kaddr;
            *vir_addr_64++ = (cmr_u64)(memory->data);
            *fd++ = memory->fd;
        }
    } else if (type == CAMERA_ISP_ANTI_FLICKER) {
        if (m_isp_afl_heap_reserved == NULL) {
            ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("auto_test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            m_isp_afl_heap_reserved = memory;
            *phy_addr++ = (cmr_uint)memory->phys_addr;
            *vir_addr++ = (cmr_uint)memory->data;
            *fd++ = memory->fd;
        } else {
            ALOGI("malloc isp afl memory, malloced type %d,request num %d, "
                  "request size 0x%x",
                  type, sum, size);
            *phy_addr++ = (cmr_uint)m_isp_afl_heap_reserved->phys_addr;
            *vir_addr++ = (cmr_uint)m_isp_afl_heap_reserved->data;
            *fd++ = m_isp_afl_heap_reserved->fd;
        }
    } else if (type == CAMERA_ISP_FIRMWARE) {
        cmr_u64 kaddr = 0;
        size_t ksize = 0;
        if (m_isp_firmware_reserved == NULL) {
            memory = alloc_camera_mem(size, 1, false);
            if (NULL == memory) {
                ALOGE("auto_test: %s,%d, failed: alloc camera mem err.\n",
                      __func__, __LINE__);
                goto mem_fail;
            }
            m_isp_firmware_reserved = memory;
        } else {
            memory = m_isp_firmware_reserved;
        }
        if (memory->ion_heap)
            memory->ion_heap->get_kaddr(&kaddr, &ksize);
        *phy_addr++ = kaddr;
        *phy_addr++ = kaddr >> 32;
        *vir_addr++ = (cmr_uint)memory->data;
        *fd++ = memory->fd;
        *fd++ = memory->dev_fd;
    } else {
        ALOGE("auto_test: %s,%s,%d, type ignore: %d, do nothing.\n", __FILE__,
              __func__, __LINE__, type);
    }

    return 0;

mem_fail:
    callback_other_free(type, 0, 0, 0, 0);
    return -1;
}

static cmr_int callback_malloc(enum camera_mem_cb_type type, cmr_u32 *size_ptr,
                               cmr_u32 *sum_ptr, cmr_uint *phy_addr,
                               cmr_uint *vir_addr, cmr_s32 *fd,
                               void *private_data) {
    cmr_int ret = 0;
    cmr_u32 size;
    cmr_u32 sum;

    ALOGI("auto_test: %s,%s,%d type %d IN\n", __FILE__, __func__, __LINE__,
          type);

    /*lock*/
    preview_lock.lock();

    if (!phy_addr || !vir_addr || !size_ptr || !sum_ptr || (0 == *size_ptr) ||
        (0 == *sum_ptr)) {
        ALOGE("auto_test: %s,%d, alloc param error 0x%lx 0x%lx 0x%lx\n",
              __func__, __LINE__, (cmr_uint)phy_addr, (cmr_uint)vir_addr,
              (cmr_uint)size_ptr);
        /*unlock*/
        preview_lock.unlock();
        return -1;
    }

    size = *size_ptr;
    sum = *sum_ptr;

    if (CAMERA_PREVIEW == type) {
        /* preview buffer */
        ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        ret = callback_preview_malloc(size, sum, phy_addr, vir_addr, fd);
    } else if (type == CAMERA_PREVIEW_RESERVED || type == CAMERA_ISP_LSC ||
               type == CAMERA_ISP_FIRMWARE || type == CAMERA_ISP_STATIS ||
               type == CAMERA_ISP_BINGING4AWB || type == CAMERA_ISP_RAWAE ||
               type == CAMERA_ISP_ANTI_FLICKER) {
        ALOGI("auto_test: %s,%s,%d IN\n", __FILE__, __func__, __LINE__);
        ret = callback_other_malloc(type, size, sum, phy_addr, vir_addr, fd);
    } else {
        ALOGE("auto_test: %s,%s,%d, type ignore: %d, do nothing.\n", __FILE__,
              __func__, __LINE__, type);
    }

    /* enable preview flag */
    preview_valid = 1;

    /*unlock*/
    preview_lock.unlock();

    return ret;
}

static void autotest_camera_startpreview(void) {
    cmr_int ret = 0;
    struct img_size preview_size;
    struct cmr_zoom_param zoom_param;

    struct cmr_range_fps_param fps_param;

    if (!oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops)
        return;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    preview_size.width = PREVIEW_WIDTH;
    preview_size.height = PREVIEW_HIGHT;

    zoom_param.mode = 1;
    zoom_param.zoom_level = 1;
    zoom_param.zoom_info.zoom_ratio = 1.00000;
    zoom_param.zoom_info.prev_aspect_ratio =
        (float)PREVIEW_WIDTH / PREVIEW_HIGHT;

    fps_param.is_recording = 0;
    fps_param.min_fps = 5;
    fps_param.max_fps = 30;
    fps_param.video_mode = 0;

    m_hal_oem->ops->camera_fast_ctrl(oem_handle, CAMERA_FAST_MODE_FD, 0);

    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_PREVIEW_SIZE,
             (cmr_uint)&preview_size);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_PREVIEW_FORMAT,
             CAMERA_DATA_FORMAT_YUV420);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_SENSOR_ROTATION, 0);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_ZOOM, (cmr_uint)&zoom_param);
    SET_PARM(m_hal_oem, oem_handle, CAMERA_PARAM_RANGE_FPS,
             (cmr_uint)&fps_param);

    /* set malloc && free callback*/
    ret = m_hal_oem->ops->camera_set_mem_func(
        oem_handle, (void *)callback_malloc, (void *)callback_free, NULL);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("auto_test: %s,%d, failed: camera set mem func error.\n",
              __func__, __LINE__);
        return;
    }

    /*start preview*/
    ret = m_hal_oem->ops->camera_start_preview(oem_handle, CAMERA_NORMAL_MODE);
    if (CMR_CAMERA_SUCCESS != ret) {
        ALOGE("auto_test: %s,%d, failed: camera start preview error.\n",
              __func__, __LINE__);
        return;
    }
}

static int autotest_camera_stoppreview(void) {
    int ret;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    if (!oem_handle || NULL == m_hal_oem || NULL == m_hal_oem->ops) {
        ALOGI("auto_test: oem is null or oem ops is null, do nothing\n");
        return -1;
    }

    ret = m_hal_oem->ops->camera_stop_preview(oem_handle);

    return ret;
}

extern "C" {
int autotest_camera_deinit() {
    cmr_int ret;

    ALOGI("auto_test: %s,%d IN\n", __func__, __LINE__);

    ret = autotest_camera_stoppreview();

    if (oem_handle != NULL && m_hal_oem != NULL && m_hal_oem->ops != NULL) {
        ret = m_hal_oem->ops->camera_deinit(oem_handle);
    }

    if (NULL != m_hal_oem && NULL != m_hal_oem->dso) {
        dlclose(m_hal_oem->dso);
    }
    if (NULL != m_hal_oem) {
        free((void *)m_hal_oem);
        m_hal_oem = NULL;
    }
    free(tmp_buf2);
    free(tmp_buf3);

    return ret;
}

static cmr_int autotest_load_hal_lib(void) {
    int ret = 0;
    if (!m_hal_oem) {
        oem_module_t *omi;

        m_hal_oem = (oem_module_t *)malloc(sizeof(oem_module_t));

        m_hal_oem->dso = dlopen(OEM_LIBRARY_PATH, RTLD_NOW);
        if (NULL == m_hal_oem->dso) {
            char const *err_str = dlerror();
            ALOGE("dlopen error%s", err_str ? err_str : "unknown");
            ret = -1;
            goto loaderror;
        }

        /* Get the address of the struct hal_module_info. */
        const char *sym = OEM_MODULE_INFO_SYM_AS_STR;
        omi = (oem_module_t *)dlsym(m_hal_oem->dso, sym);
        if (omi == NULL) {
            ALOGE("load: couldn't find symbol %s", sym);
            ret = -1;
            goto loaderror;
        }

        m_hal_oem->ops = omi->ops;

        ALOGV("loaded HAL libcamoem m_hal_oem->dso = %p", m_hal_oem->dso);
    }

    return ret;

loaderror:

    if (NULL != m_hal_oem->dso) {
        dlclose(m_hal_oem->dso);
    }
    free((void *)m_hal_oem);
    m_hal_oem = NULL;
    return ret;
}

int autotest_camera_init(int camera_id, minui_backend *p_backend,
                         GRSurface *p_draw) {
    int ret = 0;

    ALOGI("auto_test:%s,%d IN, camera_id %d\n", __func__, __LINE__, camera_id);

    gr_backend = p_backend;
    gr_draw = p_draw;
    if (NULL != gr_draw)
        ALOGI("auto_test:%s,line:%d in skd mode\n", __func__, __LINE__);
    else
        ALOGI("auto_test:%s,line:%d in BBAT mode\n", __func__, __LINE__);

    if (2 == camera_id)
        camera_id = 2; // back camera
    else if (1 == camera_id)
        camera_id = 1; // front camera
    else
        camera_id = 0; // back camera
    if (get_lcd_size(&lcd_w, &lcd_h)) {
        /*update preivew size by lcd*/
        ALOGI("auto_test:%s lcd_w=%d,lcd_h=%d\n", __func__, lcd_w, lcd_h);
    }

    tmp_buf2 = (uint8_t *)malloc(lcd_w * lcd_h * 4);
    tmp_buf3 = (uint8_t *)malloc(lcd_w * lcd_h * 4);

    if (NULL != gr_draw)
        autotest_fb_open();
    if (autotest_load_hal_lib()) {
        return -1;
    }

    ret = m_hal_oem->ops->camera_init(
        camera_id, autotest_camera_cb, &client_data, 0, &oem_handle,
        (void *)callback_malloc, (void *)callback_free);

    if (ret) {
        ALOGE("Native MMI Test: camera_init failed, ret=%d", ret);
        goto exit;
    }
    s_mem_method = autotest_iommu_is_enabled();
    ALOGI("auto_test: %s,%s,%d， s_mem_method %d IN\n", __FILE__, __func__,
          __LINE__, s_mem_method);

    autotest_camera_startpreview();

exit:
    return ret;
}

int autotest_read_cam_buf(void **pp_image_addr, int size, int *p_out_size) {
    ALOGI("auto_test E:");
    cmr_uint vir_addr;

    if (!preview_heap_array[target_buffer_id]) {
        return -1;
    }
    vir_addr = (cmr_uint)preview_heap_array[target_buffer_id]
                   ->ion_heap->getBase(); // virtual address
    *p_out_size = preview_heap_array[target_buffer_id]->phys_size; // image size
    memcpy((void *)*pp_image_addr, (void *)vir_addr, size);

    ALOGI("auto_test X:");
    return 1;
}
}

int flashlightSetValue(int value) {

    int ret = 0;
    char cmd[200] = " ";

    sprintf(cmd, "echo 0x%02x > /sys/class/misc/sprd_flash/test", value);
    ret = system(cmd) ? -1 : 0;
    ALOGD("cmd = %s,ret = %d\n", cmd,ret);

    return ret;
}

int autotest_flash(char *buf, int buf_len, char *rsp, int rsp_size) {

    ALOGD("autotest_flash %x-%x",buf[9], buf[10]);
    int ret = 0;
    if (buf[9] == 0x04) {
        if (buf[10] == 0x01) {
            ALOGD("autotest open flash");
            ret = flashlightSetValue((buf[1] & 0x0f) << 4);   //open flashlight
        } else if (buf[10] == 0x00) {
            ALOGD("autotest close flash");
            ret = flashlightSetValue(0x31);                   //close flashlight
        }
    }

    /*----------------------后续代码为通用代码，所有模块可以直接复制-----------------*/
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1);
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    p_msg_head->len = 8;

    ALOGD("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1;
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0;
    }
    ALOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
          rsp[sizeof(MSG_HEAD_T)]);
    rsp[p_msg_head->len + 2 - 1] = 0x7E; //加上数据尾标志
    ALOGD("dylib test :return len:%d", p_msg_head->len + 2);
    ALOGD("engpc->pc flash:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1], rsp[2],
          rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8], rsp[9]);

    return p_msg_head->len + 2;
    /*----------------------如上虚线之间代码为通用代码，直接赋值即可-----------------*/
}

int autotest_mipicam(char *buf, int buf_len, char *rsp, int rsp_size) {

    int ret = 0;
#if 1
    int rec_image_size = 0;
#define CAM_IDX 1
#define SBUFFER_SIZE (600 * 1024)
    cmr_u32 *p_buf = NULL;
    p_buf = new cmr_u32[600 * 1024];
    static int sensor_id = 0; // 0:back  ,1:front

    switch (buf[9]) {
    case 1:
        autotest_load_hal_lib();
        if (autotest_camera_init(sensor_id, NULL, NULL) < 0) {
            ret = -1;
        }
        break;
    case 2:
        if (autotest_read_cam_buf((void **)&p_buf, SBUFFER_SIZE,
                                  &rec_image_size) < 0) {
            ret = -1;
        } else {
            if (rec_image_size > rsp_size - 1) {
                memcpy(rsp, p_buf, 768);
                ret = 768; // rsp_size-1;
            } else {
                memcpy(rsp, p_buf, 768);
                ret = 768; // rec_image_size;
            }
        }
        break;
    case 3:
        if (autotest_camera_deinit() < 0) {
            ret = -1;
        }
        break;
    case 4:
        sensor_id = buf[10];
        break;
    default:
        break;
    }

    /*------------------------------后续代码为通用代码，所有模块可以直接复制，------------------------------------------*/
    //填充协议头7E xx xx xx xx 08 00 38
    MSG_HEAD_T *p_msg_head;
    memcpy(rsp, buf, 1 + sizeof(MSG_HEAD_T) - 1); //复制7E xx xx xx xx 08 00
    // 38至rsp,，减1是因为返回值中不包含MSG_HEAD_T的subtype（38后面的数据即为subtype）
    p_msg_head = (MSG_HEAD_T *)(rsp + 1);

    //修改返回数据的长度，如上复制的数据长度等于PC发送给engpc的数据长度，现在需要改成engpc返回给pc的数据长度。
    p_msg_head->len = 8; //返回的最基本数据格式：7E xx xx xx xx 08 00 38 xx
    // 7E，去掉头和尾的7E，共8个数据。如果需要返回数据,则直接在38
    // XX后面补充

    //填充成功或失败标志位rsp[1 +
    // sizeof(MSG_HEAD_T)]，如果ret>0,则继续填充返回的数据。
    ALOGD("p_msg_head,ret=%d", ret);

    if (ret < 0) {
        rsp[sizeof(MSG_HEAD_T)] = 1; // 38 01 表示测试失败。
    } else if (ret == 0) {
        rsp[sizeof(MSG_HEAD_T)] = 0; // 38 00 表示测试ok
    } else if (ret > 0) {
        // 38 00 表示测试ok,ret > 0,表示需要返回 ret个字节数据。
        rsp[sizeof(MSG_HEAD_T)] = 0;
        //将获取到的ret个数据，复制到38 00 后面
        memcpy(rsp + 1 + sizeof(MSG_HEAD_T), p_buf, ret);
        //返回长度：基本数据长度8+获取到的ret个字节数据长度。
        p_msg_head->len += ret;
    }
    ALOGD("rsp[1 + sizeof(MSG_HEAD_T):%d]:%d", sizeof(MSG_HEAD_T),
          rsp[sizeof(MSG_HEAD_T)]);
    //填充协议尾部的7E
    rsp[p_msg_head->len + 2 - 1] = 0x7E; //加上数据尾标志
    ALOGD("dylib test :return len:%d", p_msg_head->len + 2);
    ALOGD("engpc->pc:%x %x %x %x %x %x %x %x %x %x", rsp[0], rsp[1], rsp[2],
          rsp[3], rsp[4], rsp[5], rsp[6], rsp[7], rsp[8],
          rsp[9]); // 78 xx xx xx xx _ _38 _ _ 打印返回的10个数据

    return p_msg_head->len + 2;
/*------------------------------如上虚线之间代码为通用代码，直接赋值即可---------*/

#endif
    delete[] p_buf;
    return ret;
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {
    int moudles_num = 0; //用于表示注册的节点的数目。

    reg->type = 0x38;
    reg->subtype = 0x06;
    reg->eng_diag_func = autotest_mipicam;
    moudles_num++;

    (reg+1)->type = 0x38;
    (reg+1)->subtype = 0x0C;
    (reg+1)-> diag_ap_cmd = 0x04;
    (reg+1)->eng_diag_func = autotest_flash;
    moudles_num++;

    *num = moudles_num;
    ALOGD("register_this_module_ext: %d - %d", *num, moudles_num);
}