#ifndef _AL3200_H_
#define _AL3200_H_
enum {
	AL3200_CHANGE_MODE = 1,
	AL3200_CMD_DEPTH_MAP_MODE = 2,
	AL3200_CMD_BYPASS_MODE = 3,
	AL3200_CMD_GET_CHIP_ID = 4,
};


int al3200_power_on(int on);
int al3200_reset(int on);
int al3200_mini_ctrl(int on);
int al3200_write_str(char *filename, void *info);
int al3200_3ainfo(void *info);
int al3200_datainfo(void *info);
#endif
