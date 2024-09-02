// © 2021 NVIDIA Corporation

Declare_PartiallyFillFunctionTable_Functions(Val);

#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((const DeviceVal&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferVal&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureVal&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return ((const DeviceVal&)device).GetFormatSupport(format);
}

static void NRI_CALL GetBufferMemoryDesc(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;
    deviceVal.GetCoreInterface().GetBufferMemoryDesc(deviceVal.GetImpl(), bufferDesc, memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static void NRI_CALL GetTextureMemoryDesc(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;
    deviceVal.GetCoreInterface().GetTextureMemoryDesc(deviceVal.GetImpl(), textureDesc, memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return ((DeviceVal&)device).GetCommandQueue(commandQueueType, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    return GetDeviceVal(commandQueue).CreateCommandAllocator(commandQueue, commandAllocator);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVal&)device).CreateDescriptorPool(descriptorPoolDesc, descriptorPool);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferDesc, buffer);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureDesc, texture);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    DeviceVal& device = GetDeviceVal(*bufferViewDesc.buffer);

    return device.CreateDescriptor(bufferViewDesc, bufferView);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVal& device = GetDeviceVal(*textureViewDesc.texture);

    return device.CreateDescriptor(textureViewDesc, textureView);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVal& device = GetDeviceVal(*textureViewDesc.texture);

    return device.CreateDescriptor(textureViewDesc, textureView);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    DeviceVal& device = GetDeviceVal(*textureViewDesc.texture);

    return device.CreateDescriptor(textureViewDesc, textureView);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return ((DeviceVal&)device).CreateDescriptor(samplerDesc, sampler);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return ((DeviceVal&)device).CreatePipelineLayout(pipelineLayoutDesc, pipelineLayout);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreatePipeline(graphicsPipelineDesc, pipeline);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreatePipeline(computePipelineDesc, pipeline);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return ((DeviceVal&)device).CreateQueryPool(queryPoolDesc, queryPool);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    return ((DeviceVal&)device).CreateFence(initialValue, fence);
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    if (!(&commandBuffer))
        return;

    GetDeviceVal(commandBuffer).DestroyCommandBuffer(commandBuffer);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    if (!(&commandAllocator))
        return;

    GetDeviceVal(commandAllocator).DestroyCommandAllocator(commandAllocator);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    if (!(&descriptorPool))
        return;

    GetDeviceVal(descriptorPool).DestroyDescriptorPool(descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    if (!(&buffer))
        return;

    GetDeviceVal(buffer).DestroyBuffer(buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    if (!(&texture))
        return;

    GetDeviceVal(texture).DestroyTexture(texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    if (!(&descriptor))
        return;

    GetDeviceVal(descriptor).DestroyDescriptor(descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    if (!(&pipelineLayout))
        return;

    GetDeviceVal(pipelineLayout).DestroyPipelineLayout(pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    if (!(&pipeline))
        return;

    GetDeviceVal(pipeline).DestroyPipeline(pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    if (!(&queryPool))
        return;

    GetDeviceVal(queryPool).DestroyQueryPool(queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    if (!(&fence))
        return;

    GetDeviceVal(fence).DestroyFence(fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceVal&)device).AllocateMemory(allocateMemoryDesc, memory);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVal&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVal&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    if (!(&memory))
        return;

    GetDeviceVal(memory).FreeMemory(memory);
}

static void NRI_CALL SetDeviceDebugName(Device& device, const char* name) {
    ((DeviceVal&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name) {
    ((PipelineVal&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name) {
    ((PipelineLayoutVal&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name) {
    ((MemoryVal&)memory).SetDebugName(name);
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;

    return ((DeviceVal&)device).GetNativeObject();
}

Result DeviceVal::FillFunctionTable(CoreInterface& table) const {
    table = {};
    Core_Device_PartiallyFillFunctionTableVal(table);
    Core_Buffer_PartiallyFillFunctionTableVal(table);
    Core_CommandAllocator_PartiallyFillFunctionTableVal(table);
    Core_CommandBuffer_PartiallyFillFunctionTableVal(table);
    Core_CommandQueue_PartiallyFillFunctionTableVal(table);
    Core_Descriptor_PartiallyFillFunctionTableVal(table);
    Core_DescriptorPool_PartiallyFillFunctionTableVal(table);
    Core_DescriptorSet_PartiallyFillFunctionTableVal(table);
    Core_Fence_PartiallyFillFunctionTableVal(table);
    Core_QueryPool_PartiallyFillFunctionTableVal(table);
    Core_Texture_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Helper  ]

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    return ((DeviceVal&)device).CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    return ((DeviceVal&)device).AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL QueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    return ((DeviceVal&)device).QueryVideoMemoryInfo(memoryLocation, videoMemoryInfo);
}

Result DeviceVal::FillFunctionTable(HelperInterface& helperInterface) const {
    helperInterface = {};
    Helper_CommandQueue_PartiallyFillFunctionTableVal(helperInterface);
    Helper_Device_PartiallyFillFunctionTableVal(helperInterface);

    return ValidateFunctionTable(helperInterface);
}

#pragma endregion

#pragma region[  LowLatency  ]

Result DeviceVal::FillFunctionTable(LowLatencyInterface& table) const {
    table = {};
    if (!m_IsLowLatencySupported)
        return Result::UNSUPPORTED;

    LowLatency_CommandQueue_PartiallyFillFunctionTableVal(table);
    LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  MeshShader  ]

Result DeviceVal::FillFunctionTable(MeshShaderInterface& table) const {
    table = {};
    if (!m_IsMeshShaderSupported)
        return Result::UNSUPPORTED;

    MeshShader_CommandBuffer_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  RayTracing  ]

static void NRI_CALL GetAccelerationStructureMemoryDesc(
    const Device& device, const AccelerationStructureDesc& accelerationStructureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    DeviceVal& deviceVal = (DeviceVal&)device;

    AccelerationStructureDesc accelerationStructureDescImpl = accelerationStructureDesc;

    uint32_t geometryObjectNum = accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL ? accelerationStructureDesc.instanceOrGeometryObjectNum : 0;
    Scratch<GeometryObject> objectImplArray = AllocateScratch(deviceVal, GeometryObject, geometryObjectNum);

    if (accelerationStructureDesc.type == AccelerationStructureType::BOTTOM_LEVEL) {
        ConvertGeometryObjectsVal(objectImplArray, accelerationStructureDesc.geometryObjects, geometryObjectNum);
        accelerationStructureDescImpl.geometryObjects = objectImplArray;
    }

    deviceVal.GetRayTracingInterface().GetAccelerationStructureMemoryDesc(deviceVal.GetImpl(), accelerationStructureDescImpl, memoryLocation, memoryDesc);
    deviceVal.RegisterMemoryType(memoryDesc.type, memoryLocation);
}

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreatePipeline(pipelineDesc, pipeline);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).CreateAccelerationStructure(accelerationStructureDesc, accelerationStructure);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceVal&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    if (!(&accelerationStructure))
        return;

    GetDeviceVal(accelerationStructure).DestroyAccelerationStructure(accelerationStructure);
}

void FillFunctionTablePipelineVal(RayTracingInterface& table);

Result DeviceVal::FillFunctionTable(RayTracingInterface& table) const {
    table = {};
    if (!m_IsRayTracingSupported)
        return Result::UNSUPPORTED;

    FillFunctionTablePipelineVal(table);
    RayTracing_CommandBuffer_PartiallyFillFunctionTableVal(table);
    RayTracing_AccelerationStructure_PartiallyFillFunctionTableVal(table);
    RayTracing_Device_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  ResourceAllocator  ]

static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceVal&)device).AllocateBuffer(bufferDesc, buffer);
}

static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceVal&)device).AllocateTexture(textureDesc, texture);
}

static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& acelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).AllocateAccelerationStructure(acelerationStructureDesc, accelerationStructure);
}

Result DeviceVal::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table = {};
    ResourceAllocator_Device_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  Streamer  ]

