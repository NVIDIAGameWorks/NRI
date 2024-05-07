// © 2021 NVIDIA Corporation

#pragma once

struct ID3D11Device5;
typedef ID3D11Device5 ID3D11DeviceBest;

namespace nri {

struct CommandQueueD3D11;

struct DeviceD3D11 final : public DeviceBase {
    DeviceD3D11(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator);
    ~DeviceD3D11();

    inline ID3D11DeviceBest* GetNativeObject() const {
        return m_Device;
    }

    inline ID3D11DeviceBest* operator->() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
    }

    inline const d3d11::Ext* GetExt() const {
        return &m_Ext;
    }

    inline IDXGIAdapter* GetAdapter() const {
        return m_Adapter;
    }

    inline ID3D11DeviceContextBest* GetImmediateContext() {
        return m_ImmediateContext;
    }

    inline uint8_t GetImmediateContextVersion() {
        return m_ImmediateContextVersion;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }

    inline bool IsDeferredContextEmulated() const {
        return m_IsDeferredContextEmulated;
    }

    Result Create(const DeviceCreationDesc& deviceCreationDesc, ID3D11Device* precreatedDevice, AGSContext* agsContext, bool isNVAPILoadedInApp);

    inline void EnterCriticalSection() {
        if (m_Multithread)
            m_Multithread->Enter();
        else
            ::EnterCriticalSection(&m_CriticalSection);
    }

    inline void LeaveCriticalSection() {
        if (m_Multithread)
            m_Multithread->Leave();
        else
            ::LeaveCriticalSection(&m_CriticalSection);
    }

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device, name);
        SET_D3D_DEBUG_OBJECT_NAME(m_ImmediateContext, name);
    }

    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    void DestroySwapChain(SwapChain& swapChain);

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
    Result CreateFence(uint64_t initialValue, Fence*& queueSemaphore);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroyFence(Fence& fence);
    Result AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void FreeMemory(Memory& memory);
    FormatSupportBits GetFormatSupport(Format format) const;
    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const;
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    void Destroy();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(WrapperD3D11Interface& table) const;
    Result FillFunctionTable(HelperInterface& helperInterface) const;
    Result FillFunctionTable(LowLatencyInterface& lowLatencyInterface) const;
    Result FillFunctionTable(StreamerInterface& streamerInterface) const;

private:
    void FillDesc(const AGSDX11ReturnedParams& agsParams);

    template <typename Implementation, typename Interface, typename... Args>
    Result CreateImplementation(Interface*& entity, const Args&... args);

private:
    d3d11::Ext m_Ext = {}; // don't sort: destructor must be called last!
    ComPtr<ID3D11DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    ComPtr<ID3D11DeviceContextBest> m_ImmediateContext;
    ComPtr<ID3D11Multithread> m_Multithread;
    Vector<CommandQueueD3D11> m_CommandQueues;
    DeviceDesc m_Desc = {};
    CRITICAL_SECTION m_CriticalSection = {}; // TODO: Lock?
    CoreInterface m_CoreInterface = {};
    uint8_t m_Version = 0;
    uint8_t m_ImmediateContextVersion = 0;
    bool m_IsWrapped = false;
    bool m_IsDeferredContextEmulated = false;
};

} // namespace nri
