/**
 * otpd_listen.c ---
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#define LOG_TAG "OTPD"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <cutils/sockets.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <stdbool.h>
#include <pthread.h>
#include "otp_common.h"
#include "otpd.h"
#include "otp_fs.h"
#include "otp_buf.h"
#include "otp_os.h"

/* otpd client fd */
static int s_clientFd[MAX_CLIENT_NUM];
/* listen thread notify by pipe */
static int s_notifypipe[] = {-1, -1};
extern _RAMOTP_CTL ramOtpCtl;

void exit_otpd(void) {
    OTP_LOGD("%s:exit_otpd!!!",__FUNCTION__);
    exit(-1);
}

static uint16 read_otp_data(char *opt_data,uint16 otpdatasize,uint16 part_number) {

    uint16 readotpdatasize = 0;
    
    getMutex();
    //readotpdatasize = ramDisk_Read(ramOtpCtl.part[part_number].fdhandle,ramOtpCtl.part[part_number].fromHal.diskbuf, otpdatasize);
    readotpdatasize = ramDisk_Read(ramOtpCtl.part[part_number-1].fdhandle,opt_data, otpdatasize);
    if(readotpdatasize > 0){
        //memcpy(opt_data,ramOtpCtl.part[0].fromHal.diskbuf, otpdatasize);
        OTP_LOGD("%s: otp data is ok",__FUNCTION__);
    }
    else{
        OTP_LOGD("%s: otp data is failed",__FUNCTION__);
    }
    putMutex();
    
    return readotpdatasize;
}

static uint16 read_golden_otp_data(char *opt_data,uint16 otpdatasize,uint16 part_number) {
    uint16 golden_data_length = 0;
    
    getMutex();
    golden_data_length = ramDisk_Read_Golden(ramOtpCtl.part[part_number].fdhandle,ramOtpCtl.part[part_number].fromHal.diskbuf, otpdatasize);
    if(golden_data_length > 0){
        memcpy(opt_data,ramOtpCtl.part[part_number].fromHal.diskbuf,golden_data_length);
        OTP_LOGD("%s: otp data is ok",__FUNCTION__);
    }
    else{
        OTP_LOGD("%s: otp data is failed",__FUNCTION__);
    }
    putMutex();
    
    return golden_data_length;
}

static bool write_otp_data(char *opt_data,uint16 otpdatasize,uint16 part_number) {
    BOOLEAN ret = 0;
    ret = saveToDisk(0,opt_data,otpdatasize,part_number);
    return ret;
}

static bool write_otp_golden_data(char *opt_data,uint16 otpdatasize,uint16 part_number) {
    BOOLEAN ret = 0;
    ret = saveToDisk(1,opt_data,otpdatasize,part_number);
    return ret;
}

static int response_info_sockclients(const char *buf, const int len) {
	int i, ret;
	/* info socket clients that otp is assert/hangup/blocked */
	for (i = 0; i < MAX_CLIENT_NUM; i++) {
		// OTP_LOGD("s_clientFd[%d]=%d\n",i, s_clientFd[i]);
		if (s_clientFd[i] >= 0) {
			ret = write(s_clientFd[i], buf, len);
			OTP_LOGD("%s :write %d bytes to s_clientFd[%d]: %d",__FUNCTION__,len,i,s_clientFd[i]);
			if (ret < 0) {
				OTP_LOGE("%s:reset client_fd[%d] = -1",__FUNCTION__,i);
				close(s_clientFd[i]);
				s_clientFd[i] = -1;
			}
		}
	}
	return 0;
}

