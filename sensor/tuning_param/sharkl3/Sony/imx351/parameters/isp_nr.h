#ifdef _NR_MAP_PARAM_
static struct sensor_nr_level_map_param s_imx351_nr_level_number_map_param = {{
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25
}};

static struct sensor_nr_level_map_param s_imx351_default_nr_level_map_param = {{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
}};

static struct sensor_nr_scene_map_param s_imx351_nr_scene_map_param = {{
	0x00000003,0x00000001,0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,
	0x00000000,0x00000000,0x00000001,0x00000001,0x00000000,0x00000000,0x00000000,0x00000000
}};
#endif

#ifdef _NR_BAYER_NR_PARAM_
#include "NR/common/normal/bayer_nr_param.h"
#include "NR/common/night/bayer_nr_param.h"
#include "NR/prv_0/normal/bayer_nr_param.h"
#include "NR/prv_1/normal/bayer_nr_param.h"
#include "NR/cap_1/normal/bayer_nr_param.h"
#include "NR/video_1/normal/bayer_nr_param.h"
#include "NR/video_2/normal/bayer_nr_param.h"
#endif

#ifdef _NR_VST_PARAM_
#include "NR/common/normal/vst_param.h"
#include "NR/common/night/vst_param.h"
#include "NR/prv_0/normal/vst_param.h"
#include "NR/prv_1/normal/vst_param.h"
#include "NR/cap_1/normal/vst_param.h"
#include "NR/video_1/normal/vst_param.h"
#include "NR/video_2/normal/vst_param.h"
#endif

#ifdef _NR_IVST_PARAM_
#include "NR/common/normal/ivst_param.h"
#include "NR/common/night/ivst_param.h"
#include "NR/prv_0/normal/ivst_param.h"
#include "NR/prv_1/normal/ivst_param.h"
#include "NR/cap_1/normal/ivst_param.h"
#include "NR/video_1/normal/ivst_param.h"
#include "NR/video_2/normal/ivst_param.h"
#endif

#ifdef _NR_RGB_DITHER_PARAM_
#include "NR/common/normal/rgb_dither_param.h"
#include "NR/common/night/rgb_dither_param.h"
#include "NR/prv_0/normal/rgb_dither_param.h"
#include "NR/prv_1/normal/rgb_dither_param.h"
#include "NR/cap_1/normal/rgb_dither_param.h"
#include "NR/video_1/normal/rgb_dither_param.h"
#include "NR/video_2/normal/rgb_dither_param.h"
#endif

#ifdef _NR_BPC_PARAM_
#include "NR/common/normal/bpc_param.h"
#include "NR/common/night/bpc_param.h"
#include "NR/prv_0/normal/bpc_param.h"
#include "NR/prv_1/normal/bpc_param.h"
#include "NR/cap_1/normal/bpc_param.h"
#include "NR/video_1/normal/bpc_param.h"
#include "NR/video_2/normal/bpc_param.h"
#endif

#ifdef _NR_GRGB_PARAM_
#include "NR/common/normal/grgb_param.h"
#include "NR/common/night/grgb_param.h"
#include "NR/prv_0/normal/grgb_param.h"
#include "NR/prv_1/normal/grgb_param.h"
#include "NR/cap_1/normal/grgb_param.h"
#include "NR/video_1/normal/grgb_param.h"
#include "NR/video_2/normal/grgb_param.h"
#endif

#ifdef _NR_CFAI_PARAM_
#include "NR/common/normal/cfai_param.h"
#include "NR/common/night/cfai_param.h"
#include "NR/prv_0/normal/cfai_param.h"
#include "NR/prv_1/normal/cfai_param.h"
#include "NR/cap_1/normal/cfai_param.h"
#include "NR/video_1/normal/cfai_param.h"
#include "NR/video_2/normal/cfai_param.h"
#endif

#ifdef _NR_CCE_UVDIV_PARAM_
#include "NR/common/normal/cce_uvdiv_param.h"
#include "NR/common/night/cce_uvdiv_param.h"
#include "NR/prv_0/normal/cce_uvdiv_param.h"
#include "NR/prv_1/normal/cce_uvdiv_param.h"
#include "NR/cap_1/normal/cce_uvdiv_param.h"
#include "NR/video_1/normal/cce_uvdiv_param.h"
#include "NR/video_2/normal/cce_uvdiv_param.h"
#endif

#ifdef _NR_YNR_PARAM_
#include "NR/common/normal/ynr_param.h"
#include "NR/common/night/ynr_param.h"
#include "NR/prv_0/normal/ynr_param.h"
#include "NR/prv_1/normal/ynr_param.h"
#include "NR/cap_1/normal/ynr_param.h"
#include "NR/video_1/normal/ynr_param.h"
#include "NR/video_2/normal/ynr_param.h"
#endif

#ifdef _NR_EE_PARAM_
#include "NR/common/normal/ee_param.h"
#include "NR/common/night/ee_param.h"
#include "NR/prv_0/normal/ee_param.h"
#include "NR/prv_1/normal/ee_param.h"
#include "NR/cap_1/normal/ee_param.h"
#include "NR/video_1/normal/ee_param.h"
#include "NR/video_2/normal/ee_param.h"
#endif

#ifdef _NR_PRE_3DNR_PARAM_
#include "NR/common/normal/pre_3dnr_param.h"
#include "NR/common/night/pre_3dnr_param.h"
#include "NR/prv_0/normal/pre_3dnr_param.h"
#include "NR/prv_1/normal/pre_3dnr_param.h"
#include "NR/cap_1/normal/pre_3dnr_param.h"
#include "NR/video_1/normal/pre_3dnr_param.h"
#include "NR/video_2/normal/pre_3dnr_param.h"
#endif

