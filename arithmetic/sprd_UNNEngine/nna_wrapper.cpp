#ifdef ENABLE_IMGNPU

#include "nna_wrapper.h"
#include <memory>
#include <string>
#include <vector>
#include "unn_ion.h"

#include "vha_dnn.h"
#include "vha_session.h"
#include "vha_misc.h"
#include "vha_utils.h"
#include "vha_format_converter.h"
#include "log.h"

using nna::VhaSession;
using nna::VhaDnnLayerHandler;
using nna::VhaError;
using nna::VhaNotify;
using nna::VhaMemory;
using nna::VhaMemFlags;
using nna::VhaImportHandle;
using nna::VhaExportHandle;
using nna::VhaDnn;
using nna::VhaRet;
using nna::VhaFormatConverter;
using nna_utils::VhaScopeMem;


class VhaHandle : public VhaImportHandle {
 public:
    VhaHandle(int bufFd, size_t size, BufType type, std::string name = "dnn_buf") :
            _bufFd(bufFd), _bufPtr(nullptr), _size(size), _type(type), _name(name) {};

    VhaHandle(void* bufPtr, size_t size, BufType type, std::string name = "dnn_buf") :
            _bufFd(-1), _bufPtr(bufPtr), _size(size), _type(type), _name(name) {}

    virtual int GetBufFD() const { return _bufFd; };
    virtual void* GetBufPtr() const { return _bufPtr; };
    virtual size_t GetBufSize() const { return _size; };
    virtual BufType GetBufType() const { return _type; };
    virtual std::string GetBufName() const { return _name; };

    virtual ~VhaHandle() {}
 private:
    int _bufFd;
    void* _bufPtr;
    size_t _size;
    BufType _type;
    std::string _name;
};

class NNAObject
{

public:
    NNAObject( int timeout ) :
        timeout_(timeout) {
        api_ = std::unique_ptr<VhaSession>(nna::CreateVhaSession());
        if (api_ == nullptr || api_->GetLastError()->GetCode() !=
            VhaRet::VHA_SUCCESS) {
            LOGE("failed to create VHA session\n");
        }
        LOGD("CreateVhaSession success\n");
        assert(api_->SetInternalCachePolicy(VhaMemFlags::DEFAULT) == true);

        //assert(api_->SetExecQueueDepth(params_.task_count_) == true);

    }

    virtual ~NNAObject() {
        for (auto& inbuf : inbufs_) {
            api_->ReleaseMemory(inbuf);
        }
        inbufs_.erase(inbufs_.begin(),inbufs_.end());
        for (auto& outbuf : outbufs_) {
          api_->ReleaseMemory(outbuf);
        }
        outbufs_.erase(outbufs_.begin(),outbufs_.end());
        info_.in_attrs.erase(info_.in_attrs.begin(),info_.in_attrs.end());
        info_.out_attrs.erase(info_.out_attrs.begin(),info_.out_attrs.end());
        net_ = nullptr;
        api_ = nullptr;
        LOGD("~NNAObject success");
    }

