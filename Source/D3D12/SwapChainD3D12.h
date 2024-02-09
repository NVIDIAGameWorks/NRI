// © 2021 NVIDIA Corporation

#pragma once

struct IDXGISwapChain4;
typedef IDXGISwapChain4 IDXGISwapChainBest;

namespace nri
{

struct DeviceD3D12;
struct TextureD3D12;

struct SwapChainD3D12 : public DisplayDescHelper
{
    inline SwapChainD3D12(DeviceD3D12& device) :
        m_Device(device)
        , m_Textures(device.GetStdAllocator())
    {}

    inline DeviceD3D12& GetDevice() const
    { return m_Device; }

    ~SwapChainD3D12();

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    { SET_D3D_DEBUG_OBJECT_NAME(m_SwapChain, name); }

    inline Result GetDisplayDesc(DisplayDesc& displayDesc)
    { return DisplayDescHelper::GetDisplayDesc(m_SwapChainDesc.window.windows.hwnd, displayDesc); }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    uint32_t AcquireNextTexture();
    Result Present();

private:
    DeviceD3D12& m_Device;
    ComPtr<IDXGISwapChainBest> m_SwapChain;
    Vector<TextureD3D12*> m_Textures;
    SwapChainDesc m_SwapChainDesc = {};
    HANDLE m_FrameLatencyWaitableObject = nullptr;
    uint8_t m_Version = 0;
    UINT m_Flags = 0;
};

}
