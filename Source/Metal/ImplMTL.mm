// © 2021 NVIDIA Corporation

#include "SharedMTL.h"

using namespace nri;

#include "BufferMTL.h"
#include "CommandAllocatorMTL.h"
#include "CommandBufferMTL.h"
#include "DeviceMTL.h"
#include "DescriptorMTL.h"
#include "TextureMTL.h"
#include "PipelineLayoutMTL.h"
#include "PipelineMTL.h"
#include "DescriptorMTL.h"
#include "MemoryMTL.h"
#include "HelperDeviceMemoryAllocator.h"
#include "CommandQueueMTL.h"
#include "SwapChainMTL.h"


Result CreateDeviceMTL(const DeviceCreationDesc& desc, DeviceBase*& device) {
    StdAllocator<uint8_t> allocator(desc.allocationCallbacks);
    DeviceMTL* impl = Allocate<DeviceMTL>(allocator, desc.callbackInterface, allocator);
    Result result = impl->Create(desc, {}, false);

    
    MTLPurgeableState a;
    MTLStorageMode mode;
    if (result != Result::SUCCESS) {
        Destroy(allocator, impl);
        device = nullptr;
    } else
        device = (DeviceBase*)impl;
    return result;
}


Result CreateDeviceMTL(const DeviceCreationMTLDesc& desc, DeviceBase*& device) {
    return Result::SUCCESS;
}
//============================================================================================================================================================================================
#pragma region[  Core  ]


static void NRI_CALL SetBufferDebugName(Buffer& buffer, const char* name) {
    ((BufferMTL&)buffer).SetDebugName(name);
}

static uint64_t NRI_CALL GetBufferNativeObject(const Buffer& buffer) {
    if (!(&buffer))
        return 0;
    return uint64_t(((BufferMTL&)buffer).GetHandle());
}


static void* NRI_CALL MapBuffer(Buffer& buffer, uint64_t offset, uint64_t size) {
    return ((BufferMTL&)buffer).Map(offset, size);
}

static void NRI_CALL UnmapBuffer(Buffer& buffer) {
    ((BufferMTL&)buffer).Unmap();
}

static void NRI_CALL SetCommandAllocatorDebugName(CommandAllocator& commandAllocator, const char* name) {
  //  ((CommandAllocatorVK&)commandAllocator).SetDebugName(name);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorMTL&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
   // ((CommandAllocatorVK&)commandAllocator).Reset();
    
}

static void NRI_CALL SetCommandBufferDebugName(CommandBuffer& commandBuffer, const char* name) {
    ((CommandBufferMTL&)commandBuffer).SetDebugName(name);
}

static Result NRI_CALL BeginCommandBuffer(CommandBuffer& commandBuffer, const DescriptorPool* descriptorPool) {
    return ((CommandBufferMTL&)commandBuffer).Begin(descriptorPool);
}

static Result NRI_CALL EndCommandBuffer(CommandBuffer& commandBuffer) {
    return ((CommandBufferMTL&)commandBuffer).End();
}

static void NRI_CALL CmdSetPipelineLayout(CommandBuffer& commandBuffer, const PipelineLayout& pipelineLayout) {
    ((CommandBufferMTL&)commandBuffer).SetPipelineLayout(pipelineLayout);
}

static void NRI_CALL CmdSetPipeline(CommandBuffer& commandBuffer, const Pipeline& pipeline) {
    ((CommandBufferMTL&)commandBuffer).SetPipeline(pipeline);
}

static void NRI_CALL CmdBarrier(CommandBuffer& commandBuffer, const BarrierGroupDesc& barrierGroupDesc) {
    ((CommandBufferMTL&)commandBuffer).Barrier(barrierGroupDesc);
}

static void NRI_CALL CmdSetDescriptorPool(CommandBuffer& commandBuffer, const DescriptorPool& descriptorPool) {
    ((CommandBufferMTL&)commandBuffer).SetDescriptorPool(descriptorPool);
}

