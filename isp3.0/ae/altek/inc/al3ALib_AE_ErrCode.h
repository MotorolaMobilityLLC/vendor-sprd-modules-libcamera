////////////////////////////////////////////////////////////////////
//  File name: al3ALib_AE_ErrCode.h
//  Create Date:
//
//  Comment:
//  Define lib version
//
////////////////////////////////////////////////////////////////////

#ifndef ALAELIB_ERR_H_
#define ALAELIB_ERR_H_

#define _AL_3ALIB_SUCCESS                   0x00

#define _AL_AELIB_GLOBAL_ERR_OFFSET         0xA000

#define _AL_AELIB_CHKERR                    ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x01 )
#define _AL_AELIB_INVALID_PARAM             ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x02 )
#define _AL_AELIB_INVALID_HANDLE            ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x03 )
#define _AL_AELIB_AE_LIB_ERR                ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x04 )
#define _AL_AELIB_INVALID_ADDR              ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x04 )
#define _AL_AELIB_FAIL_INIT_AE_SET          ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x05 )
#define _AL_AELIB_FAIL_INIT_FE_SET          ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x06 )
#define _AL_AELIB_INVALID_ISO               ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x07 )
#define _AL_AELIB_MISMATCH_STATS_SIZE       ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x08 )
#define _AL_AELIB_INCONSIST_SETDAT          ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x09 )
#define _AL_AELIB_SOF_FRAME_INDEX_MISMATCH  ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x0A )
#define _AL_AELIB_INVALID_ISOLEVEL          ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x0B )
#define _AL_AELIB_INVALID_CALIB_GAIN          ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x0C )

#endif /* ALAELIB_ERR_H_ */