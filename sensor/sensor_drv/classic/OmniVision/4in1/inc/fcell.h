#ifndef __EXPORT_H__
#define __EXPORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define E_OV_FCD_OK  							0
#define E_OV_FCD_INVALIDPARA					0x10001
#define E_OV_FCD_NOMEMORY						0x10002

#define E_OV_FCD_CFA16							0x0001
#define E_OV_FCD_CFA10_MIPI						0x0002
#define E_OV_FCD_CFA10_TIGHT					0x0004
#define E_OV_FCD_MIRROR							0x0010
#define E_OV_FCD_FLIP							0x0020
#define E_OV_FCD_FLAG_DPC						0x0100
#define E_OV_FCD_FLAG_OTPDPC					0x0200
#define E_OV_FCD_FLAG_XTALK						0x0400

#define E_OV_FCD_MIRROR_XTALK					0x10000
#define E_OV_FCD_MIRROR_OTPDPC					0x20000
#define E_OV_FCD_FLIP_XTALK						0x40000
#define E_OV_FCD_FLIP_OTPDPC					0x80000


typedef struct tag_ovfcell_init
{
	int width;									/* width of fcell image */
	int height;									/* height of fcell image */
	int xtalk_len;								/*otp xtalk data length which depends on module input*/
	void* xtalk;								/*otp xtalk data buffer*/
	int otpdpc_len;								/*otp dpc data length which depends on module input*/
	void* otpdpc;								/*otp dpc buffer*/
	short ofst_xtalk[2];						/*X/Y direction offset, Range: 0~4095*/
	short imgwin_xtalk[2];						/*vendor image width/height, Range: 0~4095*/
	short ofst_otpdpc[2];						/*X/Y direction offset, Range: 0~8191*/
	int flag;									/*xtalk/otpdpc/dpc control flag*/
}ovfcell_init;

typedef struct tag_ovfcell_control
{
	int pattern;								/* data cfa pattern (MIRROR/FLIP)/DPC/OTPDPC/XTALk */
	int gain_camera;							/*camera gain  16~256*/
	int xtk_blc;								/*xtalk blc. Range 0~255*/
}ovfcell_control;

typedef void*	(*fcell_allocator)(unsigned int);
typedef void	(*fcell_deallocator)(void*);
 
unsigned int 	ov_fcell_getversion();
int 			ov_fcell_init(void* fcell_initdat, fcell_allocator allocator, fcell_deallocator deallocator);
void			ov_fcell_release();
int				ov_fcell_process(unsigned short* src, unsigned short* dst, ovfcell_control* control);


#ifdef __cplusplus
}
#endif
#endif