static void NRI_CALL CmdSetDescriptorSet(CommandBuffer& commandBuffer, uint32_t setIndex, const DescriptorSet& descriptorSet, const uint32_t* dynamicConstantBufferOffsets) {
    ((CommandBufferMTL&)commandBuffer).SetDescriptorSet(setIndex, descriptorSet, dynamicConstantBufferOffsets);
}

static void NRI_CALL CmdSetRootConstants(CommandBuffer& commandBuffer, uint32_t rootConstantIndex, const void* data, uint32_t size) {
    //((CommandBufferMTL&)commandBuffer).SetRootConstants(rootConstantIndex, data, size);
}

static void NRI_CALL CmdSetRootDescriptor(CommandBuffer& commandBuffer, uint32_t rootDescriptorIndex, Descriptor& descriptor) {
    //((CommandBufferMTL&)commandBuffer).SetRootDescriptor(rootDescriptorIndex, descriptor);
}

static void NRI_CALL CmdBeginRendering(CommandBuffer& commandBuffer, const AttachmentsDesc& attachmentsDesc) {
    ((CommandBufferMTL&)commandBuffer).BeginRendering(attachmentsDesc);
}

static void NRI_CALL CmdEndRendering(CommandBuffer& commandBuffer) {
    ((CommandBufferMTL&)commandBuffer).EndRendering();
}

static void NRI_CALL CmdSetViewports(CommandBuffer& commandBuffer, const Viewport* viewports, uint32_t viewportNum) {
    ((CommandBufferMTL&)commandBuffer).SetViewports(viewports, viewportNum);
}

static void NRI_CALL CmdSetScissors(CommandBuffer& commandBuffer, const nri::Rect* rects, uint32_t rectNum) {
    ((CommandBufferMTL&)commandBuffer).SetScissors(rects, rectNum);
}

static void NRI_CALL CmdSetDepthBounds(CommandBuffer& commandBuffer, float boundsMin, float boundsMax) {
    ((CommandBufferMTL&)commandBuffer).SetDepthBounds(boundsMin, boundsMax);
}

static void NRI_CALL CmdSetStencilReference(CommandBuffer& commandBuffer, uint8_t frontRef, uint8_t backRef) {
    ((CommandBufferMTL&)commandBuffer).SetStencilReference(frontRef, backRef);
}

static void NRI_CALL CmdSetSampleLocations(CommandBuffer& commandBuffer, const SampleLocation* locations, Sample_t locationNum, Sample_t sampleNum) {
    //((CommandBufferMTL&)commandBuffer).SetSampleLocations(locations, locationNum, sampleNum);
}

static void NRI_CALL CmdSetBlendConstants(CommandBuffer& commandBuffer, const Color32f& color) {
    ((CommandBufferMTL&)commandBuffer).SetBlendConstants(color);
}

static void NRI_CALL CmdSetShadingRate(CommandBuffer& commandBuffer, const ShadingRateDesc& shadingRateDesc) {
    ((CommandBufferMTL&)commandBuffer).SetShadingRate(shadingRateDesc);
}

static void NRI_CALL CmdSetDepthBias(CommandBuffer& commandBuffer, const DepthBiasDesc& depthBiasDesc) {
   // ((CommandBufferMTL&)commandBuffer).SetDepthBias(depthBiasDesc);
}

static void NRI_CALL CmdClearAttachments(CommandBuffer& commandBuffer, const ClearDesc* clearDescs, uint32_t clearDescNum, const nri::Rect* rects, uint32_t rectNum) {
    //((CommandBufferMTL&)commandBuffer).ClearAttachments(clearDescs, clearDescNum, rects, rectNum);
}

static void NRI_CALL CmdSetIndexBuffer(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, IndexType indexType) {
    ((CommandBufferMTL&)commandBuffer).SetIndexBuffer(buffer, offset, indexType);
}

