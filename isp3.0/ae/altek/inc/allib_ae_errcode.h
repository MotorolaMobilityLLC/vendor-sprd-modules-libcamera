/******************************************************************************
 *  File name: allib_ae_errcode.h
 *  Create Date:
 *
 *  Comment:
 *  Define lib version
 *
 *****************************************************************************/

#ifndef ALAELIB_ERR_H_
#define ALAELIB_ERR_H_

#define _AL_3ALIB_SUCCESS                  ( 0x00 )

#define _AL_AELIB_GLOBAL_ERR_OFFSET        ( 0xA000 )
#define _AL_AELIB_SETTINGFILE                    (  _AL_AELIB_GLOBAL_ERR_OFFSET + 0x100 )
#define _AL_AELIB_LIBPROCESS                     (  _AL_AELIB_GLOBAL_ERR_OFFSET + 0x200 )


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
#define _AL_AELIB_INVALID_STATS_ADDR         ( _AL_AELIB_GLOBAL_ERR_OFFSET  + 0x0D )

/* for setting file  */
#define _AL_AELIB_INVALID_GDCALIB_GAIN                        ( _AL_AELIB_SETTINGFILE + 0x01 )
#define _AL_AELIB_INVALID_INIT_PARAM                            ( _AL_AELIB_SETTINGFILE + 0x02 )
#define _AL_AELIB_INVALID_TARGETMEAN                            ( _AL_AELIB_SETTINGFILE + 0x03 )
#define _AL_AELIB_INVALID_LOWHIGHLIGHTRANGE             ( _AL_AELIB_SETTINGFILE + 0x04 )
#define _AL_AELIB_INVALID_HDR_NODE                               ( _AL_AELIB_SETTINGFILE + 0x05 )
#define _AL_AELIB_INVALID_FACECOMP_NODE                     ( _AL_AELIB_SETTINGFILE + 0x06 )
#define _AL_AELIB_INVALID_TOUCHCOMP_NODE                   ( _AL_AELIB_SETTINGFILE + 0x07 )
#define _AL_AELIB_INVALID_LVCOMP_NODE                          ( _AL_AELIB_SETTINGFILE + 0x08 )
#define _AL_AELIB_INVALID_VIDEOCOMP_NODE                    ( _AL_AELIB_SETTINGFILE + 0x09 )
#define _AL_AELIB_INVALID_FACERDCOMP_NODE                 ( _AL_AELIB_SETTINGFILE + 0x0A )
#define _AL_AELIB_INVALID_LVPCURVE_NODE                      ( _AL_AELIB_SETTINGFILE + 0x0B )
#define _AL_AELIB_INVALID_LVPCURVE_PARAM                    ( _AL_AELIB_SETTINGFILE + 0x0C )
#define _AL_AELIB_INVALID_CAPTUREPCURVE_NODE           ( _AL_AELIB_SETTINGFILE + 0x0D )
#define _AL_AELIB_INVALID_CAPTUREPCURVE_PARAM         ( _AL_AELIB_SETTINGFILE + 0x0E )
#define _AL_AELIB_INVALID_FE_NODE                                  ( _AL_AELIB_SETTINGFILE + 0x0F )
#define _AL_AELIB_INVALID_FE_PARAM                                ( _AL_AELIB_SETTINGFILE + 0x10 )
#define _AL_AELIB_INVALID_INTAE_SKYPROTECT_PARAM            ( _AL_AELIB_SETTINGFILE + 0x11 )


/* for Lib processing */
#define _AL_AELIB_PURE_DARK_IMAGE                                  (_AL_AELIB_LIBPROCESS + 0x01 )
#define _AL_AELIB_ERR_PROCESS_REGEN                              (_AL_AELIB_LIBPROCESS + 0x02 )
#define _AL_AELIB_ERR_PROCESS_BLOCKS                             (_AL_AELIB_LIBPROCESS + 0x03 )

#endif /* ALAELIB_ERR_H_ */
