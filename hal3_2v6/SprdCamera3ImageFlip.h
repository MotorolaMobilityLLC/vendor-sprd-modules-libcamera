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
#ifndef __SPRDCAMERA_FLIP_H__
#define __SPRDCAMERA_FLIP_H__

namespace sprdcamera {

class SprdCamera3ImageFlip {
  public:
    static int firstRotateFlip;
    uint64_t ReverseBytes_y(uint64_t value);
    uint64_t ReverseBytes_uv(uint64_t value);
    void imageYuvMirror(uint8_t* yuv_temp,  int w, int h);
    void imageYuv180rotation(uint8_t* yuv_temp, int w, int h);
    void imageYuvFlip(uint8_t* yuv_temp, int w, int h);
};


}; // namespace sprdcamera

#endif