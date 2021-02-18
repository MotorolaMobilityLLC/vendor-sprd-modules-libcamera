/* Copyright wenguo.li@unisoc.com. All Rights Reserved @2020.11.11.

UNN === Unisoc Neural Network

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

#ifndef _UNN_ENGINE_H
#define _UNN_ENGINE_H
#include <cstdint>
#include <cstdio>
#include <string>
#ifdef __cplusplus
extern "C" {
#endif
#define JNIEXPORT    __attribute__ ((visibility("default")))

#define UNN_MODEL_MAX_DIMS (6)
#define UNN_MODEL_MAX_inputs (20)
#define UNN_MODEL_MAX_outputs (20)
#define UNN_MODEL_MAX_TENSOR_NAME_LENGTH (1023)
#define UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH (4095)
#define UNN_MODEL_MAX_PARAM_RESV (8)
typedef enum {
    kUNN_VDSP       = 0, /*use vdsp*/
    kUNN_IMGNNA     = 1, /*use NNA*/
    kUNN_TFLITE     = 2, /*use TFLite*/
    kUNN_MNN        = 3, /*use MNN*/
} UNNRunType;

typedef enum {
    kUNN_FORWARD_CPU = 0,

    /*
     Firtly find the first available backends not equal to CPU
     If no other backends, use cpu
     */
    kUNN_FORWARD_AUTO = 4,   // Only MNN Support

    /*Hand write metal*/
    kUNN_FORWARD_METAL = 1,  // Only MNN Support

    /*NVIDIA GPU API*/
    kUNN_FORWARD_CUDA = 2,   // Only MNN Support

    /*Android / Common Device GPU API*/
    kUNN_FORWARD_OPENCL = 3, // Only tflite/MNN Support
    kUNN_FORWARD_OPENGL = 6, // Only tflite/MNN Support
    kUNN_FORWARD_VULKAN = 7, // Only MNN Support

    /* Android NNAPI */
    kUNN_FORWARD_ANN = 5, // Only tflite Support, MNN Not Support yet

    /* UNISOC BACKEND Support */
    kUNN_FORWARD_USER_1 = 8,
    kUNN_FORWARD_USER_2 = 9,
    kUNN_FORWARD_USER_3 = 10,
    kUNN_FORWARD_USER_4 = 11,

} UNNForwardType;

typedef enum {
    kUNN_NoOrder = 0,
    kUNN_NCHW    = 1,
    kUNN_NHWC    = 2,
} UNNTensorOrder;

typedef enum {
    kUNNNoType    = 0,
    kUNNFloat32   = 1,
    kUNNInt32     = 2,
    kUNNUInt8     = 3,
    kUNNInt64     = 4,
    kUNNString    = 5,
    kUNNBool      = 6,
    kUNNInt16     = 7,
    kUNNComplex64 = 8,
    kUNNInt8      = 9,
    kUNNFloat16   = 10,
    kUNNFloat64   = 11,
} UNNTensorType;

typedef enum {
    kUNN_MEM_TYPE_UNIFIED             = -1,   // invalid
    kUNN_MEM_TYPE_ION                 = 0,    // ION Buffer
    kUNN_MEM_TYPE_CPUPTR              = 1,    // Cpu Buffer
    kUNN_MEM_TYPE_DMABUF              = 2,    // DMA Buffer
} UNNBufType;

typedef struct UNNTensorBuffer {
    uint32_t        dims[UNN_MODEL_MAX_DIMS];  // batch, channel, width, height
    uint32_t        dims_size;
    char            name[UNN_MODEL_MAX_TENSOR_NAME_LENGTH + 1];
    UNNTensorType   type;
    UNNTensorOrder  order;
    uint32_t        bitdepth;
    int32_t         exponent;
    int32_t         isFloat;
    int32_t         isSigned;
    int32_t         isQuant;
    int32_t         quant_zero;  //if isQuant is 1, this value is available
    float           quant_scale; //if isQuant is 1, this value is available
    uint32_t        size;
    UNNBufType      memType  = kUNN_MEM_TYPE_CPUPTR;
    uint32_t        memAlign = 0x1000; //
    int32_t         isCache;
    int32_t         fd; //ION BUF NEEDINFO
    void            *data;
    void            *prvHdl;    //Private HANDEL, Prohibit to modify
    void            *revPtr[UNN_MODEL_MAX_PARAM_RESV];   //reserve
    int32_t         revInt[UNN_MODEL_MAX_PARAM_RESV];    //reserve
} UNNTensorBuffer;

