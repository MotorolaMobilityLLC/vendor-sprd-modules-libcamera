// log infor
#ifndef _UNNENGINE_LOG_H
#define _UNNENGINE_LOG_H
#define LOG_TAG "UNN_ENGINE"
#if (defined(ANDROID) || defined(__ANDROID__))
    #include <android/log.h>
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) // DEBUG
    #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__) // ERROR
#else
    #include <stdio.h>
    #define LOGD(...) printf(__VA_ARGS__)
    #define LOGE(...) printf(__VA_ARGS__)
#endif
#endif