// © 2021 NVIDIA Corporation

#pragma once

namespace nri
{

struct DeviceVK;
struct TextureVK;

enum class DescriptorTypeVK
{
    NONE = 0,
    BUFFER_VIEW,
    IMAGE_VIEW,
    SAMPLER,
    ACCELERATION_STRUCTURE
};

struct DescriptorBufferDesc
{
    std::array<VkBuffer, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    uint64_t offset;
    uint64_t size;
};

struct DescriptorTextureDesc
{
    std::array<VkImage, PHYSICAL_DEVICE_GROUP_MAX_SIZE> handles;
    const TextureVK* texture;
    VkImageLayout layout;
    VkImageAspectFlags aspectFlags;
    Dim_t arrayOffset;
    Dim_t arraySize;
    Mip_t mipOffset;
    Mip_t mipNum;
};

struct DescriptorVK
{
    inline DescriptorVK(DeviceVK& device) :
        m_Device(device)
    { m_BufferViews.fill(VK_NULL_HANDLE); }

    inline DeviceVK& GetDevice() const
    { return m_Device; }

    inline VkBufferView GetBufferView(uint32_t nodeIndex) const
    { return m_BufferViews[nodeIndex]; }

    inline VkImageView GetImageView(uint32_t nodeIndex) const
    { return m_ImageViews[nodeIndex]; }

    inline const VkSampler& GetSampler() const
    { return m_Sampler; }

    inline VkAccelerationStructureKHR GetAccelerationStructure(uint32_t nodeIndex) const
    { return m_AccelerationStructures[nodeIndex]; }

    inline VkBuffer GetBuffer(uint32_t nodeIndex) const
    { return m_BufferDesc.handles[nodeIndex]; }

    inline VkImage GetImage(uint32_t nodeIndex) const
    { return m_TextureDesc.handles[nodeIndex]; }

    inline const TextureVK& GetTexture() const
    { return *m_TextureDesc.texture; }

    inline DescriptorTypeVK GetType() const
    { return m_Type; }

    inline VkFormat GetFormat() const
    { return m_Format; }

    inline VkImageLayout GetImageLayout() const
    { return m_TextureDesc.layout; }

    inline const DescriptorTextureDesc& GetTextureDesc() const
    { return m_TextureDesc; }

    inline const DescriptorBufferDesc& GetBufferDesc() const
    { return m_BufferDesc; }

    inline void GetBufferInfo(uint32_t nodeIndex, VkDescriptorBufferInfo& info) const
    {
        info.buffer = m_BufferDesc.handles[nodeIndex];
        info.offset = m_BufferDesc.offset;
        info.range = m_BufferDesc.size;
    }

    inline void GetImageSubresourceRange(VkImageSubresourceRange& range) const
    {
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
    Result Create(const VkAccelerationStructureKHR* accelerationStructures, uint32_t nodeMask);

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);

private:
    template<typename T>
    Result CreateTextureView(const T& textureViewDesc);

private:
    DeviceVK& m_Device;
    union
    {
        std::array<VkBufferView, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_BufferViews;
        std::array<VkImageView, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_ImageViews;
        std::array<VkAccelerationStructureKHR, PHYSICAL_DEVICE_GROUP_MAX_SIZE> m_AccelerationStructures;
        VkSampler m_Sampler;
    };
    union
    {
        DescriptorBufferDesc m_BufferDesc;
        DescriptorTextureDesc m_TextureDesc = {};
    };
    DescriptorTypeVK m_Type = DescriptorTypeVK::NONE;
    VkFormat m_Format = VK_FORMAT_UNDEFINED;
};

}