// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "AccelerationStructureVK.h"
#include "BufferVK.h"
#include "CommandAllocatorVK.h"
#include "CommandBufferVK.h"
#include "CommandQueueVK.h"
#include "DescriptorPoolVK.h"
#include "DescriptorSetVK.h"
#include "DescriptorVK.h"
#include "FenceVK.h"
#include "HelperDeviceMemoryAllocator.h"
#include "MemoryVK.h"
#include "PipelineLayoutVK.h"
#include "PipelineVK.h"
#include "QueryPoolVK.h"
#include "Streamer.h"
#include "SwapChainVK.h"
#include "TextureVK.h"

static_assert(VK_LUID_SIZE == sizeof(uint64_t), "invalid sizeof");

constexpr uint32_t INVALID_FAMILY_INDEX = uint32_t(-1);

using namespace nri;

Result CreateDeviceVK(const DeviceCreationDesc& desc, DeviceBase*& device) {
    StdAllocator<uint8_t> allocator(desc.memoryAllocatorInterface);
    DeviceVK* implementation = Allocate<DeviceVK>(allocator, desc.callbackInterface, allocator);

    const Result res = implementation->Create(desc, {}, false);

    if (res == Result::SUCCESS) {
        device = implementation;
        return Result::SUCCESS;
    }

    Deallocate(allocator, implementation);
    return res;
}

Result CreateDeviceVK(const DeviceCreationVKDesc& desc, DeviceBase*& device) {
    StdAllocator<uint8_t> allocator(desc.memoryAllocatorInterface);
    DeviceVK* implementation = Allocate<DeviceVK>(allocator, desc.callbackInterface, allocator);
    const Result res = implementation->Create({}, desc, true);

    if (res == Result::SUCCESS) {
        device = implementation;
        return Result::SUCCESS;
    }

    Deallocate(allocator, implementation);
    return res;
}

inline bool IsExtensionSupported(const char* ext, const Vector<VkExtensionProperties>& list) {
    for (auto& e : list) {
        if (!strcmp(ext, e.extensionName))
            return true;
    }

    return false;
}

inline bool IsExtensionSupported(const char* ext, const Vector<const char*>& list) {
    for (auto& e : list) {
        if (!strcmp(ext, e))
            return true;
    }

    return false;
}

void* VKAPI_PTR vkAllocateHostMemory(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    MaybeUnused(allocationScope);

    StdAllocator<uint8_t>& stdAllocator = *(StdAllocator<uint8_t>*)pUserData;
    const auto& lowLevelAllocator = stdAllocator.GetInterface();

    return lowLevelAllocator.Allocate(lowLevelAllocator.userArg, size, alignment);
}

void* VKAPI_PTR vkReallocateHostMemory(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    MaybeUnused(allocationScope);

    StdAllocator<uint8_t>& stdAllocator = *(StdAllocator<uint8_t>*)pUserData;
    const auto& lowLevelAllocator = stdAllocator.GetInterface();

    return lowLevelAllocator.Reallocate(lowLevelAllocator.userArg, pOriginal, size, alignment);
}

void VKAPI_PTR vkFreeHostMemory(void* pUserData, void* pMemory) {
    StdAllocator<uint8_t>& stdAllocator = *(StdAllocator<uint8_t>*)pUserData;
    const auto& lowLevelAllocator = stdAllocator.GetInterface();

    return lowLevelAllocator.Free(lowLevelAllocator.userArg, pMemory);
}

void VKAPI_PTR vkHostMemoryInternalAllocationNotification(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
    MaybeUnused(pUserData);
    MaybeUnused(size);
    MaybeUnused(allocationType);
    MaybeUnused(allocationScope);
}

void VKAPI_PTR vkHostMemoryInternalFreeNotification(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {
    MaybeUnused(pUserData);
    MaybeUnused(size);
    MaybeUnused(allocationType);
    MaybeUnused(allocationScope);
}

void DeviceVK::FilterInstanceLayers(Vector<const char*>& layers) {
    uint32_t layerNum = 0;
    m_VK.EnumerateInstanceLayerProperties(&layerNum, nullptr);

    Vector<VkLayerProperties> supportedLayers(layerNum, GetStdAllocator());
    m_VK.EnumerateInstanceLayerProperties(&layerNum, supportedLayers.data());

    for (size_t i = 0; i < layers.size(); i++) {
        bool found = false;
        for (uint32_t j = 0; j < layerNum && !found; j++) {
            if (strcmp(supportedLayers[j].layerName, layers[i]) == 0)
                found = true;
        }

        if (!found)
            layers.erase(layers.begin() + i--);
    }
}

void DeviceVK::ProcessInstanceExtensions(Vector<const char*>& desiredInstanceExts) {
    // Query extensions
    uint32_t extensionNum = 0;
    m_VK.EnumerateInstanceExtensionProperties(nullptr, &extensionNum, nullptr);

    Vector<VkExtensionProperties> supportedExts(extensionNum, GetStdAllocator());
    m_VK.EnumerateInstanceExtensionProperties(nullptr, &extensionNum, supportedExts.data());

    REPORT_INFO(this, "Supported instance extensions:");
    for (const VkExtensionProperties& props : supportedExts)
        REPORT_INFO(this, "    %s (v%u)", props.extensionName, props.specVersion);

    // Mandatory
    if (IsExtensionSupported(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

#ifdef __APPLE__
    desiredInstanceExts.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    desiredInstanceExts.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    // Optional
    if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, supportedExts)) {
        desiredInstanceExts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef VK_USE_PLATFORM_WIN32_KHR
        desiredInstanceExts.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
        desiredInstanceExts.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        desiredInstanceExts.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        desiredInstanceExts.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
    }

    if (IsExtensionSupported(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, supportedExts))
        desiredInstanceExts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}

void DeviceVK::ProcessDeviceExtensions(Vector<const char*>& desiredDeviceExts, bool disableRayTracing) {
    // Query extensions
    uint32_t extensionNum = 0;
    m_VK.EnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionNum, nullptr);

    Vector<VkExtensionProperties> supportedExts(extensionNum, GetStdAllocator());
    m_VK.EnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionNum, supportedExts.data());

    REPORT_INFO(this, "Supported device extensions:");
    for (const VkExtensionProperties& props : supportedExts)
        REPORT_INFO(this, "    %s (v%u)", props.extensionName, props.specVersion);

    // Mandatory
    desiredDeviceExts.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

#ifdef __APPLE__
    if (IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
#endif

    // Optional (KHR)
    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PRESENT_ID_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PRESENT_ID_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);

    // Optional (EXT)
    if (IsExtensionSupported(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME, supportedExts) && !disableRayTracing)
        desiredDeviceExts.push_back(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);

    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME);

    // Optional
    if (IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, supportedExts))
        desiredDeviceExts.push_back(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);
}

template <typename Implementation, typename Interface, typename... Args>
Result DeviceVK::CreateImplementation(Interface*& entity, const Args&... args) {
    Implementation* implementation = Allocate<Implementation>(GetStdAllocator(), *this);
    const Result result = implementation->Create(args...);

    if (result == Result::SUCCESS) {
        entity = (Interface*)implementation;
        return Result::SUCCESS;
    }

    Deallocate(GetStdAllocator(), implementation);
    return result;
}

DeviceVK::DeviceVK(const CallbackInterface& callbacks, const StdAllocator<uint8_t>& stdAllocator) :
    DeviceBase(callbacks, stdAllocator), m_ConcurrentSharingModeQueueIndices(GetStdAllocator()) {
    m_Desc.graphicsAPI = GraphicsAPI::VULKAN;
    m_Desc.nriVersionMajor = NRI_VERSION_MAJOR;
    m_Desc.nriVersionMinor = NRI_VERSION_MINOR;
}

DeviceVK::~DeviceVK() {
    if (m_Device == VK_NULL_HANDLE)
        return;

    for (uint32_t i = 0; i < m_Queues.size(); i++)
        Deallocate(GetStdAllocator(), m_Queues[i]);

    if (m_Messenger) {
        typedef PFN_vkDestroyDebugUtilsMessengerEXT Func;
        Func destroyCallback = (Func)m_VK.GetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
        destroyCallback(m_Instance, m_Messenger, m_AllocationCallbackPtr);
    }

    if (m_OwnsNativeObjects) {
        m_VK.DestroyDevice(m_Device, m_AllocationCallbackPtr);
        m_VK.DestroyInstance(m_Instance, m_AllocationCallbackPtr);
    }

    if (m_Loader)
        UnloadSharedLibrary(*m_Loader);
}

void DeviceVK::GetAdapterDesc() {
    VkPhysicalDeviceIDProperties deviceIDProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
    VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &deviceIDProps};
    m_VK.GetPhysicalDeviceProperties2(m_PhysicalDevice, &props);

#ifdef _WIN32
    static_assert(sizeof(LUID) == VK_LUID_SIZE, "invalid sizeof");

    ComPtr<IDXGIFactory4> dxgiFactory;
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr))
        REPORT_WARNING(this, "CreateDXGIFactory2() failed, result = 0x%08X!", hr);

    ComPtr<IDXGIAdapter> adapter;
    LUID luid = *(LUID*)&deviceIDProps.deviceLUID[0];
    hr = dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&adapter));
    if (FAILED(hr))
        REPORT_WARNING(this, "IDXGIFactory4::EnumAdapterByLuid() failed, result = 0x%08X!", hr);

    DXGI_ADAPTER_DESC desc = {};
    hr = adapter->GetDesc(&desc);
    if (FAILED(hr))
        REPORT_WARNING(this, "IDXGIAdapter::GetDesc() failed, result = 0x%08X!", hr);
    else {
        wcstombs(m_Desc.adapterDesc.description, desc.Description, GetCountOf(m_Desc.adapterDesc.description) - 1);
        m_Desc.adapterDesc.luid = *(uint64_t*)&desc.AdapterLuid;
        m_Desc.adapterDesc.videoMemorySize = desc.DedicatedVideoMemory;
        m_Desc.adapterDesc.systemMemorySize = desc.DedicatedSystemMemory + desc.SharedSystemMemory;
        m_Desc.adapterDesc.deviceId = desc.DeviceId;
        m_Desc.adapterDesc.vendor = GetVendorFromID(desc.VendorId);
    }
