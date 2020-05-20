#ifndef __SPRD_VDSP_CADENCE_H__
#define __SPRD_VDSP_CADENCE_H__

int cadence_vdsp_open(void **h_vdsp);
int cadence_vdsp_close(void *h_vdsp);
int cadence_vdsp_send(void *h_vdsp, const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num);
int cadence_vdsp_Send(const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num);
int cadence_vdsp_maxfreq_lock(void *h_vdsp);
int cadence_vdsp_maxfreq_unlock(void *h_vdsp);

#endif
