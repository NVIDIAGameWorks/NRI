// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceD3D12&)fence).GetFenceValue();
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceD3D12&)fence).Wait(value);
}

static void NRI_CALL SetFenceDebugName(Fence& fence, const char* name) {
    ((FenceD3D12&)fence).SetDebugName(name);
}

#pragma endregion

Define_Core_Fence_PartiallyFillFunctionTable(D3D12);