#else
    strncpy(m_Desc.adapterDesc.description, props.properties.deviceName, sizeof(m_Desc.adapterDesc.description));
    m_Desc.adapterDesc.luid = *(uint64_t*)&deviceIDProps.deviceLUID[0];
    m_Desc.adapterDesc.deviceId = props.properties.deviceID;
    m_Desc.adapterDesc.vendor = GetVendorFromID(props.properties.vendorID);

    /* THIS IS AWFUL!
    https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceMemoryProperties.html
    In a unified memory architecture (UMA) system there is often only a single memory heap which is considered to
    be equally "local" to the host and to the device, and such an implementation must advertise the heap as device-local. */
    for (uint32_t k = 0; k < m_MemoryProps.memoryHeapCount; k++) {
        if (m_MemoryProps.memoryHeaps[k].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            m_Desc.adapterDesc.videoMemorySize += m_MemoryProps.memoryHeaps[k].size;
        else
            m_Desc.adapterDesc.systemMemorySize += m_MemoryProps.memoryHeaps[k].size;
    }
#endif
}

Result DeviceVK::Create(const DeviceCreationDesc& deviceCreationDesc, const DeviceCreationVKDesc& deviceCreationVKDesc, bool isWrapper) {
    m_OwnsNativeObjects = !isWrapper;
    m_SPIRVBindingOffsets = isWrapper ? deviceCreationVKDesc.spirvBindingOffsets : deviceCreationDesc.spirvBindingOffsets;

    { // Custom allocator
        m_AllocationCallbacks.pUserData = &GetStdAllocator();
        m_AllocationCallbacks.pfnAllocation = vkAllocateHostMemory;
        m_AllocationCallbacks.pfnReallocation = vkReallocateHostMemory;
        m_AllocationCallbacks.pfnFree = vkFreeHostMemory;
        m_AllocationCallbacks.pfnInternalAllocation = vkHostMemoryInternalAllocationNotification;
        m_AllocationCallbacks.pfnInternalFree = vkHostMemoryInternalFreeNotification;

        if (!isWrapper)
            m_AllocationCallbackPtr = &m_AllocationCallbacks;
    }

    { // Loader
        const char* loaderPath = deviceCreationVKDesc.vulkanLoaderPath ? deviceCreationVKDesc.vulkanLoaderPath : VULKAN_LOADER_NAME;
        m_Loader = LoadSharedLibrary(loaderPath);
        if (!m_Loader) {
            REPORT_ERROR(this, "Failed to load Vulkan loader: '%s'.", loaderPath);
            return Result::UNSUPPORTED;
        }
    }

    // Create instance
    Vector<const char*> desiredInstanceExts(GetStdAllocator());
    {
        Result res = ResolvePreInstanceDispatchTable();
        if (res != Result::SUCCESS)
            return res;

        if (isWrapper) {
            for (uint32_t i = 0; i < deviceCreationVKDesc.enabledExtensions.instanceExtensionNum; i++)
                desiredInstanceExts.push_back(deviceCreationVKDesc.enabledExtensions.instanceExtensions[i]);

            m_Instance = (VkInstance)deviceCreationVKDesc.vkInstance;
        } else {
            ProcessInstanceExtensions(desiredInstanceExts);

            for (uint32_t i = 0; i < deviceCreationDesc.vulkanExtensions.instanceExtensionNum; i++)
                desiredInstanceExts.push_back(deviceCreationDesc.vulkanExtensions.instanceExtensions[i]);

            res = CreateInstance(deviceCreationDesc.enableAPIValidation, desiredInstanceExts);
            if (res != Result::SUCCESS)
                return res;
        }

        res = ResolveInstanceDispatchTable(desiredInstanceExts);
        if (res != Result::SUCCESS)
            return res;
    }

    uint32_t minorVersion = 0;
    { // Group
        if (isWrapper) {
            m_PhysicalDevice = (VkPhysicalDevice)deviceCreationVKDesc.vkPhysicalDevice;

            minorVersion = deviceCreationVKDesc.isVulkan12 ? 2 : 3;
        } else {
            uint32_t deviceGroupNum = 0;
            m_VK.EnumeratePhysicalDeviceGroups(m_Instance, &deviceGroupNum, nullptr);

            VkPhysicalDeviceGroupProperties* deviceGroups = STACK_ALLOC(VkPhysicalDeviceGroupProperties, deviceGroupNum);
            VkResult result = m_VK.EnumeratePhysicalDeviceGroups(m_Instance, &deviceGroupNum, deviceGroups);
            RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkEnumeratePhysicalDevices returned %d", (int32_t)result);

            VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

            VkPhysicalDeviceIDProperties idProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES};
            props.pNext = &idProps;

            uint32_t i = 0;
            for (; i < deviceGroupNum; i++) {
                const VkPhysicalDeviceGroupProperties& group = deviceGroups[i];
                m_VK.GetPhysicalDeviceProperties2(group.physicalDevices[0], &props);

                uint32_t majorVersion = VK_VERSION_MAJOR(props.properties.apiVersion);
                minorVersion = VK_VERSION_MINOR(props.properties.apiVersion);

                bool isSupported = majorVersion * 10 + minorVersion >= 12;
                if (deviceCreationDesc.adapterDesc) {
                    const uint64_t luid = *(uint64_t*)idProps.deviceLUID;
                    if (luid == deviceCreationDesc.adapterDesc->luid) {
                        RETURN_ON_FAILURE(this, isSupported, Result::UNSUPPORTED, "Can't create a device: the specified physical device does not support Vulkan 1.2+!");
                        break;
                    }
                } else if (isSupported)
                    break;
            }

            RETURN_ON_FAILURE(this, i != deviceGroupNum, Result::INVALID_ARGUMENT, "Can't create a device: physical device not found");

            const VkPhysicalDeviceGroupProperties& group = deviceGroups[i];
            if (group.physicalDeviceCount > 1) {
                if (group.subsetAllocation == VK_FALSE)
                    REPORT_WARNING(this, "The device group does not support memory allocation on a subset of the physical devices");
            }

            m_PhysicalDevice = group.physicalDevices[0];
        }

        m_VK.GetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &m_MemoryProps);

        FillFamilyIndices(isWrapper, deviceCreationVKDesc.queueFamilyIndices, deviceCreationVKDesc.queueFamilyIndexNum);
    }

    // Get adapter description as early as possible for meaningful error reporting
    GetAdapterDesc();

    // Device extensions
    Vector<const char*> desiredDeviceExts(GetStdAllocator());

    if (isWrapper) {
        for (uint32_t i = 0; i < deviceCreationVKDesc.enabledExtensions.deviceExtensionNum; i++)
            desiredDeviceExts.push_back(deviceCreationVKDesc.enabledExtensions.deviceExtensions[i]);
    } else {
        ProcessDeviceExtensions(desiredDeviceExts, deviceCreationDesc.disableVulkanRayTracing);

        for (uint32_t i = 0; i < deviceCreationDesc.vulkanExtensions.deviceExtensionNum; i++)
            desiredDeviceExts.push_back(deviceCreationDesc.vulkanExtensions.deviceExtensions[i]);
    }

    // Device features
    VkPhysicalDeviceFeatures2 features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    void** tail = &features.pNext;

    VkPhysicalDeviceVulkan11Features features11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    APPEND_EXT(features11);

    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    APPEND_EXT(features12);

    VkPhysicalDeviceVulkan13Features features13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    if (minorVersion >= 3) {
        APPEND_EXT(features13);
    }

#ifdef __APPLE__
    VkPhysicalDevicePortabilitySubsetFeaturesKHR portabilitySubsetFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(portabilitySubsetFeatures);
    }
