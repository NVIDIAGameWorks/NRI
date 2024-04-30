// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h"

NRI_FORWARD_STRUCT(ID3D12Heap);
NRI_FORWARD_STRUCT(ID3D12Device);
NRI_FORWARD_STRUCT(ID3D12Resource);
NRI_FORWARD_STRUCT(ID3D12CommandQueue);
NRI_FORWARD_STRUCT(ID3D12CommandAllocator);
NRI_FORWARD_STRUCT(ID3D12GraphicsCommandList);

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(AccelerationStructure);

NRI_STRUCT(DeviceCreationD3D12Desc)
{
    ID3D12Device* d3d12Device;
    ID3D12CommandQueue* d3d12GraphicsQueue;
    ID3D12CommandQueue* d3d12ComputeQueue;
    ID3D12CommandQueue* d3d12CopyQueue;
    AGSContext* agsContext; // can be NULL
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    bool enableDrawParametersEmulation;
    bool enableNRIValidation;
    bool isNVAPILoaded; // At least NVAPI requires calling "NvAPI_Initialize" in DLL/EXE where the device is created in addition to NRI
};

NRI_STRUCT(CommandBufferD3D12Desc)
{
    ID3D12GraphicsCommandList* d3d12CommandList;
    ID3D12CommandAllocator* d3d12CommandAllocator;
};

NRI_STRUCT(BufferD3D12Desc)
{
    ID3D12Resource* d3d12Resource;
    uint32_t structureStride;
};

NRI_STRUCT(TextureD3D12Desc)
{
    ID3D12Resource* d3d12Resource;
};

NRI_STRUCT(MemoryD3D12Desc)
{
    ID3D12Heap* d3d12Heap;
};

NRI_STRUCT(AccelerationStructureD3D12Desc)
{
    ID3D12Resource* d3d12Resource;
    uint64_t scratchDataSize;
    uint64_t updateScratchDataSize;
};

NRI_STRUCT(WrapperD3D12Interface)
{
    NRI_NAME(Result) (NRI_CALL *CreateCommandBufferD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(CommandBufferD3D12Desc) commandBufferD3D12Desc, NRI_NAME_REF(CommandBuffer*) commandBuffer);
    NRI_NAME(Result) (NRI_CALL *CreateBufferD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(BufferD3D12Desc) bufferD3D12Desc, NRI_NAME_REF(Buffer*) buffer);
    NRI_NAME(Result) (NRI_CALL *CreateTextureD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(TextureD3D12Desc) textureD3D12Desc, NRI_NAME_REF(Texture*) texture);
    NRI_NAME(Result) (NRI_CALL *CreateMemoryD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(MemoryD3D12Desc) memoryD3D12Desc, NRI_NAME_REF(Memory*) memory);
    NRI_NAME(Result) (NRI_CALL *CreateAccelerationStructureD3D12)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(AccelerationStructureD3D12Desc) accelerationStructureD3D12Desc, NRI_NAME_REF(AccelerationStructure*) accelerationStructure);
};

NRI_API NRI_NAME(Result) NRI_CALL nriCreateDeviceFromD3D12Device(const NRI_NAME_REF(DeviceCreationD3D12Desc) deviceDesc, NRI_NAME_REF(Device*) device);

NRI_NAMESPACE_END