static void *otpd_listen_read_thread(void) {
    char bufdata[BUFFER_SIZE] = {0};
    char controlcmd[OTP_CONTROL_CMD_SIZE] = {0};
    char controldata[OTP_CONTROL_DATA_SIZE] = {0};
    char otpdata[OTP_DATA_SIZE] = {0};
    char otpdatalength[OTP_LENGTH_SIZE] = {0};
    uint16  tmp_otpdata_size = 0;
    int readnum = -1;
    int i = 0 , num = 0;
    fd_set OptClientFds;
    int NOptClientFds = 0;  // max otpclient_fd;
    uint16 part_number = 0;
    char part_id;

    OTP_LOGD("%s: enter", __FUNCTION__);
    for (;;) {
    FD_ZERO(&OptClientFds);
    NOptClientFds = s_notifypipe[0] + 1;
    FD_SET(s_notifypipe[0], &OptClientFds);
    for (i = 0; i < MAX_CLIENT_NUM; i++) {
        if (s_clientFd[i] != -1) {
            FD_SET(s_clientFd[i], &OptClientFds);
            if (s_clientFd[i] >= NOptClientFds)
            NOptClientFds = s_clientFd[i] + 1;
        }
    }
    OTP_LOGD("%s: begin select", __FUNCTION__);
    //num = select(NOptClientFds, &OptClientFds, &OptClientFds, NULL, NULL);
    num = select(NOptClientFds, &OptClientFds, NULL, NULL, NULL);
    OTP_LOGD("%s: after  select n = %d", __FUNCTION__, num);
    if (num > 0 && (FD_ISSET(s_notifypipe[0], &OptClientFds))) {
        char buf[BUFFER_SIZE];
        memset(buf, 0, sizeof(buf));
        read(s_notifypipe[0], buf, sizeof(buf) - 1);
        OTP_LOGD("%s: client cnct to otpd num = %d, buf = %s",__FUNCTION__, num, buf);
        if (num == 1) {
            continue;
        }
    }
    for (i = 0; (i < MAX_CLIENT_NUM) && (num > 0); i++) {
        int nfd = s_clientFd[i];
        
        if (nfd != -1 && FD_ISSET(nfd, &OptClientFds)) {
            num--;
            OTP_LOGD("%s: begin read ", __FUNCTION__);
            memset(bufdata, 0, BUFFER_SIZE);
            memset(controlcmd, 0, OTP_CONTROL_CMD_SIZE);
            memset(controldata, 0, OTP_CONTROL_DATA_SIZE);
            memset(otpdata, 0, OTP_DATA_SIZE);
            part_number = 0;
            readnum =  read(nfd, bufdata, sizeof(bufdata));
            OTP_LOGD("%s: after read %s", __FUNCTION__, bufdata);
            //test code
            otp_dump_data(bufdata,BUFFER_SIZE);

            if (readnum <= 0) {
                close(s_clientFd[i]);
                s_clientFd[i] = -1;
            } else {
                bool ret = 0;
                
                memcpy(controlcmd,bufdata,OTP_CONTROL_CMD_SIZE);
                memcpy(controldata,&bufdata[OTP_CONTROL_CMD_SIZE],OTP_CONTROL_DATA_SIZE);
                memcpy(otpdatalength,&bufdata[OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE],OTP_LENGTH_SIZE);
                memcpy(otpdata,&bufdata[OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE+OTP_LENGTH_SIZE],OTP_DATA_SIZE);

                tmp_otpdata_size = otpdatalength[0]+((otpdatalength[1] & 0x00ff)<<8);
                part_number = controldata[0];//0x01 bokeh 0x02 w+t 0x03 spw
                part_id = controldata[0];

                if (strstr(controlcmd, OTPD_WRITE_DATA)) {
                    //write otpdata to file system
                    if((0 == tmp_otpdata_size) 
                        ||(tmp_otpdata_size > RAMOTP_DIRTYTABLE_MAXSIZE)
                        ||(tmp_otpdata_size) < RAMOTP_DIRTYTABLE_MINSIZE){
                        tmp_otpdata_size = RAMOTP_DIRTYTABLE_DEFAULTSIZE;
                    }

                    OTP_LOGD("%s: part_number %d", __FUNCTION__, part_number);
                    ret = write_otp_data(otpdata,tmp_otpdata_size,part_number);
                    if(ret){
                        OTP_LOGE("%s, write otp data is ok size:%d ", __FUNCTION__,tmp_otpdata_size);
                        response_info_sockclients(OTPD_MSG_OK, sizeof(OTPD_MSG_OK));
                    }else{
                        OTP_LOGE("%s, write otp data is failed size:%d", __FUNCTION__,tmp_otpdata_size);
                        response_info_sockclients(OTPD_MSG_FAILED, sizeof(OTPD_MSG_FAILED));
                    }
                } else if (strstr(controlcmd, OTPD_READ_DATA)) {
                    //read file system data send to client
                    memset(otpdata, 0, sizeof(otpdata));
                    OTP_LOGD("%s: part_number %d", __FUNCTION__, part_number);
                    tmp_otpdata_size = read_otp_data(otpdata,tmp_otpdata_size,part_number);
                    otpdatalength[0] = tmp_otpdata_size & 0x00ff;
                    otpdatalength[1] = (tmp_otpdata_size & 0xff00)>>8;
                        
                    if(tmp_otpdata_size > 0){
                        OTP_LOGE("%s, read otp data is success size :%d", __FUNCTION__,tmp_otpdata_size);
                        memcpy(bufdata,OTPD_READ_RSP,OTP_CONTROL_CMD_SIZE);
                        memcpy(&bufdata[OTP_CONTROL_CMD_SIZE], &part_id,1);
                        memcpy(&bufdata[OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE],otpdatalength,OTP_LENGTH_SIZE);
                        memcpy(&bufdata[OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE+OTP_LENGTH_SIZE],otpdata,tmp_otpdata_size);
                        response_info_sockclients(bufdata, OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE+OTP_LENGTH_SIZE+tmp_otpdata_size);
                    }else{
                        OTP_LOGE("%s, read otp data is failed size:%d", __FUNCTION__,tmp_otpdata_size);
                        response_info_sockclients(OTPD_MSG_FAILED, sizeof(OTPD_MSG_FAILED));
                    }
                }else if (strstr(controlcmd, OTPD_INIT_GOLDEN_DATA)) {
                    //read file golden data  set to otp.txt and otp_bk.txt
                    ret = poweron_init_otp_golden_data(part_number);
                    if(ret){
                        OTP_LOGE("%s, otp golden data is ok", __FUNCTION__);
                        response_info_sockclients(OTPD_MSG_OK, sizeof(OTPD_MSG_OK));
                    }else{
                        OTP_LOGE("%s, read otp data is failed", __FUNCTION__);
                        response_info_sockclients(OTPD_MSG_FAILED, sizeof(OTPD_MSG_FAILED));
                    }
                }else if (strstr(controlcmd, OTPD_READ_GOLDEN_DATA)) {
                    //read file golden data  set to otp.txt and otp_bk.txt
                    memset(otpdata, 0, sizeof(otpdata));
                    tmp_otpdata_size = read_golden_otp_data(otpdata,RAMOTP_DIRTYTABLE_DEFAULTSIZE,part_number);
                    otpdatalength[0] = tmp_otpdata_size & 0x00ff;
                    otpdatalength[1] = (tmp_otpdata_size & 0xff00)>>8;

                    if(tmp_otpdata_size > 0){
                        OTP_LOGE("%s, read otp data is success size:%d", __FUNCTION__,tmp_otpdata_size);
                        memcpy(bufdata,OTPD_READ_RSP,OTP_CONTROL_CMD_SIZE);
                        memcpy(&bufdata[OTP_CONTROL_CMD_SIZE],&part_id,1);
                        memcpy(&bufdata[OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE],otpdatalength,OTP_LENGTH_SIZE);
                        memcpy(&bufdata[OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE+OTP_LENGTH_SIZE],otpdata,tmp_otpdata_size);
                        response_info_sockclients(bufdata, OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE+OTP_LENGTH_SIZE+tmp_otpdata_size);
                    }else{
                        OTP_LOGE("%s, read otp data is failed", __FUNCTION__);
                        response_info_sockclients(OTPD_MSG_FAILED, sizeof(OTPD_MSG_FAILED));
                    }
                    
                } else {
                    OTP_LOGE("%s, Don't support %s", __FUNCTION__,controlcmd);
                }
            }
            }
        }
    }
    close(s_notifypipe[0]);
    exit_otpd();
    return NULL;
}

