// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_ENUM
(
    Message, uint8_t,

    TYPE_INFO,
    TYPE_WARNING,
    TYPE_ERROR,

    MAX_NUM
);

NRI_STRUCT(MemoryAllocatorInterface)
{
    void* (*Allocate)(void* userArg, size_t size, size_t alignment);
    void* (*Reallocate)(void* userArg, void* memory, size_t size, size_t alignment);
    void (*Free)(void* userArg, void* memory);
    void* userArg;
};

NRI_STRUCT(CallbackInterface)
{
    void (*MessageCallback)(NRI_NAME(Message) messageType, const char* file, uint32_t line, const char* message, void* userArg);
    void (*AbortExecution)(void* userArg);
    void* userArg;
};

// Use largest offset for the resource type planned to be used as an unbounded array
NRI_STRUCT(SPIRVBindingOffsets)
{
    uint32_t samplerOffset;
    uint32_t textureOffset;
    uint32_t constantBufferOffset;
    uint32_t storageTextureAndBufferOffset;
};

NRI_STRUCT(VKExtensions)
{
    const char* const* instanceExtensions;
    uint32_t instanceExtensionNum;
    const char* const* deviceExtensions;
    uint32_t deviceExtensionNum;
};

NRI_STRUCT(DeviceCreationDesc)
{
    const NRI_NAME(AdapterDesc)* adapterDesc; // optional
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    NRI_NAME(SPIRVBindingOffsets) spirvBindingOffsets;
    NRI_NAME(VKExtensions) vkExtensions;
    NRI_NAME(GraphicsAPI) graphicsAPI;
    uint32_t shaderExtRegister; // D3D12/D3D11 only
    uint32_t shaderExtSpace; // D3D12 only

    // Switches (disabled by default)
    bool enableNRIValidation;
    bool enableGraphicsAPIValidation;
    bool enableD3D12DrawParametersEmulation; // not needed for VK, unsupported by D3D11
    bool enableD3D11CommandBufferEmulation; // enable? but why? (auto-enabled if deferred contexts are not supported)

    // Switches (enabled by default)
    bool disableVKRayTracing; // to save CPU memory in some implementations
};

NRI_API NRI_NAME(Result) NRI_CALL nriEnumerateAdapters(NRI_NAME(AdapterDesc)* adapterDescs, uint32_t NRI_REF adapterDescNum);
NRI_API NRI_NAME(Result) NRI_CALL nriCreateDevice(const NRI_NAME_REF(DeviceCreationDesc) deviceCreationDesc, NRI_NAME_REF(Device*) device);
NRI_API void NRI_CALL nriDestroyDevice(NRI_NAME_REF(Device) device);
NRI_API void NRI_CALL nriReportLiveObjects(); // It's global state for D3D, not needed for VK because validation is tied to the logical device

NRI_NAMESPACE_END
