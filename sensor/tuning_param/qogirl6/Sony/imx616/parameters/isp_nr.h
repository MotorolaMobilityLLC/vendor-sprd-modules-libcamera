#ifdef _NR_MAP_PARAM_
static struct sensor_nr_level_map_param s_imx616_nr_level_number_map_param = {{
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,
	25,25,25,25,25,25,25,25,25,25,25,25,25,25,25,25
}};

static struct sensor_nr_level_map_param s_imx616_default_nr_level_map_param = {{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
}};

static struct sensor_nr_scene_map_param s_imx616_nr_scene_map_param = {{
	0x00000001,0x00000000,0x00000003,0x00000000,0x00000000,0x00000000,0x00000043,0x00000000,
	0x00000000,0x00000000,0x00000003,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
}};
#endif

#ifdef _NR_BAYER_NR_PARAM_
#include "NR/common/normal/bayer_nr_param.h"
#include "NR/prv_1/normal/bayer_nr_param.h"
#include "NR/prv_1/night/bayer_nr_param.h"
#include "NR/cap_1/normal/bayer_nr_param.h"
#include "NR/cap_1/night/bayer_nr_param.h"
#include "NR/cap_1/hdr/bayer_nr_param.h"
#include "NR/video_1/normal/bayer_nr_param.h"
#include "NR/video_1/night/bayer_nr_param.h"
#endif

#ifdef _NR_VST_PARAM_
#include "NR/common/normal/vst_param.h"
#include "NR/prv_1/normal/vst_param.h"
#include "NR/prv_1/night/vst_param.h"
#include "NR/cap_1/normal/vst_param.h"
#include "NR/cap_1/night/vst_param.h"
#include "NR/cap_1/hdr/vst_param.h"
#include "NR/video_1/normal/vst_param.h"
#include "NR/video_1/night/vst_param.h"
#endif

#ifdef _NR_IVST_PARAM_
#include "NR/common/normal/ivst_param.h"
#include "NR/prv_1/normal/ivst_param.h"
#include "NR/prv_1/night/ivst_param.h"
#include "NR/cap_1/normal/ivst_param.h"
#include "NR/cap_1/night/ivst_param.h"
#include "NR/cap_1/hdr/ivst_param.h"
#include "NR/video_1/normal/ivst_param.h"
#include "NR/video_1/night/ivst_param.h"
#endif

#ifdef _NR_RGB_DITHER_PARAM_
#include "NR/common/normal/rgb_dither_param.h"
#include "NR/prv_1/normal/rgb_dither_param.h"
#include "NR/prv_1/night/rgb_dither_param.h"
#include "NR/cap_1/normal/rgb_dither_param.h"
#include "NR/cap_1/night/rgb_dither_param.h"
#include "NR/cap_1/hdr/rgb_dither_param.h"
#include "NR/video_1/normal/rgb_dither_param.h"
#include "NR/video_1/night/rgb_dither_param.h"
#endif

#ifdef _NR_BPC_PARAM_
#include "NR/common/normal/bpc_param.h"
#include "NR/prv_1/normal/bpc_param.h"
#include "NR/prv_1/night/bpc_param.h"
#include "NR/cap_1/normal/bpc_param.h"
#include "NR/cap_1/night/bpc_param.h"
#include "NR/cap_1/hdr/bpc_param.h"
#include "NR/video_1/normal/bpc_param.h"
#include "NR/video_1/night/bpc_param.h"
#endif

#ifdef _NR_CFAI_PARAM_
#include "NR/common/normal/cfai_param.h"
#include "NR/prv_1/normal/cfai_param.h"
#include "NR/prv_1/night/cfai_param.h"
#include "NR/cap_1/normal/cfai_param.h"
#include "NR/cap_1/night/cfai_param.h"
#include "NR/cap_1/hdr/cfai_param.h"
#include "NR/video_1/normal/cfai_param.h"
#include "NR/video_1/night/cfai_param.h"
#endif

#ifdef _NR_CCE_UVDIV_PARAM_
#include "NR/common/normal/cce_uvdiv_param.h"
#include "NR/prv_1/normal/cce_uvdiv_param.h"
#include "NR/prv_1/night/cce_uvdiv_param.h"
#include "NR/cap_1/normal/cce_uvdiv_param.h"
#include "NR/cap_1/night/cce_uvdiv_param.h"
#include "NR/cap_1/hdr/cce_uvdiv_param.h"
#include "NR/video_1/normal/cce_uvdiv_param.h"
#include "NR/video_1/night/cce_uvdiv_param.h"
#endif

