/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __OIS_MAIN_H__
#define __OIS_MAIN_H__

uint32_t ois_pre_open(void);
uint32_t OpenOIS(void);
uint32_t CloseOIS(void);
uint32_t  SetOisMode(unsigned char mode);
unsigned char GetOisMode(void);
unsigned short  OisLensRead(unsigned short  cmd);
uint32_t OisLensWrite(unsigned short  cmd);
uint32_t OIS_write_af(uint32_t param);

#endif // __OIS_MAIN_H__

