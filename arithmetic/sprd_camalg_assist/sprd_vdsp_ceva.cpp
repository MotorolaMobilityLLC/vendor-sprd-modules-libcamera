#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "sprd_vdsp_ceva.h"
#include <unistd.h>
#include <linux/ioctl.h>
#include "sprd_camalg_assist_log.h"
#include "sprd_camalg_assist_common.h"
#include "sprd_camalg_assist.h"

#define VDSP_IO_MAGIC			'R'
#define SPRD_VDSP_IO_SET_MSG	_IOW(VDSP_IO_MAGIC, 3, struct sprd_dsp_cmd)
#define SPRD_VDSP_IO_FAST_STOP	_IOW(VDSP_IO_MAGIC, 4, struct sprd_dsp_cmd)
#define VDSP_DRIVER_PATH		"/dev/sprd_vdsp"

typedef struct {
	int fd;
} ceva_vdsp_handle_t;

int ceva_vdsp_open(void **h_vdsp)
{
	ceva_vdsp_handle_t *handle = (ceva_vdsp_handle_t *)malloc(sizeof(ceva_vdsp_handle_t));
	if (NULL == handle) {
		CAA_LOGE("vdsp handle malloc failed\n");
		return 1;
	}
	int fd = open(VDSP_DRIVER_PATH, O_RDWR | O_CLOEXEC, 0);
	if (fd < 0) {
		free(handle);
		CAA_LOGE("vdsp open failed: fd(%s)\n", VDSP_DRIVER_PATH);
		return 1;
	}
	handle->fd = fd;
	*h_vdsp = handle;
	return 0;
}

int ceva_vdsp_close(void *h_vdsp)
{
	if (NULL == h_vdsp) {
		CAA_LOGE("invalid vdsp handle\n");
		return 1;
	}
	ceva_vdsp_handle_t *handle = (ceva_vdsp_handle_t *)h_vdsp;
	if (handle->fd < 0) {
		CAA_LOGE("vdsp fd(%d) error\n", handle->fd);
		return 1;
	}
	close(handle->fd);
	free(handle);
	return 0;
}

int ceva_vdsp_send(void *h_vdsp, const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num)
{
	ceva_vdsp_handle_t *handle = (ceva_vdsp_handle_t *)h_vdsp;
	struct sprd_dsp_cmd *cmd = NULL;
	uint32_t buffernum = h_ionmem_num - 2;
	struct sprd_dsp_buffer *dsp_buffer = NULL;
	ionmem_handle_t **buf_list = (ionmem_handle_t **)h_ionmem_list;
	int ret = 0;
	if (NULL == h_vdsp) {
		CAA_LOGE("invalid vdsp handle\n");
		ret = 1;
		goto _ERR_EXIT;
	}

	if (0 == h_ionmem_num) {
		CAA_LOGE("ionmem_num is 0\n");
		ret = 1;
		goto _ERR_EXIT;
	}

	cmd = (struct sprd_dsp_cmd *)calloc(1, sizeof(struct sprd_dsp_cmd));
	if (NULL == cmd) {
		CAA_LOGE("vdsp cmd malloc failed\n");
		ret = 1;
		goto _ERR_EXIT;
	}

	if (strlen(nsid) < SPRD_DSP_CMD_NAMESPACE_ID_SIZE) {
		strcpy(cmd->nsid , nsid);
	} else {
		CAA_LOGE("nsid is too long");
		ret = 1;
		goto _ERR_EXIT;
	}
	cmd->priority = priority;

	if (h_ionmem_num > 0) {
		cmd->in_data_fd = buf_list[0]->fd;
		cmd->in_data_size = buf_list[0]->size;
		CAA_LOGD("buffer 0: fd(%d), size(%u)\n", cmd->in_data_fd, cmd->in_data_size);
	}

	if (h_ionmem_num > 1) {
		cmd->out_data_fd = buf_list[1]->fd;
		cmd->out_data_size = buf_list[1]->size;
		CAA_LOGD("buffer 1: fd(%d), size(%u)\n", cmd->out_data_fd, cmd->out_data_size);
	}

	if (h_ionmem_num > 2) {
		if (buffernum > SPRD_DSP_CMD_INLINE_BUFFER_COUNT) {
			cmd->buffer_addr = (unsigned long)malloc(buffernum * sizeof(struct sprd_dsp_buffer));
			if (cmd->buffer_addr == 0) {
				CAA_LOGE("buffer_addr malloc failed");
				ret = 1;
				goto _ERR_EXIT;
			}
			dsp_buffer = (struct sprd_dsp_buffer *)(unsigned long)cmd->buffer_addr;
		} else {
			dsp_buffer = cmd->buffer_data;
		}
		cmd->buffer_size = buffernum * sizeof(struct sprd_dsp_buffer);
		for (uint32_t i = 0; i < buffernum; i++) {
			dsp_buffer[i].fd = buf_list[i+2]->fd;
			dsp_buffer[i].size = buf_list[i+2]->size;
			CAA_LOGD("buffer %d: fd(%d), size(%u)\n", i+2, dsp_buffer[i].fd, dsp_buffer[i].size);
		}
	}

	if (0 == priority) {
		CAA_LOGI("add SPRD_VDSP_IO_SET_MSG:%x, VDSP_IO_MAGIC:%x, sizeof struct sprd_dsp_cmd:%d",
			SPRD_VDSP_IO_SET_MSG, VDSP_IO_MAGIC, sizeof(struct sprd_dsp_cmd));
		if (ioctl(handle->fd, SPRD_VDSP_IO_SET_MSG, cmd)) {
			CAA_LOGE("fail to send cmd\n");
			ret = 1;
			goto _ERR_EXIT;
		}
	} else {
		CAA_LOGI("add SPRD_VDSP_IO_FAST_STOP:%x", SPRD_VDSP_IO_FAST_STOP);
		if (ioctl(handle->fd, SPRD_VDSP_IO_FAST_STOP, cmd)) {
			CAA_LOGE("fail to send cmd\n");
			ret = 1;
			goto _ERR_EXIT;
		}
	}

_ERR_EXIT:
	if (cmd) {
		if (cmd->buffer_addr)
			free((void *)(unsigned long)cmd->buffer_addr);
		free(cmd);
	}
	return ret;
}

int ceva_vdsp_Send(const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num)
{
	void *h_vdsp = NULL;
	int ret = 0;

	if (ceva_vdsp_open(&h_vdsp)) {
		ret = 1;
		goto _ERR_EXIT;
	}

	if (ceva_vdsp_send(h_vdsp, nsid, priority, h_ionmem_list, h_ionmem_num)) {
		ret = 1;
		goto _ERR_EXIT;
	}

_ERR_EXIT:
	if (ceva_vdsp_close(h_vdsp))
		ret = 1;
	return ret;
}
