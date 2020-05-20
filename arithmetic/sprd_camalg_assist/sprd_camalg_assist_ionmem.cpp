#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sprd_camalg_assist_log.h"
#include "sprd_camalg_assist_common.h"
#include "sprd_ion.h"
#include "MemIon.h"
#include "sprd_camalg_assist.h"

using namespace android;

JNIEXPORT void *sprd_caa_ionmem_alloc(uint32_t size, bool iscache)
{
	ionmem_handle_t *handle = (ionmem_handle_t *)malloc(sizeof(ionmem_handle_t));
	if (NULL == handle) {
		CAA_LOGE("ionmem handle malloc failed\n");
		return NULL;
	}

	MemIon *pHeapIon = NULL;
	if (iscache) {
		pHeapIon = new MemIon("/dev/ion", size, 0,  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM | ION_FLAG_NO_CLEAR);
	} else {
		pHeapIon =  new MemIon("/dev/ion", size, MemIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM | ION_FLAG_NO_CLEAR);
	}

	if (NULL == pHeapIon) {
		CAA_LOGE("alloc MemIon failed\n");
		free(handle);
		return NULL;
	}

	handle->fd = pHeapIon->getHeapID();
	handle->v_addr = pHeapIon->getBase();
	handle->size = size;
	handle->pHeapIon = pHeapIon;

	CAA_LOGD("ionmem alloc fd(%d), v_addr(%p), size(%u)\n", handle->fd, handle->v_addr, handle->size);

	return handle;
}

JNIEXPORT int sprd_caa_ionmem_free(void *h_ionmem)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return 1;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	MemIon *pHeapIon = (MemIon *)handle->pHeapIon;
	delete pHeapIon;
	free(h_ionmem);
	return 0;
}

JNIEXPORT int sprd_caa_ionmem_flush(void *h_ionmem, uint32_t size)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return 1;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	size_t size_ret = 0;
	unsigned long p_addr = 0;
	MemIon::Get_phy_addr_from_ion(handle->fd, &p_addr, &size_ret);
	return MemIon::Flush_ion_buffer(handle->fd, handle->v_addr, (void *)p_addr, size);
}

JNIEXPORT int sprd_caa_ionmem_invalid(void *h_ionmem)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return 1;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	return MemIon::Invalid_ion_buffer(handle->fd);
}

JNIEXPORT int sprd_caa_ionmem_sync(void *h_ionmem)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return 1;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	return MemIon::Sync_ion_buffer(handle->fd);
}

JNIEXPORT void *sprd_caa_ionmem_get_vaddr(void *h_ionmem)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return NULL;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	return handle->v_addr;
}

JNIEXPORT int sprd_caa_ionmem_get_fd(void *h_ionmem)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return 0;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	return handle->fd;
}

JNIEXPORT uint32_t sprd_caa_ionmem_get_size(void *h_ionmem)
{
	if (h_ionmem == NULL) {
		CAA_LOGE("invalid h_ionmem(%p)\n", h_ionmem);
		return 0;
	}
	ionmem_handle_t *handle = (ionmem_handle_t *)h_ionmem;
	return handle->size;
}