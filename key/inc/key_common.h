#ifndef _ENC_SPRD_KEY_
#define _ENC_SPRD_KEY_
#define KEY_LEN 64
#include <stdio.h>
#include <string.h>
struct key {
    char name[KEY_LEN];
    char license[KEY_LEN];
};
unsigned int get_key_len();
#endif