typedef struct UNNModelLoader {
    int32_t     isFromBuffer;     // 0: From File 1: From buffer
    char        model_name[UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH + 1];
    uint32_t    model_size;       //if isFromBuffer is 1, this value is available
    void        *model_buffer;    //if isFromBuffer is 1, this value is available
} UNNModelLoader;


typedef struct UNNModelInOutBuf {
    uint32_t inputs_count;
    UNNTensorBuffer inputs[UNN_MODEL_MAX_inputs];
    uint32_t outputs_count;
    UNNTensorBuffer outputs[UNN_MODEL_MAX_outputs];
} UNNModelInOutBuf;


typedef struct UNNSettings {
    uint32_t        scratch;                             // VDSP scratch buffer size
    uint32_t        mimport                    = 1;      // 0:NNA alloc input/output buffer  1: user alloc input/output buffer , NNA import
    uint32_t        number_of_threads          = 4;      // number_of_threads when excuting network on cpu (tflite/MNN)
    int32_t         allow_fp16                 = 0;      // gpu delegate allow_fp16 enable/disable (tflite)
    UNNForwardType  forword_type               = kUNN_FORWARD_CPU; // network inference type (tflite/MNN)
    uint32_t        p1[UNN_MODEL_MAX_PARAM_RESV];  // reserve
    void            *p2[UNN_MODEL_MAX_PARAM_RESV]; // reserve
} UNNSettings;

typedef struct UNNModel {
    UNNModelLoader    ld;
    UNNModelInOutBuf  inOut;
    UNNSettings       param;
} UNNModel;

/************************************************************
name: UNNEngineInit
description: init
param:
handle ---- UNNEngine handle
rt     ---- UNNRunType: VDSP/NNA/TFLite
return value: 0 is ok, other is failed
***********************************************************/
JNIEXPORT int UNNEngineInit(void **handle, UNNRunType rt, void* ptr);

/************************************************************
name: UNNEngineDeInit
description: deinit
param:
handle ---- UNNEngine handle
return value: 0 is ok, other is failed
***********************************************************/
JNIEXPORT int UNNEngineDeInit(void *handle);


/************************************************************
name: UNNEngineAllocTensorBuffer
description: allocate UNNTensorBuffer
param:
handle   ---- UNNEngine handle
tensor   ---- UNNTensorBuffer
return value: 0 is ok, other is failed
***********************************************************/
JNIEXPORT int UNNEngineAllocTensorBuffer(void *handle, void *tensor);


/************************************************************
name: UNNEngineAllocTensorBuffer
description: allocate UNNTensorBuffer
param:
handle   ---- UNNEngine handle
tensor   ---- UNNTensorBuffer
return value: 0 is ok, other is failed
***********************************************************/
JNIEXPORT int UNNEngineFreeTensorBuffer(void *handle, void *tensor);

/************************************************************
name: UNNEngineCreateNetwork
description: load AI model file
param:
handle   ---- UNNEngine handle
filename ---- model filename
              (eg, deeplab_model)
              (model file path is 'system/firmware' in androidq;'system_ext/firmware' in androidr)
info     ---- setting info
return value: 0 is ok, other is failed
***********************************************************/
JNIEXPORT int UNNEngineCreateNetwork(void *handle, void *info); // void *info = (UNNModel *)info;


/************************************************************
name: UNNEngineDestroyNetwork
description: Destroy Network
param:
handle   ---- UNNEngine handle
***********************************************************/
JNIEXPORT int UNNEngineDestroyNetwork(void *handle);


/************************************************************
name: UNNEngineRunSession
description: run session
param:
handle   ---- UNNEngine handle
return value: 0 is ok, other is failed
***********************************************************/
JNIEXPORT int UNNEngineRunSession(void *handle, void *info); // void *info = (UNNModelInOutBuf *)info;

#ifdef __cplusplus
}
#endif
#endif //_UNN_ENGINE_H



