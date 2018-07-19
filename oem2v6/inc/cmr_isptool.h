#ifndef _CMR_ISPTOOL_H_
#define _CMR_ISPTOOL_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "cmr_common.h"
#include "cmr_snapshot.h"

cmr_int cmr_isp_simulation_proc(cmr_handle oem_handle,
                                struct snapshot_param *param_ptr);
#ifdef __cplusplus
}
#endif

#endif