#endif

    VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_MAINTENANCE_5_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(maintenance5Features);
    }

    VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shadingRateFeatures);
    }

    VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_PRESENT_ID_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(presentIdFeatures);
    }

    VkPhysicalDevicePresentWaitFeaturesKHR presentWaitFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(presentWaitFeatures);
    }

    VkPhysicalDeviceLineRasterizationFeaturesEXT lineRasterizationFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(lineRasterizationFeatures);
    }

    VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(meshShaderFeatures);
    }

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(accelerationStructureFeatures);
    }

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayTracingPipelineFeatures);
    }

    VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayQueryFeatures);
    }

    VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenanceFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR};
    if (IsExtensionSupported(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(rayTracingMaintenanceFeatures);
    }

    VkPhysicalDeviceOpacityMicromapFeaturesEXT micromapFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(micromapFeatures);
    }

    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shaderAtomicFloatFeatures);
    }

    VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT};
    if (IsExtensionSupported(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME, desiredDeviceExts)) {
        APPEND_EXT(shaderAtomicFloat2Features);
    }

    m_VK.GetPhysicalDeviceFeatures2(m_PhysicalDevice, &features);

    { // Create device
        if (isWrapper)
            m_Device = (VkDevice)deviceCreationVKDesc.vkDevice;
        else {
            // Disable features here
            // ...

            // Create device
            Vector<VkDeviceQueueCreateInfo> queues(GetStdAllocator());
            const float priorities = 1.0f;
            for (size_t i = 0; i < m_FamilyIndices.size(); i++) {
                if (m_FamilyIndices[i] == INVALID_FAMILY_INDEX)
                    continue;

                VkDeviceQueueCreateInfo info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
                info.queueCount = 1;
                info.queueFamilyIndex = m_FamilyIndices[i];
                info.pQueuePriorities = &priorities;
                queues.push_back(info);
            }

            VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            deviceCreateInfo.pNext = &features;
            deviceCreateInfo.queueCreateInfoCount = (uint32_t)queues.size();
            deviceCreateInfo.pQueueCreateInfos = queues.data();
            deviceCreateInfo.enabledExtensionCount = (uint32_t)desiredDeviceExts.size();
            deviceCreateInfo.ppEnabledExtensionNames = desiredDeviceExts.data();

            VkResult result = m_VK.CreateDevice(m_PhysicalDevice, &deviceCreateInfo, m_AllocationCallbackPtr, &m_Device);
            RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkCreateDevice returned %d", (int32_t)result);
        }

        Result res = ResolveDispatchTable(desiredDeviceExts);
        if (res != Result::SUCCESS)
            return res;
    }

    // Finalize
    CreateCommandQueues();
    ReportDeviceGroupInfo();

    { // Desc
        // Device properties
        VkPhysicalDeviceProperties2 props = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        tail = &props.pNext;

        VkPhysicalDeviceVulkan11Properties props11 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES};
        APPEND_EXT(props11);

        VkPhysicalDeviceVulkan12Properties props12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES};
        APPEND_EXT(props12);

        VkPhysicalDeviceVulkan13Properties props13 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES};
        if (minorVersion >= 3) {
            APPEND_EXT(props13);
        }

        VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(conservativeRasterProps);
        }

        VkPhysicalDeviceLineRasterizationPropertiesEXT lineRasterizationProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(lineRasterizationProps);
        }

        VkPhysicalDeviceSampleLocationsPropertiesEXT sampleLocationsProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(sampleLocationsProps);
        }

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(rayTracingProps);
        }

        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};
        if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(accelerationStructureProps);
            m_Desc.isRayTracingSupported = true;
        }

        VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT};
        if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
            APPEND_EXT(meshShaderProps);
            m_Desc.isMeshShaderSupported = true;
        }

        m_VK.GetPhysicalDeviceProperties2(m_PhysicalDevice, &props);

        // Internal features
        m_IsDescriptorIndexingSupported = features12.descriptorIndexing;
        m_IsDeviceAddressSupported = features12.bufferDeviceAddress;
        m_IsSwapChainMutableFormatSupported = IsExtensionSupported(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME, desiredDeviceExts);
        m_IsPresentIdSupported = presentIdFeatures.presentId;
        m_IsPresentWaitSupported = m_IsPresentIdSupported && presentWaitFeatures.presentWait;
        m_IsLowLatencySupported = m_IsPresentIdSupported && IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts);

        // Fill desc
        const VkPhysicalDeviceLimits& limits = props.properties.limits;

        m_Desc.viewportMaxNum = limits.maxViewports;
        m_Desc.viewportSubPixelBits = limits.viewportSubPixelBits;
        m_Desc.viewportBoundsRange[0] = int32_t(limits.viewportBoundsRange[0]);
        m_Desc.viewportBoundsRange[1] = int32_t(limits.viewportBoundsRange[1]);

        m_Desc.attachmentMaxDim = (Dim_t)std::min(limits.maxFramebufferWidth, limits.maxFramebufferHeight);
        m_Desc.attachmentLayerMaxNum = (Dim_t)limits.maxFramebufferLayers;
        m_Desc.colorAttachmentMaxNum = (Dim_t)limits.maxColorAttachments;

        m_Desc.colorSampleMaxNum = (Sample_t)limits.framebufferColorSampleCounts;
        m_Desc.depthSampleMaxNum = (Sample_t)limits.framebufferDepthSampleCounts;
        m_Desc.stencilSampleMaxNum = (Sample_t)limits.framebufferStencilSampleCounts;
        m_Desc.zeroAttachmentsSampleMaxNum = (Sample_t)limits.framebufferNoAttachmentsSampleCounts;
        m_Desc.textureColorSampleMaxNum = (Sample_t)limits.sampledImageColorSampleCounts;
        m_Desc.textureIntegerSampleMaxNum = (Sample_t)limits.sampledImageIntegerSampleCounts;
        m_Desc.textureDepthSampleMaxNum = (Sample_t)limits.sampledImageDepthSampleCounts;
        m_Desc.textureStencilSampleMaxNum = (Sample_t)limits.sampledImageStencilSampleCounts;
        m_Desc.storageTextureSampleMaxNum = (Sample_t)limits.storageImageSampleCounts;

        m_Desc.texture1DMaxDim = (Dim_t)limits.maxImageDimension1D;
        m_Desc.texture2DMaxDim = (Dim_t)limits.maxImageDimension2D;
        m_Desc.texture3DMaxDim = (Dim_t)limits.maxImageDimension3D;
        m_Desc.textureArrayMaxDim = (Dim_t)limits.maxImageArrayLayers;
        m_Desc.texelBufferMaxDim = limits.maxTexelBufferElements;

        const VkMemoryPropertyFlags neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
            const VkMemoryType& memoryType = m_MemoryProps.memoryTypes[i];
            if ((memoryType.propertyFlags & neededFlags) == neededFlags)
                m_Desc.deviceUploadHeapSize += m_MemoryProps.memoryHeaps[memoryType.heapIndex].size;
        }

        m_Desc.memoryAllocationMaxNum = limits.maxMemoryAllocationCount;
        m_Desc.samplerAllocationMaxNum = limits.maxSamplerAllocationCount;
        m_Desc.uploadBufferTextureRowAlignment = 1;
        m_Desc.uploadBufferTextureSliceAlignment = 1;
        m_Desc.typedBufferOffsetAlignment = (uint32_t)limits.minTexelBufferOffsetAlignment;
        m_Desc.constantBufferOffsetAlignment = (uint32_t)limits.minUniformBufferOffsetAlignment;
        m_Desc.constantBufferMaxRange = limits.maxUniformBufferRange;
        m_Desc.storageBufferOffsetAlignment = (uint32_t)limits.minStorageBufferOffsetAlignment;
        m_Desc.storageBufferMaxRange = limits.maxStorageBufferRange;
        m_Desc.bufferMaxSize = props13.maxBufferSize;
        m_Desc.bufferTextureGranularity = (uint32_t)limits.bufferImageGranularity;
        m_Desc.pushConstantsMaxSize = limits.maxPushConstantsSize;

        m_Desc.boundDescriptorSetMaxNum = limits.maxBoundDescriptorSets;
        m_Desc.perStageDescriptorSamplerMaxNum = limits.maxPerStageDescriptorSamplers;
        m_Desc.perStageDescriptorConstantBufferMaxNum = limits.maxPerStageDescriptorUniformBuffers;
        m_Desc.perStageDescriptorStorageBufferMaxNum = limits.maxPerStageDescriptorStorageBuffers;
        m_Desc.perStageDescriptorTextureMaxNum = limits.maxPerStageDescriptorSampledImages;
        m_Desc.perStageDescriptorStorageTextureMaxNum = limits.maxPerStageDescriptorStorageImages;
        m_Desc.perStageResourceMaxNum = limits.maxPerStageResources;

        m_Desc.descriptorSetSamplerMaxNum = limits.maxDescriptorSetSamplers;
        m_Desc.descriptorSetConstantBufferMaxNum = limits.maxDescriptorSetUniformBuffers;
        m_Desc.descriptorSetStorageBufferMaxNum = limits.maxDescriptorSetStorageBuffers;
        m_Desc.descriptorSetTextureMaxNum = limits.maxDescriptorSetSampledImages;
        m_Desc.descriptorSetStorageTextureMaxNum = limits.maxDescriptorSetStorageImages;

        m_Desc.vertexShaderAttributeMaxNum = limits.maxVertexInputAttributes;
        m_Desc.vertexShaderStreamMaxNum = limits.maxVertexInputBindings;
        m_Desc.vertexShaderOutputComponentMaxNum = limits.maxVertexOutputComponents;

        m_Desc.tessControlShaderGenerationMaxLevel = (float)limits.maxTessellationGenerationLevel;
        m_Desc.tessControlShaderPatchPointMaxNum = limits.maxTessellationPatchSize;
        m_Desc.tessControlShaderPerVertexInputComponentMaxNum = limits.maxTessellationControlPerVertexInputComponents;
        m_Desc.tessControlShaderPerVertexOutputComponentMaxNum = limits.maxTessellationControlPerVertexOutputComponents;
        m_Desc.tessControlShaderPerPatchOutputComponentMaxNum = limits.maxTessellationControlPerPatchOutputComponents;
        m_Desc.tessControlShaderTotalOutputComponentMaxNum = limits.maxTessellationControlTotalOutputComponents;
        m_Desc.tessEvaluationShaderInputComponentMaxNum = limits.maxTessellationEvaluationInputComponents;
        m_Desc.tessEvaluationShaderOutputComponentMaxNum = limits.maxTessellationEvaluationOutputComponents;

        m_Desc.geometryShaderInvocationMaxNum = limits.maxGeometryShaderInvocations;
        m_Desc.geometryShaderInputComponentMaxNum = limits.maxGeometryInputComponents;
        m_Desc.geometryShaderOutputComponentMaxNum = limits.maxGeometryOutputComponents;
        m_Desc.geometryShaderOutputVertexMaxNum = limits.maxGeometryOutputVertices;
        m_Desc.geometryShaderTotalOutputComponentMaxNum = limits.maxGeometryTotalOutputComponents;

        m_Desc.fragmentShaderInputComponentMaxNum = limits.maxFragmentInputComponents;
        m_Desc.fragmentShaderOutputAttachmentMaxNum = limits.maxFragmentOutputAttachments;
        m_Desc.fragmentShaderDualSourceAttachmentMaxNum = limits.maxFragmentDualSrcAttachments;
        m_Desc.fragmentShaderCombinedOutputResourceMaxNum = limits.maxFragmentCombinedOutputResources;

        m_Desc.computeShaderSharedMemoryMaxSize = limits.maxComputeSharedMemorySize;
        m_Desc.computeShaderWorkGroupMaxNum[0] = limits.maxComputeWorkGroupCount[0];
        m_Desc.computeShaderWorkGroupMaxNum[1] = limits.maxComputeWorkGroupCount[1];
        m_Desc.computeShaderWorkGroupMaxNum[2] = limits.maxComputeWorkGroupCount[2];
        m_Desc.computeShaderWorkGroupInvocationMaxNum = limits.maxComputeWorkGroupInvocations;
        m_Desc.computeShaderWorkGroupMaxDim[0] = limits.maxComputeWorkGroupSize[0];
        m_Desc.computeShaderWorkGroupMaxDim[1] = limits.maxComputeWorkGroupSize[1];
        m_Desc.computeShaderWorkGroupMaxDim[2] = limits.maxComputeWorkGroupSize[2];

        m_Desc.rayTracingShaderGroupIdentifierSize = rayTracingProps.shaderGroupHandleSize;
        m_Desc.rayTracingShaderRecursionMaxDepth = rayTracingProps.maxRayRecursionDepth;
        m_Desc.rayTracingGeometryObjectMaxNum = (uint32_t)accelerationStructureProps.maxGeometryCount;
        m_Desc.rayTracingShaderTableAligment = rayTracingProps.shaderGroupBaseAlignment;
        m_Desc.rayTracingShaderTableMaxStride = rayTracingProps.maxShaderGroupStride;

        m_Desc.meshControlSharedMemoryMaxSize = meshShaderProps.maxTaskSharedMemorySize;
        m_Desc.meshControlWorkGroupInvocationMaxNum = meshShaderProps.maxTaskWorkGroupInvocations;
        m_Desc.meshControlPayloadMaxSize = meshShaderProps.maxTaskPayloadSize;
        m_Desc.meshEvaluationOutputVerticesMaxNum = meshShaderProps.maxMeshOutputVertices;
        m_Desc.meshEvaluationOutputPrimitiveMaxNum = meshShaderProps.maxMeshOutputPrimitives;
        m_Desc.meshEvaluationOutputComponentMaxNum = meshShaderProps.maxMeshOutputComponents;
        m_Desc.meshEvaluationSharedMemoryMaxSize = meshShaderProps.maxMeshSharedMemorySize;
        m_Desc.meshEvaluationWorkGroupInvocationMaxNum = meshShaderProps.maxMeshWorkGroupInvocations;

        m_Desc.timestampFrequencyHz = uint64_t(1e9 / double(limits.timestampPeriod) + 0.5);
        m_Desc.subPixelPrecisionBits = limits.subPixelPrecisionBits;
        m_Desc.subTexelPrecisionBits = limits.subTexelPrecisionBits;
        m_Desc.mipmapPrecisionBits = limits.mipmapPrecisionBits;
        m_Desc.drawIndexedIndex16ValueMax = std::min<uint32_t>(std::numeric_limits<uint16_t>::max(), limits.maxDrawIndexedIndexValue);
        m_Desc.drawIndexedIndex32ValueMax = limits.maxDrawIndexedIndexValue;
        m_Desc.drawIndirectMaxNum = limits.maxDrawIndirectCount;
        m_Desc.samplerLodBiasMin = -limits.maxSamplerLodBias;
        m_Desc.samplerLodBiasMax = limits.maxSamplerLodBias;
        m_Desc.samplerAnisotropyMax = limits.maxSamplerAnisotropy;
        m_Desc.texelOffsetMin = limits.minTexelOffset;
        m_Desc.texelOffsetMax = limits.maxTexelOffset;
        m_Desc.texelGatherOffsetMin = limits.minTexelGatherOffset;
        m_Desc.texelGatherOffsetMax = limits.maxTexelGatherOffset;
        m_Desc.clipDistanceMaxNum = limits.maxClipDistances;
        m_Desc.cullDistanceMaxNum = limits.maxCullDistances;
        m_Desc.combinedClipAndCullDistanceMaxNum = limits.maxCombinedClipAndCullDistances;

        if (conservativeRasterProps.fullyCoveredFragmentShaderInputVariable && conservativeRasterProps.primitiveOverestimationSize <= (1.0 / 256.0f))
            m_Desc.conservativeRasterTier = 3;
        else if (conservativeRasterProps.degenerateTrianglesRasterized && conservativeRasterProps.primitiveOverestimationSize < (1.0f / 2.0f))
            m_Desc.conservativeRasterTier = 2;
        else
            m_Desc.conservativeRasterTier = 1;

        m_Desc.programmableSampleLocationsTier = 0;
        if (sampleLocationsProps.sampleLocationSampleCounts)
            m_Desc.programmableSampleLocationsTier = sampleLocationsProps.variableSampleLocations ? 2 : 1; // TODO: best guess

        m_Desc.isTextureFilterMinMaxSupported = features12.samplerFilterMinmax;
        m_Desc.isLogicOpSupported = features.features.logicOp;
        m_Desc.isDepthBoundsTestSupported = features.features.depthBounds;
        m_Desc.isComputeQueueSupported = m_Queues[(uint32_t)CommandQueueType::COMPUTE] != nullptr;
        m_Desc.isCopyQueueSupported = m_Queues[(uint32_t)CommandQueueType::COPY] != nullptr;
        m_Desc.isDrawIndirectCountSupported = features12.drawIndirectCount;
        m_Desc.isIndependentFrontAndBackStencilReferenceAndMasksSupported = true;
        m_Desc.isLineSmoothingSupported = lineRasterizationFeatures.smoothLines;
        m_Desc.isCopyQueueTimestampSupported = limits.timestampComputeAndGraphics;
        m_Desc.isDispatchRaysIndirectSupported = rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect;
        m_Desc.isMeshShaderPipelineStatsSupported = meshShaderFeatures.meshShaderQueries == VK_TRUE;
        m_Desc.isDrawMeshTasksIndirectSupported = true;
        m_Desc.isEnchancedBarrierSupported = true;

        m_Desc.isShaderNativeI16Supported = features.features.shaderInt16;
        m_Desc.isShaderNativeF16Supported = features12.shaderFloat16;
        m_Desc.isShaderNativeI32Supported = true;
        m_Desc.isShaderNativeF32Supported = true;
        m_Desc.isShaderNativeI64Supported = features.features.shaderInt64;
        m_Desc.isShaderNativeF64Supported = features.features.shaderFloat64;

        m_Desc.isShaderAtomicsF16Supported = (shaderAtomicFloat2Features.shaderBufferFloat16Atomics || shaderAtomicFloat2Features.shaderSharedFloat16Atomics) ? true : false;
        m_Desc.isShaderAtomicsI32Supported = true;
        m_Desc.isShaderAtomicsF32Supported = (shaderAtomicFloatFeatures.shaderBufferFloat32Atomics || shaderAtomicFloatFeatures.shaderSharedFloat32Atomics) ? true : false;
        m_Desc.isShaderAtomicsI64Supported = (features12.shaderBufferInt64Atomics || features12.shaderSharedInt64Atomics) ? true : false;
        m_Desc.isShaderAtomicsF64Supported = (shaderAtomicFloatFeatures.shaderBufferFloat64Atomics || shaderAtomicFloatFeatures.shaderSharedFloat64Atomics) ? true : false;

        m_Desc.isSwapChainSupported = IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, desiredDeviceExts);
        m_Desc.isLowLatencySupported = IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts);
    }

    return FillFunctionTable(m_CoreInterface);
}

