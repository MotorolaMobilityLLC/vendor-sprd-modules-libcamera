
#ifndef __UNN_ION_H_
#define __UNN_ION_H_


#ifdef __cplusplus
extern "C" {
#endif

void* unn_alloc_ionmem(uint32_t size, uint8_t iscache, int32_t* fd, void** viraddr);
void* unn_alloc_ionmem2(uint32_t size, uint8_t iscache, int32_t* fd, void** viraddr, unsigned long* phyaddr);
int unn_free_ionmem(void* handle);
int unn_flush_ionmem(void* handle, void* vaddr, void* paddr, uint32_t size);
int unn_invalid_ionmem(void* handle);

#ifdef __cplusplus
}
#endif

#endif