    VhaDnn::DnnNetworkProperties getInfo() {
        return info_;
    }

//protected:
public:
    void init_network(UNNModel *model) {

        LOGD("model param, mimport = %d\n",model->param.mimport);
        mimport_ = model->param.mimport;
        if(model->ld.isFromBuffer)
        {
            //net_ = std::unique_ptr<VhaDnn>(api_->CreateDnnNetwork({model->ld.model_buffer},{model->ld.model_size}));
            net_ = std::unique_ptr<VhaDnn>(api_->CreateDnnNetwork((const char*)model->ld.model_buffer, model->ld.model_size));
        }
        else
        {
            net_ = std::unique_ptr<VhaDnn>(api_->CreateDnnNetwork({model->ld.model_name}));
        }

        /* Create the network from file */
        if (net_ == nullptr) {
            VhaError *err = api_->GetLastError();
            LOGE("failed to create net\n");
        }

        net_->GetProperties(&info_);

        LOGD("\nNetwork inputs:\n");
        for (auto& attrs : info_.in_attrs) {
            LOGD("  %s ( %d )", attrs->name().c_str(), attrs->size());
        }

        LOGD("\nNetwork outputs:\n");
        for (auto& attrs : info_.out_attrs) {
            LOGD("  %s ( %d )", attrs->name().c_str(), attrs->size());
        }
        LOGD("\n");

        if(0 == mimport_)
        {
            LOGD("Internal Allocate Memory Network Inputs.\n");
            for (auto& in : info_.in_attrs) {
                VhaMemFlags in_flags(VhaMemFlags::DEFAULT | VhaMemFlags::INPUT);
                const size_t insize = in->size();
                VhaMemory* in_buf = api_->AllocateMemory(insize, in->name().c_str(), in_flags);
                if (in_buf == nullptr) {
                    LOGD("failed to allocate vhamem\n");
                }
                inbufs_.push_back(in_buf);
            }

            LOGD("Internal Allocate Memory Network Outputs.\n");
            for (auto& out : info_.out_attrs) {
                VhaMemFlags out_flags(VhaMemFlags::DEFAULT | VhaMemFlags::OUTPUT);
                const size_t outsize = out->size();
                VhaMemory* outbuf = api_->AllocateMemory(outsize, out->name().c_str(), out_flags);
                if (outbuf == nullptr) {
                    LOGE("failed to allocate vhamem\n");
                }

                /* Clear the output buffer */
                // memset(outbuf->LockWrite(), 0, outsize);
                // outbuf->Unlock();
                outbufs_.push_back(outbuf);
            }
        }
        else if(1 == mimport_)
        {
            LOGD("Import Memory Network Inputs.\n");
            for (int id = 0; id < model->inOut.inputs_count; id++) {
                const size_t insize = info_.in_attrs[id]->size();
                // LOGD("insize: %d \n", insize);
                if (model->inOut.inputs[id].size != insize)
                    LOGE("Not perfect buffer fit\n");

                VhaMemFlags in_flags(VhaMemFlags::DEFAULT | VhaMemFlags::INPUT);

                if (model->inOut.inputs[id].memType == kUNN_MEM_TYPE_CPUPTR)
                {
                    LOGD("Input import kUNN_MEM_TYPE_CPUPTR: %d \n", insize);
                    VhaHandle handle(
                        model->inOut.inputs[id].data,
                        model->inOut.inputs[id].size,
                        VhaImportHandle::BufType::kCpuPointer, std::string("IN"));

                    // Import Memory
                    VhaMemory* inbuf = api_->ImportMemory(&handle, in_flags);
                    if (inbuf == nullptr) {
                        LOGE("failed to import vhamem \n");
                    }
                    inbufs_.push_back(inbuf);
                }
                else if (model->inOut.inputs[id].memType == kUNN_MEM_TYPE_ION)
                {
                    LOGD("Input import kUNN_MEM_TYPE_ION: %d \n", insize);
                    VhaHandle handle(
                        model->inOut.inputs[id].fd,
                        model->inOut.inputs[id].size,
                        VhaImportHandle::BufType::kFileDescriptor, std::string("IN"));

                    // Import Memory
                    VhaMemory* inbuf = api_->ImportMemory(&handle, in_flags);
                    if (inbuf == nullptr) {
                        LOGE("failed to import vhamem \n");
                    }
                    inbufs_.push_back(inbuf);
                }
            }

            LOGD("Import Memory Network Outputs.\n");
            for (int id = 0; id < model->inOut.outputs_count; id++) {
                const size_t outsize = info_.out_attrs[id]->size();
                //LOGD("outsize: %d \n", outsize);
                if (model->inOut.outputs[id].size != outsize)
                    LOGE("Not perfect buffer fit\n");

                VhaMemFlags out_flags(VhaMemFlags::DEFAULT | VhaMemFlags::OUTPUT);

                if (model->inOut.outputs[id].memType == kUNN_MEM_TYPE_CPUPTR)
                {
                    LOGD("Output import kUNN_MEM_TYPE_CPUPTR: %d \n", outsize);
                    VhaHandle handle(
                        model->inOut.outputs[id].data,
                        model->inOut.outputs[id].size,
                        VhaImportHandle::BufType::kCpuPointer, std::string("OUT"));
                    // Import Memory
                    VhaMemory* outbuf = api_->ImportMemory(&handle, out_flags);
                    if (outbuf == nullptr) {
                        LOGE("failed to import vhamem \n");
                    }
                    outbufs_.push_back(outbuf);
                }
                else if (model->inOut.outputs[id].memType == kUNN_MEM_TYPE_ION)
                {
                    LOGD("Output import kUNN_MEM_TYPE_ION: %d \n", outsize);
                    VhaHandle handle(
                        model->inOut.outputs[id].fd,
                        model->inOut.outputs[id].size,
                        VhaImportHandle::BufType::kFileDescriptor, std::string("OUT"));

                    // Import Memory
                    VhaMemory* outbuf = api_->ImportMemory(&handle, out_flags);
                    if (outbuf == nullptr) {
                        LOGE("failed to import vhamem \n");
                    }
                    outbufs_.push_back(outbuf);
                }
            }
        }
    }

    void deinit_network( ) {
        LOGD("deinit_network \n");
        for (auto& inbuf : inbufs_) {
            api_->ReleaseMemory(inbuf);
        }
        inbufs_.erase(inbufs_.begin(),inbufs_.end());
        for (auto& outbuf : outbufs_) {
          api_->ReleaseMemory(outbuf);
        }
        outbufs_.erase(outbufs_.begin(),outbufs_.end());
        info_.in_attrs.erase(info_.in_attrs.begin(),info_.in_attrs.end());
        info_.out_attrs.erase(info_.out_attrs.begin(),info_.out_attrs.end());
        net_ = nullptr;
        timeout_ = 0;
    }

