#ifndef _SYS_TEST_CASE_COMMON_H_
#define _SYS_TEST_CASE_COMMON_H_

typedef struct {
        void *dl_handle;
        int (*sprd_caa_vdsp_open)(void **h_vdsp);
        int (*sprd_caa_vdsp_close)(void *h_vdsp);
        int (*sprd_caa_vdsp_send)(void *h_vdsp, const char *nsid, int priority, void **h_ionmem_list, uint32_t h_ionmem_num);
        int (*sprd_caa_cadence_vdsp_load_library)(void *h_vdsp, const char *nsid);
        int (*sprd_caa_vdsp_Send)(const char *nsid, int priority, void **h_ionmem_list, uint32_t h_ionmem_num);
        void *(*sprd_caa_ionmem_alloc)(uint32_t size, bool iscache);
        int (*sprd_caa_ionmem_free)(void *h_ionmem);
        void *(*sprd_caa_ionmem_get_vaddr)(void *h_ionmem);
        int (*sprd_caa_ionmem_get_fd)(void *h_ionmem);
        void (*ProcessState_initWithDriver)(const char *driver);
        void (*ProcessState_startThreadPool)();
        void (*IPCThreadState_joinThreadPool)(bool isMain);
        void (*IPCThreadState_stopProcess)(bool immediate);
} camalg_assist_lib_api_t;




#endif