void *otpd_listenaccept_thread(void) {
    int sfd, n, index;
    pthread_t tid;
    pthread_attr_t attr;
	
    for (index = 0; index < MAX_CLIENT_NUM; index++) {
        s_clientFd[index] = -1;
    }
    /* s_notifypipe[1]: used for notify;
     * s_notifypipe[0]: used for receiving notify,then update the listen fds
     */
    if (pipe(s_notifypipe) < 0) {
        OTP_LOGD("%s:pipe error!\n",__FUNCTION__);
    }

    sfd = socket_local_server(SOCKET_NAME_OTPD,
                              ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (sfd < 0) {
        OTP_LOGE("%s: cannot create local socket server", __FUNCTION__);
        return NULL;
    }
	
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid, &attr, (void*)otpd_listen_read_thread, NULL);
	
    for (;;) {
        OTP_LOGD("%s: Waiting for new connect ...", __FUNCTION__);
        if ((n = accept(sfd, NULL, NULL)) == -1) {
            OTP_LOGE("%s otp accept error\n", __FUNCTION__);
            continue;
        }
		
        OTP_LOGD("%s: accept client n=%d", __FUNCTION__, n);
        for (index = 0; index < MAX_CLIENT_NUM; index++) {
            if (s_clientFd[index] == -1) {
                s_clientFd[index] = n;
                write(s_notifypipe[1], OTPD_LISTEN_NOTIFY,
                       sizeof(OTPD_LISTEN_NOTIFY));
                OTP_LOGD("%s: fill%d to client[%d]", __FUNCTION__, n, index);
                break;
            }
            /* if client_fd arrray is full, just fill the new socket to the
             * last element */
            if (index == MAX_CLIENT_NUM - 1) {
                OTP_LOGD("%s: client full, just fill %d to client[%d]",
                            __FUNCTION__, n, index);
                close(s_clientFd[index]);
                s_clientFd[index] = n;
                write(s_notifypipe[1], "0", 2);
                break;
            }
        }
    }
    close(s_notifypipe[1]);
    return NULL;
}

