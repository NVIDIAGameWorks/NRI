// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "SwapChainVK.h"
#include "CommandQueueVK.h"
#include "TextureVK.h"

using namespace nri;

static std::array<VkFormat, (size_t)SwapChainFormat::MAX_NUM> g_swapChainFormat =
{
    VK_FORMAT_R16G16B16A16_SFLOAT,              // BT709_G10_16BIT
#ifdef __APPLE__
    VK_FORMAT_B8G8R8A8_UNORM,                   // BT709_G22_8BIT
#else
    VK_FORMAT_R8G8B8A8_UNORM,                   // BT709_G22_8BIT
#endif
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,         // BT709_G22_10BIT
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,         // BT2020_G2084_10BIT
};

static std::array<VkColorSpaceKHR, (size_t)SwapChainFormat::MAX_NUM> g_colorSpace =
{
    VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,    // BT709_G10_16BIT
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,          // BT709_G22_8BIT
    VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,          // BT709_G22_10BIT
    VK_COLOR_SPACE_HDR10_ST2084_EXT,            // BT2020_G2084_10BIT
};

SwapChainVK::SwapChainVK(DeviceVK& device) :
    m_Textures(device.GetStdAllocator())
    , m_Device(device)
{}

SwapChainVK::~SwapChainVK()
{
    Destroy();
}

void SwapChainVK::Destroy()
{
    const auto& vk = m_Device.GetDispatchTable();

    for (size_t i = 0; i < m_Textures.size(); i++) {
        m_Textures[i]->ClearHandle();
        Deallocate(m_Device.GetStdAllocator(), m_Textures[i]);
    }
    m_Textures.clear();

    if (m_Handle != VK_NULL_HANDLE)
        vk.DestroySwapchainKHR(m_Device, m_Handle, m_Device.GetAllocationCallbacks());

    if (m_Surface != VK_NULL_HANDLE)
        vk.DestroySurfaceKHR(m_Device, m_Surface, m_Device.GetAllocationCallbacks());

    for (VkSemaphore& semaphore : m_Semaphores)
        vk.DestroySemaphore(m_Device, semaphore, m_Device.GetAllocationCallbacks());
}

