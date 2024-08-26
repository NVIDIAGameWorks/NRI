// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "CommandQueueVK.h"
#include "FenceVK.h"
#include "SwapChainVK.h"
#include "TextureVK.h"

using namespace nri;

static std::array<VkFormat, (size_t)SwapChainFormat::MAX_NUM> g_swapChainFormat = {
    VK_FORMAT_R16G16B16A16_SFLOAT, // BT709_G10_16BIT
#ifdef __APPLE__
    VK_FORMAT_B8G8R8A8_UNORM, // BT709_G22_8BIT
#else
    VK_FORMAT_R8G8B8A8_UNORM, // BT709_G22_8BIT
#endif
    VK_FORMAT_A2B10G10R10_UNORM_PACK32, // BT709_G22_10BIT
    VK_FORMAT_A2B10G10R10_UNORM_PACK32, // BT2020_G2084_10BIT
};

static std::array<VkColorSpaceKHR, (size_t)SwapChainFormat::MAX_NUM> g_colorSpace = {
    VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT, // BT709_G10_16BIT
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,       // BT709_G22_8BIT
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,       // BT709_G22_10BIT
    VK_COLOR_SPACE_HDR10_ST2084_EXT,         // BT2020_G2084_10BIT
};

SwapChainVK::SwapChainVK(DeviceVK& device)
    : m_Device(device)
    , m_Textures(device.GetStdAllocator()) {
}

SwapChainVK::~SwapChainVK() {
    for (size_t i = 0; i < m_Textures.size(); i++)
        Destroy(m_Device.GetStdAllocator(), m_Textures[i]);

    Destroy(m_Device.GetStdAllocator(), m_LatencyFence);

    const auto& vk = m_Device.GetDispatchTable();
    if (m_Handle)
        vk.DestroySwapchainKHR(m_Device, m_Handle, m_Device.GetAllocationCallbacks());

    if (m_Surface)
        vk.DestroySurfaceKHR(m_Device, m_Surface, m_Device.GetAllocationCallbacks());

    for (VkSemaphore semaphore : m_ImageAcquiredSemaphores) {
        if (semaphore)
            vk.DestroySemaphore(m_Device, semaphore, m_Device.GetAllocationCallbacks());
    }

    for (VkSemaphore semaphore : m_RenderingFinishedSemaphores) {
        if (semaphore)
            vk.DestroySemaphore(m_Device, semaphore, m_Device.GetAllocationCallbacks());
    }
}

Result SwapChainVK::Create(const SwapChainDesc& swapChainDesc) {
    const auto& vk = m_Device.GetDispatchTable();

    m_CommandQueue = (CommandQueueVK*)swapChainDesc.commandQueue;
    uint32_t familyIndex = m_CommandQueue->GetFamilyIndex();

    // Create semaphores
    for (VkSemaphore& semaphore : m_ImageAcquiredSemaphores) {
        VkSemaphoreTypeCreateInfo timelineCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, nullptr, VK_SEMAPHORE_TYPE_BINARY, 0};
        VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &timelineCreateInfo, 0};

        VkResult result = vk.CreateSemaphore((VkDevice)m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &semaphore);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSemaphore returned %d", (int32_t)result);
    }

    for (VkSemaphore& semaphore : m_RenderingFinishedSemaphores) {
        VkSemaphoreTypeCreateInfo timelineCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, nullptr, VK_SEMAPHORE_TYPE_BINARY, 0};
        VkSemaphoreCreateInfo createInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &timelineCreateInfo, 0};

        VkResult result = vk.CreateSemaphore((VkDevice)m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &semaphore);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSemaphore returned %d", (int32_t)result);
    }

    // Create surface
