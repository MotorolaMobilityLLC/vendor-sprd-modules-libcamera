#ifndef XTENSA_DEBUGLIB
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "sprd_hdr_api.h"
#include "util.h"
#include "common.h"
#include "assist.h"
#ifdef CEVA_TIME
	#include <ceva-time.h>
#endif

#ifdef CEVA_OPT
	#include "dma_util.h"
	#include "cevaxm.h"
	#define INTERNEL_BLOCK_SIZE 0x3B000
	uchar INTERNAL_BLOCK[INTERNEL_BLOCK_SIZE] PRAGMA_DSECT_NO_LOAD("block");

	#include "sprd_hdr_vdsp_api.h"
	#include "sprd_vdsp_cmd.h"
#elif defined XTENSA_OPT
	#include "commonDef.h"
	#include "dma_util.h"
	#define INTERNEL_BLOCK_SIZE 0x1f480
	uint8_t INTERNAL_BLOCK[INTERNEL_BLOCK_SIZE] ALIGN(64) __attribute__((section(".dram0.data")));
	uint8_t INTERNAL_BLOCK2[INTERNEL_BLOCK_SIZE] ALIGN(64) __attribute__((section(".dram1.data")));
#endif

#ifdef WIN32
	#define CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
	#include <Windows.h>
#endif

#if defined WIN32 || defined __linux__
	#include "pthread.h"
#endif

static int load_normal_exposed_image(const char* filename, uint8_t* imgbuf, int w, int h)
{
    FILE* fp = NULL;
    assert(imgbuf != NULL);

    fp = fopen(filename, "rb");
    printf("fun:%s , filename:%s, fp:%p\n" , __func__ , filename , fp);
    assert(fp != NULL);
    fread(imgbuf, 1, w*h*3/2, fp);
    fclose(fp);

    return 0;
}
static int load_hist(const char* filename, uint32_t* imgbuf, int size)
{
    FILE* fp = NULL;
    assert(imgbuf != NULL);

    fp = fopen(filename, "rb");
    printf("fun:%s , filename:%s, fp:%p\n" , __func__ , filename , fp);
    assert(fp != NULL);
    fread(imgbuf, 1, size*sizeof(uint32_t), fp);
    fclose(fp);

    return 0;
}
static int load_multi_exposed_images(char* filename[MAX_FRAME_NUM], uint8_t* img[MAX_FRAME_NUM], int w, int h)
{
    FILE* fp = NULL;
    int n, imgCnt = MAX_FRAME_NUM;
    int imgsize = w*h*3/2;

    for (n = 0; n < imgCnt; n++) {
        fp = fopen(filename[n], "rb");
        assert(fp != NULL);
	printf("fun:%s , filename:%s, fp:%p\n" , __func__ , filename[n] , fp);
        fread(img[n], 1, sizeof(uint8_t)*imgsize, fp);
        fclose(fp);
    }
    return 0;
}
#ifdef __linux__
typedef struct {
	hdr_config_t *cfg;
	ldr_image_vdsp_t *ldr;
	ldr_image_vdsp_t *dst;
} vdsp_thread_t;

camalg_assist_lib_api_t CAA_API_hdr;

