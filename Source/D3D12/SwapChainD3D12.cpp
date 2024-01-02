/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "SharedD3D12.h"
#include "SwapChainD3D12.h"
#include "TextureD3D12.h"
#include "CommandQueueD3D12.h"

using namespace nri;

static std::array<DXGI_FORMAT, (size_t)SwapChainFormat::MAX_NUM> g_swapChainFormat =
{
    DXGI_FORMAT_R16G16B16A16_FLOAT,                 // BT709_G10_16BIT
    DXGI_FORMAT_R8G8B8A8_UNORM,                     // BT709_G22_8BIT
    DXGI_FORMAT_R10G10B10A2_UNORM,                  // BT709_G22_10BIT
    DXGI_FORMAT_R10G10B10A2_UNORM,                  // BT2020_G2084_10BIT
};

static std::array<DXGI_COLOR_SPACE_TYPE, (size_t)SwapChainFormat::MAX_NUM> g_colorSpace =
{
    DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,        // BT709_G10_16BIT
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,        // BT709_G22_8BIT
    DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,        // BT709_G22_10BIT
    DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020,     // BT2020_G2084_10BIT
};

SwapChainD3D12::~SwapChainD3D12()
{
    if (m_FrameLatencyWaitableObject)
        CloseHandle(m_FrameLatencyWaitableObject);

    for (TextureD3D12* texture : m_Textures)
        Deallocate<TextureD3D12>(m_Device.GetStdAllocator(), texture);
}

Result SwapChainD3D12::Create(const SwapChainDesc& swapChainDesc)
{
    HWND hwnd = (HWND)swapChainDesc.window.windows.hwnd;
    if (!hwnd)
        return Result::INVALID_ARGUMENT;

    ID3D12Device* device = m_Device;
    CommandQueueD3D12& commandQueue = *(CommandQueueD3D12*)swapChainDesc.commandQueue;

    // Query DXGIFactory2
    HRESULT hr = m_Device.GetAdapter()->GetParent(IID_PPV_ARGS(&m_DxgiFactory2));
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIAdapter::GetParent()");

    // Is tearing supported?
    bool isTearingAllowed = false;
    ComPtr<IDXGIFactory5> dxgiFactory5;
    hr = m_DxgiFactory2->QueryInterface(IID_PPV_ARGS(&dxgiFactory5));
    if (SUCCEEDED(hr))
    {
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

    ComPtr<IDXGISwapChain1> swapChain;
    hr = m_DxgiFactory2->CreateSwapChainForHwnd((ID3D12CommandQueue*)commandQueue, hwnd, &desc, nullptr, nullptr, &swapChain);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIFactory2::CreateSwapChainForHwnd()");

    hr = m_DxgiFactory2->MakeWindowAssociation(hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGIFactory::MakeWindowAssociation()");

    // Query interfaces
    hr = swapChain->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&m_SwapChain.ptr);
    m_SwapChain.version = 4;
    if (FAILED(hr))
    {
        REPORT_WARNING(&m_Device, "QueryInterface(IDXGISwapChain4) - FAILED!");
        hr = device->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&m_SwapChain.ptr);
        m_SwapChain.version = 3;
        if (FAILED(hr))
        {
            REPORT_WARNING(&m_Device, "QueryInterface(IDXGISwapChain3) - FAILED!");
            hr = device->QueryInterface(__uuidof(IDXGISwapChain2), (void**)&m_SwapChain.ptr);
            m_SwapChain.version = 2;
            if (FAILED(hr))
            {
                REPORT_WARNING(&m_Device, "QueryInterface(IDXGISwapChain2) - FAILED!");
                hr = device->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&m_SwapChain.ptr);
                m_SwapChain.version = 1;
                if (FAILED(hr))
                {
                    REPORT_WARNING(&m_Device, "QueryInterface(IDXGISwapChain1) - FAILED!");
                    m_SwapChain.ptr = (IDXGISwapChain4*)swapChain.GetInterface();
                    m_SwapChain.version = 0;
                }
            }
        }
    }

    // Color space
    if (m_SwapChain.version >= 3)
    {
        uint32_t colorSpaceSupport = 0;
        hr = m_SwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);

        if ( !(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) )
            hr = E_FAIL;

        if (SUCCEEDED(hr))
            hr = m_SwapChain->SetColorSpace1(colorSpace);

        if (FAILED(hr))
           REPORT_WARNING(&m_Device, "IDXGISwapChain3::SetColorSpace1() - FAILED!");
    }
    else
        REPORT_ERROR(&m_Device, "IDXGISwapChain3::SetColorSpace1() is not supported by the OS!");

    // Background color
    if (m_SwapChain.version >= 1)
    {
        DXGI_RGBA color = {0.0f, 0.0f, 0.0f, 1.0f};
        hr = m_SwapChain->SetBackgroundColor(&color);
        if (FAILED(hr))
            REPORT_WARNING(&m_Device, "IDXGISwapChain1::SetBackgroundColor() - FAILED!");
    }

    // Maximum frame latency
    if (m_SwapChain.version >= 2)
    {
        // IMPORTANT: SetMaximumFrameLatency must be called BEFORE GetFrameLatencyWaitableObject!
        hr = m_SwapChain->SetMaximumFrameLatency(swapChainDesc.textureNum);
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain2::SetMaximumFrameLatency()");

        m_FrameLatencyWaitableObject = m_SwapChain->GetFrameLatencyWaitableObject();
    }

    // Finalize
    m_Flags = desc.Flags;
    m_SwapChainDesc = swapChainDesc;

    m_Textures.reserve(m_SwapChainDesc.textureNum);
    for (uint32_t i = 0; i < m_SwapChainDesc.textureNum; i++)
    {
        ComPtr<ID3D12Resource> textureNative;
        hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&textureNative));
        RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain::GetBuffer()");

        TextureD3D12Desc textureDesc = {};
        textureDesc.d3d12Resource = textureNative;

        TextureD3D12* texture = Allocate<TextureD3D12>(m_Device.GetStdAllocator(), m_Device);
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

inline Texture* const* SwapChainD3D12::GetTextures(uint32_t& textureNum) const
{
    textureNum = m_SwapChainDesc.textureNum;

    return (Texture**)m_Textures.data();
}

inline uint32_t SwapChainD3D12::AcquireNextTexture()
{
    // https://docs.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains#step-4-wait-before-rendering-each-frame
    if (m_FrameLatencyWaitableObject)
    {
        uint32_t result = WaitForSingleObjectEx(m_FrameLatencyWaitableObject, DEFAULT_TIMEOUT, TRUE);
        if (result != WAIT_OBJECT_0)
            REPORT_ERROR(&m_Device, "WaitForSingleObjectEx(): failed, result = 0x%08X!", result);
    }

    return m_SwapChain->GetCurrentBackBufferIndex();
}

inline Result SwapChainD3D12::Present()
{
    uint32_t flags = (!m_SwapChainDesc.verticalSyncInterval && (m_Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)) ? DXGI_PRESENT_ALLOW_TEARING : 0; // TODO: and not fullscreen
    HRESULT hr = m_SwapChain->Present(m_SwapChainDesc.verticalSyncInterval, flags);
    RETURN_ON_BAD_HRESULT(&m_Device, hr, "IDXGISwapChain::Present()");

    return Result::SUCCESS;
}

#include "SwapChainD3D12.hpp"
