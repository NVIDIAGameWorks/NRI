// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;
struct TextureVK;

enum class DescriptorTypeVK {
    NONE = 0,
    BUFFER_VIEW,
    IMAGE_VIEW,
    SAMPLER,
    ACCELERATION_STRUCTURE
};

struct DescriptorBufDesc {
    VkBuffer handle;
    uint64_t offset;
    uint64_t size;
};

struct DescriptorTexDesc {
    VkImage handle;
    const TextureVK* texture;
    VkImageLayout layout;
    VkImageAspectFlags aspectFlags;
    Dim_t layerOffset;
    Dim_t layerNum;
    Dim_t sliceOffset;
    Dim_t sliceNum;
    Mip_t mipOffset;
    Mip_t mipNum;
};

struct DescriptorVK {
    inline DescriptorVK(DeviceVK& device)
        : m_Device(device) {
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkBufferView GetBufferView() const {
        return m_BufferView;
    }

    inline VkImageView GetImageView() const {
        return m_ImageView;
    }

    inline const VkSampler& GetSampler() const {
        return m_Sampler;
    }

    inline VkAccelerationStructureKHR GetAccelerationStructure() const {
        return m_AccelerationStructure;
    }

    inline VkBuffer GetBuffer() const {
        return m_BufferDesc.handle;
    }

    inline VkImage GetImage() const {
        return m_TextureDesc.handle;
    }

    inline const TextureVK& GetTexture() const {
        return *m_TextureDesc.texture;
    }

    inline DescriptorTypeVK GetType() const {
        return m_Type;
    }

    inline VkFormat GetFormat() const {
        return m_Format;
    }

    inline const DescriptorTexDesc& GetTexDesc() const {
        return m_TextureDesc;
    }

    inline const DescriptorBufDesc& GetBufDesc() const {
        return m_BufferDesc;
    }

    inline bool IsDepthWritable() const {
        return m_TextureDesc.layout != VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL && m_TextureDesc.layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    inline bool IsStencilWritable() const {
        return m_TextureDesc.layout != VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL && m_TextureDesc.layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }

    inline void GetBufferInfo(VkDescriptorBufferInfo& info) const {
        info.buffer = m_BufferDesc.handle;
        info.offset = m_BufferDesc.offset;
        info.range = m_BufferDesc.size;
    }

    inline void GetImageSubresourceRange(VkImageSubresourceRange& range) const {
        range.aspectMask = m_TextureDesc.aspectFlags;
        range.baseMipLevel = m_TextureDesc.mipOffset;
        range.levelCount = m_TextureDesc.mipNum;
        range.baseArrayLayer = m_TextureDesc.layerOffset;
        range.layerCount = m_TextureDesc.layerNum;
    }

    ~DescriptorVK();

    Result Create(const BufferViewDesc& bufferViewDesc);
    Result Create(const Texture1DViewDesc& textureViewDesc);
    Result Create(const Texture2DViewDesc& textureViewDesc);
    Result Create(const Texture3DViewDesc& textureViewDesc);
    Result Create(const SamplerDesc& samplerDesc);
    Result Create(VkAccelerationStructureKHR accelerationStructure);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);

private:
    template <typename T>
    Result CreateTextureView(const T& textureViewDesc);

private:
    DeviceVK& m_Device;

    union {
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkBufferView m_BufferView;
        VkAccelerationStructureKHR m_AccelerationStructure;
        VkSampler m_Sampler;
    };

    union {
        DescriptorTexDesc m_TextureDesc = {};
        DescriptorBufDesc m_BufferDesc;
    };

    DescriptorTypeVK m_Type = DescriptorTypeVK::NONE;
    VkFormat m_Format = VK_FORMAT_UNDEFINED;
};

} // namespace nri