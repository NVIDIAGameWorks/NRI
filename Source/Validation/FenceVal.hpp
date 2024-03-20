// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static uint64_t NRI_CALL GetFenceValue(Fence& fence) {
    return ((FenceVal&)fence).GetFenceValue();
}

static void NRI_CALL Wait(Fence& fence, uint64_t value) {
    ((FenceVal&)fence).Wait(value);
}

static void NRI_CALL SetFenceDebugName(Fence& fence, const char* name) {
    ((FenceVal&)fence).SetDebugName(name);
}

#pragma endregion

Define_Core_Fence_PartiallyFillFunctionTable(Val);
