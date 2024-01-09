// © 2021 NVIDIA Corporation

#pragma region[  Core  ]

static void NRI_CALL SetCommandAllocatorDebugName(CommandAllocator& commandAllocator, const char* name) {
    ((CommandAllocatorVal&)commandAllocator).SetDebugName(name);
}

static Result NRI_CALL CreateCommandBuffer(CommandAllocator& commandAllocator, CommandBuffer*& commandBuffer) {
    return ((CommandAllocatorVal&)commandAllocator).CreateCommandBuffer(commandBuffer);
}

static void NRI_CALL ResetCommandAllocator(CommandAllocator& commandAllocator) {
    ((CommandAllocatorVal&)commandAllocator).Reset();
}

#pragma endregion

Define_Core_CommandAllocator_PartiallyFillFunctionTable(Val)
