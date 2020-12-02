#ifndef __COMPARE_FUNC_H__
#define __COMPARE_FUNC_H__
#include "test_common_header.h"

typedef enum {
    Y_RENGION,
    V_RENGION,
    U_RENGION
}RENGION;

int compare_Image(compareInfo_t &compinfo);
#endif