#ifndef _OV8858_GOLDEN_OTP_H_
#define _OV8858_GOLDEN_OTP_H_
#include "otp_parse.h"

static awb_target_packet_t golden_awb[AWB_MAX_LIGHT] = 
{
	{//d65 light
		.R = 0x0000,
		.G = 0x0000,
		.B = 0x0000,
	},
};
static uint16_t golden_lsc[2] = {

};

#endif