Result SwapChainVK::CreateSurface(const SwapChainDesc& swapChainDesc)
{
    const auto& vk = m_Device.GetDispatchTable();

#ifdef VK_USE_PLATFORM_WIN32_KHR
    if (swapChainDesc.window.windows.hwnd)
    {
        VkWin32SurfaceCreateInfoKHR win32SurfaceInfo = {};
        win32SurfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        win32SurfaceInfo.hwnd = (HWND)swapChainDesc.window.windows.hwnd;

        VkResult result = vk.CreateWin32SurfaceKHR(m_Device, &win32SurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateWin32SurfaceKHR returned %d", (int32_t)result);

        return Result::SUCCESS;
    }
#endif
#ifdef VK_USE_PLATFORM_METAL_EXT
    if (swapChainDesc.window.metal.caMetalLayer)
    {
        VkMetalSurfaceCreateInfoEXT metalSurfaceCreateInfo = {};
        metalSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
        metalSurfaceCreateInfo.pLayer = (CAMetalLayer*)swapChainDesc.window.metal.caMetalLayer;

        VkResult result = vk.CreateMetalSurfaceEXT(m_Device, &metalSurfaceCreateInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateMetalSurfaceEXT returned %d", (int32_t)result);

        return Result::SUCCESS;
    }
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    if (swapChainDesc.window.x11.dpy && swapChainDesc.window.x11.window)
    {
        VkXlibSurfaceCreateInfoKHR xlibSurfaceInfo = {};
        xlibSurfaceInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        xlibSurfaceInfo.dpy = (::Display*)swapChainDesc.window.x11.dpy;
        xlibSurfaceInfo.window = (::Window)swapChainDesc.window.x11.window;

        VkResult result = vk.CreateXlibSurfaceKHR(m_Device, &xlibSurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateXlibSurfaceKHR returned %d", (int32_t)result);

        return Result::SUCCESS;
    }
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    if (swapChainDesc.window.wayland.display && swapChainDesc.window.wayland.surface)
    {
        VkWaylandSurfaceCreateInfoKHR waylandSurfaceInfo = {};
        waylandSurfaceInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        waylandSurfaceInfo.display = (wl_display*)swapChainDesc.window.wayland.display;
        waylandSurfaceInfo.surface = (wl_surface*)swapChainDesc.window.wayland.surface;

        VkResult result = vk.CreateWaylandSurfaceKHR(m_Device, &waylandSurfaceInfo, m_Device.GetAllocationCallbacks(), &m_Surface);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateWaylandSurfaceKHR returned %d", (int32_t)result);

        return Result::SUCCESS;
    }
#endif

    return Result::INVALID_ARGUMENT;
}

Result SwapChainVK::Create(const SwapChainDesc& swapChainDesc)
{
    const auto& vk = m_Device.GetDispatchTable();

    for (VkSemaphore& semaphore : m_Semaphores)
    {
        VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO, nullptr, VK_SEMAPHORE_TYPE_BINARY, 0 };
        VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, &timelineCreateInfo, 0 };

        VkResult result = vk.CreateSemaphore((VkDevice)m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &semaphore);
        RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSemaphore returned %d", (int32_t)result);
    }

    m_CommandQueue = (CommandQueueVK*)swapChainDesc.commandQueue;

    const Result nriResult = CreateSurface(swapChainDesc);
    if (nriResult != Result::SUCCESS)
        return nriResult;

    VkBool32 supported = VK_FALSE;
    vk.GetPhysicalDeviceSurfaceSupportKHR(m_Device, m_CommandQueue->GetFamilyIndex(), m_Surface, &supported);

    if (supported == VK_FALSE)
    {
        REPORT_ERROR(&m_Device, "The specified surface is not supported by the physical device");
        return Result::UNSUPPORTED;
    }

    VkSurfaceCapabilitiesKHR capabilites = {};
    VkResult result = vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device, m_Surface, &capabilites);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfaceCapabilitiesKHR returned %d", (int32_t)result);

    const bool isWidthValid = swapChainDesc.width >= capabilites.minImageExtent.width &&
        swapChainDesc.width <= capabilites.maxImageExtent.width;
    const bool isHeightValid = swapChainDesc.height >= capabilites.minImageExtent.height &&
        swapChainDesc.height <= capabilites.maxImageExtent.height;

    if (!isWidthValid || !isHeightValid)
    {
        REPORT_ERROR(&m_Device, "Invalid SwapChainVK buffer size");
        return Result::INVALID_ARGUMENT;
    }

    uint32_t formatNum = 0;
    result = vk.GetPhysicalDeviceSurfaceFormatsKHR(m_Device, m_Surface, &formatNum, nullptr);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfaceFormatsKHR returned %d", (int32_t)result);

    VkSurfaceFormatKHR* surfaceFormats = STACK_ALLOC(VkSurfaceFormatKHR, formatNum);
    result = vk.GetPhysicalDeviceSurfaceFormatsKHR(m_Device, m_Surface, &formatNum, surfaceFormats);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfaceFormatsKHR returned %d", (int32_t)result);

    VkFormat format = g_swapChainFormat[(uint32_t)swapChainDesc.format];
    VkColorSpaceKHR colorSpace = g_colorSpace[(uint32_t)swapChainDesc.format];

    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
    uint32_t i = 0;
    for (; i < formatNum; i++)
    {
        if (surfaceFormats[i].format == format && surfaceFormats[i].colorSpace == colorSpace)
        {
            surfaceFormat = surfaceFormats[i];
            break;
        }
    }
    if (i == formatNum)
        REPORT_WARNING(&m_Device, "The requested format is not supported. Using 1st surface format from the list");

    uint32_t presentModeNum = 0;
    result = vk.GetPhysicalDeviceSurfacePresentModesKHR(m_Device, m_Surface, &presentModeNum, nullptr);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfacePresentModesKHR returned %d", (int32_t)result);

    VkPresentModeKHR* presentModes = STACK_ALLOC(VkPresentModeKHR, presentModeNum);
    result = vk.GetPhysicalDeviceSurfacePresentModesKHR(m_Device, m_Surface, &presentModeNum, presentModes);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkGetPhysicalDeviceSurfacePresentModesKHR returned %d", (int32_t)result);

    // Both of these modes use v-sync for preseting, but FIFO blocks execution
    VkPresentModeKHR desiredPresentMode = swapChainDesc.verticalSyncInterval ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

    i = 0;
    for (; i < presentModeNum; i++)
    {
        if (desiredPresentMode == presentModes[i])
            break;
    }
    if (i == presentModeNum)
    {
        REPORT_WARNING(&m_Device, "The present mode is not supported. Using the first mode from the list");
        desiredPresentMode = presentModes[0];
    }

    const uint32_t familyIndex = m_CommandQueue->GetFamilyIndex();
    const uint32_t minImageNum = std::max<uint32_t>(capabilites.minImageCount, swapChainDesc.textureNum);

    const VkSwapchainCreateInfoKHR swapchainInfo = {
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        nullptr,
        (VkSwapchainCreateFlagsKHR)0,
        m_Surface,
        minImageNum,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        { swapChainDesc.width, swapChainDesc.height },
        1,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        1,
        &familyIndex,
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        desiredPresentMode,
        VK_FALSE,
        VK_NULL_HANDLE
    };

    result = vk.CreateSwapchainKHR(m_Device, &swapchainInfo, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSwapchainKHR returned %d", (int32_t)result);

    uint32_t imageNum = 0;
    vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, nullptr);

    VkImage* imageHandles = STACK_ALLOC(VkImage, imageNum);
    vk.GetSwapchainImagesKHR(m_Device, m_Handle, &imageNum, imageHandles);

    m_Textures.resize(imageNum);
    for (i = 0; i < imageNum; i++)
    {
        TextureVKDesc desc = {};
        desc.vkImage = (NRIVkImage)imageHandles[i];
        desc.vkFormat = surfaceFormat.format;
        desc.vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        desc.vkImageType = VK_IMAGE_TYPE_2D;
        desc.width = (uint16_t)swapchainInfo.imageExtent.width;
        desc.height = (uint16_t)swapchainInfo.imageExtent.height;
        desc.depth = 1;
        desc.mipNum = 1;
        desc.arraySize = 1;
        desc.sampleNum = 1;
        desc.nodeMask = 1; // TODO: or ALL_NODES?

        TextureVK* texture = Allocate<TextureVK>(m_Device.GetStdAllocator(), m_Device);
        texture->Create(desc);

        m_Textures[i] = texture;
    }

    m_SwapChainDesc = swapChainDesc;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void SwapChainVK::SetDebugName(const char* name)
{
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SURFACE_KHR, (uint64_t)m_Surface, name);
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)m_Handle, name);
}