bool DeviceVK::GetMemoryType(MemoryLocation memoryLocation, uint32_t memoryTypeMask, MemoryTypeInfo& memoryTypeInfo) const {
    VkMemoryPropertyFlags neededFlags = 0;    // must have
    VkMemoryPropertyFlags undesiredFlags = 0; // have higher priority than desired
    VkMemoryPropertyFlags desiredFlags = 0;   // nice to have

    if (memoryLocation == MemoryLocation::DEVICE) {
        neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        undesiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    } else if (memoryLocation == MemoryLocation::DEVICE_UPLOAD) {
        neededFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        desiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        undesiredFlags = VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    } else {
        neededFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        undesiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desiredFlags = memoryLocation == MemoryLocation::HOST_READBACK ? VK_MEMORY_PROPERTY_HOST_CACHED_BIT : 0;
    }

    // Phase 1: needed, undesired and desired
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
        bool hasUndesiredFlags = undesiredFlags == 0 ? false : (m_MemoryProps.memoryTypes[i].propertyFlags & undesiredFlags) == undesiredFlags;
        bool hasDesiredFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags;

        if (isSupported && hasNeededFlags && !hasUndesiredFlags && hasDesiredFlags) {
            memoryTypeInfo.memoryTypeIndex = (MemoryTypeIndexType)i;
            memoryTypeInfo.memoryLocation = memoryLocation;

            return true;
        }
    }

    // Phase 2: needed and undesired
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
        bool hasUndesiredFlags = undesiredFlags == 0 ? false : (m_MemoryProps.memoryTypes[i].propertyFlags & undesiredFlags) == undesiredFlags;

        if (isSupported && hasNeededFlags && !hasUndesiredFlags) {
            memoryTypeInfo.memoryTypeIndex = (MemoryTypeIndexType)i;
            memoryTypeInfo.memoryLocation = memoryLocation;

            return true;
        }
    }

    // Phase 3: needed and desired
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;
        bool hasDesiredFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & desiredFlags) == desiredFlags;

        if (isSupported && hasNeededFlags && hasDesiredFlags) {
            memoryTypeInfo.memoryTypeIndex = (MemoryTypeIndexType)i;
            memoryTypeInfo.memoryLocation = memoryLocation;

            return true;
        }
    }

    // Phase 4: only needed
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        bool isSupported = memoryTypeMask & (1 << i);
        bool hasNeededFlags = (m_MemoryProps.memoryTypes[i].propertyFlags & neededFlags) == neededFlags;

        if (isSupported && hasNeededFlags) {
            memoryTypeInfo.memoryTypeIndex = (MemoryTypeIndexType)i;
            memoryTypeInfo.memoryLocation = memoryLocation;

            return true;
        }
    }

    return false;
}

