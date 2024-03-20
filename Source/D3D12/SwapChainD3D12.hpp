// © 2021 NVIDIA Corporation

#pragma region[  SwapChain  ]

static void NRI_CALL SetSwapChainDebugName(SwapChain& swapChain, const char* name) {
    ((SwapChainD3D12&)swapChain).SetDebugName(name);
}

static Texture* const* NRI_CALL GetSwapChainTextures(const SwapChain& swapChain, uint32_t& textureNum) {
    return ((SwapChainD3D12&)swapChain).GetTextures(textureNum);
}

static uint32_t NRI_CALL AcquireNextSwapChainTexture(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).AcquireNextTexture();
}

static Result NRI_CALL WaitForPresent(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).WaitForPresent();
}

static Result NRI_CALL QueuePresent(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).Present();
}

static Result NRI_CALL GetDisplayDesc(SwapChain& swapChain, DisplayDesc& displayDesc) {
    return ((SwapChainD3D12&)swapChain).GetDisplayDesc(displayDesc);
}

#pragma endregion

#pragma region[  Low latency  ]

static Result SetLatencySleepMode(SwapChain& swapChain, const LatencySleepMode& latencySleepMode) {
    return ((SwapChainD3D12&)swapChain).SetLatencySleepMode(latencySleepMode);
}

static Result SetLatencyMarker(SwapChain& swapChain, LatencyMarker latencyMarker) {
    return ((SwapChainD3D12&)swapChain).SetLatencyMarker(latencyMarker);
}

static Result LatencySleep(SwapChain& swapChain) {
    return ((SwapChainD3D12&)swapChain).LatencySleep();
}

static Result GetLatencyReport(const SwapChain& swapChain, LatencyReport& latencyReport) {
    return ((SwapChainD3D12&)swapChain).GetLatencyReport(latencyReport);
}

#pragma endregion

Define_SwapChain_PartiallyFillFunctionTable(D3D12);
Define_LowLatency_SwapChain_PartiallyFillFunctionTable(D3D12);
