/*
 * ////////////////////////////////////////////////////////////////////
 * //  File name: HW3A_Stats.h
 * //  Create Date:
 * //
 * //  Comment:
 * //  Describe common difinition of HW3A
 * //
 * ////////////////////////////////////////////////////////////////////
 */
#ifndef _AL_COMMON_TYPE_H_
#define _AL_COMMON_TYPE_H_

#define _AL_HW3A_STATS_VER              (0.02)

/* 
 * ////////////////////////////////////////////////////////////////////
 * Include files
 * ////////////////////////////////////////////////////////////////////
 */

#include "mtype.h"
#include "FrmWk_HW3A_event_type.h"
#include <sys/time.h>   /* for timestamp calling */

/* 
 * ////////////////////////////////////////////////////////////////////
 * Macro definitions
 * ////////////////////////////////////////////////////////////////////
 */

#define AL_MAX_ROI_NUM                  (5)
#define AL_MAX_EXP_ENTRIES              (11)        /* max exposure entries */
#define AL_MAX_AWB_STATS_NUM            (3072)      /* 64 x 48 total blocks */
#define AL_MAX_AE_STATS_NUM             (256)       /* 16 x 16 total blocks */
#define AL_MAX_AF_STATS_NUM             (480)       /* 16 x 30 total blocks */

#define AL_AE_HW3A_BIT_DEPTH            (10)        /* bit depth for AE algo of HW3A stats, usually 10 */
#define AL_AWB_HW3A_BIT_DEPTH           (8)         /* bit depth for AWB algo of HW3A stats, usually 8 */

/* for wrapper */
#define HW3A_MAX_DLSQL_NUM              (20)        /* maximun download sequence list number */

#define HW3A_MAGIC_NUMBER_VERSION       (20151210)

#define HW3A_METADATA_SIZE              HW3A_MAX_TOTAL_STATS_BUFFER_SIZE

/* Define for suggested single stats buffer size, including stats info */
/* This including each A stats info after A tag, take AE for exsample, 
 * including  udPixelsPerBlocks/udBankSize/ucValidBlocks
 */
/* ucValidBanks/8-align Dummy */
#define HW3A_MAX_AE_STATS_BUFFER_SIZE           ( HW3A_AE_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_AWB_STATS_BUFFER_SIZE          ( HW3A_AWB_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_AF_STATS_BUFFER_SIZE           ( HW3A_AF_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_ANTIF_STATS_BUFFER_SIZE        ( HW3A_ANTIF_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_YHIST_BUFFER_SIZE              ( HW3A_YHIST_STATS_BUFFER_SIZE + 64 )
#define HW3A_MAX_SUBIMG_BUFFER_SIZE             ( HW3A_SUBIMG_STATS_BUFFER_SIZE + 64 )

#define HW3A_MAX_TOTAL_STATS_BUFFER_SIZE        ( 182*1024 )

/* Define for MAX buffer of single stats */
#define HW3A_AE_STATS_BUFFER_SIZE               ( 31*1024  + 128 )
#define HW3A_AWB_STATS_BUFFER_SIZE              ( 26*1024  + 128 )
#define HW3A_AF_STATS_BUFFER_SIZE               ( 19*1024 + 128 )
#define HW3A_ANTIF_STATS_BUFFER_SIZE            ( 10*1024 + 128 )
#define HW3A_YHIST_STATS_BUFFER_SIZE            ( 1*1024 + 128 )
#define HW3A_SUBIMG_STATS_BUFFER_SIZE           ( 96000 + 128)

/* for wrapper */
#define HW3A_MAX_FRMWK_AE_BLOCKS                AL_MAX_AE_STATS_NUM
#define HW3A_MAX_FRMWK_AF_BLOCKS                (9)

/* 
 * ////////////////////////////////////////////////////////////////////
 * Static declarations
 * ////////////////////////////////////////////////////////////////////
 */


/* 
 * ////////////////////////////////////////////////////////////////////
 * Type declarations
 * ////////////////////////////////////////////////////////////////////
 */