struct StreamerVal : DeviceObjectVal<Streamer> {
    inline StreamerVal(DeviceVal& device, Streamer* impl)
        : DeviceObjectVal(device, impl) {
    }

    BufferVal* constantBuffer = nullptr;
    BufferVal* dynamicBuffer = nullptr;
    bool isDynamicBufferValid = false;
};

static Result CreateStreamer(Device& device, const StreamerDesc& streamerDesc, Streamer*& streamer) {
    DeviceVal& deviceVal = (DeviceVal&)device;
    bool isUpload = (streamerDesc.constantBufferMemoryLocation == MemoryLocation::HOST_UPLOAD || streamerDesc.constantBufferMemoryLocation == MemoryLocation::DEVICE_UPLOAD)
        && (streamerDesc.dynamicBufferMemoryLocation == MemoryLocation::HOST_UPLOAD || streamerDesc.dynamicBufferMemoryLocation == MemoryLocation::DEVICE_UPLOAD);
    RETURN_ON_FAILURE(&deviceVal, isUpload, Result::INVALID_ARGUMENT, "memory location must be an UPLOAD heap");

    Streamer* impl = nullptr;
    Result result = deviceVal.GetStreamerInterface().CreateStreamer(deviceVal.GetImpl(), streamerDesc, impl);

    if (result == Result::SUCCESS)
        streamer = (Streamer*)Allocate<StreamerVal>(deviceVal.GetStdAllocator(), deviceVal, impl);

    return result;
}

