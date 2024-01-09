// © 2021 NVIDIA Corporation

#pragma once

NRI_NAMESPACE_BEGIN

NRI_FORWARD_STRUCT(SwapChain);

// Color space:
//  - BT.709 - LDR https://en.wikipedia.org/wiki/Rec._709
//  - BT.2020 - HDR https://en.wikipedia.org/wiki/Rec._2020
// Transfer function:
//  - G10 - linear (gamma 1.0)
//  - G22 - sRGB (gamma ~2.2)
//  - G2084 - SMPTE ST.2084 (Perceptual Quantization)
// Bits per channel:
//  - 8, 10, 16 (float)
NRI_ENUM
(
    SwapChainFormat, uint8_t,

    BT709_G10_16BIT,
    BT709_G22_8BIT,
    BT709_G22_10BIT,
    BT2020_G2084_10BIT,

    MAX_NUM
);

NRI_STRUCT(WindowsWindow)
{
    void* hwnd; // HWND
};

NRI_STRUCT(X11Window)
{
    void* dpy; // Display*
    uint64_t window; // Window
};

NRI_STRUCT(WaylandWindow)
{
    void* display; // wl_display*
    void* surface; // wl_surface*
};

NRI_STRUCT(MetalWindow)
{
    void* caMetalLayer; // CAMetalLayer*
};

NRI_UNION(Window)
{
    // Only one entity must be initialized
    NRI_NAME(WindowsWindow) windows;
    NRI_NAME(MetalWindow) metal;
    NRI_NAME(X11Window) x11;
    NRI_NAME(WaylandWindow) wayland;
};

// SwapChain buffers will be created as "color attachment" resources
NRI_STRUCT(SwapChainDesc)
{
    NRI_NAME(Window) window;
    const NRI_NAME(CommandQueue)* commandQueue;
    NRI_NAME(Dim_t) width;
    NRI_NAME(Dim_t) height;
    uint8_t textureNum;
    NRI_NAME(SwapChainFormat) format;
    uint8_t verticalSyncInterval;
};

NRI_STRUCT(ChromaticityCoords)
{
    float x, y; // [0; 1]
};

// Describes color settings and capabilities of the closest display
//  nit = cd/m2
//  SDR = standard dynamic range
//  LDR = low dynamic range (in many cases LDR == SDR)
//  HDR = high dynamic range, assumes G2084:
//      - BT709_G10_16BIT: HDR gets enabled and applied implicitly if Windows HDR is enabled
//      - BT2020_G2084_10BIT: HDR requires explicit color conversions and enabled HDR in Windows
//  "SDR scale in HDR mode" = sdrLuminance / 80
NRI_STRUCT(DisplayDesc)
{
    NRI_NAME(ChromaticityCoords) redPrimary;
    NRI_NAME(ChromaticityCoords) greenPrimary;
    NRI_NAME(ChromaticityCoords) bluePrimary;
    NRI_NAME(ChromaticityCoords) whitePoint;
    float minLuminance; // nits
    float maxLuminance; // nits
    float maxFullFrameLuminance; // nits
    float sdrLuminance; // nits
    bool isHDR;
};

NRI_STRUCT(SwapChainInterface)
{
    NRI_NAME(Result) (NRI_CALL *CreateSwapChain)(NRI_NAME_REF(Device) device, const NRI_NAME_REF(SwapChainDesc) swapChainDesc, NRI_NAME_REF(SwapChain*) swapChain);
    void (NRI_CALL *DestroySwapChain)(NRI_NAME_REF(SwapChain) swapChain);
    void (NRI_CALL *SetSwapChainDebugName)(NRI_NAME_REF(SwapChain) swapChain, const char* name);
    NRI_NAME(Texture)* const* (NRI_CALL *GetSwapChainTextures)(const NRI_NAME_REF(SwapChain) swapChain, uint32_t NRI_REF textureNum);
    uint32_t (NRI_CALL *AcquireNextSwapChainTexture)(NRI_NAME_REF(SwapChain) swapChain); // IMPORTANT: return OUT_OF_DATE index to indicate "out of date" swap chain status (VK only)
    NRI_NAME(Result) (NRI_CALL *SwapChainPresent)(NRI_NAME_REF(SwapChain) swapChain);
    NRI_NAME(Result) (NRI_CALL *GetDisplayDesc)(NRI_NAME_REF(SwapChain) swapChain, NRI_NAME_REF(DisplayDesc) displayDesc);
};

NRI_NAMESPACE_END
