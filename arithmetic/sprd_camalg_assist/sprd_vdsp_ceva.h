#ifndef __SPRD_VDSP_CEVA_H__
#define __SPRD_VDSP_CEVA_H__

#define SPRD_DSP_CMD_INLINE_DATA_SIZE	16
#define SPRD_DSP_CMD_INLINE_BUFFER_COUNT	1
#define SPRD_DSP_CMD_NAMESPACE_ID_SIZE	16

enum sprd_dsp_cmd_flag {
	SPRD_DSP_CMD_FLAG_REQUEST_VALID = 1,  /* request valid cmd */
	SPRD_DSP_CMD_FLAG_RESPONSE_VALID = 2,  /* response valid cmd */
	SPRD_DSP_CMD_FLAG_REQUEST_NSID = 4,   /* request nsid cmd, with REQUEST_VALID */
	SPRD_DSP_CMD_FLAG_RESPONSE_DELIVERY_FAILED = 8 /*/ response fail cmd */
};

struct sprd_dsp_buffer {
	uint32_t flags;
	uint32_t size;    /* buffer size */
	int32_t fd;   /* buffer addr */
	uint32_t addr;
};

struct sprd_dsp_cmd {
	sprd_dsp_cmd_flag flag; /* flag 0 */
	uint32_t in_data_size;   /* in data size */
	int32_t in_data_fd;
	uint32_t out_data_size;  /* out data size */
	int32_t out_data_fd;
	uint32_t buffer_size; /* buffer size */
	union {
	/* if in_data_size > sizeof in_data, remap it then send to vdsp directly*/
		uint32_t in_data_addr;
		uint8_t in_data[SPRD_DSP_CMD_INLINE_DATA_SIZE];
	};
	union {
		uint32_t out_data_addr;
		uint8_t out_data[SPRD_DSP_CMD_INLINE_DATA_SIZE];
	};
	union {
		uint32_t buffer_addr; /*same as in_data_size*/
		struct sprd_dsp_buffer buffer_data[SPRD_DSP_CMD_INLINE_BUFFER_COUNT];
		uint8_t buffer_alignment[SPRD_DSP_CMD_INLINE_DATA_SIZE];
	};
	char nsid[SPRD_DSP_CMD_NAMESPACE_ID_SIZE]; /* algo id, ex. "hdr"*/
	uint32_t priority; /* 0 is normal, 1 is high priority */
};

int ceva_vdsp_open(void **h_vdsp);
int ceva_vdsp_close(void *h_vdsp);
int ceva_vdsp_send(void *h_vdsp, const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num);
int ceva_vdsp_Send(const char *nsid, int priority,
	void **h_ionmem_list, uint32_t h_ionmem_num);

#endif
