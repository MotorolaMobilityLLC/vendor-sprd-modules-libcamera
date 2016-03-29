/******************************************************************************
 * File name: allib_ae_errcode.h
 * brief	Flicker Lib. related error code.
 * author		Hubert Huang
 *
 * date		2015/10/23
 ******************************************************************************/
#ifndef ALFLICKERLIB_ERR_H_
#define ALFLICKERLIB_ERR_H_



////Error Code
#define AL_FLICKER_SUCCESS                            0x00
#define AL_FLICKERLIB_GLOBAL_ERR_OFFSET               0xA000

#define AL_FLICKERLIB_INVALID_ADDR                    (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x01)
#define AL_FLICKERLIB_MALLOC_FAIL                     (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x02)
#define AL_FLICKERLIB_INIT_FAIL                       (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x03)
#define AL_FLICKERLIB_CLOSE_FAIL                      (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x04)
#define AL_FLICKERLIB_INVALID_PARAM                   (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x05)
#define AL_FLICKERLIB_MISMATCH_STATS_SIZE             (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x06)
#define AL_FLICKERLIB_MAX_SIZE_EXCEEDED               (AL_FLICKERLIB_GLOBAL_ERR_OFFSET + 0x07)



#endif  //ALFLICKERLIB_ERR_H_