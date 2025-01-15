// © 2021 NVIDIA Corporation

#include "SharedExternal.h"

using namespace nri;

#if NRI_ENABLE_NONE_SUPPORT
Result CreateDeviceNONE(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
#endif

#if NRI_ENABLE_D3D11_SUPPORT
Result CreateDeviceD3D11(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D11(const DeviceCreationD3D11Desc& deviceDesc, DeviceBase*& device);
#endif

#if NRI_ENABLE_D3D12_SUPPORT
Result CreateDeviceD3D12(const DeviceCreationDesc& deviceCreationDesc, DeviceBase*& device);
Result CreateDeviceD3D12(const DeviceCreationD3D12Desc& deviceCreationDesc, DeviceBase*& device);
#endif

#if NRI_ENABLE_VK_SUPPORT
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

    memset(interfacePtr, 0, interfaceSize);

    if (hash == Hash(NRI_STRINGIFY(nri::CoreInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(CoreInterface)))) {
        realInterfaceSize = sizeof(CoreInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(CoreInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::HelperInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(HelperInterface)))) {
        realInterfaceSize = sizeof(HelperInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(HelperInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::LowLatencyInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(LowLatencyInterface)))) {
        realInterfaceSize = sizeof(LowLatencyInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(LowLatencyInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::MeshShaderInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(MeshShaderInterface)))) {
        realInterfaceSize = sizeof(MeshShaderInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(MeshShaderInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::RayTracingInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(RayTracingInterface)))) {
        realInterfaceSize = sizeof(RayTracingInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(RayTracingInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::StreamerInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(StreamerInterface)))) {
        realInterfaceSize = sizeof(StreamerInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(StreamerInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::SwapChainInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(SwapChainInterface)))) {
        realInterfaceSize = sizeof(SwapChainInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(SwapChainInterface*)interfacePtr);
    } else if (hash == Hash(NRI_STRINGIFY(nri::ResourceAllocatorInterface)) || hash == Hash(NRI_STRINGIFY(NRI_NAME_C(ResourceAllocatorInterface)))) {
        realInterfaceSize = sizeof(ResourceAllocatorInterface);
        if (realInterfaceSize == interfaceSize)
            result = deviceBase.FillFunctionTable(*(ResourceAllocatorInterface*)interfacePtr);
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
    }

    if (result == Result::INVALID_ARGUMENT)
        REPORT_ERROR(&deviceBase, "Unknown interface '%s'!", interfaceName);
    else if (interfaceSize != realInterfaceSize)
        REPORT_ERROR(&deviceBase, "Interface '%s' has invalid size = %u bytes, while %u bytes expected by the implementation", interfaceName, interfaceSize, realInterfaceSize);
    else if (result == Result::UNSUPPORTED)
        REPORT_WARNING(&deviceBase, "Interface '%s' is not supported by the device!", interfaceName);
    else {
        const void* const* const begin = (void**)interfacePtr;
        const void* const* const end = begin + realInterfaceSize / sizeof(void*);
        for (const void* const* current = begin; current != end; current++) {
            if (!(*current)) {
                REPORT_ERROR(&deviceBase, "Invalid function table: function #%u is NULL!", uint32_t(current - begin));
                return Result::FAILURE;
            }
        }
    }

    return result;
}

NRI_API void NRI_CALL nriBeginAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxRangePushEx(&eventAttrib);

#    else

    // TODO: add PIX

#    endif
#endif
}

NRI_API void NRI_CALL nriEndAnnotation() {
#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxRangePop();

#    else

    // TODO: add PIX

#    endif
#endif
}

NRI_API void NRI_CALL nriAnnotation(const char* name, uint32_t bgra) {
    MaybeUnused(name, bgra);

#if NRI_ENABLE_DEBUG_NAMES_AND_ANNOTATIONS
#    if NRI_ENABLE_NVTX_SUPPORT

    nvtxEventAttributes_t eventAttrib = {};
    eventAttrib.version = NVTX_VERSION;
    eventAttrib.size = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
    eventAttrib.colorType = bgra == BGRA_UNUSED ? NVTX_COLOR_UNKNOWN : NVTX_COLOR_ARGB;
    eventAttrib.color = bgra;
    eventAttrib.messageType = NVTX_MESSAGE_TYPE_ASCII;
    eventAttrib.message.ascii = name;

    nvtxMarkEx(&eventAttrib);
#    else

    // TODO: add PIX

#    endif
#endif
}

