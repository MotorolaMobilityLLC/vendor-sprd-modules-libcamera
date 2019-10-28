#ifndef _TYPE_DEF_H_
#define _TYPE_DEF_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//typedef __int64             Word64;
//typedef unsigned __int64    Word64u;
//typedef long                Word32;
//typedef unsigned long       Word32u;
//typedef short               Word16;
//typedef unsigned short      Word16u;
//typedef char                Word8;
//typedef unsigned char       Word8u;

typedef unsigned char BYTE;
typedef unsigned char *PBYTE;
typedef unsigned short WORD;
typedef unsigned short *PWORD;
typedef unsigned long DWORD;
typedef unsigned long LWORD;
typedef unsigned long ULONG;

/*
typedef __int64             Word64;
typedef unsigned __int64    Word64u;
*/
typedef long                Word32;
typedef unsigned long       Word32u;
typedef short               Word16;
typedef unsigned short      Word16u;
typedef char                Word8;
typedef unsigned char       Word8u;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define DIFF(x, y) (((x)>(y))?((x)-(y)):((y)-(x)))
//#define ABS(x) ((x<0)?-(x):(x))
#define MAXA(x,y) ((ABS(x)>ABS(y))?(x):(y))
#define MIN2(x, y) (((x)<(y))?(x):(y))
#define MAX2(x, y) (((x)>(y))?(x):(y))
#define DIFF2(x, y) (((x)-(y))*((x)-(y)))
#define CLIP(x,a,b) (MIN2(MAX2(x,a),b))
#define Clamp(x, a, b)		(((x)<(a))? (a): ((x)>(b))? (b): (x))

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef char  int8;
typedef short int16;
typedef int   int32;

typedef int                 BOOL;

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE  1
#define FALSE 0

#endif
