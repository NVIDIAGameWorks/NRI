#pragma once

#import <Metal/MTLDevice.h>

namespace nri {

struct CommandQueueMTL;

struct DeviceMTL final : public DeviceBase {
    DeviceMTL(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~DeviceMTL();


    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);

    inline operator id<MTLDevice>() const {
        return m_Device;
    }

    inline const DeviceDesc& GetDesc() const {
        return m_Desc;
    }
    
    void Destruct() override;

    template <typename Implementation, typename Interface, typename... Args>
    inline Result CreateImplementation(Interface*& entity, const Args&... args) {
        Implementation* impl = Allocate<Implementation>(GetStdAllocator(), *this);
        Result result = impl->Create(args...);

        if (result != Result::SUCCESS) {
            Destroy(GetStdAllocator(), impl);
            entity = nullptr;
        } else
            entity = (Interface*)impl;

        return result;
    }

    
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFnctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationVKDesc, bool isWrapper);
private:
    Lock m_Lock;
    id<MTLDevice>     m_Device;
    std::array<CommandQueueMTL*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    DeviceDesc m_Desc = {};
    MTLGPUFamily m_Family;
    bool m_OwnsNativeObjects = true;
};
}; // namespace nri
