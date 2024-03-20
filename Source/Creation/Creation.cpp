// © 2021 NVIDIA Corporation

#include "SharedExternal.h"

using namespace nri;

#if NRI_USE_D3D11
Result CreateDeviceD3D11(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D11(const DeviceCreationD3D11Desc& deviceDesc, DeviceBase*& device);
#endif

#if NRI_USE_D3D12
Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D12(const DeviceCreationD3D12Desc& deviceCreationDesc, DeviceBase*& device);
#endif

#if NRI_USE_VULKAN
Result CreateDeviceVK(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceVK(const DeviceCreationVKDesc& deviceDesc, DeviceBase*& device);
#endif

DeviceBase* CreateDeviceValidation(const DeviceCreationDesc& deviceCreationDesc, DeviceBase& device);

constexpr uint64_t Hash(const char* name) {
    return *name != 0 ? *name ^ (33 * Hash(name + 1)) : 5381;
}

NRI_API Result NRI_CALL nriGetInterface(const Device& device, const char* interfaceName, size_t interfaceSize, void* interfacePtr) {
    const uint64_t hash = Hash(interfaceName);
    size_t realInterfaceSize = size_t(-1);
    Result result = Result::INVALID_ARGUMENT;
    const DeviceBase& deviceBase = (DeviceBase&)device;

    if (hash == Hash(NRI_STRINGIFY(nri::CoreInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(CoreInterface)))) {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::HelperInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(HelperInterface)))) {
        realInterfaceSize = sizeof(HelperInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(HelperInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::WrapperD3D11Interface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(WrapperD3D11Interface)))) {
        realInterfaceSize = sizeof(WrapperD3D11Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D11Interface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::WrapperD3D12Interface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(WrapperD3D12Interface)))) {
        realInterfaceSize = sizeof(WrapperD3D12Interface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperD3D12Interface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::WrapperVKInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(WrapperVKInterface)))) {
        realInterfaceSize = sizeof(WrapperVKInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(WrapperVKInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::SwapChainInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(SwapChainInterface)))) {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::RayTracingInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(RayTracingInterface)))) {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::MeshShaderInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(MeshShaderInterface)))) {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::LowLatencyInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(LowLatencyInterface)))) {
        realInterfaceSize = sizeof(LowLatencyInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(LowLatencyInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::StreamerInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(StreamerInterface)))) {
        realInterfaceSize = sizeof(StreamerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(StreamerInterface*)interfacePtr);
    }

    if (result == Result::INVALID_ARGUMENT)
        REPORT_ERROR(&deviceBase, "Unknown interface '%s'!", interfaceName);
    else if (interfaceSize != realInterfaceSize)
        REPORT_ERROR(&deviceBase, "Interface '%s' has invalid size = %u bytes, while %u bytes expected by the implementation", interfaceName, interfaceSize, realInterfaceSize);
    else if (result == Result::UNSUPPORTED)
        REPORT_WARNING(&deviceBase, "Interface '%s' is not supported by the device!", interfaceName);

    return result;
}

template <typename T>
Result FinalizeDeviceCreation(const T& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device) {
    if (deviceCreationDesc.enableNRIValidation) {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (deviceVal == nullptr) {
            nriDestroyDevice((Device&)deviceImpl);
            return Result::FAILURE;
        }

        device = deviceVal;
    } else
        device = (Device*)&deviceImpl;

    return Result::SUCCESS;
}

