/**
 * otpd.h ---
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#ifndef OTPD_H_
#define OTPD_H_

#define MAX_CLIENT_NUM                      			20
#define OTP_CONTROL_CMD_SIZE               	          20
#define OTP_CONTROL_DATA_SIZE               	          10
#define OTP_LENGTH_SIZE                				2
#define OTP_DATA_SIZE              					RAMOTP_DIRTYTABLE_MAXSIZE
#define BUFFER_SIZE                OTP_CONTROL_CMD_SIZE+OTP_CONTROL_DATA_SIZE+OTP_LENGTH_SIZE+OTP_DATA_SIZE //save 254 byte otp data

#define OTPD_LISTEN_NOTIFY                                       "0"
#define SOCKET_NAME_OTPD                                         "otpd"

#define OTPD_WRITE_DATA                                              "Otp Write Data"
#define OTPD_READ_DATA                                                 "Otp Read Data"
#define OTPD_READ_GOLDEN_DATA                          "Otp Read Golden Data"
#define OTPD_INIT_GOLDEN_DATA                             "Otp Init Golden Data"
#define OTPD_READ_RSP                                                    "Otp Read Rsp"
#define OTPD_MSG_OK                                                        "Otp Data Ok"
#define OTPD_MSG_FAILED                                              "Otp Data Failed"
#define OTP_RADIO_TYPE                                                  "ro.vendor.camera.otptype"

extern char *s_otp;

void exit_otpd(void);
void *otpd_listenaccept_thread(void);
#endif  // OTPD_H_
