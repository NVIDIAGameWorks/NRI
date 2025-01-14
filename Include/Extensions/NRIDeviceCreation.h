// © 2021 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriEnum(Message, uint8_t,
    INFO,
    WARNING,
    ERROR // "wingdi.h" must not be included after
);

NriStruct(AllocationCallbacks) {
    void* (*Allocate)(void* userArg, size_t size, size_t alignment);
    void* (*Reallocate)(void* userArg, void* memory, size_t size, size_t alignment);
    void (*Free)(void* userArg, void* memory);
    void* userArg;
};

NriStruct(CallbackInterface) {
    void (*MessageCallback)(Nri(Message) messageType, const char* file, uint32_t line, const char* message, void* userArg);
    void (*AbortExecution)(void* userArg);
    void* userArg;
};

// Use largest offset for the resource type planned to be used as an unbounded array
NriStruct(SPIRVBindingOffsets) {
    uint32_t samplerOffset;
    uint32_t textureOffset;
    uint32_t constantBufferOffset;
    uint32_t storageTextureAndBufferOffset;
};

NriStruct(VKExtensions) {
    const char* const* instanceExtensions;
    uint32_t instanceExtensionNum;
    const char* const* deviceExtensions;
    uint32_t deviceExtensionNum;
};

NriStruct(DeviceCreationDesc) {
    NriOptional const NriPtr(AdapterDesc) adapterDesc;
    NriOptional Nri(CallbackInterface) callbackInterface;
    NriOptional Nri(AllocationCallbacks) allocationCallbacks;
    Nri(SPIRVBindingOffsets) spirvBindingOffsets;
    NriOptional Nri(VKExtensions) vkExtensions;
    Nri(GraphicsAPI) graphicsAPI;
    NriOptional Nri(Robustness) robustness;
    NriOptional uint32_t shaderExtRegister;     // vendor specific shader extensions (default is NRI_SHADER_EXT_REGISTER, space is always "0")

    // Switches (disabled by default)
    bool enableNRIValidation;
    bool enableGraphicsAPIValidation;
    bool enableD3D12DrawParametersEmulation;    // not needed for VK, unsupported by D3D11
    bool enableD3D11CommandBufferEmulation;     // enable? but why? (auto-enabled if deferred contexts are not supported)

    // Switches (enabled by default)
    bool disableVKRayTracing;                   // to save CPU memory in some implementations
    bool disable3rdPartyAllocationCallbacks;    // to use "allocationCallbacks" only for NRI needs
};

// if "adapterDescs == NULL", then "adapterDescNum" gets set to the number of adapters
// else "adapterDescNum" must be set to number of elements in "adapterDescs"
NRI_API Nri(Result) NRI_CALL nriEnumerateAdapters(NriPtr(AdapterDesc) adapterDescs, NonNriRef(uint32_t) adapterDescNum);

NRI_API Nri(Result) NRI_CALL nriCreateDevice(const NriRef(DeviceCreationDesc) deviceCreationDesc, NriOut NriRef(Device*) device);
NRI_API void NRI_CALL nriDestroyDevice(NriRef(Device) device);

// It's global state for D3D, not needed for VK because validation is tied to the logical device
NRI_API void NRI_CALL nriReportLiveObjects();

NriNamespaceEnd