static void NRI_CALL CmdSetVertexBuffers(CommandBuffer& commandBuffer, uint32_t baseSlot, uint32_t bufferNum, const Buffer* const* buffers, const uint64_t* offsets) {
    ((CommandBufferMTL&)commandBuffer).SetVertexBuffers(baseSlot, bufferNum, buffers, offsets);
}

static void NRI_CALL CmdDraw(CommandBuffer& commandBuffer, const DrawDesc& drawDesc) {
    ((CommandBufferMTL&)commandBuffer).Draw(drawDesc);
}

static void NRI_CALL CmdDrawIndexed(CommandBuffer& commandBuffer, const DrawIndexedDesc& drawIndexedDesc) {
    ((CommandBufferMTL&)commandBuffer).DrawIndexed(drawIndexedDesc);
}

static void NRI_CALL CmdDrawIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferMTL&)commandBuffer).DrawIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDrawIndexedIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset, uint32_t drawNum, uint32_t stride, const Buffer* countBuffer, uint64_t countBufferOffset) {
    ((CommandBufferMTL&)commandBuffer).DrawIndexedIndirect(buffer, offset, drawNum, stride, countBuffer, countBufferOffset);
}

static void NRI_CALL CmdDispatch(CommandBuffer& commandBuffer, const DispatchDesc& dispatchDesc) {
    ((CommandBufferMTL&)commandBuffer).Dispatch(dispatchDesc);
}

static void NRI_CALL CmdDispatchIndirect(CommandBuffer& commandBuffer, const Buffer& buffer, uint64_t offset) {
    ((CommandBufferMTL&)commandBuffer).DispatchIndirect(buffer, offset);
}

static void NRI_CALL CmdBeginQuery(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferMTL&)commandBuffer).BeginQuery(queryPool, offset);
}

static void NRI_CALL CmdEndQuery(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset) {
    ((CommandBufferMTL&)commandBuffer).EndQuery(queryPool, offset);
}

static void NRI_CALL CmdBeginAnnotation(CommandBuffer& commandBuffer, const char* name, uint32_t bgra) {
    ((CommandBufferMTL&)commandBuffer).BeginAnnotation(name);
}

static void NRI_CALL CmdEndAnnotation(CommandBuffer& commandBuffer) {
    ((CommandBufferMTL&)commandBuffer).EndAnnotation();
}

static void NRI_CALL CmdClearStorageBuffer(CommandBuffer& commandBuffer, const ClearStorageBufferDesc& clearDesc) {
    ((CommandBufferMTL&)commandBuffer).ClearStorageBuffer(clearDesc);
}

static void NRI_CALL CmdClearStorageTexture(CommandBuffer& commandBuffer, const ClearStorageTextureDesc& clearDesc) {
    ((CommandBufferMTL&)commandBuffer).ClearStorageTexture(clearDesc);
}

static void NRI_CALL CmdResolveTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    //((CommandBufferMTL&)commandBuffer).ResolveTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdCopyBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, uint64_t dstOffset, const Buffer& srcBuffer, uint64_t srcOffset, uint64_t size) {
    ((CommandBufferMTL&)commandBuffer).CopyBuffer(dstBuffer, dstOffset, srcBuffer, srcOffset, size);
}