void *vdsp_main(void *arg)
{
	vdsp_thread_t *param = (vdsp_thread_t *)arg;
	hdr_config_t *cfg = param->cfg;
	ldr_image_vdsp_t *ldr = param->ldr;
	ldr_image_vdsp_t *dst = param->dst;
	hdr_inst_t inst;

	int width = ldr[0].image.width;
	int height = ldr[0].image.height;
	printf("fun:%s , width:%d,height:%d\n" , __func__ , width , height);
	sprd_hdr_vdsp_open(&inst, cfg);
	sprd_hdr_vdsp_process(inst, ldr, dst);
    sprd_hdr_vdsp_close(inst);

	FILE *fp = fopen("hdr_output.bin", "wb");
	fwrite(dst->image.data, 1, width*height*3/2, fp);
	printf("fun:%s , width:%d,height:%d, fp:%p\n" , __func__ , width , height , fp);
	fclose(fp);
	printf("func:%s hdr test end\n" , __func__);
//	CAA_API_hdr.IPCThreadState_stopProcess(true);
	set_testend_flag(1);
	return NULL;
}
#endif
static ldr_image_vdsp_t ldr_vdsp[MAX_FRAME_NUM] = {0};
static ldr_image_vdsp_t dst_vdsp = {0};
static hdr_config_t _cfg;
int test_hdr(int argc, char **argv)
{
#ifdef WIN32
	_CrtDumpMemoryLeaks();
#endif

#ifdef CEVA_TIME
	start_clock();
#endif

#ifdef CEVA_OPT
	dma_init(INTERNAL_BLOCK, INTERNEL_BLOCK_SIZE, dma_printf);
#elif defined XTENSA_OPT
	uint8_t *internal_buffer[IDMA_NUM_CHANNELS] = {INTERNAL_BLOCK, INTERNAL_BLOCK2};
	uint32_t internal_buffer_size[IDMA_NUM_CHANNELS] = {INTERNEL_BLOCK_SIZE, INTERNEL_BLOCK_SIZE};
	dma_init(internal_buffer, internal_buffer_size, dma_printf);
#endif

    hdr_inst_t inst;
    hdr_config_t  *cfg = &_cfg;
    hdr_detect_t _scn, *scn = &_scn;
    hdr_stat_t _stat, *stat = &_stat;
    cli_opt_t _opt, *opt = &_opt;

    int w, h, n, img_num;
    float ev[MAX_FRAME_NUM] = {0};
    uint8_t* img_scene = NULL;
	uint32_t* hist = NULL;
    uint8_t* src[MAX_FRAME_NUM] = {NULL};
    uint8_t* dst = NULL;
	ldr_image_t ldr[MAX_FRAME_NUM] = {NULL};
//	ldr_image_vdsp_t ldr_vdsp[MAX_FRAME_NUM] = {0};
//	ldr_image_vdsp_t dst_vdsp = {0};

	//get version
	hdr_version_t version;
	sprd_hdr_version(&version);

    //set all config params as default
    sprd_hdr_config_default(cfg);

    //parse command: obtain demo options and update some config params
    memset(opt, 0, sizeof(cli_opt_t));
    parse(argc - 1, argv+1 , opt, cfg);
    
    w = cfg->img_width;
    h = cfg->img_height;
    cfg->img_stride = w;
	cfg->max_width = w;
	cfg->max_height = h;
    img_num = cfg->img_num;
    assert(img_num <= MAX_FRAME_NUM);

    //allocate memory and load input images.
    {
        if (opt->norm_expo_image) {
			if (!cfg->detect_bypass) {
                stat->w = opt->norm_image_width;
                stat->h = opt->norm_image_height;
                stat->s = opt->norm_image_width;

				img_scene = (uint8_t*)malloc(sizeof(uint8_t)*stat->w*stat->h*3/2);
                load_normal_exposed_image(opt->norm_expo_image, img_scene, stat->w, stat->h);

                stat->img = img_scene;
                stat->hist256 = NULL;
			} else {
				hist = (uint32_t*)malloc(sizeof(uint32_t)*256);
				load_hist(opt->norm_expo_image, hist, 256);
				stat->hist256 = hist;
			}
        }
        for (n = 0; n < img_num; n++) {
            src[n] = (uint8_t*)malloc(sizeof(uint8_t)*w*h*3/2);
        }
        dst = (uint8_t*)malloc(sizeof(uint8_t)*w*h*3/2);
        load_multi_exposed_images(opt->multi_expo_images, src, w, h);
    }
 
    //create/initialize hdr instance based on user config.
    sprd_hdr_open(&inst, cfg);

#if 1
    //hdr scene detection, which will return a pair of ev setting (long and short exposure)
    if (cfg->detect_bypass) {
        //scene detect outside
        scn->thres_bright = cfg->scene_param.thres_bright;
        scn->thres_dark = cfg->scene_param.thres_dark;
        sprd_hdr_scndet(scn, stat, ev);
    } else {
        //scene detect inside
        sprd_hdr_detect(inst, stat, ev);
    }    
#else
	ev[0] = -1;
	ev[1] = 1;
#endif

    //hdr core function, feed two frames (match with the suggest ev setting)
    for (n = 0; n < img_num; n++) {
        ldr[n].data = src[n];
        ldr[n].width = w;
        ldr[n].height = h;
        ldr[n].stride = w;
        ldr[n].ev = ev[n];
	printf("func:%s ldr i:%d width:%d,height:%d\n" , __func__ , n , ldr[n].width , ldr[n].height);
    }

#ifdef __linux__
	sprd_hdr_load_api(&CAA_API_hdr);

	void *h_input0 = CAA_API_hdr.sprd_caa_ionmem_alloc(w*h*3/2, 0);
	void *in_ptr0 = CAA_API_hdr.sprd_caa_ionmem_get_vaddr(h_input0);
	memcpy(in_ptr0, ldr[0].data, w*h*3/2);
	ldr_vdsp[0].image = ldr[0];
	printf("func:%s , ldr_vdsp 0 width:%d, height:%d\n" , __func__ , ldr_vdsp[0].image.width , ldr_vdsp[0].image.height);
	ldr_vdsp[0].image.data = (uint8_t *)in_ptr0;
	ldr_vdsp[0].fd = CAA_API_hdr.sprd_caa_ionmem_get_fd(h_input0);
	
	void *h_input1 = CAA_API_hdr.sprd_caa_ionmem_alloc(w*h*3/2, 0);
	void *in_ptr1 = CAA_API_hdr.sprd_caa_ionmem_get_vaddr(h_input1);
	memcpy(in_ptr1, ldr[1].data, w*h*3/2);
	ldr_vdsp[1].image = ldr[1];
	ldr_vdsp[1].image.data = (uint8_t *)in_ptr1;
	ldr_vdsp[1].fd = CAA_API_hdr.sprd_caa_ionmem_get_fd(h_input1);
	void *h_dst = CAA_API_hdr.sprd_caa_ionmem_alloc(w*h*3/2, 0);
	void *dst_ptr = CAA_API_hdr.sprd_caa_ionmem_get_vaddr(h_dst);
	dst_vdsp.image = ldr[0];
	dst_vdsp.image.data = (uint8_t *)dst_ptr;
	dst_vdsp.fd = CAA_API_hdr.sprd_caa_ionmem_get_fd(h_dst);
	printf("func:%s , ldr_vdsp 0 3 width:%d, height:%d\n" , __func__ , ldr_vdsp[0].image.width , ldr_vdsp[0].image.height);
#endif

#ifdef XTENSA_OPT
	xthal_dcache_all_writeback_inv();
#endif

	sprd_hdr_process(inst, ldr, dst);

    //destroy hdr instance, release all resources.
    sprd_hdr_close(inst);

    save_as_nv21(opt->output_prefix, dst, w, h);
    save_as_bmp(opt->output_prefix, dst, w, h);

    if (opt->norm_expo_image != NULL)
        free(opt->norm_expo_image);

    for (n = 0; n < MAX_FRAME_NUM; n++) {
        if (opt->multi_expo_images[n] != NULL)
            free(opt->multi_expo_images[n]);
	}

	if (opt->output_prefix != NULL)
		free(opt->output_prefix);

    for (n = 0; n < img_num; n++) {
        if (src[n] != NULL) free(src[n]);
    }
    if (dst != NULL) free(dst);
    if (img_scene != NULL) free(img_scene);
	if (hist != NULL) free(hist);

#ifdef __linux__

	pthread_t t;
	vdsp_thread_t arg;
	arg.cfg = cfg;
	arg.ldr = ldr_vdsp;
	arg.dst = &dst_vdsp;

	pthread_create(&t, NULL, vdsp_main, &arg);

//	CAA_API_hdr.ProcessState_startThreadPool();
//	CAA_API_hdr.IPCThreadState_joinThreadPool(true);

//	CAA_API_hdr.sprd_caa_ionmem_free(h_input0);
//	CAA_API_hdr.sprd_caa_ionmem_free(h_input1);
//	CAA_API_hdr.sprd_caa_ionmem_free(h_dst);
#endif

#ifdef WIN32
	_CrtDumpMemoryLeaks();
#endif

#ifdef CEVA_OPT
	dma_deinit();
#endif

    return 0;
}
#endif

