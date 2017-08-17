#ifndef _s5k3l8xxm3_reachtech_GOLDEN_OTP_H_
#define _s5k3l8xxm3_reachtech_GOLDEN_OTP_H_

#include "otp_common.h"

static awb_target_packet_t golden_awb[AWB_MAX_LIGHT] = {
    {
        .R = 0x141,
        .G = 0x278,
        .B = 0x151,
        .rg_ratio = 0x104,
        .bg_ratio = 0x111,
        .GrGb_ratio = 0x200,
    },
};

static cmr_u8 golden_lsc[] = {};

#endif
