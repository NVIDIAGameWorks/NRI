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
void FillTextureDesc(const T& textureViewDesc, DescriptorTextureDesc& descriptorTextureDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    const Mip_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;
    const Dim_t remainingLayers = textureDesc.layerNum - descriptorTextureDesc.layerOffset;

    descriptorTextureDesc.texture = &texture;
    descriptorTextureDesc.layout = ::GetImageLayoutForView(textureViewDesc.viewType);
    descriptorTextureDesc.aspectFlags = ::GetImageAspectFlags(textureViewDesc.format);
    descriptorTextureDesc.layerOffset = textureViewDesc.layerOffset;
    descriptorTextureDesc.layerNum = (textureViewDesc.layerNum == REMAINING_LAYERS) ? remainingLayers : textureViewDesc.layerNum;
    descriptorTextureDesc.mipOffset = textureViewDesc.mipOffset;
    descriptorTextureDesc.mipNum = (textureViewDesc.mipNum == REMAINING_MIPS) ? remainingMips : textureViewDesc.mipNum;
}

template <>
void FillTextureDesc(const Texture3DViewDesc& textureViewDesc, DescriptorTextureDesc& descriptorTextureDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;
    const TextureDesc& textureDesc = texture.GetDesc();
    const Mip_t remainingMips = textureDesc.mipNum - textureViewDesc.mipOffset;

    descriptorTextureDesc.texture = &texture;
    descriptorTextureDesc.layout = ::GetImageLayoutForView(textureViewDesc.viewType);
    descriptorTextureDesc.aspectFlags = ::GetImageAspectFlags(textureViewDesc.format);
    descriptorTextureDesc.layerOffset = 0;
    descriptorTextureDesc.layerNum = 1;
    descriptorTextureDesc.mipOffset = textureViewDesc.mipOffset;
    descriptorTextureDesc.mipNum = (textureViewDesc.mipNum == REMAINING_MIPS) ? remainingMips : textureViewDesc.mipNum;
}

template <typename T>
void FillImageSubresourceRange(const T& textureViewDesc, VkImageSubresourceRange& subresourceRange) {
    subresourceRange = {::GetImageAspectFlags(textureViewDesc.format), textureViewDesc.mipOffset,
        (textureViewDesc.mipNum == REMAINING_MIPS) ? VK_REMAINING_MIP_LEVELS : textureViewDesc.mipNum, textureViewDesc.layerOffset,
        (textureViewDesc.layerNum == REMAINING_LAYERS) ? VK_REMAINING_ARRAY_LAYERS : textureViewDesc.layerNum};
}

template <>
void FillImageSubresourceRange(const Texture3DViewDesc& textureViewDesc, VkImageSubresourceRange& subresourceRange) {
    subresourceRange = {::GetImageAspectFlags(textureViewDesc.format), textureViewDesc.mipOffset,
        (textureViewDesc.mipNum == REMAINING_MIPS) ? VK_REMAINING_MIP_LEVELS : textureViewDesc.mipNum, 0, 1};
}

template <typename T>
Result DescriptorVK::CreateTextureView(const T& textureViewDesc) {
    const TextureVK& texture = *(const TextureVK*)textureViewDesc.texture;

    VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
    imageViewUsageCreateInfo.usage = GetImageViewUsage(textureViewDesc.viewType);

    m_Type = DescriptorTypeVK::IMAGE_VIEW;
    m_Format = ::GetVkFormat(textureViewDesc.format);
    ::FillTextureDesc(textureViewDesc, m_TextureDesc);
    m_TextureDesc.handle = texture.GetHandle();

    VkImageSubresourceRange subresource = {};
    ::FillImageSubresourceRange(textureViewDesc, subresource);

    VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewCreateInfo.pNext = &imageViewUsageCreateInfo;
    imageViewCreateInfo.viewType = GetImageViewType(textureViewDesc.viewType);
    imageViewCreateInfo.format = m_Format;
    imageViewCreateInfo.subresourceRange = subresource;
    imageViewCreateInfo.image = texture.GetHandle();

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateImageView(m_Device, &imageViewCreateInfo, m_Device.GetAllocationCallbacks(), &m_ImageView);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateImageView returned %d", (int32_t)result);

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

    VkBufferViewCreateInfo info = {
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO, nullptr, (VkBufferViewCreateFlags)0, buffer.GetHandle(), m_Format, bufferViewDesc.offset, m_BufferDesc.size};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateBufferView(m_Device, &info, m_Device.GetAllocationCallbacks(), &m_BufferView);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateBufferView returned %d", (int32_t)result);

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

Result DescriptorVK::Create(const SamplerDesc& samplerDesc) {
    m_Type = DescriptorTypeVK::SAMPLER;

    const VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, nullptr, (VkSamplerCreateFlags)0, GetFilter(samplerDesc.filters.mag),
        GetFilter(samplerDesc.filters.min), GetSamplerMipmapMode(samplerDesc.filters.mip), GetSamplerAddressMode(samplerDesc.addressModes.u),
        GetSamplerAddressMode(samplerDesc.addressModes.v), GetSamplerAddressMode(samplerDesc.addressModes.w), samplerDesc.mipBias, VkBool32(samplerDesc.anisotropy > 1.0f),
        (float)samplerDesc.anisotropy, VkBool32(samplerDesc.compareFunc != CompareFunc::NONE), GetCompareOp(samplerDesc.compareFunc), samplerDesc.mipMin, samplerDesc.mipMax,
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, false};

    const auto& vk = m_Device.GetDispatchTable();
    VkResult result = vk.CreateSampler(m_Device, &samplerInfo, m_Device.GetAllocationCallbacks(), &m_Sampler);
    RETURN_ON_FAILURE(&m_Device, result == VK_SUCCESS, GetReturnCode(result), "vkCreateSampler returned %d", (int32_t)result);

    return Result::SUCCESS;
}

Result DescriptorVK::Create(VkAccelerationStructureKHR accelerationStructure) {
    m_Type = DescriptorTypeVK::ACCELERATION_STRUCTURE;
    m_AccelerationStructure = accelerationStructure;

    return Result::SUCCESS;
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