#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
        BAYER_ORDER_RG = 0,               /* RGGB */
        BAYER_ORDER_GR = 1,               /* GRBG */
        BAYER_ORDER_GB = 2,               /* GBRB */
        BAYER_ORDER_BG = 3,               /* BGGR */
} RAW_BAYER_ORDER;

typedef enum {
        AL3A_HW3A_DEV_ID_A_0 = 0,
        AL3A_HW3A_DEV_ID_B_0 = 1,
        AL3A_HW3A_DEV_ID_TOTAL
} AL3A_HW3A_DEV_ID_t;

/*
 * @struct 3A Download Sequence
 * @brief 3A Download Sequence
 */
typedef enum {
        HA3ACTRL_B_DL_TYPE_AF = 1,
        HA3ACTRL_B_DL_TYPE_AE = 2,
        HA3ACTRL_B_DL_TYPE_AWB = 3,
        HA3ACTRL_B_DL_TYPE_YHIS = 4,
        HA3ACTRL_B_DL_TYPE_AntiF = 1,
        HA3ACTRL_B_DL_TYPE_Sub = 2,
        HA3ACTRL_B_DL_TYPE_NONE = 10,
} HW3ACTRL_B_DL_SEQ_t;

/*
 * @struct 3A Download Sequence
 * @brief 3A Download Sequence
 */
typedef enum {
        HA3ACTRL_A_DL_TYPE_AntiF = 1,
        HA3ACTRL_A_DL_TYPE_Sub = 2,
        HA3ACTRL_A_DL_TYPE_W9_ALL = 3,
} HW3ACTRL_A_DL_SEQ_t;

/*
 * @typedef MID_mode
 * @brief mid mode
 */
typedef enum {
        HW3A_MF_51_MODE,
        HW3A_MF_31_MODE,
        HW3A_MF_DISABLE,
} alHW3a_MID_MODE;

typedef enum {
        OPMODE_NORMALLV = 0,
        OPMODE_AF_FLASH_AF,
        OPMODE_FLASH_AE,
        OPMODE_MAX,
} alISP_OPMODE_IDX_t;

/* 
 * ////////////////////////////////////////////////////////////////////
 * Framework related declaration
 * ////////////////////////////////////////////////////////////////////
 */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32 w;
        UINT32 h;
        UINT32 left;
        UINT32 top;
} rect_roi_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        rect_roi_t roi;
        UINT32 weight;      /* unit: 1000, set 1000 to be default */
} rect_roi_wt_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        rect_roi_wt_t roi[AL_MAX_ROI_NUM];
        UINT16  roi_count;          /* total valid ROI region numbers */
} rect_roi_config_t;
#pragma pack(pop)  /* restore old alignment setting from stack */