#ifdef _NR_YNR_PARAM_
#include "NR/common/normal/ynr_param.h"
#include "NR/prv_1/normal/ynr_param.h"
#include "NR/prv_1/night/ynr_param.h"
#include "NR/cap_1/normal/ynr_param.h"
#include "NR/cap_1/night/ynr_param.h"
#include "NR/cap_1/hdr/ynr_param.h"
#include "NR/video_1/normal/ynr_param.h"
#include "NR/video_1/night/ynr_param.h"
#endif

#ifdef _NR_EE_PARAM_
#include "NR/common/normal/ee_param.h"
#include "NR/prv_1/normal/ee_param.h"
#include "NR/prv_1/night/ee_param.h"
#include "NR/cap_1/normal/ee_param.h"
#include "NR/cap_1/night/ee_param.h"
#include "NR/cap_1/hdr/ee_param.h"
#include "NR/video_1/normal/ee_param.h"
#include "NR/video_1/night/ee_param.h"
#endif

#ifdef _NR_3DNR_PARAM_
#include "NR/common/normal/3dnr_param.h"
#include "NR/prv_1/normal/3dnr_param.h"
#include "NR/prv_1/night/3dnr_param.h"
#include "NR/cap_1/normal/3dnr_param.h"
#include "NR/cap_1/night/3dnr_param.h"
#include "NR/cap_1/hdr/3dnr_param.h"
#include "NR/video_1/normal/3dnr_param.h"
#include "NR/video_1/night/3dnr_param.h"
#endif

#ifdef _NR_PPE_PARAM_
#include "NR/common/normal/ppe_param.h"
#include "NR/prv_1/normal/ppe_param.h"
#include "NR/prv_1/night/ppe_param.h"
#include "NR/cap_1/normal/ppe_param.h"
#include "NR/cap_1/night/ppe_param.h"
#include "NR/cap_1/hdr/ppe_param.h"
#include "NR/video_1/normal/ppe_param.h"
#include "NR/video_1/night/ppe_param.h"
#endif

#ifdef _NR_YUV_NOISEFILTER_PARAM_
#include "NR/common/normal/yuv_noisefilter_param.h"
#include "NR/prv_1/normal/yuv_noisefilter_param.h"
#include "NR/prv_1/night/yuv_noisefilter_param.h"
#include "NR/cap_1/normal/yuv_noisefilter_param.h"
#include "NR/cap_1/night/yuv_noisefilter_param.h"
#include "NR/cap_1/hdr/yuv_noisefilter_param.h"
#include "NR/video_1/normal/yuv_noisefilter_param.h"
#include "NR/video_1/night/yuv_noisefilter_param.h"
#endif

#ifdef _NR_RGB_AFM_PARAM_
#include "NR/common/normal/rgb_afm_param.h"
#include "NR/prv_1/normal/rgb_afm_param.h"
#include "NR/prv_1/night/rgb_afm_param.h"
#include "NR/cap_1/normal/rgb_afm_param.h"
#include "NR/cap_1/night/rgb_afm_param.h"
#include "NR/cap_1/hdr/rgb_afm_param.h"
#include "NR/video_1/normal/rgb_afm_param.h"
#include "NR/video_1/night/rgb_afm_param.h"
#endif

#ifdef _NR_IIRCNR_PARAM_
#include "NR/common/normal/iircnr_param.h"
#include "NR/prv_1/normal/iircnr_param.h"
#include "NR/prv_1/night/iircnr_param.h"
#include "NR/cap_1/normal/iircnr_param.h"
#include "NR/cap_1/night/iircnr_param.h"
#include "NR/cap_1/hdr/iircnr_param.h"
#include "NR/video_1/normal/iircnr_param.h"
#include "NR/video_1/night/iircnr_param.h"
#endif

#ifdef _NR_YUV_PRECDN_PARAM_
#include "NR/common/normal/yuv_precdn_param.h"
#include "NR/prv_1/normal/yuv_precdn_param.h"
#include "NR/prv_1/night/yuv_precdn_param.h"
#include "NR/cap_1/normal/yuv_precdn_param.h"
#include "NR/cap_1/night/yuv_precdn_param.h"
#include "NR/cap_1/hdr/yuv_precdn_param.h"
#include "NR/video_1/normal/yuv_precdn_param.h"
#include "NR/video_1/night/yuv_precdn_param.h"
#endif

#ifdef _NR_UV_CDN_PARAM_
#include "NR/common/normal/uv_cdn_param.h"
#include "NR/prv_1/normal/uv_cdn_param.h"
#include "NR/prv_1/night/uv_cdn_param.h"
#include "NR/cap_1/normal/uv_cdn_param.h"
#include "NR/cap_1/night/uv_cdn_param.h"
#include "NR/cap_1/hdr/uv_cdn_param.h"
#include "NR/video_1/normal/uv_cdn_param.h"
#include "NR/video_1/night/uv_cdn_param.h"
#endif

