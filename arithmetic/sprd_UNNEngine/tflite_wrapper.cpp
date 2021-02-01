#ifdef ENABLE_TFLITE

#include "tflite_wrapper.h"
#include <memory>
#include <string>
#include <vector>

#include "tensorflow/lite/c/c_api.h"
#include "tensorflow/lite/c/c_api_experimental.h"
#include "tensorflow/lite/delegates/gpu/delegate.h"
#include "log.h"


class TFLITEObject{
public:
    TFLITEObject(){
        model = nullptr;
        options = nullptr;
        interpreter = nullptr;
        gpudelegate = nullptr;
    }
    virtual ~TFLITEObject() {
        if(gpudelegate)
            TfLiteGpuDelegateV2Delete(gpudelegate);
        if(interpreter)
            TfLiteInterpreterDelete(interpreter);
        if(options)
            TfLiteInterpreterOptionsDelete(options);
        if(model)
            TfLiteModelDelete(model);
        model = nullptr;
        options = nullptr;
        interpreter = nullptr;
        gpudelegate = nullptr;
    }
public:
    void init_network(UNNModel *info) {

        if(info->ld.isFromBuffer)
        {
            LOGD("tflite buffer : 0x%x, size : %d\n", info->ld.model_buffer, info->ld.model_size);
            model = TfLiteModelCreate((const char*)info->ld.model_buffer, info->ld.model_size);
        }
        else
        {
            LOGD("tflite file : %s\n", info->ld.model_name);
            model = TfLiteModelCreateFromFile(info->ld.model_name);
        }

        options = TfLiteInterpreterOptionsCreate();

        if( info->param.number_of_threads > 0)
            TfLiteInterpreterOptionsSetNumThreads(options, info->param.number_of_threads);

        if( info->param.forword_type == kUNN_FORWARD_ANN)
        {
            TfLiteInterpreterOptionsSetUseNNAPI(options, 1);
        }
        else if(info->param.forword_type == kUNN_FORWARD_OPENCL || info->param.forword_type == kUNN_FORWARD_OPENGL)
        {
            // GPU Delegate
            TfLiteGpuDelegateOptionsV2 gpu_opts = TfLiteGpuDelegateOptionsV2Default();
            gpu_opts.inference_preference =
                TFLITE_GPU_INFERENCE_PREFERENCE_SUSTAINED_SPEED;
            gpu_opts.inference_priority1 =
                info->param.allow_fp16 ? TFLITE_GPU_INFERENCE_PRIORITY_MIN_LATENCY
                        : TFLITE_GPU_INFERENCE_PRIORITY_MAX_PRECISION;

            gpudelegate = TfLiteGpuDelegateV2Create(&gpu_opts/*default options=nullptr*/);

            // Add Delegate
            TfLiteInterpreterOptionsAddDelegate(options, gpudelegate);
        }

        // Create the interpreter.
        interpreter = TfLiteInterpreterCreate(model, options);

        // Allocate tensors and populate the input tensor data.
        TfLiteInterpreterAllocateTensors(interpreter);

    }

    void deinit_network( ) {

        LOGD("deinit_network \n");

        if(gpudelegate)
            TfLiteGpuDelegateV2Delete(gpudelegate);
        if(interpreter)
            TfLiteInterpreterDelete(interpreter);
        if(options)
            TfLiteInterpreterOptionsDelete(options);
        if(model)
            TfLiteModelDelete(model);

        model = nullptr;
        options = nullptr;
        interpreter = nullptr;
        gpudelegate = nullptr;
    }

    int run_network( UNNModelInOutBuf *inOut) {
        // SetInput
        LOGD("Set Network Inputs.\n");
        for (int id = 0; id < inOut->inputs_count; id++) {
            TfLiteTensor* input_tensor = TfLiteInterpreterGetInputTensor(interpreter, id);
            TfLiteTensorCopyFromBuffer(input_tensor, (void *)inOut->inputs[id].data, inOut->inputs[id].size);
        }

        // Execute
        TfLiteInterpreterInvoke(interpreter);

        // GetOutput
        LOGD("Get Network Outputs.\n");
        for (int id = 0; id < inOut->outputs_count; id++) {
            const TfLiteTensor* output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, id);
            if(output_tensor->quantization.type == kTfLiteAffineQuantization)
            {
                TfLiteQuantizationParams param = TfLiteTensorQuantizationParams(output_tensor);
                inOut->outputs[id].isQuant     = 1;
                inOut->outputs[id].quant_zero  = param.zero_point;
                inOut->outputs[id].quant_scale = param.scale;
            }
            TfLiteTensorCopyToBuffer(output_tensor, (void *)inOut->outputs[id].data, inOut->outputs[id].size);
        }
        return 0;
    }
public:
    TfLiteModel *model;
    TfLiteInterpreterOptions *options;
    TfLiteInterpreter *interpreter;
    TfLiteDelegate *gpudelegate;
};


int TFLITEEngineInit(void **handle){
    int ire = -1;
    TFLITEObject *magic = new TFLITEObject();
    if(magic != NULL){
        ire = 0;
        *handle = magic;
        LOGD("TFLITEEngineInit OK");
    }
    else{
        *handle = NULL;
        LOGE("TFLITEEngineInit Failed!");
    }

    return ire;
}


int TFLITEEngineDeInit(void *handle){
    int ire = -1;
    if(handle != NULL){
        TFLITEObject* pTFLITE = static_cast<TFLITEObject *>(handle);
        delete pTFLITE;
        pTFLITE = NULL;
        ire = 0;
    }
    return ire;
}

int TFLITEEngineCreateNetwork(void *handle, UNNModel *info)
{
    int ire = -1;
    if (handle != NULL)
    {
        TFLITEObject* pTFLITE = static_cast<TFLITEObject *>(handle);
        pTFLITE->init_network(info);
        ire = 0;
    }
    return ire;
}

int TFLITEEngineDestroyNetwork(void *handle)
{
    int ire = -1;
    if(handle != NULL){
        TFLITEObject* pTFLITE = static_cast<TFLITEObject *>(handle);
        pTFLITE->deinit_network();
        ire = 0;
    }
    return ire;
}

static bool TFLITEEngineOK(void *handle){
    bool b = false;
    if(handle != NULL){
        TFLITEObject* pTFLITE = static_cast<TFLITEObject *>(handle);
        if (pTFLITE->model && pTFLITE->interpreter){
            b = true;
        }
    }
    return b;
}

int TFLITEEngineRunSession(void *handle, UNNModelInOutBuf *inOut){
    int ire = -99;
    if(TFLITEEngineOK(handle)){
        TFLITEObject* pTFLITE = static_cast<TFLITEObject *>(handle);
        LOGD("Start Invoke \n");
        if (ire = (int)pTFLITE->run_network( inOut ) != 0) {
            LOGE("Failed to invoke TFLITE Runtime!\n");
        }
        ire = 0;
        LOGD("Invoke is OK \n");
    }
    return ire;
}

#endif