#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32 output_w;            /* final image out width (scaled) */
        UINT32 output_h;            /* final image out height (scaled) */

        UINT32 input_width;         /* image width before ISP pipeline */
        UINT32 input_height;        /* image height before ISP pipeline */
        UINT32 input_xoffset;       /* image valid start x coordinate */
        UINT32 input_yoffset;       /* image valid start y coordinate */

        UINT32 relay_out_width;     /* relay image width after some ISP pipeline */
        UINT32 relay_out_height;    /* relay image height after some ISP pipeline */
        UINT32 relay_out_xoffset;   /* relay image start x coordinate */
        UINT32 relay_out_yoffset;   /* relay image start y coordinate */
} AL_CROP_INFO_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16  sensor_raw_w;
        UINT16  sensor_raw_h;
        rect_roi_t  sensor_raw_valid_roi;
        color_order_mode_t  raw_color_order;
        float Line_Time;            /* unit: second */
} raw_info;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        raw_info   sensor_raw_info;
        UINT32  HW_H_DIV;
        UINT32  HW_V_DIV;
        BOOL  StripeMode;           /* calculate half size of sensor info of w */
        BOOL  interlaceMode;        /* calculate half size of sensor info of h */
}  ae_HW_config_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* Below for HW3A wrapper used */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        float r;
        float g;
        float b;
} rgb_gain_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32 r;                   /* base unit: 1000 --> 1.0x */
        UINT32 g;                   /* base unit: 1000 --> 1.0x */
        UINT32 b;                   /* base unit: 1000 --> 1.0x */
} calib_wb_gain_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* 
 * ////////////////////////////////////////////////////////////////////
 * HW3A engine related declaration
 * ////////////////////////////////////////////////////////////////////
 */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16 uwBorderRatioX;
        UINT16 uwBorderRatioY;
        UINT16 uwBlkNumX;
        UINT16 uwBlkNumY;
        UINT16 uwOffsetRatioX;
        UINT16 uwOffsetRatioY;
} alHW3a_StatisticsDldRegion_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16 TokenID;
        alHW3a_StatisticsDldRegion_t tAERegion;
} alHW3a_AE_CfgInfo_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        BOOL        bEnable;                    /* Enable HW3A Histogram */
        SINT8       cCrStart;                   /* Histogram Cr Range start */
        SINT8       cCrEnd;                     /* Histogram Cr Range End */
        SINT8       cOffsetUp;                  /* Histogram Offset Range Up */
        SINT8       cOffsetDown;                /* Histogram Offset Range Down */
        SINT8       cCrPurple;                  /* Purple compensation */
        UINT8       ucOffsetPurPle;             /* Purple compensation */
        SINT8       cGrassOffset;               /* Grass compensation--offset */
        SINT8       cGrassStart;                /* Grass compensation--cr left */
        SINT8       cGrassEnd;                  /* Grass compensation--cr right */
        UINT8       ucDampGrass;                /* Grass compensation */
        SINT8       cOffset_bbr_w_start;        /* Weighting around bbr */
        SINT8       cOffset_bbr_w_end;          /* Weighting around bbr */
        UINT8       ucYFac_w;                   /* Weighting around bbr */
        UINT32      dHisInterp;
}   alHW3a_AWB_Histogram_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/*
 * @typedef alHW3a_AF_StatisticsDldRegion
 * @brief AF Statistics download region
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16 uwSizeRatioX;                    /* used to control ROI of width,
                                                 * if set 50 means use half image to get HW3A stats data */
        UINT16 uwSizeRatioY;                    /* used to control ROI of height,
                                                 * if set 50 means use half image to get HW3A stats data */
        UINT16 uwBlkNumX;                       /* block number of horizontal direction of ROI */
        UINT16 uwBlkNumY;                       /* block number of vertical direction of ROI */
        UINT16 uwOffsetRatioX;                  /* used to control ROI shift position ratio of width,
                                                 * if set 1 means offset 1% width from start horizontal position, suggest 0 */
        UINT16 uwOffsetRatioY;                  /* used to control ROI shift position ratio of height,
                                                 * if set 1 means offset 1% width from start horizontal position, suggest 0 */
}   alHW3a_AF_StatisticsDldRegion_t;
#pragma pack(pop)  /* restore old alignment setting from stack */


