// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetTextureDebugName(Texture& texture, const char* name) {
    ((TextureVal&)texture).SetDebugName(name);
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture) {
    if (!(&texture))
        return 0;

    return ((TextureVal&)texture).GetNativeObject();
}

static void NRI_CALL GetTextureMemoryInfo(const Texture& texture, MemoryLocation memoryLocation, MemoryDesc& memoryDesc) {
    ((TextureVal&)texture).GetMemoryInfo(memoryLocation, memoryDesc);
}

#pragma endregion

Define_Core_Texture_PartiallyFillFunctionTable(Val)
