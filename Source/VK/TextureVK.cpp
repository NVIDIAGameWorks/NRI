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

    VkImageCreateInfo info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    m_Device.FillCreateInfo(textureDesc, info);

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

#include "TextureVK.hpp"
