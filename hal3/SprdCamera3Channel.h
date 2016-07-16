/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
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

#ifndef __SPRDCAMERA3_CHANNEL_H__
#define __SPRDCAMERA3_CHANNEL_H__

#include <hardware/camera3.h>

#include "SprdCamera3HALHeader.h"
#include "SprdCamera3Setting.h"
#include "SprdCamera3OEMIf.h"
#include "SprdCamera3Stream.h"

using namespace android;

namespace sprdcamera {

typedef void (*channel_cb_routine)(cam_result_data_info_t *result_info, void *userdata);

#define CHANNEL_MAX_STREAM_NUM           4
#define CHANNEL_REGULAR_MAX         3
#define REGULAR_STREAM_TYPE_BASE	CAMERA_STREAM_TYPE_PREVIEW
#define REPROCESS_STREAM_TYPE_BASE	CAMERA_STREAM_TYPE_ZSL_PREVIEW
#define PIC_STREAM_TYPE_BASE		CAMERA_STREAM_TYPE_PICTURE_SNAPSHOT

typedef struct{
	int32_t cap_request_id;
	uint8_t cap_intent;
	uint8_t takepicture_cnt;
	uint8_t scene_mode;
	uint8_t sprd_zsl_enabled;
	uint8_t sprd_refocus_enabled;
} CapRequestPara;

class SprdCamera3OEMIf;

class SprdCamera3Channel
{
public:
	SprdCamera3Channel(SprdCamera3OEMIf *oem_if,
			channel_cb_routine cb_routine,
			SprdCamera3Setting *setting,
			//SprdCamera3Channel *metadata_channel,
			void *userData);
	virtual ~SprdCamera3Channel();
	virtual int start(uint32_t frame_number) = 0;
	virtual int stop(uint32_t frame_number) = 0;
	virtual int channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type) = 0;
	virtual int registerBuffers(const camera3_stream_buffer_set_t *buffer_set) = 0;
	virtual int request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber) = 0;

protected:
	SprdCamera3OEMIf	*mOEMIf;
	channel_cb_routine	mChannelCB;
	//SprdCamera3Channel	*mMetadataChannel;
	SprdCamera3Setting	*mSetting;
	void *mUserData;
};

/* SprdCamera3RegularChannel is used to handle all streams that are directly
 * generated by hardware and given to frameworks without any postprocessing at HAL.
 * Examples are: all IMPLEMENTATION_DEFINED streams, CPU_READ streams. */
class SprdCamera3RegularChannel : public SprdCamera3Channel
{
public:
	SprdCamera3RegularChannel(SprdCamera3OEMIf *oem_if,
			channel_cb_routine cb_routine,
			SprdCamera3Setting *setting,
			SprdCamera3Channel *metadata_channel,
			camera_channel_type_t channel_type,
			void *userData);
	virtual ~SprdCamera3RegularChannel();
	virtual int start(uint32_t frame_number);
	virtual int stop(uint32_t frame_number);
	virtual int channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type);
	virtual int registerBuffers(const camera3_stream_buffer_set_t *buffer_set);
	virtual int request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber);

	int addStream(camera_stream_type_t stream_type, camera3_stream_t * stream);
	int deleteStream();
	int getStream(camera_stream_type_t stream_type, SprdCamera3Stream** stream);
	int channelClearInvalidQBuff(uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type);
	int channelUnmapCurrentQBuff(uint32_t frame_num, int64_t timestamp, camera_stream_type_t stream_type);
	int channelClearAllQBuff(int64_t timestamp, camera_stream_type_t stream_type);
	int setCapturePara(camera_capture_mode_t cap_mode);






/* SprdCamera3PicChannel is for zsl preview stream, which contains a YUV stream generated
 * by the hardware, and encoded to a JPEG stream */


	int setZSLInputBuff(buffer_handle_t* buff);
	int getZSLInputBuff(cmr_uint* addr_vir, cmr_uint* addr_phy, cmr_uint* zsl_private);
	int releaseZSLInputBuff();

	static int kMaxBuffers;

private:
	int32_t	stream_num;
	camera_channel_type_t	mChannelType;

	SprdCamera3Stream*	mCamera3Stream[CHANNEL_MAX_STREAM_NUM];
	SprdCamera3Channel	*mMetadataChannel;

	buffer_handle_t*	mZSLInputBuff;
	SprdCamera3GrallocMemory* mMemory;
	hal_mem_info_t mInputBufInfo;
};

/* SprdCamera3PicChannel is for JPEG stream, which contains a YUV stream generated
 * by the hardware, and encoded to a JPEG stream */
class SprdCamera3PicChannel : public SprdCamera3Channel
{
public:
	SprdCamera3PicChannel(SprdCamera3OEMIf *oem_if,
			channel_cb_routine cb_routine,
			SprdCamera3Setting *setting,
			SprdCamera3Channel *metadata_channel,
			camera_channel_type_t channel_type,
			void *userData);
	~SprdCamera3PicChannel();

	virtual int start(uint32_t frame_number);
	virtual int stop(uint32_t frame_number);
	virtual int channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type);
	virtual int registerBuffers(const camera3_stream_buffer_set_t *buffer_set);
	virtual int32_t request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber);

	int addStream(camera_stream_type_t stream_type, camera3_stream_t * stream);
	int deleteStream();
	int getStream(camera_stream_type_t stream_type, SprdCamera3Stream** stream);
	int setCapturePara(camera_capture_mode_t cap_mode);
	int channelClearAllQBuff(int64_t timestamp, camera_stream_type_t stream_type);

	int32_t buff_index;

	static int kMaxBuffers;

private:
	int32_t	stream_num;
	camera_channel_type_t	mChannelType;

	SprdCamera3Stream*	mCamera3Stream[CHANNEL_MAX_STREAM_NUM];
	SprdCamera3Channel	*mMetadataChannel;
};

/* SprdCamera3MetadataChannel is for metadata stream generated by camera daemon. */
class SprdCamera3MetadataChannel : public SprdCamera3Channel
{
public:
	SprdCamera3MetadataChannel(SprdCamera3OEMIf *oem_if,
			channel_cb_routine cb_routine,
			SprdCamera3Setting *setting,
			//SprdCamera3Channel *metadata_channel,
			void *userData);
	virtual ~SprdCamera3MetadataChannel();

	virtual int start(uint32_t frame_number);
	virtual int stop(uint32_t frame_number);
	virtual int channelCbRoutine(uint32_t frame_number, int64_t timestamp, camera_stream_type_t stream_type);
	virtual int registerBuffers(const camera3_stream_buffer_set_t *buffer_set){return 0;};
	virtual int request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber) {return 0;};

	int getCapRequestPara(const CameraMetadata &metadata, CapRequestPara *request_para);
	//virtual int32_t request(camera3_stream_t *stream, buffer_handle_t *buffer, uint32_t frameNumber);
	int request(const CameraMetadata &metadata);
private:
};

}; // namespace sprdcamera

#endif /* __QCAMERA_CHANNEL_H__ */
