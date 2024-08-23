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

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainVal&)swapChain).GetDisplayDesc(displayDesc);
}

#pragma endregion

#pragma region[  Low latency  ]

static Result SetLatencySleepMode(SwapChain& swapChain, const LatencySleepMode& latencySleepMode) {
    return ((SwapChainVal&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainVal&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result LatencySleep(SwapChain& swapChain) {
    return ((SwapChainVal&)swapChain).LatencySleep();
}

static Result GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainVal&)swapChain).GetLatencyReport(latencyReport);
}

#pragma endregion

Define_SwapChain_SwapChain_PartiallyFillFunctionTable(Val);
Define_LowLatency_SwapChain_SwapChain_PartiallyFillFunctionTable(Val);
