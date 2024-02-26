// © 2021 NVIDIA Corporation

#include "SharedD3D11.h"

#include "SwapChainD3D11.h"
#include "TextureD3D11.h"

using namespace nri;

static std::array<DXGI_FORMAT, (size_t)SwapChainFormat::MAX_NUM> g_swapChainFormat = {
    DXGI_FORMAT_R16G16B16A16_FLOAT, // BT709_G10_16BIT
    DXGI_FORMAT_R8G8B8A8_UNORM,     // BT709_G22_8BIT
    DXGI_FORMAT_R10G10B10A2_UNORM,  // BT709_G22_10BIT
    DXGI_FORMAT_R10G10B10A2_UNORM,  // BT2020_G2084_10BIT
};

static std::array<DXGI_COLOR_SPACE_TYPE, (size_t)SwapChainFormat::MAX_NUM> g_colorSpace = {
    DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,    // BT709_G10_16BIT
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,    // BT709_G22_8BIT
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,    // BT709_G22_10BIT
    DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, // BT2020_G2084_10BIT
};

static uint8_t QueryLatestSwapChain(ComPtr<IDXGISwapChainBest>& in, ComPtr<IDXGISwapChainBest>& out) {
    static const IID versions[] = {
        __uuidof(IDXGISwapChain4),
        __uuidof(IDXGISwapChain3),
        __uuidof(IDXGISwapChain2),
        __uuidof(IDXGISwapChain1),
        __uuidof(IDXGISwapChain),
    };
    const uint8_t n = (uint8_t)GetCountOf(versions);

    uint8_t i = 0;
    for (; i < n; i++) {
        HRESULT hr = in->QueryInterface(versions[i], (void**)&out);
        if (SUCCEEDED(hr))
            break;
    }

    return n - i - 1;
}

SwapChainD3D11::~SwapChainD3D11() {
    if (m_FrameLatencyWaitableObject)
        CloseHandle(m_FrameLatencyWaitableObject);

    for (TextureD3D11* texture : m_Textures)
        Deallocate<TextureD3D11>(m_Device.GetStdAllocator(), texture);
}

Result SwapChainD3D11::Create(const SwapChainDesc& swapChainDesc) {
    HWND hwnd = (HWND)swapChainDesc.window.windows.hwnd;
    if (!hwnd)
        return Result::INVALID_ARGUMENT;

    // Query DXGIFactory2
    HRESULT hr = m_Device.GetAdapter()->GetParent(IID_PPV_ARGS(&m_DxgiFactory2));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIAdapter::GetParent()");

    // Is tearing supported?
    bool isTearingAllowed = false;
    ComPtr<IDXGIFactory5> dxgiFactory5;
    hr = m_DxgiFactory2->QueryInterface(IID_PPV_ARGS(&dxgiFactory5));
    if (SUCCEEDED(hr)) {
        uint32_t tearingSupport = 0;
        hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupport, sizeof(tearingSupport));
        isTearingAllowed = (SUCCEEDED(hr) && tearingSupport) ? true : false;
    }

    // Create swapchain
    DXGI_FORMAT format = g_swapChainFormat[(uint32_t)swapChainDesc.format];
    DXGI_COLOR_SPACE_TYPE colorSpace = g_colorSpace[(uint32_t)swapChainDesc.format];

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = swapChainDesc.width;
    desc.Height = swapChainDesc.height;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = swapChainDesc.textureNum;
    desc.Scaling = DXGI_SCALING_NONE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    if (isTearingAllowed)
        desc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    ComPtr<IDXGISwapChainBest> swapChain;
    hr = m_DxgiFactory2->CreateSwapChainForHwnd(m_Device.GetNativeObject(), hwnd, &desc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIFactory2::CreateSwapChainForHwnd()");

    m_Version = QueryLatestSwapChain(swapChain, m_SwapChain);

    hr = m_DxgiFactory2->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIFactory::MakeWindowAssociation()");

    // Color space
    if (m_Version >= 3) {
        uint32_t colorSpaceSupport = 0;
        hr = m_SwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);

        if (!(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
            hr = E_FAIL;

        if (SUCCEEDED(hr))
            hr = m_SwapChain->SetColorSpace1(colorSpace);

        if (FAILED(hr))
            REPORT_WARNING(&m_Device, "IDXGISwapChain3::SetColorSpace1() - FAILED!");
    } else
        REPORT_ERROR(&m_Device, "IDXGISwapChain3::SetColorSpace1() is not supported by the OS!");

    // Background color
    if (m_Version >= 1) {
        DXGI_RGBA color = {0.0f, 0.0f, 0.0f, 1.0f};
        hr = m_SwapChain->SetBackgroundColor(&color);
        if (FAILED(hr))
            REPORT_WARNING(&m_Device, "IDXGISwapChain1::SetBackgroundColor() - FAILED!");
    }

    // Maximum frame latency
    if (m_Version >= 2) {
        // IMPORTANT: SetMaximumFrameLatency must be called BEFORE GetFrameLatencyWaitableObject!
        hr = m_SwapChain->SetMaximumFrameLatency(swapChainDesc.textureNum);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain2::SetMaximumFrameLatency()");

        m_FrameLatencyWaitableObject = m_SwapChain->GetFrameLatencyWaitableObject();
    }

    // Finalize
    m_Flags = desc.Flags;
    m_SwapChainDesc = swapChainDesc;
    m_SwapChainDesc.textureNum = 1; // IMPORTANT: only 1 texture is available in D3D11

    m_Textures.reserve(m_SwapChainDesc.textureNum);
    for (uint32_t i = 0; i < m_SwapChainDesc.textureNum; i++) {
        ComPtr<ID3D11Resource> textureNative;
        hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&textureNative));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain::GetBuffer()");

        TextureD3D11Desc textureDesc = {};
        textureDesc.d3d11Resource = textureNative;

        TextureD3D11* texture = Allocate<TextureD3D11>(m_Device.GetStdAllocator(), m_Device);
        const Result res = texture->Create(textureDesc);
        if (res != Result::SUCCESS)
            return res;

        m_Textures.push_back(texture);
    }

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline Texture* const* SwapChainD3D11::GetTextures(uint32_t& textureNum) const {
    textureNum = m_SwapChainDesc.textureNum;

    return (Texture**)m_Textures.data();
}

inline uint32_t SwapChainD3D11::AcquireNextTexture() {
    // https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains#step-4-wait-before-rendering-each-frame
    if (m_FrameLatencyWaitableObject) {
        uint32_t result = WaitForSingleObjectEx(m_FrameLatencyWaitableObject, DEFAULT_TIMEOUT, TRUE);
        if (result != WAIT_OBJECT_0)
            REPORT_ERROR(&m_Device, "WaitForSingleObjectEx(): failed, result = 0x%08X!", result);
    }

    return 0; // IMPORTANT: only 1 texture is available in D3D11
}

inline Result SwapChainD3D11::Present() {
    uint32_t flags = (!m_SwapChainDesc.verticalSyncInterval && (m_Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)) ? DXGI_PRESENT_ALLOW_TEARING : 0; // TODO: and not fullscreen
    HRESULT hr = m_SwapChain->Present(m_SwapChainDesc.verticalSyncInterval, flags);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain::Present()");

    return Result::SUCCESS;
}

#include "SwapChainD3D11.hpp"