#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (swapChainDesc.window.windows.hwnd) {
        VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        win32SurfaceInfo.hwnd = (HWND)swapChainDesc.window.windows.hwnd;

        VkResult result = vk.CreateWin32SurfaceKHR(m_Device, &win32SurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateWin32SurfaceKHR returned %d", (int32_t)result);
    }
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    if (swapChainDesc.window.metal.caMetalLayer) {
        VkMetalSurfaceCreateInfoEXT metalSurfaceCreateInfo = {VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
        metalSurfaceCreateInfo.pLayer = (CAMetalLayer*)swapChainDesc.window.metal.caMetalLayer;

        VkResult result = vk.CreateMetalSurfaceEXT(m_Device, &metalSurfaceCreateInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateMetalSurfaceEXT returned %d", (int32_t)result);
    }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if (swapChainDesc.window.x11.dpy && swapChainDesc.window.x11.window) {
        VkXlibSurfaceCreateInfoKHR xlibSurfaceInfo = {VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR};
        xlibSurfaceInfo.dpy = (::Display*)swapChainDesc.window.x11.dpy;
        xlibSurfaceInfo.window = (::Window)swapChainDesc.window.x11.window;

        VkResult result = vk.CreateXlibSurfaceKHR(m_Device, &xlibSurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateXlibSurfaceKHR returned %d", (int32_t)result);
    }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if (swapChainDesc.window.wayland.display && swapChainDesc.window.wayland.surface) {
        VkWaylandSurfaceCreateInfoKHR waylandSurfaceInfo = {VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR};
        waylandSurfaceInfo.display = (wl_display*)swapChainDesc.window.wayland.display;
        waylandSurfaceInfo.surface = (wl_surface*)swapChainDesc.window.wayland.surface;

        VkResult result = vk.CreateWaylandSurfaceKHR(m_Device, &waylandSurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateWaylandSurfaceKHR returned %d", (int32_t)result);
    }
#endif

    // Surface caps
    uint32_t textureNum = swapChainDesc.textureNum;
    {
        VkBool32 supported = VK_FALSE;
        VkResult result = vk.GetPhysicalDeviceSurfaceSupportKHR(m_Device, familyIndex, m_Surface, &supported);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS && supported, GetReturnCode(result), "Surface is not supported");

        VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR};
        surfaceInfo.surface = m_Surface;

        VkSurfaceCapabilities2KHR sc = {VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR};

        VkLatencySurfaceCapabilitiesNV latencySurfaceCapabilities = {VK_STRUCTURE_TYPE_LATENCY_SURFACE_CAPABILITIES_NV};
        latencySurfaceCapabilities.presentModeCount = 8;
        latencySurfaceCapabilities.pPresentModes = StackAlloc(VkPresentModeKHR, latencySurfaceCapabilities.presentModeCount);

        if (m_Device.m_IsLowLatencySupported)
            sc.pNext = &latencySurfaceCapabilities;

        result = vk.GetPhysicalDeviceSurfaceCapabilities2KHR(m_Device, &surfaceInfo, &sc);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR returned %d", (int32_t)result);

        bool isWidthValid = swapChainDesc.width >= sc.surfaceCapabilities.minImageExtent.width && swapChainDesc.width <= sc.surfaceCapabilities.maxImageExtent.width;
        RETURN_ON_FAILURE(&m_Device, isWidthValid, Result::INVALID_ARGUMENT, "swapChainDesc.width is out of [%u, %u] range", sc.surfaceCapabilities.minImageExtent.width,
            sc.surfaceCapabilities.maxImageExtent.width);

        bool isHeightValid = swapChainDesc.height >= sc.surfaceCapabilities.minImageExtent.height && swapChainDesc.height <= sc.surfaceCapabilities.maxImageExtent.height;
        RETURN_ON_FAILURE(&m_Device, isHeightValid, Result::INVALID_ARGUMENT, "swapChainDesc.height is out of [%u, %u] range", sc.surfaceCapabilities.minImageExtent.height,
            sc.surfaceCapabilities.maxImageExtent.height);

        bool isTextureNumValid = textureNum >= sc.surfaceCapabilities.minImageCount && (textureNum <= sc.surfaceCapabilities.maxImageCount || sc.surfaceCapabilities.maxImageCount == 0);
        RETURN_ON_FAILURE(&m_Device, isTextureNumValid, Result::INVALID_ARGUMENT, "swapChainDesc.textureNum is out of [%u, %u] range", sc.surfaceCapabilities.minImageCount,
            sc.surfaceCapabilities.maxImageCount);
    }

    // Surface format
    VkSurfaceFormatKHR surfaceFormat = {};
    {
        uint32_t formatNum = 0;
        VkResult result = vk.GetPhysicalDeviceSurfaceFormatsKHR(m_Device, m_Surface, &formatNum, nullptr);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfaceFormatsKHR returned %d", (int32_t)result);

        VkSurfaceFormatKHR* surfaceFormats = StackAlloc(VkSurfaceFormatKHR, formatNum);
        result = vk.GetPhysicalDeviceSurfaceFormatsKHR(m_Device, m_Surface, &formatNum, surfaceFormats);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfaceFormatsKHR returned %d", (int32_t)result);

        VkFormat format = g_swapChainFormat[(uint32_t)swapChainDesc.format];
        VkColorSpaceKHR colorSpace = g_colorSpace[(uint32_t)swapChainDesc.format];

        uint32_t i = 0;
        for (; i < formatNum; i++) {
            if (surfaceFormats[i].format == format && surfaceFormats[i].colorSpace == colorSpace) {
                surfaceFormat = surfaceFormats[i];
                break;
            }
        }
        if (i == formatNum) {
            REPORT_WARNING(&m_Device, "The requested format is not supported. Using 1st surface format from the list");
            surfaceFormat = surfaceFormats[0];
        }
    }

    // Present mode
    bool allowLowLatency = swapChainDesc.allowLowLatency && m_Device.m_IsLowLatencySupported;
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    {
        uint32_t presentModeNum = 8;
        VkPresentModeKHR* presentModes = StackAlloc(VkPresentModeKHR, presentModeNum);
        VkResult result = vk.GetPhysicalDeviceSurfacePresentModesKHR(m_Device, m_Surface, &presentModeNum, presentModes);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfacePresentModesKHR returned %d", (int32_t)result);

        VkPresentModeKHR vsyncOnModes[] = {VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR};
        VkPresentModeKHR vsyncOffModes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR};
        const VkPresentModeKHR* modes = swapChainDesc.verticalSyncInterval ? vsyncOnModes : vsyncOffModes;
        static_assert(GetCountOf(vsyncOnModes) == GetCountOf(vsyncOffModes));
        static_assert(GetCountOf(vsyncOnModes) == 2);

        if (allowLowLatency)
            vsyncOffModes[0] = vsyncOffModes[1]; // dictated by "latencySurfaceCapabilities"

        uint32_t j = 0;
        for (; j < 2; j++) {
            uint32_t i = 0;
            for (; i < presentModeNum; i++) {
                if (modes[j] == presentModes[i]) {
                    presentMode = modes[j];
                    break;
                }
            }
            if (i != presentModeNum)
                break;
            REPORT_WARNING(&m_Device, "VkPresentModeKHR = %u is not supported", modes[j]);
        }
        if (j == 2)
            REPORT_WARNING(&m_Device, "No a suitable present mode found, switching to VK_PRESENT_MODE_IMMEDIATE_KHR");
    }

    { // Swap chain
        VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
        swapchainInfo.flags = m_Device.m_IsSwapChainMutableFormatSupported ? VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR : (VkSwapchainCreateFlagsKHR)0;
        swapchainInfo.surface = m_Surface;
        swapchainInfo.minImageCount = textureNum;
        swapchainInfo.imageFormat = surfaceFormat.format;
        swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = {swapChainDesc.width, swapChainDesc.height};
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 1;
        swapchainInfo.pQueueFamilyIndices = &familyIndex;
        swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.presentMode = presentMode;
        const void** tail = &swapchainInfo.pNext;

        // Mutable formats
        VkFormat mutableFormats[2];
        uint32_t mutableFormatNum = 0;
        mutableFormats[mutableFormatNum++] = surfaceFormat.format;
        switch (surfaceFormat.format) {
            case VK_FORMAT_R8G8B8A8_UNORM:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_R8G8B8A8_SRGB;
                break;
            case VK_FORMAT_R8G8B8A8_SRGB:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_R8G8B8A8_UNORM;
                break;
            case VK_FORMAT_B8G8R8A8_UNORM:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_B8G8R8A8_SRGB;
                break;
            case VK_FORMAT_B8G8R8A8_SRGB:
                mutableFormats[mutableFormatNum++] = VK_FORMAT_B8G8R8A8_UNORM;
                break;
        }

        VkImageFormatListCreateInfo imageFormatListCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO};
        imageFormatListCreateInfo.pViewFormats = mutableFormats;
        imageFormatListCreateInfo.viewFormatCount = mutableFormatNum;

        if (m_Device.m_IsSwapChainMutableFormatSupported) {
            APPEND_EXT(imageFormatListCreateInfo);
        }

        // Low latency mode
        VkSwapchainLatencyCreateInfoNV latencyCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_LATENCY_CREATE_INFO_NV};
        latencyCreateInfo.latencyModeEnable = allowLowLatency;

        if (m_Device.m_IsLowLatencySupported && allowLowLatency) {
            APPEND_EXT(latencyCreateInfo);
        }

        // Create
        VkResult result = vk.CreateSwapchainKHR(m_Device, &swapchainInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSwapchainKHR returned %d", (int32_t)result);
    }

    { // Swap chain images
        uint32_t imageNum = 0;
        vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, nullptr);

        VkImage* imageHandles = StackAlloc(VkImage, imageNum);
        vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, imageHandles);

        m_Textures.resize(imageNum);
        for (uint32_t i = 0; i < imageNum; i++) {
            TextureVKDesc desc = {};
            desc.vkImage = (VKNonDispatchableHandle)imageHandles[i];
            desc.vkFormat = surfaceFormat.format;
            desc.vkImageType = VK_IMAGE_TYPE_2D;
            desc.width = swapChainDesc.width;
            desc.height = swapChainDesc.height;
            desc.depth = 1;
            desc.mipNum = 1;
            desc.layerNum = 1;
            desc.sampleNum = 1;

            TextureVK* texture = Allocate<TextureVK>(m_Device.GetStdAllocator(), m_Device);
            texture->Create(desc);

            m_Textures[i] = texture;
        }
    }

    // Latency fence
    if (allowLowLatency) {
        m_LatencyFence = Allocate<FenceVK>(m_Device.GetStdAllocator(), m_Device);
        m_LatencyFence->Create(0);
    }

    // Finalize
    m_PresentId = GetSwapChainId();
    m_Desc = swapChainDesc;
    m_Desc.waitable = m_Device.m_IsPresentWaitSupported && m_Desc.waitable;
    m_Desc.allowLowLatency = allowLowLatency;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void SwapChainVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SURFACE_KHR, (uint64_t)m_Surface, name);
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)m_Handle, name);
}