NRI_API Result NRI_CALL nriCreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device) {
    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    DeviceCreationDesc modifiedDeviceCreationDesc = deviceCreationDesc;
    CheckAndSetDefaultCallbacks(modifiedDeviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(modifiedDeviceCreationDesc.memoryAllocatorInterface);

#if (NRI_USE_D3D11 == 1)
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D11)
        result = CreateDeviceD3D11(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if (NRI_USE_D3D12 == 1)
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
        result = CreateDeviceD3D12(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if (NRI_USE_VULKAN == 1)
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::VULKAN)
        result = CreateDeviceVK(modifiedDeviceCreationDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(modifiedDeviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D11Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D11Desc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceCreationD3D11Desc tempDeviceCreationD3D11Desc = deviceCreationD3D11Desc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationD3D11Desc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationD3D11Desc.memoryAllocatorInterface);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if (NRI_USE_D3D11 == 1)
    result = CreateDeviceD3D11(tempDeviceCreationD3D11Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationD3D12Desc.memoryAllocatorInterface;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D12Desc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceCreationD3D12Desc tempDeviceCreationD3D12Desc = deviceCreationD3D12Desc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationD3D12Desc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationD3D12Desc.memoryAllocatorInterface);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if (NRI_USE_D3D12 == 1)
    result = CreateDeviceD3D12(tempDeviceCreationD3D12Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromVkDevice(const DeviceCreationVKDesc& deviceCreationVKDesc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationVKDesc.callbackInterface;
    deviceCreationDesc.memoryAllocatorInterface = deviceCreationVKDesc.memoryAllocatorInterface;
    deviceCreationDesc.spirvBindingOffsets = deviceCreationVKDesc.spirvBindingOffsets;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VULKAN;
    deviceCreationDesc.enableNRIValidation = false;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.memoryAllocatorInterface);

    DeviceCreationVKDesc tempDeviceCreationVKDesc = deviceCreationVKDesc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationVKDesc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationVKDesc.memoryAllocatorInterface);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if (NRI_USE_VULKAN == 1)
    result = CreateDeviceVK(tempDeviceCreationVKDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API void NRI_CALL nriDestroyDevice(Device& device) {
    ((DeviceBase&)device).Destroy();
}

NRI_API Format NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat) {
    return VKFormatToNRIFormat(vkFormat);
}

NRI_API Format NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat) {
    return DXGIFormatToNRIFormat(dxgiFormat);
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(Format format) {
    MaybeUnused(format);

#if (NRI_USE_VULKAN == 1)
    return NRIFormatToVKFormat(format);
#else
    return 0;
#endif
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(Format format) {
    MaybeUnused(format);

#if (NRI_USE_D3D11 == 1 || NRI_USE_D3D12 == 1)
    return NRIFormatToDXGIFormat(format);
#else
    return 0;
#endif
}

NRI_API const char* NRI_CALL nriGetGraphicsAPIString(GraphicsAPI graphicsAPI) {
    switch (graphicsAPI) {
        case GraphicsAPI::D3D11:
            return "D3D11";
        case GraphicsAPI::D3D12:
            return "D3D12";
        case GraphicsAPI::VULKAN:
            return "VK";
        default:
            return "UNKNOWN";
    }
}

constexpr std::array<const char*, (size_t)Format::MAX_NUM> formatStrings = {
    "UNKNOWN",
    "R8_UNORM",
    "R8_SNORM",
    "R8_UINT",
    "R8_SINT",
    "RG8_UNORM",
    "RG8_SNORM",
    "RG8_UINT",
    "RG8_SINT",
    "BGRA8_UNORM",
    "BGRA8_SRGB",
    "RGBA8_UNORM",
    "RGBA8_SNORM",
    "RGBA8_UINT",
    "RGBA8_SINT",
    "RGBA8_SRGB",
    "R16_UNORM",
    "R16_SNORM",
    "R16_UINT",
    "R16_SINT",
    "R16_SFLOAT",
    "RG16_UNORM",
    "RG16_SNORM",
    "RG16_UINT",
    "RG16_SINT",
    "RG16_SFLOAT",
    "RGBA16_UNORM",
    "RGBA16_SNORM",
    "RGBA16_UINT",
    "RGBA16_SINT",
    "RGBA16_SFLOAT",
    "R32_UINT",
    "R32_SINT",
    "R32_SFLOAT",
    "RG32_UINT",
    "RG32_SINT",
    "RG32_SFLOAT",
    "RGB32_UINT",
    "RGB32_SINT",
    "RGB32_SFLOAT",
    "RGBA32_UINT",
    "RGBA32_SINT",
    "RGBA32_SFLOAT",
    "R10_G10_B10_A2_UNORM",
    "R10_G10_B10_A2_UINT",
    "R11_G11_B10_UFLOAT",
    "R9_G9_B9_E5_UFLOAT",
    "BC1_RGBA_UNORM",
    "BC1_RGBA_SRGB",
    "BC2_RGBA_UNORM",
    "BC2_RGBA_SRGB",
    "BC3_RGBA_UNORM",
    "BC3_RGBA_SRGB",
    "BC4_R_UNORM",
    "BC4_R_SNORM",
    "BC5_RG_UNORM",
    "BC5_RG_SNORM",
    "BC6H_RGB_UFLOAT",
    "BC6H_RGB_SFLOAT",
    "BC7_RGBA_UNORM",
    "BC7_RGBA_SRGB",
    "D16_UNORM",
    "D24_UNORM_S8_UINT",
    "D32_SFLOAT",
    "D32_SFLOAT_S8_UINT_X24",
    "R24_UNORM_X8",
    "X24_R8_UINT",
    "X32_R8_UINT_X24",
    "R32_SFLOAT_X8_X24",
};

NRI_API const char* NRI_CALL nriGetFormatString(Format format) {
    return formatStrings[(size_t)format];
}

#ifdef _WIN32
#    include <dxgi1_4.h>
#    include <dxgidebug.h>

static int SortAdaptersByDedicatedVideoMemorySize(const void* a, const void* b) {
    DXGI_ADAPTER_DESC ad = {};
    (*(IDXGIAdapter1**)a)->GetDesc(&ad);

    DXGI_ADAPTER_DESC bd = {};
    (*(IDXGIAdapter1**)b)->GetDesc(&bd);

    if (ad.DedicatedVideoMemory > bd.DedicatedVideoMemory)
        return -1;

    if (ad.DedicatedVideoMemory < bd.DedicatedVideoMemory)
        return 1;

    return 0;
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* adapterDescs, uint32_t& adapterDescNum) {
    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return Result::UNSUPPORTED;

    uint32_t adaptersNum = 0;
    IDXGIAdapter1* adapters[32];

    for (uint32_t i = 0; i < GetCountOf(adapters); i++) {
        IDXGIAdapter1* adapter;
        HRESULT hr = dxgifactory->EnumAdapters1(i, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;

        DXGI_ADAPTER_DESC1 desc = {};
        if (adapter->GetDesc1(&desc) == S_OK) {
            if (desc.Flags == DXGI_ADAPTER_FLAG_NONE)
                adapters[adaptersNum++] = adapter;
        }
    }

    if (!adaptersNum)
        return Result::FAILURE;

    if (adapterDescs) {
        qsort(adapters, adaptersNum, sizeof(adapters[0]), SortAdaptersByDedicatedVideoMemorySize);

        if (adaptersNum < adapterDescNum)
            adapterDescNum = adaptersNum;

        for (uint32_t i = 0; i < adapterDescNum; i++) {
            DXGI_ADAPTER_DESC desc = {};
            adapters[i]->GetDesc(&desc);

            AdapterDesc& adapterDesc = adapterDescs[i];
            memset(&adapterDesc, 0, sizeof(adapterDesc));
            wcstombs(adapterDesc.description, desc.Description, GetCountOf(adapterDesc.description) - 1);
            adapterDesc.luid = *(uint64_t*)&desc.AdapterLuid;
            adapterDesc.videoMemorySize = desc.DedicatedVideoMemory;
            adapterDesc.systemMemorySize = desc.DedicatedSystemMemory + desc.SharedSystemMemory;
            adapterDesc.deviceId = desc.DeviceId;
            adapterDesc.vendor = GetVendorFromID(desc.VendorId);
        }
    } else
        adapterDescNum = adaptersNum;

    for (uint32_t i = 0; i < adaptersNum; i++)
        adapters[i]->Release();

    return Result::SUCCESS;
}

NRI_API bool NRI_CALL nriQueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    uint64_t luid = ((DeviceBase&)device).GetDesc().adapterDesc.luid;

    ComPtr<IDXGIFactory4> dxgifactory;
    if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgifactory))))
        return false;

    ComPtr<IDXGIAdapter3> adapter;
    if (FAILED(dxgifactory->EnumAdapterByLuid(*(LUID*)&luid, IID_PPV_ARGS(&adapter))))
        return false;

    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    if (FAILED(adapter->QueryVideoMemoryInfo(0,
            (memoryLocation == MemoryLocation::DEVICE || memoryLocation == MemoryLocation::DEVICE_UPLOAD) ? DXGI_MEMORY_SEGMENT_GROUP_LOCAL : DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL,
            &info)))
        return false;

    static_assert(sizeof(VideoMemoryInfo) == sizeof(DXGI_QUERY_VIDEO_MEMORY_INFO));
    memcpy(&videoMemoryInfo, &info, sizeof(info));

    return true;
}

