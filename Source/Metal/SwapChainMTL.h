// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

// Let's keep things simple and hide it under the hood
constexpr uint32_t MAX_NUMBER_OF_FRAMES_IN_FLIGHT = 8;

struct SwapChainMTL: public DisplayDescHelper {
    SwapChainVK(DeviceMTL& device);
    ~SwapChainVK();

    Result Create(const SwapChainDesc& swapChainDesc);

private:
    DeviceMTL& m_Device;
    uint64_t m_PresentId = 0;
    uint32_t m_TextureIndex = 0;
    uint8_t m_FrameIndex = 0; // in flight, not global
};

};
