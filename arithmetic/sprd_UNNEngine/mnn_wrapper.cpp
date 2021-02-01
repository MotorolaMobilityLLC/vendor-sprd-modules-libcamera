#ifdef ENABLE_MNN

#include "mnn_wrapper.h"
#include <memory>
#include <string>
#include <vector>
#include "MNN/Interpreter.hpp"
#include "MNN/Tensor.hpp"
#include "MNN/ImageProcess.hpp"
#include "log.h"

using namespace MNN;



class MNNObject{
public:
    MNNObject(){
        model = nullptr;
        session = nullptr;
    }
    virtual ~MNNObject() {
    }
public:
    void init_network(UNNModel *info) {
        if(info->ld.isFromBuffer)
        {
            LOGD("MNN buffer : 0x%x, size : %d\n", info->ld.model_buffer, info->ld.model_size);
            model = std::unique_ptr<MNN::Interpreter>(MNN::Interpreter::createFromBuffer((const char*)info->ld.model_buffer, info->ld.model_size));
        }
        else
        {
            model = std::unique_ptr<MNN::Interpreter>(MNN::Interpreter::createFromFile(info->ld.model_name));
        }

        if (model){
            MNN::ScheduleConfig config;

            // Set ForwardType
            config.type = (MNNForwardType) info->param.forword_type;

            // Set threads
            if( info->param.number_of_threads > 0)
                config.numThread = info->param.number_of_threads;

            // Create Session
            session = model->createSession(config);
            if (NULL == session) {
                LOGE("\nMNN Failed to Create Session\n");
                exit(-1);
            }
            {
                auto tensors = model->getSessionInputAll(session);
                for (auto iter = tensors.cbegin(); iter != tensors.end(); iter++){
                    inputs.push_back(iter->first);
                }
            }
            {
                auto tensors = model->getSessionOutputAll(session);
                for (auto iter = tensors.cbegin(); iter != tensors.end(); iter++){
                    outputs.push_back(iter->first);
                }
            }
        }
    }

    void deinit_network( ) {
        LOGD("MNN deinit_network \n");
        model = nullptr;
        session = nullptr;
        inputs.erase(inputs.begin(),inputs.end());
        outputs.erase(outputs.begin(),outputs.end());
    }

    int run_network( UNNModelInOutBuf *inOut ) {
        // SetInput
        LOGD("Set Network Inputs.\n");
        for (int id = 0; id < inOut->inputs_count; id++) {
            auto t = model->getSessionInput(session, inputs[id].c_str());
            //auto t_shape = t->shape();
            //LOGD("Inputs t_shape: (%d,%d,%d,%d ).\n",t_shape[0],t_shape[1],t_shape[2],t_shape[3]);
            MNN::Tensor _in(t, t->getDimensionType(), false);
            _in.buffer().host = (uint8_t *)inOut->inputs[id].data;
            t->copyFromHostTensor(&_in);
        }

        // Execute
        model->runSession(static_cast< MNN::Session *>(session));

        // GetOutput
        LOGD("Get Network Outputs.\n");
        for (int id = 0; id < inOut->outputs_count; id++) {
            auto t = model->getSessionOutput(session, outputs[id].c_str());
            //auto t_shape = t->shape();
            //LOGD("Outputs t_shape: (%d,%d,%d,%d ).\n",t_shape[0],t_shape[1],t_shape[2],t_shape[3]);
            MNN::Tensor _out(t, t->getDimensionType(), false);
            _out.buffer().host = (uint8_t *)inOut->outputs[id].data;
            t->copyToHostTensor(&_out);

        }
        return 0;
    }
public:
    std::unique_ptr<MNN::Interpreter> model;
    MNN::Session *session;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};


int MNNEngineInit(void **handle){
    int ire = -1;
    MNNObject *magic = new MNNObject();
    if(magic != NULL){
        ire = 0;
        *handle = magic;
    }
    else{
        *handle = NULL;
    }

    return ire;
}


int MNNEngineDeInit(void *handle){
    int ire = -1;
    if(handle != NULL){
        ire = 0;
        MNNObject* pMNN = static_cast<MNNObject *>(handle);
        if (pMNN->model && pMNN->session){
            pMNN->deinit_network();
        }
        delete pMNN;
    }
    return ire;
}

int MNNEngineCreateNetwork(void *handle, UNNModel *model){
    int ire = -1;
    if(handle != NULL){
        MNNObject* pMNN = static_cast<MNNObject *>(handle);
        pMNN->init_network(model);
        ire = 0;
    }
    return ire;
}

int MNNEngineDestroyNetwork(void *handle){
    int ire = -1;
    if(handle != NULL){
        MNNObject* pMNN = static_cast<MNNObject *>(handle);
        if (pMNN->model && pMNN->session){
            pMNN->deinit_network();
        }
        ire = 0;
    }
    return ire;
}

static bool MNNEngineOK(void *handle){
    bool b = false;
    if (handle != NULL){
        MNNObject* pMNN = static_cast<MNNObject *>(handle);
        if (pMNN->model && pMNN->session){
            b = true;
        }
    }
    return b;
}

int MNNEngineRunSession(void *handle, UNNModelInOutBuf *inOut){
    int ire = -99;
    if (MNNEngineOK(handle)){
        MNNObject* pMNN = static_cast<MNNObject *>(handle);
        LOGD("Start Invoke \n");
        if (ire = (int)pMNN->run_network( inOut ) != 0) {
            LOGE("Failed to invoke MNN Runtime!\n");
        }
        ire = 0;
        LOGD("Invoke is OK \n");
    }
    return ire;
}

#endif
