// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetDescriptorDebugName(Descriptor& descriptor, const char* name) {
    ((DescriptorD3D11&)descriptor).SetDebugName(name);
}

static uint64_t NRI_CALL GetDescriptorNativeObject(const Descriptor& descriptor, uint32_t nodeIndex) {
    MaybeUnused(nodeIndex);

    if (!(&descriptor))
        return 0;

    return uint64_t((ID3D11View*)((DescriptorD3D11&)descriptor));
}

#pragma endregion

Define_Core_Descriptor_PartiallyFillFunctionTable(D3D11)