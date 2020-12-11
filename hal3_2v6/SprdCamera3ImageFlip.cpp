/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//add by chengl0724 feature for front camera buffer flip 20190114 start
#include <arm_neon.h>
#include <utils/Log.h>
#include "SprdCamera3ImageFlip.h"
#include "cmr_log.h"
namespace sprdcamera {


int SprdCamera3ImageFlip::firstRotateFlip = -1;

uint64_t SprdCamera3ImageFlip::ReverseBytes_y(uint64_t value)  //Reverse 8 Bytes of y component
{
    return (value & 0x00000000000000FFUL) << 56 |

        (value & 0x000000000000FF00UL) << 40 |

        (value & 0x0000000000FF0000UL) << 24 |

        (value & 0x00000000FF000000UL) << 8 |

        (value & 0x000000FF00000000UL) >> 8 |

        (value & 0x0000FF0000000000UL) >> 24 |

        (value & 0x00FF000000000000UL) >> 40 |

        (value & 0xFF00000000000000UL) >> 56;

}

uint64_t SprdCamera3ImageFlip::ReverseBytes_uv(uint64_t value)  //Reverse 8 Bytes of uv component
{
    return (value & 0x00000000000000FFUL) << 48 |

        (value & 0x000000000000FF00UL) << 48 |

        (value & 0x0000000000FF0000UL) << 16 |

        (value & 0x00000000FF000000UL) << 16 |

        (value & 0x000000FF00000000UL) >> 16 |

        (value & 0x0000FF0000000000UL) >> 16 |

        (value & 0x00FF000000000000UL) >> 48 |

        (value & 0xFF00000000000000UL) >> 48;

}
#if 0
void SprdCamera3ImageFlip::yuv_mirror(uint8_t* yuv_temp,  int w, int h) //for YUV420(NV21).It is need width_org align in 8Bytes
{
    int i, j;
    int a, b;
    uint64_t temp;
    uint64_t* yuv_add =NULL;
    int uindex = w*h/8;
    yuv_add = (uint64_t*)yuv_temp;
    CMR_LOGI("yuv_mirror start %s, %d",__FUNCTION__,__LINE__);
    //mirror y
    for (i = 0; i < h;i++)
    {
        a = i*w/8;
        b = (i + 1)*w/8 - 1;
        for (j = 0; j < w / 16;j++)
        {
            temp = ReverseBytes_y(yuv_add[a]);
            yuv_add[a] = ReverseBytes_y(yuv_add[b]);
            yuv_add[b] = temp;
            a++;
            b--;
        }
        if (a==b)
        {
            temp = ReverseBytes_y(yuv_add[a]);
            yuv_add[a] = temp;
        }
    }

    //mirror uv(NV21)
    yuv_add = yuv_add + uindex;
    for (i = 0; i < h / 2; i++)
    {
        a = i*w/8;
        b = (i + 1)*w/8 - 1;
        for (j = 0; j < w / 16; j++)
        {
            temp = ReverseBytes_uv(yuv_add[a]);
            yuv_add[a] = ReverseBytes_uv(yuv_add[b]);
            yuv_add[b] = temp;
            a++;
            b--;
        }
        if (a == b)
        {
            temp = ReverseBytes_uv(yuv_add[a]);
            yuv_add[a] = temp;
        }
    }
}
#endif
void SprdCamera3ImageFlip::imageYuvMirror(uint8_t* yuv_temp, int w, int h)
{
    int i, j;
    int a, b;
    uint8x8_t d[4];
    uint16x4_t q[4];
    uint16_t* uv_add = NULL;
    int uindex = w*h;
    CMR_LOGV("yuv_mirror %s, %d",__FUNCTION__,__LINE__);
    //mirror y
    for (i = 0; i < h; i++)
    {
        a = i*w;
        b = (i + 1)*w - 8;

        for (j = 0;j<w/16;j++)
        {
            d[0] = vld1_u8(yuv_temp + a);
            d[1] = vld1_u8(yuv_temp + b);
            d[2] = vrev64_u8(d[0]);
            d[3] = vrev64_u8(d[1]);
            vst1_u8(yuv_temp + b,d[2]);
            vst1_u8(yuv_temp + a,d[3]);
            a += 8;
            b -= 8;
        }
        if(a==b)   //8byte align
        {
            d[0] = vld1_u8(yuv_temp + a);
            d[2] = vrev64_u8(d[0]);
            vst1_u8(yuv_temp + a,d[2]);
        }
    }

    //mirror uv(NV21)
    uv_add = (uint16_t*) (yuv_temp+uindex);
    for (i = 0; i < h / 2; i++)
    {
        a = i*w/2;
        b = (i + 1)*w/2 - 4;
        for (j = 0;j<w/16;j++)
        {
            q[0] = vld1_u16(uv_add + a );
            q[1] = vld1_u16(uv_add + b );
            q[2] = vrev64_u16(q[0]);
            q[3] = vrev64_u16(q[1]);
            vst1_u16(uv_add + b,q[2]);
            vst1_u16(uv_add + a,q[3]);
            a += 4;
            b -= 4;
        }
        if(a==b)//8byte align
        {
            q[0] = vld1_u16(uv_add + a);
            q[2] = vrev64_u16(q[0]);
            vst1_u16(uv_add + a,q[2]);
        }
    }
}

/*this is neon c function for YUV420(NV21).It is need width_org align in 8Bytes*/
void SprdCamera3ImageFlip::imageYuv180rotation(uint8_t* yuv_temp, int w, int h)
{
    int i, j;
    int a, b;
    uint8x8_t d[4];
    uint16x4_t q[4];
    uint16_t* uv_add = NULL;
    int uindex = w*h;
    int uv_h =h/2;
    ALOGI("yuv_180rotation %s, %d",__FUNCTION__,__LINE__);
    //mirror y
    for (i = 0; i < h/2; i++)
    {
        a = i*w;
        b = (h-i) *w - 8;

        for (j = 0;j<w/8;j++)
        {
            d[0] = vld1_u8(yuv_temp + a);
            d[1] = vld1_u8(yuv_temp + b);
            d[2] = vrev64_u8(d[0]);
            d[3] = vrev64_u8(d[1]);
            vst1_u8(yuv_temp + b,d[2]);
            vst1_u8(yuv_temp + a,d[3]);
            a += 8;
            b -= 8;
        }
    }

    //mirror uv(NV21)
    uv_add = (uint16_t*) (yuv_temp+uindex);
    for (i = 0; i < uv_h/2; i++)
    {
        a = i*w/2;
        b = (uv_h - i)*w/2 - 4;
        for (j = 0;j<w/8;j++)
        {
            q[0] = vld1_u16(uv_add + a );
            q[1] = vld1_u16(uv_add + b );
            q[2] = vrev64_u16(q[0]);
            q[3] = vrev64_u16(q[1]);
            vst1_u16(uv_add + b,q[2]);
            vst1_u16(uv_add + a,q[3]);
            a += 4;
            b -= 4;
        }
    }
}

void SprdCamera3ImageFlip::imageYuvFlip(uint8_t* yuv_temp, int w, int h)
{
    int i, j;
    int a, b;
    uint8x8_t d[4];
    uint16x4_t q[4];
    uint16_t* uv_add = NULL;
    int uindex = w*h;
    int uv_h =h/2;
    CMR_LOGV("yuv_flip %s, %d",__FUNCTION__,__LINE__);
    //mirror y
    for (i = 0; i < h/2; i++)
    {
        a = i*w;
        b = (h-i-1) *w;

        for (j = 0;j<w/8;j++)
        {
            d[0] = vld1_u8(yuv_temp + a);
            d[1] = vld1_u8(yuv_temp + b);
            //d[2] = vrev64_u8(d[0]);
           // d[3] = vrev64_u8(d[1]);
            vst1_u8(yuv_temp + b,d[0]);
            vst1_u8(yuv_temp + a,d[1]);
            a += 8;
            b += 8;
        }
    }

    //mirror uv(NV21)
    uv_add = (uint16_t*) (yuv_temp+uindex);
    for (i = 0; i < uv_h/2; i++)
    {
        a = i*w/2;
        b = (uv_h - i -1)*w/2;
        for (j = 0;j<w/8;j++)
        {
            q[0] = vld1_u16(uv_add + a );
            q[1] = vld1_u16(uv_add + b );
           // q[2] = vrev64_u16(q[0]);
           // q[3] = vrev64_u16(q[1]);
            vst1_u16(uv_add + b,q[0]);
            vst1_u16(uv_add + a,q[1]);
            a += 4;
            b += 4;
        }
    }
}

}; // namespace sprdcamera
