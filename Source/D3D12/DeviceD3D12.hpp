// © 2021 NVIDIA Corporation

Declare_PartiallyFillFunctionTable_Functions(D3D12);

#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((const DeviceD3D12&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferD3D12&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureD3D12&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceD3D12&)device).GetFormatSupport(format);
}

static void NRI_CALL GetBufferMemoryDesc(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, bufferDesc);

    ((const DeviceD3D12&)device).GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    D3D12_RESOURCE_DESC desc = {};
    GetResourceDesc(&desc, textureDesc);

    ((const DeviceD3D12&)device).GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return ((DeviceD3D12&)device).GetCommandQueue(commandQueueType, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    DeviceD3D12& device = ((CommandQueueD3D12&)commandQueue).GetDevice();
    return device.CreateImplementation<CommandAllocatorD3D12>(commandAllocator, commandQueue);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorPoolD3D12>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceD3D12& device = ((const BufferD3D12*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D12& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorD3D12>(sampler, samplerDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineLayoutD3D12>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, computePipelineDesc);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceD3D12&)device).CreateImplementation<FenceD3D12>(fence, initialValue);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceD3D12&)device).CreateImplementation<QueryPoolD3D12>(queryPool, queryPoolDesc);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    Destroy((CommandBufferD3D12*)&commandBuffer);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorD3D12*)&commandAllocator);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Destroy((DescriptorPoolD3D12*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferD3D12*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureD3D12*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    Destroy((DescriptorD3D12*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Destroy((PipelineLayoutD3D12*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    Destroy((PipelineD3D12*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    Destroy((QueryPoolD3D12*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceD3D12*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceD3D12&)device).CreateImplementation<MemoryD3D12>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D12&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D12&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryD3D12*)&memory);
}

static void NRI_CALL SetDeviceDebugName(Device& device, const char* name) {
    ((DeviceD3D12&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name) {
    ((PipelineD3D12&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name) {
    ((PipelineLayoutD3D12&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name) {
    ((MemoryD3D12&)memory).SetDebugName(name);
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;

    return ((DeviceD3D12&)device).GetNativeObject();
}

Result DeviceD3D12::FillFunctionTable(CoreInterface& table) const {
    table = {};
    Core_Device_PartiallyFillFunctionTableD3D12(table);
    Core_Buffer_PartiallyFillFunctionTableD3D12(table);
    Core_CommandAllocator_PartiallyFillFunctionTableD3D12(table);
    Core_CommandBuffer_PartiallyFillFunctionTableD3D12(table);
    Core_CommandQueue_PartiallyFillFunctionTableD3D12(table);
    Core_Descriptor_PartiallyFillFunctionTableD3D12(table);
    Core_DescriptorPool_PartiallyFillFunctionTableD3D12(table);
    Core_DescriptorSet_PartiallyFillFunctionTableD3D12(table);
    Core_Fence_PartiallyFillFunctionTableD3D12(table);
    Core_QueryPool_PartiallyFillFunctionTableD3D12(table);
    Core_Texture_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Helper  ]

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D12.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D12.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    uint64_t luid = ((DeviceD3D12&)device).GetDesc().adapterDesc.luid;

    return QueryVideoMemoryInfoDXGI(luid, memoryLocation, videoMemoryInfo);
}

Result DeviceD3D12::FillFunctionTable(HelperInterface& table) const {
    table = {};
    Helper_CommandQueue_PartiallyFillFunctionTableD3D12(table);
    Helper_Device_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  LowLatency  ]

Result DeviceD3D12::FillFunctionTable(LowLatencyInterface& table) const {
    table = {};
    if (!m_Desc.isLowLatencySupported)
        return Result::UNSUPPORTED;

    LowLatency_CommandQueue_PartiallyFillFunctionTableD3D12(table);
    LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  MeshShader  ]

Result DeviceD3D12::FillFunctionTable(MeshShaderInterface& table) const {
    table = {};
    if (!m_Desc.isMeshShaderSupported)
        return Result::UNSUPPORTED;

    MeshShader_CommandBuffer_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  RayTracing  ]

static void NRI_CALL GetAccelerationStructureMemoryDesc(
    const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((DeviceD3D12&)device).GetAccelerationStructureMemoryDesc(accelerationStructureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D12&)device).CreateImplementation<PipelineD3D12>(pipeline, rayTracingPipelineDesc);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Destroy((AccelerationStructureD3D12*)&accelerationStructure);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D12&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

void FillFunctionTablePipelineD3D12(RayTracingInterface& table);

Result DeviceD3D12::FillFunctionTable(RayTracingInterface& table) const {
    table = {};
    if (!m_Desc.isRayTracingSupported)
        return Result::UNSUPPORTED;

    FillFunctionTablePipelineD3D12(table);
    RayTracing_AccelerationStructure_PartiallyFillFunctionTableD3D12(table);
    RayTracing_CommandBuffer_PartiallyFillFunctionTableD3D12(table);
    RayTracing_Device_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  ResourceAllocator  ]

static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

Result DeviceD3D12::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table = {};
    ResourceAllocator_Device_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Streamer  ]

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceD3D12& deviceD3D12 = (DeviceD3D12&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceD3D12.GetStdAllocator(), device, deviceD3D12.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D12.GetStdAllocator(), impl);
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

Result DeviceD3D12::FillFunctionTable(StreamerInterface& table) const {
    table = {};
    Streamer_Device_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceD3D12&)device).CreateImplementation<SwapChainD3D12>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainD3D12*)&swapChain);
}

Result DeviceD3D12::FillFunctionTable(SwapChainInterface& table) const {
    table = {};
    if (!m_Desc.isSwapChainSupported)
        return Result::UNSUPPORTED;

    SwapChain_Device_PartiallyFillFunctionTableD3D12(table);
    SwapChain_SwapChain_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  WrapperD3D12  ]

static Result NRI_CALL CreateCommandBufferD3D12(Device& device, const CommandBufferD3D12Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceD3D12&)device).CreateImplementation<CommandBufferD3D12>(commandBuffer, commandBufferDesc);
}

static Result NRI_CALL CreateDescriptorPoolD3D12(Device& device, const DescriptorPoolD3D12Desc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D12&)device).CreateImplementation<DescriptorPoolD3D12>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBufferD3D12(Device& device, const BufferD3D12Desc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D12&)device).CreateImplementation<BufferD3D12>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTextureD3D12(Device& device, const TextureD3D12Desc& textureDesc, Texture*& texture) {
    return ((DeviceD3D12&)device).CreateImplementation<TextureD3D12>(texture, textureDesc);
}

static Result NRI_CALL CreateMemoryD3D12(Device& device, const MemoryD3D12Desc& memoryDesc, Memory*& memory) {
    return ((DeviceD3D12&)device).CreateImplementation<MemoryD3D12>(memory, memoryDesc);
}

static Result NRI_CALL CreateAccelerationStructureD3D12(
    Device& device, const AccelerationStructureD3D12Desc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceD3D12&)device).CreateImplementation<AccelerationStructureD3D12>(accelerationStructure, accelerationStructureDesc);
}

Result DeviceD3D12::FillFunctionTable(WrapperD3D12Interface& table) const {
    table = {};
    WrapperD3D12_Device_PartiallyFillFunctionTableD3D12(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

Define_Core_Device_PartiallyFillFunctionTable(D3D12);
Define_Helper_Device_PartiallyFillFunctionTable(D3D12);
Define_RayTracing_Device_PartiallyFillFunctionTable(D3D12);
Define_Streamer_Device_PartiallyFillFunctionTable(D3D12);
Define_SwapChain_Device_PartiallyFillFunctionTable(D3D12);
Define_ResourceAllocator_Device_PartiallyFillFunctionTable(D3D12);
Define_WrapperD3D12_Device_PartiallyFillFunctionTable(D3D12);
