// © 2021 NVIDIA Corporation

#pragma once

#include "NRIDeviceCreation.h" // CallbackInterface, AllocationCallbacks

NonNriForwardStruct(AGSContext);
NonNriForwardStruct(ID3D12Heap);
NonNriForwardStruct(ID3D12Device);
NonNriForwardStruct(ID3D12Resource);
NonNriForwardStruct(D3D12_HEAP_DESC);
NonNriForwardStruct(ID3D12CommandQueue);
NonNriForwardStruct(ID3D12DescriptorHeap);
NonNriForwardStruct(ID3D12CommandAllocator);
NonNriForwardStruct(ID3D12GraphicsCommandList);

NriNamespaceBegin

NriForwardStruct(AccelerationStructure);

NriStruct(DeviceCreationD3D12Desc) {
    ID3D12Device* d3d12Device;
    ID3D12CommandQueue* d3d12GraphicsQueue;
    ID3D12CommandQueue* d3d12ComputeQueue;
    ID3D12CommandQueue* d3d12CopyQueue;
    NriOptional AGSContext* agsContext;
    Nri(CallbackInterface) callbackInterface;
    Nri(AllocationCallbacks) allocationCallbacks;
    bool enableD3D12DrawParametersEmulation;
    bool enableNRIValidation;
    bool isNVAPILoaded; // at least NVAPI requires calling "NvAPI_Initialize" in DLL/EXE where the device is created in addition to NRI
};

NriStruct(CommandBufferD3D12Desc) {
    ID3D12GraphicsCommandList* d3d12CommandList;
    ID3D12CommandAllocator* d3d12CommandAllocator;
};

NriStruct(DescriptorPoolD3D12Desc) {
    ID3D12DescriptorHeap* d3d12ResourceDescriptorHeap;
    ID3D12DescriptorHeap* d3d12SamplerDescriptorHeap;
    uint32_t descriptorSetMaxNum;
};

NriStruct(BufferD3D12Desc) {
    ID3D12Resource* d3d12Resource;
    NriOptional const NriPtr(BufferDesc) desc;  // not all information can be retrieved from the resource if not provided
    NriOptional uint32_t structureStride;       // must be provided if used as a structured or raw buffer
};

NriStruct(TextureD3D12Desc) {
    ID3D12Resource* d3d12Resource;
    NriOptional const NriPtr(TextureDesc) desc; // not all information can be retrieved from the resource if not provided
};

NriStruct(MemoryD3D12Desc) {
    ID3D12Heap* d3d12Heap;
};

NriStruct(AccelerationStructureD3D12Desc) {
    ID3D12Resource* d3d12Resource;
    uint64_t scratchDataSize;
    uint64_t updateScratchDataSize;
};

NriStruct(WrapperD3D12Interface) {
    Nri(Result) (NRI_CALL *CreateCommandBufferD3D12)            (NriRef(Device) device, const NriRef(CommandBufferD3D12Desc) commandBufferD3D12Desc, NriOut NriRef(CommandBuffer*) commandBuffer);
    Nri(Result) (NRI_CALL *CreateDescriptorPoolD3D12)           (NriRef(Device) device, const NriRef(DescriptorPoolD3D12Desc) descriptorPoolD3D12Desc, NriOut NriRef(DescriptorPool*) descriptorPool);
    Nri(Result) (NRI_CALL *CreateBufferD3D12)                   (NriRef(Device) device, const NriRef(BufferD3D12Desc) bufferD3D12Desc, NriOut NriRef(Buffer*) buffer);
    Nri(Result) (NRI_CALL *CreateTextureD3D12)                  (NriRef(Device) device, const NriRef(TextureD3D12Desc) textureD3D12Desc, NriOut NriRef(Texture*) texture);
    Nri(Result) (NRI_CALL *CreateMemoryD3D12)                   (NriRef(Device) device, const NriRef(MemoryD3D12Desc) memoryD3D12Desc, NriOut NriRef(Memory*) memory);
    Nri(Result) (NRI_CALL *CreateAccelerationStructureD3D12)    (NriRef(Device) device, const NriRef(AccelerationStructureD3D12Desc) accelerationStructureD3D12Desc, NriOut NriRef(AccelerationStructure*) accelerationStructure);
};

NRI_API Nri(Result) NRI_CALL nriCreateDeviceFromD3D12Device(const NriRef(DeviceCreationD3D12Desc) deviceDesc, NriOut NriRef(Device*) device);

NriNamespaceEnd
