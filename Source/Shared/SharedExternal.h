// © 2021 NVIDIA Corporation

#pragma once

#include "NRI.h"

#include "Extensions/NRIDeviceCreation.h"
#include "Extensions/NRIHelper.h"
#include "Extensions/NRILowLatency.h"
#include "Extensions/NRIMeshShader.h"
#include "Extensions/NRIRayTracing.h"
#include "Extensions/NRIStreamer.h"
#include "Extensions/NRISwapChain.h"
#include "Extensions/NRIWrapperD3D11.h"
#include "Extensions/NRIWrapperD3D12.h"
#include "Extensions/NRIWrapperVK.h"

#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <map>

#include "Lock.h"

typedef nri::MemoryAllocatorInterface MemoryAllocatorInterface;
#include "StdAllocator.h"

#ifdef _WIN32
#    include <dxgi1_6.h>
#else
typedef uint32_t DXGI_FORMAT;
#endif

#define NRI_STRINGIFY_(token) #token
#define NRI_STRINGIFY(token) NRI_STRINGIFY_(token)

#define RETURN_ON_BAD_HRESULT(deviceBase, hr, msg) \
    if (FAILED(hr)) { \
        (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, msg " failed, result = 0x%08X!", hr); \
        return GetResultFromHRESULT(hr); \
    }

#define RETURN_ON_FAILURE(deviceBase, condition, returnCode, format, ...) \
    if (!(condition)) { \
        (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__); \
        return returnCode; \
    }

#define REPORT_INFO(deviceBase, format, ...) (deviceBase)->ReportMessage(nri::Message::TYPE_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define REPORT_WARNING(deviceBase, format, ...) (deviceBase)->ReportMessage(nri::Message::TYPE_WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define REPORT_ERROR(deviceBase, format, ...) (deviceBase)->ReportMessage(nri::Message::TYPE_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define CHECK(condition, message) assert(condition&& message)

#define SET_D3D_DEBUG_OBJECT_NAME(obj, name) \
    if (obj) \
    obj->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)std::strlen(name), name)

#define NRI_NODE_MASK 0x1 // mGPU is not planned

#include "DeviceBase.h"

constexpr uint32_t TIMEOUT_PRESENT = 1000; // 1 sec
constexpr uint32_t TIMEOUT_FENCE = 5000;   // 5 sec

constexpr uint64_t PRESENT_INDEX_BIT_NUM = 56ull;

constexpr uint64_t MsToUs(uint32_t x) {
    return x * 1000000ull;
}

constexpr void ReturnVoid() {
}

template <typename... Args>
constexpr void MaybeUnused([[maybe_unused]] const Args&... args) {
}

// Format conversion
struct DxgiFormat {
    DXGI_FORMAT typeless;
    DXGI_FORMAT typed;
};

const DxgiFormat& GetDxgiFormat(nri::Format format);
const nri::FormatProps& GetFormatProps(nri::Format format);

nri::Format DXGIFormatToNRIFormat(uint32_t dxgiFormat);
nri::Format VKFormatToNRIFormat(uint32_t vkFormat);

uint32_t NRIFormatToDXGIFormat(nri::Format format);
uint32_t NRIFormatToVKFormat(nri::Format format);

// Misc
inline nri::Vendor GetVendorFromID(uint32_t vendorID) {
    switch (vendorID) {
        case 0x10DE:
            return nri::Vendor::NVIDIA;
        case 0x1002:
            return nri::Vendor::AMD;
        case 0x8086:
            return nri::Vendor::INTEL;
    }

    return nri::Vendor::UNKNOWN;
}

nri::Result GetResultFromHRESULT(long result);

inline nri::Dim_t GetDimension(nri::GraphicsAPI api, const nri::TextureDesc& textureDesc, nri::Dim_t dimensionIndex, nri::Mip_t mip) {
    assert(dimensionIndex < 3);

    nri::Dim_t dim = textureDesc.depth;
    if (dimensionIndex == 0)
        dim = textureDesc.width;
    else if (dimensionIndex == 1)
        dim = textureDesc.height;

    dim = (nri::Dim_t)std::max(dim >> mip, 1);

    // TODO: VK doesn't require manual alignment, but probably we should use it here and during texture creation
    if (api != nri::GraphicsAPI::VULKAN)
        dim = Align(dim, dimensionIndex < 2 ? GetFormatProps(textureDesc.format).blockWidth : 1);

    return dim;
}

