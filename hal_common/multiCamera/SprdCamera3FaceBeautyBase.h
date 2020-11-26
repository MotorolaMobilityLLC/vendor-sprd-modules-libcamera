
#ifndef SPRDCAMERAFACEBEAUTYBASE_H_HEADER
#define SPRDCAMERAFACEBEAUTYBASE_H_HEADER

#include <stdlib.h>
#include <utils/Log.h>
#include "../SprdCamera3Setting.h"
#ifdef CONFIG_SPRD_FB_VDSP_SUPPORT
#include "sprd_facebeauty_adapter.h"
#else
#include "camera_face_beauty.h"
#endif
namespace sprdcamera {

class SprdCamera3FaceBeautyBase {

#ifdef CONFIG_FACE_BEAUTY
#ifdef CONFIG_SPRD_FB_VDSP_SUPPORT
public:
    SprdCamera3FaceBeautyBase() {
        memset(&face_beauty, 0, sizeof(face_beauty));
        memset(&beautyLevels, 0, sizeof(beautyLevels));
    }
    virtual ~SprdCamera3FaceBeautyBase() {
        // deinit_fb_handle(&face_beauty);
    }
    virtual void doFaceMakeup2(struct camera_frame_type *frame,
                               faceBeautyLevels levels, FACE_Tag *face_info,
                               int work_mode) {
        int ret = 0;
        int facecount = face_info->face_num;
        beautyLevels.blemishLevel = levels.blemishLevel;
        beautyLevels.smoothLevel = levels.smoothLevel;
        beautyLevels.skinColor = levels.skinColor;
        beautyLevels.skinLevel = levels.skinLevel;
        beautyLevels.brightLevel = levels.brightLevel;
        beautyLevels.lipColor = levels.lipColor;
        beautyLevels.lipLevel = levels.lipLevel;
        beautyLevels.slimLevel = levels.slimLevel;
        beautyLevels.largeLevel = levels.largeLevel;
        fbBeautyFacetT beauty_face;
        fb_beauty_image_t beauty_image;
        if (face_info->face_num > 0) {
            for (int i = 0; i < face_info->face_num; i++) {
                beauty_face.idx = i;
                beauty_face.startX = face_info->face[i].rect[0];
                beauty_face.startY = face_info->face[i].rect[1];
                beauty_face.endX = face_info->face[i].rect[2];
                beauty_face.endY = face_info->face[i].rect[3];
                beauty_face.angle = face_info->angle[i];
                beauty_face.pose = face_info->pose[i];
                ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_FACE_CMD,
                    &beauty_face);
            }
        }
        face_beauty_set_devicetype(&face_beauty ,SPRD_CAMALG_RUN_TYPE_CPU);

        fb_chipinfo chipinfo;
#if defined(CONFIG_ISP_2_3)
        chipinfo = SHARKLE;
#elif defined(CONFIG_ISP_2_4)
        chipinfo = PIKE2;
#elif defined(CONFIG_ISP_2_5)
        chipinfo = SHARKL3;
#elif defined(CONFIG_ISP_2_6)
        chipinfo = SHARKL5;
#elif defined(CONFIG_ISP_2_7)
        chipinfo = SHARKL5PRO;
#elif defined(CONFIG_ISP_2_8)
        chipinfo = QOGIRL6;
#endif