static void NRI_CALL CmdCopyTexture(
    CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc* dstRegionDesc, const Texture& srcTexture, const TextureRegionDesc* srcRegionDesc) {
    ((CommandBufferMTL&)commandBuffer).CopyTexture(dstTexture, dstRegionDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdUploadBufferToTexture(CommandBuffer& commandBuffer, Texture& dstTexture, const TextureRegionDesc& dstRegionDesc, const Buffer& srcBuffer, const TextureDataLayoutDesc& srcDataLayoutDesc) {
    ((CommandBufferMTL&)commandBuffer).UploadBufferToTexture(dstTexture, dstRegionDesc, srcBuffer, srcDataLayoutDesc);
}

static void NRI_CALL CmdReadbackTextureToBuffer(CommandBuffer& commandBuffer, Buffer& dstBuffer, const TextureDataLayoutDesc& dstDataLayoutDesc, const Texture& srcTexture, const TextureRegionDesc& srcRegionDesc) {
    //((CommandBufferMTL&)commandBuffer).ReadbackTextureToBuffer(dstBuffer, dstDataLayoutDesc, srcTexture, srcRegionDesc);
}

static void NRI_CALL CmdCopyQueries(CommandBuffer& commandBuffer, const QueryPool& queryPool, uint32_t offset, uint32_t num, Buffer& dstBuffer, uint64_t dstOffset) {
    ((CommandBufferMTL&)commandBuffer).CopyQueries(queryPool, offset, num, dstBuffer, dstOffset);
}

static Result NRI_CALL AllocateMemory(Device& device, const AllocateMemoryDesc& allocateMemoryDesc, Memory*& memory) {
    return ((DeviceMTL&)device).CreateImplementation<MemoryMTL>(memory, allocateMemoryDesc);
}

static Result NRI_CALL BindBufferMemory(Device& device, const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceMTL&)device).BindBufferMemory(memoryBindingDescs, memoryBindingDescNum);
}

static Result NRI_CALL BindTextureMemory(Device& device, const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    return ((DeviceMTL&)device).BindTextureMemory(memoryBindingDescs, memoryBindingDescNum);
}


static void NRI_CALL CmdResetQueries(CommandBuffer& commandBuffer, QueryPool& queryPool, uint32_t offset, uint32_t num) {
    ((CommandBufferMTL&)commandBuffer).ResetQueries(queryPool, offset, num);
}

static const DeviceDesc& NRI_CALL GetDeviceDesc(const Device& device) {
    return ((DeviceMTL&)device).GetDesc();
}

static Result NRI_CALL CreateTexture(Device& device, const TextureDesc& textureDesc, Texture*& texture) {
    return ((DeviceMTL&)device).CreateImplementation<TextureMTL>(texture, textureDesc);
}


//static Result NRI_CALL CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
//    DeviceMTL& device = ((const BufferMTL*)bufferViewDesc.buffer)->GetDevice();
//    return device.CreateImplementation<DescriptorMTL>(bufferView, bufferViewDesc);
//}
//
//static Result NRI_CALL CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
//    DeviceMTL& device = ((const TextureMTL*)textureViewDesc.texture)->GetDevice();
//    return device.CreateImplementation<DescriptorMTL>(textureView, textureViewDesc);
//}
//
//static Result NRI_CALL CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
//    DeviceMTL& device = ((const TextureMTL*)textureViewDesc.texture)->GetDevice();
//    return device.CreateImplementation<DescriptorMTL>(textureView, textureViewDesc);
//}
//
//static Result NRI_CALL CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
//    DeviceMTL& device = ((const TextureMTL*)textureViewDesc.texture)->GetDevice();
//    return device.CreateImplementation<DescriptorMTL>(textureView, textureViewDesc);
//}


//static void NRI_CALL QueueSubmit(CommandQueue& commandQueue, const QueueSubmitDesc& workSubmissionDesc) {
//    ((CommandQueueMTL&)commandQueue).Submit(workSubmissionDesc, nullptr);
//}


static void NRI_CALL DestroyCommandBuffer(CommandBuffer& commandBuffer) {
    Destroy((CommandBufferMTL*)&commandBuffer);
}

static void NRI_CALL DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Destroy((CommandAllocatorMTL*)&commandAllocator);
}

static void NRI_CALL DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    //Destroy((DescriptorPoolM*)&descriptorPool);
}

static void NRI_CALL DestroyBuffer(Buffer& buffer) {
    Destroy((BufferMTL*)&buffer);
}

static void NRI_CALL DestroyTexture(Texture& texture) {
    Destroy((TextureMTL*)&texture);
}

static void NRI_CALL DestroyDescriptor(Descriptor& descriptor) {
   // Destroy((DescriptorMTL*)&descriptor);
}

