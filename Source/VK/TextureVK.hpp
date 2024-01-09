// © 2021 NVIDIA Corporation

#pragma region [  Core  ]

static void NRI_CALL SetTextureDebugName(Texture& texture, const char* name)
{
    ((TextureVK&)texture).SetDebugName(name);
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture, uint32_t nodeIndex)
{
    if (!(&texture))
        return 0;

    return uint64_t( ((TextureVK&)texture).GetHandle(nodeIndex) );
}

static void NRI_CALL GetTextureMemoryInfo(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc)
{
    ((TextureVK&)texture).GetMemoryInfo(memoryLocation, memoryDesc);
}

#pragma endregion

Define_Core_Texture_PartiallyFillFunctionTable(VK)