static void DestroyStreamer(Streamer& streamer) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;

    streamerVal.GetStreamerInterface().DestroyStreamer(*NRI_GET_IMPL(Streamer, &streamer));

    Destroy(deviceVal.GetStdAllocator(), streamerVal.constantBuffer);
    Destroy(deviceVal.GetStdAllocator(), streamerVal.dynamicBuffer);
    Destroy(deviceVal.GetStdAllocator(), &streamerVal);
}

static Buffer* GetStreamerConstantBuffer(Streamer& streamer) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    Buffer* buffer = streamerVal.GetStreamerInterface().GetStreamerConstantBuffer(*NRI_GET_IMPL(Streamer, &streamer));

    if (!streamerVal.constantBuffer)
        streamerVal.constantBuffer = Allocate<BufferVal>(deviceVal.GetStdAllocator(), deviceVal, buffer, false);

    return (Buffer*)streamerVal.constantBuffer;
}

static uint32_t UpdateStreamerConstantBuffer(Streamer& streamer, const void* data, uint32_t dataSize) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;

    if (!dataSize)
        REPORT_WARNING(&deviceVal, "'dataSize = 0'");

    return streamerVal.GetStreamerInterface().UpdateStreamerConstantBuffer(*NRI_GET_IMPL(Streamer, &streamer), data, dataSize);
}

static uint64_t AddStreamerBufferUpdateRequest(Streamer& streamer, const BufferUpdateRequestDesc& bufferUpdateRequestDesc) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    streamerVal.isDynamicBufferValid = false;

    if (!bufferUpdateRequestDesc.dataSize)
        REPORT_WARNING(&deviceVal, "'bufferUpdateRequestDesc.dataSize = 0'");

    BufferUpdateRequestDesc bufferUpdateRequestDescImpl = bufferUpdateRequestDesc;
    bufferUpdateRequestDescImpl.dstBuffer = NRI_GET_IMPL(Buffer, bufferUpdateRequestDesc.dstBuffer);

    return streamerVal.GetStreamerInterface().AddStreamerBufferUpdateRequest(*NRI_GET_IMPL(Streamer, &streamer), bufferUpdateRequestDescImpl);
}

static uint64_t AddStreamerTextureUpdateRequest(Streamer& streamer, const TextureUpdateRequestDesc& textureUpdateRequestDesc) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    streamerVal.isDynamicBufferValid = false;

    if (!textureUpdateRequestDesc.dstTexture)
        REPORT_ERROR(&deviceVal, "'textureUpdateRequestDesc.dstTexture' is NULL");
    if (!textureUpdateRequestDesc.dataRowPitch)
        REPORT_WARNING(&deviceVal, "'textureUpdateRequestDesc.dataRowPitch = 0'");
    if (!textureUpdateRequestDesc.dataSlicePitch)
        REPORT_WARNING(&deviceVal, "'textureUpdateRequestDesc.dataSlicePitch = 0'");

    TextureUpdateRequestDesc textureUpdateRequestDescImpl = textureUpdateRequestDesc;
    textureUpdateRequestDescImpl.dstTexture = NRI_GET_IMPL(Texture, textureUpdateRequestDesc.dstTexture);

    return streamerVal.GetStreamerInterface().AddStreamerTextureUpdateRequest(*NRI_GET_IMPL(Streamer, &streamer), textureUpdateRequestDescImpl);
}

static Result CopyStreamerUpdateRequests(Streamer& streamer) {
    StreamerVal& streamerVal = (StreamerVal&)streamer;
    streamerVal.isDynamicBufferValid = true;

    return streamerVal.GetStreamerInterface().CopyStreamerUpdateRequests(*NRI_GET_IMPL(Streamer, &streamer));
}

