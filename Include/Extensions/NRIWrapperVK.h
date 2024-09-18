// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h" // CallbackInterface, AllocationCallbacks, SPIRVBindingOffsets, VKExtensions

typedef void* VKHandle;
typedef int32_t VKEnum;
typedef uint32_t VKFlags;
typedef uint64_t VKNonDispatchableHandle;

NriNamespaceBegin

NriForwardStruct(AccelerationStructure);

NriStruct(DeviceCreationVKDesc) {
    Nri(CallbackInterface) callbackInterface;
    Nri(AllocationCallbacks) allocationCallbacks;
    Nri(SPIRVBindingOffsets) spirvBindingOffsets;
    Nri(VKExtensions) enabledExtensions;
    VKHandle vkInstance;
    VKHandle vkDevice;
    VKHandle vkPhysicalDevice;
    const uint32_t* queueFamilyIndices;
    uint32_t queueFamilyIndexNum;
    const char* libraryPath;
    uint8_t minorVersion; // >= 2
};

NriStruct(CommandQueueVKDesc) {
    VKHandle vkQueue;
    uint32_t queueFamilyIndex;
    Nri(CommandQueueType) commandQueueType;
};

NriStruct(CommandAllocatorVKDesc) {
    VKNonDispatchableHandle vkCommandPool;
    Nri(CommandQueueType) commandQueueType;
};

NriStruct(CommandBufferVKDesc) {
    VKHandle vkCommandBuffer;
    Nri(CommandQueueType) commandQueueType;
};

NriStruct(DescriptorPoolVKDesc) {
    VKNonDispatchableHandle vkDescriptorPool;
    uint32_t descriptorSetMaxNum;
};

NriStruct(BufferVKDesc) {
    VKNonDispatchableHandle vkBuffer;
    uint64_t size;
    NriOptional uint32_t structureStride;               // must be provided if used as a structured or raw buffer
    NriOptional uint8_t* mappedMemory;                  // must be provided if the underlying memory is mapped
    NriOptional VKNonDispatchableHandle vkDeviceMemory; // must be provided *only* if the mapped memory exists and *not* HOST_COHERENT
    NriOptional uint64_t deviceAddress;                 // must be provided for ray tracing
};

NriStruct(TextureVKDesc) {
    VKNonDispatchableHandle vkImage;
    VKEnum vkFormat;
    VKEnum vkImageType;
    Nri(Dim_t) width;
    Nri(Dim_t) height;
    Nri(Dim_t) depth;
    Nri(Mip_t) mipNum;
    Nri(Dim_t) layerNum;
    Nri(Sample_t) sampleNum;
};

NriStruct(MemoryVKDesc) {
    VKNonDispatchableHandle vkDeviceMemory;
    void* vkMappedMemory;
    uint64_t size;
    uint32_t memoryTypeIndex;
};

NriStruct(QueryPoolVKDesc) {
    VKNonDispatchableHandle vkQueryPool;
    VKEnum vkQueryType;
};

NriStruct(AccelerationStructureVKDesc) {
    VKNonDispatchableHandle vkAccelerationStructure;
    uint64_t buildScratchSize;
    uint64_t updateScratchSize;
};

NriStruct(WrapperVKInterface) {
    Nri(Result) (NRI_CALL *CreateCommandQueueVK)            (NriRef(Device) device, const NriRef(CommandQueueVKDesc) commandQueueVKDesc, NriOut NriRef(CommandQueue*) commandQueue);
    Nri(Result) (NRI_CALL *CreateCommandAllocatorVK)        (NriRef(Device) device, const NriRef(CommandAllocatorVKDesc) commandAllocatorVKDesc, NriOut NriRef(CommandAllocator*) commandAllocator);
    Nri(Result) (NRI_CALL *CreateCommandBufferVK)           (NriRef(Device) device, const NriRef(CommandBufferVKDesc) commandBufferVKDesc, NriOut NriRef(CommandBuffer*) commandBuffer);
    Nri(Result) (NRI_CALL *CreateDescriptorPoolVK)          (NriRef(Device) device, const NriRef(DescriptorPoolVKDesc) descriptorPoolVKDesc, NriOut NriRef(DescriptorPool*) descriptorPool);
    Nri(Result) (NRI_CALL *CreateBufferVK)                  (NriRef(Device) device, const NriRef(BufferVKDesc) bufferVKDesc, NriOut NriRef(Buffer*) buffer);
    Nri(Result) (NRI_CALL *CreateTextureVK)                 (NriRef(Device) device, const NriRef(TextureVKDesc) textureVKDesc, NriOut NriRef(Texture*) texture);
    Nri(Result) (NRI_CALL *CreateMemoryVK)                  (NriRef(Device) device, const NriRef(MemoryVKDesc) memoryVKDesc, NriOut NriRef(Memory*) memory);
    Nri(Result) (NRI_CALL *CreateGraphicsPipelineVK)        (NriRef(Device) device, VKNonDispatchableHandle vkPipeline, NriOut NriRef(Pipeline*) pipeline);
    Nri(Result) (NRI_CALL *CreateComputePipelineVK)         (NriRef(Device) device, VKNonDispatchableHandle vkPipeline, NriOut NriRef(Pipeline*) pipeline);
    Nri(Result) (NRI_CALL *CreateQueryPoolVK)               (NriRef(Device) device, const NriRef(QueryPoolVKDesc) queryPoolVKDesc, NriOut NriRef(QueryPool*) queryPool);
    Nri(Result) (NRI_CALL *CreateAccelerationStructureVK)   (NriRef(Device) device, const NriRef(AccelerationStructureVKDesc) accelerationStructureVKDesc, NriOut NriRef(AccelerationStructure*) accelerationStructure);
    VKHandle    (NRI_CALL *GetPhysicalDeviceVK)             (const NriRef(Device) device);
    VKHandle    (NRI_CALL *GetInstanceVK)                   (const NriRef(Device) device);
    void*       (NRI_CALL *GetInstanceProcAddrVK)           (const NriRef(Device) device);
    void*       (NRI_CALL *GetDeviceProcAddrVK)             (const NriRef(Device) device);
};

NRI_API Nri(Result) NRI_CALL nriCreateDeviceFromVkDevice(const NriRef(DeviceCreationVKDesc) deviceDesc, NriOut NriRef(Device*) device);

NriNamespaceEnd
