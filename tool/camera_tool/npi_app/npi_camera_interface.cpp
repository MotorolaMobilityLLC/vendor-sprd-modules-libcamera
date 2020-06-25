/*************************************************************************
	> File Name: npi_camera_interface.cpp
	> Author:
	> Mail:
	> Created Time: Tue 23 Jun 2020 07:52:25 PM CST
 ************************************************************************/
#define LOG_TAG "npi_camera_interface"

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
#include "./simb_camera.h"
#include "bbat_camera.h"

using namespace android;

extern "C" {
void register_this_module_ext(struct eng_callback *reg, int *num);
}

extern "C" void register_this_module_ext(struct eng_callback *reg, int *num) {
    int moudles_num = 0;

/*the following interface is used to test mipi camera*/
    reg->type = 0x38;
    reg->subtype = 0x06;
    reg->eng_diag_func = bbat_mipicam;
    moudles_num++;

/*the following interface is used to test back flash*/
    (reg + 1)->type = 0x38;
    (reg + 1)->subtype = 0x0C;
    (reg + 1)->diag_ap_cmd = 0x04;
    (reg + 1)->eng_diag_func = bbat_back_flash;
    moudles_num++;

/*the following interface is used to test front flash*/
    (reg + 2)->type = 0x38;
    (reg + 2)->subtype = 0x0C;
    (reg + 2)->diag_ap_cmd = 0x08;
    (reg + 2)->eng_diag_func = bbat_front_flash;
    moudles_num++;

/*the following interface is used for simba test*/
    sprintf((reg + 3)->at_cmd, "%s", "AT+GETCAMINFO");
    (reg + 3)->eng_linuxcmd_func = simba_camera_info_test;
    moudles_num++;

    *num = moudles_num;
    CMR_LOGI("moudles_num=%d", moudles_num);
}
