// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NRI_NAMESPACE_BEGIN

// Forward and "mimic" declarations
NRI_FORWARD_STRUCT(AccelerationStructure);
typedef void* VKHandle;
typedef uint64_t VKNonDispatchableHandle;
typedef int32_t VKEnum;
typedef uint32_t VKFlags;

NRI_STRUCT(DeviceCreationVKDesc)
{
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(AllocationCallbacks) allocationCallbacks;
    NRI_NAME(SPIRVBindingOffsets) spirvBindingOffsets;
    NRI_NAME(VKExtensions) enabledExtensions;
    VKHandle vkInstance;
    VKHandle vkDevice;
    VKHandle vkPhysicalDevice;
    const uint32_t* queueFamilyIndices;
    uint32_t queueFamilyIndexNum;
    const char* libraryPath;
    uint8_t minorVersion; // >= 2
};

NRI_STRUCT(CommandQueueVKDesc)
{
    VKHandle vkQueue;
    uint32_t queueFamilyIndex;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(CommandAllocatorVKDesc)
{
    VKNonDispatchableHandle vkCommandPool;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(CommandBufferVKDesc)
{
    VKHandle vkCommandBuffer;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(DescriptorPoolVKDesc)
{
    VKNonDispatchableHandle vkDescriptorPool;
    uint32_t descriptorSetMaxNum;
};

NRI_STRUCT(BufferVKDesc)
{
    VKNonDispatchableHandle vkBuffer;
    uint64_t size;
    NRI_OPTIONAL uint32_t structureStride; // must be provided if used as a structured or raw buffer
    NRI_OPTIONAL uint8_t* mappedMemory; // must be provided if the underlying memory is mapped
    NRI_OPTIONAL VKNonDispatchableHandle vkDeviceMemory; // must be provided *only* if the mapped memory exists and *not* HOST_COHERENT
    NRI_OPTIONAL uint64_t deviceAddress; // must be provided for ray tracing
};

NRI_STRUCT(TextureVKDesc)
{
    VKNonDispatchableHandle vkImage;
    VKEnum vkFormat;
    VKEnum vkImageType;
    NRI_NAME(Dim_t) width;
    NRI_NAME(Dim_t) height;
    NRI_NAME(Dim_t) depth;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) layerNum;
    NRI_NAME(Sample_t) sampleNum;
};

NRI_STRUCT(MemoryVKDesc)
{
    VKNonDispatchableHandle vkDeviceMemory;
    void* vkMappedMemory;
    uint64_t size;
    uint32_t memoryTypeIndex;
};

NRI_STRUCT(QueryPoolVKDesc)
{
    VKNonDispatchableHandle vkQueryPool;
    VKEnum vkQueryType;
};

NRI_STRUCT(AccelerationStructureVKDesc)
{
    VKNonDispatchableHandle vkAccelerationStructure;
    uint64_t buildScratchSize;
    uint64_t updateScratchSize;
};

NRI_STRUCT(WrapperVKInterface)
{
    NRI_NAME(Result) (NRI_CALL *CreateCommandQueueVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandQueueVKDesc) commandQueueVKDesc, NRI_NAME_REF(CommandQueue*) commandQueue);
    NRI_NAME(Result) (NRI_CALL *CreateCommandAllocatorVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandAllocatorVKDesc) commandAllocatorVKDesc, NRI_NAME_REF(CommandAllocator*) commandAllocator);
    NRI_NAME(Result) (NRI_CALL *CreateCommandBufferVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandBufferVKDesc) commandBufferVKDesc, NRI_NAME_REF(CommandBuffer*) commandBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateDescriptorPoolVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(DescriptorPoolVKDesc) descriptorPoolVKDesc, NRI_NAME_REF(DescriptorPool*) descriptorPool);
    NRI_NAME(Result) (NRI_CALL *CreateBufferVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(BufferVKDesc) bufferVKDesc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *CreateTextureVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(TextureVKDesc) textureVKDesc, NRI_NAME_REF(Texture*) texture);
    NRI_NAME(Result) (NRI_CALL *CreateMemoryVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(MemoryVKDesc) memoryVKDesc, NRI_NAME_REF(Memory*) memory);
    NRI_NAME(Result) (NRI_CALL *CreateGraphicsPipelineVK)(NRI_NAME_REF(Device) device, VKNonDispatchableHandle vkPipeline, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateComputePipelineVK)(NRI_NAME_REF(Device) device, VKNonDispatchableHandle vkPipeline, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateQueryPoolVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(QueryPoolVKDesc) queryPoolVKDesc, NRI_NAME_REF(QueryPool*) queryPool);
    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructureVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AccelerationStructureVKDesc) accelerationStructureVKDesc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);
    VKHandle (NRI_CALL *GetPhysicalDeviceVK)(const NRI_NAME_REF(Device) device);
    VKHandle (NRI_CALL *GetInstanceVK)(const NRI_NAME_REF(Device) device);
    void* (NRI_CALL *GetInstanceProcAddrVK)(const NRI_NAME_REF(Device) device);
    void* (NRI_CALL *GetDeviceProcAddrVK)(const NRI_NAME_REF(Device) device);
};

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromVkDevice(const NRI_NAME_REF(DeviceCreationVKDesc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END
