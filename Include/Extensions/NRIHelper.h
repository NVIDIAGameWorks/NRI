// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_STRUCT(VideoMemoryInfo)
{
    uint64_t budget;
    uint64_t currentUsage;
    uint64_t availableForReservation;
    uint64_t currentReservation;
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
    const NRI_NAME(TextureSubresourceUploadDesc)* subresources;
    NRI_NAME(Texture)* texture;
    NRI_NAME(AccessAndLayout) nextState;
    NRI_NAME(Mip_t) mipNum;
    NRI_NAME(Dim_t) arraySize;
};

NRI_STRUCT(BufferUploadDesc)
{
    const void* data;
    uint64_t dataSize;
    NRI_NAME(Buffer)* buffer;
    uint64_t bufferOffset;
    NRI_NAME(AccessBits) prevAccess;
    NRI_NAME(AccessBits) nextAccess;
};

NRI_STRUCT(ResourceGroupDesc)
{
    NRI_NAME(MemoryLocation) memoryLocation;
    NRI_NAME(Texture)* const* textures;
    uint32_t textureNum;
    NRI_NAME(Buffer)* const* buffers;
    uint32_t bufferNum;
};

NRI_STRUCT(HelperInterface)
{
    uint32_t (NRI_CALL *CalculateAllocationNumber)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(ResourceGroupDesc) resourceGroupDesc);
    NRI_NAME(Result) (NRI_CALL *AllocateAndBindMemory)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(ResourceGroupDesc) resourceGroupDesc, NRI_NAME(Memory)** allocations);
    NRI_NAME(Result) (NRI_CALL *ChangeResourceStates)(NRI_NAME_REF(CommandQueue) commandQueue, const NRI_NAME_REF(TransitionBarrierDesc) transitionBarriers);
    NRI_NAME(Result) (NRI_CALL *UploadData)(NRI_NAME_REF(CommandQueue) commandQueue, const NRI_NAME(TextureUploadDesc)* textureUploadDescs, uint32_t textureUploadDescNum,
        const NRI_NAME(BufferUploadDesc)* bufferUploadDescs, uint32_t bufferUploadDescNum); // TODO: add 'nodeMask' to explicitly define which GPUs data should be uploaded on
    NRI_NAME(Result) (NRI_CALL *WaitForIdle)(NRI_NAME_REF(CommandQueue) commandQueue);
};

NRI_API bool NRI_CALL nriQueryVideoMemoryInfo(const NRI_NAME_REF(Device) device, NRI_NAME(MemoryLocation) memoryLocation, NRI_NAME_REF(VideoMemoryInfo) videoMemoryInfo);

NRI_API NRI_NAME(Format) NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat);
NRI_API NRI_NAME(Format) NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat);
NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(NRI_NAME(Format) format);
NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(NRI_NAME(Format) format);

NRI_API const char* NRI_CALL nriGetGraphicsAPIString(NRI_NAME(GraphicsAPI) graphicsAPI);
NRI_API const char* NRI_CALL nriGetFormatString(NRI_NAME(Format) format);

static inline NRI_NAME(Format) NRI_FUNC_NAME(GetSupportedDepthFormat)(const NRI_NAME_REF(CoreInterface) coreInterface, const NRI_NAME_REF(Device) device, uint32_t minBits, bool stencil)
{
    if (stencil)
    {
        if (minBits <= 24)
        {
            if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D24_UNORM_S8_UINT)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
                return NRI_ENUM_MEMBER(Format, D24_UNORM_S8_UINT);
        }
    }
    else
    {
        if (minBits <= 16)
        {
            if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D16_UNORM)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
                return NRI_ENUM_MEMBER(Format, D16_UNORM);
        }
        else if (minBits <= 24)
        {
            if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D24_UNORM_S8_UINT)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
                return NRI_ENUM_MEMBER(Format, D24_UNORM_S8_UINT);
        }

        if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D32_SFLOAT)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
            return NRI_ENUM_MEMBER(Format, D32_SFLOAT);
    }

    if (NRI_REF_ACCESS(coreInterface)->GetFormatSupport(device, NRI_ENUM_MEMBER(Format, D32_SFLOAT_S8_UINT_X24)) & NRI_ENUM_MEMBER(FormatSupportBits, DEPTH_STENCIL_ATTACHMENT))
        return NRI_ENUM_MEMBER(Format, D32_SFLOAT_S8_UINT_X24);

    return NRI_ENUM_MEMBER(Format, UNKNOWN);
}

