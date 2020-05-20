#include <stdint.h>
#include "sprd_vdsp_ceva.h"
#include "sprd_vdsp_cadence.h"
#include "sprd_camalg_assist_common.h"
#include "sprd_camalg_assist.h"

JNIEXPORT int sprd_caa_vdsp_open(void **h_vdsp)
{
#ifdef VDSP_CEVA
	if (ceva_vdsp_open(h_vdsp))
		return 1;
#elif defined (VDSP_CADENCE)
	if (cadence_vdsp_open(h_vdsp))
		return 1;
#endif
	return 0;
}

JNIEXPORT int sprd_caa_vdsp_close(void *h_vdsp)
{
#ifdef VDSP_CEVA
	if (ceva_vdsp_close(h_vdsp))
		return 1;
#elif defined (VDSP_CADENCE)
	if (cadence_vdsp_close(h_vdsp))
		return 1;
#endif
	return 0;
}

JNIEXPORT int sprd_caa_vdsp_send(void *h_vdsp, const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num)
{
#ifdef VDSP_CEVA
	if (ceva_vdsp_send(h_vdsp, nsid, priority, h_ionmem_list, h_ionmem_num))
		return 1;
#elif defined (VDSP_CADENCE)
	if (cadence_vdsp_send(h_vdsp, nsid, priority, h_ionmem_list, h_ionmem_num))
		return 1;
#endif
	return 0;
}

JNIEXPORT int sprd_caa_vdsp_Send(const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num)
{
#ifdef VDSP_CEVA
	if (ceva_vdsp_Send(nsid, priority, h_ionmem_list, h_ionmem_num))
		return 1;
#elif defined (VDSP_CADENCE)
	if (cadence_vdsp_Send(nsid, priority, h_ionmem_list, h_ionmem_num))
		return 1;
#endif
	return 0;
}

JNIEXPORT int sprd_caa_vdsp_maxfreq_lock(void *h_vdsp)
{
#ifdef VDSP_CADENCE
	return cadence_vdsp_maxfreq_lock(h_vdsp);
#else
    return 0;
#endif
}

JNIEXPORT int sprd_caa_vdsp_maxfreq_unlock(void *h_vdsp)
{
#ifdef VDSP_CADENCE
	return cadence_vdsp_maxfreq_unlock(h_vdsp);
#else
    return 0;
#endif
}