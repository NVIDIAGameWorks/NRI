// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVal;

struct IsExtSupported {
    uint32_t lowLatency : 1;
    uint32_t meshShader : 1;
    uint32_t rayTracing : 1;
    uint32_t swapChain : 1;
    uint32_t wrapperD3D11 : 1;
    uint32_t wrapperD3D12 : 1;
    uint32_t wrapperVK : 1;
};

struct DeviceVal final : public DeviceBase {
    DeviceVal(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator, DeviceBase& device);
    ~DeviceVal();

    inline Device& GetImpl() const {
        return m_Device;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreAPI;
    }

    inline const HelperInterface& GetHelperInterface() const {
        return m_HelperAPI;
    }

    inline const StreamerInterface& GetStreamerInterface() const {
        return m_StreamerAPI;
    }

    inline const WrapperD3D11Interface& GetWrapperD3D11Interface() const {
        return m_WrapperD3D11API;
    }

    inline const WrapperD3D12Interface& GetWrapperD3D12Interface() const {
        return m_WrapperD3D12API;
    }

    inline const WrapperVKInterface& GetWrapperVKInterface() const {
        return m_WrapperVKAPI;
    }

    inline const SwapChainInterface& GetSwapChainInterface() const {
        return m_SwapChainAPI;
    }

    inline const RayTracingInterface& GetRayTracingInterface() const {
        return m_RayTracingAPI;
    }

    inline const MeshShaderInterface& GetMeshShaderInterface() const {
        return m_MeshShaderAPI;
    }

    inline const LowLatencyInterface& GetLowLatencyInterface() const {
        return m_LowLatencyAPI;
    }

    inline void* GetNativeObject() const {
        return m_CoreAPI.GetDeviceNativeObject(m_Device);
    }

    inline Lock& GetLock() {
        return m_Lock;
    }

    const DeviceDesc& GetDesc() const {
        return ((DeviceBase&)m_Device).GetDesc();
    }

    bool Create();
    void RegisterMemoryType(MemoryType memoryType, MemoryLocation memoryLocation);

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    void Destruct();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;
    Result FillFunctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(ResourceAllocatorInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(StreamerInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(WrapperD3D11Interface& table) const;
    Result FillFunctionTable(WrapperD3D12Interface& table) const;
    Result FillFunctionTable(WrapperVKInterface& table) const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    Result CreateFence(uint64_t initialValue, Fence*& fence);
    Result CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory);
    Result CreateMemory(const MemoryD3D12Desc& memoryDesc, Memory*& memory);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferVKDesc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferD3D11Desc& bufferDesc, Buffer*& buffer);
    Result CreateBuffer(const BufferD3D12Desc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture);
    Result CreateTexture(const TextureD3D11Desc& textureDesc, Texture*& texture);
    Result CreateTexture(const TextureD3D12Desc& textureDesc, Texture*& texture);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline);
    Result AllocateBuffer(const AllocateBufferDesc& bufferDesc, Buffer*& buffer);
    Result AllocateTexture(const AllocateTextureDesc& textureDesc, Texture*& texture);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool);
    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    Result CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateCommandQueue(const CommandQueueVKDesc& commandQueueDesc, CommandQueue*& commandQueue);
    Result CreateCommandBuffer(const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateCommandBuffer(const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool);
    Result CreateDescriptorPool(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool);
    Result CreateComputePipeline(VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline);
    Result CreateGraphicsPipeline(VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline);
    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator);
    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result AllocateAccelerationStructure(const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

    void DestroyFence(Fence& fence);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroySwapChain(SwapChain& swapChain);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyCommandBuffer(CommandBuffer& commandBuffer);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);

    void FreeMemory(Memory& memory);
    void SetDebugName(const char* name);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result AllocateMemory(const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const;
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc);
    FormatSupportBits GetFormatSupport(Format format) const;

private:
    Device& m_Device;
    String m_Name;
    CoreInterface m_CoreAPI = {};
    HelperInterface m_HelperAPI = {};
    StreamerInterface m_StreamerAPI = {};
    LowLatencyInterface m_LowLatencyAPI = {};
    MeshShaderInterface m_MeshShaderAPI = {};
    RayTracingInterface m_RayTracingAPI = {};
    ResourceAllocatorInterface m_ResourceAllocatorAPI = {};
    SwapChainInterface m_SwapChainAPI = {};
    WrapperD3D11Interface m_WrapperD3D11API = {};
    WrapperD3D12Interface m_WrapperD3D12API = {};
    WrapperVKInterface m_WrapperVKAPI = {};
    std::array<CommandQueueVal*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    UnorderedMap<MemoryType, MemoryLocation> m_MemoryTypeMap;

    union {
        uint32_t m_IsExtSupportedStorage = 0;
        IsExtSupported m_IsExtSupported;
    };

    Lock m_Lock;
};

} // namespace nri
