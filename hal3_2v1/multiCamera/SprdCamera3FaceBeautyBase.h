
#ifndef SPRDCAMERAFACEBEAUTYBASE_H_HEADER
#define SPRDCAMERAFACEBEAUTYBASE_H_HEADER

#include <stdlib.h>
#include <utils/Log.h>
#include "../SprdCamera3Setting.h"
#include "camera_face_beauty.h"

namespace sprdcamera {

class SprdCamera3FaceBeautyBase {

#ifdef CONFIG_FACE_BEAUTY
  public:
    SprdCamera3FaceBeautyBase() {
        memset(&face_beauty, 0, sizeof(face_beauty));
    }
    virtual ~SprdCamera3FaceBeautyBase() {
        // deinit_fb_handle(&face_beauty);
    }
    virtual void doFaceMakeup2(struct camera_frame_type *frame,
                               face_beauty_levels levels, FACE_Tag face_info,
                               int work_mode) {
        int sx, sy, ex, ey, angle, pose;

        beautyLevels.blemishLevel = 0;
        beautyLevels.smoothLevel = levels.smoothLevel;
        beautyLevels.skinColor = 0;
        beautyLevels.skinLevel = 0;
        beautyLevels.brightLevel = beautyLevels.smoothLevel;
        beautyLevels.lipColor = 0;
        beautyLevels.lipLevel = 0;
        beautyLevels.slimLevel = 0;
        beautyLevels.largeLevel = 0;
        if (face_info.face_num > 0) {
            /* need to fix with several faces
            for (int i = 0 ; i < face_info.face_num; i++) {
                  sx = face_info.face[i].rect[0];
                  sy = face_info.face[i].rect[1];
                  ex = face_info.face[i].rect[2];
                  ey = face_info.face[i].rect[3];
                  angle = 90;//face_info.angle[i];
                  pose = 0;//face_info.pose[i];
                  construct_fb_face(&face_beauty, i, sx, sy, ex, ey,angle,pose);
             }
             */
            sx = face_info.face[0].rect[0];
            sy = face_info.face[0].rect[1];
            ex = face_info.face[0].rect[2];
            ey = face_info.face[0].rect[3];
            angle = 270; // face_info.angle[i];
            pose = 0;    // face_info.pose[i];
            HAL_LOGV("Blur face beauty face_num: %d, rect: %d %d - %d %d.",
                     face_info.face_num, sx, sy, ex, ey);
            construct_fb_face(&face_beauty, 0 /* i */, sx, sy, ex, ey, angle,
                              pose);
        }
        init_fb_handle(&face_beauty, work_mode, 2);
        construct_fb_image(&face_beauty, frame->width, frame->height,
                           (unsigned char *)(frame->y_vir_addr),
                           (unsigned char *)(frame->y_vir_addr +
                                             frame->width * frame->height), 0);
        construct_fb_level(&face_beauty, beautyLevels);
        do_face_beauty(&face_beauty, face_info.face_num);
        deinit_fb_handle(&face_beauty);
    }

  private:
    struct class_fb face_beauty;
    struct face_beauty_levels beautyLevels;

#endif
};

} // namespace sprdcamera
#endif
