#include "sprd_fd_hw_api.h"
#include <stdio.h>
#include <malloc.h>
#include "MemIon.h"
#include <sprd_ion.h>
#include <sys/mman.h>
#include <utils/Timers.h>

//tmp
#include "fd_chip_test_input.h"
#include "fd_chip_test_output.h"

#define IMAGE_LEN 76800
#define RESULT_LEN 11696
#define PARAM_LEN 20

static fd_u32 PARA[PARAM_LEN] = {0x00000000,0x01E00280,0x00160040,0x1C14283B,0x007FFFC0,
						 0xFFFFFFFF,0xFFFFFFFF,0x00000000,0x01C20400,0x29859449,
						 0x12282E71,0x3ACAA89A,0x236D32C3,0x0C13BCEB,0x34B24714,
						 0x1D54D17C,0x06F75B65,0x2E99E58E,0x173C7FB6,0x3FDEF9DF};
typedef struct 
{
	fd_u32 para[PARAM_LEN];
}HWFD_PARA;

typedef struct sprd_camera_fd_memory {
    android::MemIon *ion_heap;
    fd_uint phys_addr;
    fd_uint phys_size;
    fd_s32 fd;
    fd_s32 dev_fd;
    void *handle;
    void *data;
    bool busy_flag;
} sprd_camera_fd_memory_t;
void InitCmdQueue(HWFD_PARA* fd_para)
{
	int i = 0;
	for(;i < PARAM_LEN; i++)
	{
		fd_para->para[i] = PARA[i];
	}
}

sprd_camera_fd_memory_t *AllocFDMem(int buf_size,int num_bufs,uint32_t is_cache)
{
    //unsigned long paddr = 0;
    //size_t psize = 0;
    //int result = 0;
    size_t mem_size = 0;
    android::MemIon *pHeapIon = NULL;

    printf("buf_size %d, num_bufs %d\n", buf_size, num_bufs);
    sprd_camera_fd_memory_t *memory = (sprd_camera_fd_memory_t *)malloc(sizeof(sprd_camera_fd_memory_t));
    if (NULL == memory) {
        printf("fatal error! memory pointer is null.\n");
        goto getpmem_fail;
    }
    memset(memory, 0, sizeof(sprd_camera_fd_memory_t));
    memory->busy_flag = false;

    mem_size = buf_size * num_bufs;
    // to make it page size aligned
    mem_size = (mem_size + 4095U) & (~4095U);
    if (mem_size == 0) {
        goto getpmem_fail;
    }

    if (1) {
        if (is_cache) {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_CAM);
        } else {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, android::MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_CAM);
        }
    } else {
        if (is_cache) {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, 0,
                                  (1 << 31) | ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pHeapIon = new android::MemIon("/dev/ion", mem_size, android::MemIon::NO_CACHING,
                                  ION_HEAP_ID_MASK_SYSTEM);
        }
    }

    if (pHeapIon == NULL || pHeapIon->getHeapID() < 0) {
        printf("pHeapIon is null or getHeapID failed\n");
        goto getpmem_fail;
    }

    if (NULL == pHeapIon->getBase() || MAP_FAILED == pHeapIon->getBase()) {
        printf("error getBase is null.\n");
        goto getpmem_fail;
    }

    memory->ion_heap = pHeapIon;
    memory->fd = pHeapIon->getHeapID();
    memory->dev_fd = pHeapIon->getIonDeviceFd();
    // memory->phys_addr is offset from memory->fd, always set 0 for yaddr
    memory->phys_addr = 0;
    memory->phys_size = mem_size;
    memory->data = pHeapIon->getBase();

    printf("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p\n",
             memory->fd, memory->phys_addr, memory->data, memory->phys_size,pHeapIon);

    return memory;