    int run_network( UNNModelInOutBuf *inOut) {

        // SetInput
        if(0 == mimport_)
        {
            LOGD("Set Network Inputs.\n");
            for (int id = 0; id < inOut->inputs_count; id++) {
                VhaMemory* inbuf = inbufs_[id];
                void *to = (void *)(inbuf->LockWrite());
                memcpy((void *)to, (void *)inOut->inputs[id].data, inOut->inputs[id].size);
                inbuf->Unlock();
            }
        }
        else if(1 == mimport_)
        {
            for (int id = 0; id < inOut->inputs_count; id++) {
                if (inOut->inputs[id].memType == kUNN_MEM_TYPE_ION && inOut->inputs[id].isCache) {
                    LOGD("flush Network inputs[%d]\n", id);
                    unn_flush_ionmem(inOut->inputs[id].prvHdl, inOut->inputs[id].data, NULL, inOut->inputs[id].size);
                }
            }
        }

        // Execute
        VhaNotify* done = net_->GetNotify();
        if (!done) {
            LOGE("Can't get Notify object. Consider increasing notifiers pool.\n");
            return -1;
        }

        if (net_->Execute({ inbufs_, outbufs_, NULL }, done)) {
            VhaNotify::NotifyStatus status = done->WaitForCompletion(timeout_);
            if (status == VhaNotify::kComplete) {
    #ifdef PRINT_STATS
                std::vector<std::string> perf_stats;
                net_->GetNetworkStatistics(&perf_stats);
                if (perf_stats.size() > 0) {
                    LOGD("\nStatistics for network execution: \n");
                    for (auto& stat : perf_stats) {
                        LOGD("   %s\n", stat);
                    }
                }
    #endif
                // GetOutput
                if(0 == mimport_)
                {
                    LOGD("Get Network Outputs.\n");
                    for (int id = 0; id < inOut->outputs_count; id++) {
                        VhaMemory* outbuf = outbufs_[id];
                        void *out = (void *)(outbuf->LockRead());
                        memcpy((void *)inOut->outputs[id].data, (void *)out, inOut->outputs[id].size);
                        outbuf->Unlock();
                    }
                }
                else if(1 == mimport_)
                {
                    for (int id = 0; id < inOut->outputs_count; id++) {
                        if (inOut->outputs[id].memType == kUNN_MEM_TYPE_ION && inOut->outputs[id].isCache) {
                            LOGE("Invalid Network output[%d]\n", id);
                            unn_invalid_ionmem(inOut->outputs[id].prvHdl);
                        }
                    }
                }
                return 0;
            }
            else if (status == VhaNotify::kError) {
                LOGE("Error while executing the network\n");
                return -1;
            }
            else if (status == VhaNotify::kTimeout) {
                LOGE("Timeout occurred while executing the network!\n");
                return -1;
            }
            else {
                LOGE("Generic execute error !\n");
                return -1;
            }
        }
        else {
            VhaError *err = net_->GetLastError();
            LOGE("execute error: %s, code: %d", err->GetName().c_str(), static_cast<int>(err->GetCode()));
            LOGE("Net Execute Failed!!\n");
            return -1;
        }
    }
public:
    std::unique_ptr<VhaSession> api_;
    std::unique_ptr<VhaDnn> net_;
    std::vector<VhaMemory*> inbufs_;
    std::vector<VhaMemory*> outbufs_;
    VhaDnn::DnnNetworkProperties info_;
    int timeout_;
    int mimport_;
};


int NNAEngineInit(void **handle)
{
    int ire = -1;
    NNAObject *magic = new NNAObject(0);
    if (magic != NULL)
    {
        ire = 0;
        *handle = magic;
        LOGD("NNAEngine success\n");;
    }
    else
    {
        *handle = NULL;
        LOGE("NNAEngineInit Failed!\n");
    }

    return ire;
}

int NNAEngineDeInit(void *handle)
{
    int ire = -1;
    if (handle != NULL)
    {
        NNAObject* pNNA = static_cast<NNAObject *>(handle);
        delete pNNA;
        ire = 0;
    }
    return ire;
}

int NNAEngineCreateNetwork(void *handle, UNNModel *model)
{
    int ire = -1;
    if (handle != NULL)
    {
        NNAObject* pNNA = static_cast<NNAObject *>(handle);
        pNNA->init_network(model);
        ire = 0;
    }
    return ire;
}

int NNAEngineDestroyNetwork(void *handle)
{
    int ire = -1;
    if (handle != NULL)
    {
        NNAObject* pNNA = static_cast<NNAObject *>(handle);
        if (pNNA->api_ && pNNA->net_)
        {
            pNNA->deinit_network();
        }
        ire = 0;
    }
    return ire;
}

static bool NNAEngineOK(void *handle)
{
    bool b = false;
    if (handle != NULL)
    {
        NNAObject* pNNA = static_cast<NNAObject *>(handle);
        if (pNNA->api_ && pNNA->net_)
        {
            b = true;
        }
    }
    return b;
}


int NNAEngineRunSession(void *handle, UNNModelInOutBuf *inOut)
{
    int ire = -99;
    if (NNAEngineOK(handle))
    {
        NNAObject* pNNA = static_cast<NNAObject *>(handle);
        LOGD("Start Invoke \n");
        if (ire = (int)pNNA->run_network( inOut ) != 0) {
            LOGE("Failed to invoke NNA Runtime!\n");
        }
        LOGD("Invoke is OK \n");
    }

    return ire;
}
#endif
