#pragma once

namespace nri {

// Let's keep things simple and hide it under the hood
constexpr uint32_t MAX_NUMBER_OF_FRAMES_IN_FLIGHT = 8;

struct SwapChainMTL: public DisplayDescHelper {
    SwapChainMTL(DeviceMTL& device)
    : m_Device(device) {
        
    }
    ~SwapChainMTL();

    Result Create(const SwapChainDesc& swapChainDesc);

private:
    CAMetalLayer*  m_MetalLayer;

    DeviceMTL& m_Device;
    uint64_t m_PresentId = 0;
    uint32_t m_TextureIndex = 0;
    uint8_t m_FrameIndex = 0; // in flight, not global
};

};