static void NRI_CALL DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
 //   Destroy((PipelineLayoutMTL*)&pipelineLayout);
}

static void NRI_CALL DestroyPipeline(Pipeline& pipeline) {
    //Destroy((PipelineMTL*)&pipeline);
}

static void NRI_CALL DestroyQueryPool(QueryPool& queryPool) {
    //Destroy((QueryPoolVK*)&queryPool);
}

static void NRI_CALL DestroyFence(Fence& fence) {
   // Destroy((Fenc*)&fence);
}

static void NRI_CALL FreeMemory(Memory& memory) {
    Destroy((MemoryMTL*)&memory);
}

static void NRI_CALL SetFenceDebugName(Fence& fence, const char* name) {
//    ((FenceMTL&)fence).SetDebugName(name);
}


static void NRI_CALL SetDeviceDebugName(Device& device, const char* name) {
//    ((DeviceMTL&)device).SetDebugName(name);
}

static void NRI_CALL SetPipelineDebugName(Pipeline& pipeline, const char* name) {
//    ((PipelineMTL&)pipeline).SetDebugName(name);
}

static void NRI_CALL SetPipelineLayoutDebugName(PipelineLayout& pipelineLayout, const char* name) {
//    ((PipelineLayoutMTL&)pipelineLayout).SetDebugName(name);
}

static void NRI_CALL SetMemoryDebugName(Memory& memory, const char* name) {
    ((MemoryMTL&)memory).SetDebugName(name);
}


static const BufferDesc& NRI_CALL GetBufferDesc(const Buffer& buffer) {
    return ((const BufferMTL&)buffer).GetDesc();
}

static const TextureDesc& NRI_CALL GetTextureDesc(const Texture& texture) {
    return ((const TextureMTL&)texture).GetDesc();
}

static void NRI_CALL GetBufferMemoryDesc(const Device& device, const BufferDesc& bufferDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceMTL&)device).GetMemoryDesc(bufferDesc, memoryLocation, memoryDesc);
}

static void NRI_CALL GetTextureMemoryDesc(const Device& device, const TextureDesc& textureDesc, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((const DeviceMTL&)device).GetMemoryDesc(textureDesc, memoryLocation, memoryDesc);
}


static Result NRI_CALL GetCommandQueue(Device& device, CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    return ((DeviceMTL&)device).GetCommandQueue(commandQueueType, commandQueue);
}


static Result NRI_CALL CreateGraphicsPipeline(Device& device, const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return ((DeviceMTL&)device).CreateImplementation<PipelineMTL>(pipeline, graphicsPipelineDesc);
}

static Result NRI_CALL CreateComputePipeline(Device& device, const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return ((DeviceMTL&)device).CreateImplementation<PipelineMTL>(pipeline, computePipelineDesc);
}


static Result NRI_CALL AllocateDescriptorSets(DescriptorPool& descriptorPool, const PipelineLayout& pipelineLayout, uint32_t setIndex, DescriptorSet** descriptorSets, uint32_t instanceNum, uint32_t variableDescriptorNum) {
    return Result::SUCCESS;
    //return ((DescriptorPoolMTL&)descriptorPool).AllocateDescriptorSets(pipelineLayout, setIndex, descriptorSets, instanceNum, variableDescriptorNum);
}


static void NRI_CALL SetDescriptorDebugName(Descriptor& descriptor, const char* name) {
//    ((DescriptorMTL&)descriptor).SetDebugName(name);
}


