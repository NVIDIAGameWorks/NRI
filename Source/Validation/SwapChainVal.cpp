/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedExternal.h"
#include "DeviceBase.h"
#include "DeviceVal.h"
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
    m_SwapChainAPI.SetSwapChainDebugName(*GetImpl(), name);
}

inline Texture* const* SwapChainVal::GetTextures(uint32_t& textureNum) const {
    Texture* const* textures = m_SwapChainAPI.GetSwapChainTextures(*GetImpl(), textureNum);

    m_Textures.resize(textureNum);
    for (uint32_t i = 0; i < textureNum; i++)
        m_Textures[i] = Allocate<TextureVal>(m_Device.GetStdAllocator(), m_Device, textures[i]);

    return (Texture* const*)m_Textures.data();
}

inline uint32_t SwapChainVal::AcquireNextTexture() {
    return m_SwapChainAPI.AcquireNextSwapChainTexture(*GetImpl());
}

inline Result SwapChainVal::Present() {
    return m_SwapChainAPI.SwapChainPresent(*GetImpl());
}

inline Result SwapChainVal::GetDisplayDesc(DisplayDesc& displayDesc) const {
    return m_SwapChainAPI.GetDisplayDesc(*GetImpl(), displayDesc);
}

#include "SwapChainVal.hpp"
