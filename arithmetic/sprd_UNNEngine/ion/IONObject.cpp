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

#define LOG_TAG "IONObject"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
//#include <cutils/atomic.h>
//#include <cutils/log.h>
//#include <android/log.h>
#include "ion.h"
#include "ion_4.12.h"
#include <linux/dma-buf.h>
#include "sprd_ion.h"
#include "IONObject.h"

//using namespace std;

// log infor
#define LOG_TAG "IONOBJECT"
#if (defined(ANDROID) || defined(__ANDROID__))
    #include <android/log.h>
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) // DEBUG
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__) // ERROR
#else
    #include <stdio.h>
    #define LOGD(...) printf(__VA_ARGS__)
    #define LOGE(...) printf(__VA_ARGS__)
#endif

namespace android {

bool IONObject::Is_ion_legacy(int fd) {
    struct ion_new_allocation_data data;

    if (fd < 0) {
      LOGE("%s:open dev ion error!", __func__);
      return true;
    }

    int err = ioctl(fd, ION_IOC_VERSION, &data);
    return (err == 0) ? false : true;
}

bool IONObject::is_ion_legacy() {
    struct ion_new_allocation_data data;

    int err = ioctl(mIonDeviceFd, ION_IOC_VERSION, &data);
    return (err == 0) ? false : true;
}

IONObject::IONObject(const char *device, size_t size, uint32_t flags,
               unsigned int memory_type)
               : mIonDeviceFd(-1),
                 mIonHandle(-1),
                 mFD(-1),
                 mSize(0),
                 mBase(NULL) {
  int open_flags = O_RDONLY;
  if (flags & NO_CACHING) open_flags |= O_SYNC;

  int fd = open(device, open_flags);
  if (fd >= 0) {
    const size_t pagesize = getpagesize();
    size = ((size + pagesize - 1) & ~(pagesize - 1));
    mapIonFd(fd, size, memory_type, flags);
  } else {
    LOGE("%s, open ion fail, %d(%s)", __func__, -errno, strerror(errno));
  }
}

int IONObject::mapIonFd(int fd, size_t size, unsigned int memory_type,
                          int uflags) {
  if (Is_ion_legacy(fd)) {
    struct ion_allocation_data data;
    struct ion_fd_data fd_data;
    struct ion_handle_data handle_data;
    void *base = NULL;

    data.len = size;
    data.align = getpagesize();
    data.heap_id_mask = memory_type;
    fd_data.fd = -1;
    fd_data.handle = -1;
    // if cached buffer , force set the lowest two bits 11
    if ((memory_type & (1 << 31))) {
      data.flags = ((memory_type & (1 << 31)) | 3);
    } else {
      data.flags = 0;
    }

    if (memory_type & ION_FLAG_NO_CLEAR) {
      data.flags |= ION_FLAG_NO_CLEAR;
    }

    if (ioctl(fd, ION_IOC_ALLOC, &data) < 0) {
      LOGE("%s: ION_IOC_ALLOC error: %d (%s)", __func__, -errno, strerror(errno));
      close(fd);
      return -errno;
    }

    if ((uflags & DONT_MAP_LOCALLY) == 0) {
      int flags = 0;

      fd_data.handle = data.handle;

      if (ioctl(fd, ION_IOC_SHARE, &fd_data) < 0) {
        LOGE("%s: ION_IOC_SHARE error: %d (%s)", __func__, -errno, strerror(errno));
        handle_data.handle = data.handle;
        ioctl(fd, ION_IOC_FREE, &handle_data);
        close(fd);
        return -errno;
      }

      base = (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED | flags,
                           fd_data.fd, 0);
      if (base == MAP_FAILED) {
        LOGE("mmap(fd=%d, size=%u) failed (%s)", fd, uint32_t(size), strerror(errno));
        handle_data.handle = data.handle;
        ioctl(fd, ION_IOC_FREE, &handle_data);
        close(fd_data.fd);
        close(fd);
        return -errno;
      }
    }
    mIonHandle = data.handle;
    mIonDeviceFd = fd;
    mFD = fd_data.fd;
    mSize = size;
    mBase = base;
  }else {
    struct ion_new_allocation_data data;
    void *base = NULL;

    data.len = size;
    data.heap_id_mask = memory_type;

    // if cached buffer , force set the lowest two bits 11
    if ((memory_type & (1 << 31))) {
      data.flags = ((memory_type & (1 << 31)) | 3);
    } else {
      data.flags = 0;
    }

    if (memory_type & ION_FLAG_NO_CLEAR) {
      data.flags |= ION_FLAG_NO_CLEAR;
    }

    if (ioctl(fd, ION_IOC_NEW_ALLOC, &data) < 0) {
      LOGE("%s: ION_IOC_ALLOC error: %d (%s)", __func__, -errno, strerror(errno));
      close(fd);
      return -errno;
    }

    base = (uint8_t *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                          data.fd, 0);
    if (base == MAP_FAILED) {
      LOGE("mmap(fd=%d, size=%u) failed (%s)", data.fd, uint32_t(size), strerror(errno));
      close(data.fd);
      close(fd);
      return -errno;
    }

    mIonDeviceFd = fd;
    mFD = data.fd;
    mSize = size;
    mBase = base;
  }
  return MEMION_NO_ERROR;
}

IONObject::~IONObject() {
  struct ion_handle_data data;

  data.handle = mIonHandle;

  munmap(mBase, mSize);
  if (mIonDeviceFd > 0) {
    if (is_ion_legacy()) {
      if (ioctl(mIonDeviceFd, ION_IOC_FREE, &data) < 0)
        LOGE("%s: ION_IOC_FREE error: %d (%s)", __func__, -errno, strerror(errno));
        ;
    }
    close(mIonDeviceFd);
  }

  //int fd = android_atomic_or(-1, &mFD);
  int fd = std::atomic_fetch_or(&mFD, -1);
  if (fd >= 0) {
    mSize = 0;
    mBase = NULL;
    close(fd);
  }
}

int IONObject::getHeapID() const { return mFD; }

void *IONObject::getBase() const { return mBase; }

int IONObject::getIonDeviceFd() const { return mIonDeviceFd; }

int IONObject::Get_phy_addr_from_ion(int buffer_fd, unsigned long *phy_addr,
                                  size_t *size) {
  int fd = open("/dev/ion", O_SYNC);
  if (fd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (Is_ion_legacy(fd)) {
      struct ion_phys_data phys_data;
      struct ion_custom_data custom_data;
      phys_data.fd_buffer = buffer_fd;
      custom_data.cmd = ION_SPRD_CUSTOM_PHYS;
      custom_data.arg = (unsigned long)&phys_data;
      ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
      *phy_addr = phys_data.phys;
      *size = phys_data.size;
    } else {
      struct ion_phy_data phy_data;
      phy_data.fd = buffer_fd;
      ret = ioctl(fd, ION_IOC_PHY, &phy_data);
      *phy_addr = phy_data.addr;
      *size = phy_data.len;
    }
    close(fd);
    if (ret) {
      LOGE("%s: Getphyaddr error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::get_phy_addr_from_ion(unsigned long *phy_addr, size_t *size) {
  if (mIonDeviceFd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (is_ion_legacy()) {
      struct ion_phys_data phys_data;
      struct ion_custom_data custom_data;
      phys_data.fd_buffer = mFD;
      custom_data.cmd = ION_SPRD_CUSTOM_PHYS;
      custom_data.arg = (unsigned long)&phys_data;
      ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
      *phy_addr = phys_data.phys;
      *size = phys_data.size;
    } else {
      struct ion_phy_data phy_data;
      phy_data.fd = mFD;
      ret = ioctl(mIonDeviceFd, ION_IOC_PHY, &phy_data);
      *phy_addr = phy_data.addr;
      *size = phy_data.len;

    }
    if (ret) {
      LOGE("%s: getphyaddr error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::Flush_ion_buffer(int buffer_fd, void *v_addr, void *p_addr,
                             size_t size) {
  int fd = open("/dev/ion", O_SYNC);
  if (fd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (Is_ion_legacy(fd)) {
      struct ion_msync_data msync_data;
      struct ion_custom_data custom_data;

      msync_data.fd_buffer = buffer_fd;
      msync_data.vaddr = (unsigned long)v_addr;
      msync_data.paddr = (unsigned long)p_addr;
      msync_data.size = size;
      custom_data.cmd = ION_SPRD_CUSTOM_MSYNC;
      custom_data.arg = (unsigned long)&msync_data;
      ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
    } else {
      struct dma_buf_sync sync_data;
      sync_data.flags = DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END;
      ret = ioctl(buffer_fd, DMA_BUF_IOCTL_SYNC, &sync_data);
    }
    close(fd);
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::Invalid_ion_buffer(int buffer_fd) {
  int fd = open("/dev/ion", O_SYNC);
  if (fd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (Is_ion_legacy(fd)) {
      struct ion_custom_data custom_data;

      custom_data.cmd = ION_SPRD_CUSTOM_INVALIDATE;
      custom_data.arg = (unsigned long)buffer_fd;
      ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
    } else {
      struct dma_buf_sync sync_data;
      sync_data.flags = DMA_BUF_SYNC_READ;
      ret = ioctl(buffer_fd, DMA_BUF_IOCTL_SYNC, &sync_data);
    }
    close(fd);
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::flush_ion_buffer(void *v_addr, void *p_addr, size_t size) {
  if (mIonDeviceFd < 0) {
    return MEMION_NO_DEV;
  } else {
    int ret;

  if (is_ion_legacy()) {
    struct ion_msync_data msync_data;
    struct ion_custom_data custom_data;

    if (((unsigned char *)v_addr < (unsigned char *)mBase) ||
        ((unsigned char *)v_addr + size > (unsigned char *)mBase + mSize)) {
      LOGE("flush_ion_buffer error  mBase=0x%lx,mSize=0x%zx", (unsigned long)mBase, mSize);
      LOGE("flush_ion_buffer error  v_addr=0x%lx,p_addr=0x%lx,size=0x%zx", (unsigned long)v_addr, (unsigned long)p_addr, size);

      return -3;
    }
    msync_data.fd_buffer = mFD;
    msync_data.vaddr = (unsigned long)v_addr;
    msync_data.paddr = (unsigned long)p_addr;
    msync_data.size = size;
    custom_data.cmd = ION_SPRD_CUSTOM_MSYNC;
    custom_data.arg = (unsigned long)&msync_data;
    ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
  } else {
    struct dma_buf_sync sync_data;
    sync_data.flags = DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END;
    ret = ioctl(mFD, DMA_BUF_IOCTL_SYNC, &sync_data);
  }

  if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::invalid_ion_buffer() {
  if (mIonDeviceFd < 0) {
    return MEMION_NO_DEV;
  } else {
    int ret;
    if (is_ion_legacy()) {
      struct ion_custom_data custom_data;

      custom_data.cmd = ION_SPRD_CUSTOM_INVALIDATE;
      custom_data.arg = (unsigned long)mFD;
      ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
    } else {
      struct dma_buf_sync sync_data;
      sync_data.flags = DMA_BUF_SYNC_READ;
      ret = ioctl(mFD, DMA_BUF_IOCTL_SYNC, &sync_data);
    }
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::Sync_ion_buffer(int buffer_fd) {
  int fd = open("/dev/ion", O_SYNC);
  if (fd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (Is_ion_legacy(fd)) {
      struct ion_fd_data fd_data;

      fd_data.handle = -1;
      fd_data.fd = buffer_fd;
      ret = ioctl(fd, ION_IOC_SYNC, &fd_data);
    } else {
      struct dma_buf_sync sync_data;
      sync_data.flags = DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END;
      ret = ioctl(buffer_fd, DMA_BUF_IOCTL_SYNC, &sync_data);
    }
    close(fd);
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::sync_ion_buffer() {
  if (mIonDeviceFd < 0) {
    LOGE("%s:ion dev fd error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (is_ion_legacy()) {
      struct ion_fd_data fd_data;

      fd_data.handle = mIonHandle;
      fd_data.fd = mFD;
      ret = ioctl(mIonDeviceFd, ION_IOC_SYNC, &fd_data);
    } else {
      struct dma_buf_sync sync_data;
      sync_data.flags = DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END;
      ret = ioctl(mFD, DMA_BUF_IOCTL_SYNC, &sync_data);
    }
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }
  return MEMION_NO_ERROR;
}

int IONObject::Get_kaddr(int buffer_fd, uint64_t *kaddr, size_t *size) {
  int fd = open("/dev/ion", O_SYNC);

  if (fd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (Is_ion_legacy(fd)) {
      struct ion_kmap_data kmap_data;
      struct ion_custom_data custom_data;

      kmap_data.fd_buffer = buffer_fd;
      custom_data.cmd = ION_SPRD_CUSTOM_MAP_KERNEL;
      custom_data.arg = (unsigned long)&kmap_data;
      ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
      *kaddr = kmap_data.kaddr;
      *size = kmap_data.size;
    } else {
      LOGE("%s: not supported", __func__);
      return MEMION_NOT_SUPPORTED;
    }
    close(fd);
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }

  return MEMION_NO_ERROR;
}

int IONObject::get_kaddr(uint64_t *kaddr, size_t *size) {
  if (mIonDeviceFd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (is_ion_legacy()) {
      struct ion_kmap_data kmap_data;
      struct ion_custom_data custom_data;

      kmap_data.fd_buffer = mFD;
      custom_data.cmd = ION_SPRD_CUSTOM_MAP_KERNEL;
      custom_data.arg = (unsigned long)&kmap_data;
      ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
      *kaddr = kmap_data.kaddr;
      *size = kmap_data.size;
    } else {
      LOGE("%s: not supported", __func__);
      return MEMION_NOT_SUPPORTED;
    }
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }

  return MEMION_NO_ERROR;
}

int IONObject::Free_kaddr(int buffer_fd) {
  int fd = open("/dev/ion", O_SYNC);

  if (fd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (Is_ion_legacy(fd)) {
      struct ion_kunmap_data kunmap_data;
      struct ion_custom_data custom_data;

      kunmap_data.fd_buffer = buffer_fd;
      custom_data.cmd = ION_SPRD_CUSTOM_UNMAP_KERNEL;
      custom_data.arg = (unsigned long)&kunmap_data;
      ret = ioctl(fd, ION_IOC_CUSTOM, &custom_data);
    } else {
      LOGE("%s: not supported", __func__);
      return MEMION_NOT_SUPPORTED;
    }

    close(fd);
    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }

  return MEMION_NO_ERROR;
}

int IONObject::free_kaddr() {
  if (mIonDeviceFd < 0) {
    LOGE("%s:open dev ion error!", __func__);
    return MEMION_NO_DEV;
  } else {
    int ret;

    if (is_ion_legacy()) {
      struct ion_kunmap_data kunmap_data;
      struct ion_custom_data custom_data;

      kunmap_data.fd_buffer = mFD;
      custom_data.cmd = ION_SPRD_CUSTOM_UNMAP_KERNEL;
      custom_data.arg = (unsigned long)&kunmap_data;
      ret = ioctl(mIonDeviceFd, ION_IOC_CUSTOM, &custom_data);
    } else {
      LOGE("%s: not supported", __func__);
      return MEMION_NOT_SUPPORTED;
    }

    if (ret) {
      LOGE("%s: return error: %d", __func__, ret);
      return MEMION_OP_FAILED;
    }
  }

  return MEMION_NO_ERROR;
}

// ---------------------------------------------------------------------------
};  // namespace android
