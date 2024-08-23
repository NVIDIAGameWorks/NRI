// © 2021 NVIDIA Corporation

Declare_PartiallyFillFunctionTable_Functions(VK);

#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((DeviceVK&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferVK&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureVK&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceVK&)device).GetFormatSupport(format);
}

static void NRI_CALL GetBufferMemoryDesc(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceVK&)device).GetMemoryDesc(bufferDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceVK&)device).GetMemoryDesc(textureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return ((DeviceVK&)device).GetCommandQueue(commandQueueType, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    DeviceVK& device = ((CommandQueueVK&)commandQueue).GetDevice();
    return device.CreateImplementation<CommandAllocatorVK>(commandAllocator, commandQueue);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceVK& device = ((const BufferVK*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorVK>(sampler, samplerDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceVK&)device).CreateImplementation<PipelineLayoutVK>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, computePipelineDesc);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceVK&)device).CreateImplementation<QueryPoolVK>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceVK&)device).CreateImplementation<FenceVK>(fence, initialValue);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    Destroy((CommandBufferVK*)&commandBuffer);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorVK*)&commandAllocator);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Destroy((DescriptorPoolVK*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferVK*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureVK*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    Destroy((DescriptorVK*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Destroy((PipelineLayoutVK*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    Destroy((PipelineVK*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    Destroy((QueryPoolVK*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceVK*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceVK&)device).CreateImplementation<MemoryVK>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVK&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVK&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryVK*)&memory);
}

static void NRI_CALL SetDeviceDebugName(Device& device, const char* name) {
    ((DeviceVK&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name) {
    ((PipelineVK&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name) {
    ((PipelineLayoutVK&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name) {
    ((MemoryVK&)memory).SetDebugName(name);
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;

    return (VkDevice)((DeviceVK&)device);
}

Result DeviceVK::FillFunctionTable(CoreInterface& table) const {
    table = {};
    Core_Device_PartiallyFillFunctionTableVK(table);
    Core_Buffer_PartiallyFillFunctionTableVK(table);
    Core_CommandAllocator_PartiallyFillFunctionTableVK(table);
    Core_CommandBuffer_PartiallyFillFunctionTableVK(table);
    Core_CommandQueue_PartiallyFillFunctionTableVK(table);
    Core_Descriptor_PartiallyFillFunctionTableVK(table);
    Core_DescriptorPool_PartiallyFillFunctionTableVK(table);
    Core_DescriptorSet_PartiallyFillFunctionTableVK(table);
    Core_Fence_PartiallyFillFunctionTableVK(table);
    Core_QueryPool_PartiallyFillFunctionTableVK(table);
    Core_Texture_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Helper  ]

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    HelperDeviceMemoryAllocator allocator(deviceVK.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    HelperDeviceMemoryAllocator allocator(deviceVK.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    return ((DeviceVK&)device).QueryVideoMemoryInfo(memoryLocation, videoMemoryInfo);
}

Result DeviceVK::FillFunctionTable(HelperInterface& helperInterface) const {
    helperInterface = {};
    Helper_CommandQueue_PartiallyFillFunctionTableVK(helperInterface);
    Helper_Device_PartiallyFillFunctionTableVK(helperInterface);

    return ValidateFunctionTable(helperInterface);
}

#pragma endregion

#pragma region[  LowLatency  ]

Result DeviceVK::FillFunctionTable(LowLatencyInterface& table) const {
    table = {};
    if (!m_Desc.isLowLatencySupported)
        return Result::UNSUPPORTED;

    LowLatency_CommandQueue_PartiallyFillFunctionTableVK(table);
    LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  MeshShader  ]

Result DeviceVK::FillFunctionTable(MeshShaderInterface& table) const {
    table = {};
    if (!m_Desc.isMeshShaderSupported)
        return Result::UNSUPPORTED;

    MeshShader_CommandBuffer_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  RayTracing  ]

static void NRI_CALL GetAccelerationStructureMemoryDesc(
    const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceVK&)device).GetMemoryDesc(accelerationStructureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, pipelineDesc);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVK&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Destroy((AccelerationStructureVK*)&accelerationStructure);
}

void FillFunctionTablePipelineVK(RayTracingInterface& table);

Result DeviceVK::FillFunctionTable(RayTracingInterface& table) const {
    table = {};
    if (!m_Desc.isRayTracingSupported)
        return Result::UNSUPPORTED;

    FillFunctionTablePipelineVK(table);
    RayTracing_CommandBuffer_PartiallyFillFunctionTableVK(table);
    RayTracing_AccelerationStructure_PartiallyFillFunctionTableVK(table);
    RayTracing_Device_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  ResourceAllocator  ]

static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

Result DeviceVK::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table = {};
    ResourceAllocator_Device_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Streamer  ]

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceVK& deviceVK = (DeviceVK&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceVK.GetStdAllocator(), device, deviceVK.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceVK.GetStdAllocator(), impl);
        streamer = nullptr;
    } else
        streamer = (Streamer*)impl;

    return result;
}

static void DestroyStreamer(Streamer& streamer) {
    Destroy(((DeviceBase&)((StreamerImpl&)streamer).GetDevice()).GetStdAllocator(), (StreamerImpl*)&streamer);
}

static Buffer* GetStreamerConstantBuffer(Streamer& streamer) {
    return ((StreamerImpl&)streamer).GetConstantBuffer();
}

static uint32_t UpdateStreamerConstantBuffer(Streamer& streamer, const void* data, uint32_t dataSize) {
    return ((StreamerImpl&)streamer).UpdateStreamerConstantBuffer(data, dataSize);
}

static uint64_t AddStreamerBufferUpdateRequest(Streamer& streamer, const BufferUpdateRequestDesc& bufferUpdateRequestDesc) {
    return ((StreamerImpl&)streamer).AddStreamerBufferUpdateRequest(bufferUpdateRequestDesc);
}

static uint64_t AddStreamerTextureUpdateRequest(Streamer& streamer, const TextureUpdateRequestDesc& textureUpdateRequestDesc) {
    return ((StreamerImpl&)streamer).AddStreamerTextureUpdateRequest(textureUpdateRequestDesc);
}

static Result CopyStreamerUpdateRequests(Streamer& streamer) {
    return ((StreamerImpl&)streamer).CopyStreamerUpdateRequests();
}

static Buffer* GetStreamerDynamicBuffer(Streamer& streamer) {
    return ((StreamerImpl&)streamer).GetDynamicBuffer();
}

static void CmdUploadStreamerUpdateRequests(CommandBuffer& commandBuffer, Streamer& streamer) {
    ((StreamerImpl&)streamer).CmdUploadStreamerUpdateRequests(commandBuffer);
}

Result DeviceVK::FillFunctionTable(StreamerInterface& table) const {
    table = {};
    Streamer_Device_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceVK&)device).CreateImplementation<SwapChainVK>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainVK*)&swapChain);
}

Result DeviceVK::FillFunctionTable(SwapChainInterface& table) const {
    table = {};
    SwapChain_Device_PartiallyFillFunctionTableVK(table);
    SwapChain_SwapChain_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  WrapperVK  ]

static Result NRI_CALL CreateCommandQueueVK(Device& device, const CommandQueueVKDesc& commandQueueVKDesc, CommandQueue*& commandQueue) {
    return ((DeviceVK&)device).CreateCommandQueue(commandQueueVKDesc, commandQueue);
}

static Result NRI_CALL CreateCommandAllocatorVK(Device& device, const CommandAllocatorVKDesc& commandAllocatorDesc, CommandAllocator*& commandAllocator) {
    return ((DeviceVK&)device).CreateImplementation<CommandAllocatorVK>(commandAllocator, commandAllocatorDesc);
}

static Result NRI_CALL CreateCommandBufferVK(Device& device, const CommandBufferVKDesc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceVK&)device).CreateImplementation<CommandBufferVK>(commandBuffer, commandBufferDesc);
}

static Result NRI_CALL CreateDescriptorPoolVK(Device& device, const DescriptorPoolVKDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVK&)device).CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBufferVK(Device& device, const BufferVKDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVK&)device).CreateImplementation<BufferVK>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTextureVK(Device& device, const TextureVKDesc& textureDesc, Texture*& texture) {
    return ((DeviceVK&)device).CreateImplementation<TextureVK>(texture, textureDesc);
}

