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
#ifndef _AWB_AL_CTRL_H_
#define _AWB_AL_CTRL_H_
/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

cmr_int awb_al_ctrl_init(void *param, void *result);
cmr_int awb_al_ctrl_deinit(void * handle, void *param, void *result);
cmr_int awb_al_ctrl_calculation(void * handle, void *param, void *result);
cmr_int awb_al_ctrl_ioctrl(void * handle, cmr_int cmd, void *param0, void *param1);

#endif
