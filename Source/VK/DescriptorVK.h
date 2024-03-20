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

struct DescriptorBufferDesc {
    VkBuffer handle;
    uint64_t offset;
    uint64_t size;
};

struct DescriptorTextureDesc {
    VkImage handle;
    const TextureVK* texture;
    VkImageLayout layout;
    VkImageAspectFlags aspectFlags;
    Dim_t arrayOffset;
    Dim_t arraySize;
    Mip_t mipOffset;
    Mip_t mipNum;
};

struct DescriptorVK {
    inline DescriptorVK(DeviceVK& device) : m_Device(device) {
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

    inline VkImageLayout GetImageLayout() const {
        return m_TextureDesc.layout;
    }

    inline const DescriptorTextureDesc& GetTextureDesc() const {
        return m_TextureDesc;
    }

    inline const DescriptorBufferDesc& GetBufferDesc() const {
        return m_BufferDesc;
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
        range.baseArrayLayer = m_TextureDesc.arrayOffset;
        range.layerCount = m_TextureDesc.arraySize;
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
        VkBufferView m_BufferView = VK_NULL_HANDLE;
        VkImageView m_ImageView;
        VkAccelerationStructureKHR m_AccelerationStructure;
        VkSampler m_Sampler;
    };
    union {
        DescriptorBufferDesc m_BufferDesc;
        DescriptorTextureDesc m_TextureDesc = {};
    };
    DescriptorTypeVK m_Type = DescriptorTypeVK::NONE;
    VkFormat m_Format = VK_FORMAT_UNDEFINED;
};

} // namespace nri