// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct TextureVal;

struct SwapChainVal : public DeviceObjectVal<SwapChain> {
    SwapChainVal(DeviceVal& device, SwapChain* swapChain, const SwapChainDesc& swapChainDesc)
        : DeviceObjectVal(device, swapChain)
        , m_Textures(device.GetStdAllocator())
        , m_SwapChainDesc(swapChainDesc) {
    }

    ~SwapChainVal();

    //================================================================================================================
    // NRI
    //================================================================================================================
    void SetDebugName(const char* name);
    Texture* const* GetTextures(uint32_t& textureNum);
    uint32_t AcquireNextTexture();
    Result WaitForPresent();
    Result Present();
    Result GetDisplayDesc(DisplayDesc& displayDesc) const;

    Result SetLatencySleepMode(const LatencySleepMode& latencySleepMode);
    Result SetLatencyMarker(LatencyMarker latencyMarker);
    Result LatencySleep();
    Result GetLatencyReport(LatencyReport& latencyReport);

private:
    Vector<TextureVal*> m_Textures;
    SwapChainDesc m_SwapChainDesc = {};
};

} // namespace nri
