#define LOG_TAG "simb_camera"

#include <utils/String16.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <media/hardware/MetadataBufferType.h>
#include "cmr_common.h"
#include "sprd_ion.h"
#include <linux/fb.h>
#include "sprd_cpp.h"
#include <dlfcn.h>
#include <utils/Mutex.h>
#include "MemIon.h"
#include "sprd_fts_type.h"
#include "../cpat_sdk/cpat_camera.h"

static int simba_handle_req(char *input, char *output);

int simba_camera_info_test(char *req, char *rsp) {
    if(!rsp || !req)
        return SENSOR_CAPT_FAIL;
    cpat_camera_get_camera_info(req, rsp);
    return 0;
}

static int simba_handle_req(char *input, char *output) {
    int count = 0;
    int input_length = (int)strlen(input);
    char *pointer_for_output = NULL;
    char temp_for_sensor[64];
    pointer_for_output = strtok(input, "\n");
    while (pointer_for_output) {
        strcat(output, pointer_for_output);
        pointer_for_output = strtok(NULL, "\n");
        count++;
    }
    return count;
}