bool DeviceVK::GetMemoryTypeByIndex(uint32_t index, MemoryTypeInfo& memoryTypeInfo) const {
    if (index >= m_MemoryProps.memoryTypeCount)
        return false;

    const VkMemoryType& memoryType = m_MemoryProps.memoryTypes[index];
    bool isHostVisible = memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    bool isDevice = memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    memoryTypeInfo.memoryTypeIndex = (MemoryTypeIndexType)index;
    if (isDevice)
        memoryTypeInfo.memoryLocation = isHostVisible ? MemoryLocation::DEVICE_UPLOAD : MemoryLocation::DEVICE;
    else
        memoryTypeInfo.memoryLocation = MemoryLocation::HOST_UPLOAD;

    return true;
}

const char* GetObjectTypeName(VkObjectType objectType) {
    switch (objectType) {
        case VK_OBJECT_TYPE_INSTANCE:
            return "VkInstance";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "VkPhysicalDevice";
        case VK_OBJECT_TYPE_DEVICE:
            return "VkDevice";
        case VK_OBJECT_TYPE_QUEUE:
            return "VkQueue";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "VkSemaphore";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "VkCommandBuffer";
        case VK_OBJECT_TYPE_FENCE:
            return "VkFence";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "VkDeviceMemory";
        case VK_OBJECT_TYPE_BUFFER:
            return "VkBuffer";
        case VK_OBJECT_TYPE_IMAGE:
            return "VkImage";
        case VK_OBJECT_TYPE_EVENT:
            return "VkEvent";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "VkQueryPool";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "VkBufferView";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "VkImageView";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "VkShaderModule";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "VkPipelineCache";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "VkPipelineLayout";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "VkRenderPass";
        case VK_OBJECT_TYPE_PIPELINE:
            return "VkPipeline";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "VkDescriptorSetLayout";
        case VK_OBJECT_TYPE_SAMPLER:
            return "VkSampler";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "VkDescriptorPool";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "VkDescriptorSet";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "VkFramebuffer";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "VkCommandPool";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
            return "VkSamplerYcbcrConversion";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
            return "VkDescriptorUpdateTemplate";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "VkSurfaceKHR";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "VkSwapchainKHR";
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return "VkDisplayKHR";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return "VkDisplayModeKHR";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return "VkDebugReportCallbackEXT";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            return "VkDebugUtilsMessengerEXT";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "VkAccelerationStructureKHR";
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
            return "VkValidationCacheEXT";
        case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
            return "VkDeferredOperationKHR";
        default:
            return "unknown";
    }
}

VkBool32 VKAPI_PTR DebugUtilsMessenger(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    MaybeUnused(messageType);

    /*
    // TODO: some messages can be muted here
    if (callbackData->messageIdNumber == XXX)
        return VK_FALSE;
    */

    Message severity = Message::TYPE_INFO;
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = Message::TYPE_ERROR;
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = Message::TYPE_WARNING;

    DeviceVK& device = *(DeviceVK*)userData;
    device.ReportMessage(severity, __FILE__, __LINE__, "%s", callbackData->pMessage);

    return VK_FALSE;
}

Result DeviceVK::CreateInstance(bool enableAPIValidation, const Vector<const char*>& desiredInstanceExts) {
    Vector<const char*> layers(GetStdAllocator());
    if (enableAPIValidation)
        layers.push_back("VK_LAYER_KHRONOS_validation");

    FilterInstanceLayers(layers);

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.apiVersion = VK_API_VERSION_1_3;

    const VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    };

    VkValidationFeaturesEXT validationFeatures = {VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
    validationFeatures.enabledValidationFeatureCount = GetCountOf(enabledValidationFeatures);
    validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

    VkInstanceCreateInfo info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pNext = enableAPIValidation ? &validationFeatures : nullptr;
#ifdef __APPLE__
    info.flags = (VkInstanceCreateFlags)VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
    info.pApplicationInfo = &appInfo;
    info.enabledLayerCount = (uint32_t)layers.size();
    info.ppEnabledLayerNames = layers.data();
    info.enabledExtensionCount = (uint32_t)desiredInstanceExts.size();
    info.ppEnabledExtensionNames = desiredInstanceExts.data();

    VkResult result = m_VK.CreateInstance(&info, m_AllocationCallbackPtr, &m_Instance);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkCreateInstance returned %d", (int32_t)result);

    if (enableAPIValidation) {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        createInfo.pUserData = this;
        createInfo.pfnUserCallback = DebugUtilsMessenger;

        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        createInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
            (PFN_vkCreateDebugUtilsMessengerEXT)m_VK.GetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
        result = vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, m_AllocationCallbackPtr, &m_Messenger);

        RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkCreateDebugUtilsMessengerEXT returned %d", (int32_t)result);
    }

    return Result::SUCCESS;
}

void DeviceVK::FillFamilyIndices(bool useEnabledFamilyIndices, const uint32_t* enabledFamilyIndices, uint32_t familyIndexNum) {
    uint32_t familyNum = 0;
    m_VK.GetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &familyNum, nullptr);

    Vector<VkQueueFamilyProperties> familyProps(familyNum, GetStdAllocator());
    m_VK.GetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &familyNum, familyProps.data());

    memset(m_FamilyIndices.data(), INVALID_FAMILY_INDEX, m_FamilyIndices.size() * sizeof(uint32_t));
    std::array<uint8_t, (uint32_t)CommandQueueType::MAX_NUM> scores = {};

    for (uint32_t i = 0; i < familyProps.size(); i++) {
        if (useEnabledFamilyIndices) {
            bool isFamilyEnabled = false;
            for (uint32_t j = 0; j < familyIndexNum && !isFamilyEnabled; j++)
                isFamilyEnabled = enabledFamilyIndices[j] == i;

            if (!isFamilyEnabled)
                continue;
        }

        VkQueueFlags flags = familyProps[i].queueFlags;
        uint8_t score;

        bool graphics = flags & VK_QUEUE_GRAPHICS_BIT;
        bool compute = flags & VK_QUEUE_COMPUTE_BIT;
        bool copy = flags & VK_QUEUE_TRANSFER_BIT;
        bool sparse = flags & VK_QUEUE_SPARSE_BINDING_BIT;
        bool protect = flags & VK_QUEUE_PROTECTED_BIT;
        bool video = flags & (VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_VIDEO_ENCODE_BIT_KHR);
        bool opticalFlow = flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV;
        bool taken = false;

        // Scores
        score = (graphics ? 100 : 0) + (compute ? 10 : 0) + (copy ? 10 : 0) + (sparse ? 5 : 0) + (opticalFlow ? 2 : 0) + (video ? 1 : 0) + (protect ? 1 : 0);
        if (!taken && graphics && score > scores[(uint32_t)CommandQueueType::GRAPHICS]) {
            m_FamilyIndices[(uint32_t)CommandQueueType::GRAPHICS] = i;
            scores[(uint32_t)CommandQueueType::GRAPHICS] = score;
            taken = true;
        }

        score = (!graphics ? 10 : 0) + (compute ? 100 : 0) + (!copy ? 10 : 0) + (sparse ? 5 : 0) + (opticalFlow ? 2 : 0) + (video ? 1 : 0) + (protect ? 1 : 0);
        if (!taken && compute && score > scores[(uint32_t)CommandQueueType::COMPUTE]) {
            m_FamilyIndices[(uint32_t)CommandQueueType::COMPUTE] = i;
            scores[(uint32_t)CommandQueueType::COMPUTE] = score;
            taken = true;
        }

        score = (!graphics ? 10 : 0) + (!compute ? 10 : 0) + (copy ? 100 : 0) + (sparse ? 5 : 0) + (opticalFlow ? 2 : 0) + (video ? 1 : 0) + (protect ? 1 : 0);
        if (!taken && copy && score > scores[(uint32_t)CommandQueueType::COPY]) {
            m_FamilyIndices[(uint32_t)CommandQueueType::COPY] = i;
            scores[(uint32_t)CommandQueueType::COPY] = score;
            taken = true;
        }
    }
}

void DeviceVK::CreateCommandQueues() {
    for (uint32_t i = 0; i < m_FamilyIndices.size(); i++) {
        if (m_FamilyIndices[i] == INVALID_FAMILY_INDEX)
            continue;

        VkQueue handle = VK_NULL_HANDLE;
        m_VK.GetDeviceQueue(m_Device, m_FamilyIndices[i], 0, &handle);

        m_Queues[i] = Allocate<CommandQueueVK>(GetStdAllocator(), *this, handle, m_FamilyIndices[i], (CommandQueueType)i);
        m_ConcurrentSharingModeQueueIndices.push_back(m_FamilyIndices[i]);
    }
}

void DeviceVK::SetDebugNameToTrivialObject(VkObjectType objectType, uint64_t handle, const char* name) {
    if (!m_VK.SetDebugUtilsObjectNameEXT)
        return;

    VkDebugUtilsObjectNameInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr, objectType, (uint64_t)handle, name};

    VkResult result = m_VK.SetDebugUtilsObjectNameEXT(m_Device, &info);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, ReturnVoid(), "vkSetDebugUtilsObjectNameEXT returned %d", (int32_t)result);
}

