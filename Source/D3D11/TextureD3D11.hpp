// © 2021 NVIDIA Corporation

#pragma region [  Core  ]

static void NRI_CALL SetTextureDebugName(Texture& texture, const char* name)
{
    ((TextureD3D11&)texture).SetDebugName(name);
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture, uint32_t nodeIndex)
{
    MaybeUnused(nodeIndex);

    if (!(&texture))
        return 0;

    return uint64_t((ID3D11Resource*)((TextureD3D11&)texture));
}

static void NRI_CALL GetTextureMemoryInfo(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc)
{
    ((TextureD3D11&)texture).GetMemoryInfo(memoryLocation, memoryDesc);
}

#pragma endregion

Define_Core_Texture_PartiallyFillFunctionTable(D3D11)
