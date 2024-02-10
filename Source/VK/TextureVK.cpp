// © 2021 NVIDIA Corporation

#include "SharedVK.h"
#include "TextureVK.h"
#include "CommandQueueVK.h"

using namespace nri;

TextureVK::~TextureVK()
{
    const auto& vk = m_Device.GetDispatchTable();

    if (!m_OwnsNativeObjects)
        return;

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if (m_Handles[i] != VK_NULL_HANDLE)
            vk.DestroyImage(m_Device, m_Handles[i], m_Device.GetAllocationCallbacks());
    }
}

Result TextureVK::Create(const TextureDesc& textureDesc)
{
    m_OwnsNativeObjects = true;
    m_ImageAspectFlags = ::GetImageAspectFlags(textureDesc.format);
    m_Desc = textureDesc;

    const VkImageType imageType = ::GetImageType(textureDesc.type);
    const VkSharingMode sharingMode = m_Device.IsConcurrentSharingModeEnabledForImages() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    const Vector<uint32_t>& queueIndices = m_Device.GetConcurrentSharingModeQueueIndices();
    uint32_t nodeMask = GetNodeMask(textureDesc.nodeMask);

    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.flags = GetImageCreateFlags(textureDesc.format);
    info.imageType = imageType;
    info.format = ::GetVkFormat(textureDesc.format);
    info.extent.width = textureDesc.width;
    info.extent.height = textureDesc.height;
    info.extent.depth = textureDesc.depth;
    info.mipLevels = textureDesc.mipNum;
    info.arrayLayers = textureDesc.arraySize;
    info.samples = (VkSampleCountFlagBits)textureDesc.sampleNum;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = GetImageUsageFlags(textureDesc.usageMask);
    info.sharingMode = sharingMode;
    info.queueFamilyIndexCount = (uint32_t)queueIndices.size();
    info.pQueueFamilyIndices = queueIndices.data();
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    const auto& vk = m_Device.GetDispatchTable();
    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
        {
            const VkResult result = vk.CreateImage(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handles[i]);

            RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result),
                "Can't create a texture: vkCreateImage returned %d.", (int32_t)result);
        }
    }

    return Result::SUCCESS;
}

Result TextureVK::Create(const TextureVKDesc& textureDesc)
{
    if (!textureDesc.vkImage)
        return Result::INVALID_ARGUMENT;

    m_OwnsNativeObjects = false;
    m_ImageAspectFlags = (VkImageAspectFlags)textureDesc.vkImageAspectFlags;
    m_Desc.type = GetTextureType((VkImageType)textureDesc.vkImageType);
    m_Desc.usageMask = (nri::TextureUsageBits)(-1); // TODO: it's not right...
    m_Desc.format = VKFormatToNRIFormat((VkFormat)textureDesc.vkFormat);
    m_Desc.width = textureDesc.width;
    m_Desc.height = textureDesc.height;
    m_Desc.depth = textureDesc.depth;
    m_Desc.mipNum = textureDesc.mipNum;
    m_Desc.arraySize = textureDesc.arraySize;
    m_Desc.sampleNum = textureDesc.sampleNum;
    m_Desc.nodeMask = textureDesc.nodeMask;

    const VkImage handle = (VkImage)textureDesc.vkImage;
    const uint32_t nodeMask = GetNodeMask(textureDesc.nodeMask);

    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize(); i++)
    {
        if ((1 << i) & nodeMask)
            m_Handles[i] = handle;
    }

    return Result::SUCCESS;
}

Dim_t TextureVK::GetSize(Dim_t dimensionIndex, Mip_t mip) const
{
    assert(dimensionIndex < 3);

    Dim_t dim = m_Desc.depth;
    if (dimensionIndex == 0)
        dim = m_Desc.width;
    else if (dimensionIndex == 1)
        dim = m_Desc.height;

    dim = (Dim_t)std::max(dim >> mip, 1);

    // TODO: VK doesn't require manual alignment, but probably we should use it here and during texture creation
    //dim = Align(dim, dimension < 2 ? GetFormatProps(m_Desc.format).blockWidth : 1);

    return dim;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void TextureVK::SetDebugName(const char* name)
{
    std::array<uint64_t, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    for (size_t i = 0; i < handles.size(); i++)
        handles[i] = (uint64_t)m_Handles[i];

    m_Device.SetDebugNameToDeviceGroupObject(VK_OBJECT_TYPE_IMAGE, handles.data(), name);
}

void TextureVK::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const
{
    VkImage handle = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < m_Device.GetPhysicalDeviceGroupSize() && handle == VK_NULL_HANDLE; i++)
        handle = m_Handles[i];

    const auto& vk = m_Device.GetDispatchTable();

    VkMemoryDedicatedRequirements dedicatedRequirements = {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS,
        nullptr
    };

    VkMemoryRequirements2 requirements = {
        VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        &dedicatedRequirements
    };

    VkImageMemoryRequirementsInfo2 info = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
        nullptr,
        handle
    };

    vk.GetImageMemoryRequirements2(m_Device, &info, &requirements);

    memoryDesc.mustBeDedicated = dedicatedRequirements.prefersDedicatedAllocation ||
        dedicatedRequirements.requiresDedicatedAllocation;

    memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
    memoryDesc.size = requirements.memoryRequirements.size;

    MemoryTypeUnpack unpack = {};
    const bool found = m_Device.GetMemoryType(memoryLocation, requirements.memoryRequirements.memoryTypeBits, unpack.info);
    CHECK(found, "Can't find suitable memory type");

    unpack.info.isDedicated = dedicatedRequirements.requiresDedicatedAllocation;

    memoryDesc.type = unpack.type;
}

#include "TextureVK.hpp"
