
#ifdef ENABLE_VDSPNN
#include "vdsp_interface.h"
#include "vdspnn_wrapper.h"
#include "sprd_nn_vdsp_cadence.h"
#include <memory>
#include <string>
#include <vector>
#include "log.h"
#include "unn_ion.h"

class VDSPNNObject
{
public:
    VDSPNNObject() {

        // Open VDSP
        if(sprd_nn_vdsp_open(&h_vdsp)){
            LOGE("open vdsp fail\n");
            exit(1);
        }
    }

    virtual ~VDSPNNObject() {

        // Release INOBuf
        if (!_model_ions.empty())
        {
            // RELEASE power hint
            if (0 != sprd_cavdsp_power_hint(h_vdsp, SPRD_VDSP_POWERHINT_LEVEL_5 , SPRD_VDSP_POWERHINT_RELEASE ))
            {
                LOGE(" sprd_cavdsp_power_hint SPRD_VDSP_POWERHINT_RELEASE fail \n");
            }
            else
            {
                LOGD(" sprd_cavdsp_power_hint SPRD_VDSP_POWERHINT_RELEASE success !\n");
            }

            if(0 == _mimport)
            {
                for (auto& ion : _model_ions) {
                    if(    NULL != ion)
                    {
                        if(sprd_nn_ionmem_free(ion) != 0){
                            LOGE("free ionbuf (0x%x) fail\n", ion);
                        }
                    }
                }
            }
            else if(1 == _mimport)
            {
                for (int id = 0; id < 3; id++) {
                    if(    NULL != _model_ions[id])
                    {
                        if(sprd_nn_ionmem_free(_model_ions[id]) != 0){
                            LOGE("free ionbuf (0x%x) fail\n", _model_ions[id]);
                        }
                    }
                }

                for (int id = 3; id < _model_ions.size(); id++) {
                    if(    NULL != _model_ions[id])
                    {
                        if(sprd_nn_ionmem_unimport(_model_ions[id]) != 0){
                            LOGE("unimport ionbuf (0x%x) fail\n", _model_ions[id]);
                        }
                    }
                }
            }
            _model_ions.erase(_model_ions.begin(),_model_ions.end());
        }

        // Close VDSP
        if(sprd_nn_vdsp_close(h_vdsp, nsid)){
            LOGE("vdsp close fail\n");
            exit(1);
        }
        memset(nsid, 0, UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH + 1);
    }
public:
    void init_network(UNNModel *info) {
        _mimport = info->param.mimport;

        if(!info->ld.isFromBuffer)
        {
            LOGD("vdsp model file : %s\n", info->ld.model_name);
            char * name = strrchr(info->ld.model_name, '/');
            if(NULL == name)
            {
                strcpy(nsid, info->ld.model_name);
            }
            else
            {
                strcpy(nsid, name+1);
            }

            // load code
            if(sprd_nn_load_vdsp_code_library(h_vdsp, info->ld.model_name, nsid)){
                LOGE("open vdsp cadence lib fail\n");
                exit(1);
            }

            // network coef buf
            void *_coef_ion = sprd_nn_load_coffe_library(info->ld.model_name);
            if(NULL == _coef_ion)
            {
                LOGE("fail to coefbuf_ion_handle\n");
                exit(1);
            }
            _model_ions.push_back(_coef_ion);

            // status buf
            void *_status_ion = sprd_nn_ionmem_alloc(4, 0);
            if(NULL == _status_ion)
            {
                LOGE("fail to stabuf_ion_handle\n");
                exit(1);
            }
            _model_ions.push_back(_status_ion);

            // scratch buf
            void *_scratch_ion = sprd_nn_ionmem_alloc(info->param.scratch, 0);
            if(NULL == _scratch_ion)
            {
                LOGE("fail to alloc scratch buffer\n");
                exit(1);
            }
            _model_ions.push_back(_scratch_ion);
        }
        else
        {
            LOGE("Canot load model from buffer\n");
            exit(1);
        }

        if(0 == _mimport)
        {
            //input buf
            LOGD("info->inOut.inputs_count: %d\n", info->inOut.inputs_count);
            for (int id = 0; id < info->inOut.inputs_count; id++) {
                LOGD("info->inOut.inputs[%d].size: %d\n", id, info->inOut.inputs[id].size);
                void *_in_ion = sprd_nn_ionmem_alloc(info->inOut.inputs[id].size, 0);
                if(NULL == _in_ion)
                {
                    LOGE("fail to malloc  input_ion_handle\n");;
                }
                LOGD("input id : %d OK\n", id);
                _model_ions.push_back(_in_ion);
            }

            // output buf
            LOGD("info->inOut.outputs_count: %d\n", info->inOut.outputs_count);
            for (int id = 0; id < info->inOut.outputs_count; id++) {
                LOGD("info->inOut.outputs[%d].size: %d\n", id, info->inOut.outputs[id].size);
                void *_out_ion = sprd_nn_ionmem_alloc(info->inOut.outputs[id].size, 0);
                if(NULL == _out_ion)
                {
                    LOGE("fail to malloc  output_ion_handle\n");
                    exit(1);
                }
                LOGD("output id : %d OK\n", id);
                _model_ions.push_back(_out_ion);
            }
        }
        else if(1 == _mimport)
        {
            LOGD("Import Memory Network Inputs.\n");
            //input buf
            for (int id = 0; id < info->inOut.inputs_count; id++) {
                if (info->inOut.inputs[id].memType == kUNN_MEM_TYPE_ION)
                {
                    void *_in_ion  = sprd_nn_ionmem_import(
                                        info->inOut.inputs[id].size,
                                        info->inOut.inputs[id].fd,
                                        info->inOut.inputs[id].data,
                                        info->inOut.inputs[id].prvHdl);
                    _model_ions.push_back(_in_ion);
                    LOGD("Import kUNN_MEM_TYPE_ION inputs[%d].size: %d\n", id, info->inOut.inputs[id].size);
                }
                else
                {
                    LOGE("model->inOut.outputs[%d].memType error!\n", id);
                }
            }

            LOGD("Import Memory Network Outputs.\n");
            // output buf
            for (int id = 0; id < info->inOut.outputs_count; id++) {
                if (info->inOut.outputs[id].memType == kUNN_MEM_TYPE_ION)
                {
                    void *_out_ion  = sprd_nn_ionmem_import(
                                        info->inOut.outputs[id].size,
                                        info->inOut.outputs[id].fd,
                                        info->inOut.outputs[id].data,
                                        info->inOut.outputs[id].prvHdl);

                    _model_ions.push_back(_out_ion);
                    LOGD("Import kUNN_MEM_TYPE_ION outputs[%d].size: %d\n", id, info->inOut.outputs[id].size);
                }
                else
                {
                    LOGE("model->inOut.outputs[%d].memType error!\n", id);
                }
            }
        }

        // ACQUIRE power hint
        if (0 != sprd_cavdsp_power_hint( h_vdsp, SPRD_VDSP_POWERHINT_LEVEL_5 , SPRD_VDSP_POWERHINT_ACQUIRE ))
        {
            LOGE(" sprd_cavdsp_power_hint SPRD_VDSP_POWERHINT_ACQUIRE fail \n");
        }
        else
        {
            LOGD(" sprd_cavdsp_power_hint SPRD_VDSP_POWERHINT_ACQUIRE success !\n");
        }
    }

