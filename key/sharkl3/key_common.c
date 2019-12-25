#include "key_common.h"

struct key input_key[] = {
    {"HDR",      "license"},
    {"filter",   "license"},
    {"FaceID",   "license"},
    {"portrait", "license"},
};

unsigned int get_key_len()
{
    if(sizeof(input_key))
        return sizeof(input_key);
    else
        return -1;
    return 0;
}

struct key *retrieve_key()
{
    return input_key;
}
