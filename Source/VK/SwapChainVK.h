// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct CommandQueueVK;
struct TextureVK;

// Let's keep things simple and hide it under the hood
constexpr uint32_t MAX_NUMBER_OF_FRAMES_IN_FLIGHT = 8;

struct SwapChainVK : public DisplayDescHelper {
    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    SwapChainVK(DeviceVK& device);
    ~SwapChainVK();

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline Result GetDisplayDesc(DisplayDesc& displayDesc) {
        return DisplayDescHelper::GetDisplayDesc(m_SwapChainDesc.window.windows.hwnd, displayDesc);
    }

    void SetDebugName(const char* name);
    Texture* const* GetTextures(uint32_t& textureNum) const;
    uint32_t AcquireNextTexture();
    Result Present();

  private:
    void Destroy();
    Result CreateSurface(const SwapChainDesc& swapChainDesc);

  private:
    Vector<TextureVK*> m_Textures;
    std::array<VkSemaphore, MAX_NUMBER_OF_FRAMES_IN_FLIGHT> m_Semaphores;
    SwapChainDesc m_SwapChainDesc = {};
    VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    DeviceVK& m_Device;
    CommandQueueVK* m_CommandQueue = nullptr;
    uint32_t m_TextureIndex = 0;
    uint32_t m_FrameIndex = 0;
};

} // namespace nri