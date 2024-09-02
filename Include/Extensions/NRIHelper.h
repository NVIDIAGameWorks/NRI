// © 2021 NVIDIA Corporation

#pragma once

NriNamespaceBegin

NriStruct(VideoMemoryInfo) {
    uint64_t budgetSize;    // the OS-provided video memory budget. If "usageSize" > "budgetSize", the application may incur stuttering or performance penalties
    uint64_t usageSize;     // specifies the application’s current video memory usage
};

NriStruct(TextureSubresourceUploadDesc) {
    const void* slices;
    uint32_t sliceNum;
    uint32_t rowPitch;
    uint32_t slicePitch;
};

NriStruct(TextureUploadDesc) {
    NriOptional const NriPtr(TextureSubresourceUploadDesc) subresources; // if provided, must include ALL subresources = layerNum * mipNum
    NriPtr(Texture) texture;
    Nri(AccessLayoutStage) after;
    Nri(PlaneBits) planes;
};

NriStruct(BufferUploadDesc) {
    const void* data;
    uint64_t dataSize;
    NriPtr(Buffer) buffer;
    uint64_t bufferOffset;
    Nri(AccessStage) after;
};

NriStruct(ResourceGroupDesc) {
    Nri(MemoryLocation) memoryLocation;
    NriPtr(Texture) const* textures;
    uint32_t textureNum;
    NriPtr(Buffer) const* buffers;
    uint32_t bufferNum;
    uint64_t preferredMemorySize; // desired chunk size (but can be greater if a resource doesn't fit), 256 Mb if 0
};

NriStruct(FormatProps) {
    const char* name;            // format name
    Nri(Format) format;          // self
    uint8_t redBits;             // R (or depth) bits
    uint8_t greenBits;           // G (or stencil) bits (0 if channels < 2)
    uint8_t blueBits;            // B bits (0 if channels < 3)
    uint8_t alphaBits;           // A (or shared exponent) bits (0 if channels < 4)
    uint32_t stride         : 6; // block size in bytes
    uint32_t blockWidth     : 4; // 1 for plain formats, >1 for compressed
    uint32_t blockHeight    : 4; // 1 for plain formats, >1 for compressed
    uint32_t isBgr          : 1; // reversed channels (RGBA => BGRA)
    uint32_t isCompressed   : 1; // block-compressed format
    uint32_t isDepth        : 1; // has depth component
    uint32_t isExpShared    : 1; // shared exponent in alpha channel
    uint32_t isFloat        : 1; // floating point
    uint32_t isPacked       : 1; // 16- or 32- bit packed
    uint32_t isInteger      : 1; // integer
    uint32_t isNorm         : 1; // [0; 1] normalized
    uint32_t isSigned       : 1; // signed
    uint32_t isSrgb         : 1; // sRGB
    uint32_t isStencil      : 1; // has stencil component
    uint32_t unused         : 7;
};

NriStruct(HelperInterface) {
    // Optimized memory allocation for a group of resources
    uint32_t (NRI_CALL *CalculateAllocationNumber)(const NriRef(Device) device, const NriRef(ResourceGroupDesc) resourceGroupDesc);
    Nri(Result) (NRI_CALL *AllocateAndBindMemory)(NriRef(Device) device, const NriRef(ResourceGroupDesc) resourceGroupDesc, NriPtr(Memory)* allocations);

    // Populate resources with data (not for streaming!)
    Nri(Result) (NRI_CALL *UploadData)(NriRef(CommandQueue) commandQueue, const NriPtr(TextureUploadDesc) textureUploadDescs, uint32_t textureUploadDescNum, const NriPtr(BufferUploadDesc) bufferUploadDescs, uint32_t bufferUploadDescNum);

    // WFI
    Nri(Result) (NRI_CALL *WaitForIdle)(NriRef(CommandQueue) commandQueue);

    // Information about video memory
    Nri(Result) (NRI_CALL *QueryVideoMemoryInfo)(const NriRef(Device) device, Nri(MemoryLocation) memoryLocation, NriOut NriRef(VideoMemoryInfo) videoMemoryInfo);
};

// Format utilities
NRI_API Nri(Format) NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat);
NRI_API Nri(Format) NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat);
NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(Nri(Format) format);
NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(Nri(Format) format);
NRI_API const NriRef(FormatProps) NRI_CALL nriGetFormatProps(Nri(Format) format);

// Strings
NRI_API const char* NRI_CALL nriGetGraphicsAPIString(Nri(GraphicsAPI) graphicsAPI);

// "TextureDesc" constructors
static inline Nri(TextureDesc) NriFunc(Texture1D)(Nri(Format) format,
    uint16_t width,
    Nri(Mip_t) mipNum NriDefault(1),
    Nri(Dim_t) layerNum NriDefault(1),
    Nri(TextureUsageBits) usageMask NriDefault(NriScopedMember(TextureUsageBits, SHADER_RESOURCE)))
{
    Nri(TextureDesc) textureDesc = NriZero;
    textureDesc.type = NriScopedMember(TextureType, TEXTURE_1D);
    textureDesc.format = format;
    textureDesc.usageMask = usageMask;
    textureDesc.width = width;
    textureDesc.height = 1;
    textureDesc.depth = 1;
    textureDesc.mipNum = mipNum;
    textureDesc.layerNum = layerNum;
    textureDesc.sampleNum = 1;

    return textureDesc;
}