#ifdef _NR_UV_POSTCDN_PARAM_
#include "NR/common/normal/uv_postcdn_param.h"
#include "NR/prv_1/normal/uv_postcdn_param.h"
#include "NR/prv_1/night/uv_postcdn_param.h"
#include "NR/cap_1/normal/uv_postcdn_param.h"
#include "NR/cap_1/night/uv_postcdn_param.h"
#include "NR/cap_1/hdr/uv_postcdn_param.h"
#include "NR/video_1/normal/uv_postcdn_param.h"
#include "NR/video_1/night/uv_postcdn_param.h"
#endif

#ifdef _NR_CNR_PARAM_
#include "NR/common/normal/cnr_param.h"
#include "NR/prv_1/normal/cnr_param.h"
#include "NR/prv_1/night/cnr_param.h"
#include "NR/cap_1/normal/cnr_param.h"
#include "NR/cap_1/night/cnr_param.h"
#include "NR/cap_1/hdr/cnr_param.h"
#include "NR/video_1/normal/cnr_param.h"
#include "NR/video_1/night/cnr_param.h"
#endif

#ifdef _NR_IMBALANCE_PARAM_
#include "NR/common/normal/imbalance_param.h"
#include "NR/prv_1/normal/imbalance_param.h"
#include "NR/prv_1/night/imbalance_param.h"
#include "NR/cap_1/normal/imbalance_param.h"
#include "NR/cap_1/night/imbalance_param.h"
#include "NR/cap_1/hdr/imbalance_param.h"
#include "NR/video_1/normal/imbalance_param.h"
#include "NR/video_1/night/imbalance_param.h"
#endif

#ifdef _NR_SW3DNR_PARAM_
#include "NR/common/normal/sw3dnr_param.h"
#include "NR/prv_1/normal/sw3dnr_param.h"
#include "NR/prv_1/night/sw3dnr_param.h"
#include "NR/cap_1/normal/sw3dnr_param.h"
#include "NR/cap_1/night/sw3dnr_param.h"
#include "NR/cap_1/hdr/sw3dnr_param.h"
#include "NR/video_1/normal/sw3dnr_param.h"
#include "NR/video_1/night/sw3dnr_param.h"
#endif

#ifdef _NR_BWU_BWD_PARAM_
#include "NR/common/normal/bwud_param.h"
#include "NR/prv_1/normal/bwud_param.h"
#include "NR/prv_1/night/bwud_param.h"
#include "NR/cap_1/normal/bwud_param.h"
#include "NR/cap_1/night/bwud_param.h"
#include "NR/cap_1/hdr/bwud_param.h"
#include "NR/video_1/normal/bwud_param.h"
#include "NR/video_1/night/bwud_param.h"
#endif

#ifdef _NR_YNRS_PARAM_
#include "NR/common/normal/ynrs_param.h"
#include "NR/prv_1/normal/ynrs_param.h"
#include "NR/prv_1/night/ynrs_param.h"
#include "NR/cap_1/normal/ynrs_param.h"
#include "NR/cap_1/night/ynrs_param.h"
#include "NR/cap_1/hdr/ynrs_param.h"
#include "NR/video_1/normal/ynrs_param.h"
#include "NR/video_1/night/ynrs_param.h"
#endif

#ifdef _NR_CNR3_PARAM_
#include "NR/common/normal/cnr3_param.h"
#include "NR/prv_1/normal/cnr3_param.h"
#include "NR/prv_1/night/cnr3_param.h"
#include "NR/cap_1/normal/cnr3_param.h"
#include "NR/cap_1/night/cnr3_param.h"
#include "NR/cap_1/hdr/cnr3_param.h"
#include "NR/video_1/normal/cnr3_param.h"
#include "NR/video_1/night/cnr3_param.h"
#endif

#ifdef _NR_MFNR_PARAM_
#include "NR/common/normal/mfnr_param.h"
#include "NR/prv_1/normal/mfnr_param.h"
#include "NR/prv_1/night/mfnr_param.h"
#include "NR/cap_1/normal/mfnr_param.h"
#include "NR/cap_1/night/mfnr_param.h"
#include "NR/cap_1/hdr/mfnr_param.h"
#include "NR/video_1/normal/mfnr_param.h"
#include "NR/video_1/night/mfnr_param.h"
#endif

#ifdef _NR_POST_EE_PARAM_
#include "NR/common/normal/post_ee_param.h"
#include "NR/prv_1/normal/post_ee_param.h"
#include "NR/prv_1/night/post_ee_param.h"
#include "NR/cap_1/normal/post_ee_param.h"
#include "NR/cap_1/night/post_ee_param.h"
#include "NR/cap_1/hdr/post_ee_param.h"
#include "NR/video_1/normal/post_ee_param.h"
#include "NR/video_1/night/post_ee_param.h"
#endif