void DeviceVK::ReportDeviceGroupInfo() {
    String text(GetStdAllocator());

    REPORT_INFO(this, "Memory heaps:");
    for (uint32_t i = 0; i < m_MemoryProps.memoryHeapCount; i++) {
        text.clear();

        if (m_MemoryProps.memoryHeaps[i].flags == 0)
            text += "*SYSMEM* ";
        if (m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            text += "DEVICE_LOCAL_BIT ";
        if (m_MemoryProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
            text += "MULTI_INSTANCE_BIT ";

        double size = double(m_MemoryProps.memoryHeaps[i].size) / (1024.0 * 1024.0);
        REPORT_INFO(this, "  Heap #%u: %.f Mb - %s", i, size, text.c_str());
    }

    REPORT_INFO(this, "Memory types:");
    for (uint32_t i = 0; i < m_MemoryProps.memoryTypeCount; i++) {
        text.clear();

        REPORT_INFO(this, "  Memory type #%u", i);
        REPORT_INFO(this, "    Heap #%u", m_MemoryProps.memoryTypes[i].heapIndex);

        VkMemoryPropertyFlags flags = m_MemoryProps.memoryTypes[i].propertyFlags;
        if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            text += "DEVICE_LOCAL_BIT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            text += "HOST_VISIBLE_BIT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            text += "HOST_COHERENT_BIT ";
        if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            text += "HOST_CACHED_BIT ";
        if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
            text += "LAZILY_ALLOCATED_BIT ";
        if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
            text += "PROTECTED_BIT ";
        if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
            text += "DEVICE_COHERENT_BIT_AMD ";
        if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
            text += "DEVICE_UNCACHED_BIT_AMD ";
        if (flags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
            text += "RDMA_CAPABLE_BIT_NV ";

        if (!text.empty())
            REPORT_INFO(this, "    %s", text.c_str());
    }
}

#define MERGE_TOKENS2(a, b) a##b
#define MERGE_TOKENS3(a, b, c) a##b##c

#define GET_DEVICE_OPTIONAL_CORE_OR_KHR_PROC(name) \
    m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
    if (!m_VK.name) { \
        m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS3(vk, name, KHR))); \
    }

#define GET_DEVICE_CORE_OR_KHR_PROC(name) \
    GET_DEVICE_OPTIONAL_CORE_OR_KHR_PROC(name) \
    if (!m_VK.name) { \
        REPORT_ERROR(this, "Failed to get device function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        return Result::UNSUPPORTED; \
    }

#define GET_DEVICE_PROC(name) \
    m_VK.name = (PFN_vk##name)m_VK.GetDeviceProcAddr(m_Device, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
    if (!m_VK.name) { \
        REPORT_ERROR(this, "Failed to get device function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        return Result::UNSUPPORTED; \
    }

#define GET_INSTANCE_PROC(name) \
    m_VK.name = (PFN_vk##name)m_VK.GetInstanceProcAddr(m_Instance, NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
    if (!m_VK.name) { \
        REPORT_ERROR(this, "Failed to get instance function: '%s'", NRI_STRINGIFY(MERGE_TOKENS2(vk, name))); \
        return Result::UNSUPPORTED; \
    }

Result DeviceVK::ResolvePreInstanceDispatchTable() {
    m_VK = {};

    m_VK.GetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)GetSharedLibraryFunction(*m_Loader, "vkGetInstanceProcAddr");
    if (!m_VK.GetInstanceProcAddr) {
        REPORT_ERROR(this, "Failed to get 'vkGetInstanceProcAddr'");
        return Result::UNSUPPORTED;
    }

    GET_INSTANCE_PROC(CreateInstance);
    GET_INSTANCE_PROC(EnumerateInstanceExtensionProperties);
    GET_INSTANCE_PROC(EnumerateInstanceLayerProperties);

    return Result::SUCCESS;
}

Result DeviceVK::ResolveInstanceDispatchTable(const Vector<const char*>& desiredInstanceExts) {
    GET_INSTANCE_PROC(GetDeviceProcAddr);
    GET_INSTANCE_PROC(DestroyInstance);
    GET_INSTANCE_PROC(DestroyDevice);
    GET_INSTANCE_PROC(GetPhysicalDeviceMemoryProperties);
    GET_INSTANCE_PROC(GetDeviceGroupPeerMemoryFeatures);
    GET_INSTANCE_PROC(GetPhysicalDeviceFormatProperties);
    GET_INSTANCE_PROC(CreateDevice);
    GET_INSTANCE_PROC(GetDeviceQueue);
    GET_INSTANCE_PROC(EnumeratePhysicalDeviceGroups);
    GET_INSTANCE_PROC(GetPhysicalDeviceProperties2);
    GET_INSTANCE_PROC(GetPhysicalDeviceFeatures2);
    GET_INSTANCE_PROC(GetPhysicalDeviceQueueFamilyProperties);
    GET_INSTANCE_PROC(EnumerateDeviceExtensionProperties);

    if (IsExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_PROC(SetDebugUtilsObjectNameEXT);
        GET_INSTANCE_PROC(CmdBeginDebugUtilsLabelEXT);
        GET_INSTANCE_PROC(CmdEndDebugUtilsLabelEXT);
    }

    if (IsExtensionSupported(VK_KHR_SURFACE_EXTENSION_NAME, desiredInstanceExts)) {
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceFormatsKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceSupportKHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfaceCapabilities2KHR);
        GET_INSTANCE_PROC(GetPhysicalDeviceSurfacePresentModesKHR);
        GET_INSTANCE_PROC(DestroySurfaceKHR);

#if VK_USE_PLATFORM_WIN32_KHR
        GET_INSTANCE_PROC(CreateWin32SurfaceKHR);
        GET_INSTANCE_PROC(GetMemoryWin32HandlePropertiesKHR);
#endif
#if VK_USE_PLATFORM_METAL_EXT
        GET_INSTANCE_PROC(CreateMetalSurfaceEXT);
#endif
#if VK_USE_PLATFORM_XLIB_KHR
        GET_INSTANCE_PROC(CreateXlibSurfaceKHR);
#endif
#if VK_USE_PLATFORM_WAYLAND_KHR
        GET_INSTANCE_PROC(CreateWaylandSurfaceKHR);
#endif
    }

    return Result::SUCCESS;
}

Result DeviceVK::ResolveDispatchTable(const Vector<const char*>& desiredDeviceExts) {
    GET_DEVICE_CORE_OR_KHR_PROC(CreateBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateImage);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateBufferView);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateImageView);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateSampler);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateFramebuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateQueryPool);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateCommandPool);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateSemaphore);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateDescriptorPool);
    GET_DEVICE_CORE_OR_KHR_PROC(CreatePipelineLayout);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateDescriptorSetLayout);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateShaderModule);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateGraphicsPipelines);
    GET_DEVICE_CORE_OR_KHR_PROC(CreateComputePipelines);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyImage);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyBufferView);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyImageView);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroySampler);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyFramebuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyQueryPool);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyCommandPool);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroySemaphore);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyDescriptorPool);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyPipelineLayout);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyDescriptorSetLayout);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyShaderModule);
    GET_DEVICE_CORE_OR_KHR_PROC(DestroyPipeline);
    GET_DEVICE_CORE_OR_KHR_PROC(AllocateMemory);
    GET_DEVICE_CORE_OR_KHR_PROC(MapMemory);
    GET_DEVICE_CORE_OR_KHR_PROC(UnmapMemory);
    GET_DEVICE_CORE_OR_KHR_PROC(FreeMemory);
    GET_DEVICE_CORE_OR_KHR_PROC(QueueWaitIdle);
    GET_DEVICE_CORE_OR_KHR_PROC(QueueSubmit2);
    GET_DEVICE_CORE_OR_KHR_PROC(GetSemaphoreCounterValue);
    GET_DEVICE_CORE_OR_KHR_PROC(WaitSemaphores);
    GET_DEVICE_CORE_OR_KHR_PROC(ResetCommandPool);
    GET_DEVICE_CORE_OR_KHR_PROC(ResetDescriptorPool);
    GET_DEVICE_CORE_OR_KHR_PROC(AllocateCommandBuffers);
    GET_DEVICE_CORE_OR_KHR_PROC(AllocateDescriptorSets);
    GET_DEVICE_CORE_OR_KHR_PROC(FreeCommandBuffers);
    GET_DEVICE_CORE_OR_KHR_PROC(FreeDescriptorSets);
    GET_DEVICE_CORE_OR_KHR_PROC(UpdateDescriptorSets);
    GET_DEVICE_CORE_OR_KHR_PROC(BindBufferMemory2);
    GET_DEVICE_CORE_OR_KHR_PROC(BindImageMemory2);
    GET_DEVICE_CORE_OR_KHR_PROC(GetBufferMemoryRequirements2);
    GET_DEVICE_CORE_OR_KHR_PROC(GetImageMemoryRequirements2);
    GET_DEVICE_CORE_OR_KHR_PROC(BeginCommandBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdSetViewport);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdSetScissor);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdSetDepthBounds);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdSetStencilReference);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdSetBlendConstants);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdClearAttachments);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdClearColorImage);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdBindVertexBuffers);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdBindIndexBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdBindPipeline);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdBindDescriptorSets);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdPushConstants);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDispatch);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDispatchIndirect);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDraw);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDrawIndexed);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDrawIndirect);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDrawIndirectCount);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDrawIndexedIndirect);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdDrawIndexedIndirectCount);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdCopyBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdCopyImage);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdCopyBufferToImage);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdCopyImageToBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdPipelineBarrier2);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdBeginQuery);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdEndQuery);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdWriteTimestamp);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdCopyQueryPoolResults);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdResetQueryPool);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdFillBuffer);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdBeginRendering);
    GET_DEVICE_CORE_OR_KHR_PROC(CmdEndRendering);
    GET_DEVICE_CORE_OR_KHR_PROC(EndCommandBuffer);

    GET_DEVICE_OPTIONAL_CORE_OR_KHR_PROC(GetBufferDeviceAddress);
    if (!m_VK.GetBufferDeviceAddress)
        m_IsDeviceAddressSupported = false;

    // IMPORTANT: {} is mandatory here!

    if (IsExtensionSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(AcquireNextImageKHR);
        GET_DEVICE_PROC(QueuePresentKHR);
        GET_DEVICE_PROC(CreateSwapchainKHR);
        GET_DEVICE_PROC(DestroySwapchainKHR);
        GET_DEVICE_PROC(GetSwapchainImagesKHR);
    }

    if (IsExtensionSupported(VK_KHR_PRESENT_WAIT_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(WaitForPresentKHR);
    }

    if (IsExtensionSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CreateAccelerationStructureKHR);
        GET_DEVICE_PROC(DestroyAccelerationStructureKHR);
        GET_DEVICE_PROC(GetAccelerationStructureDeviceAddressKHR);
        GET_DEVICE_PROC(GetAccelerationStructureBuildSizesKHR);
        GET_DEVICE_PROC(CmdBuildAccelerationStructuresKHR);
        GET_DEVICE_PROC(CmdCopyAccelerationStructureKHR);
        GET_DEVICE_PROC(CmdWriteAccelerationStructuresPropertiesKHR);
    }

    if (IsExtensionSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CreateRayTracingPipelinesKHR);
        GET_DEVICE_PROC(GetRayTracingShaderGroupHandlesKHR);
        GET_DEVICE_PROC(CmdTraceRaysKHR);
        GET_DEVICE_PROC(CmdTraceRaysIndirect2KHR);
    }

    if (IsExtensionSupported(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdSetSampleLocationsEXT);
    }

    if (IsExtensionSupported(VK_EXT_MESH_SHADER_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(CmdDrawMeshTasksEXT);
        GET_DEVICE_PROC(CmdDrawMeshTasksIndirectEXT);
    }

    if (IsExtensionSupported(VK_NV_LOW_LATENCY_2_EXTENSION_NAME, desiredDeviceExts)) {
        GET_DEVICE_PROC(GetLatencyTimingsNV);
        GET_DEVICE_PROC(LatencySleepNV);
        GET_DEVICE_PROC(SetLatencyMarkerNV);
        GET_DEVICE_PROC(SetLatencySleepModeNV);
    }

    return Result::SUCCESS;
}

