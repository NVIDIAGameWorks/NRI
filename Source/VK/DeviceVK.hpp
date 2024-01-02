/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

Declare_PartiallyFillFunctionTable_Functions(VK)

#pragma region [  Core  ]

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device)
{
    return ((DeviceVK&)device).GetDesc();
}

static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer)
{
    return ((const BufferVK&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture)
{
    return ((const TextureVK&)texture).GetDesc();
}

static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue)
{
    return ((DeviceVK&)device).GetCommandQueue(commandQueueType, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator)
{
    DeviceVK& device = ((CommandQueueVK&)commandQueue).GetDevice();
    return device.CreateCommandAllocator(commandQueue, commandAllocator);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool)
{
    return ((DeviceVK&)device).CreateDescriptorPool(descriptorPoolDesc, descriptorPool);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferDesc& bufferDesc, Buffer*& buffer)
{
    return ((DeviceVK&)device).CreateBuffer(bufferDesc, buffer);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture)
{
    return ((DeviceVK&)device).CreateTexture(textureDesc, texture);
}

static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView)
{
    DeviceVK& device = ((const BufferVK*)bufferViewDesc.buffer)->GetDevice();
    return device.CreateBufferView(bufferViewDesc, bufferView);
}

static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView)
{
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateTexture1DView(textureViewDesc, textureView);
}

static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView)
{
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateTexture2DView(textureViewDesc, textureView);
}

static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView)
{
    DeviceVK& device = ((const TextureVK*)textureViewDesc.texture)->GetDevice();
    return device.CreateTexture3DView(textureViewDesc, textureView);
}

static Result NRI_CALL CreateSampler(Device& device, const SamplerDesc& samplerDesc, Descriptor*& sampler)
{
    return ((DeviceVK&)device).CreateSampler(samplerDesc, sampler);
}

static Result NRI_CALL CreatePipelineLayout(Device& device, const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout)
{
    return ((DeviceVK&)device).CreatePipelineLayout(pipelineLayoutDesc, pipelineLayout);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline)
{
    return ((DeviceVK&)device).CreatePipeline(graphicsPipelineDesc, pipeline);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline)
{
    return ((DeviceVK&)device).CreatePipeline(computePipelineDesc, pipeline);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool)
{
    return ((DeviceVK&)device).CreateQueryPool(queryPoolDesc, queryPool);
}

