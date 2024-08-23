// © 2021 NVIDIA Corporation

Declare_PartiallyFillFunctionTable_Functions(D3D11);

#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((const DeviceD3D11&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferD3D11&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureD3D11&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceD3D11&)device).GetFormatSupport(format);
}

static void NRI_CALL GetBufferMemoryDesc(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceD3D11&)device).GetMemoryDesc(bufferDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceD3D11&)device).GetMemoryDesc(textureDesc, memoryLocation, memoryDesc);
}

static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return ((DeviceD3D11&)device).GetCommandQueue(commandQueueType, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    DeviceD3D11& device = ((CommandQueueD3D11&)commandQueue).GetDevice();
    return device.CreateCommandAllocator(commandQueue, commandAllocator);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceD3D11&)device).CreateImplementation<DescriptorPoolD3D11>(descriptorPool, descriptorPoolDesc);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D11&)device).CreateImplementation<BufferD3D11>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceD3D11&)device).CreateImplementation<TextureD3D11>(texture, textureDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceD3D11& device = ((const BufferD3D11*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(bufferView, bufferViewDesc);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D11& device = ((const TextureD3D11*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D11& device = ((const TextureD3D11*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceD3D11& device = ((const TextureD3D11*)textureViewDesc.texture)->GetDevice();
    return device.CreateImplementation<DescriptorD3D11>(textureView, textureViewDesc);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceD3D11&)device).CreateImplementation<DescriptorD3D11>(sampler, samplerDesc);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceD3D11&)device).CreateImplementation<PipelineLayoutD3D11>(pipelineLayout, pipelineLayoutDesc);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D11&)device).CreateImplementation<PipelineD3D11>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceD3D11&)device).CreateImplementation<PipelineD3D11>(pipeline, computePipelineDesc);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceD3D11&)device).CreateImplementation<QueryPoolD3D11>(queryPool, queryPoolDesc);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceD3D11&)device).CreateImplementation<FenceD3D11>(fence, initialValue);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorD3D11*)&commandAllocator);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return;

    CommandBufferHelper& commandBufferHelper = (CommandBufferHelper&)commandBuffer;
    Destroy(commandBufferHelper.GetStdAllocator(), &commandBufferHelper);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Destroy((DescriptorPoolD3D11*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferD3D11*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureD3D11*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    Destroy((DescriptorD3D11*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Destroy((PipelineLayoutD3D11*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    Destroy((PipelineD3D11*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    Destroy((QueryPoolD3D11*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceD3D11*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceD3D11&)device).CreateImplementation<MemoryD3D11>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D11&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceD3D11&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryD3D11*)&memory);
}

static void NRI_CALL SetDeviceDebugName(Device& device, const char* name) {
    ((DeviceD3D11&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name) {
    ((PipelineD3D11&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name) {
    ((PipelineLayoutD3D11&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name) {
    ((MemoryD3D11&)memory).SetDebugName(name);
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;

    return ((DeviceD3D11&)device).GetNativeObject();
}

void Core_CommandBufferEmu_PartiallyFillFunctionTable(CoreInterface& table);

Result DeviceD3D11::FillFunctionTable(CoreInterface& table) const {
    table = {};
    Core_Device_PartiallyFillFunctionTableD3D11(table);
    Core_Buffer_PartiallyFillFunctionTableD3D11(table);
    Core_CommandAllocator_PartiallyFillFunctionTableD3D11(table);
    Core_CommandQueue_PartiallyFillFunctionTableD3D11(table);
    Core_Descriptor_PartiallyFillFunctionTableD3D11(table);
    Core_DescriptorPool_PartiallyFillFunctionTableD3D11(table);
    Core_DescriptorSet_PartiallyFillFunctionTableD3D11(table);
    Core_Fence_PartiallyFillFunctionTableD3D11(table);
    Core_QueryPool_PartiallyFillFunctionTableD3D11(table);
    Core_Texture_PartiallyFillFunctionTableD3D11(table);

    if (m_IsDeferredContextEmulated)
        Core_CommandBufferEmu_PartiallyFillFunctionTable(table);
    else
        Core_CommandBuffer_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Helper  ]

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D11.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    HelperDeviceMemoryAllocator allocator(deviceD3D11.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    uint64_t luid = ((DeviceD3D11&)device).GetDesc().adapterDesc.luid;

    return QueryVideoMemoryInfoDXGI(luid, memoryLocation, videoMemoryInfo);
}

Result DeviceD3D11::FillFunctionTable(HelperInterface& table) const {
    table = {};
    Helper_CommandQueue_PartiallyFillFunctionTableD3D11(table);
    Helper_Device_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  LowLatency  ]

Result DeviceD3D11::FillFunctionTable(LowLatencyInterface& table) const {
    table = {};
    if (!m_Desc.isLowLatencySupported)
        return Result::UNSUPPORTED;

    LowLatency_CommandQueue_PartiallyFillFunctionTableD3D11(table);
    LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  ResourceAllocator  ]

static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    Result result = ((DeviceD3D11&)device).CreateImplementation<BufferD3D11>(buffer, bufferDesc.desc);
    if (result == Result::SUCCESS) {
        result = ((BufferD3D11*)buffer)->Create(bufferDesc.memoryLocation, bufferDesc.memoryPriority);
        if (result != Result::SUCCESS) {
            Destroy(((DeviceD3D11&)device).GetStdAllocator(), (BufferD3D11*)buffer);
            buffer = nullptr;
        }
    }

    return result;
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    Result result = ((DeviceD3D11&)device).CreateImplementation<TextureD3D11>(texture, textureDesc.desc);
    if (result == Result::SUCCESS) {
        result = ((TextureD3D11*)texture)->Create(textureDesc.memoryLocation, textureDesc.memoryPriority);
        if (result != Result::SUCCESS) {
            Destroy(((DeviceD3D11&)device).GetStdAllocator(), (TextureD3D11*)texture);
            texture = nullptr;
        }
    }

    return result;
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    MaybeUnused(device, accelerationStructureDesc, accelerationStructure);

    return Result::UNSUPPORTED;
}

Result DeviceD3D11::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table = {};
    ResourceAllocator_Device_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Streamer  ]

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;
    StreamerImpl* impl = Allocate<StreamerImpl>(deviceD3D11.GetStdAllocator(), device, deviceD3D11.GetCoreInterface());
    Result result = impl->Create(streamerDesc);

    if (result != Result::SUCCESS) {
        Destroy(deviceD3D11.GetStdAllocator(), impl);
        streamer = nullptr;
    } else
        streamer = (Streamer*)impl;

    return result;
}

static void DestroyStreamer(Streamer& streamer) {
    Destroy((StreamerImpl*)&streamer);
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

Result DeviceD3D11::FillFunctionTable(StreamerInterface& table) const {
    table = {};
    Streamer_Device_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceD3D11&)device).CreateImplementation<SwapChainD3D11>(swapChain, swapChainDesc);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainD3D11*)&swapChain);
}

Result DeviceD3D11::FillFunctionTable(SwapChainInterface& table) const {
    table = {};
    SwapChain_Device_PartiallyFillFunctionTableD3D11(table);
    SwapChain_SwapChain_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  WrapperD3D11  ]

static Result NRI_CALL CreateCommandBufferD3D11(Device& device, const CommandBufferD3D11Desc& commandBufferDesc, CommandBuffer*& commandBuffer) {
    DeviceD3D11& deviceD3D11 = (DeviceD3D11&)device;

    return ::CreateCommandBuffer(deviceD3D11, commandBufferDesc.d3d11DeviceContext, commandBuffer);
}

static Result NRI_CALL CreateBufferD3D11(Device& device, const BufferD3D11Desc& bufferDesc, Buffer*& buffer) {
    return ((DeviceD3D11&)device).CreateImplementation<BufferD3D11>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTextureD3D11(Device& device, const TextureD3D11Desc& textureDesc, Texture*& texture) {
    return ((DeviceD3D11&)device).CreateImplementation<TextureD3D11>(texture, textureDesc);
}

Result DeviceD3D11::FillFunctionTable(WrapperD3D11Interface& table) const {
    table = {};
    WrapperD3D11_Device_PartiallyFillFunctionTableD3D11(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

Define_Core_Device_PartiallyFillFunctionTable(D3D11);
Define_Helper_Device_PartiallyFillFunctionTable(D3D11);
Define_Streamer_Device_PartiallyFillFunctionTable(D3D11);
Define_SwapChain_Device_PartiallyFillFunctionTable(D3D11);
Define_ResourceAllocator_Device_PartiallyFillFunctionTable(D3D11);
Define_WrapperD3D11_Device_PartiallyFillFunctionTable(D3D11);
