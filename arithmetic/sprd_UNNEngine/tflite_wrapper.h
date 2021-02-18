/* Copyright wenguo.li@unisoc.com. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef _TFLITE_WRAPPER_H
#define _TFLITE_WRAPPER_H
#include <cstdint>
#include <cstdio>
#include <string>
#include "unn_engine.h"
#ifdef __cplusplus
extern "C" {
#endif

#define JNIEXPORT  __attribute__ ((visibility ("default")))

JNIEXPORT int TFLITEEngineInit(void **handle);
JNIEXPORT int TFLITEEngineDeInit(void *handle);
JNIEXPORT int TFLITEEngineCreateNetwork(void *handle, UNNModel *param);
JNIEXPORT int TFLITEEngineDestroyNetwork(void *handle);
JNIEXPORT int TFLITEEngineRunSession(void *handle, UNNModelInOutBuf *param);

// TODO: ONLY TFLITE
//JNIEXPORT void TFLITEEngineResizeInputTensor(void *handle, UNNModelInOutBuf *param);
//JNIEXPORT void TFLITEEngineResizeSession(void *handle);
#ifdef __cplusplus
}
#endif
#endif