// String conversion
void ConvertCharToWchar(const char* in, wchar_t* out, size_t outLen);
void ConvertWcharToChar(const wchar_t* in, char* out, size_t outLen);

// Callbacks setup
void CheckAndSetDefaultCallbacks(nri::CallbackInterface& callbackInterface);

// Swap chain ID
uint64_t GetSwapChainId();

inline uint64_t GetPresentIndex(uint64_t presentId) {
    return presentId & ((1ull << PRESENT_INDEX_BIT_NUM) - 1ull);
}

// Shared library
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
template <typename T>
struct ComPtr {
    inline ComPtr(T* lComPtr = nullptr) : m_ComPtr(lComPtr) {
        static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");

        if (m_ComPtr)
            m_ComPtr->AddRef();
    }

    inline ComPtr(const ComPtr<T>& lComPtrObj) {
        static_assert(std::is_base_of<IUnknown, T>::value, "T needs to be IUnknown based");

        m_ComPtr = lComPtrObj.m_ComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();
    }

    inline ComPtr(ComPtr<T>&& lComPtrObj) {
        m_ComPtr = lComPtrObj.m_ComPtr;
        lComPtrObj.m_ComPtr = nullptr;
    }

    inline T* operator=(T* lComPtr) {
        if (m_ComPtr)
            m_ComPtr->Release();

        m_ComPtr = lComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();

        return m_ComPtr;
    }

    inline T* operator=(const ComPtr<T>& lComPtrObj) {
        if (m_ComPtr)
            m_ComPtr->Release();

        m_ComPtr = lComPtrObj.m_ComPtr;

        if (m_ComPtr)
            m_ComPtr->AddRef();

        return m_ComPtr;
    }

    inline ~ComPtr() {
        if (m_ComPtr) {
            m_ComPtr->Release();
            m_ComPtr = nullptr;
        }
    }

    inline T** operator&() {
        // The assert on operator& usually indicates a bug. Could be a potential memory leak.
        // If this really what is needed, however, use GetInterface() explicitly.
        assert(m_ComPtr == nullptr);
        return &m_ComPtr;
    }

    inline operator T*() const {
        return m_ComPtr;
    }

    inline T* GetInterface() const {
        return m_ComPtr;
    }

    inline T& operator*() const {
        return *m_ComPtr;
    }

    inline T* operator->() const {
        return m_ComPtr;
    }

    inline bool operator!() const {
        return (nullptr == m_ComPtr);
    }

    inline bool operator<(T* lComPtr) const {
        return m_ComPtr < lComPtr;
    }

    inline bool operator!=(T* lComPtr) const {
        return !operator==(lComPtr);
    }

    inline bool operator==(T* lComPtr) const {
        return m_ComPtr == lComPtr;
    }

protected:
    T* m_ComPtr;
};

bool HasOutput();
nri::Result QueryVideoMemoryInfoDXGI(uint64_t luid, nri::MemoryLocation memoryLocation, nri::VideoMemoryInfo& videoMemoryInfo);

struct DisplayDescHelper {
public:
    nri::Result GetDisplayDesc(void* hwnd, nri::DisplayDesc& displayDesc);

protected:
    ComPtr<IDXGIFactory2> m_DxgiFactory2;
    nri::DisplayDesc m_DisplayDesc = {};
    bool m_HasDisplayDesc = false;
};

#else

struct DisplayDescHelper {
    inline nri::Result GetDisplayDesc(void* hwnd, nri::DisplayDesc& displayDesc) {
        MaybeUnused(hwnd);

        displayDesc = {};
        displayDesc.sdrLuminance = 80.0f;
        displayDesc.maxLuminance = 80.0f;

        return nri::Result::UNSUPPORTED;
    }
};

#endif
