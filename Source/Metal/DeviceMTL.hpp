// © 2021 NVIDIA Corporation

Declare_PartiallyFillFunctionTable_Functions(MTL);

#pragma region[  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((const DeviceMTL&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferMTL&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureMTL&)texture).GetDesc();
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format) {
    return (FormatSupportBits)0;
    //return ((const DeviceDesc&)device).GetFormatSupport(format);
}

static void NRI_CALL GetBufferMemoryDesc(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    //D3D12_RESOURCE_DESC desc = {};
    //GetResourceDesc(&desc, bufferDesc);

    //((const DeviceD3D12&)device).GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    //D3D12_RESOURCE_DESC desc = {};
    //GetResourceDesc(&desc, textureDesc);

    //((const DeviceD3D12&)device).GetMemoryDesc(memoryLocation, desc, memoryDesc);
}

static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return ((DeviceMTL&)device).GetCommandQueue(commandQueueType, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    //DeviceMTL& device = ((CommandQueueD3D12&)commandQueue).GetDevice();
    //return device.CreateImplementation<CommandAllocatorD3D12>(commandAllocator, commandQueue);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    //return ((DeviceMTL&)device).CreateImplementation<DescriptorPoolMTL>(descriptorPool, descriptorPoolDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer) {
    return ((DeviceMTL&)device).CreateImplementation<BufferMTL>(buffer, bufferDesc);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceMTL&)device).CreateImplementation<TextureMTL>(texture, textureDesc);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    //DeviceMTL& device = ((const BufferD3D12*)bufferViewDesc.buffer)->GetDevice();
    //return device.CreateImplementation<DescriptorD3D12>(bufferView, bufferViewDesc);
    return Result::SUCCESS;
}


static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    //DeviceMTL& device = ((const TextureMTL*)textureViewDesc.texture)->GetDevice();
    //return device.CreateImplementation<DescriptorMTL>(textureView, textureViewDesc);

    
    //DeviceMTL& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    //return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    //DeviceMTL& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    //return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    //DeviceMTL& device = ((const TextureD3D12*)textureViewDesc.texture)->GetDevice();
    //return device.CreateImplementation<DescriptorD3D12>(textureView, textureViewDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    //return ((DeviceMTL&)device).CreateImplementation<DescriptorD3D12>(sampler, samplerDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    //return ((DeviceMTL&)device).CreateImplementation<PipelineLayoutD3D12>(pipelineLayout, pipelineLayoutDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    //return ((DeviceMTL&)device).CreateImplementation<PipelineD3D12>(pipeline, graphicsPipelineDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    //return ((DeviceMTL&)device).CreateImplementation<PipelineD3D12>(pipeline, computePipelineDesc);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence) {
    //return ((DeviceMTL&)device).CreateImplementation<FenceD3D12>(fence, initialValue);
    return Result::SUCCESS;
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    //return ((DeviceMTL&)device).CreateImplementation<QueryPoolD3D12>(queryPool, queryPoolDesc);
    return Result::SUCCESS;
}

static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    //Destroy((CommandBufferD3D12*)&commandBuffer);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    //Destroy((CommandAllocatorD3D12*)&commandAllocator);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    //Destroy((DescriptorPoolD3D12*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    //Destroy((BufferD3D12*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    //Destroy((TextureD3D12*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
    //Destroy((DescriptorD3D12*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    //Destroy((PipelineLayoutD3D12*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    //Destroy((PipelineD3D12*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    //Destroy((QueryPoolD3D12*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
    Destroy((FenceMTL*)&fence);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return Result::SUCCESS;
    //return ((DeviceMTL&)device).CreateImplementation<MemoryD3D12>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    //return ((DeviceMTL&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
    return Result::SUCCESS;
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    //return ((DeviceMTL&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
    return Result::SUCCESS;
}

static void NRI_CALL FreeMemory(Memory& memory) {
//    Destroy((MemoryD3D12*)&memory);
}

static void NRI_CALL SetDeviceDebugName(Device& device, const char* name) {
//    ((DeviceMTL&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name) {
//    ((PipelineD3D12&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name) {
//    ((PipelineLayoutD3D12&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name) {
//    ((MemoryMetal&)memory).SetDebugName(name);
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device) {
    if (!(&device))
        return nullptr;
    return (DeviceMTL*)&((DeviceMTL&)device);

    //return ((DeviceMetal&)device).GetNativeObject();
}

Result DeviceMTL::FillFunctionTable(CoreInterface& table) const {
    table = {};
    Core_Device_PartiallyFillFunctionTableMTL(table);
    Core_Buffer_PartiallyFillFunctionTableMTL(table);
    Core_CommandAllocator_PartiallyFillFunctionTableMTL(table);
    Core_CommandBuffer_PartiallyFillFunctionTableMTL(table);
    Core_CommandQueue_PartiallyFillFunctionTableMTL(table);
    Core_Descriptor_PartiallyFillFunctionTableMTL(table);
    Core_DescriptorPool_PartiallyFillFunctionTableMTL(table);
    Core_DescriptorSet_PartiallyFillFunctionTableMTL(table);
    Core_Fence_PartiallyFillFunctionTableMTL(table);
    Core_QueryPool_PartiallyFillFunctionTableMTL(table);
    Core_Texture_PartiallyFillFunctionTableMTL(table);
    return ValidateFunctionTable(table);
}

#pragma endregion

Result DeviceMTL::FillFunctionTable(HelperInterface& table) const {
    table = {};
    return Result::UNSUPPORTED;
}

Result DeviceMTL::FillFunctionTable(LowLatencyInterface& table) const {
    table = {};
    return Result::UNSUPPORTED;
}

//Result DeviceMTL::FillFunctionTable(MeshShaderInterface& table) const {
//    table = {};
//    return Result::UNSUPPORTED;
//}

Result DeviceMTL::FillFunctionTable(RayTracingInterface& table) const {
    table = {};
    return Result::UNSUPPORTED;
}

Result DeviceMTL::FillFunctionTable(StreamerInterface& table) const {
    table = {};
    return Result::UNSUPPORTED;
}

Result DeviceMTL::FillFunctionTable(SwapChainInterface& table) const {
    table = {};
    return Result::UNSUPPORTED;
}

Result DeviceMTL::FillFunctionTable(ResourceAllocatorInterface& table) const {
    table = {};
    return Result::UNSUPPORTED;
}

Define_Core_Device_PartiallyFillFunctionTable(MTL);
//Define_Helper_Device_PartiallyFillFunctionTable(MTL);
//Define_RayTracing_Device_PartiallyFillFunctionTable(MTL);
//Define_Streamer_Device_PartiallyFillFunctionTable(MTL);
//Define_SwapChain_Device_PartiallyFillFunctionTable(MTL);
//Define_ResourceAllocator_Device_PartiallyFillFunctionTable(MTL);