        face_beauty_init(&face_beauty, 0, 2, chipinfo);
        beauty_image.inputImage.format = SPRD_CAMALG_IMG_NV21;
        beauty_image.inputImage.addr[0] = (void*)frame->y_vir_addr;
        beauty_image.inputImage.addr[1] = (void*)frame->uv_vir_addr;
        beauty_image.inputImage.addr[2] = (void*)frame->uv_vir_addr;
        beauty_image.inputImage.ion_fd = frame->fd;
        beauty_image.inputImage.offset[0] = 0;
        beauty_image.inputImage.offset[1] = frame->width * frame->height;
        beauty_image.inputImage.width = frame->width;
        beauty_image.inputImage.height = frame->height;
        beauty_image.inputImage.stride = frame->width;
        beauty_image.inputImage.size = frame->width * frame->height *3/2;
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_IMAGE_CMD,
            (void*)&beauty_image);
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_CONSTRUCT_LEVEL_CMD,
            (void*)&beautyLevels);
        ret = face_beauty_ctrl(&face_beauty, FB_BEAUTY_PROCESS_CMD,
            (void*)&facecount);
        face_beauty_deinit(&face_beauty);
    }
    virtual bool isFaceBeautyOn(faceBeautyLevels levels) {
        return (levels.blemishLevel || levels.brightLevel ||
                levels.largeLevel || levels.lipColor || levels.lipLevel ||
                levels.skinColor || levels.skinLevel || levels.slimLevel ||
                levels.smoothLevel);
    }

  private:
    struct fb_beauty_param face_beauty;
    struct faceBeautyLevels beautyLevels;
#else
  public:
    SprdCamera3FaceBeautyBase() {
        memset(&face_beauty, 0, sizeof(face_beauty));
        memset(&beautyLevels, 0, sizeof(beautyLevels));
    }
    virtual ~SprdCamera3FaceBeautyBase() {
        // deinit_fb_handle(&face_beauty);
    }
    virtual void doFaceMakeup2(struct camera_frame_type *frame,
                               face_beauty_levels levels, FACE_Tag *face_info,
                               int work_mode) {
        struct fb_beauty_face_t faceinfo;
	memset(&faceinfo, 0, sizeof(fb_beauty_face_t));

        beautyLevels.blemishLevel = levels.blemishLevel;
        beautyLevels.smoothLevel = levels.smoothLevel;
        beautyLevels.skinColor = levels.skinColor;
        beautyLevels.skinLevel = levels.skinLevel;
        beautyLevels.brightLevel = levels.brightLevel;
        beautyLevels.lipColor = levels.lipColor;
        beautyLevels.lipLevel = levels.lipLevel;
        beautyLevels.slimLevel = levels.slimLevel;
        beautyLevels.largeLevel = levels.largeLevel;
        if (face_info->face_num > 0) {
            for (int i = 0; i < face_info->face_num; i++) {
                faceinfo.startX= face_info->face[i].rect[0];
                faceinfo.startY= face_info->face[i].rect[1];
                faceinfo.endX= face_info->face[i].rect[2];
                faceinfo.endY= face_info->face[i].rect[3];
                faceinfo.angle = face_info->angle[i];
                faceinfo.pose = face_info->pose[i];
                faceinfo.idx = i;
                construct_fb_face(&face_beauty, faceinfo);
            }
        }

        fb_chipinfo chipinfo;
#if defined(CONFIG_ISP_2_3)
        chipinfo = SHARKLE;
#elif defined(CONFIG_ISP_2_4)
        chipinfo = PIKE2;
#elif defined(CONFIG_ISP_2_5)
        chipinfo = SHARKL3;
#elif defined(CONFIG_ISP_2_6)
        chipinfo = SHARKL5;
#elif defined(CONFIG_ISP_2_7)
        chipinfo = SHARKL5PRO;
#elif defined(CONFIG_ISP_2_8)
        chipinfo = QOGIRL6;
#endif
        init_fb_handle(&face_beauty, work_mode, 2, chipinfo);
        construct_fb_image(
            &face_beauty, frame->width, frame->height,
            (unsigned char *)(frame->y_vir_addr),
            (unsigned char *)(frame->y_vir_addr + frame->width * frame->height),
            0);
        construct_fb_level(&face_beauty, beautyLevels);
        do_face_beauty(&face_beauty, face_info->face_num);
        deinit_fb_handle(&face_beauty);
    }
    virtual bool isFaceBeautyOn(face_beauty_levels levels) {
        return (levels.blemishLevel || levels.brightLevel ||
                levels.largeLevel || levels.lipColor || levels.lipLevel ||
                levels.skinColor || levels.skinLevel || levels.slimLevel ||
                levels.smoothLevel);
    }

  private:
    struct class_fb face_beauty;
    struct face_beauty_levels beautyLevels;
#endif
#endif
};

} // namespace sprdcamera
#endif
