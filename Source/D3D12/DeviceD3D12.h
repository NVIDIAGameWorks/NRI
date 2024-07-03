// © 2021 NVIDIA Corporation

#pragma once

struct IDXGIAdapter;
struct ID3D12DescriptorHeap;
struct ID3D12CommandSignature;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

#ifdef NRI_USE_AGILITY_SDK
struct ID3D12Device14;
typedef ID3D12Device14 ID3D12DeviceBest;
#else
struct ID3D12Device5;
typedef ID3D12Device5 ID3D12DeviceBest;
#endif

namespace nri {

struct CommandQueueD3D12;

constexpr size_t DESCRIPTOR_HEAP_TYPE_NUM = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
constexpr uint32_t DESCRIPTORS_BATCH_SIZE = 1024;

struct DeviceD3D12 final : public DeviceBase {
    DeviceD3D12(const CallbackInterface& callbacks, StdAllocator<uint8_t>& stdAllocator);
    ~DeviceD3D12();

    inline ID3D12DeviceBest* GetNativeObject() const {
        return m_Device;
    }

    inline ID3D12DeviceBest* operator->() const {
        return m_Device;
    }

    inline uint8_t GetVersion() const {
        return m_Version;
    }

    inline const d3d12::Ext* GetExt() const {
        return &m_Ext;
    }

    inline IDXGIAdapter* GetAdapter() const {
        return m_Adapter;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }

    inline void FreeDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHandle& descriptorHandle) {
        ExclusiveScope lock(m_FreeDescriptorLocks[type]);
        auto& freeDescriptors = m_FreeDescriptors[type];
        freeDescriptors.push_back(descriptorHandle);
    }

    template <typename Implementation, typename Interface, typename... Args>
    Result CreateImplementation(Interface*& entity, const Args&... args);

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationD3D12Desc& deviceCreationD3D12Desc);
    Result CreateDefaultDrawSignatures(ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation);
    Result CreateCpuOnlyVisibleDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
    Result GetDescriptorHandle(D3D12_DESCRIPTOR_HEAP_TYPE type, DescriptorHandle& descriptorHandle);
    DescriptorPointerCPU GetDescriptorPointerCPU(const DescriptorHandle& descriptorHandle);
    void GetMemoryInfo(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc, MemoryDesc& memoryDesc) const;

    ID3D12CommandSignature* GetDrawCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawIndexedCommandSignature(uint32_t stride, ID3D12RootSignature* rootSignature);
    ID3D12CommandSignature* GetDrawMeshCommandSignature(uint32_t stride);
    ID3D12CommandSignature* GetDispatchRaysCommandSignature() const;
    ID3D12CommandSignature* GetDispatchCommandSignature() const;

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name) {
        SET_D3D_DEBUG_OBJECT_NAME(m_Device, name);
    }

    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    void DestroySwapChain(SwapChain& swapChain);

    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result CreateCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result CreateCommandQueue(void* d3d12commandQueue, CommandQueueD3D12*& commandQueue);
    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateDescriptor(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateDescriptor(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateDescriptor(const AccelerationStructure& accelerationStructure, Descriptor*& accelerationStructureView);
    Result CreateDescriptor(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline);
    Result CreateFence(uint64_t initialValue, Fence*& fence);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateCommandBuffer(const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateDescriptorPool(const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferD3D12Desc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureD3D12Desc& textureDesc, Texture*& texture);
    Result CreateMemory(const MemoryD3D12Desc& memoryDesc, Memory*& memory);
    Result CreateAccelerationStructure(const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyFence(Fence& queueSemaphore);
    void DestroyQueryPool(QueryPool& queryPool);
    Result AllocateMemory(const MemoryType memoryType, uint64_t size, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void FreeMemory(Memory& memory);
    FormatSupportBits GetFormatSupport(Format format) const;
    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const;
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);

    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    inline const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    void Destroy();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(WrapperD3D12Interface& wrapperD3D12Interface) const;
    Result FillFunctionTable(RayTracingInterface& rayTracingInterface) const;
    Result FillFunctionTable(MeshShaderInterface& meshShaderInterface) const;
    Result FillFunctionTable(HelperInterface& helperInterface) const;
    Result FillFunctionTable(LowLatencyInterface& lowLatencyInterface) const;
    Result FillFunctionTable(StreamerInterface& streamerInterface) const;

private:
    void FillDesc(bool enableDrawParametersEmulation);
    MemoryType GetMemoryType(MemoryLocation memoryLocation, const D3D12_RESOURCE_DESC& resourceDesc) const;
    ComPtr<ID3D12CommandSignature> CreateCommandSignature(
        D3D12_INDIRECT_ARGUMENT_TYPE indirectArgumentType, uint32_t stride, ID3D12RootSignature* rootSignature, bool enableDrawParametersEmulation = false);

private:
    d3d12::Ext m_Ext = {}; // don't sort: destructor must be called last!
    ComPtr<ID3D12DeviceBest> m_Device;
    ComPtr<IDXGIAdapter> m_Adapter;
    std::array<CommandQueueD3D12*, (uint32_t)CommandQueueType::MAX_NUM> m_CommandQueues = {};
    Vector<DescriptorHeapDesc> m_DescriptorHeaps;
    Vector<Vector<DescriptorHandle>> m_FreeDescriptors;
    DeviceDesc m_Desc = {};
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawCommandSignatures;
    UnorderedMap<uint64_t, ComPtr<ID3D12CommandSignature>> m_DrawIndexedCommandSignatures;
    UnorderedMap<uint32_t, ComPtr<ID3D12CommandSignature>> m_DrawMeshCommandSignatures;
    ComPtr<ID3D12CommandSignature> m_DispatchCommandSignature;
    ComPtr<ID3D12CommandSignature> m_DispatchRaysCommandSignature;
    CoreInterface m_CoreInterface = {};
    uint8_t m_Version = 0;
    bool m_IsWrapped = false;
    std::array<Lock, DESCRIPTOR_HEAP_TYPE_NUM> m_FreeDescriptorLocks;
    Lock m_DescriptorHeapLock;
    Lock m_QueueLock;
};

} // namespace nri