void DeviceVK::Destroy() {
    Deallocate(GetStdAllocator(), this);
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DeviceVK::SetDebugName(const char* name) {
    SetDebugNameToTrivialObject(VK_OBJECT_TYPE_DEVICE, (uint64_t)m_Device, name);
}

inline Result DeviceVK::GetCommandQueue(CommandQueueType commandQueueType, CommandQueue*& commandQueue) {
    ExclusiveScope lock(m_Lock);

    if (m_FamilyIndices[(uint32_t)commandQueueType] == INVALID_FAMILY_INDEX) {
        REPORT_WARNING(this, "%s command queue is not supported by the device!",
            commandQueueType == CommandQueueType::GRAPHICS ? "GRAPHICS" : (commandQueueType == CommandQueueType::COMPUTE ? "COMPUTE" : "COPY"));

        return Result::UNSUPPORTED;
    }

    commandQueue = (CommandQueue*)m_Queues[(uint32_t)commandQueueType];

    return Result::SUCCESS;
}

inline Result DeviceVK::CreateCommandAllocator(const CommandQueue& commandQueue, CommandAllocator*& commandAllocator) {
    return CreateImplementation<CommandAllocatorVK>(commandAllocator, commandQueue);
}

inline Result DeviceVK::CreateDescriptorPool(const DescriptorPoolDesc& descriptorPoolDesc, DescriptorPool*& descriptorPool) {
    return CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolDesc);
}

Result DeviceVK::CreateBuffer(const BufferDesc& bufferDesc, Buffer*& buffer) { // TODO: not inline
    return CreateImplementation<BufferVK>(buffer, bufferDesc);
}

inline Result DeviceVK::CreateTexture(const TextureDesc& textureDesc, Texture*& texture) {
    return CreateImplementation<TextureVK>(texture, textureDesc);
}

inline Result DeviceVK::CreateBufferView(const BufferViewDesc& bufferViewDesc, Descriptor*& bufferView) {
    return CreateImplementation<DescriptorVK>(bufferView, bufferViewDesc);
}

inline Result DeviceVK::CreateTexture1DView(const Texture1DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

inline Result DeviceVK::CreateTexture2DView(const Texture2DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

inline Result DeviceVK::CreateTexture3DView(const Texture3DViewDesc& textureViewDesc, Descriptor*& textureView) {
    return CreateImplementation<DescriptorVK>(textureView, textureViewDesc);
}

inline Result DeviceVK::CreateSampler(const SamplerDesc& samplerDesc, Descriptor*& sampler) {
    return CreateImplementation<DescriptorVK>(sampler, samplerDesc);
}

inline Result DeviceVK::CreatePipelineLayout(const PipelineLayoutDesc& pipelineLayoutDesc, PipelineLayout*& pipelineLayout) {
    return CreateImplementation<PipelineLayoutVK>(pipelineLayout, pipelineLayoutDesc);
}

inline Result DeviceVK::CreatePipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, Pipeline*& pipeline) {
    return CreateImplementation<PipelineVK>(pipeline, graphicsPipelineDesc);
}

inline Result DeviceVK::CreatePipeline(const ComputePipelineDesc& computePipelineDesc, Pipeline*& pipeline) {
    return CreateImplementation<PipelineVK>(pipeline, computePipelineDesc);
}

inline Result DeviceVK::CreateQueryPool(const QueryPoolDesc& queryPoolDesc, QueryPool*& queryPool) {
    return CreateImplementation<QueryPoolVK>(queryPool, queryPoolDesc);
}

inline Result DeviceVK::CreateFence(uint64_t initialValue, Fence*& fence) {
    return CreateImplementation<FenceVK>(fence, initialValue);
}

inline Result DeviceVK::CreateSwapChain(const SwapChainDesc& swapChainDesc, SwapChain*& swapChain) {
    return CreateImplementation<SwapChainVK>(swapChain, swapChainDesc);
}

inline Result DeviceVK::CreatePipeline(const RayTracingPipelineDesc& rayTracingPipelineDesc, Pipeline*& pipeline) {
    return CreateImplementation<PipelineVK>(pipeline, rayTracingPipelineDesc);
}

inline Result DeviceVK::CreateAccelerationStructure(const AccelerationStructureDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

inline Result DeviceVK::CreateCommandQueue(const CommandQueueVKDesc& commandQueueVKDesc, CommandQueue*& commandQueue) {
    ExclusiveScope lock(m_Lock);

    const uint32_t commandQueueTypeIndex = (uint32_t)commandQueueVKDesc.commandQueueType;
    const bool isFamilyIndexSame = m_FamilyIndices[commandQueueTypeIndex] == commandQueueVKDesc.familyIndex;
    const bool isQueueSame = (VkQueue)m_Queues[commandQueueTypeIndex] == (VkQueue)commandQueueVKDesc.vkQueue;
    if (isFamilyIndexSame && isQueueSame) {
        commandQueue = (CommandQueue*)m_Queues[commandQueueTypeIndex];
        return Result::SUCCESS;
    }

    CreateImplementation<CommandQueueVK>(commandQueue, commandQueueVKDesc);
    Deallocate(GetStdAllocator(), m_Queues[commandQueueTypeIndex]);

    m_FamilyIndices[commandQueueTypeIndex] = commandQueueVKDesc.familyIndex;
    m_Queues[commandQueueTypeIndex] = (CommandQueueVK*)commandQueue;

    return Result::SUCCESS;
}

inline Result DeviceVK::CreateCommandAllocator(const CommandAllocatorVKDesc& commandAllocatorVKDesc, CommandAllocator*& commandAllocator) {
    return CreateImplementation<CommandAllocatorVK>(commandAllocator, commandAllocatorVKDesc);
}

inline Result DeviceVK::CreateCommandBuffer(const CommandBufferVKDesc& commandBufferVKDesc, CommandBuffer*& commandBuffer) {
    return CreateImplementation<CommandBufferVK>(commandBuffer, commandBufferVKDesc);
}

inline Result DeviceVK::CreateDescriptorPool(const DescriptorPoolVKDesc& descriptorPoolVKDesc, DescriptorPool*& descriptorPool) {
    return CreateImplementation<DescriptorPoolVK>(descriptorPool, descriptorPoolVKDesc);
}

inline Result DeviceVK::CreateBuffer(const BufferVKDesc& bufferDesc, Buffer*& buffer) {
    return CreateImplementation<BufferVK>(buffer, bufferDesc);
}

inline Result DeviceVK::CreateTexture(const TextureVKDesc& textureVKDesc, Texture*& texture) {
    return CreateImplementation<TextureVK>(texture, textureVKDesc);
}

inline Result DeviceVK::CreateMemory(const MemoryVKDesc& memoryVKDesc, Memory*& memory) {
    return CreateImplementation<MemoryVK>(memory, memoryVKDesc);
}

inline Result DeviceVK::CreateGraphicsPipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline) {
    PipelineVK* implementation = Allocate<PipelineVK>(GetStdAllocator(), *this);
    const Result result = implementation->CreateGraphics(vkPipeline);

    if (result != Result::SUCCESS) {
        pipeline = (Pipeline*)implementation;
        return result;
    }

    Deallocate(GetStdAllocator(), implementation);

    return result;
}

inline Result DeviceVK::CreateComputePipeline(NRIVkPipeline vkPipeline, Pipeline*& pipeline) {
    PipelineVK* implementation = Allocate<PipelineVK>(GetStdAllocator(), *this);
    const Result result = implementation->CreateCompute(vkPipeline);

    if (result != Result::SUCCESS) {
        pipeline = (Pipeline*)implementation;
        return result;
    }

    Deallocate(GetStdAllocator(), implementation);

    return result;
}

inline Result DeviceVK::CreateQueryPool(const QueryPoolVKDesc& queryPoolVKDesc, QueryPool*& queryPool) {
    return CreateImplementation<QueryPoolVK>(queryPool, queryPoolVKDesc);
}

inline Result DeviceVK::CreateAccelerationStructure(const AccelerationStructureVKDesc& accelerationStructureDesc, AccelerationStructure*& accelerationStructure) {
    return CreateImplementation<AccelerationStructureVK>(accelerationStructure, accelerationStructureDesc);
}

inline void DeviceVK::DestroyCommandAllocator(CommandAllocator& commandAllocator) {
    Deallocate(GetStdAllocator(), (CommandAllocatorVK*)&commandAllocator);
}

inline void DeviceVK::DestroyDescriptorPool(DescriptorPool& descriptorPool) {
    Deallocate(GetStdAllocator(), (DescriptorPoolVK*)&descriptorPool);
}

void DeviceVK::DestroyBuffer(Buffer& buffer) { // TODO: not inline
    Deallocate(GetStdAllocator(), (BufferVK*)&buffer);
}

inline void DeviceVK::DestroyTexture(Texture& texture) {
    Deallocate(GetStdAllocator(), (TextureVK*)&texture);
}

inline void DeviceVK::DestroyDescriptor(Descriptor& descriptor) {
    Deallocate(GetStdAllocator(), (DescriptorVK*)&descriptor);
}

inline void DeviceVK::DestroyPipelineLayout(PipelineLayout& pipelineLayout) {
    Deallocate(GetStdAllocator(), (PipelineLayoutVK*)&pipelineLayout);
}

inline void DeviceVK::DestroyPipeline(Pipeline& pipeline) {
    Deallocate(GetStdAllocator(), (PipelineVK*)&pipeline);
}

inline void DeviceVK::DestroyQueryPool(QueryPool& queryPool) {
    Deallocate(GetStdAllocator(), (QueryPoolVK*)&queryPool);
}

inline void DeviceVK::DestroyFence(Fence& fence) {
    Deallocate(GetStdAllocator(), (FenceVK*)&fence);
}

inline void DeviceVK::DestroySwapChain(SwapChain& swapChain) {
    Deallocate(GetStdAllocator(), (SwapChainVK*)&swapChain);
}

inline void DeviceVK::DestroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    Deallocate(GetStdAllocator(), (AccelerationStructureVK*)&accelerationStructure);
}