NRI_API void NRI_CALL nriReportLiveObjects() {
    ComPtr<IDXGIDebug1> pDebug;
    HRESULT hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug));
    if (SUCCEEDED(hr))
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, (DXGI_DEBUG_RLO_FLAGS)((uint32_t)DXGI_DEBUG_RLO_DETAIL | (uint32_t)DXGI_DEBUG_RLO_IGNORE_INTERNAL));
}

#else
#    include <vulkan/vulkan.h>
#    define GET_VK_FUNCTION(instance, name) \
        const auto name = (PFN_##name)vkGetInstanceProcAddr(instance, #name); \
        if (name == nullptr) \
        return Result::UNSUPPORTED

static int SortAdaptersByDedicatedVideoMemorySize(const void* pa, const void* pb) {
    AdapterDesc* a = (AdapterDesc*)pa;
    AdapterDesc* b = (AdapterDesc*)pb;

    if (a->videoMemorySize > b->videoMemorySize)
        return -1;

    if (a->videoMemorySize < b->videoMemorySize)
        return 1;

    return 0;
}

NRI_API Result NRI_CALL nriEnumerateAdapters(AdapterDesc* adapterDescs, uint32_t& adapterDescNum) {
    Library* loader = LoadSharedLibrary(VULKAN_LOADER_NAME);
    if (!loader)
        return Result::UNSUPPORTED;

    const auto vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*loader, "vkGetInstanceProcAddr");
    if (!vkGetInstanceProcAddr)
        return Result::UNSUPPORTED;

    const auto vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    if (!vkCreateInstance)
        return Result::UNSUPPORTED;

    // Create instance
    VkApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

#    ifdef __APPLE__
    std::array<const char*, 2> instanceExtensions = {"VK_KHR_get_physical_device_properties2", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME};

    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
    instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#    endif

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

    Result nriResult = Result::FAILURE;
    if (result == VK_SUCCESS) {
        // Get needed functions
        GET_VK_FUNCTION(instance, vkDestroyInstance);
        GET_VK_FUNCTION(instance, vkEnumeratePhysicalDeviceGroups);
        GET_VK_FUNCTION(instance, vkGetPhysicalDeviceProperties2);
        GET_VK_FUNCTION(instance, vkGetPhysicalDeviceMemoryProperties);

        uint32_t deviceGroupNum = 0;
        result = vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, nullptr);

        if (result == VK_SUCCESS && deviceGroupNum) {
            if (adapterDescs) {
                // Query device groups
                VkPhysicalDeviceGroupProperties* deviceGroupProperties = STACK_ALLOC(VkPhysicalDeviceGroupProperties, deviceGroupNum);
                vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

                // Query device groups properties
                AdapterDesc* adapterDescsSorted = STACK_ALLOC(AdapterDesc, deviceGroupNum);
                for (uint32_t i = 0; i < deviceGroupNum; i++) {
                    VkPhysicalDeviceIDProperties deviceIDProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
                    VkPhysicalDeviceProperties2 properties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
                    properties2.pNext = &deviceIDProperties;

                    VkPhysicalDevice physicalDevice = deviceGroupProperties[i].physicalDevices[0];
                    vkGetPhysicalDeviceProperties2(physicalDevice, &properties2);

                    VkPhysicalDeviceMemoryProperties memoryProperties = {};
                    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

                    AdapterDesc& adapterDesc = adapterDescsSorted[i];
                    memset(&adapterDesc, 0, sizeof(adapterDesc));
                    strncpy(adapterDesc.description, properties2.properties.deviceName, sizeof(adapterDesc.description));
                    adapterDesc.luid = *(uint64_t*)&deviceIDProperties.deviceLUID[0];
                    adapterDesc.deviceId = properties2.properties.deviceID;
                    adapterDesc.vendor = GetVendorFromID(properties2.properties.vendorID);

                    /* THIS IS AWFUL!
                    https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
                    In a unified memory architecture (UMA) system there is often only a single memory heap which is considered to
                    be equally "local" to the host and to the device, and such an implementation must advertise the heap as device-local. */
                    for (uint32_t k = 0; k < memoryProperties.memoryHeapCount; k++) {
                        if (memoryProperties.memoryHeaps[k].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                            adapterDesc.videoMemorySize += memoryProperties.memoryHeaps[k].size;
                        else
                            adapterDesc.systemMemorySize += memoryProperties.memoryHeaps[k].size;
                    }
                }

                // Sort by video memory size
                qsort(adapterDescsSorted, deviceGroupNum, sizeof(adapterDescsSorted[0]), SortAdaptersByDedicatedVideoMemorySize);

                // Copy to output
                if (deviceGroupNum < adapterDescNum)
                    adapterDescNum = deviceGroupNum;

                for (uint32_t i = 0; i < adapterDescNum; i++)
                    *adapterDescs++ = *adapterDescsSorted++;
            } else
                adapterDescNum = deviceGroupNum;

            nriResult = Result::SUCCESS;
        }

        if (instance)
            vkDestroyInstance(instance, nullptr);
    }

    UnloadSharedLibrary(*loader);

    return nriResult;
}

NRI_API bool NRI_CALL nriQueryVideoMemoryInfo(const Device& device, MemoryLocation memoryLocation, VideoMemoryInfo& videoMemoryInfo) {
    MaybeUnused(device);
    MaybeUnused(memoryLocation);
    MaybeUnused(videoMemoryInfo);

    // TODO: use VK_EXT_memory_budget
    return false;
}

NRI_API void NRI_CALL nriReportLiveObjects() {
}

#endif