#if NRI_ENABLE_NVTX_SUPPORT
#    if (defined __linux__)
#        include <sys/syscall.h>
#    elif (defined __APPLE__)
#        include <sys/syscall.h>
#    elif (defined __ANDROID__)
#        include <unistd.h>
#    elif (defined _WIN64)
#        include <processthreadsapi.h>
#    endif

NRI_API void NRI_CALL nriSetThreadName(const char* name) {
#    if (defined __linux__)
    nvtxNameOsThreadA(syscall(SYS_gettid), name);
#    elif (defined __APPLE__)
    nvtxNameOsThreadA(syscall(SYS_thread_selfid), name);
#    elif (defined __ANDROID__)
    nvtxNameOsThreadA(gettid(), name);
#    elif (defined _WIN64)
    nvtxNameOsThreadA(GetCurrentThreadId(), name);
#    endif
}

#else

NRI_API void NRI_CALL nriSetThreadName(const char*) {
}

#endif

template <typename T>
Result FinalizeDeviceCreation(const T& deviceCreationDesc, DeviceBase& deviceImpl, Device*& device) {
    if (deviceCreationDesc.enableNRIValidation && deviceCreationDesc.graphicsAPI != GraphicsAPI::NONE) {
        Device* deviceVal = (Device*)CreateDeviceValidation(deviceCreationDesc, deviceImpl);
        if (!deviceVal) {
            nriDestroyDevice((Device&)deviceImpl);
            return Result::FAILURE;
        }

        device = deviceVal;
    } else {
        device = (Device*)&deviceImpl;

#if NRI_ENABLE_NVTX_SUPPORT
        nvtxInitialize(nullptr); // needed only to avoid stalls on the first use
#endif
    }

    return Result::SUCCESS;
}

