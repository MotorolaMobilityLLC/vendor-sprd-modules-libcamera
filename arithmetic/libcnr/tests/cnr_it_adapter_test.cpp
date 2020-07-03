/**
 * Copyright (C) 2020 UNISOC Technologies Co.,Ltd.
 */

#include <gtest/gtest.h>

#include "sprd_yuv_denoise_adapter.h"

TEST(hdr, sprd_hdr_adpt_init_test) {
	sprd_yuv_denoise_init_t my_cnr_param = {128, 128, 0};
	EXPECT_EQ(NULL, sprd_yuv_denoise_adpt_init(NULL));
	EXPECT_NE((void *)NULL, sprd_yuv_denoise_adpt_init(&my_cnr_param));
}

