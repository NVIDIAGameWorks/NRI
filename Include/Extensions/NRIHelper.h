// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_STRUCT(VideoMemoryInfo)
{
    uint64_t budgetSize; // the OS-provided video memory budget. If "usageSize" > "budgetSize", the application may incur stuttering or performance penalties
    uint64_t usageSize; // specifies the application’s current video memory usage
};

NRI_STRUCT(TextureSubresourceUploadDesc)
{
    const void* slices;
    uint32_t sliceNum;
    uint32_t rowPitch;
    uint32_t slicePitch;
};

NRI_STRUCT(TextureUploadDesc)
{
    const NRI_NAME(TextureSubresourceUploadDesc)* subresources; // must include ALL subresources = arraySize * mipNum
    NRI_NAME(Texture)* texture;
    NRI_NAME(AccessLayoutStage) after;
};

NRI_STRUCT(BufferUploadDesc)
{
    const void* data;
    uint64_t dataSize;
    NRI_NAME(Buffer)* buffer;
    uint64_t bufferOffset;
    NRI_NAME(AccessStage) after;
};

NRI_STRUCT(ResourceGroupDesc)
{
    NRI_NAME(MemoryLocation) memoryLocation;
    NRI_NAME(Texture)* const* textures;
    uint32_t textureNum;
    NRI_NAME(Buffer)* const* buffers;
    uint32_t bufferNum;
    uint64_t preferredMemorySize; // desired chunk size (but can be greater if a resource doesn't fit), 256 Mb if 0
};

NRI_STRUCT(FormatProps)
{
    const char* name;            // format name
    NRI_NAME(Format) format;     // self
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

NRI_STRUCT(HelperInterface)
{
    // Optimized memory allocation for a group of resources
    uint32_t (NRI_CALL *CalculateAllocationNumber)(const NRI_NAME_REF(Device) device, const NRI_NAME_REF(ResourceGroupDesc) resourceGroupDesc);
    NRI_NAME(Result) (NRI_CALL *AllocateAndBindMemory)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(ResourceGroupDesc) resourceGroupDesc, NRI_NAME(Memory)** allocations);

    // Populate resources with data (not for streaming!)
    NRI_NAME(Result) (NRI_CALL *UploadData)(NRI_NAME_REF(CommandQueue) commandQueue, const NRI_NAME(TextureUploadDesc)* textureUploadDescs, uint32_t textureUploadDescNum, const NRI_NAME(BufferUploadDesc)* bufferUploadDescs, uint32_t bufferUploadDescNum);

    // WFI
    NRI_NAME(Result) (NRI_CALL *WaitForIdle)(NRI_NAME_REF(CommandQueue) commandQueue);

    // Information about video memory
    NRI_NAME(Result) (NRI_CALL *QueryVideoMemoryInfo)(const NRI_NAME_REF(Device) device, NRI_NAME(MemoryLocation) memoryLocation, NRI_NAME_REF(VideoMemoryInfo) videoMemoryInfo);
};

// Format utilities
NRI_API NRI_NAME(Format) NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat);
NRI_API NRI_NAME(Format) NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat);
NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(NRI_NAME(Format) format);
NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(NRI_NAME(Format) format);
NRI_API const NRI_NAME_REF(FormatProps) NRI_CALL nriGetFormatProps(NRI_NAME(Format) format);

// Strings
NRI_API const char* NRI_CALL nriGetGraphicsAPIString(NRI_NAME(GraphicsAPI) graphicsAPI);

// A friendly way to get a supported depth format
static inline NRI_NAME(Format) NRI_FUNC_NAME(GetSupportedDepthFormat)(const NRI_NAME_REF(CoreInterface) coreInterface, const NRI_NAME_REF(Device) device, uint32_t minBits, bool stencil)
{
    if (minBits <= 16 && !stencil) {
        if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D16_UNORM)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NRI_ENUM_MEMBER(Format, D16_UNORM);
    }

    if (minBits <= 24) {
        if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D24_UNORM_S8_UINT)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NRI_ENUM_MEMBER(Format, D24_UNORM_S8_UINT);
    }

    if (minBits <= 32 && !stencil) {
        if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D32_SFLOAT)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NRI_ENUM_MEMBER(Format, D32_SFLOAT);
    }

    if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D32_SFLOAT_S8_UINT_X24)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
        return NRI_ENUM_MEMBER(Format, D32_SFLOAT_S8_UINT_X24);

    // Should be unreachable
    return NRI_ENUM_MEMBER(Format, UNKNOWN);
}

// "TextureDesc" constructors
static inline NRI_NAME(TextureDesc) NRI_FUNC_NAME(Texture1D)(NRI_NAME(Format) format,
    uint16_t width,
    NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(1),
    NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(1),
    NRI_NAME(TextureUsageBits) usageMask NRI_DEFAULT_VALUE(NRI_ENUM_MEMBER(TextureUsageBits, SHADER_RESOURCE)))
{
    NRI_NAME(TextureDesc) textureDesc = NRI_ZERO_INIT;
    textureDesc.type = NRI_ENUM_MEMBER(TextureType, TEXTURE_1D);
    textureDesc.format = format;
    textureDesc.usageMask = usageMask;
    textureDesc.width = width;
    textureDesc.height = 1;
    textureDesc.depth = 1;
    textureDesc.mipNum = mipNum;
    textureDesc.arraySize = arraySize;
    textureDesc.sampleNum = 1;

    return textureDesc;
}

