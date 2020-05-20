#ifdef VDSP_CADENCE
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sprd_vdsp_cadence.h"
#include "sprd_camalg_assist_log.h"
#include "sprd_camalg_assist_common.h"
#include "sprd_camalg_assist.h"
#include "vdsp_interface.h"
#include "properties.h"

typedef struct {
	struct vdsp_handle hd;
	int libloaded;
	char nsid[256];
} cadence_vdsp_handle_t;

int cadence_vdsp_open(void **h_vdsp)
{
	double ts = getTime();
	cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)calloc(1, sizeof(cadence_vdsp_handle_t));
	if (NULL == handle) {
		CAA_LOGE("vdsp handle malloc failed\n");
		return 1;
	}

	if (sprd_cavdsp_open_device(SPRD_VDSP_WORK_NORMAL, &handle->hd)) {
		CAA_LOGE("vdsp open failed\n");
		free(handle);
		return 1;
	}

	*h_vdsp = handle;

	CAA_LOGD("cadence_vdsp_open success(%.3f ms)\n", getTime()-ts);

	return 0;
}

int cadence_vdsp_close(void *h_vdsp)
{
	double ts = getTime();
	if (NULL == h_vdsp) {
		CAA_LOGE("invalid handle\n");
		return 1;
	}

	cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)h_vdsp;

	if (handle->libloaded && sprd_cavdsp_unloadlibrary((void *)&handle->hd, handle->nsid)) {
		CAA_LOGE("unload library %s failed\n", handle->nsid);
		return 1;
	}

	if (sprd_cavdsp_close_device(&handle->hd)) {
		CAA_LOGE("vdsp close failed\n");
		return 1;
	}

	free(h_vdsp);

	CAA_LOGD("cadence_vdsp_close success(%.3f ms)\n", getTime()-ts);

	return 0;
}

void *load_library(const char *nsid)
{
	char libname[256];
	sprintf(libname, "/vendor/firmware/%s_cadence.bin", nsid);
	FILE *fp = fopen(libname, "rb");
	if (NULL == fp) {
		CAA_LOGE("%s open failed\n", libname);
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);

	ionmem_handle_t *h_libion = (ionmem_handle_t *)sprd_caa_ionmem_alloc(size, 0);

	if (h_libion) {
		void *buf = h_libion->v_addr;
		fseek(fp, 0, SEEK_SET);
		fread(buf, 1, size, fp);
	}

	fclose(fp);

	return h_libion;
}

JNIEXPORT int sprd_caa_cadence_vdsp_load_library(void *h_vdsp, const char *nsid)
{
	cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)h_vdsp;
	struct sprd_vdsp_client_inout client_buffer;
	ionmem_handle_t *h_libion = NULL;
	int ret = 0;
	if (0 == handle->libloaded) {
		double ts = getTime();
		strcpy(handle->nsid, nsid);

		h_libion = (ionmem_handle_t *)load_library(nsid);
		if (NULL == h_libion) {
			ret = 1;
			goto _ERR_EXIT;
		}

		client_buffer.fd = h_libion->fd;
		client_buffer.viraddr = h_libion->v_addr;
		client_buffer.size = h_libion->size;
		client_buffer.flag = SPRD_VDSP_XRP_READ_WRITE;

		if (sprd_cavdsp_loadlibrary((void *)&handle->hd, nsid, &client_buffer)) {
			CAA_LOGE("load library %s failed\n", nsid);
			ret = 1;
			goto _ERR_EXIT;
		}

		handle->libloaded = 1;

		CAA_LOGD("cadence_vdsp_load_library success(%.3f ms)\n", getTime()-ts);
	}

_ERR_EXIT:
	if (h_libion)
		sprd_caa_ionmem_free(h_libion);
	return ret;
}

void dump_buffer(int buf_id, void *buf, uint32_t size)
{
    char isdump[256];
    property_get("persist.vendor.cam.assist.dump_vdsp_cmd_buf", isdump , "");
    if (strcmp(isdump, "yes"))
        return;
    char fullname[256];
    sprintf(fullname, "/data/vendor/cameraserver/vdsp_cmd_buf%d.bin", buf_id);
    FILE *fp = fopen(fullname, "wb");
    if (!fp) {
        CAA_LOGE("open %s failed\n", fullname);
        return;
    }
    fwrite(buf, 1, size, fp);
    fclose(fp);
}

