// © 2021 NVIDIA Corporation

#include "SharedVK.h"

#include "BufferVK.h"
#include "DescriptorVK.h"
#include "TextureVK.h"

using namespace nri;

DescriptorVK::~DescriptorVK() {
    const auto& vk = m_Device.GetDispatchTable();

    switch (m_Type) {
        case DescriptorTypeVK::NONE:
        case DescriptorTypeVK::ACCELERATION_STRUCTURE:
            break;
        case DescriptorTypeVK::BUFFER_VIEW:
            if (m_BufferView)
                vk.DestroyBufferView(m_Device, m_BufferView, m_Device.GetAllocationCallbacks());
            break;
        case DescriptorTypeVK::IMAGE_VIEW:
            if (m_ImageView)
                vk.DestroyImageView(m_Device, m_ImageView, m_Device.GetAllocationCallbacks());
            break;
        case DescriptorTypeVK::SAMPLER:
            if (m_Sampler)
                vk.DestroySampler(m_Device, m_Sampler, m_Device.GetAllocationCallbacks());
            break;
    }
}

template <typename T>
Result DescriptorVK::CreateTextureView(const T& textureViewDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    Mip_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    Dim_t remainingLayers = textureDesc.layerNum - textureViewDesc.layerOffset;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    VkImageSubresourceRange subresource = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips : textureViewDesc.mipNum,
        textureViewDesc.layerOffset,
        textureViewDesc.layerNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.layerNum,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresource;
    createInfo.image = texture.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateImageView(m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &m_ImageView);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateImageView returned %d", (int32_t)result);

    m_Type = DescriptorTypeVK::IMAGE_VIEW;
    m_Format = createInfo.format;
    m_TextureDesc.handle = texture.GetHandle();
    m_TextureDesc.texture = &texture;
    m_TextureDesc.layout = GetImageLayoutForView(textureViewDesc.viewType, textureViewDesc.flags);
    m_TextureDesc.aspectFlags = GetImageAspectFlags(textureViewDesc.format);
    m_TextureDesc.layerOffset = textureViewDesc.layerOffset;
    m_TextureDesc.layerNum = (Dim_t)subresource.layerCount;
    m_TextureDesc.sliceOffset = 0;
    m_TextureDesc.sliceNum = 1;
    m_TextureDesc.mipOffset = textureViewDesc.mipOffset;
    m_TextureDesc.mipNum = (Mip_t)subresource.levelCount;

    return Result::SUCCESS;
}

