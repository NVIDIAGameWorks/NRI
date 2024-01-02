/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

namespace nri
{

struct DeviceVK;
struct CommandQueueVK;
struct TextureVK;

struct SwapChainVK : public DisplayDescHelper
{
    inline DeviceVK& GetDevice() const
    { return m_Device; }

    SwapChainVK(DeviceVK& device);
    ~SwapChainVK();

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result GetDisplayDesc(DisplayDesc& displayDesc)
    { return DisplayDescHelper::GetDisplayDesc(m_SwapChainDesc.window.windows.hwnd, displayDesc); }

    void SetDebugName(const char* name);
    Texture* const* GetTextures(uint32_t& textureNum) const;
    uint32_t AcquireNextTexture();
    Result Present();

private:
    void Destroy();
    Result CreateSurface(const SwapChainDesc& swapChainDesc);

private:
    Vector<TextureVK*> m_Textures;
    SwapChainDesc m_SwapChainDesc = {};
    VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSemaphore m_Semaphore = VK_NULL_HANDLE;
    DeviceVK& m_Device;
    const CommandQueueVK* m_CommandQueue = nullptr;
    uint32_t m_TextureIndex = 0;
};

}