getpmem_fail:
    if (memory != NULL) {
        free(memory);
        memory = NULL;
    }
    return NULL;
}
void FreeFDMem(sprd_camera_fd_memory_t *memory)
{
    if (memory)
	{
        if (memory->ion_heap)
		{
            printf("fd=0x%x, phys_addr=0x%lx, virt_addr=%p, size=0x%lx, heap=%p\n",
                memory->fd, memory->phys_addr, memory->data, memory->phys_size,memory->ion_heap);
            delete memory->ion_heap;
            memory->ion_heap = NULL;
        } else {
            printf("memory->ion_heap is null:fd=0x%x, phys_addr=0x%lx, "
                     "virt_addr=%p, size=0x%lx,heap=%p \n",
                     memory->fd, memory->phys_addr, memory->data,
                     memory->phys_size,memory->ion_heap);
        }
        free(memory);
        memory = NULL;
    }
}
sprd_camera_fd_memory_t * CopyToIon(void *data,fd_u32 size)
{
	sprd_camera_fd_memory_t *memory = NULL;
	
	memory = AllocFDMem(size, 1, false);
	if (NULL == memory)
	{
	    ALOGE("error memory is null.");
	    goto mem_fail;
    }

	memcpy(memory->data,data,size);
	return memory;

mem_fail:
	FreeFDMem(memory);
	return NULL;
}







#include <stdlib.h>
#include <string>
#include <memory>
#include <time.h>

int main(int argc, char* argv[])
{
	//printf("%s \n",HW_FdGetVersion());
	int num = 0;
	if(argc >=2)
	{
		num = atoi(argv[1]);
	}

	
	HWFD_DETECTOR_HANDLE hDT;
	HWFD_REQUEST request;
	HWFD_RESPONSE response;

	std::string yuvfile = "/data/1.yuv";

    std::shared_ptr<char> y_data(new char[320*240]);

    FILE* pf = fopen(yuvfile.c_str(),"rb");
    if(pf)
    {
        fread(y_data.get(),320*240,1,pf);
        fclose(pf);
    }
    else
    {
        printf("can not open the input image!\n");
        //return -1;
    }

	int ret = hwfd_open(&hDT);
	if(ret == HWFD_ERROR)
		return -1;

	printf("Get FD state is %d\n",hwfd_is_busy(hDT));


	//fd_u32 size = grayImage->height * grayImage->height * 3 / 2;
	//sprd_camera_fd_memory_t *input_mem = CopyToIon(grayImage->data,size);
	sprd_camera_fd_memory_t* input_image = CopyToIon(&input,IMAGE_LEN*4);
	request.fd_image_addr = input_image->fd;

	//request.fd_image_addr_debug = y_data.get();
	//request.fd_image_size_debug = 320*240;

	HWFD_PARA hwfd_para;
	InitCmdQueue(&hwfd_para);
	
	request.fd_cmd_addr = &hwfd_para.para;
	request.fd_cmd_size = PARAM_LEN*4;
	request.fd_cmd_num = 0x00000000;
	request.fd_image_linestep = 0x00000280;


	//request2.fd_cmd_addr = NULL;//cmd queue addr
	//request2.fd_cmd_size = 0;//cmd queue size
	//request2.fd_cmd_num = 0x00000000;
	//request2.fd_image_addr = y_data.get();
	//request2.fd_image_linestep = 0x00000280;
	//request2.fd_image_size = 320*240;


	int i = 0;
	for(;i < num;i++)
	{
		int64_t start_time = systemTime(CLOCK_MONOTONIC);
		hwfd_start_fd(hDT,&request,&response);
		//hwfd_start_fd2(hDT,&request2,&response);//for debug
		int64_t end_time = systemTime(CLOCK_MONOTONIC);


		printf("Get face num %02X,take %ld ms\n",response.face_count,(end_time - start_time)/1000000);
	}

	
	//tmp compare result
	//for(int i = 0;i < RESULT_LEN;i++)
	//{
	//	fd_u32 val = *(volatile fd_u32*)(out_buff + i*4);
	//	if(val != output[i])
	//	{
	//		ALOGD("Result is diff .");
	//		break;
	//	}
	//}
	FILE *fp = fopen("/data/fd_out" , "wb");
	if (fp) {
		fwrite(response.data, sizeof(fd_u8), OUT_BUFFER_SIZE, fp);
		fflush(fp);
		fclose(fp);
		printf("out saved.\n");
	}
	else
	{
		printf("fd out open error:%d\n" , errno);
	}

	
	//FreeFDMem(input_image);
	hwfd_close(&hDT);
	return 0;
}
