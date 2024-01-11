// © 2021 NVIDIA Corporation

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
    CommandQueueVK* m_CommandQueue = nullptr;
    uint32_t m_TextureIndex = 0;
};

}