/*
 * @typedef alHW3a_AF_CfgInfo_t
 * @brief AF configuration information
 */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT16 TokenID;
        alHW3a_AF_StatisticsDldRegion_t tAFRegion;
        BOOL  bEnableAFLUT;
        UINT16 auwLUT[259];
        UINT16 auwAFLUT[259];
        UINT8 aucWeight[6];
        UINT16 uwSH;
        UINT8 ucThMode;
        UINT8 aucIndex[82];
        UINT16 auwTH[4];
        UINT16 pwTV[4];
        UINT32 udAFoffset;
        BOOL bAF_PY_Enable;
        BOOL bAF_LPF_Enable;
        alHW3a_MID_MODE nFilterMode;
        UINT8 ucFilterID;
        UINT16 uwLineCnt;
} alHW3a_AF_CfgInfo_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT8                           TokenID;
        alHW3a_StatisticsDldRegion_t    tAWBRegion;             /* AWB Stat ROI region */
        UINT8                           ucYFactor[16];          /* ALTEK format for Y weighting */
        SINT8                           BBrFactor[33];          /* ALTEK format for CC data */
        UINT16                          uwRGain;                /* ALTEK format of calibration data */
        UINT16                          uwGGain;                /* ALTEK format of calibration data */
        UINT16                          uwBGain;                /* ALTEK format of calibration data */
        UINT8                           ucCrShift;              /* Cr shift for stat data */
        UINT8                           ucOffsetShift;          /* offset shift for stat data */
        UINT8                           ucQuantize;             /* Set ucQuantize = 0 (fixed) 
                                                                 * since crs = Cr- [(cbs+awb_quantize_damp)>>awb_damp] */
        UINT8                           ucDamp;                 /* Set ucDamp = 7 (fixed) 
                                                                 * since crs = Cr- [(cbs+awb_quantize_damp)>>awb_damp] */
        UINT8                           ucSumShift;             /* Sum shift = 5 (fixed), based on ISP sampling points.
                                                                 * [9:0]G' = (sum(G[i,j]/2) + 2^(awb_sum_shift-1)) >> awb_sum_shift */
        alHW3a_AWB_Histogram_t          tHis;
        UINT16                          uwRLinearGain;          /* calib_r_gain with foramt scale 128, normalized by g */
        UINT16                          uwBLinearGain;          /* calib_b_gain with foramt scale 128, normalized by g */
}   alHW3a_AWB_CfgInfo_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        unsigned short                           TokenID;
        unsigned char                            PixPow;
        unsigned char                            PixDiv;
        unsigned short                           PixOff;
        unsigned short                           LineOff;
        unsigned short                           LineSize;

}   alHW3a_Flicker_CfgInfo_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/** HW3A stats info of packed data
\Total size should refer to define of HW3A_MAX_TOTAL_STATS_BUFFER_SIZE
\Here only partial stats info, not including stats data
*/
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;               /* HW3A FW magic number */
        UINT16 uHWengineID;             /* HW3A engine ID, used for 3A behavior */
        UINT16 uFrameIdx;               /* HW3a_frame_idx */
        UINT32 uCheckSum;               /* check sum for overall stats data */

        /* AE */
        UINT16 uAETag;
        UINT16 uAETokenID;              /* valid setting number, which same when calling XXX_setAECfg,
                                         * for 3A libs synchronization */
        UINT32 uAEStatsSize;            /* AE stats size, including AE stat info */
        UINT32 uAEStatsAddr;            /* offset, start from pointer */

        /* AWB */
        UINT16 uAWBTag;
        UINT16 uAWBTokenID;
        UINT32 uAWBStatsSize;           /* AWB stats size, including AWB stat info */
        UINT32 uAWBStatsAddr;           /* offset, start from pointer */

        /* AF */
        UINT16 uAFTag;
        UINT16 uAFTokenID;
        UINT32 uAFStatsSize;            /* AF stats size, including AF stat info */
        UINT32 uAFStatsAddr;            /* offset, start from pointer */

        /* Y-hist */
        UINT16 uYHistTag;
        UINT16 uYHistTokenID;
        UINT32 uYHistStatsSize;         /* Yhist stats size, including Yhist stat info */
        UINT32 uYHistStatsAddr;         /* offset, start from pointer */

        /* anti-flicker */
        UINT16 uAntiFTag;
        UINT16 uAntiFTokenID;
        UINT32 uAntiFStatsSize;         /* anti-flicker stats size, including anti-flicker stat info */
        UINT32 uAntiFStatsAddr;         /* offset, start from pointer */

        /* sub-sample */
        UINT16 uSubsampleTag;
        UINT16 uSubsampleTokenID;
        UINT32 uSubsampleStatsSize;     /* subsample size, including subsample image info */
        UINT32 uSubsampleStatsAddr;     /* offset, start from pointer */

} ISP_DRV_META_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* AE stats info */
        UINT32 udPixelsPerBlocks;
        UINT32 udBankSize;
        UINT8  ucValidBlocks;
        UINT8  ucValidBanks;
} ISP_DRV_META_AE_STATS_INFO_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;               /* HW3a_frame_idx */

        /* AE info */
        UINT8  pAE_Stats[HW3A_AE_STATS_BUFFER_SIZE];
        UINT16 uAETokenID;
        UINT32 uAEStatsSize;
        UINT16 uPseudoFlag;             /* 0: normal stats, 1: PseudoFlag flag 
                                         * (for lib, smoothing/progressive run) */

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

        ISP_DRV_META_AE_STATS_INFO_t ae_stats_info;

} ISP_DRV_META_AE_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* AWB stats info */
        UINT32 udPixelsPerBlocks;
        UINT32 udBankSize;
        UINT16 uwSub_x;
        UINT16 uwSub_y;
        UINT16 uwWin_x;
        UINT16 uwWin_y;
        UINT16 uwWin_w;
        UINT16 uwWin_h;
        UINT16 uwTotalCount;
        UINT16 uwGrassCount;
        UINT8  ucValidBlocks;
        UINT8  ucValidBanks;
} ISP_DRV_META_AWB_STATS_INFO_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;               /* HW3a_frame_idx */

        /* AWB info */
        UINT8 pAWB_Stats[HW3A_AWB_STATS_BUFFER_SIZE];
        UINT16 uAWBTokenID;
        UINT32 uAWBStatsSize;
        UINT16 uPseudoFlag;             /* 0: normal stats, 1: PseudoFlag flag 
                                         * (for lib, smoothing/progressive run) */

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

        ISP_DRV_META_AWB_STATS_INFO_t awb_stats_info;

} ISP_DRV_META_AWB_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* AF stats info */
        UINT32 udPixelsPerBlocks;
        UINT32 udBankSize;
        UINT8  ucValidBlocks;
        UINT8  ucValidBanks;
} ISP_DRV_META_AF_STATS_INFO_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;               /* HW3a_frame_idx */

        /* AF info */
        UINT8  pAF_Stats[HW3A_AF_STATS_BUFFER_SIZE];
        UINT16 uAFTokenID;
        UINT32 uAFStatsSize;

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

        ISP_DRV_META_AF_STATS_INFO_t af_stats_info;

} ISP_DRV_META_AF_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;               /* HW3a_frame_idx */

        /* YHist info */
        UINT8  pYHist_Stats[HW3A_YHIST_STATS_BUFFER_SIZE];
        UINT16 uYHistTokenID;
        UINT32 uYHistStatsSize;

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

} ISP_DRV_META_YHist_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;               /* HW3a_frame_idx */

        /* anti-flicker info */
        UINT8  pAntiF_Stats[HW3A_ANTIF_STATS_BUFFER_SIZE];
        UINT16 uAntiFTokenID;
        UINT32 uAntiFStatsSize;

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

} ISP_DRV_META_AntiF_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Subsample stats info */
        UINT32 udBufferTotalSize;
        UINT16  ucValidW;
        UINT16  ucValidH;
} ISP_DRV_META_Subsample_STATS_INFO_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;               /* HW3a_frame_idx */

        /* Subsample info */
        UINT8 pSubsample_Stats[HW3A_SUBIMG_STATS_BUFFER_SIZE];
        UINT16 uSubsampleTokenID;
        UINT32 uSubsampleStatsSize;

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

        ISP_DRV_META_Subsample_STATS_INFO_t Subsample_stats_info;

} ISP_DRV_META_Subsample_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT8 ucAHBSensoreID;
        UINT8 ucIsSingle3AMode;
        UINT8 ucPreview_Baisc_DldSeqLength;
        UINT8 ucPreview_Adv_DldSeqLength;
        UINT8 ucFastConverge_Baisc_DldSeqLength;
        UINT8 aucPreview_Baisc_DldSeq[HW3A_MAX_DLSQL_NUM];
        UINT8 aucPreview_Adv_DldSeq[HW3A_MAX_DLSQL_NUM];
        UINT8 aucFastConverge_Baisc_DldSeq[HW3A_MAX_DLSQL_NUM];
} alISP_DldSequence_t;
#pragma pack(pop)  /* restore old alignment setting from stack */