static Result NRI_CALL CreateFence(Device& device, uint64_t initialValue, Fence*& fence)
{
    return ((DeviceVK&)device).CreateFence(initialValue, fence);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator)
{
    if(!(&commandAllocator))
        return;

    ((CommandAllocatorVK&)commandAllocator).GetDevice().DestroyCommandAllocator(commandAllocator);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool)
{
    if(!(&descriptorPool))
        return;

    ((DescriptorPoolVK&)descriptorPool).GetDevice().DestroyDescriptorPool(descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer)
{
    if(!(&buffer))
        return;

    ((BufferVK&)buffer).GetDevice().DestroyBuffer(buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture)
{
    if(!(&texture))
        return;

    ((TextureVK&)texture).GetDevice().DestroyTexture(texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor)
{
    if(!(&descriptor))
        return;

    ((DescriptorVK&)descriptor).GetDevice().DestroyDescriptor(descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout)
{
    if(!(&pipelineLayout))
        return;

    ((PipelineLayoutVK&)pipelineLayout).GetDevice().DestroyPipelineLayout(pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline)
{
    if(!(&pipeline))
        return;

    ((PipelineVK&)pipeline).GetDevice().DestroyPipeline(pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool)
{
    if(!(&queryPool))
        return;

    ((QueryPoolVK&)queryPool).GetDevice().DestroyQueryPool(queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence)
{
    if(!(&fence))
        return;

    ((FenceVK&)fence).GetDevice().DestroyFence(fence);
}

static Result NRI_CALL AllocateMemory(Device& device, uint32_t nodeMask, MemoryType memoryType, uint64_t size, Memory*& memory)
{
    return ((DeviceVK&)device).AllocateMemory(nodeMask, memoryType, size, memory);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum)
{
    return ((DeviceVK&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum)
{
    return ((DeviceVK&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL FreeMemory(Memory& memory)
{
    if(!(&memory))
        return;

    ((MemoryVK&)memory).GetDevice().FreeMemory(memory);
}

static FormatSupportBits NRI_CALL GetFormatSupport(const Device& device, Format format)
{
    return ((const DeviceVK&)device).GetFormatSupport(format);
}

static void NRI_CALL SetDeviceDebugName(Device& device, const char* name)
{
    ((DeviceVK&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name)
{
    ((PipelineVK&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name)
{
    ((PipelineLayoutVK&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name)
{
    ((MemoryVK&)memory).SetDebugName(name);
}

static void* NRI_CALL GetDeviceNativeObject(const Device& device)
{
    if (!(&device))
        return nullptr;

    return (VkDevice)((DeviceVK&)device);
}

Result DeviceVK::FillFunctionTable(CoreInterface& coreInterface) const
{
    coreInterface = {};
    coreInterface.GetDeviceDesc = ::GetDeviceDesc;
    coreInterface.GetBufferDesc = ::GetBufferDesc;
    coreInterface.GetTextureDesc = ::GetTextureDesc;
    coreInterface.GetFormatSupport = ::GetFormatSupport;
    coreInterface.GetCommandQueue = ::GetCommandQueue;
    coreInterface.CreateCommandAllocator = ::CreateCommandAllocator;
    coreInterface.CreateDescriptorPool = ::CreateDescriptorPool;
    coreInterface.CreateBuffer = ::CreateBuffer;
    coreInterface.CreateTexture = ::CreateTexture;
    coreInterface.CreateBufferView = ::CreateBufferView;
    coreInterface.CreateTexture1DView = ::CreateTexture1DView;
    coreInterface.CreateTexture2DView = ::CreateTexture2DView;
    coreInterface.CreateTexture3DView = ::CreateTexture3DView;
    coreInterface.CreateSampler = ::CreateSampler;
    coreInterface.CreatePipelineLayout = ::CreatePipelineLayout;
    coreInterface.CreateGraphicsPipeline = ::CreateGraphicsPipeline;
    coreInterface.CreateComputePipeline = ::CreateComputePipeline;
    coreInterface.CreateQueryPool = ::CreateQueryPool;
    coreInterface.CreateFence = ::CreateFence;
    coreInterface.DestroyCommandAllocator = ::DestroyCommandAllocator;
    coreInterface.DestroyDescriptorPool = ::DestroyDescriptorPool;
    coreInterface.DestroyBuffer = ::DestroyBuffer;
    coreInterface.DestroyTexture = ::DestroyTexture;
    coreInterface.DestroyDescriptor = ::DestroyDescriptor;
    coreInterface.DestroyPipelineLayout = ::DestroyPipelineLayout;
    coreInterface.DestroyPipeline = ::DestroyPipeline;
    coreInterface.DestroyQueryPool = ::DestroyQueryPool;
    coreInterface.DestroyFence = ::DestroyFence;
    coreInterface.AllocateMemory = ::AllocateMemory;
    coreInterface.BindBufferMemory = ::BindBufferMemory;
    coreInterface.BindTextureMemory = ::BindTextureMemory;
    coreInterface.FreeMemory = ::FreeMemory;
    coreInterface.SetDeviceDebugName = ::SetDeviceDebugName;
    coreInterface.SetPipelineDebugName = ::SetPipelineDebugName;
    coreInterface.SetPipelineLayoutDebugName = ::SetPipelineLayoutDebugName;
    coreInterface.SetMemoryDebugName = ::SetMemoryDebugName;
    coreInterface.GetDeviceNativeObject = ::GetDeviceNativeObject;

    Core_Buffer_PartiallyFillFunctionTableVK(coreInterface);
    Core_CommandAllocator_PartiallyFillFunctionTableVK(coreInterface);
    Core_CommandBuffer_PartiallyFillFunctionTableVK(coreInterface);
    Core_CommandQueue_PartiallyFillFunctionTableVK(coreInterface);
    Core_Descriptor_PartiallyFillFunctionTableVK(coreInterface);
    Core_DescriptorPool_PartiallyFillFunctionTableVK(coreInterface);
    Core_DescriptorSet_PartiallyFillFunctionTableVK(coreInterface);
    Core_Fence_PartiallyFillFunctionTableVK(coreInterface);
    Core_QueryPool_PartiallyFillFunctionTableVK(coreInterface);
    Core_Texture_PartiallyFillFunctionTableVK(coreInterface);

    return ValidateFunctionTable(coreInterface);
}

#pragma endregion

#pragma region [  SwapChain  ]

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain)
{
    return ((DeviceVK&)device).CreateSwapChain(swapChainDesc, swapChain);
}

static void NRI_CALL DestroySwapChain(SwapChain& swapChain)
{
    if(!(&swapChain))
        return;

    return ((SwapChainVK&)swapChain).GetDevice().DestroySwapChain(swapChain);
}

Result DeviceVK::FillFunctionTable(SwapChainInterface& swapChainInterface) const
{
    swapChainInterface = {};
    swapChainInterface.CreateSwapChain = ::CreateSwapChain;
    swapChainInterface.DestroySwapChain = ::DestroySwapChain;

    SwapChain_PartiallyFillFunctionTableVK(swapChainInterface);

    return ValidateFunctionTable(swapChainInterface);
}

#pragma endregion

#pragma region [  WrapperVK  ]

static Result NRI_CALL CreateCommandQueue(Device& device, const CommandQueueVKDesc& commandQueueVKDesc, CommandQueue*& commandQueue)
{
    return ((DeviceVK&)device).CreateCommandQueue(commandQueueVKDesc, commandQueue);
}

static Result NRI_CALL CreateCommandAllocator(Device& device, const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator)
{
    return ((DeviceVK&)device).CreateCommandAllocator(commandAllocatorVKDesc, commandAllocator);
}

static Result NRI_CALL CreateCommandBuffer(Device& device, const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer)
{
    return ((DeviceVK&)device).CreateCommandBuffer(commandBufferVKDesc, commandBuffer);
}

static Result NRI_CALL CreateDescriptorPool(Device& device, const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool)
{
    return ((DeviceVK&)device).CreateDescriptorPool(descriptorPoolVKDesc, descriptorPool);
}

static Result NRI_CALL CreateBuffer(Device& device, const BufferVKDesc& bufferVKDesc, Buffer*& buffer)
{
    return ((DeviceVK&)device).CreateBuffer(bufferVKDesc, buffer);
}

static Result NRI_CALL CreateTexture(Device& device, const TextureVKDesc& textureVKDesc, Texture*& texture)
{
    return ((DeviceVK&)device).CreateTexture(textureVKDesc, texture);
}

static Result NRI_CALL CreateMemory(Device& device, const MemoryVKDesc& memoryVKDesc, Memory*& memory)
{
    return ((DeviceVK&)device).CreateMemory(memoryVKDesc, memory);
}

static Result NRI_CALL CreateGraphicsPipeline(Device& device, NRIVkPipeline vkPipeline, Pipeline*& pipeline)
{
    return ((DeviceVK&)device).CreateGraphicsPipeline(vkPipeline, pipeline);
}

static Result NRI_CALL CreateComputePipeline(Device& device, NRIVkPipeline vkPipeline, Pipeline*& pipeline)
{
    return ((DeviceVK&)device).CreateComputePipeline(vkPipeline, pipeline);
}

static Result NRI_CALL CreateQueryPool(Device& device, const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool)
{
    return ((DeviceVK&)device).CreateQueryPool(queryPoolVKDesc, queryPool);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureVKDesc& accelerationStructureVKDesc, AccelerationStructure*& accelerationStructure)
{
    return ((DeviceVK&)device).CreateAccelerationStructure(accelerationStructureVKDesc, accelerationStructure);
}

static NRIVkPhysicalDevice NRI_CALL GetVkPhysicalDevice(const Device& device)
{
    return (VkPhysicalDevice)((DeviceVK&)device);
}

static NRIVkInstance NRI_CALL GetVkInstance(const Device& device)
{
    return (VkInstance)((DeviceVK&)device);
}

static void* NRI_CALL GetVkGetInstanceProcAddr(const Device& device)
{
    return ((DeviceVK&)device).GetDispatchTable().GetInstanceProcAddr;
}

static void* NRI_CALL GetVkGetDeviceProcAddr(const Device& device)
{
    return ((DeviceVK&)device).GetDispatchTable().GetDeviceProcAddr;
}

Result DeviceVK::FillFunctionTable(WrapperVKInterface& wrapperVKInterface) const
{
    wrapperVKInterface = {};
    wrapperVKInterface.CreateCommandQueueVK = ::CreateCommandQueue;
    wrapperVKInterface.CreateCommandAllocatorVK = ::CreateCommandAllocator;
    wrapperVKInterface.CreateCommandBufferVK = ::CreateCommandBuffer;
    wrapperVKInterface.CreateDescriptorPoolVK = ::CreateDescriptorPool;
    wrapperVKInterface.CreateBufferVK = ::CreateBuffer;
    wrapperVKInterface.CreateTextureVK = ::CreateTexture;
    wrapperVKInterface.CreateMemoryVK = ::CreateMemory;
    wrapperVKInterface.CreateGraphicsPipelineVK = ::CreateGraphicsPipeline;
    wrapperVKInterface.CreateComputePipelineVK = ::CreateComputePipeline;
    wrapperVKInterface.CreateQueryPoolVK = ::CreateQueryPool;
    wrapperVKInterface.CreateAccelerationStructureVK = ::CreateAccelerationStructure;
    wrapperVKInterface.GetVkPhysicalDevice = ::GetVkPhysicalDevice;
    wrapperVKInterface.GetVkInstance = ::GetVkInstance;
    wrapperVKInterface.GetVkGetDeviceProcAddr = ::GetVkGetDeviceProcAddr;
    wrapperVKInterface.GetVkGetInstanceProcAddr = ::GetVkGetInstanceProcAddr;

    return ValidateFunctionTable(wrapperVKInterface);
}

#pragma endregion

#pragma region [  RayTracing  ]

static Result NRI_CALL CreateRayTracingPipeline(Device& device, const RayTracingPipelineDesc& pipelineDesc, Pipeline*& pipeline)
{
    return ((DeviceVK&)device).CreatePipeline(pipelineDesc, pipeline);
}

static Result NRI_CALL CreateAccelerationStructure(Device& device, const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure)
{
    return ((DeviceVK&)device).CreateAccelerationStructure(accelerationStructureDesc, accelerationStructure);
}

static Result NRI_CALL BindAccelerationStructureMemory(Device& device, const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum)
{
    return ((DeviceVK&)device).BindAccelerationStructureMemory(memoryBindingDescs, memoryBindingDescNum);
}

static void NRI_CALL DestroyAccelerationStructure(AccelerationStructure& accelerationStructure)
{
    if(!(&accelerationStructure))
        return;

    return ((AccelerationStructureVK&)accelerationStructure).GetDevice().DestroyAccelerationStructure(accelerationStructure);
}

void FillFunctionTablePipelineVK(RayTracingInterface& rayTracingInterface);

Result DeviceVK::FillFunctionTable(RayTracingInterface& rayTracingInterface) const
{
    if (!supportedFeatures.rayTracing)
        return Result::UNSUPPORTED;

    rayTracingInterface = {};
    rayTracingInterface.CreateRayTracingPipeline = ::CreateRayTracingPipeline;
    rayTracingInterface.CreateAccelerationStructure = ::CreateAccelerationStructure;
    rayTracingInterface.BindAccelerationStructureMemory = ::BindAccelerationStructureMemory;
    rayTracingInterface.DestroyAccelerationStructure = ::DestroyAccelerationStructure;

    RayTracing_CommandBuffer_PartiallyFillFunctionTableVK(rayTracingInterface);
    RayTracing_AccelerationStructure_PartiallyFillFunctionTableVK(rayTracingInterface);
    FillFunctionTablePipelineVK(rayTracingInterface);

    return ValidateFunctionTable(rayTracingInterface);
}

#pragma endregion

#pragma region [  MeshShader  ]

Result DeviceVK::FillFunctionTable(MeshShaderInterface& meshShaderInterface) const
{
    if (!supportedFeatures.meshShader)
        return Result::UNSUPPORTED;

    meshShaderInterface = {};

    MeshShader_CommandBuffer_PartiallyFillFunctionTableVK(meshShaderInterface);

    return ValidateFunctionTable(meshShaderInterface);
}

#pragma endregion

#pragma region [  Helper  ]

static uint32_t NRI_CALL CountAllocationNum(Device& device, const ResourceGroupDesc& resourceGroupDesc)
{
    return ((DeviceVK&)device).CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations)
{
    return ((DeviceVK&)device).AllocateAndBindMemory(resourceGroupDesc, allocations);
}

Result DeviceVK::FillFunctionTable(HelperInterface& helperInterface) const
{
    helperInterface = {};
    helperInterface.CalculateAllocationNumber = ::CountAllocationNum;
    helperInterface.AllocateAndBindMemory = ::AllocateAndBindMemory;

    Helper_CommandQueue_PartiallyFillFunctionTableVK(helperInterface);

    return ValidateFunctionTable(helperInterface);
}

#pragma endregion
