// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(AccelerationStructure);

typedef uint64_t NRIVkCommandPool;
typedef uint64_t NRIVkImage;
typedef uint64_t NRIVkBuffer;
typedef uint64_t NRIVkDeviceMemory;
typedef uint64_t NRIVkQueryPool;
typedef uint64_t NRIVkPipeline;
typedef uint64_t NRIVkDescriptorPool;
typedef uint64_t NRIVkImageView;
typedef uint64_t NRIVkBufferView;
typedef uint64_t NRIVkAccelerationStructureKHR;

typedef void* NRIVkInstance;
typedef void* NRIVkPhysicalDevice;
typedef void* NRIVkDevice;
typedef void* NRIVkQueue;
typedef void* NRIVkCommandBuffer;

NRI_STRUCT(DeviceCreationVKDesc)
{
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    NRI_NAME(SPIRVBindingOffsets) spirvBindingOffsets;
    NRI_NAME(VulkanExtensions) enabledExtensions;
    NRIVkInstance vkInstance;
    NRIVkDevice vkDevice;
    NRIVkPhysicalDevice vkPhysicalDevice;
    const uint32_t* queueFamilyIndices;
    uint32_t queueFamilyIndexNum;
    const char* vulkanLoaderPath;
    bool isVulkan12;
};

NRI_STRUCT(CommandQueueVKDesc)
{
    NRIVkQueue vkQueue;
    uint32_t familyIndex;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(CommandAllocatorVKDesc)
{
    NRIVkCommandPool vkCommandPool;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(CommandBufferVKDesc)
{
    NRIVkCommandBuffer vkCommandBuffer;
    NRI_NAME(CommandQueueType) commandQueueType;
};

NRI_STRUCT(DescriptorPoolVKDesc)
{
    NRIVkDescriptorPool vkDescriptorPool;
    uint32_t descriptorSetMaxNum;
};

NRI_STRUCT(BufferVKDesc)
{
    NRIVkBuffer vkBuffer;
    NRI_NAME(Memory)* memory;
    uint64_t size;
    uint64_t memoryOffset;
    uint64_t deviceAddress;
    uint32_t structureStride;
};

NRI_STRUCT(TextureVKDesc)
{
    NRIVkImage vkImage;
    uint32_t vkFormat;
    uint32_t vkImageAspectFlags;
    uint32_t vkImageType;
    NRI_NAME(Dim_t) width;
    NRI_NAME(Dim_t) height;
    NRI_NAME(Dim_t) depth;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) arraySize;
    NRI_NAME(Sample_t) sampleNum;
};

NRI_STRUCT(MemoryVKDesc)
{
    NRIVkDeviceMemory vkDeviceMemory;
    uint64_t size;
    uint32_t memoryTypeIndex;
};

NRI_STRUCT(QueryPoolVKDesc)
{
    NRIVkQueryPool vkQueryPool;
    uint32_t vkQueryType;
};

NRI_STRUCT(AccelerationStructureVKDesc)
{
    NRIVkAccelerationStructureKHR vkAccelerationStructure;
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
    NRI_NAME(Result) (NRI_CALL *CreateGraphicsPipelineVK)(NRI_NAME_REF(Device) device, NRIVkPipeline vkPipeline, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateComputePipelineVK)(NRI_NAME_REF(Device) device, NRIVkPipeline vkPipeline, NRI_NAME_REF(Pipeline*) pipeline);
    NRI_NAME(Result) (NRI_CALL *CreateQueryPoolVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(QueryPoolVKDesc) queryPoolVKDesc, NRI_NAME_REF(QueryPool*) queryPool);
    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructureVK)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AccelerationStructureVKDesc) accelerationStructureVKDesc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);

    NRIVkPhysicalDevice (NRI_CALL *GetVkPhysicalDevice)(const NRI_NAME_REF(Device) device);
    NRIVkInstance (NRI_CALL *GetVkInstance)(const NRI_NAME_REF(Device) device);
    void* (NRI_CALL *GetVkGetInstanceProcAddr)(const NRI_NAME_REF(Device) device);
    void* (NRI_CALL *GetVkGetDeviceProcAddr)(const NRI_NAME_REF(Device) device);
};

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromVkDevice(const NRI_NAME_REF(DeviceCreationVKDesc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END