/* For AE wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32 r;
        UINT32 g;
        UINT32 b;
} al3AWrapper_AE_wb_gain_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* For AE wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32  uStructureSize;     /* here for confirmation */

        /* Input */
        /* WB info */
        rgb_gain_t stat_WB_2Y;      /* record WB when translate Y from R/G/B */

        /* Output */
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;           /* HW3a_frame_idx */

        /* AE info */
        UINT16 uAETokenID;
        UINT32 uAEStatsSize;

        /* AE stats info */
        UINT32 udPixelsPerBlocks;
        UINT32 udBankSize;
        UINT8  ucValidBlocks;
        UINT8  ucValidBanks;
        UINT8  ucStatsDepth;        /* 8: 8 bits, 10: 10 bits */

        UINT16 uPseudoFlag;         /* 0: normal stats, 1: PseudoFlag flag
                                     *(for lib, smoothing/progressive run) */

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

        /* AE stats */
        BOOL   bIsStatsByAddr;      /* true: use addr to passing stats, flase: use array define */
        UINT32 statsY[AL_MAX_AE_STATS_NUM];
        UINT32 statsR[AL_MAX_AE_STATS_NUM];
        UINT32 statsG[AL_MAX_AE_STATS_NUM];
        UINT32 statsB[AL_MAX_AE_STATS_NUM];
        void* ptStatsY;             /* store stats Y, each element should be UINT32, 256 elements */
        void* ptStatsR;             /* store stats Y, each element should be UINT32, 256 elements */
        void* ptStatsG;             /* store stats Y, each element should be UINT32, 256 elements */
        void* ptStatsB;             /* store stats Y, each element should be UINT32, 256 elements */

} al3AWrapper_Stats_AE_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

