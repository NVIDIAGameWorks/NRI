// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "CommandQueueVK.h"
#include "TextureVK.h"

using namespace nri;

TextureVK::~TextureVK() {
    const auto& vk = m_Device.GetDispatchTable();

    if (m_OwnsNativeObjects)
        vk.DestroyImage(m_Device, m_Handle, m_Device.GetAllocationCallbacks());
}

Result TextureVK::Create(const TextureDesc& textureDesc) {
    m_OwnsNativeObjects = true;
    m_ImageAspectFlags = ::GetImageAspectFlags(textureDesc.format);
    m_Desc = textureDesc;

    const VkImageType imageType = ::GetImageType(textureDesc.type);
    const VkSharingMode sharingMode = m_Device.IsConcurrentSharingModeEnabledForImages() ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
    const Vector<uint32_t>& queueIndices = m_Device.GetConcurrentSharingModeQueueIndices();

   VkImageCreateFlags flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT; // typeless
    const FormatProps& formatProps = GetFormatProps(textureDesc.format);
    if (formatProps.blockWidth > 1)
        flags |= VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT; // format can be used to create a view with an uncompressed format (1 texel covers 1 block)
    if (textureDesc.arraySize >= 6 && textureDesc.width == textureDesc.height)
        flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // allow cube maps
    if (textureDesc.type == nri::TextureType::TEXTURE_3D)
        flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT; // allow 3D demotion to a set of layers // TODO: hook up "VK_EXT_image_2d_view_of_3d"?
    if (m_Device.GetDesc().isProgrammableSampleLocationsSupported && textureDesc.format >= Format::D16_UNORM)
        flags |= VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT;

    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.flags = flags;
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
    VkResult result = vk.CreateImage(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Handle);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateImage returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result TextureVK::Create(const TextureVKDesc& textureDesc) {
    if (!textureDesc.vkImage)
        return Result::INVALID_ARGUMENT;

    m_OwnsNativeObjects = false;
    m_ImageAspectFlags = (VkImageAspectFlags)textureDesc.vkImageAspectFlags;
    m_Desc.type = GetTextureType((VkImageType)textureDesc.vkImageType);
    m_Desc.usageMask = (TextureUsageBits)(-1); // TODO: it's not right...
    m_Desc.format = VKFormatToNRIFormat((VkFormat)textureDesc.vkFormat);
    m_Desc.width = textureDesc.width;
    m_Desc.height = textureDesc.height;
    m_Desc.depth = textureDesc.depth;
    m_Desc.mipNum = textureDesc.mipNum;
    m_Desc.arraySize = textureDesc.arraySize;
    m_Desc.sampleNum = textureDesc.sampleNum;

    m_Handle = (VkImage)textureDesc.vkImage;

    return Result::SUCCESS;
}

//================================================================================================================
// NRI
//================================================================================================================

inline void TextureVK::SetDebugName(const char* name) {
    m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_Handle, name);
}

void TextureVK::GetMemoryInfo(MemoryLocation memoryLocation, MemoryDesc& memoryDesc) const {
    VkMemoryDedicatedRequirements dedicatedRequirements = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS, nullptr};
    VkMemoryRequirements2 requirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, &dedicatedRequirements};
    VkImageMemoryRequirementsInfo2 info = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, nullptr, m_Handle};

    const auto& vk = m_Device.GetDispatchTable();
    vk.GetImageMemoryRequirements2(m_Device, &info, &requirements);

    memoryDesc.mustBeDedicated = dedicatedRequirements.prefersDedicatedAllocation || dedicatedRequirements.requiresDedicatedAllocation;
    memoryDesc.alignment = (uint32_t)requirements.memoryRequirements.alignment;
    memoryDesc.size = requirements.memoryRequirements.size;

    MemoryTypeUnpack unpack = {};
    bool found = m_Device.GetMemoryType(memoryLocation, requirements.memoryRequirements.memoryTypeBits, unpack.info);
    RETURN_ON_FAILURE(&m_Device, found, ReturnVoid(), "Can't find suitable memory type");

    unpack.info.isDedicated = dedicatedRequirements.requiresDedicatedAllocation;

    memoryDesc.type = unpack.type;
}

#include "TextureVK.hpp"
