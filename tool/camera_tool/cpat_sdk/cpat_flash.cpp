/*************************************************************************
	> File Name: cpat_flash.cpp
	> Author:
	> Mail:
	> Created Time: Thu 18 Jun 2020 11:02:30 AM CST
 ************************************************************************/

#define LOG_TAG "cpat_flash"

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

#include "cpat_flash.h"

using namespace android;

static int cpat_flash_set_value(int value);

static int cpat_flash_set_value(int value)
{
    int fd;
    int ret = 0;
    char buffer[8] = {0};
    CMR_LOGI("E, flash value:0x%x",value);

    char * flash_interface = "/sys/class/misc/sprd_flash/test";
    fd = open(flash_interface, O_RDWR);
    if(fd < 0){
        CMR_LOGE("open %s failed!", flash_interface);
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "0x%x", value);
    ret = write(fd, buffer, strlen(buffer));
    if (ret != strlen(buffer)) {
        CMR_LOGE("write value failed!");
        close(fd);
        return -1;
    }

    close(fd);
    return ret;
}

int cpat_open_back_white_flash(void)
{
    int ret = 0;
    ret = cpat_flash_set_value(0x10);
    return ret;
}

int cpat_open_back_color_flash(void)
{
    int ret = 0;
    ret = cpat_flash_set_value(0x20);
    return ret;
}

int cpat_close_back_flash(void)
{
    int ret = 0;
    ret = cpat_flash_set_value(0x31);
    return ret;
}

int cpat_open_front_flash(void)
{
    int ret = 0;
    ret = cpat_flash_set_value(0x8072);
    return ret;
}

int cpat_close_front_flash(void)
{
    int ret = 0;
    ret = cpat_flash_set_value(0x8000);
    return ret;
}

