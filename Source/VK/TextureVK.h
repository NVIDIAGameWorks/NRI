// © 2021 NVIDIA Corporation

#pragma once

namespace nri {

struct DeviceVK;

struct TextureVK {
    inline TextureVK(DeviceVK& device) : m_Device(device) {
    }

    inline VkImage GetHandle() const {
        return m_Handle;
    }

    inline DeviceVK& GetDevice() const {
        return m_Device;
    }

    inline VkExtent3D GetExtent() const {
        return {m_Desc.width, m_Desc.height, m_Desc.depth};
    }

    inline const TextureDesc& GetDesc() const {
        return m_Desc;
    }

    inline Dim_t GetSize(Dim_t dimensionIndex, Mip_t mip = 0) const {
        return GetDimension(GraphicsAPI::VK, m_Desc, dimensionIndex, mip);
    }

    ~TextureVK();

    Result Create(const TextureDesc& textureDesc);
    Result Create(const TextureVKDesc& textureDesc);
    Result Create(const AllocateTextureDesc& textureDesc);
    VkImageAspectFlags GetImageAspectFlags() const;
    void DestroyVma();

    //================================================================================================================
    // NRI
    //================================================================================================================

    void SetDebugName(const char* name);

private:
    DeviceVK& m_Device;
    VkImage m_Handle = VK_NULL_HANDLE;
    TextureDesc m_Desc = {};
    VmaAllocation_T* m_VmaAllocation = nullptr;
    bool m_OwnsNativeObjects = true;
};

} // namespace nri