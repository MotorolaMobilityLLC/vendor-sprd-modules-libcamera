#ifndef __SPRD_PORTRAIT_SCENE_ADAPTER_HEADER_H__
#define __SPRD_PORTRAIT_SCENE_ADAPTER_HEADER_H__
#include <stdlib.h>

#include "sprd_camalg_adapter.h"
#include "unisoc_portrait_scene_interface.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef enum sprd_portrait_scene_cmd{
    SPRD_PORTRAIT_SCENE_WEIGHT_CMD = 0,
    SPRD_PORTRAIT_SCENE_PROCESS_CMD,
    SPRD_PORTRAIT_SCENE_FUSE_CMD,
    SPRD_PORTRAIT_SCENE_MAX_CMD,
} sprd_portrait_scene_cmd_t;

typedef struct sprd_portrait_scene_adapter_mask {
	sprd_portrait_scene_channel_t ch;
	uint8_t *src_YUV;
	uint8_t *dst_YUV;
	uint16_t *mask;
} sprd_portrait_scene_adapter_mask_t;

typedef struct sprd_portrait_scene_adapter_fuse {
	uint8_t *src_YUV;
	uint8_t *dst_YUV;
	sprd_portrait_scene_fuse_t fuse;
} sprd_portrait_scene_adapter_fuse_t;

//#define CLOCKS_PER_SEC 1000
#define CLK_TCK 1000

#define BYTE unsigned char   
#define WORD unsigned short int 

#define DWORD unsigned int   
#define SDWORD signed int

#define SBYTE signed char   
#define SWORD signed short int

int load_JPEG_header(BYTE *buf, DWORD length_of_file, DWORD *X_image, DWORD *Y_image);
void decode_JPEG_image();
int get_JPEG_buffer(WORD X_image, WORD Y_image, BYTE **address_dest_buffer);

//char error_string[90];

typedef struct s_BM_header {
	WORD BMP_id; // 'B''M'   
	DWORD size; // size in bytes of the BMP file   
	DWORD zero_res; // 0   
	DWORD offbits; // 54   
	DWORD biSize; // 0x28   
	DWORD Width;  // X   
	DWORD Height;  // Y   
	WORD  biPlanes; // 1   
	WORD  biBitCount; // 24   
	DWORD biCompression; // 0 = BI_RGB   
	DWORD biSizeImage; // 0   
	DWORD biXPelsPerMeter; // 0xB40   
	DWORD biYPelsPerMeter; // 0xB40   
	DWORD biClrUsed; //0   
	DWORD biClrImportant; //0   
} BM_header;
typedef struct s_RGB {
	BYTE B;
	BYTE G;
	BYTE R;
} RGB;

typedef struct {
	BYTE Length[17];  // k =1-16 ; L[k] indicates the number of Huffman codes of length k   
	WORD minor_code[17];  // indicates the value of the smallest Huffman code of length k   
	WORD major_code[17];  // similar, but the highest code   
	BYTE V[65536];  // V[k][j] = Value associated to the j-th Huffman code of length k   
	// High nibble = nr of previous 0 coefficients   
	// Low nibble = size (in bits) of the coefficient which will be taken from the data stream   
} Huffman_table;

typedef void(*decode_MCU_func)(DWORD);

// Used markers:   
#define SOI 0xD8   
#define EOI 0xD9   
#define APP0 0xE0   
#define SOF 0xC0   
#define DQT 0xDB   
#define DHT 0xC4   
#define SOS 0xDA   
#define DRI 0xDD   
#define COM 0xFE

#define exit_func(err) { strcpy(error_string, err); return 0;}
#define BYTE_p(i) bp=buf[(i)++]   
#define WORD_p(i) wp=(((WORD)(buf[(i)]))<<8) + buf[(i)+1]; (i)+=2 

#define RIGHT_SHIFT(x,shft) ((shift_temp = (x)) < 0 ? (shift_temp >> (shft)) | ((~(0L)) << (32 - (shft))) : (shift_temp >> (shft)))
#define DESCALE(x,n)  RIGHT_SHIFT((x) + (1L << ((n)-1)), n)   
#define RANGE_MASK 1023L

JNIEXPORT void write_buf_to_BMP(BYTE *im_buffer, WORD X_bitmap, WORD Y_bitmap, char *BMPname);
JNIEXPORT DWORD filesize(FILE *fp);
JNIEXPORT int jpg2bmp(BYTE *jpg_buf0, BYTE *bmp_buf, DWORD length_of_file, DWORD X_image, DWORD Y_image);

JNIEXPORT void *sprd_portrait_scene_adpt_init(sprd_portrait_scene_init_t *_param);
/* sprd_portrait_scene_adpt_init
* usage: call once at initialization
* return value: handle
* @ param: reserved, can be set NULL
*/

JNIEXPORT int sprd_portrait_scene_adpt_deinit(void *handle, sprd_portrait_scene_channel_t ch);
/* sprd_portrait_scene_adpt_deinit
* usage: call once at deinitialization
* return value: 0 is ok, other value is failed
* @ handle: algo handle
*/

JNIEXPORT int sprd_portrait_scene_adpt_ctrl(void *handle, sprd_portrait_scene_cmd_t cmd, void *param);
/* sprd_portrait_scene_adpt_ctrl
* usage: call each frame	
* return value: 0 is ok, other value is failed
* @ handle: algo handle
* @ cmd: switch  executing path
* @ param: depend on cmd type:
*- SPRD_PORTRAIT_SCENE_WEIGHT_CMD
*- SPRD_PORTRAIT_SCENE_PROCESS_CMD
*- SPRD_PORTRAIT_SCENE_FUSE_CMD
*/

JNIEXPORT int sprd_portrait_scene_get_devicetype(enum camalg_run_type *type);
/* sprd_portrait_scene_get_devicetype
* usage: portrait_scene adapter get running type, the output is type, such as cpu/vdsp/cpu_vdsp_fusion
* return value: 0 is ok, other value is failed
*/

#ifdef __cplusplus
}
#endif

#endif
