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

NRI_STRUCT(SPIRVBindingOffsets)
{
    uint32_t samplerOffset;
    uint32_t textureOffset;
    uint32_t constantBufferOffset;
    uint32_t storageTextureAndBufferOffset;
};

NRI_STRUCT(VulkanExtensions)
{
    const char* const* instanceExtensions;
    uint32_t instanceExtensionNum;
    const char* const* deviceExtensions;
    uint32_t deviceExtensionNum;
};

NRI_STRUCT(DeviceCreationDesc)
{
    const NRI_NAME(AdapterDesc)* adapterDesc;
    NRI_NAME(CallbackInterface) callbackInterface;
    NRI_NAME(MemoryAllocatorInterface) memoryAllocatorInterface;
    NRI_NAME(SPIRVBindingOffsets) spirvBindingOffsets;
    NRI_NAME(VulkanExtensions) vulkanExtensions;
    NRI_NAME(GraphicsAPI) graphicsAPI;
    bool enableNRIValidation;
    bool enableAPIValidation;
    bool enableD3D12DrawParametersEmulation;
    bool enableD3D11CommandBufferEmulation; // force enable, but why?
    bool disableVulkanRayTracing; // to save some CPU memory
};

NRI_API NRI_NAME(Result) NRI_CALL nriEnumerateAdapters(NRI_NAME(AdapterDesc)* adapterDescs, uint32_t NRI_REF adapterDescNum);
NRI_API NRI_NAME(Result) NRI_CALL nriCreateDevice(const NRI_NAME_REF(DeviceCreationDesc) deviceCreationDesc, NRI_NAME_REF(Device*) device);
NRI_API void NRI_CALL nriDestroyDevice(NRI_NAME_REF(Device) device);
NRI_API void NRI_CALL nriReportLiveObjects(); // It's global state for D3D, not needed for VK because validation is tied to the logical device

NRI_NAMESPACE_END