/* For Flicker wrapper of HW3A stats */
#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        UINT32  uStructureSize;    /* here for confirmation */

        /* Output */
        /* Common info */
        UINT32 uMagicNum;
        UINT16 uHWengineID;
        UINT16 uFrameIdx;           /* HW3a_frame_idx */

        /* anti-flicker info */
        UINT8  pAntiF_Stats[HW3A_ANTIF_STATS_BUFFER_SIZE];
        UINT16 uAntiFTokenID;
        UINT32 uAntiFStatsSize;

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

} al3AWrapper_Stats_Flicker_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#pragma pack(push) /* push current alignment setting to stack */
#pragma pack(4)    /* new alignment setting */
typedef struct {
        /* Common info */
        UINT32  uMagicNum;
        UINT16  uHWengineID;
        UINT16  uFrameIdx;          /* HW3a_frame_idx */

        /* AWB info */
        UINT8   *pAWB_Stats;
        UINT16  uAWBTokenID;
        UINT32  uAWBStatsSize;
        UINT16  uPseudoFlag;        /* 0: normal stats, 1: PseudoFlag flag
                                     * (for lib, smoothing/progressive run) */

        /* framework time/frame idx info */
        struct timeval systemTime;
        UINT32 udsys_sof_idx;

        /* AWB stats info */
        UINT32  udPixelsPerBlocks;
        UINT32  udBankSize;
        UINT8   ucValidBlocks;
        UINT8   ucValidBanks;
        UINT8   ucStatsDepth;       /* 8: 8 bits, 10: 10 bits */
        UINT8   ucStats_format;     /* 0: ISP format */
} al3AWrapper_Stats_AWB_t;
#pragma pack(pop)  /* restore old alignment setting from stack */

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* _AL_COMMON_TYPE_H_ */
