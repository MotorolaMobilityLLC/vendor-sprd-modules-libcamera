#ifndef _SPRD_HDR_API_H_
#define _SPRD_HDR_API_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern   "C"
{
#endif

/*! version information */
typedef struct _tag_hdr_version_t
{
    uint8_t    major;              /*!< API major version */
    uint8_t    minor;              /*!< API minor version */
    uint8_t    micro;              /*!< API micro version */
    uint8_t    nano;               /*!< API nano version */
    char    built_date[0x20];    /*!< API built date */
    char    built_time[0x20];    /*!< API built time */
    char    built_rev[0x100];    /*!< API built version, linked with vcs resivion> */
} hdr_version_t;

/*! hdr statistic parameters */
typedef struct _tag_hdr_stat_t
{
    uint32_t*   hist256;            /*!< histogram buffer pointer */
    uint8_t*    img;                /*!< gray image buffer pointer */
    int         w;                  /*!< image width */
    int         h;                  /*!< image height */
    int         s;                  /*!< image buffer stride */
} hdr_stat_t;

/*! hdr scndet out */
typedef struct _tag_hdr_callback_t
{
    int         tuning_param_index;
    uint64_t    clock;
} hdr_callback_t;

/*! hdr_detect parameters */
typedef struct _tag_hdr_detect_t
{
    uint8_t     thres_dark;         //[0, 40]
    uint8_t     thres_bright;       //[180, 255]
    int         bv;
    uint8_t     face_num;
    uint16_t    abl_weight;
    uint32_t    iso;
    void*       tuning_param;
    int         tuning_param_size;
    hdr_callback_t     callback;
} hdr_detect_t;

/*! LDR image data structure */
typedef struct _tag_ldr_image_t
{
    uint8_t*    data;
    int         width;
    int         height;
    int         stride;
    float       ev;
    void*       callback;
} ldr_image_t;

typedef struct
{
    ldr_image_t image;
    int    fd;
} ldr_image_vdsp_t;

/*! mf_hdr align parameters */
typedef struct _tag_hdr_align_t
{
    uint8_t     align_layer;        //[6, 8]
    uint8_t     align_noiseT;       //[4,20]
    float       align_thres[2];     //[0.3, 0.7]
} hdr_align_t;

/*! mf_hdr fusion parameters */
typedef struct
{
    uint8_t     deghost_en;         //[0,  1]
    uint8_t     layer_minus;        //[0,  4]
    uint8_t     thres_oe;           //[0,255]
    uint8_t     thres_mv;           //[0,255]
    uint8_t     radius_e;           //[1,  5]
    uint16_t    dist_thres;         //[50,255]
    uint16_t    mv_num_thres;       //[0, 300]
    uint8_t     mv_border;          //[0, 50]
    uint8_t     mv_dilate_thres;    //[0,255]
    uint8_t     thres_oe2;          //[0, 255]
    uint8_t     w_yoe_m;            //[0,255]
    float       w_yoe_s;            //[0.0, 1.0]
    uint8_t     w_uv_m;             //[0,255]
    float       w_uv_s;             //[0.0, 1.0]
    float       sat_strength;       //[1.0, 5.0]
} hdr_fusion_t;

/*! mf_hdr ltm parameters */
typedef struct _tag_hdr_ltm_t
{
    uint8_t    contrast_en;        //[0,  1]
    uint8_t    strength;           //[1,  7]
    uint8_t    texture_counter_en; //[0,  1]
    float      text_point_alpha;   //[0,  1]
    uint8_t    text_point_thres;   //[1, 31]
    uint8_t    text_prop_thres;    //[1, 31]
} hdr_ltm_t;

typedef struct _tag_hdr_config_t
{
    int    max_width;
    int    max_height;
    int    img_width;
    int    img_height;
    int    img_stride;
    int    img_num;
    char*  core_str;
    uint8_t    detect_bypass;  //[0,  1]
    uint8_t    align_bypass;   //[0, 1]
    hdr_detect_t    scene_param;
    hdr_align_t    align_param;
    hdr_fusion_t    fusion_param;
    hdr_ltm_t    hdr_ltm_param;
    void*    tuning_param;
    int      tuning_param_size;
    void*    (*malloc)(size_t size, char* type);
    void     (*free)(void* addr);
} hdr_config_t;

typedef struct {
    void *dl_handle;
    int (*sprd_caa_vdsp_open)(void **h_vdsp);
    int (*sprd_caa_vdsp_close)(void *h_vdsp);
    int (*sprd_caa_vdsp_send)(void *h_vdsp, const char *nsid, int priority, void **h_ionmem_list, uint32_t h_ionmem_num);
    int (*sprd_caa_cadence_vdsp_load_library)(void *h_vdsp, const char *nsid);
    int (*sprd_caa_vdsp_Send)(const char *nsid, int priority, void **h_ionmem_list, uint32_t h_ionmem_num);
    int (*sprd_caa_vdsp_maxfreq_lock)(void *h_vdsp);
    int (*sprd_caa_vdsp_maxfreq_unlock)(void *h_vdsp);
    void *(*sprd_caa_ionmem_alloc)(uint32_t size, bool iscache);
    int (*sprd_caa_ionmem_free)(void *h_ionmem);
    void *(*sprd_caa_ionmem_get_vaddr)(void *h_ionmem);
    int (*sprd_caa_ionmem_get_fd)(void *h_ionmem);
    void (*ProcessState_initWithDriver)(const char *driver);
    void (*ProcessState_startThreadPool)();
    void (*IPCThreadState_joinThreadPool)(bool isMain);
    void (*IPCThreadState_stopProcess)(bool immediate);
} camalg_assist_lib_api_t;

typedef void * hdr_inst_t;

#ifdef _USRDLL
    #ifdef SPRD_ISP_EXPORTS
        #define SPRD_ISP_API    __declspec(dllexport)
    #else
        #define SPRD_ISP_API    __declspec(dllimport)
    #endif
#else
    #define SPRD_ISP_API
#endif

SPRD_ISP_API int sprd_hdr_scndet_multi_inst(hdr_detect_t* scndet, hdr_stat_t* stat, float* ev, int *p_smooth_flag, int *p_frameID);
SPRD_ISP_API int sprd_hdr_scndet(hdr_detect_t* param, hdr_stat_t* stat, float ev[2]);
SPRD_ISP_API int sprd_hdr_version(hdr_version_t* version);
SPRD_ISP_API int sprd_hdr_config_default(hdr_config_t* cfg);
SPRD_ISP_API int sprd_hdr_open(hdr_inst_t* inst, hdr_config_t* cfg);
SPRD_ISP_API int sprd_hdr_detect(hdr_inst_t inst, hdr_stat_t* stat, float ev[2]);
SPRD_ISP_API int sprd_hdr_process(hdr_inst_t inst, ldr_image_t* input, uint8_t* output);
SPRD_ISP_API int sprd_hdr_close(hdr_inst_t inst);
SPRD_ISP_API int sprd_hdr_fast_stop(hdr_inst_t inst);
SPRD_ISP_API void sprd_set_buffer_base(hdr_inst_t inst, uint8_t *base);

// vdsp
#if defined (__linux__) || defined (WIN32)
SPRD_ISP_API int sprd_hdr_vdsp_open(hdr_inst_t* inst, hdr_config_t* cfg);
SPRD_ISP_API int sprd_hdr_vdsp_process(hdr_inst_t inst, ldr_image_vdsp_t* vdsp_input, ldr_image_vdsp_t* vdsp_output);
SPRD_ISP_API int sprd_hdr_vdsp_close(hdr_inst_t inst);
SPRD_ISP_API int sprd_hdr_load_api(camalg_assist_lib_api_t *libapi);
#endif

#ifdef __cplusplus
}
#endif

#endif