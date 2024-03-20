// © 2021 NVIDIA Corporation

#include "SharedExternal.h"
#include "SharedVal.h"

#include "SwapChainVal.h"
#include "TextureVal.h"

using namespace nri;

SwapChainVal::~SwapChainVal() {
    for (size_t i = 0; i < m_Textures.size(); i++)
        Deallocate(m_Device.GetStdAllocator(), m_Textures[i]);
}

inline void SwapChainVal::SetDebugName(const char* name) {
    m_Name = name;
    GetSwapChainInterface().SetSwapChainDebugName(*GetImpl(), name);
}

inline Texture* const* SwapChainVal::GetTextures(uint32_t& textureNum) {
    Texture* const* textures = GetSwapChainInterface().GetSwapChainTextures(*GetImpl(), textureNum);

    if (m_Textures.empty()) {
        for (uint32_t i = 0; i < textureNum; i++) {
            TextureVal* textureVal = Allocate<TextureVal>(m_Device.GetStdAllocator(), m_Device, textures[i]);
            m_Textures.push_back(textureVal);
        }
    }

    return (Texture* const*)m_Textures.data();
}

inline uint32_t SwapChainVal::AcquireNextTexture() {
    return GetSwapChainInterface().AcquireNextSwapChainTexture(*GetImpl());
}

inline Result SwapChainVal::WaitForPresent() {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.waitable, Result::FAILURE, "Swap chain has not been created with 'waitable = true'");

    return GetSwapChainInterface().WaitForPresent(*GetImpl());
}

inline Result SwapChainVal::Present() {
    return GetSwapChainInterface().QueuePresent(*GetImpl());
}

inline Result SwapChainVal::GetDisplayDesc(DisplayDesc& displayDesc) const {
    return GetSwapChainInterface().GetDisplayDesc(*GetImpl(), displayDesc);
}

inline Result SwapChainVal::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().SetLatencySleepMode(*GetImpl(), latencySleepMode);
}

inline Result SwapChainVal::SetLatencyMarker(LatencyMarker latencyMarker) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().SetLatencyMarker(*GetImpl(), latencyMarker);
}

inline Result SwapChainVal::LatencySleep() {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().LatencySleep(*GetImpl());
}

inline Result SwapChainVal::GetLatencyReport(LatencyReport& latencyReport) {
    RETURN_ON_FAILURE(&m_Device, m_SwapChainDesc.allowLowLatency, Result::FAILURE, "Swap chain has not been created with 'allowLowLatency = true'");

    return GetLowLatencyInterface().GetLatencyReport(*GetImpl(), latencyReport);
}

#include "SwapChainVal.hpp"