NRI_API Result NRI_CALL nriCreateDevice(const DeviceCreationDesc& deviceCreationDesc, Device*& device) {
    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

    DeviceCreationDesc modifiedDeviceCreationDesc = deviceCreationDesc;
    CheckAndSetDefaultCallbacks(modifiedDeviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(modifiedDeviceCreationDesc.allocationCallbacks);

#if NRI_ENABLE_NONE_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::NONE)
        result = CreateDeviceNONE(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if NRI_ENABLE_D3D11_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D11)
        result = CreateDeviceD3D11(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if NRI_ENABLE_D3D12_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::D3D12)
        result = CreateDeviceD3D12(modifiedDeviceCreationDesc, deviceImpl);
#endif

#if NRI_ENABLE_VK_SUPPORT
    if (modifiedDeviceCreationDesc.graphicsAPI == GraphicsAPI::VK)
        result = CreateDeviceVK(modifiedDeviceCreationDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(modifiedDeviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D11Device(const DeviceCreationD3D11Desc& deviceCreationD3D11Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D11Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D11Desc.allocationCallbacks;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D11;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D11Desc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.allocationCallbacks);

    DeviceCreationD3D11Desc tempDeviceCreationD3D11Desc = deviceCreationD3D11Desc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationD3D11Desc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationD3D11Desc.allocationCallbacks);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_D3D11_SUPPORT
    result = CreateDeviceD3D11(tempDeviceCreationD3D11Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromD3D12Device(const DeviceCreationD3D12Desc& deviceCreationD3D12Desc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationD3D12Desc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationD3D12Desc.allocationCallbacks;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::D3D12;
    deviceCreationDesc.enableNRIValidation = deviceCreationD3D12Desc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.allocationCallbacks);

    DeviceCreationD3D12Desc tempDeviceCreationD3D12Desc = deviceCreationD3D12Desc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationD3D12Desc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationD3D12Desc.allocationCallbacks);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_D3D12_SUPPORT
    result = CreateDeviceD3D12(tempDeviceCreationD3D12Desc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API Result NRI_CALL nriCreateDeviceFromVkDevice(const DeviceCreationVKDesc& deviceCreationVKDesc, Device*& device) {
    DeviceCreationDesc deviceCreationDesc = {};
    deviceCreationDesc.callbackInterface = deviceCreationVKDesc.callbackInterface;
    deviceCreationDesc.allocationCallbacks = deviceCreationVKDesc.allocationCallbacks;
    deviceCreationDesc.spirvBindingOffsets = deviceCreationVKDesc.spirvBindingOffsets;
    deviceCreationDesc.graphicsAPI = GraphicsAPI::VK;
    deviceCreationDesc.enableNRIValidation = deviceCreationVKDesc.enableNRIValidation;

    CheckAndSetDefaultCallbacks(deviceCreationDesc.callbackInterface);
    CheckAndSetDefaultAllocator(deviceCreationDesc.allocationCallbacks);

    DeviceCreationVKDesc tempDeviceCreationVKDesc = deviceCreationVKDesc;

    CheckAndSetDefaultCallbacks(tempDeviceCreationVKDesc.callbackInterface);
    CheckAndSetDefaultAllocator(tempDeviceCreationVKDesc.allocationCallbacks);

    Result result = Result::UNSUPPORTED;
    DeviceBase* deviceImpl = nullptr;

#if NRI_ENABLE_VK_SUPPORT
    result = CreateDeviceVK(tempDeviceCreationVKDesc, deviceImpl);
#endif

    if (result != Result::SUCCESS)
        return result;

    return FinalizeDeviceCreation(deviceCreationDesc, *deviceImpl, device);
}

NRI_API void NRI_CALL nriDestroyDevice(Device& device) {
    ((DeviceBase&)device).Destruct();
}

NRI_API Format NRI_CALL nriConvertVKFormatToNRI(uint32_t vkFormat) {
    return VKFormatToNRIFormat(vkFormat);
}

NRI_API Format NRI_CALL nriConvertDXGIFormatToNRI(uint32_t dxgiFormat) {
    return DXGIFormatToNRIFormat(dxgiFormat);
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToVK(Format format) {
    MaybeUnused(format);

#if NRI_ENABLE_VK_SUPPORT
    return NRIFormatToVKFormat(format);
#else
    return 0;
#endif
}

NRI_API uint32_t NRI_CALL nriConvertNRIFormatToDXGI(Format format) {
    MaybeUnused(format);

#if (NRI_ENABLE_D3D11_SUPPORT || NRI_ENABLE_D3D12_SUPPORT)
    return NRIFormatToDXGIFormat(format);
#else
    return 0;
#endif
}

NRI_API const FormatProps& NRI_CALL nriGetFormatProps(Format format) {
    return GetFormatProps(format);
}

NRI_API const char* NRI_CALL nriGetGraphicsAPIString(GraphicsAPI graphicsAPI) {
    switch (graphicsAPI) {
        case GraphicsAPI::NONE:
            return "NONE";
        case GraphicsAPI::D3D11:
            return "D3D11";
        case GraphicsAPI::D3D12:
            return "D3D12";
        case GraphicsAPI::VK:
            return "VK";
        default:
            return "UNKNOWN";
    }
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
            wcstombs(adapterDesc.name, desc.Description, GetCountOf(adapterDesc.name) - 1);
            adapterDesc.luid = *(uint64_t*)&desc.AdapterLuid;
            adapterDesc.videoMemorySize = desc.DedicatedVideoMemory; // TODO: add "desc.DedicatedSystemMemory"?
            adapterDesc.sharedSystemMemorySize = desc.SharedSystemMemory;
            adapterDesc.deviceId = desc.DeviceId;
            adapterDesc.vendor = GetVendorFromID(desc.VendorId);
        }
    } else
        adapterDescNum = adaptersNum;

    for (uint32_t i = 0; i < adaptersNum; i++)
        adapters[i]->Release();

    return Result::SUCCESS;
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
        if (!name) \
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
                VkPhysicalDeviceGroupProperties* deviceGroupProperties = (VkPhysicalDeviceGroupProperties*)alloca(sizeof(VkPhysicalDeviceGroupProperties) * deviceGroupNum);
                for (uint32_t i = 0; i < deviceGroupNum; i++) {
                    deviceGroupProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
                    deviceGroupProperties[i].pNext = nullptr;
                }
                vkEnumeratePhysicalDeviceGroups(instance, &deviceGroupNum, deviceGroupProperties);

                // Query device groups properties
                AdapterDesc* adapterDescsSorted = (AdapterDesc*)alloca(sizeof(AdapterDesc) * deviceGroupNum);
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
                    strncpy(adapterDesc.name, properties2.properties.deviceName, sizeof(adapterDesc.name));
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
                            adapterDesc.sharedSystemMemorySize += memoryProperties.memoryHeaps[k].size;
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

NRI_API void NRI_CALL nriReportLiveObjects() {
}

#endif