    void deinit_network( ) {

        LOGD("deinit_network \n");

        if (!_model_ions.empty())
        {
            // RELEASE power hint
            if (0 != sprd_cavdsp_power_hint(h_vdsp, SPRD_VDSP_POWERHINT_LEVEL_5 , SPRD_VDSP_POWERHINT_RELEASE ))
            {
                LOGE(" sprd_cavdsp_power_hint SPRD_VDSP_POWERHINT_RELEASE fail \n");
            }
            else
            {
                LOGD(" sprd_cavdsp_power_hint SPRD_VDSP_POWERHINT_RELEASE success !\n");
            }

            if(0 == _mimport)
            {
                for (auto& ion : _model_ions) {
                    if(    NULL != ion)
                    {
                        if(sprd_nn_ionmem_free(ion) != 0){
                            LOGE("free ionbuf (0x%x) fail\n", ion);
                        }
                    }
                }
            }
            else if(1 == _mimport)
            {
                for (int id = 0; id < 3; id++) {
                    if(    NULL != _model_ions[id])
                    {
                        if(sprd_nn_ionmem_free(_model_ions[id]) != 0){
                            LOGE("free ionbuf (0x%x) fail\n", _model_ions[id]);
                        }
                    }
                }

                for (int id = 3; id < _model_ions.size(); id++) {
                    if(    NULL != _model_ions[id])
                    {
                        if(sprd_nn_ionmem_unimport(_model_ions[id]) != 0){
                            LOGE("unimport ionbuf (0x%x) fail\n", _model_ions[id]);
                        }
                    }
                }
            }
            _model_ions.erase(_model_ions.begin(),_model_ions.end());
        }
        memset(nsid, 0, UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH + 1);
    }

