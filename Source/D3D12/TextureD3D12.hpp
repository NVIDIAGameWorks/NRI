// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetTextureDebugName(Texture& texture, const char* name) {
    ((TextureD3D12&)texture).SetDebugName(name);
}

static uint64_t NRI_CALL GetTextureNativeObject(const Texture& texture) {
    if (!(&texture))
        return 0;

    return uint64_t((ID3D12Resource*)((TextureD3D12&)texture));
}

#pragma endregion

Define_Core_Texture_PartiallyFillFunctionTable(D3D12);
