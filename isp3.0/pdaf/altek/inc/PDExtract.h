#ifndef __PDData_H__
#define __PDData_H__
#include "alPDAF.h"

struct altek_pos_info {
    unsigned short pd_pos_x;
    unsigned short pd_pos_y;
};
struct altek_pdaf_info {
    unsigned short pd_offset_x;
    unsigned short pd_offset_y;
    unsigned short pd_pitch_x;
    unsigned short pd_pitch_y;
    unsigned short pd_density_x;
    unsigned short pd_density_y;
    unsigned short pd_block_num_x;
    unsigned short pd_block_num_y;
    unsigned short pd_pos_size;
    unsigned short pd_raw_crop_en;
    struct altek_pos_info *pd_pos_r;
    struct altek_pos_info *pd_pos_l;
};
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
extern int alPDExtract_VersionInfo_Get(void *VersionBuffer, int BufferSize);
extern int alPDExtract_GetSize(struct altek_pdaf_info *PDSensorInfo,
                               alPD_RECT *InputROI,
                               unsigned short RawFileWidth,
                               unsigned short RawFileHeight,
                               unsigned short *PDDataWidth,
                               unsigned short *PDDataHeight);

extern int alPDExtract_Run(unsigned char *RawFile,
                           alPD_RECT *InputROI,
                           unsigned short RawFileWidth,
                           unsigned short RawFileHeight,
                           unsigned short *PDOutLeft,
                           unsigned short *PDOutRight);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __PDData_H__ */
