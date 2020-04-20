/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without *
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  *
*/

#include "SprdCamera3Factory.h"

static struct hw_module_methods_t methods = {
    .open = sprdcamera::SprdCamera3Factory::open,
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common =
        {
            .tag = HARDWARE_MODULE_TAG,
            .module_api_version = CAMERA_MODULE_API_VERSION_2_5,
            .hal_api_version = HARDWARE_HAL_API_VERSION,
            .id = CAMERA_HARDWARE_MODULE_ID,
            .name = "Sprd Camera HAL3",
            .author = "Spreadtrum Corporation",
            .methods = &methods,
            .dso = NULL,
        },
    .get_number_of_cameras =
        sprdcamera::SprdCamera3Factory::get_number_of_cameras,
    .get_camera_info = sprdcamera::SprdCamera3Factory::get_camera_info,
    .set_callbacks = sprdcamera::SprdCamera3Factory::set_callbacks,
    .get_vendor_tag_ops = sprdcamera::SprdCamera3Factory::get_vendor_tag_ops,
    .open_legacy = sprdcamera::SprdCamera3Factory::open_legacy,
    .set_torch_mode = sprdcamera::SprdCamera3Factory::set_torch_mode,
    .init = sprdcamera::SprdCamera3Factory::init,
    .get_physical_camera_info =
        sprdcamera::SprdCamera3Factory::get_physical_camera_info,
    .is_stream_combination_supported =
        sprdcamera::SprdCamera3Factory::is_stream_combination_supported,
    .notify_device_state_change =
        sprdcamera::SprdCamera3Factory::notify_device_state_change,
};