static inline Nri(TextureDesc) NriFunc(Texture2D)(Nri(Format) format,
    Nri(Dim_t) width,
    Nri(Dim_t) height,
    Nri(Mip_t) mipNum NriDefault(1),
    Nri(Dim_t) layerNum NriDefault(1),
    Nri(TextureUsageBits) usageMask NriDefault(NriScopedMember(TextureUsageBits, SHADER_RESOURCE)),
    Nri(Sample_t) sampleNum NriDefault(1))
{
    Nri(TextureDesc) textureDesc = NriZero;
    textureDesc.type = NriScopedMember(TextureType, TEXTURE_2D);
    textureDesc.format = format;
    textureDesc.usageMask = usageMask;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.depth = 1;
    textureDesc.mipNum = mipNum;
    textureDesc.layerNum = layerNum;
    textureDesc.sampleNum = sampleNum;

    return textureDesc;
}

static inline Nri(TextureDesc) NriFunc(Texture3D)(Nri(Format) format,
    Nri(Dim_t) width,
    Nri(Dim_t) height,
    uint16_t depth,
    Nri(Mip_t) mipNum NriDefault(1),
    Nri(TextureUsageBits) usageMask NriDefault(NriScopedMember(TextureUsageBits, SHADER_RESOURCE)))
{
    Nri(TextureDesc) textureDesc = NriZero;
    textureDesc.type = NriScopedMember(TextureType, TEXTURE_3D);
    textureDesc.format = format;
    textureDesc.usageMask = usageMask;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.depth = depth;
    textureDesc.mipNum = mipNum;
    textureDesc.layerNum = 1;
    textureDesc.sampleNum = 1;

    return textureDesc;
}

// "TextureBarrierDesc" constructors
static inline Nri(TextureBarrierDesc) NriFunc(TextureBarrier)(NriPtr(Texture) texture,
    Nri(AccessLayoutStage) before,
    Nri(AccessLayoutStage) after,
    Nri(Mip_t) mipOffset NriDefault(0),
    Nri(Mip_t) mipNum NriDefault(Nri(REMAINING_MIPS)),
    Nri(Dim_t) layerOffset NriDefault(0),
    Nri(Dim_t) layerNum NriDefault(Nri(REMAINING_LAYERS)))
{
    Nri(TextureBarrierDesc) textureBarrierDesc = NriZero;
    textureBarrierDesc.texture = texture;
    textureBarrierDesc.before = before;
    textureBarrierDesc.after = after;
    textureBarrierDesc.mipOffset = mipOffset;
    textureBarrierDesc.mipNum = mipNum;
    textureBarrierDesc.layerOffset = layerOffset;
    textureBarrierDesc.layerNum = layerNum;

    return textureBarrierDesc;
}

static inline Nri(TextureBarrierDesc) NriFunc(TextureBarrierFromUnknown)(NriPtr(Texture) texture,
    Nri(AccessLayoutStage) after,
    Nri(Mip_t) mipOffset NriDefault(0),
    Nri(Mip_t) mipNum NriDefault(Nri(REMAINING_MIPS)),
    Nri(Dim_t) layerOffset NriDefault(0),
    Nri(Dim_t) layerNum NriDefault(Nri(REMAINING_LAYERS)))
{
    Nri(TextureBarrierDesc) textureBarrierDesc = NriZero;
    textureBarrierDesc.texture = texture;
    textureBarrierDesc.before.access = NriScopedMember(AccessBits, UNKNOWN);
    textureBarrierDesc.before.layout = NriScopedMember(Layout, UNKNOWN);
    textureBarrierDesc.before.stages = NriScopedMember(StageBits, ALL);
    textureBarrierDesc.after = after;
    textureBarrierDesc.mipOffset = mipOffset;
    textureBarrierDesc.mipNum = mipNum;
    textureBarrierDesc.layerOffset = layerOffset;
    textureBarrierDesc.layerNum = layerNum;

    return textureBarrierDesc;
}

static inline Nri(TextureBarrierDesc) NriFunc(TextureBarrierFromState)(NriRef(TextureBarrierDesc) prevState,
    Nri(AccessLayoutStage) after,
    Nri(Mip_t) mipOffset NriDefault(0),
    Nri(Mip_t) mipNum NriDefault(Nri(REMAINING_MIPS)))
{
    NriDeref(prevState)->mipOffset = mipOffset;
    NriDeref(prevState)->mipNum = mipNum;
    NriDeref(prevState)->before = NriDeref(prevState)->after;
    NriDeref(prevState)->after = after;

    return *NriDeref(prevState);
}

NriNamespaceEnd
