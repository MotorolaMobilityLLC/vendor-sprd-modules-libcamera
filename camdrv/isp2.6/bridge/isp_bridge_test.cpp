#define LOG_TAG "ispbr_test"

#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <log/log.h>

#include "isp_bridge.h"

static const int kDefaultThreadNum = 5;
static const int kDefaultLoopCount = 30;

int main(int argc, char **argv) {
    int threadNum = kDefaultThreadNum;
    int loopCount = kDefaultLoopCount;

    if (argc > 1) {
        threadNum = atoi(argv[1]);
        if (threadNum < 2)
            threadNum = kDefaultThreadNum;
    }

    if (argc > 2) {
        loopCount = atoi(argv[2]);
        if (loopCount < 1)
            loopCount = kDefaultLoopCount;
    }

    ALOGI("test start with %d thread and %d loop", threadNum, loopCount);

    std::vector<std::thread> threads;
    for (int i = 0; i < threadNum; i++)
        threads.push_back(
            std::thread([=](std::tuple<uint32_t, void *, bool> user) {
                std::stringstream s;
                s << std::this_thread::get_id();
                std::string id = s.str();

                uint32_t cameraId = std::get<0>(user);
                void *ispHandle = std::get<1>(user);
                bool isMaster = std::get<2>(user);

                ALOGI("thread(%s) starts with camera_id: %u, isp_handle %p, "
                      "is_master %d",
                      id.c_str(), cameraId, ispHandle, isMaster);

                int result = 0;
                for (int i = 0; i < loopCount; i++) {
                    ALOGI("thread(%s) in loop %d", id.c_str(), i);
                    result = isp_br_init(cameraId, ispHandle, isMaster);
                    if (result < 0) {
                        ALOGE("thread(%s) fail to call isp_br_init, ret %d",
                              id.c_str(), result);
                        continue;
                    }

                    uint32_t role = CAM_SENSOR_MAX;
                    result = isp_br_ioctrl(CAM_SENSOR_MASTER, GET_SENSOR_ROLE,
                                           &cameraId, &role);
                    if (result < 0) {
                        ALOGE("thread(%s) fail to call isp_br_ioctrl, ret %d",
                              id.c_str(), result);
                        goto deinit;
                    }
                    ALOGI("thread(%s) camera %u inited with role %u",
                          id.c_str(), cameraId, role);

                deinit:
                    result = isp_br_deinit(cameraId);
                    if (result < 0) {
                        ALOGE("thread(%s) fail to call isp_br_deinit, ret %d",
                              id.c_str(), result);
                    }
                }

                ALOGI("thread(%s) exits", id.c_str());
            }, std::make_tuple(i, reinterpret_cast<void *>(i), !i)));

    for (auto &t : threads)
        t.join();

    ALOGI("test finished");

    return 0;
}