inline Texture* const* SwapChainVK::GetTextures(uint32_t& textureNum) const {
    textureNum = (uint32_t)m_Textures.size();
    return (Texture* const*)m_Textures.data();
}

inline uint32_t SwapChainVK::AcquireNextTexture() {
    ExclusiveScope lock(m_CommandQueue->GetLock());
    const auto& vk = m_Device.GetDispatchTable();

    // Acquire next image (signal)
    VkSemaphore imageAcquiredSemaphore = m_ImageAcquiredSemaphores[m_FrameIndex];
    VkResult result = vk.AcquireNextImageKHR(m_Device, m_Handle, MsToUs(TIMEOUT_PRESENT), imageAcquiredSemaphore, VK_NULL_HANDLE, &m_TextureIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR)
        m_TextureIndex = OUT_OF_DATE; // TODO: find a better way, instead of returning an invalid index
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        REPORT_ERROR(&m_Device, "vkAcquireNextImageKHR returned %d", (int32_t)result);

    return m_TextureIndex;
}

inline Result SwapChainVK::WaitForPresent() {
    const auto& vk = m_Device.GetDispatchTable();

    if (m_Desc.waitable && GetPresentIndex(m_PresentId) != 0) {
        VkResult result = vk.WaitForPresentKHR(m_Device, m_Handle, m_PresentId - 1, MsToUs(TIMEOUT_PRESENT));
        return GetReturnCode(result);
    }

    return Result::UNSUPPORTED;
}

