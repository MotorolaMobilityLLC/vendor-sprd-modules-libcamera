#ifndef _CMC637_TRULY_OTP_H_
#define _CMC637_TRULY_OTP_H_
#include "otp_common.h"

static awb_target_packet_t truly_awb[AWB_MAX_LIGHT] = {
    {
        // d65 light
        .R = 0x0187,
        .G = 0x02E0,
        .B = 0x01C5,
    },
    {
        // d65 light
        .R = 0x0222,
        .G = 0x02E0,
        .B = 0x0128,
    },

};
static uint16_t truly_lsc[] = {
0x9349, 0xe406, 0x3ce1, 0x4f32, 0x93cb, 0x98b2, 0x23b0, 0xc86c, 0xc207,
0x647e, 0xed1f, 0x0747, 0x8742, 0x23c8, 0x4990, 0xd297, 0x28b4, 0x0932,
0x070e, 0x2b64, 0x4379, 0x8eae, 0xe338, 0x38b7, 0x6929, 0x2a09, 0x8102,
0x1e60, 0x0748, 0x81c6, 0x6070, 0x4f1c, 0xea07, 0x8211, 0x22ec, 0x8972,
0x0294, 0x4cb7, 0x9533, 0x2d0e, 0xf3f4, 0x35b0, 0xcbc8, 0x829d, 0x1896,
0xd322, 0xd047, 0x6c81, 0x19bc, 0x4643, 0x518c, 0xe064, 0xd419, 0xd446,
0x7e71, 0x2268, 0x8972, 0xc29e, 0x24bb, 0x0a35, 0x89cf, 0xc6f3, 0x2bb4,
0x49b1, 0x822a, 0x8c7d, 0x8e1c, 0x8486, 0x5be1, 0x1630, 0x057d, 0x2164,
0x6c5c, 0x9818, 0xcd46, 0x7e51, 0x22e0, 0x89ac, 0xe2b7, 0x7cc4, 0x4a37,
0xec4d, 0xa382, 0x2434, 0x0814, 0xe1d1, 0xf068, 0x8317, 0x4bc5, 0x4fe1,
0x13b8, 0x84ff, 0x514b, 0x0458, 0x9a18, 0xd346, 0x81c1, 0x2414, 0x0a25,
0x72e2, 0x64cf, 0x0c32, 0x6c8b, 0x8922, 0x1e6c, 0x46d0, 0x8186, 0x3c58,
0xbd14, 0x2344, 0x47e1, 0x1238, 0x44be, 0x8144, 0x6c58, 0xd018, 0xe806,
0x88b1, 0x2678, 0x8aea, 0x0315, 0x98c2, 0x4b2a, 0x0d09, 0x7472, 0x19d8,
0x05c1, 0xd14c, 0xb44b, 0x4011, 0x0d04, 0x43f1, 0x11b4, 0x84bf, 0xf14b,
0xc45b, 0x3e19, 0x0a47, 0x9332, 0x29f4, 0x0bd6, 0xd2f6, 0x5ca6, 0xfd24,
0xc447, 0x63f1, 0x162c, 0x84f9, 0xd122, 0x5c43, 0x0c10, 0x0604, 0x43f1,
0x1230, 0xc4fc, 0x8161, 0x1863, 0xf31c, 0x3e47, 0xa392, 0x2e40, 0x4bb7,
0xb293, 0xa48f, 0x001f, 0x8ac7, 0x5771, 0x13a4, 0x8476, 0x810b, 0x0040,
0x0c10, 0x0c04, 0x47a1, 0x139c, 0x4573, 0xd188, 0x606e, 0xdf1f, 0x8708,
0xb7c2, 0x2f14, 0x0a54, 0x3242, 0x247f, 0x381c, 0x6046, 0x4f31, 0x1208,
0x0437, 0x7104, 0x4840, 0x3710, 0x20c4, 0x4f11, 0x15f4, 0xc629, 0xd1bd,
0x9c7d, 0x2723, 0xe0ca, 0xbfe2, 0x29fc, 0x4930, 0x0207, 0x7873, 0xad19,
0x46c5, 0x4aa1, 0x1174, 0x4433, 0x1109, 0x6c43, 0xa911, 0x4644, 0x5a61,
0x1948, 0xc722, 0x1202, 0x5491, 0xb029, 0x150b, 0xacd3, 0x25f4, 0xc869,
0xe1de, 0xfc6a, 0x6917, 0x3c85, 0x4a21, 0x11d0, 0x8466, 0x211c, 0xc84a,
0x6313, 0x7d45, 0x6a21, 0x1db0, 0x8856, 0x8259, 0x38aa, 0xea30, 0xd48c,
0x9ec2, 0x2350, 0xc7e6, 0xa1c7, 0x5866, 0x5c17, 0x4305, 0x4da1, 0x1338,
0x84d9, 0xe142, 0x4455, 0x6217, 0xc4c6, 0x7dc1, 0x2318, 0x09d2, 0x12cb,
0xb4c9, 0xff36, 0xa38b, 0x9692, 0x21c4, 0xc7a7, 0xf1bd, 0x9465, 0x9517,
0x5a05, 0x5551, 0x1594, 0x8591, 0x217a, 0xd066, 0xa21b, 0x1907, 0x9542,
0x29b8, 0x8bd5, 0x9355, 0x94ea, 0x5733, 0x85cb, 0x9172, 0x2118, 0xc797,
0x21c1, 0xf869, 0x0f18, 0x7f86, 0x60e1, 0x1900, 0x4694, 0xf1c3, 0x2479,
0x1521, 0x81c9, 0xb392, 0x32c0, 0x8e5e, 0xa405, 0x30df, 0xf631, 0x794a,
0x9092, 0x2160, 0x87c8, 0x11d5, 0x4870, 0xc51b, 0xb546, 0x7071, 0x1d58,
0x07cf, 0xe218, 0x8090, 0xe927, 0x0c4a, 0xdcb3, 0x3f70, 0x1255, 0xe3de,
0x50d7, 0xea30, 0x7e8a, 0x93e2, 0x22a0, 0x8833, 0x91f8, 0xb07b, 0xbb1e,
0xf947, 0x8391, 0x22b4, 0x8947, 0xe27f, 0xf4ad, 0x682f, 0xd78d, 0x2f23,
0x4d01, 0x8fe7, 0xa377, 0xdcc6, 0x3e2c, 0x5f4a, 0x8e12, 0x21b4, 0xc81c,
0x91fd, 0xd87e, 0x231f, 0x1e08, 0x8f02, 0x2620, 0x4a3f, 0x82ca, 0x20c5,
0xd437, 0x098f, 0x0005, 0x92ac, 0xc3eb, 0x78dc, 0x3831, 0x930b, 0x98c2,
0x23bc, 0x0883, 0xe20b, 0xc47f, 0x031f, 0x0bc8, 0x8862, 0x23f4, 0xc996,
0x1296, 0xa0b4, 0xc731, 0xf40d, 0x2743, 0x4115, 0x4e35, 0xc322, 0xdcb3,
0x5628, 0x2989, 0x8182, 0x1ea4, 0x0757, 0x91cb, 0xac71, 0x5f1c, 0xed07,
0x8281, 0x22dc, 0x0969, 0x4291, 0x94b5, 0x4e32, 0x1b0e, 0xecf4, 0x33f8,
0x0b7f, 0xd291, 0xf894, 0xd321, 0xd287, 0x6d71, 0x1a20, 0x4657, 0x7191,
0x2c65, 0xe11a, 0xd506, 0x7ea1, 0x2248, 0x8961, 0xa298, 0x60b9, 0xda34,
0x6b0e, 0xc0f3, 0x2a8c, 0x897e, 0xa224, 0x807c, 0x961c, 0x8906, 0x5d41,
0x167c, 0x0591, 0x5169, 0xac5d, 0xa018, 0xcd06, 0x7e31, 0x22a4, 0xc994,
0x22af, 0xa4c2, 0xd036, 0xd24c, 0x9f22, 0x236c, 0xc7f2, 0xa1cc, 0x0468,
0x9218, 0x5045, 0x50f1, 0x13f4, 0x050f, 0x1150, 0x1859, 0x9b18, 0xd2c6,
0x8111, 0x23cc, 0xca0e, 0xe2d6, 0xa4cc, 0xb630, 0x5b0a, 0x85c2, 0x1df4,
0x06bc, 0xc185, 0x7c58, 0xce14, 0x2744, 0x48d1, 0x1274, 0x84cd, 0x0147,
0x8459, 0xcf18, 0xe606, 0x87a1, 0x2614, 0x8aca, 0x7308, 0x28bb, 0x0e29,
0x00c9, 0x7252, 0x1990, 0x05bb, 0x914d, 0xec4c, 0x4d11, 0x1004, 0x44f1,
0x11f0, 0xc4c9, 0x614d, 0xcc5c, 0x3d19, 0x0747, 0x9262, 0x2974, 0xcbba,
0xc2db, 0x3ca0, 0xc923, 0xbbc7, 0x6271, 0x1608, 0x44fa, 0x8124, 0x8044,
0x1210, 0x0844, 0x4481, 0x1248, 0x44fd, 0x5162, 0x0863, 0xe31c, 0x3947,
0xa172, 0x2d84, 0x0b4d, 0x427b, 0xcc8b, 0xd71e, 0x8486, 0x5681, 0x1388,
0xc479, 0xc10c, 0x0040, 0x0d10, 0x0d04, 0x47b1, 0x1390, 0x856d, 0x7186,
0x246e, 0xc81f, 0x7d88, 0xb452, 0x2d3c, 0x89f4, 0xc22d, 0x807b, 0x171b,
0x5b86, 0x4ea1, 0x120c, 0xc43b, 0x5104, 0x4840, 0x3810, 0x2084, 0x4ec1,
0x15d4, 0x8620, 0xc1ba, 0x3c7c, 0xfa23, 0xd249, 0xb852, 0x285c, 0x48dc,
0x41f7, 0x0070, 0x9919, 0x4505, 0x4ae1, 0x1180, 0x4434, 0x0109, 0x7043,
0xa711, 0x4404, 0x59b1, 0x1910, 0x870a, 0x01fb, 0x948f, 0x7c28, 0xf64b,
0xa672, 0x249c, 0x8825, 0xd1d2, 0xa068, 0x6017, 0x3cc5, 0x4a51, 0x11d4,
0x8464, 0xf11c, 0xac49, 0x5613, 0x7885, 0x68c1, 0x1d58, 0xc835, 0xc24d,
0x0ca6, 0x502f, 0xb68c, 0x9922, 0x222c, 0x87b3, 0x41bd, 0x2065, 0x5717,
0x4185, 0x4d31, 0x1310, 0x44d2, 0x0140, 0xfc55, 0x4b16, 0xbd86, 0x7b71,
0x2258, 0x09a1, 0x32b8, 0x14c4, 0x8334, 0x890b, 0x9152, 0x20c8, 0xc77a,
0x81b5, 0x6464, 0x8817, 0x55c5, 0x5461, 0x1554, 0x8582, 0x5174, 0x4464,
0x781b, 0x0ec7, 0x91d2, 0x28b4, 0xcb82, 0xa33c, 0x50df, 0xe431, 0x704a,
0x8d02, 0x2048, 0x476e, 0xa1ba, 0xac67, 0xfa18, 0x7a05, 0x5f51, 0x1894,
0x4675, 0x01b9, 0x4877, 0xd520, 0x7008, 0xae52, 0x3128, 0x0de8, 0x63d9,
0x34d5, 0x8a2f, 0x63ca, 0x8c72, 0x20a0, 0x07a7, 0x81ce, 0xe06e, 0xa71a,
0xad46, 0x6e51, 0x1ccc, 0x07a0, 0xb20a, 0x588c, 0x9326, 0xf1ca, 0xd482,
0x3d24, 0x5180, 0x63af, 0x60ce, 0x8b2e, 0x6c8a, 0x9032, 0x21dc, 0x0809,
0x21f0, 0x1879, 0x931e, 0xef87, 0x80a1, 0x21d4, 0xc905, 0x926c, 0x60a8,
0xee2e, 0xb24c, 0x22e3, 0x49e1, 0xcf2e, 0xc352, 0x6cbe, 0xf72b, 0x5009,
0x8b72, 0x2110, 0xc7fa, 0x11f2, 0x3c7c, 0xfd1f, 0x1247, 0x8ba2, 0x251c,
0xc9f7, 0x92b4, 0x0cbe, 0x3635, 0xe0cf, 0x0004, 0x92c7, 0x53ec, 0x1cdb,
0x1d31, 0x8bcb, 0x9702, 0x2370, 0x486a, 0xa206, 0x8c7e, 0xf91f, 0x0a87,
0x87d2, 0x23c4, 0xc98c, 0x4293, 0x84b3, 0xcc31, 0xf10d, 0x2963, 0x4165,
0x0e3c, 0x2322, 0x78b3, 0x4128, 0x22c9, 0x8022, 0x1e44, 0x0744, 0xa1c6,
0x7c70, 0x571c, 0xea47, 0x81f1, 0x22ac, 0xc95f, 0xd28e, 0x88b4, 0x4c32,
0x1bce, 0xee04, 0x3444, 0x4b81, 0xd28f, 0xb493, 0xbe21, 0xcd07, 0x6c01,
0x19cc, 0xc646, 0xb18d, 0x1064, 0xd91a, 0xd3c6, 0x7df1, 0x2228, 0x8955,
0x6296, 0x68b9, 0xdf34, 0x728e, 0xc1d3, 0x2aac, 0xc97a, 0xb221, 0x487b,
0x851c, 0x8446, 0x5c21, 0x1648, 0x8587, 0xf166, 0x945c, 0x9a18, 0xcac6,
0x7d91, 0x228c, 0x0997, 0xc2af, 0x00c2, 0xf337, 0xd80c, 0xa002, 0x236c,
0xc7f3, 0xe1ca, 0xc867, 0x8417, 0x4cc5, 0x5021, 0x13d0, 0x450a, 0x014f,
0x1459, 0x9618, 0xd246, 0x80e1, 0x23c8, 0x8a0e, 0xf2d9, 0x30cd, 0xca31,
0x5e8a, 0x8682, 0x1df4, 0x86b9, 0x1182, 0x4858, 0xc514, 0x2544, 0x4881,
0x126c, 0x84cb, 0xd147, 0x7458, 0xce18, 0xe646, 0x87b1, 0x2620, 0x0ace,
0xd30e, 0x98bd, 0x1a29, 0x0289, 0x72a2, 0x1980, 0x85b4, 0xf14a, 0xd04b,
0x4711, 0x0f04, 0x44d1, 0x11f4, 0x44cc, 0x614e, 0xcc5c, 0x3d19, 0x0707,
0x9262, 0x2994, 0x4bc4, 0x42e5, 0x80a2, 0xd323, 0xbd07, 0x6281, 0x15f4,
0xc4f5, 0x3122, 0x7044, 0x1010, 0x0844, 0x44d1, 0x125c, 0xc503, 0x8162,
0x0863, 0xe31c, 0x3a07, 0xa222, 0x2de8, 0x4b78, 0x7281, 0x008c, 0xde1f,
0x8506, 0x5671, 0x1378, 0x8473, 0x910b, 0x0040, 0x0f10, 0x0e44, 0x4821,
0x13a8, 0xc573, 0x8187, 0x346e, 0xcb1f, 0x8148, 0xb5a2, 0x2dfc, 0xca0c,
0xa232, 0x947c, 0x1c1b, 0x5c06, 0x4e71, 0x11f8, 0x4438, 0x7104, 0x5440,
0x4010, 0x2204, 0x4f41, 0x15f0, 0xc628, 0x21bc, 0x4c7d, 0x0823, 0xd60a,
0xbac2, 0x28f0, 0x48f1, 0x11fb, 0x1c71, 0x9b19, 0x44c5, 0x4a81, 0x1178,
0x4432, 0x5109, 0x8843, 0xae11, 0x4644, 0x5a31, 0x1930, 0xc712, 0xb1fd,
0xd48f, 0x8c28, 0x00cb, 0xa883, 0x24f4, 0x0836, 0x21d6, 0xa869, 0x5e17,
0x3c05, 0x4a31, 0x11cc, 0x4465, 0x611d, 0xcc4a, 0x6113, 0x7b05, 0x6951,
0x1d88, 0xc840, 0x4251, 0x94a8, 0x8e2f, 0xbfcc, 0x9ac2, 0x226c, 0xc7c0,
0x21bf, 0x2065, 0x5817, 0x41c5, 0x4d61, 0x131c, 0x44d7, 0xa142, 0x2855,
0x5617, 0xc046, 0x7c41, 0x22a0, 0xc9b5, 0xc2bf, 0xe8c6, 0xa734, 0x900b,
0x9262, 0x2114, 0x8781, 0xa1b6, 0x6064, 0x8817, 0x55c5, 0x54a1, 0x1564,
0x858a, 0x2177, 0x7065, 0x841b, 0x1207, 0x92e2, 0x2900, 0xcb9d, 0x8345,
0xfce3, 0x0231, 0x75cb, 0x8e22, 0x2070, 0xc771, 0xb1ba, 0xb067, 0xfb18,
0x7ac5, 0x5fb1, 0x18b4, 0x0681, 0xe1bd, 0x8877, 0xe720, 0x74c8, 0xb002,
0x31a0, 0x8e0f, 0x63e3, 0xc0d8, 0xa02f, 0x680a, 0x8d52, 0x20ac, 0x07a9,
0x91cf, 0xf06e, 0xa91a, 0xaf06, 0x6ec1, 0x1cfc, 0x07ac, 0x220e, 0xb08e,
0xad26, 0xf88a, 0xd732, 0x3de0, 0x11b5, 0xb3b9, 0xc4d0, 0x9d2e, 0x6f0a,
0x9072, 0x21dc, 0xc809, 0x21ef, 0x2479, 0x991e, 0xf287, 0x8151, 0x220c,
0x4915, 0x8270, 0xb0a9, 0x092e, 0xb94d, 0x2793, 0x4ab1, 0x4f52, 0x035a,
0x84c0, 0xfb2b, 0x50c9, 0x8b92, 0x2120, 0x0800, 0x61f4, 0x707c, 0x041f,
0x1408, 0x8c22, 0x254c, 0xca03, 0xa2b8, 0xa0bf, 0x6535, 0xefcf, 0x0004,
0x11e8, 0xf3c3, 0x2cd2, 0xae2f, 0x73ca, 0x9282, 0x226c, 0x0830, 0x11f9,
0xc07c, 0xc21e, 0xfb07, 0x83a1, 0x229c, 0x0932, 0xc279, 0x60ab, 0x2f2f,
0xc6cd, 0x1d63, 0x3e45, 0xcd74, 0xb2f6, 0x80a9, 0xd026, 0x0c48, 0x7b52,
0x1d40, 0x8707, 0x31b7, 0x8c6d, 0x0f1b, 0xd747, 0x7c91, 0x2128, 0x48ed,
0x626d, 0xb0ab, 0x822f, 0xe3cd, 0xe143, 0x316c, 0x0ae1, 0xc26f, 0x308c,
0x6f20, 0xbcc7, 0x68f1, 0x1918, 0xc61d, 0xb182, 0x3861, 0x9c19, 0xc146,
0x7891, 0x2098, 0xc8e0, 0xc274, 0x88af, 0x1e31, 0x468e, 0xb803, 0x2868,
0xc8ff, 0x5208, 0x2c76, 0x4f1b, 0x7946, 0x59e1, 0x15c0, 0x4566, 0x515d,
0xcc5a, 0x5c17, 0xb8c6, 0x7831, 0x20e0, 0x0924, 0x228a, 0x40b8, 0x5334,
0xb1cc, 0x97f2, 0x21d4, 0x4798, 0x41b8, 0x1464, 0x5f17, 0x4685, 0x4ea1,
0x1370, 0xc4eb, 0x3146, 0x3856, 0x5a17, 0xbf06, 0x7b51, 0x2230, 0xc991,
0xf2b5, 0xc0c3, 0x422e, 0x434a, 0x8082, 0x1cc0, 0x067b, 0xd177, 0xe455,
0xb213, 0x2184, 0x4781, 0x1218, 0x04b1, 0x213f, 0x9c56, 0x8c17, 0xd2c6,
0x8201, 0x2460, 0x8a47, 0xc2e9, 0x84b4, 0xb327, 0xed08, 0x6e31, 0x18b4,
0xc58d, 0xe143, 0xa84a, 0x3f11, 0x0cc4, 0x4401, 0x11a8, 0x44b1, 0x5145,
0xe059, 0xfa18, 0xf386, 0x8c21, 0x27b4, 0x8b3b, 0xd2c1, 0xec9a, 0x8421,
0xac87, 0x5f91, 0x1570, 0x84de, 0xc11f, 0x5c43, 0x0c10, 0x0644, 0x43e1,
0x1208, 0xc4e2, 0x5158, 0x1c60, 0x9b1b, 0x2307, 0x9ab2, 0x2bb8, 0xcae6,
0x5263, 0xc886, 0xa21d, 0x7906, 0x5461, 0x1324, 0xc46b, 0x810a, 0x0040,
0x0b10, 0x0b84, 0x46f1, 0x1344, 0xc54f, 0xf17c, 0x046a, 0x771e, 0x6408,
0xad62, 0x2bbc, 0x8991, 0x9219, 0xa477, 0xee1a, 0x53c5, 0x4d61, 0x11d8,
0x4436, 0x7104, 0x4840, 0x3710, 0x1ec4, 0x4db1, 0x1568, 0x45f7, 0xa1ac,
0xec78, 0x9b21, 0xb989, 0xb202, 0x26e8, 0x4891, 0xf1e7, 0x506c, 0x7618,
0x3e45, 0x49d1, 0x1164, 0x8432, 0x2109, 0x6443, 0x9c11, 0x3ec4, 0x57a1,
0x1860, 0x06d4, 0xe1eb, 0x0889, 0x1227, 0xdb4b, 0xa062, 0x2350, 0xc7e1,
0xa1c3, 0x0065, 0x4317, 0x3885, 0x49c1, 0x11cc, 0x0460, 0x611b, 0x6c49,
0x3f13, 0x6f85, 0x6621, 0x1c78, 0x87f2, 0xa23a, 0x78a0, 0xec2d, 0x9e8b,
0x9382, 0x20fc, 0x4776, 0x41b0, 0x9462, 0x3d16, 0x3d85, 0x4c91, 0x12e8,
0x84c5, 0xe13c, 0x8c53, 0x2216, 0xb046, 0x77a1, 0x213c, 0x8950, 0xc29f,
0x58bd, 0x1432, 0x734b, 0x8c12, 0x1fb8, 0x873b, 0x91a7, 0xc061, 0x6816,
0x4f05, 0x52e1, 0x1500, 0x856a, 0xd16c, 0x7061, 0x3b1a, 0xfb87, 0x8cd1,
0x2740, 0x8b14, 0xc31e, 0xa8d8, 0x7f2f, 0x594a, 0x87c2, 0x1f08, 0xc725,
0x91aa, 0x0c64, 0xd818, 0x7105, 0x5d21, 0x17f8, 0x464c, 0x11ac, 0x2c73,
0x831f, 0x5908, 0xa7a2, 0x2f3c, 0xcd6c, 0x93b7, 0x58cd, 0x1b2d, 0x480a,
0x8662, 0x1f64, 0x875f, 0x31bf, 0x206b, 0x741a, 0xa146, 0x6b21, 0x1bec,
0x4766, 0x71f8, 0xe887, 0x2624, 0xd34a, 0xcc62, 0x3b24, 0x10f3, 0xc382,
0x54c4, 0x192c, 0x51ca, 0x8a42, 0x2098, 0x87c8, 0x41df, 0x2075, 0x581d,
0xe087, 0x7c71, 0x209c, 0x08b2, 0xa254, 0x74a1, 0x5d2c, 0x8b4c, 0x1a93,
0x4731, 0x4e6a, 0x532d, 0x78b7, 0x8a29, 0x3949, 0x85e2, 0x2004, 0x07b9,
0x81e3, 0x6078, 0xbc1e, 0x0047, 0x8632, 0x23c0, 0xc98b, 0xc299, 0xc8b6,
0xa632, 0xc0ce, 0x0004};

#endif