    int run_network(UNNModelInOutBuf *inOut )
    {
        int iRet = -1;
        if (!_model_ions.empty())
        {
            if(0 == _mimport)
            {
                LOGD("Set Network Inputs.\n");
                for (int id = 0; id < inOut->inputs_count; id++) {
                    void *_in = (void *)(sprd_nn_ionmem_get_vaddr(_model_ions[id + 3]));
                    memcpy(_in, (void *)inOut->inputs[id].data, inOut->inputs[id].size);
                }
            }
            else if(1 == _mimport)
            {
                for (int id = 0; id < inOut->inputs_count; id++) {
                    if (inOut->inputs[id].memType == kUNN_MEM_TYPE_ION && inOut->inputs[id].isCache) {
                        LOGD("flush Network inputs[%d]\n", id);
                        unn_flush_ionmem(inOut->inputs[id].prvHdl, inOut->inputs[id].data, NULL, inOut->inputs[id].size);
                    }
                }
            }
            if(NULL == nsid)
            {
                LOGE("nsid is null\n");
                return iRet;
            }
            LOGD("vdsp run network nsid: %s\n", nsid);
            iRet = sprd_nn_vdsp_send(h_vdsp,
                              nsid,
                              0, /*int priority*/
                              (void **)&_model_ions[0],
                              _model_ions.size()/*uint32_t h_ionmem_num*/);

            if(!iRet)
            {
                if(0 == _mimport)
                {
                    LOGD("Get Network Outputs.\n");
                    for (int id = 0; id < inOut->outputs_count; id++) {
                        void *_out = (void *)(sprd_nn_ionmem_get_vaddr(_model_ions[3 + inOut->inputs_count + id]));
                        memcpy((void *)inOut->outputs[id].data, _out, inOut->outputs[id].size);
                    }
                }
                else if(1 == _mimport)
                {
                    for (int id = 0; id < inOut->outputs_count; id++) {
                        if (inOut->outputs[id].memType == kUNN_MEM_TYPE_ION && inOut->outputs[id].isCache) {
                            LOGD("Invalid Network output[%d]\n", id);
                            unn_invalid_ionmem(inOut->outputs[id].prvHdl);
                        }
                    }
                }
            }
            else
            {
                LOGE("vdsp send cmd fail\n");
                return iRet;
            }
        }
        return iRet;
    }

public:
    char nsid[UNN_MODEL_MAX_MODEL_PATH_NAME_LENGTH + 1] = {0};
    void *h_vdsp = NULL;
    std::vector<void *> _model_ions;    // model ION BUFFER
    int  _mimport = 0;
};


int VDSPNNEngineInit(void **handle)
{
    int ire = -1;
    VDSPNNObject *magic = new VDSPNNObject();
    if (magic != NULL)
    {
        ire = 0;
        *handle = magic;
    }
    else
    {
        *handle = NULL;
        LOGD("VDSPNNEngineInit Failed!\n");
    }
    return ire;
}

int VDSPNNEngineDeInit(void *handle)
{
    int ire = -1;
    if (handle != NULL)
    {
        VDSPNNObject* pVDSPNN = static_cast<VDSPNNObject *>(handle);
        delete pVDSPNN;
        ire = 0;
    }
    return ire;
}

int VDSPNNEngineCreateNetwork(void *handle, UNNModel *model)
{
    int ire = -1;
    if (handle != NULL)
    {
        VDSPNNObject* pVDSPNN = static_cast<VDSPNNObject *>(handle);
        if (pVDSPNN->h_vdsp)
        {
            pVDSPNN->init_network(model);
        }
        ire = 0;
    }
    return ire;
}

int VDSPNNEngineDestroyNetwork(void *handle)
{
    int ire = -1;
    if (handle != NULL)
    {
        VDSPNNObject* pVDSPNN = static_cast<VDSPNNObject *>(handle);
        if (pVDSPNN->h_vdsp && (!pVDSPNN->_model_ions.empty()))
        {
            pVDSPNN->deinit_network();
        }
        ire = 0;
    }
    return ire;
}

static bool VDSPNNEngineOK(void *handle)
{
    bool b = false;
    if (handle != NULL)
    {
        VDSPNNObject* pVDSPNN = static_cast<VDSPNNObject *>(handle);
        if (pVDSPNN->h_vdsp && (!pVDSPNN->_model_ions.empty()))
        {
            b = true;
        }
    }
    return b;
}

int VDSPNNEngineRunSession(void *handle, UNNModelInOutBuf *inOut)
{
    int ire = -99;
    if (VDSPNNEngineOK(handle))
    {
        VDSPNNObject* pVDSPNN = static_cast<VDSPNNObject *>(handle);
        LOGD("Start Invoke \n");
        if (ire = (int)pVDSPNN->run_network( inOut ) != 0) {
            LOGD("Failed to invoke VDSP Runtime!\n");
        }
        LOGD("Invoke is OK \n");
    }

    return ire;
}

#endif
