// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetCommandAllocatorDebugName(CommandAllocator& commandAllocator, const char* name) {
    ((CommandAllocatorD3D12&)commandAllocator).SetDebugName(name);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorD3D12&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorD3D12&)commandAllocator).Reset();
}

#pragma endregion

Define_Core_CommandAllocator_PartiallyFillFunctionTable(D3D12);