inline Result SwapChainVK::Present() {
    ExclusiveScope lock(m_CommandQueue->GetLock());

    if (m_TextureIndex == OUT_OF_DATE)
        return Result::OUT_OF_DATE;

    const auto& vk = m_Device.GetDispatchTable();
    VkSemaphore imageAcquiredSemaphore = m_ImageAcquiredSemaphores[m_FrameIndex];
    VkSemaphore renderingFinishedSemaphore = m_RenderingFinishedSemaphores[m_FrameIndex];

    { // Wait & Signal
        VkSemaphoreSubmitInfo waitSemaphore = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitSemaphore.semaphore = imageAcquiredSemaphore;
        waitSemaphore.stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        VkSemaphoreSubmitInfo signalSemaphore = {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalSemaphore.semaphore = renderingFinishedSemaphore;
        signalSemaphore.stageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        VkSubmitInfo2 submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = &waitSemaphore;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = &signalSemaphore;

        VkLatencySubmissionPresentIdNV presentId = {VK_STRUCTURE_TYPE_LATENCY_SUBMISSION_PRESENT_ID_NV};
        presentId.presentID = m_PresentId;
        if (m_Desc.allowLowLatency)
            submitInfo.pNext = &presentId;

        VkResult result = vk.QueueSubmit2(*m_CommandQueue, 1, &submitInfo, VK_NULL_HANDLE);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkQueueSubmit2 returned %d", (int32_t)result);
    }

    // Present (wait)
    VkResult result;
    {
        VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderingFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Handle;
        presentInfo.pImageIndices = &m_TextureIndex;

        VkPresentIdKHR presentId = {VK_STRUCTURE_TYPE_PRESENT_ID_KHR};
        presentId.swapchainCount = 1;
        presentId.pPresentIds = &m_PresentId;

        if (m_Device.m_IsPresentIdSupported)
            presentInfo.pNext = &presentId;

        if (m_Desc.allowLowLatency)
            SetLatencyMarker((LatencyMarker)VK_LATENCY_MARKER_PRESENT_START_NV);

        result = vk.QueuePresentKHR(*m_CommandQueue, &presentInfo);
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_ERROR_SURFACE_LOST_KHR)
            REPORT_ERROR(&m_Device, "vkQueuePresentKHR returned %d", (int32_t)result);

        if (m_Desc.allowLowLatency)
            SetLatencyMarker((LatencyMarker)VK_LATENCY_MARKER_PRESENT_END_NV);
    }

    m_FrameIndex = (m_FrameIndex + 1) % MAX_NUMBER_OF_FRAMES_IN_FLIGHT;
    m_PresentId++;

    return GetReturnCode(result);
}

