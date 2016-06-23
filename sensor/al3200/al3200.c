#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "cmr_types.h"
#include "al3200.h"
#include <video/al3200_drv_kernel.h>

int al3200_power_on(int on)
{
	int fd = 0;
	int ret;

	fd = open("dev/mini_isp_device", O_RDWR, 0);
	if (0 > fd) {
		SENSOR_LOGE("can not open file al3200_power_on \n");
		return 0;
	}
	SENSOR_LOGE("on = %d", on);
	ret = ioctl(fd, AL3200_IO_POWER, &on);
	if (ret) {
		SENSOR_LOGE("failed to AL3200_IO_POWER %d", ret);
	}
	close(fd);

	return ret;

}

int al3200_reset(int on)
{
	int fd = 0;
	int ret;

	fd = open("dev/mini_isp_device", O_RDWR, 0);
	if (0 > fd) {
		SENSOR_LOGE("can not open file al3200_reset \n");
		return 0;
	}

	SENSOR_LOGE("on = %d", on);
	ret = ioctl(fd, AL3200_IO_RESET, &on);
	if (ret) {
		SENSOR_LOGE("failed to AL3200_IO_RESET %d", ret);
	}
	close(fd);

	return ret;
}

int al3200_mini_ctrl(int on)
{
	int ret = 0;
	int fd = 0;

	fd = open("dev/mini_isp_device", O_RDWR, 0);
	if (0 > fd) {
		SENSOR_LOGE("can not open file al3200_mini_ctrl \n");
		return 0;
	}
	SENSOR_LOGE("on = %d", ret);
	switch (on) {
	case AL3200_CHANGE_MODE:
		ret = ioctl(fd, AL3200_IO_CHANGE_MODE, NULL);
		if (ret) {
			SENSOR_LOGE("failed to AL3200_CHANGE_MODE %d", ret);
		}
		close(fd);
		break;
	case AL3200_CMD_DEPTH_MAP_MODE:
		ret = ioctl(fd, AL3200_IO_DEPTHMAP_MODE, NULL);
		if (ret) {
			SENSOR_LOGE("failed to AL3200_CMD_DEPTH_MAP_MODE %d", ret);
		}
		close(fd);
		break;
	case AL3200_CMD_BYPASS_MODE:
		ret = ioctl(fd, AL3200_IO_BYPASS_BIN, NULL);
		if (ret) {
			SENSOR_LOGE("failed to AL3200_CMD_BYPASS_MODE %d", ret);
		}
		close(fd);
		break;
	case AL3200_CMD_GET_CHIP_ID:
		ret = ioctl(fd, AL3200_IO_CHIP_ID, NULL);
		if (ret) {
			SENSOR_LOGE("failed to AL3200_CMD_GET_CHIP_ID %d", ret);
		}
		close(fd);
		break;
	default:
		return -ENOTTY;
	}

	return ret;
}

int al3200_3ainfo(void *info)
{
	int fd;
	int ret = 0;

	fd = open("dev/mini_isp_device", O_RDWR, 0);
	if (0 > fd) {
		SENSOR_LOGE("can not open file al3200_3ainfo \n");
		return 0;
	}
	struct ISPCMD_DEPTH3AINFO* al3200_3ainfo = (struct ISPCMD_DEPTH3AINFO*)info;

	ret = ioctl(fd, AL3200_IO_3AINFO, &al3200_3ainfo);
	if (ret) {
		SENSOR_LOGE("failed to AL3200_IO_3AINFO %d", ret);
	}
	close(fd);

	return ret;

}

int al3200_datainfo(void *info)
{
	int fd;
	int ret = 0;

	fd = open("dev/mini_isp_device", O_RDWR, 0);
	if (0 > fd) {
		SENSOR_LOGE("can not open file al3200_datainfo \n");
		return 0;
	}
	struct ISPDATA_INFO* al3200_datainfo = (struct ISPDATA_INFO*)info;

	ret = ioctl(fd, AL3200_IO_DATAINFO, &al3200_datainfo);
	if (ret) {
		SENSOR_LOGE("failed to AL3200_IO_DATAINFO %d", ret);
	}
	close(fd);

	return ret;
}