static Buffer* GetStreamerDynamicBuffer(Streamer& streamer) {
    DeviceVal& deviceVal = GetDeviceVal(streamer);
    StreamerVal& streamerVal = (StreamerVal&)streamer;

    if (!streamerVal.isDynamicBufferValid)
        REPORT_ERROR(&deviceVal, "'GetStreamerDynamicBuffer' must be called after 'CopyStreamerUpdateRequests'");

    Buffer* buffer = streamerVal.GetStreamerInterface().GetStreamerDynamicBuffer(*NRI_GET_IMPL(Streamer, &streamer));

    if (NRI_GET_IMPL(Buffer, streamerVal.dynamicBuffer) != buffer) {
        Destroy(deviceVal.GetStdAllocator(), streamerVal.dynamicBuffer);
        streamerVal.dynamicBuffer = nullptr;
    }

    if (!streamerVal.dynamicBuffer)
        streamerVal.dynamicBuffer = Allocate<BufferVal>(deviceVal.GetStdAllocator(), deviceVal, buffer, false);

    return (Buffer*)streamerVal.dynamicBuffer;
}

static void CmdUploadStreamerUpdateRequests(CommandBuffer& commandBuffer, Streamer& streamer) {
    StreamerVal& streamerVal = (StreamerVal&)streamer;

    streamerVal.GetStreamerInterface().CmdUploadStreamerUpdateRequests(*NRI_GET_IMPL(CommandBuffer, &commandBuffer), *NRI_GET_IMPL(Streamer, &streamer));
}

Result DeviceVal::FillFunctionTable(StreamerInterface& table) const {
    table = {};
    Streamer_Device_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceVal&)device).CreateSwapChain(swapChainDesc, swapChain);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    if (!(&swapChain))
        return;

    GetDeviceVal(swapChain).DestroySwapChain(swapChain);
}

Result DeviceVal::FillFunctionTable(SwapChainInterface& table) const {
    table = {};
    if (!m_IsSwapChainSupported)
        return Result::UNSUPPORTED;

    SwapChain_Device_PartiallyFillFunctionTableVal(table);
    SwapChain_SwapChain_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
}

#pragma endregion

#pragma region[  WrapperD3D11  ]

#if NRI_USE_D3D11

static Result NRI_CALL CreateCommandBufferD3D11(Device& device, const CommandBufferD3D11Desc& commandBufferD3D11Desc, CommandBuffer*& commandBuffer) {
    return ((DeviceVal&)device).CreateCommandBuffer(commandBufferD3D11Desc, commandBuffer);
}

static Result NRI_CALL CreateBufferD3D11(Device& device, const BufferD3D11Desc& bufferD3D11Desc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferD3D11Desc, buffer);
}

static Result NRI_CALL CreateTextureD3D11(Device& device, const TextureD3D11Desc& textureD3D11Desc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureD3D11Desc, texture);
}

Define_WrapperD3D11_Device_PartiallyFillFunctionTable(Val);

#endif

Result DeviceVal::FillFunctionTable(WrapperD3D11Interface& table) const {
    table = {};
#if NRI_USE_D3D11
    if (!m_IsWrapperD3D11Supported)
        return Result::UNSUPPORTED;

    WrapperD3D11_Device_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
#else
    return Result::UNSUPPORTED;
#endif
}

#pragma endregion

#pragma region[  WrapperD3D12  ]

#if NRI_USE_D3D12

static Result NRI_CALL CreateCommandBufferD3D12(Device& device, const CommandBufferD3D12Desc& commandBufferD3D12Desc, CommandBuffer*& commandBuffer) {
    return ((DeviceVal&)device).CreateCommandBuffer(commandBufferD3D12Desc, commandBuffer);
}

static Result NRI_CALL CreateDescriptorPoolD3D12(Device& device, const DescriptorPoolD3D12Desc& descriptorPoolD3D12Desc, DescriptorPool*& descriptorPool) {
    return ((DeviceVal&)device).CreateDescriptorPool(descriptorPoolD3D12Desc, descriptorPool);
}

static Result NRI_CALL CreateBufferD3D12(Device& device, const BufferD3D12Desc& bufferD3D12Desc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferD3D12Desc, buffer);
}

static Result NRI_CALL CreateTextureD3D12(Device& device, const TextureD3D12Desc& textureD3D12Desc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureD3D12Desc, texture);
}

static Result NRI_CALL CreateMemoryD3D12(Device& device, const MemoryD3D12Desc& memoryD3D12Desc, Memory*& memory) {
    return ((DeviceVal&)device).CreateMemory(memoryD3D12Desc, memory);
}

static Result NRI_CALL CreateAccelerationStructureD3D12(
    Device& device, const AccelerationStructureD3D12Desc& accelerationStructureD3D12Desc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).CreateAccelerationStructure(accelerationStructureD3D12Desc, accelerationStructure);
}

