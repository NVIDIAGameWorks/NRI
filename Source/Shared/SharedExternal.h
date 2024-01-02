/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once

#include "NRI.h"
#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRISwapChain.h"
#include "Extensions/NRIWrapperD3D11.h"
#include "Extensions/NRIWrapperD3D12.h"
#include "Extensions/NRIWrapperVK.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIHelper.h"

#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <map>

#include "Lock.h"

typedef nri::MemoryAllocatorInterface MemoryAllocatorInterface;
#include "StdAllocator.h"

#include "HelperWaitIdle.h"
#include "HelperDeviceMemoryAllocator.h"
#include "HelperDataUpload.h"
#include "HelperResourceStateChange.h"

#ifdef _WIN32
    #include <dxgi1_6.h>
#else
    typedef uint32_t DXGI_FORMAT;
#endif

constexpr uint32_t PHYSICAL_DEVICE_GROUP_MAX_SIZE = 4;
constexpr uint32_t COMMAND_QUEUE_TYPE_NUM = (uint32_t)nri::CommandQueueType::MAX_NUM;
constexpr uint32_t DEFAULT_TIMEOUT = 5000; // 5 sec
constexpr uint64_t VK_DEFAULT_TIMEOUT = DEFAULT_TIMEOUT * 1000000ull;

constexpr void ReturnVoid()
{}

template<typename... Args> constexpr void MaybeUnused([[maybe_unused]] const Args&... args)
{}

constexpr uint32_t GetNodeMask(uint32_t mask)
{ return mask == nri::ALL_NODES ? 0xff : mask; }

