// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct CommandQueueVK;

struct DeviceVK final : public DeviceBase {
    inline operator VkDevice() const {
        return m_Device;
    }

    inline operator VkPhysicalDevice() const {
        return m_PhysicalDevice;
    }

    inline operator VkInstance() const {
        return m_Instance;
    }

    inline const DispatchTable& GetDispatchTable() const {
        return m_VK;
    }

    inline const VkAllocationCallbacks* GetAllocationCallbacks() const {
        return m_AllocationCallbackPtr;
    }

    inline const std::array<uint32_t, (uint32_t)CommandQueueType::MAX_NUM>& GetQueueFamilyIndices() const {
        return m_FamilyIndices;
    }

    inline const SPIRVBindingOffsets& GetSPIRVBindingOffsets() const {
        return m_SPIRVBindingOffsets;
    }

    inline const CoreInterface& GetCoreInterface() const {
        return m_CoreInterface;
    }

    inline bool IsConcurrentSharingModeEnabledForBuffers() const {
        return m_ConcurrentSharingModeQueueIndices.size() > 1;
    }

    inline bool IsConcurrentSharingModeEnabledForImages() const {
        return m_ConcurrentSharingModeQueueIndices.size() > 1;
    }

    inline const Vector<uint32_t>& GetConcurrentSharingModeQueueIndices() const {
        return m_ConcurrentSharingModeQueueIndices;
    }

    DeviceVK(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator);
    ~DeviceVK();

    Result Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationVKDesc& deviceCreationVKDesc, bool isWrapper);
    void FillCreateInfo(const BufferDesc& bufferDesc, VkBufferCreateInfo& info) const;
    void FillCreateInfo(const TextureDesc& bufferDesc, VkImageCreateInfo& info) const;
    void GetMemoryDesc(const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    void GetMemoryDesc(const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const;
    bool GetMemoryType(MemoryLocation memoryLocation, uint32_t memoryTypeMask, MemoryTypeInfo& memoryTypeInfo) const;
    bool GetMemoryTypeByIndex(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const;
    void SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);
    Result GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue);
    Result CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator);
    Result CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureDesc& textureDesc, Texture*& texture);
    Result CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView);
    Result CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView);
    Result CreateSampler(const SamplerDesc& samplerDesc, Descriptor*& sampler);
    Result CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout);
    Result CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline);
    Result CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline);
    Result CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool);
    Result CreateFence(uint64_t initialValue, Fence*& fence);
    Result CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain);
    Result CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline);
    Result CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    Result CreateCommandQueue(const CommandQueueVKDesc& commandQueueDesc, CommandQueue*& commandQueue);
    Result CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator);
    Result CreateCommandBuffer(const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer);
    Result CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool);
    Result CreateBuffer(const BufferVKDesc& bufferDesc, Buffer*& buffer);
    Result CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture);
    Result CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory);
    Result CreateGraphicsPipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
    Result CreateComputePipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline);
    Result CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool);
    Result CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure);
    void DestroyCommandAllocator(CommandAllocator& commandAllocator);
    void DestroyDescriptorPool(DescriptorPool& descriptorPool);
    void DestroyBuffer(Buffer& buffer);
    void DestroyTexture(Texture& texture);
    void DestroyDescriptor(Descriptor& descriptor);
    void DestroyPipelineLayout(PipelineLayout& pipelineLayout);
    void DestroyPipeline(Pipeline& pipeline);
    void DestroyQueryPool(QueryPool& queryPool);
    void DestroyFence(Fence& fence);
    void DestroySwapChain(SwapChain& swapChain);
    void DestroyAccelerationStructure(AccelerationStructure& accelerationStructure);
    Result AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory);
    Result BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    Result BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum);
    void FreeMemory(Memory& memory);
    FormatSupportBits GetFormatSupport(Format format) const;
    uint32_t CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const;
    Result AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations);
    Result QueryVideoMemoryInfo(MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) const;

    //================================================================================================================
    // DeviceBase
    //================================================================================================================

    const DeviceDesc& GetDesc() const {
        return m_Desc;
    }

    void Destroy();
    Result FillFunctionTable(CoreInterface& table) const;
    Result FillFunctionTable(SwapChainInterface& table) const;
    Result FillFunctionTable(WrapperVKInterface& wrapperVKInterface) const;
    Result FillFunctionTable(RayTracingInterface& rayTracingInterface) const;
    Result FillFunctionTable(MeshShaderInterface& meshShaderInterface) const;
    Result FillFunctionTable(HelperInterface& helperInterface) const;
    Result FillFunctionTable(LowLatencyInterface& lowLatencyInterface) const;
    Result FillFunctionTable(StreamerInterface& streamerInterface) const;

private:
    void FilterInstanceLayers(Vector<const char*>& layers);
    void ProcessInstanceExtensions(Vector<const char*>& desiredInstanceExts);
    void ProcessDeviceExtensions(Vector<const char*>& desiredDeviceExts, bool disableRayTracing);
    void FillFamilyIndices(bool useEnabledFamilyIndices, const uint32_t* enabledFamilyIndices, uint32_t familyIndexNum);
    void CreateCommandQueues();
    void ReportDeviceGroupInfo();
    void GetAdapterDesc();
    Result CreateInstance(bool enableAPIValidation, const Vector<const char*>& desiredInstanceExts);
    Result ResolvePreInstanceDispatchTable();
    Result ResolveInstanceDispatchTable(const Vector<const char*>& desiredInstanceExts);
    Result ResolveDispatchTable(const Vector<const char*>& desiredDeviceExts);

    template <typename Implementation, typename Interface, typename... Args>
    Result CreateImplementation(Interface*& entity, const Args&... args);

public:
    bool m_IsDescriptorIndexingSupported = false;
    bool m_IsDeviceAddressSupported = false;
    bool m_IsSwapChainMutableFormatSupported = false;
    bool m_IsPresentIdSupported = false;
    bool m_IsPresentWaitSupported = false;
    bool m_IsLowLatencySupported = false;
    bool m_IsMemoryBudgetSupported = false;

private:
    Vector<uint32_t> m_ConcurrentSharingModeQueueIndices;
    VkPhysicalDevice m_PhysicalDevice = nullptr;
    std::array<uint32_t, (uint32_t)CommandQueueType::MAX_NUM> m_FamilyIndices = {};
    std::array<CommandQueueVK*, (uint32_t)CommandQueueType::MAX_NUM> m_Queues = {};
    DispatchTable m_VK = {};
    DeviceDesc m_Desc = {};
    VkPhysicalDeviceMemoryProperties m_MemoryProps = {};
    VkAllocationCallbacks m_AllocationCallbacks = {};
    SPIRVBindingOffsets m_SPIRVBindingOffsets = {};
    CoreInterface m_CoreInterface = {};
    Library* m_Loader = nullptr;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkAllocationCallbacks* m_AllocationCallbackPtr = nullptr;
    VkDebugUtilsMessengerEXT m_Messenger = VK_NULL_HANDLE;
    bool m_OwnsNativeObjects = false;
    Lock m_Lock;
};

} // namespace nri