Define_WrapperD3D12_Device_PartiallyFillFunctionTable(Val);

#endif

Result DeviceVal::FillFunctionTable(WrapperD3D12Interface& table) const {
    table = {};
#if NRI_USE_D3D12
    if (!m_IsWrapperD3D12Supported)
        return Result::UNSUPPORTED;

    WrapperD3D12_Device_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);

#else
    return Result::UNSUPPORTED;
#endif
}

#pragma endregion

#pragma region[  WrapperVK  ]

#if NRI_USE_VULKAN

static Result NRI_CALL CreateCommandQueueVK(Device& device, const CommandQueueVKDesc& commandQueueVKDesc, CommandQueue*& commandQueue) {
    return ((DeviceVal&)device).CreateCommandQueue(commandQueueVKDesc, commandQueue);
}

static Result NRI_CALL CreateCommandAllocatorVK(Device& device, const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    return ((DeviceVal&)device).CreateCommandAllocator(commandAllocatorVKDesc, commandAllocator);
}

static Result NRI_CALL CreateCommandBufferVK(Device& device, const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    return ((DeviceVal&)device).CreateCommandBuffer(commandBufferVKDesc, commandBuffer);
}

static Result NRI_CALL CreateDescriptorPoolVK(Device& device, const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    return ((DeviceVal&)device).CreateDescriptorPool(descriptorPoolVKDesc, descriptorPool);
}

static Result NRI_CALL CreateBufferVK(Device& device, const BufferVKDesc& bufferVKDesc, Buffer*& buffer) {
    return ((DeviceVal&)device).CreateBuffer(bufferVKDesc, buffer);
}

static Result NRI_CALL CreateTextureVK(Device& device, const TextureVKDesc& textureVKDesc, Texture*& texture) {
    return ((DeviceVal&)device).CreateTexture(textureVKDesc, texture);
}

static Result NRI_CALL CreateMemoryVK(Device& device, const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    return ((DeviceVal&)device).CreateMemory(memoryVKDesc, memory);
}

static Result NRI_CALL CreateGraphicsPipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreateGraphicsPipeline(vkPipeline, pipeline);
}

static Result NRI_CALL CreateComputePipelineVK(Device& device, VKNonDispatchableHandle vkPipeline, Pipeline*& pipeline) {
    return ((DeviceVal&)device).CreateComputePipeline(vkPipeline, pipeline);
}

static Result NRI_CALL CreateQueryPoolVK(Device& device, const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    return ((DeviceVal&)device).CreateQueryPool(queryPoolVKDesc, queryPool);
}

static Result NRI_CALL CreateAccelerationStructureVK(
    Device& device, const AccelerationStructureVKDesc& accelerationStructureVKDesc, AccelerationStructure*& accelerationStructure) {
    return ((DeviceVal&)device).CreateAccelerationStructure(accelerationStructureVKDesc, accelerationStructure);
}

static VKHandle NRI_CALL GetPhysicalDeviceVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetPhysicalDeviceVK(((DeviceVal&)device).GetImpl());
}

static VKHandle NRI_CALL GetInstanceVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetInstanceVK(((DeviceVal&)device).GetImpl());
}

static void* NRI_CALL GetInstanceProcAddrVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetInstanceProcAddrVK(((DeviceVal&)device).GetImpl());
}

static void* NRI_CALL GetDeviceProcAddrVK(const Device& device) {
    return ((DeviceVal&)device).GetWrapperVKInterface().GetDeviceProcAddrVK(((DeviceVal&)device).GetImpl());
}

Define_WrapperVK_Device_PartiallyFillFunctionTable(Val);

#endif

Result DeviceVal::FillFunctionTable(WrapperVKInterface& table) const {
    table = {};
#if NRI_USE_VULKAN
    if (!m_IsWrapperVKSupported)
        return Result::UNSUPPORTED;

    WrapperVK_Device_PartiallyFillFunctionTableVal(table);

    return ValidateFunctionTable(table);
#else
    return Result::UNSUPPORTED;
#endif
}

#pragma endregion

Define_Core_Device_PartiallyFillFunctionTable(Val);
Define_Helper_Device_PartiallyFillFunctionTable(Val);
Define_RayTracing_Device_PartiallyFillFunctionTable(Val);
Define_Streamer_Device_PartiallyFillFunctionTable(Val);
Define_SwapChain_Device_PartiallyFillFunctionTable(Val);
Define_ResourceAllocator_Device_PartiallyFillFunctionTable(Val);