#define RETURN_ON_BAD_HRESULT(deviceBase, hr, msg) \
    if ( FAILED(hr) ) \
    { \
        (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, msg##" failed, result = 0x%08X!", hr); \
        return GetResultFromHRESULT(hr); \
    }

#define RETURN_ON_FAILURE(deviceBase, condition, returnCode, format, ...) \
    if ( !(condition) ) \
    { \
        (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__); \
        return returnCode; \
    }

#define REPORT_INFO(deviceBase, format, ...) \
    (deviceBase)->ReportMessage(nri::Message::TYPE_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define REPORT_WARNING(deviceBase, format, ...) \
    (deviceBase)->ReportMessage(nri::Message::TYPE_WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define REPORT_ERROR(deviceBase, format, ...) \
    (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)

#if _DEBUG
    #define CHECK(deviceBase, condition, format, ...) \
        if ( !(condition) ) \
            (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
    #define CHECK(deviceBase, condition, format, ...) \
        ((void)sizeof((void)(condition), 0))
#endif

#define SET_D3D_DEBUG_OBJECT_NAME(obj, name) \
    if (obj) \
        obj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)std::strlen(name), name)

inline nri::Vendor GetVendorFromID(uint32_t vendorID)
{
    switch (vendorID)
    {
    case 0x10DE: return nri::Vendor::NVIDIA;
    case 0x1002: return nri::Vendor::AMD;
    case 0x8086: return nri::Vendor::INTEL;
    }

    return nri::Vendor::UNKNOWN;
}

struct DxgiFormat
{
    DXGI_FORMAT typeless;
    DXGI_FORMAT typed;
};

struct FormatProps
{
    uint16_t stride;
    uint8_t blockWidth;
    bool isInteger;
};

const DxgiFormat& GetDxgiFormat(nri::Format format);
const FormatProps& GetFormatProps(nri::Format format);

void CheckAndSetDefaultCallbacks(nri::CallbackInterface& callbackInterface);
void ConvertCharToWchar(const char* in, wchar_t* out, size_t outLen);
void ConvertWcharToChar(const wchar_t* in, char* out, size_t outLen);
nri::Result GetResultFromHRESULT(long result);

nri::Format DXGIFormatToNRIFormat(uint32_t dxgiFormat);
nri::Format VKFormatToNRIFormat(uint32_t vkFormat);

uint32_t NRIFormatToDXGIFormat(nri::Format format);
uint32_t NRIFormatToVKFormat(nri::Format format);

struct Library;
Library* LoadSharedLibrary(const char* path);
void* GetSharedLibraryFunction(Library& library, const char* name);
void UnloadSharedLibrary(Library& library);
extern const char* VULKAN_LOADER_NAME;

//================================================================================================================
// TODO: This code is Windows/D3D specific, so it's probably better to move it into a separate header
#ifdef _WIN32

struct IUnknown;

// This struct acts as a smart pointer for IUnknown pointers making sure to call AddRef() and Release() as needed.
template<typename T>
struct ComPtr
{
    inline ComPtr(T* lComPtr = nullptr) : m_ComPtr(lComPtr)
    {
        static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");

        if (m_ComPtr)
            m_ComPtr->AddRef();
    }

    inline ComPtr(const ComPtr<T>& lComPtrObj)
    {
        static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");

        m_ComPtr = lComPtrObj.m_ComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();
    }

    inline ComPtr(ComPtr<T>&& lComPtrObj)
    {
        m_ComPtr = lComPtrObj.m_ComPtr;
        lComPtrObj.m_ComPtr = nullptr;
    }

    inline T* operator=(T* lComPtr)
    {
        if (m_ComPtr)
            m_ComPtr->Release();

        m_ComPtr = lComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();

        return m_ComPtr;
    }

    inline T* operator=(const ComPtr<T>& lComPtrObj)
    {
        if (m_ComPtr)
            m_ComPtr->Release();

        m_ComPtr = lComPtrObj.m_ComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();

        return m_ComPtr;
    }

    inline ~ComPtr()
    {
        if (m_ComPtr)
        {
            m_ComPtr->Release();
            m_ComPtr = nullptr;
        }
    }

    inline T** operator&()
    {
        // The assert on operator& usually indicates a bug. Could be a potential memory leak.
        // If this really what is needed, however, use GetInterface() explicitly.
        assert(m_ComPtr == nullptr);
        return &m_ComPtr;
    }

    inline operator T*() const
    { return m_ComPtr; }

    inline T* GetInterface() const
    { return m_ComPtr; }

    inline T& operator*() const
    { return *m_ComPtr; }

    inline T* operator->() const
    { return m_ComPtr; }

    inline bool operator!() const
    { return (nullptr == m_ComPtr); }

    inline bool operator<(T* lComPtr) const
    { return m_ComPtr < lComPtr; }

    inline bool operator!=(T* lComPtr) const
    { return !operator==(lComPtr); }

    inline bool operator==(T* lComPtr) const
    { return m_ComPtr == lComPtr; }

protected:
    T* m_ComPtr;
};

constexpr nri::FormatSupportBits COMMON_SUPPORT =
    nri::FormatSupportBits::TEXTURE |
    nri::FormatSupportBits::STORAGE_TEXTURE |
    nri::FormatSupportBits::BUFFER |
    nri::FormatSupportBits::STORAGE_BUFFER |
    nri::FormatSupportBits::COLOR_ATTACHMENT |
    nri::FormatSupportBits::VERTEX_BUFFER;

constexpr nri::FormatSupportBits COMMON_SUPPORT_WITHOUT_VERTEX =
    nri::FormatSupportBits::TEXTURE |
    nri::FormatSupportBits::STORAGE_TEXTURE |
    nri::FormatSupportBits::BUFFER |
    nri::FormatSupportBits::STORAGE_BUFFER |
    nri::FormatSupportBits::COLOR_ATTACHMENT;

constexpr nri::FormatSupportBits D3D_FORMAT_SUPPORT_TABLE[] = {
    nri::FormatSupportBits::UNSUPPORTED, // UNKNOWN,

    COMMON_SUPPORT_WITHOUT_VERTEX, // R8_UNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // R8_SNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // R8_UINT,
    COMMON_SUPPORT_WITHOUT_VERTEX, // R8_SINT,

    COMMON_SUPPORT_WITHOUT_VERTEX, // RG8_UNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RG8_SNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RG8_UINT,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RG8_SINT,

    COMMON_SUPPORT_WITHOUT_VERTEX, // BGRA8_UNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // BGRA8_SRGB,

    COMMON_SUPPORT_WITHOUT_VERTEX, // RGBA8_UNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RGBA8_SNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RGBA8_UINT,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RGBA8_SINT,
    COMMON_SUPPORT_WITHOUT_VERTEX, // RGBA8_SRGB,

    COMMON_SUPPORT, // R16_UNORM,
    COMMON_SUPPORT, // R16_SNORM,
    COMMON_SUPPORT, // R16_UINT,
    COMMON_SUPPORT, // R16_SINT,
    COMMON_SUPPORT, // R16_SFLOAT,

    COMMON_SUPPORT, // RG16_UNORM,
    COMMON_SUPPORT, // RG16_SNORM,
    COMMON_SUPPORT, // RG16_UINT,
    COMMON_SUPPORT, // RG16_SINT,
    COMMON_SUPPORT, // RG16_SFLOAT,

    COMMON_SUPPORT, // RGBA16_UNORM,
    COMMON_SUPPORT, // RGBA16_SNORM,
    COMMON_SUPPORT, // RGBA16_UINT,
    COMMON_SUPPORT, // RGBA16_SINT,
    COMMON_SUPPORT, // RGBA16_SFLOAT,

    COMMON_SUPPORT, // R32_UINT,
    COMMON_SUPPORT, // R32_SINT,
    COMMON_SUPPORT, // R32_SFLOAT,

    COMMON_SUPPORT, // RG32_UINT,
    COMMON_SUPPORT, // RG32_SINT,
    COMMON_SUPPORT, // RG32_SFLOAT,

    COMMON_SUPPORT, // RGB32_UINT,
    COMMON_SUPPORT, // RGB32_SINT,
    COMMON_SUPPORT, // RGB32_SFLOAT,

    COMMON_SUPPORT, // RGBA32_UINT,
    COMMON_SUPPORT, // RGBA32_SINT,
    COMMON_SUPPORT, // RGBA32_SFLOAT,

    COMMON_SUPPORT_WITHOUT_VERTEX, // R10_G10_B10_A2_UNORM,
    COMMON_SUPPORT_WITHOUT_VERTEX, // R10_G10_B10_A2_UINT,
    COMMON_SUPPORT_WITHOUT_VERTEX, // R11_G11_B10_UFLOAT,
    COMMON_SUPPORT_WITHOUT_VERTEX, // R9_G9_B9_E5_UFLOAT,

    nri::FormatSupportBits::TEXTURE, // BC1_RGBA_UNORM,
    nri::FormatSupportBits::TEXTURE, // BC1_RGBA_SRGB,
    nri::FormatSupportBits::TEXTURE, // BC2_RGBA_UNORM,
    nri::FormatSupportBits::TEXTURE, // BC2_RGBA_SRGB,
    nri::FormatSupportBits::TEXTURE, // BC3_RGBA_UNORM,
    nri::FormatSupportBits::TEXTURE, // BC3_RGBA_SRGB,
    nri::FormatSupportBits::TEXTURE, // BC4_R_UNORM,
    nri::FormatSupportBits::TEXTURE, // BC4_R_SNORM,
    nri::FormatSupportBits::TEXTURE, // BC5_RG_UNORM,
    nri::FormatSupportBits::TEXTURE, // BC5_RG_SNORM,
    nri::FormatSupportBits::TEXTURE, // BC6H_RGB_UFLOAT,
    nri::FormatSupportBits::TEXTURE, // BC6H_RGB_SFLOAT,
    nri::FormatSupportBits::TEXTURE, // BC7_RGBA_UNORM,
    nri::FormatSupportBits::TEXTURE, // BC7_RGBA_SRGB,

    // DEPTH_STENCIL_ATTACHMENT views
    nri::FormatSupportBits::DEPTH_STENCIL_ATTACHMENT, // D16_UNORM,
    nri::FormatSupportBits::DEPTH_STENCIL_ATTACHMENT, // D24_UNORM_S8_UINT,
    nri::FormatSupportBits::DEPTH_STENCIL_ATTACHMENT, // D32_SFLOAT,
    nri::FormatSupportBits::DEPTH_STENCIL_ATTACHMENT, // D32_SFLOAT_S8_UINT_X24,

    // Depth-stencil specific SHADER_RESOURCE views
    nri::FormatSupportBits::TEXTURE, // R24_UNORM_X8,
    nri::FormatSupportBits::TEXTURE, // X24_R8_UINT,
    nri::FormatSupportBits::TEXTURE, // X32_R8_UINT_X24,
    nri::FormatSupportBits::TEXTURE, // R32_SFLOAT_X8_X24,

    // MAX_NUM
};

static_assert(GetCountOf(D3D_FORMAT_SUPPORT_TABLE) == (size_t)nri::Format::MAX_NUM, "some format is missing");

struct DisplayDescHelper
{
public:
    nri::Result GetDisplayDesc(void* hwnd, nri::DisplayDesc& displayDesc);
protected:
    ComPtr<IDXGIFactory2> m_DxgiFactory2;
    nri::DisplayDesc m_DisplayDesc = {};
    bool m_HasDisplayDesc = false;
};

#else

struct DisplayDescHelper
{
    inline nri::Result GetDisplayDesc(void* hwnd, nri::DisplayDesc& displayDesc)
    {
        MaybeUnused(hwnd);

        displayDesc = {};
        displayDesc.sdrLuminance = 80.0f;
        displayDesc.maxLuminance = 80.0f;

        return nri::Result::UNSUPPORTED;
    }
};

#endif
