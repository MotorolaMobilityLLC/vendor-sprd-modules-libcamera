#ifdef _NR_MAP_PARAM_
static struct sensor_nr_level_map_param s_ov08d10_syp_nr_level_number_map_param = {{
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25
}};

static struct sensor_nr_level_map_param s_ov08d10_syp_default_nr_level_map_param = {{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
}};

static struct sensor_nr_scene_map_param s_ov08d10_syp_nr_scene_map_param = {{
	0x00000049,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
}};
#endif

#ifdef _NR_BAYER_NR_PARAM_
#include "NR/common/normal/bayer_nr_param.h"
#include "NR/common/portrait/bayer_nr_param.h"
#include "NR/common/hdr/bayer_nr_param.h"
#endif

#ifdef _NR_VST_PARAM_
#include "NR/common/normal/vst_param.h"
#include "NR/common/portrait/vst_param.h"
#include "NR/common/hdr/vst_param.h"
#endif

#ifdef _NR_IVST_PARAM_
#include "NR/common/normal/ivst_param.h"
#include "NR/common/portrait/ivst_param.h"
#include "NR/common/hdr/ivst_param.h"
#endif

#ifdef _NR_RGB_DITHER_PARAM_
#include "NR/common/normal/rgb_dither_param.h"
#include "NR/common/portrait/rgb_dither_param.h"
#include "NR/common/hdr/rgb_dither_param.h"
#endif

#ifdef _NR_BPC_PARAM_
#include "NR/common/normal/bpc_param.h"
#include "NR/common/portrait/bpc_param.h"
#include "NR/common/hdr/bpc_param.h"
#endif

#ifdef _NR_GRGB_PARAM_
#include "NR/common/normal/grgb_param.h"
#include "NR/common/portrait/grgb_param.h"
#include "NR/common/hdr/grgb_param.h"
#endif

#ifdef _NR_CFAI_PARAM_
#include "NR/common/normal/cfai_param.h"
#include "NR/common/portrait/cfai_param.h"
#include "NR/common/hdr/cfai_param.h"
#endif

#ifdef _NR_CCE_UVDIV_PARAM_
#include "NR/common/normal/cce_uvdiv_param.h"
#include "NR/common/portrait/cce_uvdiv_param.h"
#include "NR/common/hdr/cce_uvdiv_param.h"
#endif

#ifdef _NR_YNR_PARAM_
#include "NR/common/normal/ynr_param.h"
#include "NR/common/portrait/ynr_param.h"
#include "NR/common/hdr/ynr_param.h"
#endif

#ifdef _NR_EE_PARAM_
#include "NR/common/normal/ee_param.h"
#include "NR/common/portrait/ee_param.h"
#include "NR/common/hdr/ee_param.h"
#endif

#ifdef _NR_3DNR_PARAM_
#include "NR/common/normal/3dnr_param.h"
#include "NR/common/portrait/3dnr_param.h"
#include "NR/common/hdr/3dnr_param.h"
#endif

#ifdef _NR_PPE_PARAM_
#include "NR/common/normal/ppe_param.h"
#include "NR/common/portrait/ppe_param.h"
#include "NR/common/hdr/ppe_param.h"
#endif

#ifdef _NR_YUV_NOISEFILTER_PARAM_
#include "NR/common/normal/yuv_noisefilter_param.h"
#include "NR/common/portrait/yuv_noisefilter_param.h"
#include "NR/common/hdr/yuv_noisefilter_param.h"
#endif

#ifdef _NR_RGB_AFM_PARAM_
#include "NR/common/normal/rgb_afm_param.h"
#include "NR/common/portrait/rgb_afm_param.h"
#include "NR/common/hdr/rgb_afm_param.h"
#endif

#ifdef _NR_IIRCNR_PARAM_
#include "NR/common/normal/iircnr_param.h"
#include "NR/common/portrait/iircnr_param.h"
#include "NR/common/hdr/iircnr_param.h"
#endif

#ifdef _NR_YUV_PRECDN_PARAM_
#include "NR/common/normal/yuv_precdn_param.h"
#include "NR/common/portrait/yuv_precdn_param.h"
#include "NR/common/hdr/yuv_precdn_param.h"
#endif

#ifdef _NR_UV_CDN_PARAM_
#include "NR/common/normal/uv_cdn_param.h"
#include "NR/common/portrait/uv_cdn_param.h"
#include "NR/common/hdr/uv_cdn_param.h"
#endif

#ifdef _NR_UV_POSTCDN_PARAM_
#include "NR/common/normal/uv_postcdn_param.h"
#include "NR/common/portrait/uv_postcdn_param.h"
#include "NR/common/hdr/uv_postcdn_param.h"
#endif

#ifdef _NR_CNR_PARAM_
#include "NR/common/normal/cnr_param.h"
#include "NR/common/portrait/cnr_param.h"
#include "NR/common/hdr/cnr_param.h"
#endif

#ifdef _NR_IMBALANCE_PARAM_
#include "NR/common/normal/imbalance_param.h"
#include "NR/common/portrait/imbalance_param.h"
#include "NR/common/hdr/imbalance_param.h"
#endif

#ifdef _NR_SW3DNR_PARAM_
#include "NR/common/normal/sw3dnr_param.h"
#include "NR/common/portrait/sw3dnr_param.h"
#include "NR/common/hdr/sw3dnr_param.h"
#endif

#ifdef _NR_BWU_BWD_PARAM_
#include "NR/common/normal/bwud_param.h"
#include "NR/common/portrait/bwud_param.h"
#include "NR/common/hdr/bwud_param.h"
#endif

#ifdef _NR_YNRS_PARAM_
#include "NR/common/normal/ynrs_param.h"
#include "NR/common/portrait/ynrs_param.h"
#include "NR/common/hdr/ynrs_param.h"
#endif

#ifdef _NR_CNR3_PARAM_
#include "NR/common/normal/cnr3_param.h"
#include "NR/common/portrait/cnr3_param.h"
#include "NR/common/hdr/cnr3_param.h"
#endif

#ifdef _NR_MFNR_PARAM_
#include "NR/common/normal/mfnr_param.h"
#include "NR/common/portrait/mfnr_param.h"
#include "NR/common/hdr/mfnr_param.h"
#endif

