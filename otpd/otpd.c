/**
 * otpd.c ---
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <signal.h>
#include "otpd.h"
#include <sys/prctl.h>
#include <errno.h>
#include <cutils/android_filesystem_config.h>
#include <sys/capability.h>
#include "otp_common.h"
#include "otp_fs.h"
#include "otp_buf.h"

#define MAX_CAP_NUM         (CAP_TO_INDEX(CAP_LAST_CAP) + 1)
char *s_otp;

void switchUser( ) {

	prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

	if (setresuid(AID_SYSTEM, AID_SYSTEM, AID_SYSTEM) == -1) {
		OTP_LOGE("%s setresuid failed: %s", __FUNCTION__,strerror(errno));
	}

	struct __user_cap_header_struct header;
	memset(&header, 0, sizeof(header));
	header.version = _LINUX_CAPABILITY_VERSION_3;
	header.pid = 0;

	struct __user_cap_data_struct data[MAX_CAP_NUM];
	memset(&data, 0, sizeof(data));

	data[CAP_TO_INDEX(CAP_NET_ADMIN)].effective |= CAP_TO_MASK(CAP_NET_ADMIN);
	data[CAP_TO_INDEX(CAP_NET_ADMIN)].permitted |= CAP_TO_MASK(CAP_NET_ADMIN);

	data[CAP_TO_INDEX(CAP_NET_RAW)].effective |= CAP_TO_MASK(CAP_NET_RAW);
	data[CAP_TO_INDEX(CAP_NET_RAW)].permitted |= CAP_TO_MASK(CAP_NET_RAW);

	data[CAP_TO_INDEX(CAP_BLOCK_SUSPEND)].effective |= CAP_TO_MASK(CAP_BLOCK_SUSPEND);
	data[CAP_TO_INDEX(CAP_BLOCK_SUSPEND)].permitted |= CAP_TO_MASK(CAP_BLOCK_SUSPEND);

	if (capset(&header, &data[0]) == -1) {
		OTP_LOGE("%s capset failed:",__FUNCTION__);
	}

}

int main( ) {

	int ret = 0;
	pthread_t tid;
	pthread_attr_t attr;
	struct sigaction action;

	memset(&action, 0x00, sizeof(action));
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);

	ret = sigaction(SIGPIPE, &action, NULL);

	if (ret < 0) {
		OTP_LOGE("%s : sigaction() failed!",__FUNCTION__);
		exit_otpd();
	}

	OTP_LOGD("%s :start switchUser!!!",__FUNCTION__);

	switchUser();

	s_otp = (char *)malloc(PROPERTY_VALUE_MAX);
	property_get(OTP_RADIO_TYPE, (char *)s_otp, "");
	OTP_LOGE("%s = %s", OTP_RADIO_TYPE, s_otp);

#if 0 //for test
	if (strcmp(s_otp, "") == 0) {
	free((char *)s_otp);
	exit_otpd();
	}
#endif

	init_otp_parament();
	poweron_init_otpdata();

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&tid, &attr, (void*)otpd_listenaccept_thread, NULL)< 0) {
		OTP_LOGE("Failed to create otpd listen accept thread");
	}

	do {
		pause();
	} while (1);

	return 0;
}
