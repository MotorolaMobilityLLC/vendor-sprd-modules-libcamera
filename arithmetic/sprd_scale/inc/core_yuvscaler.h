#ifndef __CORE_YUVSCALER_H__
#define __CORE_YUVSCALER_H__

typedef signed char scaler_int8;
typedef short scaler_int16;
typedef int scaler_int32;
typedef unsigned char scaler_uint8;
typedef unsigned short scaler_uint16;
typedef unsigned int scaler_uint32;

#ifdef __cplusplus
extern "C" {
#endif

int Y_U_V420_scaler(scaler_uint8 *dst_buf, scaler_uint16 dst_width,
                    scaler_uint16 dst_height, scaler_uint8 *src_buf,
                    scaler_uint16 src_width, scaler_uint16 src_height);

#ifdef __cplusplus
};
#endif

#endif