inline Result SwapChainVK::SetLatencySleepMode(const LatencySleepMode& latencySleepMode) {
    VkLatencySleepModeInfoNV sleepModeInfo = {VK_STRUCTURE_TYPE_LATENCY_SLEEP_MODE_INFO_NV};
    sleepModeInfo.lowLatencyMode = latencySleepMode.lowLatencyMode;
    sleepModeInfo.lowLatencyBoost = latencySleepMode.lowLatencyBoost;
    sleepModeInfo.minimumIntervalUs = latencySleepMode.minIntervalUs;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.SetLatencySleepModeNV(m_Device, m_Handle, &sleepModeInfo);

    return GetReturnCode(result);
}

inline Result SwapChainVK::SetLatencyMarker(LatencyMarker latencyMarker) {
    VkSetLatencyMarkerInfoNV markerInfo = {VK_STRUCTURE_TYPE_SET_LATENCY_MARKER_INFO_NV};
    markerInfo.presentID = m_PresentId;
    markerInfo.marker = (VkLatencyMarkerNV)latencyMarker;

    const auto& vk = m_Device.GetDispatchTable();
    vk.SetLatencyMarkerNV(m_Device, m_Handle, &markerInfo);

    return Result::SUCCESS;
}

inline Result SwapChainVK::LatencySleep() {
    VkLatencySleepInfoNV sleepInfo = {VK_STRUCTURE_TYPE_LATENCY_SLEEP_INFO_NV};
    sleepInfo.signalSemaphore = *m_LatencyFence;
    sleepInfo.value = m_PresentId;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.LatencySleepNV(m_Device, m_Handle, &sleepInfo);

    if (result == VK_SUCCESS)
        m_LatencyFence->Wait(m_PresentId);

    return GetReturnCode(result);
}

