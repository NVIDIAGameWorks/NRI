// © 2021 NVIDIA Corporation

#pragma region[  SwapChain  ]

static void NRI_CALL SetSwapChainDebugName(SwapChain& swapChain, const char* name) {
    ((SwapChainVal&)swapChain).SetDebugName(name);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainVal&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL SwapChainPresent(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainVal&)swapChain).GetDisplayDesc(displayDesc);
}

#pragma endregion

Define_SwapChain_PartiallyFillFunctionTable(Val)