static inline NRI_NAME(TextureDesc) NRI_FUNC_NAME(Texture2D)(NRI_NAME(Format) format,
    NRI_NAME(Dim_t) width,
    NRI_NAME(Dim_t) height,
    NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(1),
    NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(1),
    NRI_NAME(TextureUsageBits) usageMask NRI_DEFAULT_VALUE(NRI_ENUM_MEMBER(TextureUsageBits, SHADER_RESOURCE)),
    NRI_NAME(Sample_t) sampleNum NRI_DEFAULT_VALUE(1))
{
    NRI_NAME(TextureDesc) textureDesc = NRI_ZERO_INIT;
    textureDesc.type = NRI_ENUM_MEMBER(TextureType, TEXTURE_2D);
    textureDesc.format = format;
    textureDesc.usageMask = usageMask;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.depth = 1;
    textureDesc.mipNum = mipNum;
    textureDesc.arraySize = arraySize;
    textureDesc.sampleNum = sampleNum;

    return textureDesc;
}

static inline NRI_NAME(TextureDesc) NRI_FUNC_NAME(Texture3D)(NRI_NAME(Format) format,
    NRI_NAME(Dim_t) width,
    NRI_NAME(Dim_t) height,
    uint16_t depth,
    NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(1),
    NRI_NAME(TextureUsageBits) usageMask NRI_DEFAULT_VALUE(NRI_ENUM_MEMBER(TextureUsageBits, SHADER_RESOURCE)))
{
    NRI_NAME(TextureDesc) textureDesc = NRI_ZERO_INIT;
    textureDesc.type = NRI_ENUM_MEMBER(TextureType, TEXTURE_3D);
    textureDesc.format = format;
    textureDesc.usageMask = usageMask;
    textureDesc.width = width;
    textureDesc.height = height;
    textureDesc.depth = depth;
    textureDesc.mipNum = mipNum;
    textureDesc.arraySize = 1;
    textureDesc.sampleNum = 1;

    return textureDesc;
}

// "TextureBarrierDesc" constructors
static inline NRI_NAME(TextureBarrierDesc) NRI_FUNC_NAME(TextureBarrier)(NRI_NAME(Texture)* texture,
    NRI_NAME(AccessLayoutStage) before,
    NRI_NAME(AccessLayoutStage) after,
    NRI_NAME(Mip_t) mipOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_MIP_LEVELS)),
    NRI_NAME(Dim_t) arrayOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_ARRAY_LAYERS)))
{
    NRI_NAME(TextureBarrierDesc) textureBarrierDesc = NRI_ZERO_INIT;
    textureBarrierDesc.texture = texture;
    textureBarrierDesc.before = before;
    textureBarrierDesc.after = after;
    textureBarrierDesc.mipOffset = mipOffset;
    textureBarrierDesc.mipNum = mipNum;
    textureBarrierDesc.arrayOffset = arrayOffset;
    textureBarrierDesc.arraySize = arraySize;

    return textureBarrierDesc;
}

static inline NRI_NAME(TextureBarrierDesc) NRI_FUNC_NAME(TextureBarrierFromUnknown)(NRI_NAME(Texture)* texture,
    NRI_NAME(AccessLayoutStage) after,
    NRI_NAME(Mip_t) mipOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_MIP_LEVELS)),
    NRI_NAME(Dim_t) arrayOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_ARRAY_LAYERS)))
{
    NRI_NAME(TextureBarrierDesc) textureBarrierDesc = NRI_ZERO_INIT;
    textureBarrierDesc.texture = texture;
    textureBarrierDesc.before.access = NRI_ENUM_MEMBER(AccessBits, UNKNOWN);
    textureBarrierDesc.before.layout = NRI_ENUM_MEMBER(Layout, UNKNOWN);
    textureBarrierDesc.before.stages = NRI_ENUM_MEMBER(StageBits, ALL);
    textureBarrierDesc.after = after;
    textureBarrierDesc.mipOffset = mipOffset;
    textureBarrierDesc.mipNum = mipNum;
    textureBarrierDesc.arrayOffset = arrayOffset;
    textureBarrierDesc.arraySize = arraySize;

    return textureBarrierDesc;
}

static inline NRI_NAME(TextureBarrierDesc) NRI_FUNC_NAME(TextureBarrierFromState)(NRI_NAME_REF(TextureBarrierDesc) prevState,
    NRI_NAME(AccessLayoutStage) after,
    NRI_NAME(Mip_t) mipOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_MIP_LEVELS)))
{
    NRI_REF_ACCESS(prevState)->mipOffset = mipOffset;
    NRI_REF_ACCESS(prevState)->mipNum = mipNum;
    NRI_REF_ACCESS(prevState)->before = NRI_REF_ACCESS(prevState)->after;
    NRI_REF_ACCESS(prevState)->after = after;

    return *NRI_REF_ACCESS(prevState);
}

NRI_NAMESPACE_END
