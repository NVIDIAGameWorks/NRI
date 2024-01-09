// © 2021 NVIDIA Corporation

#pragma once

struct IDXGISwapChain4;

namespace nri
{

struct DeviceD3D11;
struct TextureD3D11;

struct VersionedSwapChain
{
    inline ~VersionedSwapChain()
    {}

    inline IDXGISwapChain4* operator->() const
    { return ptr; }

    ComPtr<IDXGISwapChain4> ptr;
    uint8_t version = 0;
};

struct SwapChainD3D11 : public DisplayDescHelper
{
    inline SwapChainD3D11(DeviceD3D11& device) :
        m_Device(device)
        , m_Textures(device.GetStdAllocator())
    {}

    inline DeviceD3D11& GetDevice() const
    { return m_Device; }

    ~SwapChainD3D11();

    Result Create(const SwapChainDesc& swapChainDesc);

    //================================================================================================================
    // NRI
    //================================================================================================================

    inline void SetDebugName(const char* name)
    { SET_D3D_DEBUG_OBJECT_NAME(m_SwapChain.ptr, name); }

    inline Result GetDisplayDesc(DisplayDesc& displayDesc)
    { return DisplayDescHelper::GetDisplayDesc(m_SwapChainDesc.window.windows.hwnd, displayDesc); }

    Texture* const* GetTextures(uint32_t& textureNum) const;
    uint32_t AcquireNextTexture();
    Result Present();

private:
    DeviceD3D11& m_Device;
    VersionedSwapChain m_SwapChain;
    Vector<TextureD3D11*> m_Textures;
    SwapChainDesc m_SwapChainDesc = {};
    HANDLE m_FrameLatencyWaitableObject = nullptr;
    UINT m_Flags = 0;
};

}