Result DeviceMTL::FillFunctionTable(CoreInterface& table) const {
    table.GetDeviceDesc = ::GetDeviceDesc;
    table.GetBufferDesc = ::GetBufferDesc;
    table.GetTextureDesc = ::GetTextureDesc;
    //table.GetFormatSupport = ::GetFormatSupport;
    //table.GetQuerySize = ::GetQuerySize;
    table.GetBufferMemoryDesc = ::GetBufferMemoryDesc;
    table.GetTextureMemoryDesc = ::GetTextureMemoryDesc;
    table.GetCommandQueue = ::GetCommandQueue;
    //table.CreateCommandAllocator = ::CreateCommandAllocator;
    table.CreateCommandBuffer = ::CreateCommandBuffer;
    //table.CreateDescriptorPool = ::CreateDescriptorPool;
    //table.CreateBuffer = ::CreateBuffer;
    table.CreateTexture = ::CreateTexture;
    //table.CreateBufferView = ::CreateBufferView;
    //table.CreateTexture1DView = ::CreateTexture1DView;
    //table.CreateTexture2DView = ::CreateTexture2DView;
    //table.CreateTexture3DView = ::CreateTexture3DView;
    //table.CreateSampler = ::CreateSampler;
    //table.CreatePipelineLayout = ::CreatePipelineLayout;
    table.CreateGraphicsPipeline = ::CreateGraphicsPipeline;
    table.CreateComputePipeline = ::CreateComputePipeline;
    
//    table.CreateQueryPool = ::CreateQueryPool;
//    table.CreateFence = ::CreateFence;
    table.DestroyCommandAllocator = ::DestroyCommandAllocator;
    table.DestroyCommandBuffer = ::DestroyCommandBuffer;
    table.DestroyDescriptorPool = ::DestroyDescriptorPool;
    table.DestroyBuffer = ::DestroyBuffer;
    table.DestroyTexture = ::DestroyTexture;
    table.DestroyDescriptor = ::DestroyDescriptor;
    table.DestroyPipelineLayout = ::DestroyPipelineLayout;
    table.DestroyPipeline = ::DestroyPipeline;
    table.DestroyQueryPool = ::DestroyQueryPool;
    table.DestroyFence = ::DestroyFence;
    table.AllocateMemory = ::AllocateMemory;
    table.BindBufferMemory = ::BindBufferMemory;
    table.BindTextureMemory = ::BindTextureMemory;
    table.FreeMemory = ::FreeMemory;
    table.BeginCommandBuffer = ::BeginCommandBuffer;
    table.CmdSetDescriptorPool = ::CmdSetDescriptorPool;
    table.CmdSetDescriptorSet = ::CmdSetDescriptorSet;
    table.CmdSetPipelineLayout = ::CmdSetPipelineLayout;
    table.CmdSetPipeline = ::CmdSetPipeline;
    table.CmdSetRootConstants = ::CmdSetRootConstants;
    table.CmdSetRootDescriptor = ::CmdSetRootDescriptor;
    table.CmdBarrier = ::CmdBarrier;
    table.CmdSetIndexBuffer = ::CmdSetIndexBuffer;
    table.CmdSetVertexBuffers = ::CmdSetVertexBuffers;
    table.CmdSetViewports = ::CmdSetViewports;
    table.CmdSetScissors = ::CmdSetScissors;
    table.CmdSetStencilReference = ::CmdSetStencilReference;
    table.CmdSetDepthBounds = ::CmdSetDepthBounds;
    table.CmdSetBlendConstants = ::CmdSetBlendConstants;
    table.CmdSetSampleLocations = ::CmdSetSampleLocations;
    table.CmdSetShadingRate = ::CmdSetShadingRate;
    table.CmdSetDepthBias = ::CmdSetDepthBias;
    table.CmdBeginRendering = ::CmdBeginRendering;
    table.CmdClearAttachments = ::CmdClearAttachments;
    table.CmdDraw = ::CmdDraw;
    table.CmdDrawIndexed = ::CmdDrawIndexed;
    table.CmdDrawIndirect = ::CmdDrawIndirect;
    table.CmdDrawIndexedIndirect = ::CmdDrawIndexedIndirect;
    table.CmdEndRendering = ::CmdEndRendering;
    table.CmdDispatch = ::CmdDispatch;
    table.CmdDispatchIndirect = ::CmdDispatchIndirect;
    table.CmdCopyBuffer = ::CmdCopyBuffer;
    table.CmdCopyTexture = ::CmdCopyTexture;
    table.CmdUploadBufferToTexture = ::CmdUploadBufferToTexture;
    table.CmdReadbackTextureToBuffer = ::CmdReadbackTextureToBuffer;
    table.CmdClearStorageBuffer = ::CmdClearStorageBuffer;
    table.CmdClearStorageTexture = ::CmdClearStorageTexture;
    table.CmdResolveTexture = ::CmdResolveTexture;
    table.CmdResetQueries = ::CmdResetQueries;
//    table.CmdBeginQuery = ::CmdBeginQuery;
//    table.CmdEndQuery = ::CmdEndQuery;
    table.CmdCopyQueries = ::CmdCopyQueries;
    table.CmdBeginAnnotation = ::CmdBeginAnnotation;
    table.CmdEndAnnotation = ::CmdEndAnnotation;
    table.EndCommandBuffer = ::EndCommandBuffer;
//    table.QueueSubmit = ::QueueSubmit;
//    table.Wait = ::Wait;
//    table.GetFenceValue = ::GetFenceValue;
//    table.UpdateDescriptorRanges = ::UpdateDescriptorRanges;
//    table.UpdateDynamicConstantBuffers = ::UpdateDynamicConstantBuffers;
//    table.CopyDescriptorSet = ::CopyDescriptorSet;
    table.AllocateDescriptorSets = ::AllocateDescriptorSets;
//    table.ResetDescriptorPool = ::ResetDescriptorPool;
    table.ResetCommandAllocator = ::ResetCommandAllocator;
    table.MapBuffer = ::MapBuffer;
    table.UnmapBuffer = ::UnmapBuffer;
    table.SetDeviceDebugName = ::SetDeviceDebugName;
    table.SetFenceDebugName = ::SetFenceDebugName;
    table.SetDescriptorDebugName = ::SetDescriptorDebugName;
    table.SetPipelineDebugName = ::SetPipelineDebugName;
    table.SetCommandBufferDebugName = ::SetCommandBufferDebugName;
    table.SetBufferDebugName = ::SetBufferDebugName;
//    table.SetTextureDebugName = ::SetTextureDebugName;
//    table.SetCommandQueueDebugName = ::SetCommandQueueDebugName;
    table.SetCommandAllocatorDebugName = ::SetCommandAllocatorDebugName;
//    table.SetDescriptorPoolDebugName = ::SetDescriptorPoolDebugName;
//    table.SetPipelineLayoutDebugName = ::SetPipelineLayoutDebugName;
//    table.SetQueryPoolDebugName = ::SetQueryPoolDebugName;
//    table.SetDescriptorSetDebugName = ::SetDescriptorSetDebugName;
//    table.SetMemoryDebugName = ::SetMemoryDebugName;
//    table.GetDeviceNativeObject = ::GetDeviceNativeObject;
//    table.GetCommandBufferNativeObject = ::GetCommandBufferNativeObject;
    table.GetBufferNativeObject = ::GetBufferNativeObject;
//    table.GetTextureNativeObject = ::GetTextureNativeObject;
//    table.GetDescriptorNativeObject = ::GetDescriptorNativeObject;

    return Result::SUCCESS;
}