inline Texture* const* SwapChainVK::GetTextures(uint32_t& textureNum) const
{
    textureNum = (uint32_t)m_Textures.size();
    return (Texture* const*)m_Textures.data();
}

inline uint32_t SwapChainVK::AcquireNextTexture()
{
    ExclusiveScope lock(m_CommandQueue->GetLock());

    const auto& vk = m_Device.GetDispatchTable();

    VkSemaphore semaphore = m_Semaphores[m_FrameIndex];
    VkResult result = vk.AcquireNextImageKHR(m_Device, m_Handle, VK_DEFAULT_TIMEOUT, semaphore, VK_NULL_HANDLE, &m_TextureIndex);

    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)
    {
        /*
        TODO: currently we acquire swap chain image this way:
            SwapChainVK::AcquireNextTexture
                AcquireNextImageKHR(semaphore) - signal
                QueueSubmit(semaphore) - wait
            SwapChainVK::Present()
                QueueSubmit(semaphore) - signal
                QueuePresentKHR(semaphore) - wait
        Would it be better to use 2 semaphores?
            SwapChainVK::AcquireNextTexture
                AcquireNextImageKHR(imageAcquiredSemaphore) - signal
            SwapChainVK::Present()
                QueueSubmit(imageAcquiredSemaphore) - wait
                QueueSubmit(renderingFinishedSemaphore) - signal
                QueuePresentKHR(renderingFinishedSemaphore) - wait
        */
        const uint32_t waitDstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 1, &semaphore, &waitDstStageMask, 0, nullptr, 0, nullptr };
        result = vk.QueueSubmit(*m_CommandQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
            REPORT_ERROR(&m_Device, "vkQueueSubmit returned %d", (int32_t)result);
    }
    else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_SURFACE_LOST_KHR) // TODO: find a better way, instead of returning an invalid index
        m_TextureIndex = OUT_OF_DATE;
    else
        REPORT_ERROR(&m_Device, "vkAcquireNextImageKHR returned %d", (int32_t)result);

    return m_TextureIndex;
}

inline Result SwapChainVK::Present()
{
    ExclusiveScope lock(m_CommandQueue->GetLock());

    if (m_TextureIndex == OUT_OF_DATE)
        return Result::OUT_OF_DATE;

    const auto& vk = m_Device.GetDispatchTable();

    VkSemaphore semaphore = m_Semaphores[m_FrameIndex];
    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr, 0, nullptr, nullptr, 0, nullptr, 1, &semaphore };
    VkResult result = vk.QueueSubmit(*m_CommandQueue, 1, &submitInfo, VK_NULL_HANDLE);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkQueueSubmit returned %d", (int32_t)result);

    const VkPresentInfoKHR info = {
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        1, &semaphore,
        1, &m_Handle,
        &m_TextureIndex,
        nullptr
    };

    result = vk.QueuePresentKHR(*m_CommandQueue, &info);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR && result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_ERROR_SURFACE_LOST_KHR)
        REPORT_ERROR(&m_Device, "vkQueuePresentKHR returned %d", (int32_t)result);

    m_FrameIndex = (m_FrameIndex + 1) % MAX_NUMBER_OF_FRAMES_IN_FLIGHT;

    return GetReturnCode(result);
}

#include "SwapChainVK.hpp"
