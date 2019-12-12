#include "key.h"
int get_input_key(char *input_key)
{
    if(strlen(INPUT_KEY)) {
        memcpy(input_key, INPUT_KEY, sizeof(INPUT_KEY));
        return 0;
    } else
        return 1;
}