static uint32_t NRI_CALL CalculateAllocationNumber(const Device& device, const ResourceGroupDesc& resourceGroupDesc) {
    DeviceMTL& deviceMTL = (DeviceMTL&)device;
    HelperDeviceMemoryAllocator allocator(deviceMTL.GetCoreInterface(), (Device&)device);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

static Result NRI_CALL UploadData(CommandQueue& commandQueue, const TextureUploadDesc* textureUploadDescs, uint32_t textureUploadDescNum, const BufferUploadDesc* bufferUploadDescs, uint32_t bufferUploadDescNum) {
    return ((CommandQueueMTL&)commandQueue).UploadData(textureUploadDescs, textureUploadDescNum, bufferUploadDescs, bufferUploadDescNum);
}

static Result NRI_CALL AllocateAndBindMemory(Device& device, const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    DeviceMTL& deviceMTL = (DeviceMTL&)device;
    HelperDeviceMemoryAllocator allocator(deviceMTL.GetCoreInterface(), device);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

static Result NRI_CALL WaitForIdle(CommandQueue& commandQueue) {
    if (!(&commandQueue))
        return Result::SUCCESS;

    return ((CommandQueueMTL&)commandQueue).WaitForIdle();
}

#pragma endregion

Result DeviceMTL::FillFunctionTable(HelperInterface& table) const {
    table.CalculateAllocationNumber = ::CalculateAllocationNumber;
    table.AllocateAndBindMemory = ::AllocateAndBindMemory;
    table.UploadData = ::UploadData;
    table.WaitForIdle = ::WaitForIdle;
//    table.QueryVideoMemoryInfo = ::QueryVideoMemoryInfo;
    return Result::SUCCESS;
}
Result DeviceMTL::FillFunctionTable(LowLatencyInterface& table) const {
    
    return Result::SUCCESS;
}
Result DeviceMTL::FillFunctionTable(MeshShaderInterface& table) const {
    
    return Result::SUCCESS;
}
Result DeviceMTL::FillFunctionTable(RayTracingInterface& table) const {
    return Result::SUCCESS;
}
Result DeviceMTL::FillFunctionTable(StreamerInterface& table) const {
    return Result::SUCCESS;
}

static void NRI_CALL SetSwapChainDebugName(SwapChain& swapChain, const char* name) {
    ((SwapChainMTL&)swapChain).SetDebugName(name);
}

//static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
//    return ((SwapChainMTL&)swapChain).GetDisplayDesc(displayDesc);
//}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainMTL&)swapChain).Present();
}

static Result NRI_CALL CreateSwapChain(Device& device, const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return ((DeviceMTL&)device).CreateImplementation<SwapChainMTL>(swapChain, swapChainDesc);
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainMTL&)swapChain).WaitForPresent();
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainMTL&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainMTL&)swapChain).AcquireNextTexture();
}


