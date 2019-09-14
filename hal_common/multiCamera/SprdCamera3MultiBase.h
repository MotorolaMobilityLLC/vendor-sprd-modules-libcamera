/* Copyright (c) 2017, The Linux Foundataion. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef SPRDCAMERAMULTIBASE_H_HEADER
#define SPRDCAMERAMULTIBASE_H_HEADER

#include <stdlib.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <utils/List.h>
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <hardware/camera.h>
#include <system/camera.h>
#include <sys/mman.h>
#include <sprd_ion.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBuffer.h>
#include "SprdCamera3HWI.h"
//#include "ts_makeup_api.h"
#include "SprdMultiCam3Common.h"
#include "hal_yuvprocess.h"
#include "core_yuvscaler.h"
namespace sprdcamera {

typedef enum {
    PREVIEW_REQUEST_STATE = 0,
    WAIT_FIRST_YUV_STATE,  // wait first yuv frame
    WAIT_SECOND_YUV_STATE, // wait second yuv frame
    WAIT_THIRD_YUV_STATE,  // wait third yuv frame
    REPROCESS_STATE,       // encode yuv to jpeg
    WAIT_JPEG_STATE,
} request_state;

typedef struct {
    int width;
    int height;
} custom_res;

typedef enum {
    RES_START = 0,
    RES_0_3M,
    RES_2M,
    RES_1080P,
    RES_5M,
    RES_8M,
    RES_12M,
    RES_13M,
    RES_MULTI,
    RES_END
} custom_size;

#define CUSTOM_RES_NUM 6
typedef struct custom_stream_info {
    custom_size size;
    custom_res res[CUSTOM_RES_NUM];
} custom_stream_info_t;

class SprdCamera3MultiBase {
  public:
    request_state mReqState;
    int mIommuEnabled;
    SprdCamera3MultiBase();
    virtual ~SprdCamera3MultiBase();
    virtual int allocateOne(int w, int h, new_mem_t *new_mem, int type);
    int initialize(multiCameraMode mode, SprdCamera3HWI *hwi);
    virtual void freeOneBuffer(new_mem_t *buffer);
    virtual int validateCaptureRequest(camera3_capture_request_t *request);
    virtual void convertToRegions(int32_t *rect, int32_t *region, int weight);
    virtual uint8_t getCoveredValue(CameraMetadata &frame_settings,
                                    SprdCamera3HWI *hwiSub,
                                    SprdCamera3HWI *hwiMin,
                                    int convered_camera_id);
    virtual buffer_handle_t *popRequestList(List<buffer_handle_t *> &list);
    virtual buffer_handle_t *popBufferList(List<new_mem_t *> &list,
                                           camera_buffer_type_t type);
    virtual buffer_handle_t *popBufferList(List<new_mem_t *> &list, int width,
                                           int height);
    virtual void pushBufferList(new_mem_t *localbuffer,
                                buffer_handle_t *backbuf, int localbuffer_num,
                                List<new_mem_t *> &list);
    virtual int getStreamType(camera3_stream_t *new_stream);
    virtual void dumpFps();
    virtual void dumpData(unsigned char *addr, int type, int size, int param1,
                          int param2, int param3, const char param4[20]);
    virtual bool matchTwoFrame(hwi_frame_buffer_info_t result1,
                               List<hwi_frame_buffer_info_t> &list,
                               hwi_frame_buffer_info_t *result2);
    virtual bool matchThreeFrame(hwi_frame_buffer_info_t result1,
                               List<hwi_frame_buffer_info_t> &list2,List<hwi_frame_buffer_info_t> &list3,
                               hwi_frame_buffer_info_t *result2, hwi_frame_buffer_info_t *result3);
    virtual hwi_frame_buffer_info_t *
    pushToUnmatchedQueue(hwi_frame_buffer_info_t new_buffer_info,
                         List<hwi_frame_buffer_info_t> &queue);
    virtual hwi_frame_buffer_info_t *
    pushToQueue(hwi_frame_buffer_info_t new_buffer_info,
                List<hwi_frame_buffer_info_t> *queue);
    virtual bool alignTransform(void *src, int w_old, int h_old, int w_new,
                                int h_new, void *dest);
    virtual int convertToImg_frm(void *phy_addr, void *vir_addr, int width,
                                 int height, int fd, cmr_u32 format,
                                 img_frm *out);
    virtual int convertToImg_frm(buffer_handle_t *in, img_frm *out,
                                 cmr_u32 format, void *vir_addr);
    virtual int findGraphicBuf(List<new_mem_t *> &list,
                               native_handle_t *native_handle,
                               sp<GraphicBuffer> &pbuffer);
    virtual int findWidthHeigth(List<new_mem_t *> &list,
                                native_handle_t *native_handle, int *width,
                                int *height);
    virtual int map(buffer_handle_t *buffer, void **vir_addr);
    virtual int unmap(buffer_handle_t *buffer);
    /*
#ifdef CONFIG_FACE_BEAUTY
virtual void doFaceMakeup(struct camera_frame_type *frame,
                          int perfect_level, int *face_info);

virtual void convert_face_info(int *ptr_cam_face_inf, int width,
                               int height);
#endif
*/
    bool ScaleNV21(uint8_t *a_ucDstBuf, uint16_t a_uwDstWidth,
                   uint16_t a_uwDstHeight, uint8_t *a_ucSrcBuf,
                   uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                   uint32_t a_udFileSize);
    bool DepthRangLinear(uint8_t *a_ucDstBuf, uint16_t *a_ucSrcBuf,
                         uint16_t a_uwDstWidth, uint16_t a_uwDstHeight,
                         int *far, int *near);
    int hwScale(uint8_t *dst_buf, uint16_t dst_width, uint16_t dst_height,
                uint16_t dst_fd, uint8_t *src_buf, uint16_t src_width,
                uint16_t src_height, uint16_t src_fd);
    int swScale(uint8_t *dst_buf, uint16_t dst_width, uint16_t dst_height,
                uint16_t dst_fd, uint8_t *src_buf, uint16_t src_width,
                uint16_t src_height, uint16_t src_fd);
    int Yuv420Scale(uint8_t *dst_buf, uint16_t dst_width, uint16_t dst_height,
                 uint8_t *src_buf, uint16_t src_width,
                uint16_t src_height);
    int NV21Rotate(int8_t *dst_buf, uint16_t dst_fd, int8_t *src_buf,
                   uint16_t src_fd, uint16_t width, uint16_t height,
                   uint8_t angle);
    bool DepthRotateCCW90(uint16_t *a_uwDstBuf, uint16_t *a_uwSrcBuf,
                          uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                          uint32_t a_udFileSize);
    bool DepthRotateCCW180(uint16_t *a_uwDstBuf, uint16_t *a_uwSrcBuf,
                           uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                           uint32_t a_udFileSize);
    bool NV21Rotate90(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                      uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                      uint32_t a_udFileSize);
    bool NV21Rotate180(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                       uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                       uint32_t a_udFileSize);
    bool Raw8Rotate(uint8_t *a_ucDstBuf, uint8_t *a_ucSrcBuf,
                    uint16_t a_uwSrcWidth, uint16_t a_uwSrcHeight,
                    uint8_t angle);
    int flushIonBuffer(int buffer_fd, void *v_addr, size_t size);
    uint32_t getJpegSize(uint8_t *jpegBuffer, uint32_t maxSize);
    void setJpegSize(char *jpeg_base, uint32_t max_jpeg_size,
                     uint32_t jpeg_size);
    int jpeg_encode_exif_simplify(img_frm *src_img, img_frm *pic_enc_img,
                                  struct img_frm *dst_img, SprdCamera3HWI *hwi);

    int jpeg_encode_exif_simplify(buffer_handle_t *src_private_handle,
                                  void *src_vir_addr,
                                  buffer_handle_t *pic_enc_private_handle,
                                  void *pic_vir_addr,
                                  buffer_handle_t *dst_private_handle,
                                  void *dst_vir_addr, SprdCamera3HWI *hwi);
    int jpeg_encode_exif_simplify_format(
        buffer_handle_t *src_private_handle, void *src_vir_addr,
        buffer_handle_t *pic_enc_private_handle, void *pic_vir_addr,
        buffer_handle_t *dst_private_handle, void *dst_vir_addr,
        SprdCamera3HWI *hwi, uint8_t fmt, cmr_uint rotation, cmr_uint flip_on);
    static void addAvailableStreamSize(CameraMetadata &metadata,
                                const char *resolution);
    void setLogicIdTag(CameraMetadata &metadata, uint8_t *physical_ids,
                       uint8_t physical_ids_size);
    static int get_support_res_size(const char *resolution);

  private:
    Mutex mBufferListLock;
    int mVFrameCount;
    int mVLastFrameCount;
    nsecs_t mVLastFpsTime;
    // for convered feature
    List<int> mLumaList;
    multiCameraMode mCameraMode;
    int mLowLumaConut;
    int mconut;
    uint32_t mCurScene;
    uint8_t mBrightConut;
    uint8_t mLowConut;
    uint8_t mDarkConut;
    int64_t mMatchTimeThreshold;
    SprdCamera3HWI *mHwi;
};
}
#endif