int cadence_vdsp_send(void *h_vdsp, const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num)
{
	int ret = 0;
	cadence_vdsp_handle_t *handle = (cadence_vdsp_handle_t *)h_vdsp;
	struct sprd_vdsp_client_inout *bufferGroup = NULL;
	struct sprd_vdsp_client_inout *in = NULL, *out = NULL, *buffer = NULL;
	ionmem_handle_t **ionGroup = (ionmem_handle_t **)h_ionmem_list;
	int bufnum = 0;

	if (NULL == h_vdsp) {
		CAA_LOGE("invalid handle\n");
		ret = 1;
		goto _ERR_EXIT;
	}

	if (0 == h_ionmem_num) {
		CAA_LOGE("ionmem_num is 0\n");
		ret = 1;
		goto _ERR_EXIT;
	}

	if (sprd_caa_cadence_vdsp_load_library(h_vdsp, nsid)) {
		ret = 1;
		goto _ERR_EXIT;
	}

	bufferGroup = (struct sprd_vdsp_client_inout *)calloc(h_ionmem_num, sizeof(struct sprd_vdsp_client_inout));
	if (NULL == bufferGroup) {
		CAA_LOGE("bufferGroup malloc failed\n");
		ret = 1;
		goto _ERR_EXIT;
	}

	if (h_ionmem_num > 0) {
		in = &bufferGroup[0];
		in->fd = ionGroup[0]->fd;
		in->viraddr = ionGroup[0]->v_addr;
		in->size = ionGroup[0]->size;
		in->flag = SPRD_VDSP_XRP_READ_WRITE;
		dump_buffer(0, in->viraddr, in->size);
		CAA_LOGD("buffer 0: fd(%d), size(%u)\n", ionGroup[0]->fd, ionGroup[0]->size);
	}

	if (h_ionmem_num > 1) {
		out = &bufferGroup[1];
		out->fd = ionGroup[1]->fd;
		out->viraddr = ionGroup[1]->v_addr;
		out->size = ionGroup[1]->size;
		out->flag = SPRD_VDSP_XRP_READ_WRITE;
		dump_buffer(1, out->viraddr, out->size);
		CAA_LOGD("buffer 1: fd(%d), size(%u)\n", ionGroup[1]->fd, ionGroup[1]->size);
	}

	if (h_ionmem_num > 2) {
		bufnum = h_ionmem_num - 2;
		buffer = &bufferGroup[2];
		for (uint32_t i = 2; i < h_ionmem_num; i++) {
			buffer[i-2].fd = ionGroup[i]->fd;
			buffer[i-2].viraddr = ionGroup[i]->v_addr;
			buffer[i-2].size = ionGroup[i]->size;
			buffer[i-2].flag = SPRD_VDSP_XRP_READ_WRITE;
			dump_buffer(i, buffer[i-2].viraddr, buffer[i-2].size);
			CAA_LOGD("buffer %d: fd(%d), size(%u)\n", i, ionGroup[i]->fd, ionGroup[i]->size);
		}
	}

	ret = sprd_cavdsp_send_cmd(&handle->hd, nsid, in, out, buffer, bufnum, priority);

_ERR_EXIT:
	if (bufferGroup)
		free(bufferGroup);
	return ret;
}

int cadence_vdsp_Send(const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num)
{
	void *h_vdsp = NULL;
	int ret = 0;

	if (cadence_vdsp_open(&h_vdsp)) {
		ret = 1;
		goto _ERR_EXIT;
	}

	if (cadence_vdsp_send(h_vdsp, nsid, priority, h_ionmem_list, h_ionmem_num)) {
		ret = 1;
		goto _ERR_EXIT;
	}

_ERR_EXIT:
	if (cadence_vdsp_close(h_vdsp))
		ret = 1;
	return ret;
}

int cadence_vdsp_maxfreq_lock(void *h_vdsp)
{
	if (NULL == h_vdsp) {
		CAA_LOGE("invalid handle\n");
		return 1;
	}
	return sprd_cavdsp_power_hint(h_vdsp, SPRD_VDSP_POWERHINT_LEVEL_5, SPRD_VDSP_POWERHINT_ACQUIRE);
}

int cadence_vdsp_maxfreq_unlock(void *h_vdsp)
{
	if (NULL == h_vdsp) {
		CAA_LOGE("invalid handle\n");
		return 1;
	}
	return sprd_cavdsp_power_hint(h_vdsp, SPRD_VDSP_POWERHINT_LEVEL_5, SPRD_VDSP_POWERHINT_RELEASE);
}

#endif