static void NRI_CALL DestroySwapChain(SwapChain& swapChain) {
    Destroy((SwapChainMTL*)&swapChain);
}

Result DeviceMTL::FillFunctionTable(SwapChainInterface& table) const {
    
    if (!m_Desc.isSwapChainSupported)
        return Result::UNSUPPORTED;

    table.CreateSwapChain = ::CreateSwapChain;
    table.DestroySwapChain = ::DestroySwapChain;
    table.SetSwapChainDebugName = ::SetSwapChainDebugName;
    table.GetSwapChainTextures = ::GetSwapChainTextures;
    table.AcquireNextSwapChainTexture = ::AcquireNextSwapChainTexture;
    table.WaitForPresent = ::WaitForPresent;
    table.QueuePresent = ::QueuePresent;
//    table.GetDisplayDesc = ::GetDisplayDesc;
    return Result::SUCCESS;
}


//static Result AllocateBuffer(Device& device, const AllocateBufferDesc& bufferDesc, Buffer*& buffer) {
//    return ((DeviceMTL&)device).CreateImplementation<BufferMTL>(buffer, bufferDesc);
//}
//
//static Result AllocateTexture(Device& device, const AllocateTextureDesc& textureDesc, Texture*& texture) {
//    return ((DeviceMTL&)device).CreateImplementation<TextureMTL>(texture, textureDesc);
//}

//static Result AllocateAccelerationStructure(Device& device, const AllocateAccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
//    return ((DeviceMTL&)device).CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
//}

Result DeviceMTL::FillFunctionTable(ResourceAllocatorInterface& table) const {
//    table.AllocateBuffer = ::AllocateBuffer;
//    table.AllocateTexture = ::AllocateTexture;
//    table.AllocateAccelerationStructure = ::AllocateAccelerationStructure;

    return Result::SUCCESS;
}