static inline NRI_NAME(TextureDesc) NRI_FUNC_NAME(Texture1D)(NRI_NAME(Format) format, uint16_t width, NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(1), NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(1),
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

static inline NRI_NAME(TextureDesc) NRI_FUNC_NAME(Texture2D)(NRI_NAME(Format) format, NRI_NAME(Dim_t) width, NRI_NAME(Dim_t) height, NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(1), NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(1),
    NRI_NAME(TextureUsageBits) usageMask NRI_DEFAULT_VALUE(NRI_ENUM_MEMBER(TextureUsageBits, SHADER_RESOURCE)), NRI_NAME(Sample_t) sampleNum NRI_DEFAULT_VALUE(1))
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

static inline NRI_NAME(TextureDesc) NRI_FUNC_NAME(Texture3D)(NRI_NAME(Format) format, NRI_NAME(Dim_t) width, NRI_NAME(Dim_t) height, uint16_t depth, NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(1),
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

static inline NRI_NAME(TextureTransitionBarrierDesc) NRI_FUNC_NAME(TextureTransition)(NRI_NAME(Texture)* texture, NRI_NAME(AccessAndLayout) prevState, NRI_NAME(AccessAndLayout) nextState,
    NRI_NAME(Mip_t) mipOffset NRI_DEFAULT_VALUE(0), NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_MIP_LEVELS)), NRI_NAME(Dim_t) arrayOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_ARRAY_LAYERS)))
{
    NRI_NAME(TextureTransitionBarrierDesc) textureTransitionBarrierDesc = NRI_ZERO_INIT;
    textureTransitionBarrierDesc.texture = texture;
    textureTransitionBarrierDesc.prevState = prevState;
    textureTransitionBarrierDesc.nextState = nextState;
    textureTransitionBarrierDesc.mipOffset = mipOffset;
    textureTransitionBarrierDesc.mipNum = mipNum;
    textureTransitionBarrierDesc.arrayOffset = arrayOffset;
    textureTransitionBarrierDesc.arraySize = arraySize;

    return textureTransitionBarrierDesc;
}

static inline NRI_NAME(TextureTransitionBarrierDesc) NRI_FUNC_NAME(TextureTransitionFromUnknown)(NRI_NAME(Texture)* texture, NRI_NAME(AccessAndLayout) nextState,
    NRI_NAME(Mip_t) mipOffset NRI_DEFAULT_VALUE(0), NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_MIP_LEVELS)), NRI_NAME(Dim_t) arrayOffset NRI_DEFAULT_VALUE(0),
    NRI_NAME(Dim_t) arraySize NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_ARRAY_LAYERS)))
{
    NRI_NAME(TextureTransitionBarrierDesc) textureTransitionBarrierDesc = NRI_ZERO_INIT;
    textureTransitionBarrierDesc.texture = texture;
    textureTransitionBarrierDesc.prevState.acessBits = NRI_ENUM_MEMBER(AccessBits, UNKNOWN);
    textureTransitionBarrierDesc.prevState.layout = NRI_ENUM_MEMBER(TextureLayout, UNKNOWN);
    textureTransitionBarrierDesc.nextState = nextState;
    textureTransitionBarrierDesc.mipOffset = mipOffset;
    textureTransitionBarrierDesc.mipNum = mipNum;
    textureTransitionBarrierDesc.arrayOffset = arrayOffset;
    textureTransitionBarrierDesc.arraySize = arraySize;

    return textureTransitionBarrierDesc;
}

static inline NRI_NAME(TextureTransitionBarrierDesc) NRI_FUNC_NAME(TextureTransitionFromState)(NRI_NAME_REF(TextureTransitionBarrierDesc) prevState, NRI_NAME(AccessAndLayout) nextState,
    NRI_NAME(Mip_t) mipOffset NRI_DEFAULT_VALUE(0), NRI_NAME(Mip_t) mipNum NRI_DEFAULT_VALUE(NRI_NAME(REMAINING_MIP_LEVELS)))
{
    NRI_REF_ACCESS(prevState)->mipOffset = mipOffset;
    NRI_REF_ACCESS(prevState)->mipNum = mipNum;
    NRI_REF_ACCESS(prevState)->prevState = NRI_REF_ACCESS(prevState)->nextState;
    NRI_REF_ACCESS(prevState)->nextState = nextState;

    return *NRI_REF_ACCESS(prevState);
}

NRI_NAMESPACE_END