#ifdef _NR_CAP_3DNR_PARAM_
#include "NR/common/normal/cap_3dnr_param.h"
#include "NR/common/night/cap_3dnr_param.h"
#include "NR/prv_0/normal/cap_3dnr_param.h"
#include "NR/prv_1/normal/cap_3dnr_param.h"
#include "NR/cap_1/normal/cap_3dnr_param.h"
#include "NR/video_1/normal/cap_3dnr_param.h"
#include "NR/video_2/normal/cap_3dnr_param.h"
#endif

#ifdef _NR_YUV_NOISEFILTER_PARAM_
#include "NR/common/normal/yuv_noisefilter_param.h"
#include "NR/common/night/yuv_noisefilter_param.h"
#include "NR/prv_0/normal/yuv_noisefilter_param.h"
#include "NR/prv_1/normal/yuv_noisefilter_param.h"
#include "NR/cap_1/normal/yuv_noisefilter_param.h"
#include "NR/video_1/normal/yuv_noisefilter_param.h"
#include "NR/video_2/normal/yuv_noisefilter_param.h"
#endif

#ifdef _NR_RGB_AFM_PARAM_
#include "NR/common/normal/rgb_afm_param.h"
#include "NR/common/night/rgb_afm_param.h"
#include "NR/prv_0/normal/rgb_afm_param.h"
#include "NR/prv_1/normal/rgb_afm_param.h"
#include "NR/cap_1/normal/rgb_afm_param.h"
#include "NR/video_1/normal/rgb_afm_param.h"
#include "NR/video_2/normal/rgb_afm_param.h"
#endif

#ifdef _NR_IIRCNR_PARAM_
#include "NR/common/normal/iircnr_param.h"
#include "NR/common/night/iircnr_param.h"
#include "NR/prv_0/normal/iircnr_param.h"
#include "NR/prv_1/normal/iircnr_param.h"
#include "NR/cap_1/normal/iircnr_param.h"
#include "NR/video_1/normal/iircnr_param.h"
#include "NR/video_2/normal/iircnr_param.h"
#endif

#ifdef _NR_YUV_PRECDN_PARAM_
#include "NR/common/normal/yuv_precdn_param.h"
#include "NR/common/night/yuv_precdn_param.h"
#include "NR/prv_0/normal/yuv_precdn_param.h"
#include "NR/prv_1/normal/yuv_precdn_param.h"
#include "NR/cap_1/normal/yuv_precdn_param.h"
#include "NR/video_1/normal/yuv_precdn_param.h"
#include "NR/video_2/normal/yuv_precdn_param.h"
#endif

#ifdef _NR_UV_CDN_PARAM_
#include "NR/common/normal/uv_cdn_param.h"
#include "NR/common/night/uv_cdn_param.h"
#include "NR/prv_0/normal/uv_cdn_param.h"
#include "NR/prv_1/normal/uv_cdn_param.h"
#include "NR/cap_1/normal/uv_cdn_param.h"
#include "NR/video_1/normal/uv_cdn_param.h"
#include "NR/video_2/normal/uv_cdn_param.h"
#endif

#ifdef _NR_UV_POSTCDN_PARAM_
#include "NR/common/normal/uv_postcdn_param.h"
#include "NR/common/night/uv_postcdn_param.h"
#include "NR/prv_0/normal/uv_postcdn_param.h"
#include "NR/prv_1/normal/uv_postcdn_param.h"
#include "NR/cap_1/normal/uv_postcdn_param.h"
#include "NR/video_1/normal/uv_postcdn_param.h"
#include "NR/video_2/normal/uv_postcdn_param.h"
#endif

#ifdef _NR_CNR_PARAM_
#include "NR/common/normal/cnr_param.h"
#include "NR/common/night/cnr_param.h"
#include "NR/prv_0/normal/cnr_param.h"
#include "NR/prv_1/normal/cnr_param.h"
#include "NR/cap_1/normal/cnr_param.h"
#include "NR/video_1/normal/cnr_param.h"
#include "NR/video_2/normal/cnr_param.h"
#endif

#ifdef _NR_YNRS_PARAM_
#include "NR/common/normal/ynrs_param.h"
#include "NR/common/night/ynrs_param.h"
#include "NR/prv_0/normal/ynrs_param.h"
#include "NR/prv_1/normal/ynrs_param.h"
#include "NR/cap_1/normal/ynrs_param.h"
#include "NR/video_1/normal/ynrs_param.h"
#include "NR/video_2/normal/ynrs_param.h"
#endif

#ifdef _NR_CNR3_PARAM_
#include "NR/common/normal/cnr3_param.h"
#include "NR/common/night/cnr3_param.h"
#include "NR/prv_0/normal/cnr3_param.h"
#include "NR/prv_1/normal/cnr3_param.h"
#include "NR/cap_1/normal/cnr3_param.h"
#include "NR/video_1/normal/cnr3_param.h"
#include "NR/video_2/normal/cnr3_param.h"
#endif

#ifdef _NR_MFNR_PARAM_
#include "NR/common/normal/mfnr_param.h"
#include "NR/common/night/mfnr_param.h"
#include "NR/prv_0/normal/mfnr_param.h"
#include "NR/prv_1/normal/mfnr_param.h"
#include "NR/cap_1/normal/mfnr_param.h"
#include "NR/video_1/normal/mfnr_param.h"
#include "NR/video_2/normal/mfnr_param.h"
#endif