inline Result SwapChainVK::GetLatencyReport(LatencyReport& latencyReport) {
    VkLatencyTimingsFrameReportNV timingsInfo[64] = {};
    for (uint32_t i = 0; i < GetCountOf(timingsInfo); i++)
        timingsInfo[i].sType = VK_STRUCTURE_TYPE_LATENCY_TIMINGS_FRAME_REPORT_NV;

    VkGetLatencyMarkerInfoNV getTimingsInfo = {VK_STRUCTURE_TYPE_GET_LATENCY_MARKER_INFO_NV};
    getTimingsInfo.pTimings = timingsInfo;
    getTimingsInfo.timingCount = GetCountOf(timingsInfo);

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetLatencyTimingsNV(m_Device, m_Handle, &getTimingsInfo);

    latencyReport = {};
    if (getTimingsInfo.timingCount >= 64) {
        const uint32_t i = 63;
        latencyReport.inputSampleTimeUs = timingsInfo[i].inputSampleTimeUs;
        latencyReport.simulationStartTimeUs = timingsInfo[i].simStartTimeUs;
        latencyReport.simulationEndTimeUs = timingsInfo[i].simEndTimeUs;
        latencyReport.renderSubmitStartTimeUs = timingsInfo[i].renderSubmitStartTimeUs;
        latencyReport.renderSubmitEndTimeUs = timingsInfo[i].renderSubmitEndTimeUs;
        latencyReport.presentStartTimeUs = timingsInfo[i].presentStartTimeUs;
        latencyReport.presentEndTimeUs = timingsInfo[i].presentEndTimeUs;
        latencyReport.driverStartTimeUs = timingsInfo[i].driverStartTimeUs;
        latencyReport.driverEndTimeUs = timingsInfo[i].driverEndTimeUs;
        latencyReport.osRenderQueueStartTimeUs = timingsInfo[i].osRenderQueueStartTimeUs;
        latencyReport.osRenderQueueEndTimeUs = timingsInfo[i].osRenderQueueEndTimeUs;
        latencyReport.gpuRenderStartTimeUs = timingsInfo[i].gpuRenderStartTimeUs;
        latencyReport.gpuRenderEndTimeUs = timingsInfo[i].gpuRenderEndTimeUs;
    }

    return Result::SUCCESS;
}

#include "SwapChainVK.hpp"