inline Result DeviceVK::AllocateMemory(MemoryType memoryType, uint64_t size, Memory*& memory) {
    return CreateImplementation<MemoryVK>(memory, memoryType, size);
}

inline Result DeviceVK::BindBufferMemory(const BufferMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (memoryBindingDescNum == 0)
        return Result::SUCCESS;

    VkBindBufferMemoryInfo* infos = STACK_ALLOC(VkBindBufferMemoryInfo, memoryBindingDescNum);
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const BufferMemoryBindingDesc& bindingDesc = memoryBindingDescs[i];

        MemoryVK& memoryImpl = *(MemoryVK*)bindingDesc.memory;
        BufferVK& bufferImpl = *(BufferVK*)bindingDesc.buffer;

        const MemoryTypeUnpack unpack = {memoryImpl.GetType()};
        const MemoryTypeInfo& memoryTypeInfo = unpack.info;

        if (memoryTypeInfo.isDedicated == 1)
            memoryImpl.CreateDedicated(bufferImpl);

        VkBindBufferMemoryInfo& info = infos[i];
        info = {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
        info.buffer = bufferImpl.GetHandle();
        info.memory = memoryImpl.GetHandle();
        info.memoryOffset = bindingDesc.offset;

        if (IsHostVisibleMemory(memoryTypeInfo.memoryLocation))
            bufferImpl.SetHostMemory(memoryImpl, info.memoryOffset);
    }

    VkResult result = m_VK.BindBufferMemory2(m_Device, memoryBindingDescNum, infos);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkBindBufferMemory2 returned %d", (int32_t)result);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        BufferVK& bufferImpl = *(BufferVK*)memoryBindingDescs[i].buffer;
        bufferImpl.ReadDeviceAddress();
    }

    return Result::SUCCESS;
}

inline Result DeviceVK::BindTextureMemory(const TextureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (memoryBindingDescNum == 0)
        return Result::SUCCESS;

    VkBindImageMemoryInfo* infos = STACK_ALLOC(VkBindImageMemoryInfo, memoryBindingDescNum);
    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const TextureMemoryBindingDesc& bindingDesc = memoryBindingDescs[i];

        MemoryVK& memoryImpl = *(MemoryVK*)bindingDesc.memory;
        TextureVK& textureImpl = *(TextureVK*)bindingDesc.texture;

        const MemoryTypeUnpack unpack = {memoryImpl.GetType()};
        const MemoryTypeInfo& memoryTypeInfo = unpack.info;

        if (memoryTypeInfo.isDedicated)
            memoryImpl.CreateDedicated(textureImpl);

        VkBindImageMemoryInfo& info = infos[i];
        info = {VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
        info.image = textureImpl.GetHandle();
        info.memory = memoryImpl.GetHandle();
        info.memoryOffset = bindingDesc.offset;
    }

    VkResult result = m_VK.BindImageMemory2(m_Device, memoryBindingDescNum, infos);
    RETURN_ON_FAILURE(this, result == VK_SUCCESS, GetReturnCode(result), "vkBindImageMemory2 returned %d", (int32_t)result);

    return Result::SUCCESS;
}

inline Result DeviceVK::BindAccelerationStructureMemory(const AccelerationStructureMemoryBindingDesc* memoryBindingDescs, uint32_t memoryBindingDescNum) {
    if (memoryBindingDescNum == 0)
        return Result::SUCCESS;

    BufferMemoryBindingDesc* infos = ALLOCATE_SCRATCH(*this, BufferMemoryBindingDesc, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum; i++) {
        const AccelerationStructureMemoryBindingDesc& bindingDesc = memoryBindingDescs[i];
        AccelerationStructureVK& accelerationStructure = *(AccelerationStructureVK*)bindingDesc.accelerationStructure;

        BufferMemoryBindingDesc& bufferMemoryBinding = infos[i];
        bufferMemoryBinding = {};
        bufferMemoryBinding.buffer = (Buffer*)accelerationStructure.GetBuffer();
        bufferMemoryBinding.memory = bindingDesc.memory;
        bufferMemoryBinding.offset = bindingDesc.offset;
    }

    Result result = BindBufferMemory(infos, memoryBindingDescNum);

    for (uint32_t i = 0; i < memoryBindingDescNum && result == Result::SUCCESS; i++) {
        AccelerationStructureVK& accelerationStructure = *(AccelerationStructureVK*)memoryBindingDescs[i].accelerationStructure;
        result = accelerationStructure.FinishCreation();
    }

    FREE_SCRATCH(*this, infos, memoryBindingDescNum);

    return result;
}

inline void DeviceVK::FreeMemory(Memory& memory) {
    Deallocate(GetStdAllocator(), (MemoryVK*)&memory);
}

inline FormatSupportBits DeviceVK::GetFormatSupport(Format format) const {
    FormatSupportBits mask = FormatSupportBits::UNSUPPORTED;

    const VkFormat vkFormat = GetVkFormat(format);

    VkFormatProperties formatProperties = {};
    m_VK.GetPhysicalDeviceFormatProperties(m_PhysicalDevice, vkFormat, &formatProperties);

#define UPDATE_SUPPORT_BITS(required, bit) \
    if ((formatProperties.optimalTilingFeatures & (required)) == (required)) \
        mask |= bit;

    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, FormatSupportBits::TEXTURE);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT, FormatSupportBits::STORAGE_TEXTURE);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, FormatSupportBits::COLOR_ATTACHMENT);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, FormatSupportBits::DEPTH_STENCIL_ATTACHMENT);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT, FormatSupportBits::BLEND);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT, FormatSupportBits::STORAGE_TEXTURE_ATOMICS);

#undef UPDATE_SUPPORT_BITS

#define UPDATE_SUPPORT_BITS(required, bit) \
    if ((formatProperties.bufferFeatures & (required)) == (required)) \
        mask |= bit;

    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT, FormatSupportBits::BUFFER);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT, FormatSupportBits::STORAGE_BUFFER);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, FormatSupportBits::VERTEX_BUFFER);
    UPDATE_SUPPORT_BITS(VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT, FormatSupportBits::STORAGE_BUFFER_ATOMICS);

#undef UPDATE_SUPPORT_BITS

    return mask;
}

inline uint32_t DeviceVK::CalculateAllocationNumber(const ResourceGroupDesc& resourceGroupDesc) const {
    HelperDeviceMemoryAllocator allocator(m_CoreInterface, (Device&)*this);

    return allocator.CalculateAllocationNumber(resourceGroupDesc);
}

inline Result DeviceVK::AllocateAndBindMemory(const ResourceGroupDesc& resourceGroupDesc, Memory** allocations) {
    HelperDeviceMemoryAllocator allocator(m_CoreInterface, (Device&)*this);

    return allocator.AllocateAndBindMemory(resourceGroupDesc, allocations);
}

#include "DeviceVK.hpp"
