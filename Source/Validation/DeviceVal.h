// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVal;

struct DeviceVal final : public DeviceBase {
    DeviceVal(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator, DeviceBase& device);
    ~DeviceVal();

    bool Create();
    void RegisterMemoryType(MemoryType memoryType, MemoryLocation memoryLocation);

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

    //================================================================================================================
    // NRI
    //================================================================================================================
    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    void DestroySwapChain(SwapChain& swapChain);
    void SetDebugName(const char* name);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateFence(uint64_t initialValue, Fence*& fence);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroyFence(Fence& queueSemaphore);
    Result AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void FreeMemory(Memory& memory);

    Result CreateRayTracingPipeline(const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline);
    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);
    FormatSupportBits GetFormatSupport(Format format) const;

    Result CreateCommandQueueVK(const CommandQueueVKDesc& commandQueueDesc, CommandQueue*& commandQueue);
    Result CreateCommandAllocatorVK(const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator);
    Result CreateCommandBufferVK(const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateDescriptorPoolVK(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool);
    Result CreateBufferVK(const BufferVKDesc& bufferDesc, Buffer*& buffer);
    Result CreateTextureVK(const TextureVKDesc& textureVKDesc, Texture*& texture);
    Result CreateMemoryVK(const MemoryVKDesc& memoryVKDesc, Memory*& memory);
    Result CreateGraphicsPipelineVK(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
    Result CreateComputePipelineVK(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
    Result CreateQueryPoolVK(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool);
    Result CreateAccelerationStructureVK(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

    Result CreateCommandBufferD3D11(const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateBufferD3D11(const BufferD3D11Desc& bufferDesc, Buffer*& buffer);
    Result CreateTextureD3D11(const TextureD3D11Desc& textureDesc, Texture*& texture);

    Result CreateCommandBufferD3D12(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateDescriptorPoolD3D12(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool);
    Result CreateBufferD3D12(const BufferD3D12Desc& bufferDesc, Buffer*& buffer);
    Result CreateTextureD3D12(const TextureD3D12Desc& textureDesc, Texture*& texture);
    Result CreateMemoryD3D12(const MemoryD3D12Desc& memoryDesc, Memory*& memory);
    Result CreateAccelerationStructureD3D12(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);

    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const;
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);

    //================================================================================================================
    // DeviceBase
    //================================================================================================================
    const DeviceDesc& GetDesc() const;

    void Destroy();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(HelperInterface& table) const;
    Result FillFunctionTable(StreamerInterface& streamerInterface) const;
    Result FillFunctionTable(WrapperD3D11Interface& table) const;
    Result FillFunctionTable(WrapperD3D12Interface& table) const;
    Result FillFunctionTable(WrapperVKInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(RayTracingInterface& table) const;
    Result FillFunctionTable(MeshShaderInterface& table) const;
    Result FillFunctionTable(LowLatencyInterface& table) const;

private:
    Device& m_Device;
    String m_Name;
    CoreInterface m_CoreAPI = {};
    HelperInterface m_HelperAPI = {};
    StreamerInterface m_StreamerAPI = {};
    WrapperD3D11Interface m_WrapperD3D11API = {};
    WrapperD3D12Interface m_WrapperD3D12API = {};
    WrapperVKInterface m_WrapperVKAPI = {};
    SwapChainInterface m_SwapChainAPI = {};
    RayTracingInterface m_RayTracingAPI = {};
    MeshShaderInterface m_MeshShaderAPI = {};
    LowLatencyInterface m_LowLatencyAPI = {};
    std::array<CommandQueueVal*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    UnorderedMap<MemoryType, MemoryLocation> m_MemoryTypeMap;
    bool m_IsWrapperD3D11Supported = false;
    bool m_IsWrapperD3D12Supported = false;
    bool m_IsWrapperVKSupported = false;
    bool m_IsSwapChainSupported = false;
    bool m_IsRayTracingSupported = false;
    bool m_IsMeshShaderSupported = false;
    bool m_IsLowLatencySupported = false;
    Lock m_Lock;
};

} // namespace nri
