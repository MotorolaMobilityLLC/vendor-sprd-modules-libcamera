#include "key_common.h"

/*
 * struct key input_key is the container of the name and the
 * license of IP feature please add the name and license of 
 * your IP feature in the following array of struct named 
 * input_key as the input_key_ex shows.
 * 
 * static struct key input_key_ex[] = {
 *      {"faceid",    "license"},
 *      {"HDR",       "license"},
 *      {"warp",      "license"},
 * }
 */

static struct key input_key[] = {

};


/*
 * Please do not modify the code below
 */

unsigned int get_key_len()
{
    return sizeof(input_key);
}

struct key *retrieve_key()
{
    return input_key;
}
