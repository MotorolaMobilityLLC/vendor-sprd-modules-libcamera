#ifndef _ALGOINT_TYPE_H
#define _ALGOINT_TYPE_H

#include <stdint.h>

//INT
typedef unsigned char       BOOLEAN;
typedef signed char         int8;
typedef short               int16;
typedef int                 int32;
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef long long           int64;
typedef unsigned long long  uint64;
#if 0
//LIMIT MACROS 
#define INT8_MIN	(-0x7f - 1)
#define INT16_MIN	(-0x7fff - 1)
#define INT32_MIN	(-0x7fffffff - 1)

#define INT8_MAX	0x7f
#define INT16_MAX	0x7fff
#define INT32_MAX	0x7fffffff
#define UINT8_MAX	0xff
#define UINT16_MAX	0xffff
#define UINT32_MAX	0xffffffff
#endif

//CORE DLL
#ifdef _MSC_VER
#   ifdef ISP_USRDLL
#       ifdef ISP_DLL_EXPORTS
#           define ISP_CORE_API __declspec(dllexport)
#       else
#           define ISP_CORE_API __declspec(dllimport)
#       endif
#   else
#       define ISP_CORE_API
#   endif
#else
#   define ISP_CORE_API
#endif


#endif
