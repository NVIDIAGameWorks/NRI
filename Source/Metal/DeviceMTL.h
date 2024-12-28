#pragma once

#import <Metal/MTLDevice.h>

namespace nri {

struct CommandQueueMTL;

struct DeviceMTL final : public DeviceBase {
public:
    DeviceMTL(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~DeviceMTL();


    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);

    inline operator id<MTLDevice>() const {
        return m_Device;
    }
    
    inline id<MTLDevice> GetHandle() const {
        return m_Device;
    }
    
    template <typename Implementation, typename Interface, typename... Args>
    inline Result CreateImplementation(Interface*& entity, const Args&... args) {
        Implementation* impl = Allocate<Implementation>(GetStdAllocator(), *this);
        Result result = impl->Create(args...);

        if (result != Result::SUCCESS) {
            Destroy(GetStdAllocator(), impl);
            entity = nullptr;
        } else {
            entity = (Interface*)impl;
        }

        return result;
    }
    
    
    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }
    
    //void GetMemoryTypeInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    //bool GetMemoryTypeInfo(MemoryLocation memoryLocation, MemoryTypeInfo& memoryTypeInfo) const;
    
    const DeviceDesc& GetDesc() const override {
        return m_Desc;
    }
    
    
    id<MTLRenderPipelineState> GetClearPipeline();
    
    void Destruct() override;
    Result FillFunctionTable(CoreInterface& table) const override;
    Result FillFunctionTable(HelperInterface& table) const override;
    Result FillFunctionTable(LowLatencyInterface& table) const override;
    Result FillFunctionTable(MeshShaderInterface& table) const override;
    Result FillFunctionTable(RayTracingInterface& table) const override;
    Result FillFunctionTable(StreamerInterface& table) const override;
    Result FillFunctionTable(SwapChainInterface& table) const override;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const override;

    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);

    
    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationMTLDesc& deviceCreationVKDesc, bool isWrapper);
private:
    //Lock m_Lock;
    id<MTLDevice>     m_Device;
    std::array<CommandQueueMTL*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    CoreInterface m_CoreInterface = {};
    DeviceDesc m_Desc = {};
    MTLGPUFamily m_Family;
    bool m_OwnsNativeObjects = true;
    Lock m_Lock;
};
}; // namespace nri
