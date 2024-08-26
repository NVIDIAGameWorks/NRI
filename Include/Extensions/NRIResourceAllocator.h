// © 2021 NVIDIA Corporation

#pragma once

// Convenient creation of resources, which get returned already bound to memory.
// AMD Virtual Memory Allocator is used for memory allocations management:
//  https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//  https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator

NRI_NAMESPACE_BEGIN

NRI_STRUCT(AllocateBufferDesc)
{
    NRI_NAME(BufferDesc) desc;
    NRI_NAME(MemoryLocation) memoryLocation;
    float memoryPriority; // [-1; 1]: low < 0, normal = 0, high > 0
};

NRI_STRUCT(AllocateTextureDesc)
{
    NRI_NAME(TextureDesc) desc;
    NRI_NAME(MemoryLocation) memoryLocation;
    float memoryPriority;
};

NRI_STRUCT(AllocateAccelerationStructureDesc)
{
    NRI_NAME(AccelerationStructureDesc) desc; // TODO: requires NRIRayTracing.h
    NRI_NAME(MemoryLocation) memoryLocation;
    float memoryPriority;
};

NRI_STRUCT(ResourceAllocatorInterface)
{
    NRI_NAME(Result) (NRI_CALL *AllocateBuffer)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AllocateBufferDesc) bufferDesc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *AllocateTexture)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AllocateTextureDesc) textureDesc, NRI_NAME_REF(Texture*) texture);
    NRI_NAME(Result) (NRI_CALL *AllocateAccelerationStructure)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AllocateAccelerationStructureDesc) accelerationStructureDesc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);
};

NRI_NAMESPACE_END