static Result NRI_CALL CreateMemoryVK(Device& device, const MemoryVKDesc& memoryDesc, Memory*& memory) {
    return ((DeviceVK&)device).CreateImplementation<MemoryVK>(memory, memoryDesc);
}

static Result NRI_CALL CreateGraphicsPipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline);
}

static Result NRI_CALL CreateComputePipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVK&)device).CreateImplementation<PipelineVK>(pipeline, VK_PIPELINE_BIND_POINT_COMPUTE, vkPipeline);
}

static Result NRI_CALL CreateQueryPoolVK(Device& device, const QueryPoolVKDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceVK&)device).CreateImplementation<QueryPoolVK>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateAccelerationStructureVK(Device& device, const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVK&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

static VKHandle NRI_CALL GetPhysicalDeviceVK(const Device& device) {
    return (VkPhysicalDevice)((DeviceVK&)device);
}

static VKHandle NRI_CALL GetInstanceVK(const Device& device) {
    return (VkInstance)((DeviceVK&)device);
}

static void* NRI_CALL GetInstanceProcAddrVK(const Device& device) {
    return ((DeviceVK&)device).GetDispatchTable().GetInstanceProcAddr;
}

static void* NRI_CALL GetDeviceProcAddrVK(const Device& device) {
    return ((DeviceVK&)device).GetDispatchTable().GetDeviceProcAddr;
}

Result DeviceVK::FillFunctionTable(WrapperVKInterface& table) const {
    table = {};
    WrapperVK_Device_PartiallyFillFunctionTableVK(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

Define_Core_Device_PartiallyFillFunctionTable(VK);
Define_Helper_Device_PartiallyFillFunctionTable(VK);
Define_RayTracing_Device_PartiallyFillFunctionTable(VK);
Define_Streamer_Device_PartiallyFillFunctionTable(VK);
Define_SwapChain_Device_PartiallyFillFunctionTable(VK);
Define_ResourceAllocator_Device_PartiallyFillFunctionTable(VK);
Define_WrapperVK_Device_PartiallyFillFunctionTable(VK);