template <>
Result DescriptorVK::CreateTextureView(const Texture3DViewDesc& textureViewDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    Mip_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    Dim_t remainingLayers = textureDesc.layerNum - textureViewDesc.sliceOffset;

    VkImageViewSlicedCreateInfoEXT slicesInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_SLICED_CREATE_INFO_EXT};
    slicesInfo.sliceOffset = textureViewDesc.sliceOffset;
    slicesInfo.sliceCount = textureViewDesc.sliceNum == REMAINING_LAYERS ? remainingLayers : textureViewDesc.sliceNum;

    VkImageViewUsageCreateInfo usageInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    usageInfo.usage = GetImageViewUsage(textureViewDesc.viewType);
    
    if (m_Device.m_IsImageSlicedViewSupported)
        usageInfo.pNext = &slicesInfo;

    VkImageSubresourceRange subresource = {
        GetImageAspectFlags(textureViewDesc.format),
        textureViewDesc.mipOffset,
        textureViewDesc.mipNum == REMAINING_MIPS ? remainingMips : textureViewDesc.mipNum,
        0,
        1,
    };

    VkImageViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    createInfo.pNext = &usageInfo;
    createInfo.viewType = GetImageViewType(textureViewDesc.viewType);
    createInfo.format = GetVkFormat(textureViewDesc.format);
    createInfo.subresourceRange = subresource;
    createInfo.image = texture.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateImageView(m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &m_ImageView);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateImageView returned %d", (int32_t)result);

    m_Type = DescriptorTypeVK::IMAGE_VIEW;
    m_Format = createInfo.format;
    m_TextureDesc.handle = texture.GetHandle();
    m_TextureDesc.texture = &texture;
    m_TextureDesc.layout = GetImageLayoutForView(textureViewDesc.viewType);
    m_TextureDesc.aspectFlags = GetImageAspectFlags(textureViewDesc.format);
    m_TextureDesc.layerOffset = 0;
    m_TextureDesc.layerNum = 1;
    m_TextureDesc.sliceOffset = textureViewDesc.sliceOffset;
    m_TextureDesc.sliceNum = (Dim_t)slicesInfo.sliceCount;
    m_TextureDesc.mipOffset = textureViewDesc.mipOffset;
    m_TextureDesc.mipNum = (Mip_t)subresource.levelCount;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const BufferViewDesc& bufferViewDesc) {
    const BufferVK& buffer = *(const BufferVK*)bufferViewDesc.buffer;

    m_Type = DescriptorTypeVK::BUFFER_VIEW;
    m_Format = GetVkFormat(bufferViewDesc.format);
    m_BufferDesc.offset = bufferViewDesc.offset;
    m_BufferDesc.size = (bufferViewDesc.size == WHOLE_SIZE) ? VK_WHOLE_SIZE : bufferViewDesc.size;
    m_BufferDesc.handle = buffer.GetHandle();

    if (bufferViewDesc.format == Format::UNKNOWN)
        return Result::SUCCESS;

    VkBufferViewCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO};
    createInfo.flags = (VkBufferViewCreateFlags)0;
    createInfo.buffer = buffer.GetHandle();
    createInfo.format = m_Format;
    createInfo.offset = bufferViewDesc.offset;
    createInfo.range = m_BufferDesc.size;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateBufferView(m_Device, &createInfo, m_Device.GetAllocationCallbacks(), &m_BufferView);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateBufferView returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const SamplerDesc& samplerDesc) {
    VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    info.pNext = nullptr;
    info.flags = (VkSamplerCreateFlags)0;
    info.magFilter = GetFilter(samplerDesc.filters.mag);
    info.minFilter = GetFilter(samplerDesc.filters.min);
    info.mipmapMode = GetSamplerMipmapMode(samplerDesc.filters.mip);
    info.addressModeU = GetSamplerAddressMode(samplerDesc.addressModes.u);
    info.addressModeV = GetSamplerAddressMode(samplerDesc.addressModes.v);
    info.addressModeW = GetSamplerAddressMode(samplerDesc.addressModes.w);
    info.mipLodBias = samplerDesc.mipBias;
    info.anisotropyEnable = VkBool32(samplerDesc.anisotropy > 1.0f);
    info.maxAnisotropy = (float)samplerDesc.anisotropy;
    info.compareEnable = VkBool32(samplerDesc.compareFunc != CompareFunc::NONE);
    info.compareOp = GetCompareOp(samplerDesc.compareFunc);
    info.minLod = samplerDesc.mipMin;
    info.maxLod = samplerDesc.mipMax;
    info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    info.unnormalizedCoordinates = false;

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateSampler(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_Sampler);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSampler returned %d", (int32_t)result);

    m_Type = DescriptorTypeVK::SAMPLER;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(VkAccelerationStructureKHR accelerationStructure) {
    m_AccelerationStructure = accelerationStructure;
    m_Type = DescriptorTypeVK::ACCELERATION_STRUCTURE;

    return Result::SUCCESS;
}

Result DescriptorVK::Create(const Texture1DViewDesc& textureViewDesc) {
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const Texture2DViewDesc& textureViewDesc) {
    return CreateTextureView(textureViewDesc);
}

Result DescriptorVK::Create(const Texture3DViewDesc& textureViewDesc) {
    return CreateTextureView(textureViewDesc);
}

//================================================================================================================
// NRI
//================================================================================================================

inline void DescriptorVK::SetDebugName(const char* name) {
    switch (m_Type) {
        case DescriptorTypeVK::BUFFER_VIEW:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_BUFFER_VIEW, (uint64_t)m_BufferView, name);
            break;

        case DescriptorTypeVK::IMAGE_VIEW:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_ImageView, name);
            break;

        case DescriptorTypeVK::SAMPLER:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_SAMPLER, (uint64_t)m_Sampler, name);
            break;

        case DescriptorTypeVK::ACCELERATION_STRUCTURE:
            m_Device.SetDebugNameToTrivialObject(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, (uint64_t)m_AccelerationStructure, name);
            break;

        default:
            CHECK(false, "unexpected descriptor type");
            break;
    }
}

#include "DescriptorVK.hpp"