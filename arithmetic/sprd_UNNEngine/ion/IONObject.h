/*
 * Copyright 2015 The Android Open Source Project
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

#ifndef _IONOBJECT_H_
#define _IONOBJECT_H_

#include <stdlib.h>
#include <stdint.h>
//#include <utils/SortedVector.h>
//#include <utils/threads.h>
#include <thread>
#include <atomic>

namespace android {

// ---------------------------------------------------------------------------

class IONObject {
 public:
  enum {
    READ_ONLY = 0x00000001,
    // memory won't be mapped locally, but will be mapped in the remote
    // process.
    DONT_MAP_LOCALLY = 0x00000100,
    NO_CACHING = 0x00000200
  };

  typedef enum {
    MEMION_NO_ERROR = 0,
    MEMION_NO_DEV = -1,
    MEMION_OP_FAILED = -2,
    MEMION_NOT_SUPPORTED = -3,
  } IONObjectRet;

  static bool Is_ion_legacy(int fd);
  bool is_ion_legacy();
  IONObject(const char *, size_t, uint32_t, unsigned int);
  ~IONObject();
  int getHeapID() const;
  void *getBase() const;
  int getIonDeviceFd() const;

  static int Get_phy_addr_from_ion(int fd, unsigned long *phy_addr,
                                   size_t *size);
  int get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size);
  static int Flush_ion_buffer(int buffer_fd, void *v_addr, void *p_addr,
                              size_t size);
  static int Invalid_ion_buffer(int buffer_fd);
  int flush_ion_buffer(void *v_addr, void *p_addr, size_t size);
  int invalid_ion_buffer();
  static int Sync_ion_buffer(int buffer_fd);
  int sync_ion_buffer();
  int Get_kaddr(int buffer_fd, uint64_t *kaddr, size_t *size);
  int get_kaddr(uint64_t *kaddr, size_t *size);

  int Free_kaddr(int buffer_fd);
  int free_kaddr();

 private:
  int mapIonFd(int fd, size_t size, unsigned int memory_type, int flags);

  int mIonDeviceFd; /*fd we get from open("/dev/ion")*/
  int mIonHandle;   /*handle we get from ION_IOC_ALLOC*/
  //int mFD;
  std::atomic<int32_t> mFD;
  size_t mSize;
  void *mBase;
};
// ---------------------------------------------------------------------------
};  // namespace android

#endif  // MEMION_H_
